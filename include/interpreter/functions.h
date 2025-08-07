#pragma once

#include "interpreter/basic_interpreter.h"
#include <map>
#include <string>
#include <vector>

namespace basic {

class Variables;

class Functions {
public:
    Functions();
    
    void define(const std::string& name, const std::string& body);
    Value call(const std::string& name, const std::vector<Value>& args, Variables* variables);
    std::map<std::string, std::string> getAll() const;
    void clear();
    bool exists(const std::string& name) const;
    
private:
    std::map<std::string, std::string> functions_;
    
    // Built-in functions
    Value callBuiltin(const std::string& name, const std::vector<Value>& args);
    Value abs(const std::vector<Value>& args);
    Value sin(const std::vector<Value>& args);
    Value cos(const std::vector<Value>& args);
    Value tan(const std::vector<Value>& args);
    Value sqrt(const std::vector<Value>& args);
    Value log(const std::vector<Value>& args);
    Value exp(const std::vector<Value>& args);
    Value len(const std::vector<Value>& args);
    Value mid(const std::vector<Value>& args);
    Value left(const std::vector<Value>& args);
    Value right(const std::vector<Value>& args);
    Value val(const std::vector<Value>& args);
    Value str(const std::vector<Value>& args);
};

} // namespace basic 