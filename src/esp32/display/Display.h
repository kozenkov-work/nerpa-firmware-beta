#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "u8g2.h"

class Display {
public:
    Display() = default;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    bool begin();
    void showMessage(const std::string& message);
    void showStartMenu(int selectedIndex);
    void showSettings();
    void showFileList(const std::vector<std::string>& files,
                      int selectedIndex, int topIndex);
    void showConsole(const std::vector<std::string>& lines, int scrollIndex);

#ifdef NERPA_DISPLAY_DIAGNOSTIC
    [[noreturn]] void runDiagnostic();
#endif

private:
    static uint8_t byteCallback(u8x8_t* display, uint8_t message,
                                uint8_t value, void* data);
    static uint8_t gpioAndDelayCallback(u8x8_t* display, uint8_t message,
                                        uint8_t value, void* data);

    void beginFrame(const char* location);
    void sendFrame();
    void drawStatusBar(const char* location);

#ifdef NERPA_DISPLAY_DIAGNOSTIC
    using TestDraw = void (*)(u8g2_t& display);
    void runTest(const char* name, TestDraw draw);
#endif

    static Display* activeInstance_;

    i2c_master_bus_handle_t bus_ = nullptr;
    i2c_master_dev_handle_t device_ = nullptr;
    u8g2_t display_{};
    std::array<uint8_t, 129> transferBuffer_{};
    std::size_t transferLength_ = 0;
    esp_err_t lastError_ = ESP_OK;
    std::size_t frameCount_ = 0;
};
