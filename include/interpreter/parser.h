#pragma once

#include "interpreter/basic_interpreter.h"
#include <memory>
#include <vector>

namespace basic {

// AST Node implementations
class ProgramNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    NodeType getType() const override { return NodeType::PROGRAM; }
    std::string toString() const override;
};

class StatementListNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    NodeType getType() const override { return NodeType::STATEMENT_LIST; }
    std::string toString() const override;
};

class LetStatementNode : public ASTNode {
public:
    std::string variableName;
    std::unique_ptr<ASTNode> value;
    
    NodeType getType() const override { return NodeType::LET_STATEMENT; }
    std::string toString() const override;
};

class IfStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> thenStatement;
    std::unique_ptr<ASTNode> elseStatement;
    
    NodeType getType() const override { return NodeType::IF_STATEMENT; }
    std::string toString() const override;
};

class ForStatementNode : public ASTNode {
public:
    std::string variableName;
    std::unique_ptr<ASTNode> startValue;
    std::unique_ptr<ASTNode> endValue;
    std::unique_ptr<ASTNode> stepValue;
    std::unique_ptr<ASTNode> body;
    
    NodeType getType() const override { return NodeType::FOR_STATEMENT; }
    std::string toString() const override;
};

class WhileStatementNode : public ASTNode {
public:
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> body;
    
    NodeType getType() const override { return NodeType::WHILE_STATEMENT; }
    std::string toString() const override;
};

class PrintStatementNode : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> expressions;
    
    NodeType getType() const override { return NodeType::PRINT_STATEMENT; }
    std::string toString() const override;
};

class InputStatementNode : public ASTNode {
public:
    std::string prompt;
    std::string variableName;
    
    NodeType getType() const override { return NodeType::INPUT_STATEMENT; }
    std::string toString() const override;
};

class FunctionCallNode : public ASTNode {
public:
    std::string functionName;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    
    NodeType getType() const override { return NodeType::FUNCTION_CALL; }
    std::string toString() const override;
};

class BinaryExpressionNode : public ASTNode {
public:
    TokenType operator_;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    
    NodeType getType() const override { return NodeType::BINARY_EXPRESSION; }
    std::string toString() const override;
};

class UnaryExpressionNode : public ASTNode {
public:
    TokenType operator_;
    std::unique_ptr<ASTNode> operand;
    
    NodeType getType() const override { return NodeType::UNARY_EXPRESSION; }
    std::string toString() const override;
};

class LiteralNode : public ASTNode {
public:
    Value value;
    
    NodeType getType() const override { return NodeType::LITERAL; }
    std::string toString() const override;
};

class IdentifierNode : public ASTNode {
public:
    std::string name;
    
    NodeType getType() const override { return NodeType::IDENTIFIER; }
    std::string toString() const override;
};

class Parser {
public:
    Parser();
    
    std::unique_ptr<ASTNode> parse(const std::vector<Token>& tokens);
    std::unique_ptr<ASTNode> parseLine(const std::vector<Token>& tokens);

private:
    std::vector<Token> tokens_;
    size_t current_;
    
    // Parsing methods
    std::unique_ptr<ASTNode> parseProgram();
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseLetStatement();
    std::unique_ptr<ASTNode> parseIfStatement();
    std::unique_ptr<ASTNode> parseForStatement();
    std::unique_ptr<ASTNode> parseWhileStatement();
    std::unique_ptr<ASTNode> parsePrintStatement();
    std::unique_ptr<ASTNode> parseInputStatement();
    std::unique_ptr<ASTNode> parseExpression();
    std::unique_ptr<ASTNode> parseTerm();
    std::unique_ptr<ASTNode> parseFactor();
    std::unique_ptr<ASTNode> parsePrimary();
    std::unique_ptr<ASTNode> parseFunctionCall();
    
    // Helper methods
    Token current() const;
    Token peek() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool match(TokenType type);
    void advance();
    void consume(TokenType type, const std::string& message);
    void synchronize();
};

} // namespace basic 