#include "interpreter/basic_interpreter.h"
#include "interpreter/parser.h"
#include "interpreter/lexer.h"
#include "interpreter/runtime.h"
#include "interpreter/variables.h"
#include "interpreter/functions.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>
#include <chrono>
#include <thread>

namespace basic {

BasicInterpreter::BasicInterpreter() 
    : currentLine_(0), running_(false), debugging_(false), paused_(false) {
    
    parser_ = std::make_unique<Parser>();
    lexer_ = std::make_unique<Lexer>();
    runtime_ = std::make_unique<Runtime>();
    variables_ = std::make_unique<Variables>();
    functions_ = std::make_unique<Functions>();
}

BasicInterpreter::~BasicInterpreter() = default;

bool BasicInterpreter::loadProgram(const std::string& source) {
    source_ = source;
    lines_.clear();
    
    std::istringstream iss(source);
    std::string line;
    while (std::getline(iss, line)) {
        lines_.push_back(line);
    }
    
    currentLine_ = 0;
    lastError_.clear();
    
    // Parse the program
    try {
        auto tokens = lexer_->tokenize(source);
        auto ast = parser_->parse(tokens);
        if (!ast) {
            lastError_ = "Failed to parse program";
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        lastError_ = e.what();
        return false;
    }
}

bool BasicInterpreter::execute() {
    if (lines_.empty()) {
        lastError_ = "No program loaded";
        return false;
    }
    
    running_ = true;
    currentLine_ = 0;
    lastError_.clear();
    
    try {
        while (running_ && currentLine_ < lines_.size()) {
            // Check for breakpoints
            if (debugging_ && breakpoints_.find(currentLine_ + 1) != breakpoints_.end()) {
                paused_ = true;
                while (paused_ && running_) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
            }
            
            if (!running_) break;
            
            std::string line = lines_[currentLine_];
            if (!line.empty()) {
                if (!executeLine(line)) {
                    return false;
                }
            }
            
            currentLine_++;
        }
    } catch (const std::exception& e) {
        lastError_ = e.what();
        running_ = false;
        return false;
    }
    
    running_ = false;
    return true;
}

bool BasicInterpreter::executeLine(const std::string& line) {
    if (line.empty()) return true;
    
    try {
        // Remove line number if present
        std::string code = line;
        std::regex lineNumRegex(R"(^\s*(\d+)\s*(.*))");
        std::smatch match;
        if (std::regex_match(line, match, lineNumRegex)) {
            code = match[2].str();
        }
        
        // Skip empty lines and comments
        if (code.empty() || code[0] == '\'') {
            return true;
        }
        
        // Tokenize the line
        auto tokens = lexer_->tokenize(code);
        if (tokens.empty()) return true;
        
        // Parse the line
        auto ast = parser_->parseLine(tokens);
        if (!ast) {
            lastError_ = "Failed to parse line: " + line;
            return false;
        }
        
        // Execute the line
        Value result = runtime_->execute(ast.get(), variables_.get(), functions_.get());
        
        return true;
    } catch (const std::exception& e) {
        lastError_ = "Error executing line: " + std::string(e.what());
        return false;
    }
}

Value BasicInterpreter::evaluateExpression(const std::string& expr) {
    // Tokenize
    auto tokens = lexer_->tokenize(expr);
    if (tokens.empty()) throw std::runtime_error("Empty or invalid expression");
    // Parse
    auto ast = parser_->parseLine(tokens);
    if (!ast) throw std::runtime_error("Failed to parse expression");
    // Execute
    return runtime_->execute(ast.get(), variables_.get(), functions_.get());
}

void BasicInterpreter::setBreakpoint(int line) {
    breakpoints_.insert(line);
}

void BasicInterpreter::removeBreakpoint(int line) {
    breakpoints_.erase(line);
}

void BasicInterpreter::step() {
    if (paused_) {
        paused_ = false;
        // Execute one line
        if (currentLine_ < lines_.size()) {
            executeLine(lines_[currentLine_]);
            currentLine_++;
        }
        paused_ = true;
    }
}

void BasicInterpreter::continueExecution() {
    paused_ = false;
}

void BasicInterpreter::pause() {
    paused_ = true;
}

void BasicInterpreter::setVariable(const std::string& name, const Value& value) {
    variables_->set(name, value);
}

Value BasicInterpreter::getVariable(const std::string& name) {
    return variables_->get(name);
}

std::map<std::string, Value> BasicInterpreter::getAllVariables() {
    return variables_->getAll();
}

void BasicInterpreter::defineFunction(const std::string& name, const std::string& body) {
    functions_->define(name, body);
}

Value BasicInterpreter::callFunction(const std::string& name, const std::vector<Value>& args) {
    return functions_->call(name, args, variables_.get());
}

bool BasicInterpreter::isRunning() const {
    return running_;
}

int BasicInterpreter::getCurrentLine() const {
    return currentLine_ + 1; // Convert to 1-based line numbers
}

std::string BasicInterpreter::getCurrentSource() const {
    return source_;
}

std::string BasicInterpreter::getLastError() const {
    return lastError_;
}

void BasicInterpreter::clearError() {
    lastError_.clear();
}

} // namespace basic 