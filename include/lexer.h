#pragma once
#include <string>
#include <vector>

enum class TokenType {
    FUNCTION, RETURN, NEW, IMPORT, IF, ELSE, WHILE, FOR, PRINT, INPUT,
    INT, FLOAT, BOOL, CHAR, STRING,
    NUMBER, STRING_LIT, IDENT,
    LPAREN, RPAREN, LBRACE, RBRACE, EQUALS, COMMA, SEMICOLON,
    PLUS, MINUS, STAR, SLASH, EQEQ, NEQ, LT, GT, LTLT, GTGT, AND, OR,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();
private:
    std::string src;
    int pos;
    int line;
    char current();
    char peek();
    void advance();
    void skipWhitespace();
    Token readNumber();
    Token readString();
    Token readIdent();
};