#pragma once

#include "interpreter/basic_interpreter.h"
#include <memory>

namespace basic {

// Global functions for DAP integration
void setInterpreter(BasicInterpreter* interpreter);
BasicInterpreter* getInterpreter();

// Helper function to convert Value to string for DAP
std::string valueToString(const Value& value);

// Forward declarations
class Variables;
class Functions;
class ProgramNode;
class LetStatementNode;
class IfStatementNode;
class ForStatementNode;
class NextStatementNode;
class WhileStatementNode;
class PrintStatementNode;
class InputStatementNode;
class FunctionCallNode;
class BinaryExpressionNode;
class UnaryExpressionNode;
class LiteralNode;
class IdentifierNode;

class RuntimeBlock {
public:
    // For loop for now - TBD add others
    int line;
    std::string variableName;
    double currentVal;
    double endVal;
    double stepVal;
    RuntimeBlock(std::string name,double _current, double _end, double _step) : line(0) , variableName(name), currentVal(_current), endVal(_end), stepVal(_step) {
    }
};

class Runtime {
public:
    std::vector< std::unique_ptr<RuntimeBlock> > block;
    Runtime();
    
    Value execute(const ASTNode* node, Variables* variables, Functions* functions);
    
private:
    Value executeProgram(const ProgramNode* node, Variables* variables, Functions* functions);
    Value executeLetStatement(const LetStatementNode* node, Variables* variables, Functions* functions);
    Value executeIfStatement(const IfStatementNode* node, Variables* variables, Functions* functions);
    Value executeForStatement(const ForStatementNode* node, Variables* variables, Functions* functions);
    Value executeNextStatement(const NextStatementNode* node, Variables* variables, Functions* functions);
    Value executeWhileStatement(const WhileStatementNode* node, Variables* variables, Functions* functions);
    Value executePrintStatement(const PrintStatementNode* node, Variables* variables, Functions* functions);
    Value executeInputStatement(const InputStatementNode* node, Variables* variables, Functions* functions);
    Value executeFunctionCall(const FunctionCallNode* node, Variables* variables, Functions* functions);
    Value executeBinaryExpression(const BinaryExpressionNode* node, Variables* variables, Functions* functions);
    Value executeUnaryExpression(const UnaryExpressionNode* node, Variables* variables, Functions* functions);
    Value executeLiteral(const LiteralNode* node, Variables* variables, Functions* functions);
    Value executeIdentifier(const IdentifierNode* node, Variables* variables, Functions* functions);
    
    // Helper methods
    bool isTruthy(const Value& value);
    bool isEqual(const Value& a, const Value& b);
    bool isLessThan(const Value& a, const Value& b);
    bool isGreaterThan(const Value& a, const Value& b);
    Value add(const Value& a, const Value& b);
    Value subtract(const Value& a, const Value& b);
    Value multiply(const Value& a, const Value& b);
    Value divide(const Value& a, const Value& b);
    Value modulo(const Value& a, const Value& b);
    Value power(const Value& a, const Value& b);
};
} // namespace basic 