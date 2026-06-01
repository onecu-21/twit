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
    if (type == "int")    return llvm::Type::getInt32Ty(context);
    if (type == "float")  return llvm::Type::getFloatTy(context);
    if (type == "bool")   return llvm::Type::getInt1Ty(context);
    if (type == "char")   return llvm::Type::getInt8Ty(context);
    if (type == "string") return llvm::Type::getInt8PtrTy(context);
    if (type == "void")   return llvm::Type::getVoidTy(context);
    throw std::runtime_error("Unknown type: " + type);
}

llvm::Value* CodeGen::genExpr(ASTNode* node) {
    if (auto* n = dynamic_cast<NumberExpr*>(node)) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), n->value);
    }
    if (auto* n = dynamic_cast<FloatExpr*>(node)) {
        return llvm::ConstantFP::get(llvm::Type::getFloatTy(context), n->value);
    }
    if (auto* n = dynamic_cast<BoolExpr*>(node)) {
        return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context), n->value ? 1 : 0);
    }
    if (auto* n = dynamic_cast<StringExpr*>(node)) {
        return builder.CreateGlobalStringPtr(n->value);
    }
    if (auto* n = dynamic_cast<IdentExpr*>(node)) {
        llvm::Value* val = namedValues[n->name];
        if (!val) throw std::runtime_error("Unknown variable: " + n->name);
        llvm::Type* type = val->getType()->getPointerElementType();
        return builder.CreateLoad(type, val, n->name);
    }
    if (auto* n = dynamic_cast<AssignExpr*>(node)) {
        llvm::Value* ptr = namedValues[n->name];
        if (!ptr) throw std::runtime_error("Unknown variable: " + n->name);
        llvm::Type* type = ptr->getType()->getPointerElementType();
        llvm::Value* val = genExpr(n->value.get());
        if (n->op == "=") {
            builder.CreateStore(val, ptr);
        } else {
            llvm::Value* cur = builder.CreateLoad(type, ptr, n->name);
            llvm::Value* result;
            if (n->op == "+=") result = builder.CreateAdd(cur, val, "addtmp");
            else if (n->op == "-=") result = builder.CreateSub(cur, val, "subtmp");
            else if (n->op == "*=") result = builder.CreateMul(cur, val, "multmp");
            else if (n->op == "/=") result = builder.CreateSDiv(cur, val, "divtmp");
            else throw std::runtime_error("Unknown assign op: " + n->op);
            builder.CreateStore(result, ptr);
        }
        return val;
    }
    if (auto* n = dynamic_cast<UnaryExpr*>(node)) {
        if (n->op == "-") {
            llvm::Value* val = genExpr(n->operand.get());
            return builder.CreateNeg(val, "negtmp");
        }
        if (n->op == "!") {
            llvm::Value* val = genExpr(n->operand.get());
            return builder.CreateNot(val, "nottmp");
        }
        if (n->op == "++" || n->op == "--") {
            auto* ident = dynamic_cast<IdentExpr*>(n->operand.get());
            if (!ident) throw std::runtime_error("++ / -- requires variable");
            llvm::Value* ptr = namedValues[ident->name];
            llvm::Value* cur = builder.CreateLoad(llvm::Type::getInt32Ty(context), ptr, ident->name);
            llvm::Value* one = llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 1);
            llvm::Value* result = (n->op == "++")
                ? builder.CreateAdd(cur, one, "inctmp")
                : builder.CreateSub(cur, one, "dectmp");
            builder.CreateStore(result, ptr);
            return n->prefix ? result : cur;
        }
    }
    if (auto* n = dynamic_cast<BinaryExpr*>(node)) {
        llvm::Value* l = genExpr(n->left.get());
        llvm::Value* r = genExpr(n->right.get());
        if (n->op == "+")  return builder.CreateAdd(l, r, "addtmp");
        if (n->op == "-")  return builder.CreateSub(l, r, "subtmp");
        if (n->op == "*")  return builder.CreateMul(l, r, "multmp");
        if (n->op == "/")  return builder.CreateSDiv(l, r, "divtmp");
        if (n->op == "%")  return builder.CreateSRem(l, r, "remtmp");
        if (n->op == "==") return builder.CreateICmpEQ(l, r, "eqtmp");
        if (n->op == "!=") return builder.CreateICmpNE(l, r, "netmp");
        if (n->op == "<")  return builder.CreateICmpSLT(l, r, "lttmp");
        if (n->op == ">")  return builder.CreateICmpSGT(l, r, "gttmp");
        if (n->op == "<=") return builder.CreateICmpSLE(l, r, "letmp");
        if (n->op == ">=") return builder.CreateICmpSGE(l, r, "getmp");
        if (n->op == "&&") return builder.CreateAnd(l, r, "andtmp");
        if (n->op == "||") return builder.CreateOr(l, r, "ortmp");
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
    if (auto* n = dynamic_cast<IndexExpr*>(node)) {
        llvm::Value* ptr = namedValues[n->name];
        if (!ptr) throw std::runtime_error("Unknown variable: " + n->name);
        llvm::Value* idx = genExpr(n->index.get());
        llvm::Value* elemPtr = builder.CreateGEP(
            ptr->getType()->getPointerElementType(), ptr,
            {llvm::ConstantInt::get(llvm::Type::getInt32Ty(context), 0), idx}, "idxtmp");
        return builder.CreateLoad(llvm::Type::getInt32Ty(context), elemPtr, "elemtmp");
    }
    throw std::runtime_error("Unknown expression type");
}

void CodeGen::genStmt(ASTNode* node) {
    if (auto* n = dynamic_cast<VarDeclStmt*>(node)) {
        llvm::Type* type = getLLVMType(n->type);
        llvm::AllocaInst* alloca;
        if (n->arraySize >= 0) {
            llvm::ArrayType* arrType = llvm::ArrayType::get(type, n->arraySize);
            alloca = builder.CreateAlloca(arrType, nullptr, n->name);
        } else {
            alloca = builder.CreateAlloca(type, nullptr, n->name);
        }
        namedValues[n->name] = alloca;
        if (n->init) {
            llvm::Value* val = genExpr(n->init.get());
            builder.CreateStore(val, alloca);
        }
        return;
    }
    if (auto* n = dynamic_cast<ReturnStmt*>(node)) {
        builder.CreateRet(genExpr(n->value.get()));
        return;
    }
    if (auto* n = dynamic_cast<PrintStmt*>(node)) {
        llvm::Function* printfFn = module->getFunction("printf");
        if (!printfFn) {
            llvm::FunctionType* ft = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context),
                {llvm::Type::getInt8PtrTy(context)}, true);
            printfFn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "printf", module.get());
        }
        for (auto& arg : n->args) {
            llvm::Value* val = genExpr(arg.get());
            llvm::Value* fmt;
            if (val->getType()->isIntegerTy(32))      fmt = builder.CreateGlobalStringPtr("%d\n");
            else if (val->getType()->isIntegerTy(1))  fmt = builder.CreateGlobalStringPtr("%d\n");
            else if (val->getType()->isFloatTy())     fmt = builder.CreateGlobalStringPtr("%f\n");
            else if (val->getType()->isPointerTy())   fmt = builder.CreateGlobalStringPtr("%s\n");
            else                                      fmt = builder.CreateGlobalStringPtr("%d\n");
            builder.CreateCall(printfFn, {fmt, val});
        }
        return;
    }
    if (auto* n = dynamic_cast<InputStmt*>(node)) {
        llvm::Function* scanfFn = module->getFunction("scanf");
        if (!scanfFn) {
            llvm::FunctionType* ft = llvm::FunctionType::get(
                llvm::Type::getInt32Ty(context),
                {llvm::Type::getInt8PtrTy(context)}, true);
            scanfFn = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "scanf", module.get());
        }
        for (auto& varName : n->vars) {
            llvm::Value* ptr = namedValues[varName];
            if (!ptr) throw std::runtime_error("Unknown variable: " + varName);
            builder.CreateCall(scanfFn, {builder.CreateGlobalStringPtr("%d"), ptr});
        }
        return;
    }
    if (auto* n = dynamic_cast<IfStmt*>(node)) {
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0), "ifcond");

        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(context, "merge");
        llvm::BasicBlock* thenBB  = llvm::BasicBlock::Create(context, "then", fn);
        llvm::BasicBlock* nextBB  = n->elseIfs.empty() && n->elseBody.empty()
            ? mergeBB
            : llvm::BasicBlock::Create(context, "elif");

        builder.CreateCondBr(cond, thenBB, nextBB);
        builder.SetInsertPoint(thenBB);
        for (auto& s : n->thenBody) genStmt(s.get());
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBB);

        for (size_t i = 0; i < n->elseIfs.size(); i++) {
            fn->getBasicBlockList().push_back(dynamic_cast<llvm::BasicBlock*>(nextBB));
            builder.SetInsertPoint(nextBB);
            llvm::Value* c = genExpr(n->elseIfs[i].first.get());
            c = builder.CreateICmpNE(c, llvm::ConstantInt::get(c->getType(), 0), "elifcond");
            llvm::BasicBlock* elifBB = llvm::BasicBlock::Create(context, "elifbody", fn);
            bool isLast = (i == n->elseIfs.size() - 1);
            llvm::BasicBlock* afterBB = (isLast && n->elseBody.empty())
                ? mergeBB
                : llvm::BasicBlock::Create(context, "after");
            builder.CreateCondBr(c, elifBB, afterBB);
            builder.SetInsertPoint(elifBB);
            for (auto& s : n->elseIfs[i].second) genStmt(s.get());
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBB);
            nextBB = afterBB;
        }

        if (!n->elseBody.empty()) {
            fn->getBasicBlockList().push_back(dynamic_cast<llvm::BasicBlock*>(nextBB));
            builder.SetInsertPoint(nextBB);
            for (auto& s : n->elseBody) genStmt(s.get());
            if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(mergeBB);
        }

        fn->getBasicBlockList().push_back(mergeBB);
        builder.SetInsertPoint(mergeBB);
        return;
    }
    if (auto* n = dynamic_cast<WhileStmt*>(node)) {
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* condBB  = llvm::BasicBlock::Create(context, "whilecond", fn);
        llvm::BasicBlock* bodyBB  = llvm::BasicBlock::Create(context, "whilebody");
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "whileafter");
        breakTargets.push(afterBB);
        continueTargets.push(condBB);
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0), "whilecond");
        builder.CreateCondBr(cond, bodyBB, afterBB);
        fn->getBasicBlockList().push_back(bodyBB);
        builder.SetInsertPoint(bodyBB);
        for (auto& s : n->body) genStmt(s.get());
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(condBB);
        fn->getBasicBlockList().push_back(afterBB);
        builder.SetInsertPoint(afterBB);
        breakTargets.pop();
        continueTargets.pop();
        return;
    }
    if (auto* n = dynamic_cast<ForStmt*>(node)) {
        llvm::Function* fn = builder.GetInsertBlock()->getParent();
        genStmt(n->init.get());
        llvm::BasicBlock* condBB  = llvm::BasicBlock::Create(context, "forcond", fn);
        llvm::BasicBlock* bodyBB  = llvm::BasicBlock::Create(context, "forbody");
        llvm::BasicBlock* updateBB = llvm::BasicBlock::Create(context, "forupdate");
        llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(context, "forafter");
        breakTargets.push(afterBB);
        continueTargets.push(updateBB);
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        llvm::Value* cond = genExpr(n->cond.get());
        cond = builder.CreateICmpNE(cond, llvm::ConstantInt::get(cond->getType(), 0), "forcond");
        builder.CreateCondBr(cond, bodyBB, afterBB);
        fn->getBasicBlockList().push_back(bodyBB);
        builder.SetInsertPoint(bodyBB);
        for (auto& s : n->body) genStmt(s.get());
        if (!builder.GetInsertBlock()->getTerminator()) builder.CreateBr(updateBB);
        fn->getBasicBlockList().push_back(updateBB);
        builder.SetInsertPoint(updateBB);
        genExpr(n->update.get());
        builder.CreateBr(condBB);
        fn->getBasicBlockList().push_back(afterBB);
        builder.SetInsertPoint(afterBB);
        breakTargets.pop();
        continueTargets.pop();
        return;
    }
    if (dynamic_cast<BreakStmt*>(node)) {
        if (breakTargets.empty()) throw std::runtime_error("break outside loop");
        builder.CreateBr(breakTargets.top());
        return;
    }
    if (dynamic_cast<ContinueStmt*>(node)) {
        if (continueTargets.empty()) throw std::runtime_error("continue outside loop");
        builder.CreateBr(continueTargets.top());
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
    llvm::FunctionType* ft = llvm::FunctionType::get(getLLVMType(fn->returnType), paramTypes, false);
    llvm::Function* func = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, fn->name, module.get());
    int i = 0;
    for (auto& arg : func->args()) arg.setName(fn->params[i++].second);
    llvm::BasicBlock* bb = llvm::BasicBlock::Create(context, "entry", func);
    builder.SetInsertPoint(bb);
    namedValues.clear();
    for (auto& arg : func->args()) {
        llvm::AllocaInst* alloca = builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
        builder.CreateStore(&arg, alloca);
        namedValues[std::string(arg.getName())] = alloca;
    }
    for (auto& stmt : fn->body) genStmt(stmt.get());
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