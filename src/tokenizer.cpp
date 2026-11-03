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
        if (name == "ADD") return OpCode::ADD;
        if (name == "SUB") return OpCode::SUB;
        if (name == "MUL") return OpCode::MUL;
        if (name == "MOD") return OpCode::MOD;
        if (name == "DIV") return OpCode::DIV;
        if (name == "EQ") return OpCode::EQ;
        if (name == "LT") return OpCode::LT;
        if (name == "GT") return OpCode::GT;
        if (name == "HOST") return OpCode::HOST;
        if (name == "JMP") return OpCode::JMP;
        if (name == "DUP") return OpCode::DUP;
        if (name == "HALT") return OpCode::HALT;
        throw std::runtime_error("Unknown instruction: " + name);
    }

    Program parseProgram(std::istream& input) {
        Program program;
        Section section = Section::None;
        std::string line;

        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
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

                int id = std::stoi(line.substr(0, colon));
                std::string value = line.substr(colon + 1);

                value = value.substr(1, value.size() - 2);

                program.strings.emplace(id, value);
            }
            else if (section == Section::Code) {
                const auto colon = line.find(':');

                int lineNumber = std::stoi(line.substr(0, colon));
                std::istringstream lineInput(line.substr(colon + 1));

                std::string name;
                lineInput >> name;

                Instruction instruction{};
                instruction.op = parseOpCode(name);

                lineInput >> instruction.arg1;
                lineInput >> instruction.arg2;

                program.lineToIndex.emplace(
                    lineNumber, program.code.size()
                );
                program.code.push_back(instruction);
            }
        }
        return program;
    }
}

Program Tokenizer::load(const std::string& filename) {
    std::ifstream input(filename);
    return parseProgram(input);
}

Program Tokenizer::loadFromString(const std::string& text) {
    std::istringstream input(text);
    return parseProgram(input);
}
