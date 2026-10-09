#include "stack.h"
#include <iostream>

Stack::Stack() {}

void Stack::push(Value value) {
    stack_.push_back(value);
}

void Stack::dup() {
    if (empty()) return;
    stack_.push_back(stack_.back());
}

void Stack::print() const {
    for(size_t i = 0; i < stack_.size(); i++) std::cout << 
    stack_[i]
    << std::endl;
}

bool Stack::empty() const {
    return stack_.empty();
}

Value Stack::pop() {
    if(empty()) { 
        return -1;
    }

    Value val = stack_.back();
    stack_.pop_back();
    return val; 
}

size_t Stack::size() {
    return stack_.size();
}

void Stack::add() { 
    if (stack_.size() < 2) return;

    Value b = pop(); 
    Value a = pop(); 

    try {
        push(a.getInt() + b.getInt());
    } catch (const std::exception& e) {
        push(a);
        push(b);
    }
}