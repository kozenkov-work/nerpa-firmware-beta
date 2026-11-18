#pragma once

#include "host.h"

class DesktopHost : public Host {
public:
    void debugPrint(const std::string& text) override;
    int pinMode(int pin, int mode) override;
    int digitalWrite(int pin, int value) override;
    int digitalRead(int pin) override;
};