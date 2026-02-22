#include "debugger.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <program>\n";
        return 1;
    }
    
    Debugger dbg(argv[1]);
    std::vector<std::string> args(argv + 2, argv + argc);
    dbg.run(args);
    
    return 0;
}
