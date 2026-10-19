#include <iostream>

#include "stack.h"
#include "program.h"
#include "tokenizer.h"
#include "desktop_host.h"
#include "vm.h"

using namespace std;
int main() {
    Tokenizer* tokenizer = new Tokenizer;
    DesktopHost* host = new DesktopHost;

    Program program = tokenizer->load("./program_files/file.txt");

    VM vm;
    vm.run(program, host);

    return 0;
}