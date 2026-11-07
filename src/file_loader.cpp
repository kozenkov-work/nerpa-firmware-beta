#include "file_loader.h"

FileLoader::FileLoader(const std::string& filename) : file_(filename) {}
std::optional<std::string> FileLoader::readLine() {
    if (!file_.is_open()) return std::nullopt;
    
    std::string line;

    if (std::getline(file_, line)) return line;
    return std::nullopt;
}