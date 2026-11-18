#include "tokenizer.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

enum class Section {
    None,
    Config,
    Variables,
    Strings,
    Code
};

OpCode parseOpCode(const std::string& name) {
    if (name == "CONST") return OpCode::CONST;
    if (name == "POP") return OpCode::POP;
    if (name == "DUP") return OpCode::DUP;
    if (name == "MATH") return OpCode::MATH;
    if (name == "GET") return OpCode::GET;
    if (name == "SET") return OpCode::SET;
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
    if (name == "JMPF") return OpCode::JMPF;
    if (name == "CALL") return OpCode::CALL;
    if (name == "RET") return OpCode::RET;
    if (name == "HALT") return OpCode::HALT;
    throw std::runtime_error("Unknown instruction: " + name);
}

int parseInt(const std::string& text, const std::string& description) {
    std::size_t parsed = 0;
    const int value = std::stoi(text, &parsed);
    if (parsed != text.size()) throw std::runtime_error("Invalid " + description + ": " + text);
    return value;
}

Value parseConfigValue(int id, const std::string& text) {
    if (id == 0 || id == 1) {
        std::size_t parsed = 0;
        const double value = std::stod(text, &parsed);
        if (parsed != text.size()) throw std::runtime_error("Invalid config number: " + text);
        const Value number(value);
        number.getInt();
        return number;
    }
    if (id == 2) {
        const int value = parseInt(text, "config bool");
        if (value != 0 && value != 1) throw std::runtime_error("Config bool must be 0 or 1");
        return Value(value == 1);
    }
    throw std::runtime_error("Unknown config ID: " + std::to_string(id));
}

std::size_t colonPosition(const std::string& line) {
    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) throw std::runtime_error("Expected ':' in line: " + line);
    return colon;
}

Value defaultVariable(int type) {
    switch (type) {
        case 0: return Value(0.0);
        case 1: return Value(false);
        case 2: return Value(std::string{});
        case 3: return Value();
        default: throw std::runtime_error("Unknown variable type: " + std::to_string(type));
    }
}

bool hasArgument(OpCode op) {
    return op == OpCode::CONST || op == OpCode::MATH || op == OpCode::GET ||
           op == OpCode::SET || op == OpCode::HOST || op == OpCode::JMP ||
           op == OpCode::JMPF || op == OpCode::CALL;
}

Program parseProgram(std::istream& input) {
    Program program;
    Section section = Section::None;
    std::string line;
    std::size_t sourceLine = 0;

    while (std::getline(input, line)) {
        ++sourceLine;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        try {
            const auto enterSection = [&](Section next) {
                if (static_cast<int>(next) <= static_cast<int>(section)) {
                    throw std::runtime_error("Sections are repeated or out of contract order");
                }
                section = next;
            };
            if (line == "*config") { enterSection(Section::Config); continue; }
            if (line == "*variables") { enterSection(Section::Variables); continue; }
            if (line == "*strings") { enterSection(Section::Strings); continue; }
            if (line == "*code") { enterSection(Section::Code); continue; }
            if (line.front() == '*') throw std::runtime_error("Unknown section: " + line);
            if (section == Section::None) throw std::runtime_error("Data outside a section");

            const std::size_t colon = colonPosition(line);
            const int id = parseInt(line.substr(0, colon), "ID");

            if (section == Section::Config) {
                if (!program.config.emplace(id, parseConfigValue(id, line.substr(colon + 1))).second) {
                    throw std::runtime_error("Duplicate config ID: " + std::to_string(id));
                }
            } else if (section == Section::Variables) {
                const int type = parseInt(line.substr(colon + 1), "variable type");
                if (!program.variables.emplace(id, defaultVariable(type)).second) {
                    throw std::runtime_error("Duplicate variable ID: " + std::to_string(id));
                }
            } else if (section == Section::Strings) {
                const std::string value = line.substr(colon + 1);
                if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
                    throw std::runtime_error("String must be enclosed in double quotes");
                }
                if (!program.strings.emplace(id, value.substr(1, value.size() - 2)).second) {
                    throw std::runtime_error("Duplicate string ID: " + std::to_string(id));
                }
            } else {
                std::istringstream lineInput(line.substr(colon + 1));
                std::string name;
                lineInput >> name;

                Instruction instruction{};
                instruction.op = parseOpCode(name);
                if (instruction.op == OpCode::CONST) {
                    if (!(lineInput >> instruction.arg1 >> instruction.arg2)) {
                        throw std::runtime_error("CONST expects type and value");
                    }
                } else if (hasArgument(instruction.op)) {
                    if (!(lineInput >> instruction.arg1)) {
                        throw std::runtime_error(name + " expects one argument");
                    }
                }

                std::string extra;
                if (lineInput >> extra) throw std::runtime_error("Unexpected argument: " + extra);
                if (!program.lineToIndex.emplace(id, program.code.size()).second) {
                    throw std::runtime_error("Duplicate code line: " + std::to_string(id));
                }
                program.code.push_back(instruction);
            }
        } catch (const std::exception& error) {
            throw std::runtime_error(
                "Line " + std::to_string(sourceLine) + ": " + error.what()
            );
        }
    }
    program.config.emplace(0, Value(0.0));
    program.config.emplace(2, Value(true));
    if (program.config.find(1) == program.config.end()) {
        throw std::runtime_error("Missing required config parameter 1 (loop line)");
    }
    for (int id : {0, 1}) {
        const int lineNumber = program.config.at(id).getInt();
        if (program.lineToIndex.find(lineNumber) == program.lineToIndex.end()) {
            throw std::runtime_error(
                std::string(id == 0 ? "setup" : "loop") + " line not found: " +
                std::to_string(lineNumber)
            );
        }
    }
    return program;
}

}

Program Tokenizer::load(const std::string& filename) {
    std::ifstream input(filename);
    if (!input.is_open()) throw std::runtime_error("Cannot open program: " + filename);
    return parseProgram(input);
}

Program Tokenizer::loadFromString(const std::string& text) {
    std::istringstream input(text);
    return parseProgram(input);
}
