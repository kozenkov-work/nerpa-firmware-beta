#include "tokenizer.h"
#include "vm.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

class TestHost final : public Host {
public:
    void debugPrint(const std::string& text) override { output.push_back(text); }
    int pinMode(int pin, int mode) override {
        pinModes.emplace_back(pin, mode);
        return 0;
    }
    int digitalWrite(int pin, int value) override {
        digitalWrites.emplace_back(pin, value);
        return 0;
    }
    int digitalRead(int) override { return digitalReadValue; }
    int analogWrite(int pin, double value) override {
        analogWrites.emplace_back(pin, value);
        return 0;
    }
    double analogRead(int) override { return analogReadValue; }
    bool shouldStop() override {
        ++stopChecks;
        return stopAfterChecks > 0 && stopChecks >= stopAfterChecks;
    }

    std::vector<std::string> output;
    std::vector<std::pair<int, int>> pinModes;
    std::vector<std::pair<int, int>> digitalWrites;
    std::vector<std::pair<int, double>> analogWrites;
    int digitalReadValue = 0;
    double analogReadValue = 0.25;
    int stopChecks = 0;
    int stopAfterChecks = 0;
};

void expect(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void expectNear(double actual, double expected, const std::string& message) {
    if (std::fabs(actual - expected) > 1e-9) throw std::runtime_error(message);
}

Program parse(const std::string& source) {
    Tokenizer tokenizer;
    return tokenizer.loadFromString(source);
}

void run(const std::string& source, TestHost& host) {
    const Program program = parse(source);
    host.stopAfterChecks = 2; // Initial instruction check, then the first loop boundary.
    VM vm;
    vm.run(program, &host);
}

void expectThrow(const std::string& source, const std::string& message) {
    try {
        TestHost host;
        run(source, host);
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void testMathVariablesAndTypes() {
    const std::string source = R"(*config
1:10
*variables
0:0
1:1
2:2
3:3
*strings
0:"Nerpa"
1:"!"
*code
0:CONST 3 0
1:RET
10:CONST 0 4
11:HOST 5
12:CONST 0 2
13:MATH 2
14:SET 0
15:CONST 0 4
16:GET 0
17:HOST 4
18:POP
19:CONST 2 0
20:CONST 2 1
21:MATH 4
22:HOST 0
23:POP
24:CONST 1 1
25:CONST 1 0
26:MATH 6
27:MATH 7
28:SET 1
29:CONST 0 2
30:GET 1
31:HOST 2
32:POP
33:CONST 3 0
34:SET 3
35:GET 3
36:POP
37:CONST 3 0
38:RET
)";
    const Program program = parse(source);
    expect(program.config.at(0).getInt() == 0, "Default setup line is wrong");
    expect(program.config.at(1).getInt() == 10, "Loop config was not parsed");
    expect(program.config.at(2).getBool(), "Config parameter 2 default is wrong");

    TestHost host;
    host.stopAfterChecks = 2;
    VM vm;
    vm.run(program, &host);
    expect(host.analogWrites.size() == 1, "Analog write was not called");
    expectNear(host.analogWrites[0].second, 0.5, "MATH multiplication/variable value is wrong");
    expect(host.output == std::vector<std::string>{"Nerpa!"}, "String concatenation failed");
    expect(host.digitalWrites.size() == 1 && host.digitalWrites[0].second == 0,
           "Boolean OR/NOT or variable SET failed");
}

void testArithmeticAndComparisons() {
    struct ArithmeticCase { int operation; double a; double b; };
    const ArithmeticCase cases[] = {{0, 0.25, 0.25}, {1, 0.75, 0.25}, {3, 0.25, 0.5}};
    for (const ArithmeticCase& item : cases) {
        TestHost host;
        const std::string source = "*config\n1:10\n*code\n"
            "0:CONST 3 0\n1:RET\n"
            "10:CONST 0 4\n"
            "11:CONST 0 " + std::to_string(item.a) + "\n"
            "12:CONST 0 " + std::to_string(item.b) + "\n"
            "13:MATH " + std::to_string(item.operation) + "\n"
            "14:HOST 4\n15:POP\n16:CONST 3 0\n17:RET\n";
        run(source, host);
        expectNear(host.analogWrites.at(0).second, 0.5, "Numeric MATH result is wrong");
    }

    TestHost host;
    run(R"(*config
1:10
*variables
0:1
*code
0:CONST 3 0
1:RET
10:CONST 0 1
11:CONST 0 2
12:LT
13:SET 0
14:CONST 0 2
15:GET 0
16:HOST 2
17:POP
18:CONST 0 3
19:CONST 0 2
20:GT
21:SET 0
22:CONST 0 2
23:GET 0
24:HOST 2
25:POP
26:CONST 1 1
27:CONST 1 1
28:EQ
29:SET 0
30:CONST 0 2
31:GET 0
32:HOST 2
33:POP
34:CONST 3 0
35:RET
)", host);
    expect(host.digitalWrites.size() == 3, "Not all comparisons executed");
    for (const auto& write : host.digitalWrites) expect(write.second == 1, "Comparison result is wrong");
}

void testControlFlowAndDup() {
    TestHost host;
    run(R"(*config
1:10
*strings
0:"unused"
1:"done"
2:"called"
*code
0:CONST 3 0
1:RET
10:CALL 30
11:POP
12:CONST 1 0
13:JMPF 16
14:CONST 2 0
15:HOST 0
16:CONST 2 1
17:DUP
18:HOST 0
19:POP
20:HOST 0
21:POP
22:CONST 3 0
23:RET
30:CONST 2 2
31:HOST 0
32:POP
33:CONST 3 0
34:RET
)", host);
    expect(host.output == std::vector<std::string>({"called", "done", "done"}),
           "CALL/RET/JMPF/DUP control flow failed");
}

void testHostIds() {
    TestHost host;
    host.digitalReadValue = 1;
    run(R"(*config
1:10
*code
0:CONST 3 0
1:RET
10:CONST 0 2
11:CONST 0 1
12:HOST 1
13:POP
14:CONST 0 2
15:CONST 1 1
16:HOST 2
17:POP
18:CONST 0 13
19:HOST 3
20:POP
21:CONST 0 4
22:CONST 0 0.5
23:HOST 4
24:POP
25:CONST 0 34
26:HOST 5
27:POP
28:CONST 3 0
29:RET
)", host);
    expect(host.pinModes == std::vector<std::pair<int, int>>({{2, 1}}), "pin_mode ID is wrong");
    expect(host.digitalWrites == std::vector<std::pair<int, int>>({{2, 1}}), "digital_write ID is wrong");
    expect(host.analogWrites.size() == 1 && host.analogWrites[0].first == 4,
           "analog_write ID is wrong");
}

void testErrors() {
    expectThrow("*code\n0:CONST 3 0\n1:RET\n", "Missing loop config was ignored");
    expectThrow("*code\n0:CONST 3 0\n1:RET\n*config\n1:0\n", "Section order was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:POP\n", "POP underflow was ignored");
    expectThrow("*config\n1:10\n*variables\n0:0\n*code\n0:CONST 3 0\n1:RET\n10:CONST 1 1\n11:SET 0\n", "SET type mismatch was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:GET 99\n", "Unknown variable was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:CONST 0 1\n11:CONST 0 0\n12:MATH 3\n", "Division by zero was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:CONST 3 1\n", "Invalid null was accepted");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:HOST 99\n", "Unknown host function was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:RET\n", "Missing loop return value was ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:CONST 3 0\n11:CONST 3 0\n12:RET\n", "Extra loop return values were ignored");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:CONST 3 0\n", "Function without RET was accepted");
    expectThrow("*config\n1:10\n*code\n0:CONST 3 0\n1:RET\n10:CALL 20\n11:RET\n20:RET\n", "Nested CALL return rule was ignored");
}

}

int main() {
    try {
        testMathVariablesAndTypes();
        testArithmeticAndComparisons();
        testControlFlowAndDup();
        testHostIds();
        testErrors();
        std::cout << "All VM tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
