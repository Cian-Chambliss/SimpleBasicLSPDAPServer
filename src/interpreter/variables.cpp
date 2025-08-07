#include "interpreter/variables.h"
#include <stdexcept>

namespace basic {

Variables::Variables() {}

void Variables::set(const std::string& name, const Value& value) {
    variables_[name] = value;
}

Value Variables::get(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }
    // Return default value (0) for undefined variables
    return Value{0};
}

std::map<std::string, Value> Variables::getAll() const {
    return variables_;
}

void Variables::clear() {
    variables_.clear();
}

bool Variables::exists(const std::string& name) const {
    return variables_.find(name) != variables_.end();
}

} // namespace basic 