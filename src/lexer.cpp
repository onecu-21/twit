#include "../include/lexer.h"
#include <stdexcept>

Lexer::Lexer(const std::string& source) : src(source), pos(0), line(1) {}

char Lexer::current() {
    return pos < src.size() ? src[pos] : '\0';
}

char Lexer::peek() {
    return (pos + 1) < src.size() ? src[pos + 1] : '\0';
}

void Lexer::advance() {
    if (current() == '\n') line++;
    pos++;
}

void Lexer::skipWhitespace() {
    while (current() == ' ' || current() == '\t' ||
           current() == '\n' || current() == '\r')
        advance();
}

Token Lexer::readNumber() {
    std::string num;
    while (isdigit(current())) {
        num += current();
        advance();
    }
    return {TokenType::NUMBER, num, line};
}

Token Lexer::readString() {
    advance();
    std::string str;
    while (current() != '"' && current() != '\0') {
        str += current();
        advance();
    }
    advance();
    return {TokenType::STRING_LIT, str, line};
}

Token Lexer::readIdent() {
    std::string ident;
    while (isalnum(current()) || current() == '_') {
        ident += current();
        advance();
    }
    if (ident == "function") return {TokenType::FUNCTION, ident, line};
    if (ident == "return")   return {TokenType::RETURN,   ident, line};
    if (ident == "new")      return {TokenType::NEW,      ident, line};
    if (ident == "import")   return {TokenType::IMPORT,   ident, line};
    if (ident == "if")       return {TokenType::IF,       ident, line};
    if (ident == "else")     return {TokenType::ELSE,     ident, line};
    if (ident == "while")    return {TokenType::WHILE,    ident, line};
    if (ident == "for")      return {TokenType::FOR,      ident, line};
    if (ident == "print")    return {TokenType::PRINT,    ident, line};
    if (ident == "input")    return {TokenType::INPUT,    ident, line};
    if (ident == "int")      return {TokenType::INT,      ident, line};
    if (ident == "float")    return {TokenType::FLOAT,    ident, line};
    if (ident == "bool")     return {TokenType::BOOL,     ident, line};
    if (ident == "char")     return {TokenType::CHAR,     ident, line};
    if (ident == "string")   return {TokenType::STRING,   ident, line};
    return {TokenType::IDENT, ident, line};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();
        if (current() == '\0') break;

        if (current() == '/' && peek() == '/') {
            while (current() != '\n' && current() != '\0')
                advance();
            continue;
        }

        if (isdigit(current())) {
            tokens.push_back(readNumber());
        } else if (current() == '"') {
            tokens.push_back(readString());
        } else if (isalpha(current()) || current() == '_') {
            tokens.push_back(readIdent());
        } else {
            char c = current();
            advance();
            switch (c) {
                case '(': tokens.push_back({TokenType::LPAREN,    "(", line}); break;
                case ')': tokens.push_back({TokenType::RPAREN,    ")", line}); break;
                case '{': tokens.push_back({TokenType::LBRACE,    "{", line}); break;
                case '}': tokens.push_back({TokenType::RBRACE,    "}", line}); break;
                case ',': tokens.push_back({TokenType::COMMA,     ",", line}); break;
                case ';': tokens.push_back({TokenType::SEMICOLON, ";", line}); break;
                case '+': tokens.push_back({TokenType::PLUS,      "+", line}); break;
                case '-': tokens.push_back({TokenType::MINUS,     "-", line}); break;
                case '*': tokens.push_back({TokenType::STAR,      "*", line}); break;
                case '/': tokens.push_back({TokenType::SLASH,     "/", line}); break;
                case '<':
                    if (current() == '<') {
                        advance();
                        tokens.push_back({TokenType::LTLT, "<<", line});
                    } else {
                        tokens.push_back({TokenType::LT, "<", line});
                    }
                    break;
                case '>':
                    if (current() == '>') {
                        advance();
                        tokens.push_back({TokenType::GTGT, ">>", line});
                    } else {
                        tokens.push_back({TokenType::GT, ">", line});
                    }
                    break;
                case '=':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::EQEQ,   "==", line}); }
                    else                  { tokens.push_back({TokenType::EQUALS, "=",  line}); }
                    break;
                case '!':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::NEQ, "!=", line}); }
                    break;
                case '&':
                    if (current() == '&') { advance(); tokens.push_back({TokenType::AND, "&&", line}); }
                    break;
                case '|':
                    if (current() == '|') { advance(); tokens.push_back({TokenType::OR,  "||", line}); }
                    break;
                default:
                    throw std::runtime_error("Unknown character: " + std::string(1, c));
            }
        }
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}