#pragma once
#include <vector>

class Stack {

public:
    Stack();

    int pop();
    
    void dup();
    void add();
    void push(int value);

    void print() const;
    bool empty() const;

    size_t size();

private:
    std::vector<int> stack_;
};