#pragma once

#include <string>

#include "host.h"

class Esp32Host final : public Host {
public:
    using ConsolePrinter = void (*)(void* context, const std::string& text);

    Esp32Host(void* context, ConsolePrinter consolePrinter)
        : context_(context), consolePrinter_(consolePrinter) {}

    void debugPrint(const std::string& text) override;
    int pinMode(int pin, int mode) override;
    int digitalWrite(int pin, int value) override;
    int digitalRead(int pin) override;

private:
    void* context_;
    ConsolePrinter consolePrinter_;
};
