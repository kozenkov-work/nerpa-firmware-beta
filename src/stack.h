#pragma once
#include "value.h"
#include <vector>

class Stack {

public:
    Stack();

    Value pop();
    
    void dup();
    void add();
    void push(Value value);

    void print() const;
    bool empty() const;

    size_t size();

private:
    std::vector<Value> stack_;
};