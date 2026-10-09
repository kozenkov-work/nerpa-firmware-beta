#pragma once
#include <variant>
#include <iostream>
#include <string>

class Value {

public:

    using Data = std::variant<int, std::string, double, bool>;

    Value(int value);
    Value(std::string value);
    Value(double value);
    Value(bool value);

    int getInt() const;
    double getDouble() const;
    std::string getString() const;
    bool getBool() const;
 
    friend std::ostream& operator<<(std::ostream& os, const Value& val) {
        std::visit([&os](const auto& item) {
            using T = std::decay_t<decltype(item)>;
            if constexpr (std::is_same_v<T, bool>) {
                os << (item ? "true" : "false");
            } else {
                os << item;
            }
        }, val.data_);

        return os;
    }
private:
    Data data_;
};