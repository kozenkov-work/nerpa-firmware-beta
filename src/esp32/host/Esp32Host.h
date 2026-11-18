#pragma once

#include <string>

#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "host.h"

class Esp32Host final : public Host {
public:
    using ConsolePrinter = void (*)(void* context, const std::string& text);
    using StopChecker = bool (*)(void* context);

    Esp32Host(void* context, ConsolePrinter consolePrinter, StopChecker stopChecker);
    ~Esp32Host() override;

    void debugPrint(const std::string& text) override;
    int pinMode(int pin, int mode) override;
    int digitalWrite(int pin, int value) override;
    int digitalRead(int pin) override;
    int analogWrite(int pin, double value) override;
    double analogRead(int pin) override;
    bool shouldStop() override;

private:
    void* context_;
    ConsolePrinter consolePrinter_;
    StopChecker stopChecker_;

    bool ledcTimerConfigured_ = false;
    int ledcChannelPins_[LEDC_CHANNEL_MAX];
    adc_oneshot_unit_handle_t adcHandles_[2] = {nullptr, nullptr};
};
