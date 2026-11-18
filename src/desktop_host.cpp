#include "desktop_host.h"

#include <cmath>
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

int DesktopHost::analogWrite(int pin, double value) {
    if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
        std::cout << "analogWrite(pin=" << pin << ", value=" << value
                  << ") -> -1 (value out of range)\n";
        return -1;
    }
    std::cout << "analogWrite(pin=" << pin << ", value=" << value << ") -> 0\n";
    return 0;
}

double DesktopHost::analogRead(int pin) {
    std::cout << "analogRead(pin=" << pin << ") -> 0\n";
    return 0.0;
}
