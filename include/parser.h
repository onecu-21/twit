#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../include/lexer.h"

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct NumberExpr : ASTNode {
    int value;
    NumberExpr(int v) : value(v) {}
};

struct StringExpr : ASTNode {
    std::string value;
    StringExpr(const std::string& v) : value(v) {}
};

struct IdentExpr : ASTNode {
    std::string name;
    IdentExpr(const std::string& n) : name(n) {}
};

struct BinaryExpr : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> left, right;
    BinaryExpr(std::string op, std::unique_ptr<ASTNode> l, std::unique_ptr<ASTNode> r)
        : op(op), left(std::move(l)), right(std::move(r)) {}
};

struct CallExpr : ASTNode {
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct VarDeclStmt : ASTNode {
    std::string type;
    std::string name;
    std::unique_ptr<ASTNode> init;
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
};

struct IfStmt : ASTNode {
    std::unique_ptr<ASTNode> cond;
    std::vector<std::unique_ptr<ASTNode>> thenBody;
    std::vector<std::unique_ptr<ASTNode>> elseBody;
};

struct WhileStmt : ASTNode {
    std::unique_ptr<ASTNode> cond;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct ForStmt : ASTNode {
    std::unique_ptr<ASTNode> init;
    std::unique_ptr<ASTNode> cond;
    std::unique_ptr<ASTNode> update;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct PrintStmt : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct InputStmt : ASTNode {
    std::vector<std::string> vars;
};

struct ExprStmt : ASTNode {
    std::unique_ptr<ASTNode> expr;
};

struct FunctionDecl : ASTNode {
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct Program {
    std::vector<std::string> imports;
    std::vector<std::unique_ptr<FunctionDecl>> functions;
};

class Parser {
public:
    Parser(std::vector<Token> tokens);
    Program parse();
private:
    std::vector<Token> tokens;
    int pos;
    Token current();
    Token peek();
    Token consume();
    Token expect(TokenType type);
    bool check(TokenType type);
    std::unique_ptr<FunctionDecl> parseFunction();
    std::vector<std::unique_ptr<ASTNode>> parseBody();
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseVarDecl();
    std::unique_ptr<ASTNode> parseReturn();
    std::unique_ptr<ASTNode> parseIf();
    std::unique_ptr<ASTNode> parseWhile();
    std::unique_ptr<ASTNode> parseFor();
    std::unique_ptr<ASTNode> parsePrint();
    std::unique_ptr<ASTNode> parseInput();
    std::unique_ptr<ASTNode> parseExpr();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseAddSub();
    std::unique_ptr<ASTNode> parseMulDiv();
    std::unique_ptr<ASTNode> parsePrimary();
};