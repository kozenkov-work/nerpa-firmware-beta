#include "vm.h"
#include <stdexcept>

VM::VM(const Program& program, Host& host)
    : program_(program),
      host_(host) {
}

void VM::run() {
    running_ = true;
    if (ip_ >= program_.code.size()) {
        throw std::runtime_error("Instruction pointer out of bounds");
    }
    while(running_) {
        const Instruction instruction = program_.code[ip_];  
        switch (instruction.op)
        {
            case OpCode::CONST: {
                switch (instruction.arg1) {
                    case 0: // number
                        stack_.push(Value(instruction.arg2));
                        break;

                    case 1: // bool
                        stack_.push(Value(instruction.arg2 != 0));
                        break;

                    case 2: { // string
                        auto it = program_.strings.find(instruction.arg2);

                        if (it == program_.strings.end()) {
                            throw std::runtime_error("Unknown string ID");
                        }

                        stack_.push(Value(it->second));
                        break;
                    }

                    default:
                        throw std::runtime_error("Unknown CONST type");
                }

                ++ip_;
                break;
            }
            case OpCode::ADD: {
                stack_.print();
                stack_.add();
                ++ip_;
                break;
            }
            case OpCode::HOST: {
                switch (instruction.arg1) {
                    case 0: {
                        Value value = stack_.pop();
                        host_.debugPrint(value.getString());
                        break;
                    }
                    case 1: {
                        Value value = stack_.pop();
                        host_.debugPrint(std::to_string(value.getInt()));
                        break;
                    }
                }

                ++ip_;
                break;
            }
            case OpCode::HALT: {
                running_ = false;
                break;
            }
        }
    }
}