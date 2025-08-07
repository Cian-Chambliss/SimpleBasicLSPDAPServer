#pragma once

#include "interpreter/basic_interpreter.h"
#include <string>
#include <vector>
#include <map>

namespace basic {

class Lexer {
public:
    Lexer();
    
    std::vector<Token> tokenize(const std::string& input);
    std::string tokenTypeToString(TokenType type);

private:
    std::map<std::string, TokenType> keywords_;
    void setupKeywords();
};

} // namespace basic 