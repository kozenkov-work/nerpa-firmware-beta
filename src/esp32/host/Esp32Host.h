#pragma once

#include <string>

#include "host.h"

class Esp32Host final : public Host {
public:
    using ConsolePrinter = void (*)(void* context, const std::string& text);
    using StopChecker = bool (*)(void* context);

    Esp32Host(void* context, ConsolePrinter consolePrinter, StopChecker stopChecker)
        : context_(context), consolePrinter_(consolePrinter), stopChecker_(stopChecker) {}

    void debugPrint(const std::string& text) override;
    int pinMode(int pin, int mode) override;
    int digitalWrite(int pin, int value) override;
    int digitalRead(int pin) override;
    bool shouldStop() override;

private:
    void* context_;
    ConsolePrinter consolePrinter_;
    StopChecker stopChecker_;
};
