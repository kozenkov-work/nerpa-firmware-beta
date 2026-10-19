#include "vm.h"
#include <stdexcept>

void VM::run(const Program& program, Host* host) {
    running_ = true;
    
    while(running_) {
        if (ip_ >= program.code.size()) throw std::runtime_error("Instruction pointer out of bounds");
        
        const Instruction& instruction = program.code[ip_];
        
        switch (instruction.op) {
            case OpCode::CONST: {
                switch (instruction.arg1) {
                    case 0: // number
                        stack_.push(Value(instruction.arg2));
                        break;

                    case 1: // bool
                        stack_.push(Value(instruction.arg2 != 0));
                        break;

                    case 2: { // string
                        auto it = program.strings.find(instruction.arg2);

                        if (it == program.strings.end()) throw std::runtime_error("Unknown string ID");

                        stack_.push(Value(it->second));
                        break;
                    }

                    default:
                        throw std::runtime_error("Unknown CONST type");
                }
                break;
            }

            case OpCode::ADD: {
                stack_.print();
                stack_.add();
                break;
            }

            case OpCode::HOST: {
                switch (instruction.arg1) {
                    case 0: {
                        Value value = stack_.top();
                        host->debugPrint(value.getString());
                        break;
                    }

                    case 1: {
                        Value value = stack_.top();
                        host->debugPrint(std::to_string(value.getInt()));
                        break;
                    }

                    default: throw std::runtime_error("Unknown host function ID");
                }
                break;
            }

            case OpCode::JMP: {
                if (instruction.arg1 >= 0) ip_ = instruction.arg1;
                break;
            }

            case OpCode::HALT: {
                running_ = false;
                break;
            }
            
            default: throw std::runtime_error("Unknown opcode");
        }
        ++ip_; 
        /* нужно будет потом сделать так, чтобы в зависимости от интсрукции, мы могли 
        изменять этот ip_ как надо, типо для go to point и тд...
        */
    }
}