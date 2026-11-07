#pragma once

#include <string>
#include <vector>

#include "Buttons.h"
#include "Display.h"
#include "Esp32Host.h"
#include "ProgramStorage.h"

class App {
public:
    App();

    bool setup();
    void loop();

private:
    enum class State { Start, Settings, FileList, Console };

    static void printFromHost(void* context, const std::string& text);

    bool loadFileList();
    void printToConsole(const std::string& text);
    void runProgram(const std::string& fileName);
    void updateScreen();

    void handleStart(const Buttons::Events& events);
    void handleFileList(const Buttons::Events& events);
    void handleConsole(const Buttons::Events& events);

    Display display_;
    Buttons buttons_;
    ProgramStorage storage_;
    Esp32Host host_;

    State state_ = State::Start;
    int startMenuIndex_ = 0;
    std::vector<std::string> fileNames_;
    int fileMenuIndex_ = 0;
    int fileTopIndex_ = 0;

    std::vector<std::string> consoleLines_;
    int consoleScrollIndex_ = 0;
    bool consoleAutoScroll_ = true;
};
