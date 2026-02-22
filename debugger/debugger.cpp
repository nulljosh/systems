#include "debugger.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstring>

#ifdef __APPLE__
    #define PTRACE_PEEKDATA PT_READ_D
    #define PTRACE_POKEDATA PT_WRITE_D
#else
    #include <sys/user.h>
#endif

Debugger::Debugger(const std::string& prog) 
    : prog_path(prog), pid(0), bp_id_counter(1), running(false) {}

void Debugger::run(const std::vector<std::string>& args) {
    std::cout << "=== Minimal Debugger ===\n";
    
    std::string input;
    while (true) {
        std::cout << "(dbg) ";
        if (!std::getline(std::cin, input)) break;
        if (input.empty()) continue;
        
        std::istringstream iss(input);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "run") cmd_run();
        else if (cmd == "break" || cmd == "b") {
            std::string addr;
            iss >> addr;
            cmd_break(addr);
        }
        else if (cmd == "continue" || cmd == "c") cmd_continue();
        else if (cmd == "step" || cmd == "s") cmd_step();
        else if (cmd == "print" || cmd == "p") {
            std::string reg;
            iss >> reg;
            cmd_print(reg);
        }
        else if (cmd == "quit" || cmd == "q") cmd_quit();
        else if (cmd == "help") cmd_help();
        else std::cout << "Unknown command\n";
    }
}

void Debugger::cmd_run() {
    if (pid > 0) {
        std::cout << "Already running\n";
        return;
    }
    
    pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed\n";
        return;
    }
    
    if (pid == 0) {
        // Child
        #ifdef __APPLE__
            ptrace(PT_TRACE_ME, 0, 0, 0);
        #else
            ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
        #endif
        execl(prog_path.c_str(), prog_path.c_str(), nullptr);
        exit(1);
    }
    
    // Parent
    int status;
    waitpid(pid, &status, WUNTRACED);
    std::cout << "Program loaded (PID: " << pid << ")\n";
    running = true;
}

void Debugger::cmd_break(const std::string& addr_str) {
    if (pid == 0) {
        std::cout << "Run program first\n";
        return;
    }
    
    uint64_t addr = parse_addr(addr_str);
    if (addr == 0) return;
    
    uint8_t orig = read_byte(addr);
    write_byte(addr, 0xCC);  // INT3
    
    Breakpoint bp{addr, orig, bp_id_counter++};
    bps[addr] = bp;
    
    std::cout << "Breakpoint " << bp.id << " at 0x" << std::hex << addr << std::dec << "\n";
}

void Debugger::cmd_continue() {
    if (pid == 0) {
        std::cout << "Run program first\n";
        return;
    }
    
    #ifdef __APPLE__
        ptrace(PT_CONTINUE, pid, (caddr_t)1, 0);
    #else
        ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    #endif
    
    int status;
    waitpid(pid, &status, WUNTRACED);
    
    if (WIFEXITED(status)) {
        std::cout << "Program exited\n";
        pid = 0;
        running = false;
    } else if (WIFSTOPPED(status)) {
        int sig = WSTOPSIG(status);
        if (sig == SIGTRAP) {
            uint64_t pc = get_pc();
            auto it = bps.find(pc - 1);
            if (it != bps.end()) {
                std::cout << "Breakpoint " << it->second.id << " hit at 0x" 
                         << std::hex << pc << std::dec << "\n";
            } else {
                std::cout << "Stopped at 0x" << std::hex << pc << std::dec << "\n";
            }
        }
    }
}

void Debugger::cmd_step() {
    if (pid == 0) {
        std::cout << "Run program first\n";
        return;
    }
    
    #ifdef __APPLE__
        ptrace(PT_STEP, pid, (caddr_t)1, 0);
    #else
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    #endif
    
    int status;
    waitpid(pid, &status, WUNTRACED);
    
    uint64_t pc = get_pc();
    std::cout << "Stepped to 0x" << std::hex << pc << std::dec << "\n";
}

void Debugger::cmd_print(const std::string& reg_or_addr) {
    if (pid == 0) {
        std::cout << "Run program first\n";
        return;
    }
    
    if (reg_or_addr[0] == '$') {
        // Register
        std::string reg = reg_or_addr.substr(1);
        uint64_t val = get_reg(reg);
        std::cout << "$" << reg << " = 0x" << std::hex << val << std::dec << "\n";
    } else if (reg_or_addr[0] == '*') {
        // Memory dereference
        uint64_t addr = parse_addr(reg_or_addr.substr(1));
        uint64_t val = read_u64(addr);
        std::cout << "*0x" << std::hex << addr << " = 0x" << val << std::dec << "\n";
    } else {
        // Try as address
        uint64_t addr = parse_addr(reg_or_addr);
        uint8_t val = read_byte(addr);
        std::cout << "0x" << std::hex << addr << " = 0x" << (int)val << std::dec << "\n";
    }
}

void Debugger::cmd_quit() {
    if (pid > 0) {
        kill(pid, SIGKILL);
    }
    exit(0);
}

void Debugger::cmd_help() {
    std::cout << "\nCommands:\n"
              << "  run              - Start program\n"
              << "  break <addr>     - Set breakpoint\n"
              << "  continue (c)     - Continue execution\n"
              << "  step (s)         - Single step\n"
              << "  print $reg       - Print register\n"
              << "  print *addr      - Print memory\n"
              << "  quit             - Exit\n\n";
}

uint64_t Debugger::parse_addr(const std::string& s) {
    try {
        if (s.substr(0, 2) == "0x" || s.substr(0, 2) == "0X") {
            return std::stoull(s, nullptr, 16);
        }
        return std::stoull(s, nullptr, 10);
    } catch (...) {
        std::cerr << "Invalid address\n";
        return 0;
    }
}

uint8_t Debugger::read_byte(uint64_t addr) {
    if (pid == 0) return 0;
    
    errno = 0;
    #ifdef __APPLE__
        long word = ptrace(PT_READ_D, pid, (caddr_t)addr, 0);
    #else
        long word = ptrace(PTRACE_PEEKDATA, pid, (void*)addr, nullptr);
    #endif
    
    return (uint8_t)(word & 0xFF);
}

void Debugger::write_byte(uint64_t addr, uint8_t val) {
    if (pid == 0) return;
    
    uint64_t aligned = addr & ~(sizeof(long) - 1);
    long word = 0;
    
    #ifdef __APPLE__
        word = ptrace(PT_READ_D, pid, (caddr_t)aligned, 0);
    #else
        word = ptrace(PTRACE_PEEKDATA, pid, (void*)aligned, nullptr);
    #endif
    
    int offset = addr - aligned;
    uint8_t* bytes = (uint8_t*)&word;
    bytes[offset] = val;
    
    #ifdef __APPLE__
        ptrace(PT_WRITE_D, pid, (caddr_t)aligned, word);
    #else
        ptrace(PTRACE_POKEDATA, pid, (void*)aligned, word);
    #endif
}

uint64_t Debugger::read_u64(uint64_t addr) {
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        result |= ((uint64_t)read_byte(addr + i)) << (i * 8);
    }
    return result;
}

void Debugger::write_u64(uint64_t addr, uint64_t val) {
    for (int i = 0; i < 8; i++) {
        write_byte(addr + i, (val >> (i * 8)) & 0xFF);
    }
}

uint64_t Debugger::get_pc() {
    #ifdef __APPLE__
        // Simplified - would need Mach APIs
        // For now return 0, breakpoint detection works on address anyway
        return 0;
    #else
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        return regs.rip;
    #endif
}

void Debugger::set_pc(uint64_t pc) {
    #ifdef __APPLE__
        // Simplified
    #else
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        regs.rip = pc;
        ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    #endif
}

uint64_t Debugger::get_reg(const std::string& reg) {
    #ifdef __APPLE__
        return 0;  // Simplified for now
    #else
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        
        if (reg == "rax") return regs.rax;
        if (reg == "rbx") return regs.rbx;
        if (reg == "rcx") return regs.rcx;
        if (reg == "rdx") return regs.rdx;
        if (reg == "rsi") return regs.rsi;
        if (reg == "rdi") return regs.rdi;
        if (reg == "rsp") return regs.rsp;
        if (reg == "rbp") return regs.rbp;
        if (reg == "rip") return regs.rip;
        
        return 0;
    #endif
}

void Debugger::set_reg(const std::string& reg, uint64_t val) {
    #ifdef __APPLE__
        // Simplified
    #else
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        
        if (reg == "rax") regs.rax = val;
        else if (reg == "rbx") regs.rbx = val;
        else if (reg == "rcx") regs.rcx = val;
        else if (reg == "rdx") regs.rdx = val;
        else if (reg == "rsi") regs.rsi = val;
        else if (reg == "rdi") regs.rdi = val;
        else if (reg == "rsp") regs.rsp = val;
        else if (reg == "rbp") regs.rbp = val;
        else if (reg == "rip") regs.rip = val;
        
        ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    #endif
}
