#include "Esp32Host.h"

#include <cstdio>

#include "driver/gpio.h"
#include "esp_err.h"

void Esp32Host::debugPrint(const std::string& text) {
    std::printf("%s\n", text.c_str());
    consolePrinter_(context_, text);
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
