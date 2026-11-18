#include "stack.h"

#include <cmath>
#include <iostream>
#include <stdexcept>

void Stack::push(Value value) {
    stack_.push_back(std::move(value));
}

void Stack::dup() {
    if (empty()) throw std::runtime_error("DUP: stack is empty");
    stack_.push_back(stack_.back());
}

void Stack::print() const {
    for (const Value& value : stack_) std::cout << value << '\n';
}

bool Stack::empty() const {
    return stack_.empty();
}

Value Stack::top() const {
    if (empty()) throw std::runtime_error("Stack is empty");
    return stack_.back();
}

Value Stack::pop() {
    if (empty()) throw std::runtime_error("Stack is empty");
    Value value = stack_.back();
    stack_.pop_back();
    return value;
}

void Stack::math(int operationId) {
    if (operationId == 7) {
        const bool value = pop().getBool();
        push(!value);
        return;
    }

    if (operationId < 0 || operationId > 6) {
        throw std::runtime_error("MATH: unknown operation ID: " + std::to_string(operationId));
    }
    if (size() < 2) throw std::runtime_error("MATH: expected two operands");

    const Value b = pop();
    const Value a = pop();
    switch (operationId) {
        case 0: push(a.getNumber() + b.getNumber()); break;
        case 1: push(a.getNumber() - b.getNumber()); break;
        case 2: push(a.getNumber() * b.getNumber()); break;
        case 3:
            if (b.getNumber() == 0.0) throw std::runtime_error("MATH: division by zero");
            push(a.getNumber() / b.getNumber());
            break;
        case 4: push(a.getString() + b.getString()); break;
        case 5: push(a.getBool() && b.getBool()); break;
        case 6: push(a.getBool() || b.getBool()); break;
        default: break;
    }
}

void Stack::add() { math(0); }
void Stack::sub() { math(1); }
void Stack::mul() { math(2); }
void Stack::div() { math(3); }

void Stack::mod() {
    if (size() < 2) throw std::runtime_error("MOD: expected two operands");
    const double b = pop().getNumber();
    const double a = pop().getNumber();
    if (b == 0.0) throw std::runtime_error("MOD: division by zero");
    push(std::fmod(a, b));
}

void Stack::eq() {
    if (size() < 2) throw std::runtime_error("EQ: expected two operands");
    const Value b = pop();
    const Value a = pop();
    push(a.equals(b));
}

void Stack::lt() {
    if (size() < 2) throw std::runtime_error("LT: expected two operands");
    const double b = pop().getNumber();
    const double a = pop().getNumber();
    push(a < b);
}

void Stack::gt() {
    if (size() < 2) throw std::runtime_error("GT: expected two operands");
    const double b = pop().getNumber();
    const double a = pop().getNumber();
    push(a > b);
}

std::size_t Stack::size() const {
    return stack_.size();
}

void Stack::clear() {
    stack_.clear();
}
