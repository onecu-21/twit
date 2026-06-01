#pragma once
#include <string>
#include <vector>

enum class TokenType {
    FUNCTION, RETURN, NEW, IMPORT, IF, ELSE, WHILE, FOR, PRINT, INPUT, BREAK, CONTINUE, STRUCT,
    INT, FLOAT, BOOL, CHAR, STRING,
    NUMBER, FLOAT_LIT, STRING_LIT, IDENT, TRUE_LIT, FALSE_LIT,
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    EQUALS, COMMA, SEMICOLON, DOT,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    EQEQ, NEQ, LT, GT, LTE, GTE,
    LTLT, GTGT,
    AND, OR, NOT,
    PLUSPLUS, MINUSMINUS,
    PLUSEQ, MINUSEQ, STAREQ, SLASHEQ,
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