#include "value.h"

#include <cmath>
#include <limits>
#include <stdexcept>

Value::Value() : data_(std::monostate{}) {}
Value::Value(int value) : data_(static_cast<double>(value)) {}
Value::Value(double value) : data_(value) {}
Value::Value(bool value) : data_(value) {}
Value::Value(std::string value) : data_(std::move(value)) {}

int Value::getInt() const {
    const double value = getNumber();
    if (!std::isfinite(value) || std::trunc(value) != value ||
        value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        throw std::runtime_error("Value is not an integer");
    }
    return static_cast<int>(value);
}

double Value::getNumber() const {
    if (const double* value = std::get_if<double>(&data_)) return *value;
    throw std::runtime_error("Value is not a number");
}

float Value::getFloat() const {
    return static_cast<float>(getNumber());
}

bool Value::getBool() const {
    if (const bool* value = std::get_if<bool>(&data_)) return *value;
    throw std::runtime_error("Value is not bool");
}

std::string Value::getString() const {
    if (const std::string* value = std::get_if<std::string>(&data_)) return *value;
    throw std::runtime_error("Value is not string");
}

bool Value::isNull() const {
    return std::holds_alternative<std::monostate>(data_);
}

bool Value::sameType(const Value& other) const {
    return data_.index() == other.data_.index();
}

bool Value::equals(const Value& other) const {
    return sameType(other) && data_ == other.data_;
}
