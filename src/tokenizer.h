#pragma once

#include "program.h"
#include <string>

class Tokenizer {
public:
    Program load(const std::string& filename);
    Program loadFromString(const std::string& text);
};