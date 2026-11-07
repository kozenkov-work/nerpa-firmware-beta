#pragma once

#include <string>

enum class HostFunction {
    DebugPrintString = 0,
    DebugPrintInt = 1,
    PinMode = 2,
    DigitalWrite = 3,
    DigitalRead = 4
};

class Host {

public:

    virtual ~Host() = default;
    virtual void debugPrint(const std::string& text) = 0;
    virtual int pinMode(int pin, int mode) = 0;
    virtual int digitalWrite(int pin, int value) = 0;
    virtual int digitalRead(int pin) = 0;

};