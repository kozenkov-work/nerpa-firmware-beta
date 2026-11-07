#include "Display.h"

#include <cstdio>
#include <cstring>

#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {
constexpr gpio_num_t DISPLAY_SDA = GPIO_NUM_21;
constexpr gpio_num_t DISPLAY_SCL = GPIO_NUM_22;
constexpr uint8_t DISPLAY_ADDRESS = 0x3C;
constexpr int MAX_LINES = 3;
constexpr int LINE_HEIGHT = 15;
constexpr int START_Y = 30;

#ifdef NERPA_DISPLAY_DIAGNOSTIC
void drawBlack(u8g2_t& /* display */) {}
void drawWhite(u8g2_t& display) { u8g2_DrawBox(&display, 0, 0, 128, 64); }
void drawFrame(u8g2_t& display) { u8g2_DrawFrame(&display, 0, 0, 128, 64); }

void drawLines(u8g2_t& display) {
    for (int x = 0; x < 128; x += 32) u8g2_DrawVLine(&display, x, 0, 64);
    u8g2_DrawVLine(&display, 127, 0, 64);
    for (int y = 0; y < 64; y += 16) u8g2_DrawHLine(&display, 0, y, 128);
    u8g2_DrawHLine(&display, 0, 63, 128);
}

void drawCheckerboard(u8g2_t& display) {
    for (int y = 0; y < 64; y += 16) {
        for (int x = 0; x < 128; x += 16) {
            if (((x / 16) + (y / 16)) % 2 == 0) {
                u8g2_DrawBox(&display, x, y, 16, 16);
            }
        }
    }
}

void drawPixels(u8g2_t& display) {
    u8g2_DrawPixel(&display, 0, 0);
    u8g2_DrawPixel(&display, 127, 0);
    u8g2_DrawPixel(&display, 0, 63);
    u8g2_DrawPixel(&display, 127, 63);
    u8g2_DrawPixel(&display, 64, 32);
}

void drawEnglishText(u8g2_t& display) {
    u8g2_DrawStr(&display, 0, 16, "Nerpa OLED");
    u8g2_DrawStr(&display, 0, 32, "SSD1306 test");
    u8g2_DrawStr(&display, 0, 48, "128 x 64");
}

void drawRussianText(u8g2_t& display) {
    u8g2_DrawUTF8(&display, 0, 16, "Привет, Nerpa");
    u8g2_DrawUTF8(&display, 0, 32, "Дисплей");
    u8g2_DrawUTF8(&display, 0, 48, "Русский текст");
}
#endif
} // namespace

Display* Display::activeInstance_ = nullptr;

uint8_t Display::byteCallback(u8x8_t* display, uint8_t message,
                              uint8_t value, void* data) {
    Display* self = activeInstance_;
    if (self == nullptr || display != u8g2_GetU8x8(&self->display_)) return 0;

    switch (message) {
        case U8X8_MSG_BYTE_INIT:
        case U8X8_MSG_BYTE_START_TRANSFER:
            self->transferLength_ = 0;
            return 1;
        case U8X8_MSG_BYTE_SEND:
            if (self->device_ == nullptr) {
                self->lastError_ = ESP_ERR_INVALID_STATE;
                return 0;
            }
            if ((value > 0 && data == nullptr) ||
                self->transferLength_ + value > self->transferBuffer_.size()) {
                self->lastError_ = ESP_ERR_INVALID_SIZE;
                return 0;
            }
            if (value > 0) {
                std::memcpy(self->transferBuffer_.data() + self->transferLength_,
                            data, value);
                self->transferLength_ += value;
            }
            return 1;
        case U8X8_MSG_BYTE_END_TRANSFER: {
            if (self->transferLength_ == 0) return 1;
            const esp_err_t result = i2c_master_transmit(
                self->device_, self->transferBuffer_.data(), self->transferLength_, 1000
            );
            self->transferLength_ = 0;
            if (result != ESP_OK) self->lastError_ = result;
            return result == ESP_OK;
        }
        case U8X8_MSG_BYTE_SET_DC:
            // SSD13xx I2C передаёт control byte в потоке, отдельный GPIO DC не нужен.
            return 1;
        default:
            return 0;
    }
}

uint8_t Display::gpioAndDelayCallback(u8x8_t* /* display */, uint8_t message,
                                      uint8_t value, void* /* data */) {
    switch (message) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            return 1;
        case U8X8_MSG_DELAY_MILLI:
            vTaskDelay(pdMS_TO_TICKS(value));
            return 1;
        case U8X8_MSG_DELAY_10MICRO:
            esp_rom_delay_us(value * 10);
            return 1;
        case U8X8_MSG_DELAY_I2C:
            esp_rom_delay_us(value * 5);
            return 1;
        default:
            return 1;
    }
}

bool Display::begin() {
    i2c_master_bus_config_t busConfig{};
    busConfig.clk_source = I2C_CLK_SRC_DEFAULT;
    busConfig.i2c_port = I2C_NUM_0;
    busConfig.scl_io_num = DISPLAY_SCL;
    busConfig.sda_io_num = DISPLAY_SDA;
    busConfig.glitch_ignore_cnt = 7;
    busConfig.flags.enable_internal_pullup = true;

    lastError_ = i2c_new_master_bus(&busConfig, &bus_);
    if (lastError_ != ESP_OK) {
        std::printf("I2C bus initialization failed: %s\n", esp_err_to_name(lastError_));
        return false;
    }

    i2c_device_config_t deviceConfig{};
    deviceConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    deviceConfig.device_address = DISPLAY_ADDRESS;
    deviceConfig.scl_speed_hz = 400000;
    lastError_ = i2c_master_bus_add_device(bus_, &deviceConfig, &device_);
    if (lastError_ != ESP_OK) {
        std::printf("I2C display device setup failed: %s\n", esp_err_to_name(lastError_));
        return false;
    }

    lastError_ = i2c_master_probe(bus_, DISPLAY_ADDRESS, 1000);
    if (lastError_ != ESP_OK) {
        std::printf("Display did not acknowledge at I2C 0x%02X: %s\n",
                    DISPLAY_ADDRESS, esp_err_to_name(lastError_));
        return false;
    }
    std::printf("I2C probe: address 0x%02X acknowledged\n", DISPLAY_ADDRESS);

    lastError_ = ESP_OK;
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &display_, U8G2_R0, byteCallback, gpioAndDelayCallback
    );
    activeInstance_ = this;
    std::printf("Stage: U8G2 setup completed\n");
    u8g2_InitDisplay(&display_);
    std::printf("Stage: U8G2 display initialization finished\n");
    u8g2_SetPowerSave(&display_, 0);
    u8g2_SetFont(&display_, u8g2_font_6x13_t_cyrillic);
    return lastError_ == ESP_OK;
}

void Display::drawStatusBar(const char* location) {
    u8g2_DrawUTF8(&display_, 0, 11, location);
    constexpr const char* battery = "[||||]~";
    u8g2_DrawUTF8(&display_, 128 - u8g2_GetUTF8Width(&display_, battery), 11, battery);
    u8g2_DrawLine(&display_, 0, 14, 127, 14);
}

void Display::beginFrame(const char* location) {
    u8g2_ClearBuffer(&display_);
    drawStatusBar(location);
}

void Display::sendFrame() {
    lastError_ = ESP_OK;
    if (frameCount_ == 0) std::printf("Stage: first frame transfer started\n");
    u8g2_SendBuffer(&display_);
    ++frameCount_;
    if (frameCount_ == 1) std::printf("Stage: first frame transfer finished\n");
    if (lastError_ != ESP_OK) {
        std::printf("Display update failed: %s\n", esp_err_to_name(lastError_));
    }
}

void Display::showMessage(const std::string& message) {
    u8g2_ClearBuffer(&display_);
    u8g2_DrawUTF8(&display_, 0, 30, message.c_str());
    sendFrame();
}

void Display::showStartMenu(int selectedIndex) {
    static constexpr const char* items[] = {"Настройки", "Выбор программы"};
    beginFrame("Старт");
    for (int i = 0; i < 2; ++i) {
        const std::string text = (i == selectedIndex ? "> " : "  ") + std::string(items[i]);
        u8g2_DrawUTF8(&display_, 0, START_Y + i * LINE_HEIGHT, text.c_str());
    }
    sendFrame();
}

void Display::showSettings() {
    beginFrame("Настройки");
    u8g2_DrawUTF8(&display_, 0, START_Y, "Тут пока пусто.");
    sendFrame();
}

void Display::showFileList(const std::vector<std::string>& files,
                           int selectedIndex, int topIndex) {
    beginFrame("ПО");
    if (files.empty()) {
        u8g2_DrawUTF8(&display_, 0, START_Y, "Файлы не найдены");
        sendFrame();
        return;
    }

    for (int i = 0; i < MAX_LINES; ++i) {
        const int index = topIndex + i;
        if (index >= static_cast<int>(files.size())) break;
        const std::string text = (index == selectedIndex ? "> " : "  ") + files[index];
        u8g2_DrawUTF8(&display_, 0, START_Y + i * LINE_HEIGHT, text.c_str());
    }
    sendFrame();
}

void Display::showConsole(const std::vector<std::string>& lines, int scrollIndex) {
    beginFrame("Консоль");
    for (int i = 0; i < MAX_LINES; ++i) {
        const int index = scrollIndex + i;
        if (index >= static_cast<int>(lines.size())) break;
        u8g2_DrawUTF8(&display_, 0, START_Y + i * LINE_HEIGHT, lines[index].c_str());
    }
    sendFrame();
}

#ifdef NERPA_DISPLAY_DIAGNOSTIC
void Display::runTest(const char* name, TestDraw draw) {
    std::printf("Display test START: %s\n", name);
    lastError_ = ESP_OK;
    u8g2_ClearBuffer(&display_);
    draw(display_);
    u8g2_SendBuffer(&display_);
    if (lastError_ == ESP_OK) {
        std::printf("Display test END: %s (I2C OK)\n", name);
    } else {
        std::printf("Display test END: %s (I2C error: %s)\n",
                    name, esp_err_to_name(lastError_));
    }
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void Display::runDiagnostic() {
    std::printf("Display diagnostic: starting I2C, SDA=21 SCL=22, 400 kHz\n");
    std::printf("Display diagnostic: driver SSD1306 128x64, address 0x3C\n");
    if (!begin()) {
        std::printf("U8G2 initialization failed: %s\n", esp_err_to_name(lastError_));
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    std::printf("U8G2 initialization finished: I2C transport OK\n");

    while (true) {
        runTest("black", drawBlack);
        runTest("white", drawWhite);
        runTest("frame", drawFrame);
        runTest("lines", drawLines);
        runTest("checkerboard", drawCheckerboard);
        runTest("pixels", drawPixels);
        runTest("English text", drawEnglishText);
        runTest("Russian text", drawRussianText);
    }
}
#endif
