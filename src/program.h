
#pragma once

#include "instruction.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

struct Program {
    
    std::vector<Instruction> code;

    std::unordered_map<int, std::string> strings;

    std::unordered_map<int, std::size_t> lineToIndex;
};

