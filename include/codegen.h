#pragma once
#include <memory>
#include <map>
#include <stack>
#include <string>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"

#include "../include/parser.h"

class CodeGen {
public:
    CodeGen();
    void generate(Program& program);
    void writeToFile(const std::string& filename);

private:
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::Value*> namedValues;
    std::stack<llvm::BasicBlock*> breakTargets;
    std::stack<llvm::BasicBlock*> continueTargets;

    llvm::Type* getLLVMType(const std::string& type);
    llvm::Value* genExpr(ASTNode* node);
    void genStmt(ASTNode* node);
    void genFunction(FunctionDecl* fn);
};