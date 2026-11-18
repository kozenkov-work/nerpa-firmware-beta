#pragma once

#include <iostream>
#include <string>
#include <type_traits>
#include <variant>

class Value {
public:
    using Data = std::variant<double, std::string, bool, std::monostate>;

    Value();
    Value(int value);
    Value(double value);
    Value(std::string value);
    Value(bool value);

    int getInt() const;
    double getNumber() const;
    float getFloat() const;
    std::string getString() const;
    bool getBool() const;
    bool isNull() const;
    bool sameType(const Value& other) const;
    bool equals(const Value& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Value& val) {
        std::visit([&os](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, bool>) {
                os << (item ? "true" : "false");
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                os << "null";
            } else {
                os << item;
            }
        }, val.data_);

        return os;
    }

private:
    Data data_;
};
