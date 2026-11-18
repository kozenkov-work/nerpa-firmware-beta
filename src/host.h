#pragma once

#include <string>

enum class HostFunction {
    DebugPrint = 0,
    PinMode = 1,
    DigitalWrite = 2,
    DigitalRead = 3,
    AnalogWrite = 4,
    AnalogRead = 5,
    DebugPrintInt = 6 // Backward-compatible extension; IDs 0-5 follow the beta contract.
};

class Host {

public:

    virtual ~Host() = default;
    virtual void debugPrint(const std::string& text) = 0;
    virtual int pinMode(int pin, int mode) = 0;
    virtual int digitalWrite(int pin, int value) = 0;
    virtual int digitalRead(int pin) = 0;
    virtual int analogWrite(int pin, double value) = 0;
    virtual double analogRead(int pin) = 0;
    virtual bool shouldStop() { return false; }

};