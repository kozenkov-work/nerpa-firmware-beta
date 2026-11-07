#pragma once

#include <fstream>
#include <optional>

class FileLoader {
public:
    FileLoader(const std::string& filename_);

    std::optional<std::string> readLine();

private:
    std::ifstream file_;
};