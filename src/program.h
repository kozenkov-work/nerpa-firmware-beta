
#pragma once

#include "instruction.h"
#include "value.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

struct Program {
    std::vector<Instruction> code;

    // Typed config: 0=setup line (default 0), 1=loop line (required), 2=bool (default true).
    std::unordered_map<int, Value> config;
    std::unordered_map<int, Value> variables;
    std::unordered_map<int, std::string> strings;

    std::unordered_map<int, std::size_t> lineToIndex;
};

