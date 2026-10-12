#include <iostream>

#include "stack.h"
#include "program.h"
#include "tokenizer.h"
#include "desktopHost.h"
#include "vm.h"

using namespace std;
int main() {
    Tokenizer tokenizer;
    Program program = tokenizer.load("file.txt");

    DesktopHost host;
    VM vm(program, host);

    vm.run();
    return 0;
}