#pragma once

#include "host.h"

class DesktopHost : public Host {
public:
    void debugPrint(const std::string& text) override;
};