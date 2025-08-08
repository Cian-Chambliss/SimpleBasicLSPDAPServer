#include "interpreter/parser.h"
#include <stdexcept>
#include <sstream>

namespace basic {

Parser::Parser() : current_(0) {}

std::unique_ptr<ASTNode> Parser::parse(const std::vector<Token>& tokens) {
    tokens_ = tokens;
    current_ = 0;
    return parseProgram();
}

std::unique_ptr<ASTNode> Parser::parseLine(const std::vector<Token>& tokens) {
    tokens_ = tokens;
    current_ = 0;
    return parseStatement();
}

std::unique_ptr<ASTNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>();
    
    while (!isAtEnd()) {
        auto statement = parseStatement();
        if (statement) {
            program->statements.push_back(std::move(statement));
        }
    }
    
    return program;
}

std::unique_ptr<ASTNode> Parser::parseStatement() {
    if (isAtEnd()) return nullptr;
    
    try {
        if (match(TokenType::LET)) {
            return parseLetStatement();
        } else if (match(TokenType::IF)) {
            return parseIfStatement();
        } else if (match(TokenType::FOR)) {
            return parseForStatement();
        } else if (match(TokenType::NEXT)) {
            return parseNextStatement();
        } else if (match(TokenType::WHILE)) {
            return parseWhileStatement();
        } else if (match(TokenType::PRINT)) {
            return parsePrintStatement();
        } else if (match(TokenType::INPUT)) {
            return parseInputStatement();
        } else if (check(TokenType::IDENTIFIER)) {
            // Could be assignment or function call
            advance();
            if (match(TokenType::ASSIGN)) {
                auto letStmt = std::make_unique<LetStatementNode>();
                letStmt->variableName = current().value;
                letStmt->value = parseExpression();
                return letStmt;
            } else {
                // Function call
                current_--;
                return parseFunctionCall();
            }
        } else {
            // Try to parse as expression
            return parseExpression();
        }
    } catch (const std::exception& e) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<ASTNode> Parser::parseLetStatement() {
    auto letStmt = std::make_unique<LetStatementNode>();
    letStmt->line = current().line;
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected identifier after LET");
    }
    
    letStmt->variableName = current().value;
    advance();
    
    if (!match(TokenType::ASSIGN)) {
        throw std::runtime_error("Expected '=' after variable name");
    }
    
    letStmt->value = parseExpression();
    return letStmt;
}

std::unique_ptr<ASTNode> Parser::parseIfStatement() {
    auto ifStmt = std::make_unique<IfStatementNode>();
    ifStmt->line = current().line;
    
    ifStmt->condition = parseExpression();
    
    if (!match(TokenType::THEN)) {
        throw std::runtime_error("Expected THEN after IF condition");
    }
    
    ifStmt->thenStatement = parseStatement();
    
    if (match(TokenType::ELSE)) {
        ifStmt->elseStatement = parseStatement();
    }
    
    return ifStmt;
}

std::unique_ptr<ASTNode> Parser::parseForStatement() {
    auto forStmt = std::make_unique<ForStatementNode>();
    forStmt->line = current().line;
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected identifier after FOR");
    }
    
    forStmt->variableName = current().value;
    advance();
    
    if (!match(TokenType::ASSIGN)) {
        throw std::runtime_error("Expected '=' after FOR variable");
    }
    
    forStmt->startValue = parseExpression();
    
    if (!match(TokenType::TO)) {
        throw std::runtime_error("Expected TO in FOR statement");
    }
    
    forStmt->endValue = parseExpression();
    
    if (match(TokenType::STEP)) {
        forStmt->stepValue = parseExpression();
    }
    
    // For now, we'll assume the body is a single statement
    // In a full implementation, you'd want to handle multiple statements
    forStmt->body = parseStatement();
    
    return forStmt;
}

std::unique_ptr<ASTNode> Parser::parseNextStatement() {
    auto nextStmt = std::make_unique<NextStatementNode>();
    return nextStmt;
}

std::unique_ptr<ASTNode> Parser::parseWhileStatement() {
    auto whileStmt = std::make_unique<WhileStatementNode>();
    whileStmt->line = current().line;
    
    whileStmt->condition = parseExpression();
    whileStmt->body = parseStatement();
    
    return whileStmt;
}

std::unique_ptr<ASTNode> Parser::parsePrintStatement() {
    auto printStmt = std::make_unique<PrintStatementNode>();
    printStmt->line = current().line;
    
    while (!isAtEnd() && !check(TokenType::SEMICOLON) && !check(TokenType::COLON)) {
        printStmt->expressions.push_back(parseExpression());
        
        if (match(TokenType::COMMA)) {
            continue;
        }
    }
    
    return printStmt;
}

std::unique_ptr<ASTNode> Parser::parseInputStatement() {
    auto inputStmt = std::make_unique<InputStatementNode>();
    inputStmt->line = current().line;
    
    if (check(TokenType::STRING)) {
        inputStmt->prompt = current().value;
        advance();
        
        if (!match(TokenType::COMMA)) {
            throw std::runtime_error("Expected comma after INPUT prompt");
        }
    }
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected variable name in INPUT statement");
    }
    
    inputStmt->variableName = current().value;
    advance();
    
    return inputStmt;
}

std::unique_ptr<ASTNode> Parser::parseFunctionCall() {
    auto funcCall = std::make_unique<FunctionCallNode>();
    
    if (!check(TokenType::IDENTIFIER)) {
        throw std::runtime_error("Expected function name");
    }
    
    funcCall->functionName = current().value;
    advance();
    
    if (match(TokenType::LPAREN)) {
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
            funcCall->arguments.push_back(parseExpression());
            
            if (!match(TokenType::COMMA)) {
                break;
            }
        }
        
        consume(TokenType::RPAREN, "Expected ')' after function arguments");
    }
    
    return funcCall;
}

std::unique_ptr<ASTNode> Parser::parseExpression() {
    auto left = parseTerm();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS) ||
           match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL) ||
           match(TokenType::LESS) || match(TokenType::LESS_EQUAL) ||
           match(TokenType::GREATER) || match(TokenType::GREATER_EQUAL)) {
        
        auto binaryExpr = std::make_unique<BinaryExpressionNode>();
        binaryExpr->operator_ = current().type;
        binaryExpr->left = std::move(left);
        binaryExpr->right = parseTerm();
        left = std::move(binaryExpr);
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseTerm() {
    auto left = parseFactor();
    
    while (match(TokenType::MULTIPLY) || match(TokenType::DIVIDE) || match(TokenType::MOD)) {
        auto binaryExpr = std::make_unique<BinaryExpressionNode>();
        binaryExpr->operator_ = current().type;
        binaryExpr->left = std::move(left);
        binaryExpr->right = parseFactor();
        left = std::move(binaryExpr);
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parseFactor() {
    auto left = parsePrimary();
    
    while (match(TokenType::POWER)) {
        auto binaryExpr = std::make_unique<BinaryExpressionNode>();
        binaryExpr->operator_ = TokenType::POWER;
        binaryExpr->left = std::move(left);
        binaryExpr->right = parsePrimary();
        left = std::move(binaryExpr);
    }
    
    return left;
}

std::unique_ptr<ASTNode> Parser::parsePrimary() {
    if (match(TokenType::NUMBER)) {
        auto literal = std::make_unique<LiteralNode>();
        try {
            if (current().value.find('.') != std::string::npos) {
                literal->value = std::stod(last().value);
            } else {
                literal->value = std::stoi(last().value);
            }
        } catch (...) {
            literal->value = 0;
        }
        return literal;
    }
    
    if (match(TokenType::STRING)) {
        auto literal = std::make_unique<LiteralNode>();
        literal->value = last().value;
        return literal;
    }
    
    if (match(TokenType::IDENTIFIER)) {
        auto identifier = std::make_unique<IdentifierNode>();
        identifier->name = last().value;
        return identifier;
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression");
        return expr;
    }
    
    throw std::runtime_error("Unexpected token: " + current().value);
}

// Helper methods
Token Parser::current() const {
    return tokens_[current_];
}
Token Parser::last() const {
    if( current_ > 0 )
        return tokens_[current_ - 1 ];
    return tokens_[current_];
}


Token Parser::peek() const {
    if (isAtEnd()) return Token(TokenType::EOF_TOKEN, "", 0, 0);
    return tokens_[current_ + 1];
}

bool Parser::isAtEnd() const {
    return current_ >= tokens_.size() || current().type == TokenType::EOF_TOKEN;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return current().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::advance() {
    if (!isAtEnd()) current_++;
}

void Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        advance();
        return;
    }
    throw std::runtime_error(message);
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (current().type == TokenType::SEMICOLON) {
            advance();
            return;
        }
        
        switch (current().type) {
            case TokenType::LET:
            case TokenType::IF:
            case TokenType::FOR:
            case TokenType::WHILE:
            case TokenType::PRINT:
            case TokenType::INPUT:
                return;
            default:
                break;
        }
        
        advance();
    }
}

// AST Node toString implementations
std::string ProgramNode::toString() const {
    std::string result = "Program:\n";
    for (const auto& stmt : statements) {
        result += "  " + stmt->toString() + "\n";
    }
    return result;
}

std::string StatementListNode::toString() const {
    std::string result = "StatementList:\n";
    for (const auto& stmt : statements) {
        result += "  " + stmt->toString() + "\n";
    }
    return result;
}

std::string LetStatementNode::toString() const {
    return "LET " + variableName + " = " + value->toString();
}

std::string IfStatementNode::toString() const {
    std::string result = "IF " + condition->toString() + " THEN " + thenStatement->toString();
    if (elseStatement) {
        result += " ELSE " + elseStatement->toString();
    }
    return result;
}

std::string ForStatementNode::toString() const {
    std::string result = "FOR " + variableName + " = " + startValue->toString() + 
                        " TO " + endValue->toString();
    if (stepValue) {
        result += " STEP " + stepValue->toString();
    }
    result += " " + body->toString();
    return result;
}

std::string NextStatementNode::toString() const {
    std::string result = "NEXT";
    return result;
}


std::string WhileStatementNode::toString() const {
    return "WHILE " + condition->toString() + " " + body->toString();
}

std::string PrintStatementNode::toString() const {
    std::string result = "PRINT ";
    for (size_t i = 0; i < expressions.size(); ++i) {
        if (i > 0) result += ", ";
        result += expressions[i]->toString();
    }
    return result;
}

std::string InputStatementNode::toString() const {
    std::string result = "INPUT ";
    if (!prompt.empty()) {
        result += "\"" + prompt + "\", ";
    }
    result += variableName;
    return result;
}

std::string FunctionCallNode::toString() const {
    std::string result = functionName + "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) result += ", ";
        result += arguments[i]->toString();
    }
    result += ")";
    return result;
}

std::string BinaryExpressionNode::toString() const {
    return "(" + left->toString() + " " + std::to_string(static_cast<int>(operator_)) + " " + right->toString() + ")";
}

std::string UnaryExpressionNode::toString() const {
    return "(" + std::to_string(static_cast<int>(operator_)) + " " + operand->toString() + ")";
}

std::string LiteralNode::toString() const {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + v + "\"";
        } else {
            return std::to_string(v);
        }
    }, value);
}

std::string IdentifierNode::toString() const {
    return name;
}

} // namespace basic 