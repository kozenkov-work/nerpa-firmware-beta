#include "vm.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t STOP_CHECK_INTERVAL = 32;

struct CallFrame {
    std::size_t returnIp;
    std::size_t stackBase;
    bool entryFunction;
};

int constId(double value) {
    const Value number(value);
    return number.getInt();
}
}

void VM::run(const Program& program, Host* host) {
    const auto loopConfig = program.config.find(1);
    if (loopConfig == program.config.end()) {
        throw std::runtime_error("Missing required config parameter 1 (loop line)");
    }
    const int setupLine = program.config.find(0) == program.config.end()
        ? 0 : program.config.at(0).getInt();
    const int loopLine = loopConfig->second.getInt();

    const auto lineIndex = [&](int line, const char* function) {
        const auto it = program.lineToIndex.find(line);
        if (it == program.lineToIndex.end()) {
            throw std::runtime_error(
                std::string(function) + " line not found: " + std::to_string(line)
            );
        }
        return it->second;
    };

    const std::size_t loopIp = lineIndex(loopLine, "loop");
    ip_ = lineIndex(setupLine, "setup");
    running_ = true;
    stack_.clear();
    auto variables = program.variables;
    std::vector<CallFrame> callStack;
    callStack.push_back({0, 0, true});
    bool runningSetup = true;

    std::uint32_t instructionCounter = 0;
    while (running_) {
        if (instructionCounter++ % STOP_CHECK_INTERVAL == 0 &&
            host != nullptr && host->shouldStop()) {
            running_ = false;
            break;
        }
        if (ip_ >= program.code.size()) {
            throw std::runtime_error("Function reached end of code without RET");
        }

        const Instruction& instruction = program.code[ip_];
        std::size_t nextIp = ip_ + 1;

        const auto jumpTo = [&](int line, const char* operation) {
            const auto it = program.lineToIndex.find(line);
            if (it == program.lineToIndex.end()) {
                throw std::runtime_error(
                    std::string(operation) + ": target line not found: " + std::to_string(line)
                );
            }
            nextIp = it->second;
        };

        switch (instruction.op) {
            case OpCode::CONST:
                switch (instruction.arg1) {
                    case 0:
                        stack_.push(Value(instruction.arg2));
                        break;
                    case 1:
                        stack_.push(Value(instruction.arg2 != 0.0));
                        break;
                    case 2: {
                        const int id = constId(instruction.arg2);
                        const auto it = program.strings.find(id);
                        if (it == program.strings.end()) {
                            throw std::runtime_error("CONST: unknown string ID: " + std::to_string(id));
                        }
                        stack_.push(Value(it->second));
                        break;
                    }
                    case 3:
                        if (instruction.arg2 != 0.0) {
                            throw std::runtime_error("CONST: null value must be 0");
                        }
                        stack_.push(Value());
                        break;
                    default:
                        throw std::runtime_error("CONST: unknown value type");
                }
                break;

            case OpCode::POP: stack_.pop(); break;
            case OpCode::DUP: stack_.dup(); break;
            case OpCode::MATH: stack_.math(instruction.arg1); break;
            case OpCode::ADD: stack_.add(); break;
            case OpCode::SUB: stack_.sub(); break;
            case OpCode::MUL: stack_.mul(); break;
            case OpCode::DIV: stack_.div(); break;
            case OpCode::MOD: stack_.mod(); break;
            case OpCode::EQ: stack_.eq(); break;
            case OpCode::LT: stack_.lt(); break;
            case OpCode::GT: stack_.gt(); break;

            case OpCode::GET: {
                const auto it = variables.find(instruction.arg1);
                if (it == variables.end()) {
                    throw std::runtime_error("GET: unknown variable ID: " + std::to_string(instruction.arg1));
                }
                stack_.push(it->second);
                break;
            }

            case OpCode::SET: {
                const auto it = variables.find(instruction.arg1);
                if (it == variables.end()) {
                    throw std::runtime_error("SET: unknown variable ID: " + std::to_string(instruction.arg1));
                }
                const Value value = stack_.pop();
                if (!it->second.sameType(value)) {
                    throw std::runtime_error("SET: variable type mismatch");
                }
                it->second = value;
                break;
            }

            case OpCode::HOST: {
                if (host == nullptr) throw std::runtime_error("HOST: host pointer is null");

                switch (static_cast<HostFunction>(instruction.arg1)) {
                    case HostFunction::DebugPrint:
                        host->debugPrint(stack_.pop().getString());
                        stack_.push(0);
                        break;

                    case HostFunction::DebugPrintInt: {
                        host->debugPrint(std::to_string(stack_.pop().getInt()));
                        stack_.push(0);
                        break;
                    }

                    case HostFunction::PinMode: {
                        if (stack_.size() < 2) throw std::runtime_error("pinMode: expected pin and mode");
                        const int mode = stack_.pop().getInt();
                        const int pin = stack_.pop().getInt();
                        stack_.push(host->pinMode(pin, mode));
                        break;
                    }

                    case HostFunction::DigitalWrite: {
                        if (stack_.size() < 2) throw std::runtime_error("digitalWrite: expected pin and value");
                        const bool value = stack_.pop().getBool();
                        const int pin = stack_.pop().getInt();
                        stack_.push(host->digitalWrite(pin, value ? 1 : 0));
                        break;
                    }

                    case HostFunction::DigitalRead: {
                        const int pin = stack_.pop().getInt();
                        stack_.push(host->digitalRead(pin) != 0);
                        break;
                    }

                    case HostFunction::AnalogWrite: {
                        if (stack_.size() < 2) throw std::runtime_error("analogWrite: expected pin and value");
                        const double value = stack_.pop().getNumber();
                        const int pin = stack_.pop().getInt();
                        stack_.push(host->analogWrite(pin, value));
                        break;
                    }

                    case HostFunction::AnalogRead: {
                        const int pin = stack_.pop().getInt();
                        const double value = host->analogRead(pin);
                        if (value < 0.0) throw std::runtime_error("analogRead: host operation failed");
                        stack_.push(value);
                        break;
                    }

                    default:
                        throw std::runtime_error("HOST: unknown function ID");
                }
                break;
            }

            case OpCode::JMP:
                jumpTo(instruction.arg1, "JMP");
                break;

            case OpCode::JMPF:
                if (!stack_.pop().getBool()) jumpTo(instruction.arg1, "JMPF");
                break;

            case OpCode::CALL:
                callStack.push_back({nextIp, stack_.size(), false});
                jumpTo(instruction.arg1, "CALL");
                break;

            case OpCode::RET: {
                if (callStack.empty()) throw std::runtime_error("RET: call stack is empty");
                const CallFrame frame = callStack.back();
                if (stack_.size() != frame.stackBase + 1) {
                    throw std::runtime_error("RET: function must return exactly one value");
                }
                callStack.pop_back();

                if (!frame.entryFunction) {
                    nextIp = frame.returnIp;
                    break;
                }

                stack_.pop(); // setup/loop return values are intentionally discarded.
                if (runningSetup) {
                    runningSetup = false;
                } else if (host != nullptr && host->shouldStop()) {
                    running_ = false;
                    break;
                }

                nextIp = loopIp;
                callStack.push_back({0, stack_.size(), true});
                break;
            }

            case OpCode::HALT:
                running_ = false;
                break;

            default:
                throw std::runtime_error("Unknown opcode");
        }

        ip_ = nextIp;
    }
}
