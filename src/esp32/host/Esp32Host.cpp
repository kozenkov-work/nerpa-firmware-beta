#include "Esp32Host.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "driver/gpio.h"
#include "esp_err.h"

namespace {
constexpr ledc_mode_t PWM_MODE = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_t PWM_TIMER = LEDC_TIMER_0;
constexpr uint32_t PWM_FREQUENCY_HZ = 5000;
constexpr uint32_t PWM_MAX_DUTY = 255;
}

Esp32Host::Esp32Host(void* context, ConsolePrinter consolePrinter, StopChecker stopChecker)
    : context_(context), consolePrinter_(consolePrinter), stopChecker_(stopChecker) {
    std::fill(std::begin(ledcChannelPins_), std::end(ledcChannelPins_), -1);
}

Esp32Host::~Esp32Host() {
    for (adc_oneshot_unit_handle_t& handle : adcHandles_) {
        if (handle == nullptr) continue;
        const esp_err_t result = adc_oneshot_del_unit(handle);
        if (result != ESP_OK) {
            std::printf("ADC deinitialization failed: %s\n", esp_err_to_name(result));
        }
    }
}

void Esp32Host::debugPrint(const std::string& text) {
    std::printf("%s\n", text.c_str());
    consolePrinter_(context_, text);
}

bool Esp32Host::shouldStop() {
    return stopChecker_(context_);
}

int Esp32Host::pinMode(int pin, int mode) {
    gpio_mode_t gpioMode;
    switch (mode) {
        case 0: gpioMode = GPIO_MODE_INPUT; break;
        case 1: gpioMode = GPIO_MODE_OUTPUT; break;
        case 2: gpioMode = GPIO_MODE_INPUT; break;
        case 3: gpioMode = GPIO_MODE_INPUT; break;
        default:
            std::printf("pinMode(pin=%d): unknown mode %d\n", pin, mode);
            return -1;
    }

    esp_err_t result = gpio_set_direction(static_cast<gpio_num_t>(pin), gpioMode);
    if (result == ESP_OK && mode == 2) {
        result = gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLUP_ONLY);
    } else if (result == ESP_OK && mode == 3) {
        result = gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLDOWN_ONLY);
    }

    if (result != ESP_OK) {
        std::printf("pinMode(pin=%d, mode=%d) failed: %s\n",
                    pin, mode, esp_err_to_name(result));
    }
    return result;
}

int Esp32Host::digitalWrite(int pin, int value) {
    const esp_err_t result = gpio_set_level(
        static_cast<gpio_num_t>(pin), value != 0 ? 1 : 0
    );
    if (result != ESP_OK) {
        std::printf("digitalWrite(pin=%d) failed: %s\n", pin, esp_err_to_name(result));
    }
    return result;
}

int Esp32Host::digitalRead(int pin) {
    return gpio_get_level(static_cast<gpio_num_t>(pin));
}

int Esp32Host::analogWrite(int pin, double value) {
    if (pin < 0 || !std::isfinite(value) || value < 0.0 || value > 1.0) {
        std::printf("analogWrite(pin=%d, value=%f): invalid argument\n", pin, value);
        return ESP_ERR_INVALID_ARG;
    }

    if (!ledcTimerConfigured_) {
        ledc_timer_config_t timerConfig{};
        timerConfig.speed_mode = PWM_MODE;
        timerConfig.duty_resolution = LEDC_TIMER_8_BIT;
        timerConfig.timer_num = PWM_TIMER;
        timerConfig.freq_hz = PWM_FREQUENCY_HZ;
        timerConfig.clk_cfg = LEDC_AUTO_CLK;

        const esp_err_t result = ledc_timer_config(&timerConfig);
        if (result != ESP_OK) {
            std::printf("LEDC timer initialization failed: %s\n", esp_err_to_name(result));
            return result;
        }
        ledcTimerConfigured_ = true;
    }

    const int channelIndex = pin % LEDC_CHANNEL_MAX;
    if (ledcChannelPins_[channelIndex] >= 0 && ledcChannelPins_[channelIndex] != pin) {
        std::printf("analogWrite(pin=%d): PWM channel %d is already used by GPIO%d\n",
                    pin, channelIndex, ledcChannelPins_[channelIndex]);
        return ESP_ERR_INVALID_STATE;
    }

    const auto channel = static_cast<ledc_channel_t>(channelIndex);
    if (ledcChannelPins_[channelIndex] < 0) {
        ledc_channel_config_t channelConfig{};
        channelConfig.gpio_num = pin;
        channelConfig.speed_mode = PWM_MODE;
        channelConfig.channel = channel;
        channelConfig.timer_sel = PWM_TIMER;
        channelConfig.duty = 0;
        channelConfig.hpoint = 0;

        const esp_err_t result = ledc_channel_config(&channelConfig);
        if (result != ESP_OK) {
            std::printf("analogWrite(pin=%d): channel initialization failed: %s\n",
                        pin, esp_err_to_name(result));
            return result;
        }
        ledcChannelPins_[channelIndex] = pin;
    }

    const uint32_t duty = static_cast<uint32_t>(std::lround(value * PWM_MAX_DUTY));
    const esp_err_t result = ledc_set_duty_and_update(PWM_MODE, channel, duty, 0);
    if (result != ESP_OK) {
        std::printf("analogWrite(pin=%d) failed: %s\n", pin, esp_err_to_name(result));
    }
    return result;
}

double Esp32Host::analogRead(int pin) {
    adc_unit_t unit;
    adc_channel_t channel;
    esp_err_t result = adc_oneshot_io_to_channel(pin, &unit, &channel);
    if (result != ESP_OK || (unit != ADC_UNIT_1 && unit != ADC_UNIT_2)) {
        std::printf("analogRead(pin=%d): GPIO is not an ADC input\n", pin);
        return -1.0;
    }

    const int unitIndex = unit == ADC_UNIT_1 ? 0 : 1;
    if (adcHandles_[unitIndex] == nullptr) {
        adc_oneshot_unit_init_cfg_t unitConfig{};
        unitConfig.unit_id = unit;
        unitConfig.ulp_mode = ADC_ULP_MODE_DISABLE;
        result = adc_oneshot_new_unit(&unitConfig, &adcHandles_[unitIndex]);
        if (result != ESP_OK) {
            std::printf("analogRead(pin=%d): ADC initialization failed: %s\n",
                        pin, esp_err_to_name(result));
            return -1.0;
        }
    }

    adc_oneshot_chan_cfg_t channelConfig{};
    channelConfig.atten = ADC_ATTEN_DB_12;
    channelConfig.bitwidth = ADC_BITWIDTH_12;
    result = adc_oneshot_config_channel(adcHandles_[unitIndex], channel, &channelConfig);
    if (result != ESP_OK) {
        std::printf("analogRead(pin=%d): channel configuration failed: %s\n",
                    pin, esp_err_to_name(result));
        return -1.0;
    }

    int raw = 0;
    result = adc_oneshot_read(adcHandles_[unitIndex], channel, &raw);
    if (result != ESP_OK) {
        std::printf("analogRead(pin=%d) failed: %s\n", pin, esp_err_to_name(result));
        return -1.0;
    }
    return static_cast<double>(raw) / 4095.0;
}
