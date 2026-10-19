
#include "tokenizer.h"
#include "file_loader.h"

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
        if (name == "CONST") return OpCode::CONST; //
        if (name == "ADD") return OpCode::ADD; //
        if (name == "SUB") return OpCode::SUB; // 
        if (name == "MUL") return OpCode::MUL; // 
        if (name == "MOD") return OpCode::MOD; // 
        if (name == "DIV") return OpCode::DIV; // 
        if (name == "EQ") return OpCode::EQ; //
        if (name == "LT") return OpCode::LT; //
        if (name == "GT") return OpCode::GT;
        if (name == "HOST") return OpCode::HOST;
        if (name == "JMP") return OpCode::JMP; // 
        if (name == "DUP") return OpCode::DUP; // 
        if (name == "HALT")  return OpCode::HALT; // 
        // слеши, это типо метка, что это есть в вмке
        throw std::runtime_error("Unknown instruction: " + name);
    }   
}
Program Tokenizer::load(const std::string& filename) {
    
    FileLoader loader(filename);
    Program program;
    Section section = Section::None;
    std::string line;

    while (auto optLine = loader.readLine()) {
        std::string line = *optLine;
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
            std::istringstream input(line.substr(colon + 1));

            std::string name;
            input >> name;

            Instruction instruction{};
            instruction.op = parseOpCode(name);
            
            input >> instruction.arg1;
            input >> instruction.arg2;

            program.lineToIndex.emplace(
                lineNumber, program.code.size()
            );
            program.code.push_back(instruction);
        }
    }
    return program;
}