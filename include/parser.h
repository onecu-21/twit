#pragma once
#include <string>
#include <vector>
#include <memory>
#include "../include/lexer.h"
//who the hell made windows
struct ASTNode {
    virtual ~ASTNode() = default;
};

// 표현식
struct NumberExpr : ASTNode {
    int value;
    NumberExpr(int v) : value(v) {}
};

struct FloatExpr : ASTNode {
    float value;
    FloatExpr(float v) : value(v) {}
};

struct StringExpr : ASTNode {
    std::string value;
    StringExpr(const std::string& v) : value(v) {}
};

struct BoolExpr : ASTNode {
    bool value;
    BoolExpr(bool v) : value(v) {}
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

struct UnaryExpr : ASTNode {
    std::string op;
    std::unique_ptr<ASTNode> operand;
    bool prefix;
    UnaryExpr(std::string op, std::unique_ptr<ASTNode> operand, bool prefix)
        : op(op), operand(std::move(operand)), prefix(prefix) {}
};

struct CallExpr : ASTNode {
    std::string callee;
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct IndexExpr : ASTNode {
    std::string name;
    std::unique_ptr<ASTNode> index;
};

struct MemberExpr : ASTNode {
    std::unique_ptr<ASTNode> object;
    std::string member;
};

struct MemberAssignExpr : ASTNode {
    std::string object;
    std::string member;
    std::string op;
    std::unique_ptr<ASTNode> value;
};

struct AssignExpr : ASTNode {
    std::string name;
    std::string op;
    std::unique_ptr<ASTNode> value;
};

// 구문
struct VarDeclStmt : ASTNode {
    std::string type;
    std::string name;
    int arraySize = -1;
    std::unique_ptr<ASTNode> init;
};

struct ReturnStmt : ASTNode {
    std::unique_ptr<ASTNode> value;
};

struct IfStmt : ASTNode {
    std::unique_ptr<ASTNode> cond;
    std::vector<std::unique_ptr<ASTNode>> thenBody;
    std::vector<std::pair<std::unique_ptr<ASTNode>, std::vector<std::unique_ptr<ASTNode>>>> elseIfs;
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

struct BreakStmt : ASTNode {};
struct ContinueStmt : ASTNode {};

struct PrintStmt : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> args;
};

struct InputStmt : ASTNode {
    std::vector<std::string> vars;
};

struct ExprStmt : ASTNode {
    std::unique_ptr<ASTNode> expr;
};

struct StructDecl : ASTNode {
    std::string name;
    std::vector<std::pair<std::string, std::string>> fields;
};

struct FunctionDecl : ASTNode {
    std::string name;
    std::string returnType;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::unique_ptr<ASTNode>> body;
};

struct Program {
    std::vector<std::string> imports;
    std::vector<std::unique_ptr<StructDecl>> structs;
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
    std::unique_ptr<StructDecl> parseStruct();
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
    std::unique_ptr<ASTNode> parseAssign();
    std::unique_ptr<ASTNode> parseOr();
    std::unique_ptr<ASTNode> parseAnd();
    std::unique_ptr<ASTNode> parseComparison();
    std::unique_ptr<ASTNode> parseAddSub();
    std::unique_ptr<ASTNode> parseMulDiv();
    std::unique_ptr<ASTNode> parseUnary();
    std::unique_ptr<ASTNode> parsePostfix();
    std::unique_ptr<ASTNode> parsePrimary();
};