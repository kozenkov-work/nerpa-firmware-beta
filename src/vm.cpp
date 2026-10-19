
#include "vm.h"

#include <stdexcept>
#include <string>

void VM::run(const Program& program, Host* host) {
    ip_ = 0;
    running_ = true;
    stack_.clear();

    while (running_) {
        if (ip_ >= program.code.size()) throw std::runtime_error("Instruction pointer out of bounds");

        const Instruction& instruction = program.code[ip_];

        std::size_t nextIp = ip_ + 1;

        switch (instruction.op) {

            case OpCode::CONST: {
                switch (instruction.arg1) {

                    case 0: {
                        stack_.push(Value(instruction.arg2));
                        break;
                    }

                    case 1: {
                        stack_.push(Value(instruction.arg2 != 0));
                        break;
                    }

                    case 2: {
                        auto it = program.strings.find(instruction.arg2);

                        if (it == program.strings.end()) {
                            throw std::runtime_error(
                                "CONST: unknown string ID: " +
                                std::to_string(instruction.arg2)
                            );
                        }

                        stack_.push(Value(it->second));
                        break;
                    }

                    default: throw std::runtime_error("CONST: unknown value type");
                }

                break;
            }

            case OpCode::ADD: {
                stack_.add();
                break;
            }

            case OpCode::SUB: {
                stack_.sub();
                break;
            }

            case OpCode::MUL: {
                stack_.mul();
                break;
            }

            case OpCode::DIV: {
                stack_.div();
                break;
            }

            case OpCode::MOD: {
                stack_.mod();
                break;
            }

            case OpCode::EQ: {
                stack_.eq();
                break;
            }

            case OpCode::LT: {
                stack_.lt();
                break;
            }

            case OpCode::GT: {
                stack_.gt();
                break;
            }

            case OpCode::DUP: {
                stack_.dup();
                break;
            }

            case OpCode::HOST: {
                if (host == nullptr) throw std::runtime_error("HOST: host pointer is null");


                if (stack_.empty()) throw std::runtime_error("HOST: stack is empty");

                const Value value = stack_.top();

                switch (instruction.arg1) {

                    case 0: {
                        host->debugPrint(value.getString());
                        break;
                    }

                    case 1: {
                        host->debugPrint(
                            std::to_string(value.getInt())
                        );
                        break;
                    }

                    default: throw std::runtime_error("HOST: unknown function ID");
                }

                break;
            }

            case OpCode::JMP: {
                auto it = program.lineToIndex.find(instruction.arg1);

                if (it == program.lineToIndex.end()) {
                    throw std::runtime_error(
                        "JMP: target line not found: " +
                        std::to_string(instruction.arg1)
                    );
                }

                nextIp = it->second;
                break;
            }

            case OpCode::HALT: {
                running_ = false;
                break;
            }

            default: throw std::runtime_error("Unknown opcode");
        }

        ip_ = nextIp;
    }
}
