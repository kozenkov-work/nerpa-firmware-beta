#include "App.h"

#include <cstdio>
#include <exception>
#include <stdexcept>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tokenizer.h"
#include "vm.h"

namespace {
constexpr int MAX_LINES = 3;
constexpr std::size_t MAX_CONSOLE_LINES = 15;
}

App::App() : host_(this, printFromHost) {}

bool App::setup() {
    std::printf("Nerpa: device starting\n");
    const bool buttonsReady = buttons_.begin();
    std::printf("Stage: button initialization finished (%s)\n",
                buttonsReady ? "OK" : "error");

    if (!display_.begin()) {
        std::printf("Display initialization failed\n");
        return false;
    }
    std::printf("Display configured: SSD1306 128x64, I2C 0x3C, SDA=21 SCL=22\n");

    display_.showMessage("Монтирование SD...");
    const unsigned long stackFreeWords = uxTaskGetStackHighWaterMark(nullptr);
    std::printf("Stage: first frame completed; stack high-water mark %lu words (%lu bytes)\n",
                stackFreeWords, stackFreeWords * sizeof(StackType_t));
    std::printf("Mounting SD card: CS=5 MOSI=23 MISO=19 SCK=18\n");
    if (!storage_.mount()) {
        display_.showMessage("Ошибка SD-карты!");
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    updateScreen();
    return true;
}

void App::loop() {
    const Buttons::Events events = buttons_.read();
    switch (state_) {
        case State::Start:
            handleStart(events);
            break;
        case State::Settings:
            if (events.back) {
                state_ = State::Start;
                updateScreen();
            }
            break;
        case State::FileList:
            handleFileList(events);
            break;
        case State::Console:
            handleConsole(events);
            break;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
}

void App::printFromHost(void* context, const std::string& text) {
    auto* app = static_cast<App*>(context);
    app->printToConsole(text);
    app->updateScreen();
}

bool App::loadFileList() {
    fileTopIndex_ = 0;
    return storage_.loadFileList(fileNames_);
}

void App::printToConsole(const std::string& text) {
    consoleLines_.push_back(text);
    if (consoleLines_.size() > MAX_CONSOLE_LINES) {
        consoleLines_.erase(consoleLines_.begin());
    }

    if (consoleAutoScroll_) {
        consoleScrollIndex_ = consoleLines_.size() > MAX_LINES
            ? static_cast<int>(consoleLines_.size()) - MAX_LINES : 0;
    }
}

void App::runProgram(const std::string& fileName) {
    consoleLines_.clear();
    consoleScrollIndex_ = 0;
    consoleAutoScroll_ = true;
    state_ = State::Console;

    printToConsole("Запуск: " + fileName);
    std::printf("Program selected: %s\n", fileName.c_str());
    updateScreen();

    try {
        if (!storage_.isProgramAvailable(fileName)) {
            throw std::runtime_error("Program file is not available: " + fileName);
        }

        const std::string path = storage_.programPath(fileName);
        std::printf("Reading and tokenizing %s\n", path.c_str());
        Tokenizer tokenizer;
        const Program program = tokenizer.load(path);
        std::printf("Program loaded: %zu instruction(s)\n", program.code.size());
        printToConsole("Исполнение...");
        updateScreen();

        VM vm;
        vm.run(program, &host_);
        std::printf("Program finished: %s\n", fileName.c_str());
        printToConsole("Программа завершена");
    } catch (const std::exception& error) {
        std::printf("Nerpa error: %s\n", error.what());
        printToConsole(std::string("Ошибка: ") + error.what());
    }
    updateScreen();
}

void App::updateScreen() {
    switch (state_) {
        case State::Start:
            display_.showStartMenu(startMenuIndex_);
            break;
        case State::Settings:
            display_.showSettings();
            break;
        case State::FileList:
            display_.showFileList(fileNames_, fileMenuIndex_, fileTopIndex_);
            break;
        case State::Console:
            display_.showConsole(consoleLines_, consoleScrollIndex_);
            break;
    }
}

void App::handleStart(const Buttons::Events& events) {
    if (events.up && startMenuIndex_ > 0) --startMenuIndex_;
    if (events.down && startMenuIndex_ < 1) ++startMenuIndex_;
    if (events.select) {
        if (startMenuIndex_ == 0) {
            state_ = State::Settings;
        } else {
            if (!loadFileList()) {
                display_.showMessage("Ошибка чтения SD!");
                return;
            }
            fileMenuIndex_ = 0;
            state_ = State::FileList;
        }
    }
    if (events.any()) updateScreen();
}

void App::handleFileList(const Buttons::Events& events) {
    if (events.up && fileMenuIndex_ > 0) --fileMenuIndex_;
    if (events.down && fileMenuIndex_ < static_cast<int>(fileNames_.size()) - 1) {
        ++fileMenuIndex_;
    }
    if (events.back) state_ = State::Start;
    if (events.select && !fileNames_.empty()) runProgram(fileNames_[fileMenuIndex_]);

    if (fileMenuIndex_ < fileTopIndex_) fileTopIndex_ = fileMenuIndex_;
    else if (fileMenuIndex_ >= fileTopIndex_ + MAX_LINES) {
        fileTopIndex_ = fileMenuIndex_ - MAX_LINES + 1;
    }
    if (events.any()) updateScreen();
}

void App::handleConsole(const Buttons::Events& events) {
    if (events.up && consoleScrollIndex_ > 0) {
        --consoleScrollIndex_;
        consoleAutoScroll_ = false;
    }
    if (events.down && consoleScrollIndex_ + MAX_LINES < static_cast<int>(consoleLines_.size())) {
        ++consoleScrollIndex_;
        if (consoleScrollIndex_ + MAX_LINES >= static_cast<int>(consoleLines_.size())) {
            consoleAutoScroll_ = true;
        }
    }
    if (events.back) state_ = State::FileList;
    if (events.any()) updateScreen();
}
