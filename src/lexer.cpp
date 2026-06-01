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
    if (current() == '.') {
        num += current();
        advance();
        while (isdigit(current())) {
            num += current();
            advance();
        }
        return {TokenType::FLOAT_LIT, num, line};
    }
    return {TokenType::NUMBER, num, line};
}

Token Lexer::readString() {
    advance();
    std::string str;
    while (current() != '"' && current() != '\0') {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n': str += '\n'; break;
                case 't': str += '\t'; break;
                case '"': str += '"'; break;
                case '\\': str += '\\'; break;
                default: str += current(); break;
            }
        } else {
            str += current();
        }
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
    if (ident == "function") return {TokenType::FUNCTION,  ident, line};
    if (ident == "return")   return {TokenType::RETURN,    ident, line};
    if (ident == "new")      return {TokenType::NEW,       ident, line};
    if (ident == "import")   return {TokenType::IMPORT,    ident, line};
    if (ident == "if")       return {TokenType::IF,        ident, line};
    if (ident == "else")     return {TokenType::ELSE,      ident, line};
    if (ident == "while")    return {TokenType::WHILE,     ident, line};
    if (ident == "for")      return {TokenType::FOR,       ident, line};
    if (ident == "print")    return {TokenType::PRINT,     ident, line};
    if (ident == "input")    return {TokenType::INPUT,     ident, line};
    if (ident == "break")    return {TokenType::BREAK,     ident, line};
    if (ident == "continue") return {TokenType::CONTINUE,  ident, line};
    if (ident == "struct")   return {TokenType::STRUCT,    ident, line};
    if (ident == "true")     return {TokenType::TRUE_LIT,  ident, line};
    if (ident == "false")    return {TokenType::FALSE_LIT, ident, line};
    if (ident == "int")      return {TokenType::INT,       ident, line};
    if (ident == "float")    return {TokenType::FLOAT,     ident, line};
    if (ident == "bool")     return {TokenType::BOOL,      ident, line};
    if (ident == "char")     return {TokenType::CHAR,      ident, line};
    if (ident == "string")   return {TokenType::STRING,    ident, line};
    return {TokenType::IDENT, ident, line};
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();
        if (current() == '\0') break;

        if (current() == '/' && peek() == '/') {
            while (current() != '\n' && current() != '\0') advance();
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
                case '[': tokens.push_back({TokenType::LBRACKET,  "[", line}); break;
                case ']': tokens.push_back({TokenType::RBRACKET,  "]", line}); break;
                case ',': tokens.push_back({TokenType::COMMA,     ",", line}); break;
                case ';': tokens.push_back({TokenType::SEMICOLON, ";", line}); break;
                case '.': tokens.push_back({TokenType::DOT,       ".", line}); break;
                case '%': tokens.push_back({TokenType::PERCENT,   "%", line}); break;
                case '!':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::NEQ,  "!=", line}); }
                    else                  { tokens.push_back({TokenType::NOT, "!", line}); }
                    break;
                case '+':
                    if (current() == '+') { advance(); tokens.push_back({TokenType::PLUSPLUS,  "++", line}); }
                    else if (current() == '=') { advance(); tokens.push_back({TokenType::PLUSEQ, "+=", line}); }
                    else { tokens.push_back({TokenType::PLUS, "+", line}); }
                    break;
                case '-':
                    if (current() == '-') { advance(); tokens.push_back({TokenType::MINUSMINUS, "--", line}); }
                    else if (current() == '=') { advance(); tokens.push_back({TokenType::MINUSEQ, "-=", line}); }
                    else { tokens.push_back({TokenType::MINUS, "-", line}); }
                    break;
                case '*':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::STAREQ,  "*=", line}); }
                    else { tokens.push_back({TokenType::STAR, "*", line}); }
                    break;
                case '/':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::SLASHEQ, "/=", line}); }
                    else { tokens.push_back({TokenType::SLASH, "/", line}); }
                    break;
                case '<':
                    if (current() == '<') { advance(); tokens.push_back({TokenType::LTLT, "<<", line}); }
                    else if (current() == '=') { advance(); tokens.push_back({TokenType::LTE, "<=", line}); }
                    else { tokens.push_back({TokenType::LT, "<", line}); }
                    break;
                case '>':
                    if (current() == '>') { advance(); tokens.push_back({TokenType::GTGT, ">>", line}); }
                    else if (current() == '=') { advance(); tokens.push_back({TokenType::GTE, ">=", line}); }
                    else { tokens.push_back({TokenType::GT, ">", line}); }
                    break;
                case '=':
                    if (current() == '=') { advance(); tokens.push_back({TokenType::EQEQ,   "==", line}); }
                    else { tokens.push_back({TokenType::EQUALS, "=", line}); }
                    break;
                case '&':
                    if (current() == '&') { advance(); tokens.push_back({TokenType::AND, "&&", line}); }
                    break;
                case '|':
                    if (current() == '|') { advance(); tokens.push_back({TokenType::OR, "||", line}); }
                    break;
                default:
                    throw std::runtime_error("Unknown character: " + std::string(1, c) + " at line " + std::to_string(line));
            }
        }
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}