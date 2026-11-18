#include "Buttons.h"

#include <cstdio>
#include <initializer_list>

#include "esp_err.h"
#include "freertos/task.h"

namespace {
constexpr TickType_t DEBOUNCE_TIME = pdMS_TO_TICKS(40);
}

void Buttons::resetIsr(void* context) {
    auto* buttons = static_cast<Buttons*>(context);
    buttons->resetInterruptPending_ = true;
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

    resetInterruptPending_ = false;
    lastResetHandledAt_ = xTaskGetTickCount() - DEBOUNCE_TIME;
    esp_err_t result = gpio_set_direction(RESET_PIN, GPIO_MODE_INPUT);
    if (result == ESP_OK) result = gpio_set_pull_mode(RESET_PIN, GPIO_PULLUP_ONLY);
    if (result == ESP_OK) result = gpio_set_intr_type(RESET_PIN, GPIO_INTR_NEGEDGE);
    if (result != ESP_OK) {
        std::printf("RESET GPIO %d initialization failed: %s\n",
                    static_cast<int>(RESET_PIN), esp_err_to_name(result));
        success = false;
    } else {
        result = gpio_install_isr_service(0);
        if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
            std::printf("GPIO ISR service initialization failed: %s\n", esp_err_to_name(result));
            success = false;
        } else {
            result = gpio_isr_handler_add(RESET_PIN, resetIsr, this);
            if (result == ESP_OK) result = gpio_intr_enable(RESET_PIN);
            if (result != ESP_OK) {
                std::printf("RESET interrupt registration failed: %s\n", esp_err_to_name(result));
                success = false;
            }
        }
    }

    if (success) {
        std::printf("Buttons configured: UP=13 DOWN=14 SELECT=27 BACK=26; RESET=25 (pull-up, interrupt)\n");
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
    events.reset = readReset();
    return events;
}

bool Buttons::readReset() {
    if (!resetInterruptPending_) return false;
    resetInterruptPending_ = false;

    const TickType_t now = xTaskGetTickCount();
    if (now - lastResetHandledAt_ < DEBOUNCE_TIME) return false;
    lastResetHandledAt_ = now;
    return true;
}
