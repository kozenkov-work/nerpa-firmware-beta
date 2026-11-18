#pragma once

#include <string>
#include <vector>

#include "driver/sdmmc_types.h"

class ProgramStorage {
public:
    bool mount();
    bool loadFileList(std::vector<std::string>& files);
    bool isProgramAvailable(const std::string& fileName) const;
    std::string programPath(const std::string& fileName) const;
    bool isMounted() const { return mounted_; }

private:
    sdmmc_card_t* card_ = nullptr;
    bool mounted_ = false;
};
