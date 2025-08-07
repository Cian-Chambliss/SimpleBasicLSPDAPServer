#include "interpreter/runtime.h"
#include "interpreter/variables.h"
#include "interpreter/functions.h"
#include "interpreter/parser.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

// Add these includes for DAP OutputEvent support
#include "dap/dap_server.h"
#include <sstream>

namespace basic {

// Add a pointer to the DAPServer (optional: set via constructor or setter)
static dap::DAPServer* g_dapServer = nullptr;
void setDAPServer(dap::DAPServer* server) { g_dapServer = server; }

Runtime::Runtime() {}

Value Runtime::execute(const ASTNode* node, Variables* variables, Functions* functions) {
    if (!node) return Value{};
    
    switch (node->getType()) {
        case NodeType::PROGRAM:
            return executeProgram(static_cast<const ProgramNode*>(node), variables, functions);
        case NodeType::LET_STATEMENT:
            return executeLetStatement(static_cast<const LetStatementNode*>(node), variables, functions);
        case NodeType::IF_STATEMENT:
            return executeIfStatement(static_cast<const IfStatementNode*>(node), variables, functions);
        case NodeType::FOR_STATEMENT:
            return executeForStatement(static_cast<const ForStatementNode*>(node), variables, functions);
        case NodeType::WHILE_STATEMENT:
            return executeWhileStatement(static_cast<const WhileStatementNode*>(node), variables, functions);
        case NodeType::PRINT_STATEMENT:
            return executePrintStatement(static_cast<const PrintStatementNode*>(node), variables, functions);
        case NodeType::INPUT_STATEMENT:
            return executeInputStatement(static_cast<const InputStatementNode*>(node), variables, functions);
        case NodeType::FUNCTION_CALL:
            return executeFunctionCall(static_cast<const FunctionCallNode*>(node), variables, functions);
        case NodeType::BINARY_EXPRESSION:
            return executeBinaryExpression(static_cast<const BinaryExpressionNode*>(node), variables, functions);
        case NodeType::UNARY_EXPRESSION:
            return executeUnaryExpression(static_cast<const UnaryExpressionNode*>(node), variables, functions);
        case NodeType::LITERAL:
            return executeLiteral(static_cast<const LiteralNode*>(node), variables, functions);
        case NodeType::IDENTIFIER:
            return executeIdentifier(static_cast<const IdentifierNode*>(node), variables, functions);
        default:
            return Value{};
    }
}

Value Runtime::executeProgram(const ProgramNode* node, Variables* variables, Functions* functions) {
    Value result;
    for (const auto& statement : node->statements) {
        result = this->execute(statement.get(), variables, functions);
    }
    return result;
}

Value Runtime::executeLetStatement(const LetStatementNode* node, Variables* variables, Functions* functions) {
    Value value = this->execute(node->value.get(), variables, functions);
    variables->set(node->variableName, value);
    return value;
}

Value Runtime::executeIfStatement(const IfStatementNode* node, Variables* variables, Functions* functions) {
    Value condition = this->execute(node->condition.get(), variables, functions);
    
    if (this->isTruthy(condition)) {
        return this->execute(node->thenStatement.get(), variables, functions);
    } else if (node->elseStatement) {
        return this->execute(node->elseStatement.get(), variables, functions);
    }
    
    return Value{};
}

Value Runtime::executeForStatement(const ForStatementNode* node, Variables* variables, Functions* functions) {
    Value start = this->execute(node->startValue.get(), variables, functions);
    Value end = this->execute(node->endValue.get(), variables, functions);
    Value step = node->stepValue ? this->execute(node->stepValue.get(), variables, functions) : Value{1};
    
    // Convert to numeric values
    double startVal = std::visit([](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, double>) return v;
        else return 0.0;
    }, start);
    
    double endVal = std::visit([](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, double>) return v;
        else return 0.0;
    }, end);
    
    double stepVal = std::visit([](const auto& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return static_cast<double>(v);
        else if constexpr (std::is_same_v<T, double>) return v;
        else return 1.0;
    }, step);
    
    variables->set(node->variableName, Value{startVal});
    
    while (true) {
        Value current = variables->get(node->variableName);
        double currentVal = std::visit([](const auto& v) -> double {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, int>) return static_cast<double>(v);
            else if constexpr (std::is_same_v<T, double>) return v;
            else return 0.0;
        }, current);
        
        if (stepVal > 0 && currentVal > endVal) break;
        if (stepVal < 0 && currentVal < endVal) break;
        
        this->execute(node->body.get(), variables, functions);
        
        currentVal += stepVal;
        variables->set(node->variableName, Value{currentVal});
    }
    
    return Value{};
}

Value Runtime::executeWhileStatement(const WhileStatementNode* node, Variables* variables, Functions* functions) {
    while (this->isTruthy(this->execute(node->condition.get(), variables, functions))) {
        this->execute(node->body.get(), variables, functions);
    }
    return Value{};
}

Value Runtime::executePrintStatement(const PrintStatementNode* node, Variables* variables, Functions* functions) {
    std::ostringstream oss;
    for (size_t i = 0; i < node->expressions.size(); ++i) {
        Value value = this->execute(node->expressions[i].get(), variables, functions);

        std::visit([&oss](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::string>) {
                oss << v;
            } else {
                oss << v;
            }
        }, value);

        if (i < node->expressions.size() - 1) {
            oss << " ";
        }
    }
    oss << std::endl;

    // Output to DAP OutputEvent if running under DAP
    if (g_dapServer && g_dapServer->isRunning()) {
        g_dapServer->sendOutputEvent(oss.str(), "stdout");
    } else {
        std::cout << oss.str();
    }
    return Value{};
}

Value Runtime::executeInputStatement(const InputStatementNode* node, Variables* variables, Functions* functions) {
    if (!node->prompt.empty()) {
        std::cout << node->prompt;
    }
    
    std::string input;
    std::getline(std::cin, input);
    
    // Try to convert to number if possible
    try {
        if (input.find('.') != std::string::npos) {
            double value = std::stod(input);
            variables->set(node->variableName, Value{value});
        } else {
            int value = std::stoi(input);
            variables->set(node->variableName, Value{value});
        }
    } catch (...) {
        variables->set(node->variableName, Value{input});
    }
    
    return Value{};
}

Value Runtime::executeFunctionCall(const FunctionCallNode* node, Variables* variables, Functions* functions) {
    std::vector<Value> args;
    for (const auto& arg : node->arguments) {
        args.push_back(this->execute(arg.get(), variables, functions));
    }
    
    return functions->call(node->functionName, args, variables);
}

Value Runtime::executeBinaryExpression(const BinaryExpressionNode* node, Variables* variables, Functions* functions) {
    Value left = this->execute(node->left.get(), variables, functions);
    Value right = this->execute(node->right.get(), variables, functions);
    
    switch (node->operator_) {
        case TokenType::PLUS:
            return this->add(left, right);
        case TokenType::MINUS:
            return this->subtract(left, right);
        case TokenType::MULTIPLY:
            return this->multiply(left, right);
        case TokenType::DIVIDE:
            return this->divide(left, right);
        case TokenType::MOD:
            return this->modulo(left, right);
        case TokenType::POWER:
            return this->power(left, right);
        case TokenType::EQUAL:
            return Value{this->isEqual(left, right)};
        case TokenType::NOT_EQUAL:
            return Value{!this->isEqual(left, right)};
        case TokenType::LESS:
            return Value{this->isLessThan(left, right)};
        case TokenType::LESS_EQUAL:
            return Value{this->isLessThan(left, right) || this->isEqual(left, right)};
        case TokenType::GREATER:
            return Value{this->isGreaterThan(left, right)};
        case TokenType::GREATER_EQUAL:
            return Value{this->isGreaterThan(left, right) || this->isEqual(left, right)};
        default:
            return Value{};
    }
}

Value Runtime::executeUnaryExpression(const UnaryExpressionNode* node, Variables* variables, Functions* functions) {
    Value operand = this->execute(node->operand.get(), variables, functions);
    
    switch (node->operator_) {
        case TokenType::MINUS:
            return std::visit([](const auto& v) -> Value {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, int>) return Value{-v};
                else if constexpr (std::is_same_v<T, double>) return Value{-v};
                else return Value{0};
            }, operand);
        case TokenType::NOT:
            return Value{!this->isTruthy(operand)};
        default:
            return operand;
    }
}

Value Runtime::executeLiteral(const LiteralNode* node, Variables* variables, Functions* functions) {
    return node->value;
}

Value Runtime::executeIdentifier(const IdentifierNode* node, Variables* variables, Functions* functions) {
    return variables->get(node->name);
}

bool Runtime::isTruthy(const Value& value) {
    return std::visit([](const auto& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, bool>) return v;
        else if constexpr (std::is_same_v<T, int>) return v != 0;
        else if constexpr (std::is_same_v<T, double>) return v != 0.0;
        else if constexpr (std::is_same_v<T, std::string>) return !v.empty();
        else return false;
    }, value);
}

bool Runtime::isEqual(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> bool {
        return std::visit([&va](const auto& vb) -> bool {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr (std::is_same_v<Ta, Tb>) {
                if constexpr (std::is_same_v<Ta, std::string>) {
                    return va == vb;
                } else if constexpr (std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) {
                    return static_cast<double>(va) == static_cast<double>(vb);
                } else if constexpr (std::is_same_v<Ta, bool>) {
                    return va == vb;
                } else {
                    return false;
                }
            } else {
                // Convert to double for comparison
                double da = 0.0, db = 0.0;
                if constexpr (std::is_same_v<Ta, int>) da = static_cast<double>(va);
                else if constexpr (std::is_same_v<Ta, double>) da = va;
                if constexpr (std::is_same_v<Tb, int>) db = static_cast<double>(vb);
                else if constexpr (std::is_same_v<Tb, double>) db = vb;
                return da == db;
            }
        }, b);
    }, a);
}

bool Runtime::isLessThan(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> bool {
        return std::visit([&va](const auto& vb) -> bool {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr (std::is_same_v<Ta, Tb>) {
                return va < vb;
            } else {
                // Convert to double for comparison
                double da = 0.0, db = 0.0;
                if constexpr (std::is_same_v<Ta, int>) da = static_cast<double>(va);
                else if constexpr (std::is_same_v<Ta, double>) da = va;
                if constexpr (std::is_same_v<Tb, int>) db = static_cast<double>(vb);
                else if constexpr (std::is_same_v<Tb, double>) db = vb;
                return da < db;
            }
        }, b);
    }, a);
}

bool Runtime::isGreaterThan(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> bool {
        return std::visit([&va](const auto& vb) -> bool {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr (std::is_same_v<Ta, Tb>) {
                return va > vb;
            } else {
                // Convert to double for comparison
                double da = 0.0, db = 0.0;
                if constexpr (std::is_same_v<Ta, int>) da = static_cast<double>(va);
                else if constexpr (std::is_same_v<Ta, double>) da = va;
                if constexpr (std::is_same_v<Tb, int>) db = static_cast<double>(vb);
                else if constexpr (std::is_same_v<Tb, double>) db = vb;
                return da > db;
            }
        }, b);
    }, a);
}

Value Runtime::add(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr (std::is_same_v<Ta, std::string> && std::is_same_v<Tb, std::string>) {
                return Value{va + vb};
            } else if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                                (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                return Value{da + db};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

Value Runtime::subtract(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                         (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                return Value{da - db};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

Value Runtime::multiply(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                         (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                return Value{da * db};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

Value Runtime::divide(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                         (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                if (db == 0.0) {
                    throw std::runtime_error("Division by zero");
                }
                return Value{da / db};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

Value Runtime::modulo(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                         (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                if (db == 0.0) {
                    throw std::runtime_error("Modulo by zero");
                }
                return Value{std::fmod(da, db)};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

Value Runtime::power(const Value& a, const Value& b) {
    return std::visit([&b](const auto& va) -> Value {
        return std::visit([&va](const auto& vb) -> Value {
            using Ta = std::decay_t<decltype(va)>;
            using Tb = std::decay_t<decltype(vb)>;
            if constexpr ((std::is_same_v<Ta, int> || std::is_same_v<Ta, double>) && 
                         (std::is_same_v<Tb, int> || std::is_same_v<Tb, double>)) {
                double da = static_cast<double>(va);
                double db = static_cast<double>(vb);
                return Value{std::pow(da, db)};
            } else {
                return Value{0.0};
            }
        }, b);
    }, a);
}

} // namespace basic