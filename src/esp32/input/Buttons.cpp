#include "Buttons.h"

#include <cstdio>
#include <initializer_list>

#include "esp_err.h"
#include "freertos/task.h"

namespace {
constexpr TickType_t DEBOUNCE_TIME = pdMS_TO_TICKS(40);
}

bool Buttons::begin() {
    bool success = true;
    for (ButtonState* button : {&up_, &down_, &select_, &back_}) {
        esp_err_t result = gpio_set_direction(button->pin, GPIO_MODE_INPUT);
        if (result == ESP_OK) {
            result = gpio_set_pull_mode(button->pin, GPIO_PULLUP_ONLY);
        }
        if (result != ESP_OK) {
            std::printf("Button GPIO %d initialization failed: %s\n",
                        static_cast<int>(button->pin), esp_err_to_name(result));
            success = false;
            continue;
        }

        const bool level = gpio_get_level(button->pin) != 0;
        button->stableState = level;
        button->lastReading = level;
        button->readingChangedAt = xTaskGetTickCount();
    }

    if (success) {
        std::printf("Buttons configured: UP=13 DOWN=14 SELECT=27 BACK=26 (pull-up)\n");
    }
    return success;
}

bool Buttons::update(ButtonState& button) {
    const bool reading = gpio_get_level(button.pin) != 0;
    const TickType_t now = xTaskGetTickCount();

    if (reading != button.lastReading) {
        button.lastReading = reading;
        button.readingChangedAt = now;
    }

    if (reading != button.stableState &&
        now - button.readingChangedAt >= DEBOUNCE_TIME) {
        button.stableState = reading;
        return !reading; // Нажатие активно при LOW.
    }
    return false;
}

Buttons::Events Buttons::read() {
    Events events;
    events.up = update(up_);
    events.down = update(down_);
    events.select = update(select_);
    events.back = update(back_);
    return events;
}
