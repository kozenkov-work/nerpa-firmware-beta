
#include "tokenizer.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

enum class Section {
    None,
    Strings,
    Code
};

OpCode parseOpCode(const std::string& name) {
    if (name == "CONST") return OpCode::CONST;
    if (name == "ADD")   return OpCode::ADD;
    if (name == "HALT")  return OpCode::HALT;
    if (name == "HOST") return OpCode::HOST;
    throw std::runtime_error("Unknown instruction: " + name);
}

}

Program Tokenizer::load(const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    Program program;
    Section section = Section::None;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        if (line == "*strings") {
            section = Section::Strings;
            continue;
        }

        if (line == "*code") {
            section = Section::Code;
            continue;
        }

        if (section == Section::Strings) {
            const auto colon = line.find(':');

            if (colon == std::string::npos) {
                throw std::runtime_error("Invalid string entry: " + line);
            }

            int id = std::stoi(line.substr(0, colon));
            std::string value = line.substr(colon + 1);

            if (value.size() < 2 ||
                value.front() != '"' ||
                value.back() != '"') {
                throw std::runtime_error("Invalid string value: " + line);
            }

            value = value.substr(1, value.size() - 2);

            if (!program.strings.emplace(id, value).second) {
                throw std::runtime_error("Duplicate string ID");
            }
        }
        else if (section == Section::Code) {
            const auto colon = line.find(':');

            if (colon == std::string::npos) {
                throw std::runtime_error("Invalid instruction: " + line);
            }

            int lineNumber = std::stoi(line.substr(0, colon));
            std::istringstream input(line.substr(colon + 1));

            std::string name;
            input >> name;

            if (name.empty()) {
                throw std::runtime_error("Missing instruction name");
            }

            Instruction instruction{};
            instruction.op = parseOpCode(name);

            if (name == "CONST") {
                if (!(input >> instruction.arg1 >> instruction.arg2)) {
                    throw std::runtime_error("CONST requires 2 arguments");
                }
            }
            
            if (name == "HOST") {
                if (!(input >> instruction.arg1 )) {
                    throw std::runtime_error("HOST requires 1 arguments");
                }
            }

            if (name == "ADD" || name == "HALT") {
                std::string extra;
                if (input >> extra) {
                    throw std::runtime_error(
                        name + " does not accept arguments"
                    );
                }
            }

            if (program.lineToIndex.contains(lineNumber)) {
                throw std::runtime_error("Duplicate instruction number");
            }

            program.lineToIndex.emplace(
                lineNumber, program.code.size()
            );
            program.code.push_back(instruction);
        }
        else {
            throw std::runtime_error("Data outside a known section: " + line);
        }
    }

    if (program.code.empty()) {
        throw std::runtime_error("Program contains no instructions");
    }

    return program;
}