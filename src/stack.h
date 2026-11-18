#pragma once

#include "value.h"

#include <cstddef>
#include <vector>

class Stack {
public:
    Value pop();
    Value top() const;

    void math(int operationId);
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
    std::size_t size() const;

private:
    std::vector<Value> stack_;
};
