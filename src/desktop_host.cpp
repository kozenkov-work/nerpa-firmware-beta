#include "desktop_host.h"
#include <iostream>

void DesktopHost::debugPrint(const std::string& text) {
    std::cout << text << "\n";
}

int DesktopHost::pinMode(int pin, int mode) {
    if (mode < 0 || mode > 3) {
        std::cout << "pinMode(pin=" << pin << ", mode=" << mode
                  << ") -> -1 (unknown mode)\n";
        return -1;
    }

    std::cout << "pinMode(pin=" << pin << ", mode=" << mode << ") -> 0\n";
    return 0;
}

int DesktopHost::digitalWrite(int pin, int value) {
    std::cout << "digitalWrite(pin=" << pin << ", value=" << (value != 0)
              << ") -> 0\n";
    return 0;
}

int DesktopHost::digitalRead(int pin) {
    std::cout << "digitalRead(pin=" << pin << ") -> 0\n";
    return 0;
}