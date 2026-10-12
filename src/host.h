#pragma once

#include <string>

class Host {

public:

    virtual ~Host() = default;
    virtual void debugPrint(const std::string& text) = 0;

};