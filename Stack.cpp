#include "Stack.h"
#include <iostream>

Stack::Stack() {}

void Stack::push(int value) {
    stack_.push_back(value);
}

void Stack::dup() {
    if (empty()) return;
    stack_.push_back(stack_.back());
}

void Stack::print() const {
    for(size_t i = 0; i < stack_.size(); i++) std::cout << stack_[i] << std::endl;
}

bool Stack::empty() const {
    return stack_.empty();
}

int Stack::pop() {
    if(empty()) { 
        return -1;
    }

    int val = stack_.back();
    stack_.pop_back();
    return val; 
}

size_t Stack::size() {
    return stack_.size();
}

void Stack::add() { 
    if(size() <= 1) return;
    push(pop() + pop()); 
}