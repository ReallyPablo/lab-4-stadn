// main.cpp — the prompt.
//
// GIVEN, in full. This file is scaffolding: it reads a line, splits it into
// words, and calls one of your functions. It uses `while`, `if` and functions,
// which the course only explains properly in Labs 4 and 7. That is on purpose —
// in Lab 1 you read this file, you do not write it.
//
// What you add in later labs is one more `else if` branch per command.
#include <iostream>
#include <sstream>
#include <string>
#include <cstdint>
#include "cpu.hpp"
#include "dump.hpp"
#include "memory.hpp"


// Turn a word into a number. Accepts decimal (65) and hex (0x41).
// Returns false if the word is not a number at all.
static bool parse_number(const std::string& word, long& out) {
    try {
        std::size_t used = 0;
        // base 0 means: look at the prefix. "0x41" is hex, "65" is decimal.
        out = std::stol(word, &used, 0);
        return used == word.size();  // reject things like "12abc"
    } catch (...) {
        return false;
    }
}

static void print_help() {
    std::cout << "commands:\n"
              << "  dump              print all " << MEM_SIZE << " bytes\n"
              << "  get <addr>        show one byte four ways\n"
              << "  set <addr> <val>  write one byte (dec or 0x hex)\n"
              << "  inc <addr>        add 1 to the byte at addr (wraps)\n"
              << "  help              this list\n"
              << "  quit              leave\n";
}

int main() {
    Memory mem;  // 4096 bytes, on the stack, zeroed by the {} in memory.hpp

    CPU cpu;
    cpu.mem = &mem;

    std::cout << "ember 0.1 - 4096 bytes of memory you can see. Type `help`.\n";

    std::string line;
    while (true) {
        std::cout << "ember> ";

        // getline reads one whole line. It returns false at end of input
        // (Ctrl-D), which is how the loop ends if you never type `quit`.
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;
        }

        // Split the line into words: "set 0 65" -> cmd="set", args "0" and "65".
        std::istringstream words(line);
        std::string cmd;
        words >> cmd;

        if (cmd.empty()) {
            continue;  // the user just pressed Enter
        } else if (cmd == "quit" || cmd == "exit") {
            break;
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "dump") {
            dump(mem);
        } else if (cmd == "get") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: get <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                show_byte(mem_get(mem, static_cast<std::size_t>(addr)));
            }
            } else if (cmd == "set") {
            std::string a, v;
            long addr = 0, value = 0;
            if (!(words >> a) || !(words >> v) || !parse_number(a, addr) ||
                !parse_number(v, value)) {
                std::cout << "usage: set <addr> <value>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else if (value < 0 || value > 255) {
                std::cout << "a byte is 0..255, got " << value << '\n';
            } else if (!mem_set(mem, static_cast<std::size_t>(addr),
                                static_cast<Byte>(value))) {
                std::cout << "address " << addr << " is outside 0.." << MEM_SIZE - 1
                          << '\n';
            }
        } else if (cmd == "inc") {
            std::string a;
            long addr = 0;
            if (!(words >> a) || !parse_number(a, addr)) {
                std::cout << "usage: inc <addr>\n";
            } else if (addr < 0) {
                std::cout << "address must not be negative\n";
            } else {
                std::size_t address = static_cast<std::size_t>(addr);
                Byte value = mem_get(mem, address);
                mem_set(mem, address, static_cast<Byte>(value + 1));  // wrap
            }
        } else if (cmd == "reg") {
            std::string r, v;
            long value = 0;
            if (!(words >> r) || !(words >> v) || !parse_number(v, value) || value < 0 || value > 255) {
                std::cout << "usage: reg <a|b> <0..255>\n";
            } else if (r == "a") {
                cpu.a = (Byte)value;
            } else if (r == "b") {
                cpu.b = (Byte)value;
            } else {
                std::cout << "unknown register: " << r << '\n';
            }
            } else if (cmd == "regs") {
                dump_regs(cpu);
            } else if (cmd == "step") {
                step(cpu);
                        } else if (cmd == "get16" || cmd == "set16") {
            std::string a, v;
            long addr = 0, value = 0;
            bool ok = (bool)(words >> a) && parse_number(a, addr) && addr >= 0;
            if (ok && cmd == "set16") {
                ok = (bool)(words >> v) && parse_number(v, value) && value >= 0 && value <= 0xFFFF;
            }
            if (!ok) {
                std::cout << "usage: get16 <addr> | set16 <addr> <0..65535>\n";
            } else if (cmd == "get16") {
                std::cout << get16(mem, (std::size_t)addr) << '\n';
            } else if (!set16(mem, (std::size_t)addr, (std::uint16_t)value)) {
                std::cout << "address out of range\n";
            }
        } else if (cmd == "load") {
            std::string a, w;
            long addr = 0, value = 0;
            if (!(words >> a) || !parse_number(a, addr) || addr < 0) {
                std::cout << "usage: load <addr> <byte> <byte> ...\n";
            } else {
                while (words >> w) {
                    if (!parse_number(w, value) || value < 0 || value > 255 ||
                        !mem_set(mem, (std::size_t)addr, (Byte)value)) {
                        std::cout << "bad byte or address at " << addr << '\n';
                        break;
                    }
                    ++addr;
                }
            }
        } else if (cmd == "find") {
            std::string l, h, n;
            long lo = 0, hi = 0, needle = 0;
            if (!(words >> l >> h >> n) || !parse_number(l, lo) ||
                !parse_number(h, hi) || !parse_number(n, needle) ||
                lo < 0 || hi < lo || hi > (long)MEM_SIZE ||
                needle < 0 || needle > 255) {
                std::cout << "usage: find <lo> <hi> <byte>\n";
            } else {
                std::uint16_t r = find(mem, (std::uint16_t)lo, (std::uint16_t)hi, (Byte)needle);
                if (r == hi) {
                    std::cout << "not found\n";
                } else {
                    std::cout << "found at " << r << '\n';
                }
            }
        } else if (cmd == "reset") {
            cpu.pc = 0;
            cpu.h = 0;
            cpu.halted = false;
        } else if (cmd == "run") {
            run(cpu);
            } else {
            
                std::cout << "unknown command: " << cmd << " (try `help`)\n";
        }
    }

    return 0;  // 0 means "success" to the operating system
}
