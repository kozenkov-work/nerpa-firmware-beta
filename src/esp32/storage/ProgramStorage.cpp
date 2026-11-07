#include "ProgramStorage.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>

#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

namespace {
constexpr gpio_num_t SD_CS = GPIO_NUM_5;
constexpr gpio_num_t SD_MOSI = GPIO_NUM_23;
constexpr gpio_num_t SD_MISO = GPIO_NUM_19;
constexpr gpio_num_t SD_CLK = GPIO_NUM_18;
constexpr const char* MOUNT_POINT = "/sdcard";
}

bool ProgramStorage::mount() {
    if (mounted_) return true;

    sdmmc_host_t sdHost = SDSPI_HOST_DEFAULT();
    sdHost.max_freq_khz = 400;
    spi_bus_config_t busConfig{};
    busConfig.mosi_io_num = SD_MOSI;
    busConfig.miso_io_num = SD_MISO;
    busConfig.sclk_io_num = SD_CLK;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.max_transfer_sz = 4000;

    const auto spiHost = static_cast<spi_host_device_t>(sdHost.slot);
    esp_err_t result = spi_bus_initialize(spiHost, &busConfig, SDSPI_DEFAULT_DMA);
    if (result != ESP_OK) {
        std::printf("SPI initialization failed: %s\n", esp_err_to_name(result));
        return false;
    }

    sdspi_device_config_t slotConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
    slotConfig.gpio_cs = SD_CS;
    slotConfig.host_id = spiHost;

    esp_vfs_fat_sdmmc_mount_config_t mountConfig{};
    mountConfig.format_if_mount_failed = false;
    mountConfig.max_files = 5;
    mountConfig.allocation_unit_size = 16 * 1024;

    result = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &sdHost, &slotConfig,
                                     &mountConfig, &card_);
    if (result != ESP_OK) {
        std::printf("SD mount failed: %s\n", esp_err_to_name(result));
        spi_bus_free(spiHost);
        return false;
    }

    mounted_ = true;
    std::printf("SD card mounted at %s\n", MOUNT_POINT);
    return true;
}

bool ProgramStorage::loadFileList(std::vector<std::string>& files) {
    files.clear();
    if (!mounted_) {
        std::printf("Cannot list programs: SD card is not mounted\n");
        return false;
    }

    DIR* root = opendir(MOUNT_POINT);
    if (root == nullptr) {
        std::printf("Cannot open SD root directory: %s\n", std::strerror(errno));
        return false;
    }

    while (const dirent* entry = readdir(root)) {
        const std::string name = entry->d_name;
        const std::string path = programPath(name);
        struct stat info{};
        const int statResult = stat(path.c_str(), &info);
        if (statResult == 0 && S_ISREG(info.st_mode)) {
            files.push_back(name);
            std::printf("SD file: %s\n", name.c_str());
        } else if (statResult != 0) {
            std::printf("Cannot inspect %s: %s\n", path.c_str(), std::strerror(errno));
        }
    }
    closedir(root);
    std::sort(files.begin(), files.end());
    std::printf("SD file list: %zu file(s)\n", files.size());
    return true;
}

bool ProgramStorage::isProgramAvailable(const std::string& fileName) const {
    if (!mounted_) return false;
    struct stat info{};
    const std::string path = programPath(fileName);
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

std::string ProgramStorage::programPath(const std::string& fileName) const {
    return std::string(MOUNT_POINT) + "/" + fileName;
}
