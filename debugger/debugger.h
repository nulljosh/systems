#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

struct Breakpoint {
    uint64_t addr;
    uint8_t original_byte;
    int id;
};

class Debugger {
public:
    Debugger(const std::string& prog);
    void run(const std::vector<std::string>& args);
    
private:
    std::string prog_path;
    pid_t pid;
    std::map<uint64_t, Breakpoint> bps;
    int bp_id_counter;
    
    void cmd_break(const std::string& addr_str);
    void cmd_continue();
    void cmd_step();
    void cmd_print(const std::string& reg_or_addr);
    void cmd_run();
    void cmd_quit();
    void cmd_help();
    
    uint64_t parse_addr(const std::string& s);
    uint64_t read_u64(uint64_t addr);
    void write_u64(uint64_t addr, uint64_t val);
    uint8_t read_byte(uint64_t addr);
    void write_byte(uint64_t addr, uint8_t val);
    
    uint64_t get_pc();
    void set_pc(uint64_t pc);
    uint64_t get_reg(const std::string& reg);
    void set_reg(const std::string& reg, uint64_t val);
    
    bool running;
};
