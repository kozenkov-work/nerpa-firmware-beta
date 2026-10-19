#pragma once
#include "value.h"
#include <vector>

class Stack {

public:
    Stack();

    Value pop();
    Value top();
    
    void div();
    void mod();
    void eq();
    void lt();
    void gt();
    void mul();
    void dup();
    void sub();
    void add();
    void push(Value value);
    void clear();

    void print() const;
    bool empty() const;

    size_t size();

private:
    std::vector<Value> stack_;
};