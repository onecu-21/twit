#include "../include/codegen.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <stdexcept>

CodeGen::CodeGen() : builder(context) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    module = std::make_unique<llvm::Module>("twit", context);
}

llvm::Type* CodeGen::getLLVMType(const std::string& type) {
    if (type == "int")   return llvm::Type::getInt32Ty(context);
    if (type == "float") return llvm::Type::getFloatTy(context);
    if (type == "bool")  return llvm::Type::getInt1Ty(context);
    if (type == "char")  return llvm::Type::getInt8Ty(context);
    if (type == "void")  return llvm::Type::getVoidTy(context);
    throw std::runtime_error("Unknown type: " + type);
}

llvm::Value* CodeGen::genExpr(ASTNode* node) {
    if (auto* n = dynamic_cast<NumberExpr*>(node)) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), n->value);
    }
    if (auto* n = dynamic_cast<IdentExpr*>(node)) {
        llvm::Value* val = namedValues[n->name];
        if (!val) throw std::runtime_error("Unknown variable: " + n->name);
        return builder.CreateLoad(llvm::Type::getInt32Ty(context), val, n->name);
    }
    if (auto* n = dynamic_cast<BinaryExpr*>(node)) {
        llvm::Value* l = genExpr(n->left.get());
        llvm::Value* r = genExpr(n->right.get());
        if (n->op == "+")  return builder.CreateAdd(l, r, "addtmp");
        if (n->op == "-")  return builder.CreateSub(l, r, "subtmp");
        if (n->op == "*")  return builder.CreateMul(l, r, "multmp");
        if (n->op == "/")  return builder.CreateSDiv(l, r, "divtmp");
        if (n->op == "==") return builder.CreateICmpEQ(l, r, "eqtmp");
        if (n->op == "!=") return builder.CreateICmpNE(l, r, "netmp");
        if (n->op == "<")  return builder.CreateICmpSLT(l, r, "lttmp");
        if (n->op == ">")  return builder.CreateICmpSGT(l, r, "gttmp");
        throw std::runtime_error("Unknown operator: " + n->op);
    }
    if (auto* n = dynamic_cast<CallExpr*>(node)) {
        llvm::Function* fn = module->getFunction(n->callee);
        if (!fn) throw std::runtime_error("Unknown function: " + n->callee);
        std::vector<llvm::Value*> args;
        for (auto& arg : n->args)
            args.push_back(genExpr(arg.get()));
        return builder.CreateCall(fn, args, "calltmp");
    }
    if (auto* n = dynamic_cast<StringExpr*>(node)) {
        return builder.CreateGlobalStringPtr(n->value);
    }
    throw std::runtime_error("Unknown expression type");
}

void CodeGen::genStmt(ASTNode* node) {
    if (auto* n = dynamic_cast<VarDeclStmt*>(node)) {
        llvm::AllocaInst* alloca = builder.CreateAlloca(
            getLLVMType(n->type), nullptr, n->name);
        namedValues[n->name] = alloca;
        if (n->init) {
            llvm::Value* val = genExpr(n->init.get());
            builder.CreateStore(val, alloca);
        }
        return;
    }
    if (auto* n = dynamic_cast<ReturnStmt*>(node)) {
        llvm::Value* val = genExpr(n->value.get());
        builder.CreateRet(val);
        return;
    }
    if (auto* n = dynamic_cast<PrintStmt*>(node)) {
        llvm::Function* printfFn = module->getFunction("printf");
        if (!printfFn) {
            llvm::FunctionType* printfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context),
                {llvm::Type::getInt8PtrTy(context)},
                true);
            printfFn = llvm::Function::Create(
                printfType, llvm::Function::ExternalLinkage, "printf", module.get());
        }
        for (auto& arg : n->args) {
            llvm::Value* val = genExpr(arg.get());
            llvm::Type* valType = val->getType();
            llvm::Value* fmt;
            if (valType->isIntegerTy(32)) {
                fmt = builder.CreateGlobalStringPtr("%d\n");
            } else if (valType->isFloatTy()) {
                fmt = builder.CreateGlobalStringPtr("%f\n");
            } else if (valType->isPointerTy()) {
                fmt = builder.CreateGlobalStringPtr("%s\n");
            } else {
                fmt = builder.CreateGlobalStringPtr("%d\n");
            }
            builder.CreateCall(printfFn, {fmt, val});
        }
        return;
    }
    if (auto* n = dynamic_cast<InputStmt*>(node)) {
        llvm::Function* scanfFn = module->getFunction("scanf");
        if (!scanfFn) {
            llvm::FunctionType* scanfType = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context),
                {llvm::Type::getInt8PtrTy(context)},
                true);
            scanfFn = llvm::Function::Create(
                scanfType, llvm::Function::ExternalLinkage, "scanf", module.get());
        }
        for (auto& varName : n->vars) {
            llvm::Value* ptr = namedValues[varName];
            if (!ptr) throw std::runtime_error("Unknown variable: " + varName);
            llvm::Value* fmt = builder.CreateGlobalStringPtr("%d");
            builder.CreateCall(scanfFn, {fmt, ptr});
        }
        return;
    }
    if (auto* n = dynamic_cast<IfStmt*>(node)) {
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond,
            llvm::ConstantInt::get(cond->getType(), 0), "ifcond");
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* thenBB  = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock* elseBB  = llvm::BasicBlock::Create(context, "else");
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, "merge");
        builder.CreateCondBr(cond, thenBB, elseBB);
        builder.SetInsertPoint(thenBB);
        for (auto& s : n->thenBody) genStmt(s.get());
        builder.CreateBr(mergeBB);
        fn->getBasicBlockList().push_back(elseBB);
        builder.SetInsertPoint(elseBB);
        for (auto& s : n->elseBody) genStmt(s.get());
        builder.CreateBr(mergeBB);
        fn->getBasicBlockList().push_back(mergeBB);
        builder.SetInsertPoint(mergeBB);
        return;
    }
    if (auto* n = dynamic_cast<WhileStmt*>(node)) {
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB  = llvm::BasicBlock::Create(context, "whilecond", fn);
        llvm::BasicBlock* bodyBB  = llvm::BasicBlock::Create(context, "whilebody");
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "whileafter");
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond,
            llvm::ConstantInt::get(cond->getType(), 0), "whilecond");
        builder.CreateCondBr(cond, bodyBB, afterBB);
        fn->getBasicBlockList().push_back(bodyBB);
        builder.SetInsertPoint(bodyBB);
        for (auto& s : n->body) genStmt(s.get());
        builder.CreateBr(condBB);
        fn->getBasicBlockList().push_back(afterBB);
        builder.SetInsertPoint(afterBB);
        return;
    }
    if (auto* n = dynamic_cast<ForStmt*>(node)) {
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        genStmt(n->init.get());
        llvm::BasicBlock* condBB  = llvm::BasicBlock::Create(context, "forcond", fn);
        llvm::BasicBlock* bodyBB  = llvm::BasicBlock::Create(context, "forbody");
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "forafter");
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond,
            llvm::ConstantInt::get(cond->getType(), 0), "forcond");
        builder.CreateCondBr(cond, bodyBB, afterBB);
        fn->getBasicBlockList().push_back(bodyBB);
        builder.SetInsertPoint(bodyBB);
        for (auto& s : n->body) genStmt(s.get());
        genExpr(n->update.get());
        builder.CreateBr(condBB);
        fn->getBasicBlockList().push_back(afterBB);
        builder.SetInsertPoint(afterBB);
        return;
    }
    if (auto* n = dynamic_cast<ExprStmt*>(node)) {
        genExpr(n->expr.get());
        return;
    }
    throw std::runtime_error("Unknown statement type");
}

void CodeGen::genFunction(FunctionDecl* fn) {
    std::vector<llvm::Type*> paramTypes;
    for (auto& p : fn->params)
        paramTypes.push_back(getLLVMType(p.first));
    llvm::FunctionType* ft = llvm::FunctionType::get(
        getLLVMType(fn->returnType), paramTypes, false);
    llvm::Function* func = llvm::Function::Create(
        ft, llvm::Function::ExternalLinkage, fn->name, module.get());
    int i = 0;
    for (auto& arg : func->args())
        arg.setName(fn->params[i++].second);
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(bb);
    namedValues.clear();
    for (auto& arg : func->args()) {
        llvm::AllocaInst* alloca = builder.CreateAlloca(
            arg.getType(), nullptr, arg.getName());
        builder.CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    for (auto& stmt : fn->body)
        genStmt(stmt.get());
    llvm::verifyFunction(*func);
}

void CodeGen::generate(Program& program) {
    for (auto& fn : program.functions)
        genFunction(fn.get());
}

void CodeGen::writeToFile(const std::string& filename) {
    std::error_code ec;
    llvm::raw_fd_ostream dest(filename, ec, llvm::sys::fs::OF_None);
    if (ec) throw std::runtime_error("Could not open file: " + ec.message());
    module->print(dest, nullptr);
}