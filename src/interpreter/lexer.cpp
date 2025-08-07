#include "interpreter/lexer.h"
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace basic {

Lexer::Lexer() {
    setupKeywords();
}

void Lexer::setupKeywords() {
    keywords_ = {
        {"LET", TokenType::LET},
        {"IF", TokenType::IF},
        {"THEN", TokenType::THEN},
        {"ELSE", TokenType::ELSE},
        {"FOR", TokenType::FOR},
        {"TO", TokenType::TO},
        {"STEP", TokenType::STEP},
        {"NEXT", TokenType::NEXT},
        {"WHILE", TokenType::WHILE},
        {"WEND", TokenType::WEND},
        {"DO", TokenType::DO},
        {"LOOP", TokenType::LOOP},
        {"UNTIL", TokenType::UNTIL},
        {"SUB", TokenType::SUB},
        {"END", TokenType::END},
        {"FUNCTION", TokenType::FUNCTION},
        {"RETURN", TokenType::RETURN},
        {"PRINT", TokenType::PRINT},
        {"INPUT", TokenType::INPUT},
        {"READ", TokenType::READ},
        {"DATA", TokenType::DATA},
        {"RESTORE", TokenType::RESTORE},
        {"DIM", TokenType::DIM}
    };
}

std::vector<Token> Lexer::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::string current;
    int line = 1;
    int column = 1;
    int pos = 0;
    
    while (pos < input.length()) {
        char ch = input[pos];
        
        // Skip whitespace
        if (std::isspace(ch)) {
            if (ch == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            pos++;
            continue;
        }
        
        // Comments
        if (ch == '\'') {
            // Skip to end of line
            while (pos < input.length() && input[pos] != '\n') {
                pos++;
            }
            continue;
        }
        
        // Numbers
        if (std::isdigit(ch) || ch == '.') {
            int startColumn = column;
            std::string number;
            bool hasDecimal = false;
            
            while (pos < input.length()) {
                ch = input[pos];
                if (std::isdigit(ch)) {
                    number += ch;
                } else if (ch == '.' && !hasDecimal) {
                    number += ch;
                    hasDecimal = true;
                } else {
                    break;
                }
                pos++;
                column++;
            }
            
            tokens.emplace_back(TokenType::NUMBER, number, line, startColumn);
            continue;
        }
        
        // Strings
        if (ch == '"') {
            int startColumn = column;
            std::string str;
            pos++; // Skip opening quote
            column++;
            
            while (pos < input.length() && input[pos] != '"') {
                if (input[pos] == '\\' && pos + 1 < input.length()) {
                    // Handle escape sequences
                    pos++;
                    column++;
                    switch (input[pos]) {
                        case 'n': str += '\n'; break;
                        case 't': str += '\t'; break;
                        case 'r': str += '\r'; break;
                        case '"': str += '"'; break;
                        case '\\': str += '\\'; break;
                        default: str += '\\'; str += input[pos]; break;
                    }
                } else {
                    str += input[pos];
                }
                pos++;
                column++;
            }
            
            if (pos < input.length()) {
                pos++; // Skip closing quote
                column++;
            }
            
            tokens.emplace_back(TokenType::STRING, str, line, startColumn);
            continue;
        }
        
        // Identifiers and keywords
        if (std::isalpha(ch) || ch == '_') {
            int startColumn = column;
            std::string identifier;
            
            while (pos < input.length()) {
                ch = input[pos];
                if (std::isalnum(ch) || ch == '_') {
                    identifier += ch;
                } else {
                    break;
                }
                pos++;
                column++;
            }
            
            // Check if it's a keyword
            auto it = keywords_.find(identifier);
            if (it != keywords_.end()) {
                tokens.emplace_back(it->second, identifier, line, startColumn);
            } else {
                tokens.emplace_back(TokenType::IDENTIFIER, identifier, line, startColumn);
            }
            continue;
        }
        
        // Operators and delimiters
        int startColumn = column;
        TokenType tokenType = TokenType::UNKNOWN;
        std::string value;
        
        switch (ch) {
            case '+':
                tokenType = TokenType::PLUS;
                value = "+";
                break;
            case '-':
                tokenType = TokenType::MINUS;
                value = "-";
                break;
            case '*':
                tokenType = TokenType::MULTIPLY;
                value = "*";
                break;
            case '/':
                tokenType = TokenType::DIVIDE;
                value = "/";
                break;
            case '%':
                tokenType = TokenType::MOD;
                value = "%";
                break;
            case '^':
                tokenType = TokenType::POWER;
                value = "^";
                break;
            case '(':
                tokenType = TokenType::LPAREN;
                value = "(";
                break;
            case ')':
                tokenType = TokenType::RPAREN;
                value = ")";
                break;
            case ',':
                tokenType = TokenType::COMMA;
                value = ",";
                break;
            case ';':
                tokenType = TokenType::SEMICOLON;
                value = ";";
                break;
            case ':':
                tokenType = TokenType::COLON;
                value = ":";
                break;
            case '=':
                tokenType = TokenType::ASSIGN;
                value = "=";
                break;
            case '<':
                if (pos + 1 < input.length() && input[pos + 1] == '=') {
                    tokenType = TokenType::LESS_EQUAL;
                    value = "<=";
                    pos++;
                    column++;
                } else if (pos + 1 < input.length() && input[pos + 1] == '>') {
                    tokenType = TokenType::NOT_EQUAL;
                    value = "<>";
                    pos++;
                    column++;
                } else {
                    tokenType = TokenType::LESS;
                    value = "<";
                }
                break;
            case '>':
                if (pos + 1 < input.length() && input[pos + 1] == '=') {
                    tokenType = TokenType::GREATER_EQUAL;
                    value = ">=";
                    pos++;
                    column++;
                } else {
                    tokenType = TokenType::GREATER;
                    value = ">";
                }
                break;
            default:
                throw std::runtime_error("Unknown character: " + std::string(1, ch));
        }
        
        tokens.emplace_back(tokenType, value, line, startColumn);
        pos++;
        column++;
    }
    
    // Add EOF token
    tokens.emplace_back(TokenType::EOF_TOKEN, "", line, column);
    
    return tokens;
}

std::string Lexer::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::LET: return "LET";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::FOR: return "FOR";
        case TokenType::TO: return "TO";
        case TokenType::STEP: return "STEP";
        case TokenType::NEXT: return "NEXT";
        case TokenType::WHILE: return "WHILE";
        case TokenType::WEND: return "WEND";
        case TokenType::DO: return "DO";
        case TokenType::LOOP: return "LOOP";
        case TokenType::UNTIL: return "UNTIL";
        case TokenType::SUB: return "SUB";
        case TokenType::END: return "END";
        case TokenType::FUNCTION: return "FUNCTION";
        case TokenType::RETURN: return "RETURN";
        case TokenType::PRINT: return "PRINT";
        case TokenType::INPUT: return "INPUT";
        case TokenType::READ: return "READ";
        case TokenType::DATA: return "DATA";
        case TokenType::RESTORE: return "RESTORE";
        case TokenType::DIM: return "DIM";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::MOD: return "MOD";
        case TokenType::POWER: return "POWER";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER: return "GREATER";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::UNKNOWN: return "UNKNOWN";
        default: return "UNKNOWN";
    }
}

} // namespace basic 