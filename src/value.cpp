#include "value.h"

Value::Value(int value) : data_(value) {}
Value::Value(bool value) : data_(value) {}
Value::Value(std::string value) : data_(value) {}
Value::Value(float value) : data_(value) {}

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

float Value::getFloat() const {
    if (const float* value = std::get_if<float>(&data_)) return *value;
    throw std::runtime_error("Value is not double");
}

