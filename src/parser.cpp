#include "../include/parser.h"
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens(tokens), pos(0) {}

Token Parser::current() {
    return tokens[pos];
}

Token Parser::peek() {
    return tokens[pos + 1];
}

Token Parser::consume() {
    return tokens[pos++];
}

Token Parser::expect(TokenType type) {
    if (current().type != type)
        throw std::runtime_error("Unexpected token: " + current().value + " at line " + std::to_string(current().line));
    return consume();
}

bool Parser::check(TokenType type) {
    return current().type == type;
}

Program Parser::parse() {
    Program program;
    while (!check(TokenType::EOF_TOKEN)) {
        if (check(TokenType::IMPORT)) {
            consume();
            program.imports.push_back(expect(TokenType::IDENT).value);
        } else if (check(TokenType::FUNCTION)) {
            program.functions.push_back(parseFunction());
        } else {
            throw std::runtime_error("Unexpected token at top level: " + current().value);
        }
    }
    return program;
}

std::unique_ptr<FunctionDecl> Parser::parseFunction() {
    expect(TokenType::FUNCTION);
    auto fn = std::make_unique<FunctionDecl>();
    fn->name = expect(TokenType::IDENT).value;
    expect(TokenType::LPAREN);
    while (!check(TokenType::RPAREN)) {
        std::string type = consume().value;
        std::string name = expect(TokenType::IDENT).value;
        fn->params.push_back({type, name});
        if (check(TokenType::COMMA)) consume();
    }
    expect(TokenType::RPAREN);
    expect(TokenType::EQUALS);
    fn->returnType = consume().value;
    expect(TokenType::LBRACE);
    fn->body = parseBody();
    expect(TokenType::RBRACE);
    return fn;
}

std::vector<std::unique_ptr<ASTNode>> Parser::parseBody() {
    std::vector<std::unique_ptr<ASTNode>> stmts;
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOKEN)) {
        stmts.push_back(parseStatement());
    }
    return stmts;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (check(TokenType::NEW))    return parseVarDecl();
    if (check(TokenType::RETURN)) return parseReturn();
    if (check(TokenType::IF))     return parseIf();
    if (check(TokenType::WHILE))  return parseWhile();
    if (check(TokenType::FOR))    return parseFor();
    if (check(TokenType::PRINT))  return parsePrint();
    if (check(TokenType::INPUT))  return parseInput();
    auto expr = std::make_unique<ExprStmt>();
    expr->expr = parseExpr();
    return expr;
}

std::unique_ptr<ASTNode> Parser::parseVarDecl() {
    expect(TokenType::NEW);
    auto decl = std::make_unique<VarDeclStmt>();
    decl->type = consume().value;
    decl->name = expect(TokenType::IDENT).value;
    if (check(TokenType::EQUALS)) {
        consume();
        decl->init = parseExpr();
    }
    return decl;
}

std::unique_ptr<ASTNode> Parser::parseReturn() {
    expect(TokenType::RETURN);
    auto ret = std::make_unique<ReturnStmt>();
    ret->value = parseExpr();
    return ret;
}

std::unique_ptr<ASTNode> Parser::parseIf() {
    expect(TokenType::IF);
    auto stmt = std::make_unique<IfStmt>();
    expect(TokenType::LPAREN);
    stmt->cond = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::LBRACE);
    stmt->thenBody = parseBody();
    expect(TokenType::RBRACE);
    if (check(TokenType::ELSE)) {
        consume();
        expect(TokenType::LBRACE);
        stmt->elseBody = parseBody();
        expect(TokenType::RBRACE);
    }
    return stmt;
}

std::unique_ptr<ASTNode> Parser::parseWhile() {
    expect(TokenType::WHILE);
    auto stmt = std::make_unique<WhileStmt>();
    expect(TokenType::LPAREN);
    stmt->cond = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::LBRACE);
    stmt->body = parseBody();
    expect(TokenType::RBRACE);
    return stmt;
}

std::unique_ptr<ASTNode> Parser::parseFor() {
    expect(TokenType::FOR);
    auto stmt = std::make_unique<ForStmt>();
    expect(TokenType::LPAREN);
    stmt->init = parseVarDecl();
    expect(TokenType::SEMICOLON);
    stmt->cond = parseExpr();
    expect(TokenType::SEMICOLON);
    stmt->update = parseExpr();
    expect(TokenType::RPAREN);
    expect(TokenType::LBRACE);
    stmt->body = parseBody();
    expect(TokenType::RBRACE);
    return stmt;
}

std::unique_ptr<ASTNode> Parser::parsePrint() {
    expect(TokenType::PRINT);
    auto stmt = std::make_unique<PrintStmt>();
    while (check(TokenType::LTLT)) {
        consume();
        stmt->args.push_back(parseExpr());
    }
    return stmt;
}

std::unique_ptr<ASTNode> Parser::parseInput() {
    expect(TokenType::INPUT);
    auto stmt = std::make_unique<InputStmt>();
    while (check(TokenType::GTGT)) {
        consume();
        stmt->vars.push_back(expect(TokenType::IDENT).value);
    }
    return stmt;
}

std::unique_ptr<ASTNode> Parser::parseExpr() {
    return parseComparison();
}

std::unique_ptr<ASTNode> Parser::parseComparison() {
    auto left = parseAddSub();
    while (check(TokenType::EQEQ) || check(TokenType::NEQ) ||
           check(TokenType::LT)   || check(TokenType::GT)) {
        std::string op = consume().value;
        auto right = parseAddSub();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseAddSub() {
    auto left = parseMulDiv();
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        std::string op = consume().value;
        auto right = parseMulDiv();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parseMulDiv() {
    auto left = parsePrimary();
    while (check(TokenType::STAR) || check(TokenType::SLASH)) {
        std::string op = consume().value;
        auto right = parsePrimary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (check(TokenType::NUMBER)) {
        return std::make_unique<NumberExpr>(std::stoi(consume().value));
    }
    if (check(TokenType::STRING_LIT)) {
        return std::make_unique<StringExpr>(consume().value);
    }
    if (check(TokenType::IDENT)) {
        std::string name = consume().value;
        if (check(TokenType::LPAREN)) {
            consume();
            auto call = std::make_unique<CallExpr>();
            call->callee = name;
            while (!check(TokenType::RPAREN)) {
                call->args.push_back(parseExpr());
                if (check(TokenType::COMMA)) consume();
            }
            expect(TokenType::RPAREN);
            return call;
        }
        return std::make_unique<IdentExpr>(name);
    }
    if (check(TokenType::LPAREN)) {
        consume();
        auto expr = parseExpr();
        expect(TokenType::RPAREN);
        return expr;
    }
    throw std::runtime_error("Unexpected token in expression: " + current().value);
}