#include <cstdio>
#include <exception>
#include <string>

#include "driver/gpio.h"
#include "esp_err.h"

#include "host.h"
#include "tokenizer.h"
#include "vm.h"

extern const unsigned char embedded_program_start[] asm("_binary_file_txt_start");
extern const unsigned char embedded_program_end[] asm("_binary_file_txt_end");

namespace {
class Esp32Host final : public Host {
public:
    void debugPrint(const std::string& text) override {
        std::printf("%s\n", text.c_str());
    }

    int pinMode(int pin, int mode) override {
        gpio_mode_t gpioMode;

        switch (mode) {
            case 0: gpioMode = GPIO_MODE_INPUT; break;
            case 1: gpioMode = GPIO_MODE_INPUT_OUTPUT; break;
            case 2: gpioMode = GPIO_MODE_INPUT; break;
            case 3: gpioMode = GPIO_MODE_INPUT; break;
            default:
                std::printf("pinMode(pin=%d, mode=%d) -> -1 (unknown mode)\n", pin, mode);
                return -1;
        }

        esp_err_t result = gpio_set_direction(static_cast<gpio_num_t>(pin), gpioMode);
        if (result == ESP_OK && mode == 2) {
            result = gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLUP_ONLY);
        } else if (result == ESP_OK && mode == 3) {
            result = gpio_set_pull_mode(static_cast<gpio_num_t>(pin), GPIO_PULLDOWN_ONLY);
        }

        std::printf("pinMode(pin=%d, mode=%d) -> %d (%s)\n",
                    pin, mode, result, esp_err_to_name(result));
        return result;
    }

    int digitalWrite(int pin, int value) override {
        const esp_err_t result = gpio_set_level(
            static_cast<gpio_num_t>(pin), value != 0 ? 1 : 0
        );
        std::printf("digitalWrite(pin=%d, value=%d) -> %d (%s)\n",
                    pin, value != 0 ? 1 : 0, result, esp_err_to_name(result));
        return result;
    }

    int digitalRead(int pin) override {
        const int value = gpio_get_level(static_cast<gpio_num_t>(pin));
        std::printf("digitalRead(pin=%d) -> %d\n", pin, value);
        return value;
    }
};
}

extern "C" void app_main() {
    try {
        std::string programText(
            reinterpret_cast<const char*>(embedded_program_start),
            reinterpret_cast<const char*>(embedded_program_end)
        );
        if (!programText.empty() && programText.back() == '\0') {
            programText.pop_back();
        }

        Tokenizer tokenizer;
        Esp32Host host;
        VM vm;

        std::printf("Nerpa: running embedded program\n");
        const Program program = tokenizer.loadFromString(programText);
        vm.run(program, &host);
        std::printf("Nerpa: program finished\n");
    } catch (const std::exception& error) {
        std::printf("Nerpa error: %s\n", error.what());
    }

    std::fflush(stdout);
}
