#include "cpu.hpp"
#include "alu.hpp"
#include <iomanip>
#include <iostream>

DecodedInstruction decode(Byte op) {
    DecodedInstruction inst;
    inst.group = (op >> 4) & 0x0F;
    inst.index = op & 0x0F;
    return inst;
}

void dump_regs(const CPU& cpu) {
    std::cout << "PC:" << cpu.pc
              << " H:" << cpu.h
              << " A:" << (int)cpu.a
              << " B:" << (int)cpu.b
              << " Z:" << cpu.f.z
              << " N:" << cpu.f.n
              << " C:" << cpu.f.c << '\n';
}

static std::size_t op_size(Byte op) {
    switch (op) {
    case OP_LOADI_A:
    case OP_LOADI_B:
        return 2;
    case OP_LOAD_A:
    case OP_LOAD_B:
    case OP_STORE_A:
    case OP_STORE_B:
    case OP_LOADH:
    case OP_JMP:
    case OP_JZ:
    case OP_JNZ:
        return 3;
    default:
        return 1;
    }
}

static void fault(CPU& cpu, const char* msg) {
    std::cout << "error: " << msg << " (PC=" << cpu.pc << ")\n";
    cpu.halted = true;
}

static void unknown(CPU& cpu, Byte op) {
    std::cout << "error: unknown opcode 0x" << std::hex << std::setw(2)
              << std::setfill('0') << (int)op << std::dec << std::setfill(' ')
              << " (PC=" << cpu.pc << ")\n";
    cpu.halted = true;
}

void step(CPU& cpu) {
    if (cpu.halted) {
        std::cout << "halted\n";
        return;
    }

    Byte op = mem_get(*cpu.mem, cpu.pc);
    std::size_t size = op_size(op);

    if ((std::size_t)cpu.pc + size > MEM_SIZE) {
        fault(cpu, "instruction runs past end of memory");
        return;
    }

    DecodedInstruction inst = decode(op);
    Byte group = inst.group;

    if (group == 0x0) {
        switch (op) {
        case OP_HALT: cpu.halted = true; break;
        case OP_NOP: break;
        case OP_OUT: std::cout << (char)cpu.a << std::flush; break;
        case OP_OUTN: std::cout << (int)cpu.a << ' ' << std::flush; break;
        default: unknown(cpu, op); return;
        }
    } else if (group == 0x1) {
        switch (op) {
        case OP_ADD: cpu.a = alu_add(cpu.a, cpu.b, cpu.f); break;
        case OP_SUB: cpu.a = alu_sub(cpu.a, cpu.b, cpu.f); break;
        case OP_AND: cpu.a = alu_and(cpu.a, cpu.b, cpu.f); break;
        case OP_OR:  cpu.a = alu_or(cpu.a, cpu.b, cpu.f);  break;
        case OP_XOR: cpu.a = alu_xor(cpu.a, cpu.b, cpu.f); break;
        case OP_NOT: cpu.a = alu_not(cpu.a, cpu.f);        break;
        case OP_SHL: cpu.a = alu_shl(cpu.a, cpu.f);        break;
        case OP_SHR: cpu.a = alu_shr(cpu.a, cpu.f);        break;
        case OP_INC: cpu.a = alu_inc(cpu.a, cpu.f);        break;
        case OP_DEC: cpu.a = alu_dec(cpu.a, cpu.f);        break;
        case OP_CMP: alu_sub(cpu.a, cpu.b, cpu.f);         break;
        default: unknown(cpu, op); return;
        }
    } else if (group == 0x2) {
        std::uint16_t addr = (size == 3) ? get16(*cpu.mem, cpu.pc + 1) : 0;
        bool uses_abs = (op == OP_LOAD_A || op == OP_LOAD_B ||
                         op == OP_STORE_A || op == OP_STORE_B);
        bool uses_h = (op == OP_LOAD_AH || op == OP_STORE_HA);

        if (uses_abs && addr >= MEM_SIZE) {
            fault(cpu, "address out of range");
            return;
        }
        if (uses_h && cpu.h >= MEM_SIZE) {
            fault(cpu, "H out of range");
            return;
        }

        switch (op) {
        case OP_LOADI_A: cpu.a = mem_get(*cpu.mem, cpu.pc + 1); break;
        case OP_LOADI_B: cpu.b = mem_get(*cpu.mem, cpu.pc + 1); break;
        case OP_LOAD_A: cpu.a = mem_get(*cpu.mem, addr); break;
        case OP_LOAD_B: cpu.b = mem_get(*cpu.mem, addr); break;
        case OP_STORE_A: mem_set(*cpu.mem, addr, cpu.a); break;
        case OP_STORE_B: mem_set(*cpu.mem, addr, cpu.b); break;
        case OP_MOV_AB: cpu.a = cpu.b; break;
        case OP_MOV_BA: cpu.b = cpu.a; break;
        case OP_LOADH: cpu.h = addr; break;
        case OP_LOAD_AH: cpu.a = mem_get(*cpu.mem, cpu.h); break;
        case OP_STORE_HA: mem_set(*cpu.mem, cpu.h, cpu.a); break;
        case OP_INCH: cpu.h = (std::uint16_t)(cpu.h + 1); break;
        case OP_DECH: cpu.h = (std::uint16_t)(cpu.h - 1); break;
        case OP_HLOW: cpu.a = (Byte)(cpu.h & 0xFF); break;
        default: unknown(cpu, op); return;
        }
    } else if (group == 0x3) {
        std::uint16_t addr = get16(*cpu.mem, cpu.pc + 1);
        bool take = false;
        if (op == OP_JMP) take = true;
        else if (op == OP_JZ) take = cpu.f.z;
        else if (op == OP_JNZ) take = !cpu.f.z;
        else { unknown(cpu, op); return; }
        if (take) {
            cpu.pc = addr;
            return;
        }
    } else {
        unknown(cpu, op);
        return;
    }

    cpu.pc = (std::uint16_t)(cpu.pc + size);
}

void run(CPU& cpu) {
    const int LIMIT = 100000;
    int steps = 0;
    while (!cpu.halted && steps < LIMIT) {
        step(cpu);
        ++steps;
    }
    if (!cpu.halted) {
        std::cout << "\nstep limit reached, PC=" << cpu.pc << '\n';
    } else {
        std::cout << '\n';
    }
}