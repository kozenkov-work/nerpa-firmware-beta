#include "value.h"
enum class OpCode {

    CONST,
    POP,
    DUP,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    EQ,
    LT,
    GT,
    HALT,
    JMP,
    JMPF,
    CALL,
    RET,
    HOST

};
struct Instruction
{
    OpCode op;
    int arg1;
    int arg2;
};
