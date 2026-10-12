#pragma once

#include "stack.h"
#include "program.h"
#include "host.h"

class VM {

public:
    VM(const Program& program_, Host& host_);

    void run();

private:
    Stack stack_;
    const Program& program_;
    Host& host_;
    
    std::size_t ip_ = 0;
    bool running_ = false;
};