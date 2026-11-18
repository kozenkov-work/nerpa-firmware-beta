#pragma once

enum class OpCode {
    CONST,
    POP,
    DUP,
    MATH,
    GET,
    SET,
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

struct Instruction {
    OpCode op;
    int arg1 = 0;
    double arg2 = 0.0;
};
