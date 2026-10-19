#pragma once

#include "stack.h"
#include "program.h"
#include "host.h"

class VM {

public:
    void run(const Program& program, Host* host);

private:
    Stack stack_;
    
    std::size_t ip_ = 0;
    bool running_ = false;
};