#pragma once
#include "value.h"
#include <vector>

class Stack {

public:
    Stack();

    Value pop();
    Value top();
    
    void dup();
    void add();
    void push(Value value);
    void clear();

    void print() const;
    bool empty() const;

    size_t size();

private:
    std::vector<Value> stack_;
};