#include "interpreter/functions.h"
#include "interpreter/variables.h"
#include "interpreter/lexer.h"
#include "interpreter/parser.h"
#include "interpreter/runtime.h"
#include <cmath>
#include <stdexcept>
#include <sstream>

namespace basic {

Functions::Functions() {}

void Functions::define(const std::string& name, const std::string& body) {
    functions_[name] = body;
}

Value Functions::call(const std::string& name, const std::vector<Value>& args, Variables* variables) {
    // First check if it's a built-in function
    Value builtinResult = callBuiltin(name, args);
    if (builtinResult.index() != 0) { // Not empty
        return builtinResult;
    }
    
    // Check if it's a user-defined function
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        // For now, we'll return a simple implementation
        // In a full implementation, you'd want to parse and execute the function body
        return Value{0};
    }
    if (args.size() == 0) {
        // Check for variables....
        if (variables->exists(name)) {
            return variables->get(name);
        }
        throw std::runtime_error("Symbol '" + name + "' not defined");
    }
    
    throw std::runtime_error("Function '" + name + "' not defined");
}

std::map<std::string, std::string> Functions::getAll() const {
    return functions_;
}

void Functions::clear() {
    functions_.clear();
}

bool Functions::exists(const std::string& name) const {
    return functions_.find(name) != functions_.end();
}

Value Functions::callBuiltin(const std::string& name, const std::vector<Value>& args) {
    if (name == "ABS") return abs(args);
    if (name == "SIN") return sin(args);
    if (name == "COS") return cos(args);
    if (name == "TAN") return tan(args);
    if (name == "SQRT") return sqrt(args);
    if (name == "LOG") return log(args);
    if (name == "EXP") return exp(args);
    if (name == "LEN") return len(args);
    if (name == "MID") return mid(args);
    if (name == "LEFT") return left(args);
    if (name == "RIGHT") return right(args);
    if (name == "VAL") return val(args);
    if (name == "STR") return str(args);
    
    return Value{}; // Empty value indicates not a built-in function
}

Value Functions::abs(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("ABS function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return Value{std::abs(v)};
        else if constexpr (std::is_same_v<T, double>) return Value{std::abs(v)};
        else return Value{0};
    }, args[0]);
}

Value Functions::sin(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("SIN function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            return Value{std::sin(static_cast<double>(v))};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::cos(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("COS function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            return Value{std::cos(static_cast<double>(v))};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::tan(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("TAN function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            return Value{std::tan(static_cast<double>(v))};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::sqrt(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("SQRT function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            double val = static_cast<double>(v);
            if (val < 0) {
                throw std::runtime_error("SQRT of negative number");
            }
            return Value{std::sqrt(val)};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::log(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("LOG function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            double val = static_cast<double>(v);
            if (val <= 0) {
                throw std::runtime_error("LOG of non-positive number");
            }
            return Value{std::log(val)};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::exp(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("EXP function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            return Value{std::exp(static_cast<double>(v))};
        } else {
            return Value{0.0};
        }
    }, args[0]);
}

Value Functions::len(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("LEN function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return Value{static_cast<int>(v.length())};
        } else {
            return Value{0};
        }
    }, args[0]);
}

Value Functions::mid(const std::vector<Value>& args) {
    if (args.size() < 2 || args.size() > 3) {
        throw std::runtime_error("MID function requires 2 or 3 arguments");
    }
    
    std::string str = std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) return v;
        else return "";
    }, args[0]);
    
    int start = std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return v;
        else if constexpr (std::is_same_v<T, double>) return static_cast<int>(v);
        else return 0;
    }, args[1]);
    
    int length = args.size() == 3 ? std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return v;
        else if constexpr (std::is_same_v<T, double>) return static_cast<int>(v);
        else return 0;
    }, args[2]) : str.length() - start + 1;
    
    if (start < 1 || start > static_cast<int>(str.length())) {
        return Value{""};
    }
    
    size_t startPos = start - 1; // Convert to 0-based
    size_t endPos = std::min(startPos + length, str.length());
    
    return Value{str.substr(startPos, endPos - startPos)};
}

Value Functions::left(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw std::runtime_error("LEFT function requires exactly 2 arguments");
    }
    
    std::string str = std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) return v;
        else return "";
    }, args[0]);
    
    int length = std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return v;
        else if constexpr (std::is_same_v<T, double>) return static_cast<int>(v);
        else return 0;
    }, args[1]);
    
    if (length <= 0) return Value{""};
    if (length >= static_cast<int>(str.length())) return Value{str};
    
    return Value{str.substr(0, length)};
}

Value Functions::right(const std::vector<Value>& args) {
    if (args.size() != 2) {
        throw std::runtime_error("RIGHT function requires exactly 2 arguments");
    }
    
    std::string str = std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) return v;
        else return "";
    }, args[0]);
    
    int length = std::visit([](const auto& v) -> int {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int>) return v;
        else if constexpr (std::is_same_v<T, double>) return static_cast<int>(v);
        else return 0;
    }, args[1]);
    
    if (length <= 0) return Value{""};
    if (length >= static_cast<int>(str.length())) return Value{str};
    
    return Value{str.substr(str.length() - length)};
}

Value Functions::val(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("VAL function requires exactly 1 argument");
    }
    
    std::string str = std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) return v;
        else return "";
    }, args[0]);
    
    try {
        if (str.find('.') != std::string::npos) {
            return Value{std::stod(str)};
        } else {
            return Value{std::stoi(str)};
        }
    } catch (...) {
        return Value{0};
    }
}

Value Functions::str(const std::vector<Value>& args) {
    if (args.size() != 1) {
        throw std::runtime_error("STR function requires exactly 1 argument");
    }
    
    return std::visit([](const auto& v) -> Value {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) return v;
        else {
            std::ostringstream oss;
            oss << v;
            return Value{oss.str()};
        }
    }, args[0]);
}

} // namespace basic 