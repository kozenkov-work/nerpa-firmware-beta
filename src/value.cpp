#include "value.h"

Value::Value(int value) : data_(value) {}
Value::Value(std::string value) : data_(value) {}
Value::Value(double value) : data_(value) {}

int Value::getInt() const {
    if (const int* value = std::get_if<int>(&data_)) return *value;
    throw std::runtime_error("Value is not int");
}

bool Value::getBool() const {
    if (const bool* value = std::get_if<bool>(&data_)) return *value;
    throw std::runtime_error("Value is not bool");

}

std::string Value::getString() const {
    if (const std::string* value = std::get_if<std::string>(&data_)) return *value;
    throw std::runtime_error("Value is not str");
}

double Value::getDouble() const {
    if (const double* value = std::get_if<double>(&data_)) return *value;
    throw std::runtime_error("Value is not double");
}

