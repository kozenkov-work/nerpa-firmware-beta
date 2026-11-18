#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

class Buttons {
public:
    struct Events {
        bool up = false;
        bool down = false;
        bool select = false;
        bool back = false;

        bool any() const { return up || down || select || back; }
    };

    bool begin();
    Events read();

private:
    struct ButtonState {
        gpio_num_t pin;
        bool stableState = true;
        bool lastReading = true;
        TickType_t readingChangedAt = 0;
    };

    bool update(ButtonState& button);

    ButtonState up_{GPIO_NUM_13};
    ButtonState down_{GPIO_NUM_14};
    ButtonState select_{GPIO_NUM_27};
    ButtonState back_{GPIO_NUM_26};
};
