#pragma once

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <variant>
#include <optional>
#include <set> // Added for breakpoints

namespace basic {

// Forward declarations
class ASTNode;
class Parser;
class Lexer;
class Runtime;
class Variables;
class Functions;

// Value types
using Value = std::variant<int, double, std::string, bool>;

// Token types
enum class TokenType {
    // Keywords
    LET, IF, THEN, ELSE, FOR, TO, STEP, NEXT, WHILE, WEND, DO, LOOP, UNTIL,
    SUB, END, FUNCTION, RETURN, PRINT, INPUT, READ, DATA, RESTORE, DIM,
    
    // Operators
    PLUS, MINUS, MULTIPLY, DIVIDE, MOD, POWER,
    EQUAL, NOT_EQUAL, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    AND, OR, NOT,
    
    // Delimiters
    LPAREN, RPAREN, COMMA, SEMICOLON, COLON, ASSIGN,
    
    // Literals
    NUMBER, STRING, IDENTIFIER,
    
    // Special
    NEWLINE, EOF_TOKEN, UNKNOWN
};

// Token structure
struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l, int c) 
        : type(t), value(v), line(l), column(c) {}
};

// AST Node types
enum class NodeType {
    PROGRAM, STATEMENT_LIST, STATEMENT,
    LET_STATEMENT, IF_STATEMENT, FOR_STATEMENT, WHILE_STATEMENT,
    PRINT_STATEMENT, INPUT_STATEMENT, FUNCTION_CALL, SUB_CALL,
    BINARY_EXPRESSION, UNARY_EXPRESSION, LITERAL, IDENTIFIER,
    VARIABLE_DECLARATION, ARRAY_ACCESS
};

// AST Node base class
class ASTNode {
public:
    int line;
    
    ASTNode() : line(0) {}
    virtual ~ASTNode() = default;
    virtual NodeType getType() const = 0;
    virtual std::string toString() const = 0;
};

// Main interpreter class
class BasicInterpreter {
public:
    BasicInterpreter();
    ~BasicInterpreter();
    
    // Main execution methods
    bool loadProgram(const std::string& source);
    bool execute();
    bool executeLine(const std::string& line);
    
    // Debugging support
    void setBreakpoint(int line);
    void removeBreakpoint(int line);
    void step();
    void continueExecution();
    void pause();
    
    // Variable management
    void setVariable(const std::string& name, const Value& value);
    Value getVariable(const std::string& name);
    std::map<std::string, Value> getAllVariables();
    
    // Function management
    void defineFunction(const std::string& name, const std::string& body);
    Value callFunction(const std::string& name, const std::vector<Value>& args);
    
    // Runtime state
    bool isRunning() const;
    int getCurrentLine() const;
    std::string getCurrentSource() const;
    
    // Error handling
    std::string getLastError() const;
    void clearError();

    Value evaluateExpression(const std::string& expr);

private:
    std::unique_ptr<Parser> parser_;
    std::unique_ptr<Lexer> lexer_;
    std::unique_ptr<Runtime> runtime_;
    std::unique_ptr<Variables> variables_;
    std::unique_ptr<Functions> functions_;
    
    std::string source_;
    std::vector<std::string> lines_;
    int currentLine_;
    bool running_;
    std::string lastError_;
    
    // Debug state
    bool debugging_;
    std::set<int> breakpoints_;
    bool paused_;
};

} // namespace basic 