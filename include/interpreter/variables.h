#pragma once

#include "interpreter/basic_interpreter.h"
#include <map>
#include <string>

namespace basic {

class Variables {
public:
    Variables();
    
    void set(const std::string& name, const Value& value);
    Value get(const std::string& name) const;
    std::map<std::string, Value> getAll() const;
    void clear();
    bool exists(const std::string& name) const;
    
private:
    std::map<std::string, Value> variables_;
};

} // namespace basic 