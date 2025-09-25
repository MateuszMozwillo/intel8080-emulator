#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HALT 0b01110110

typedef enum {
    B,
    C,
    D,
    E,
    H,
    L,
    M,
    A
} Register;

Register extract_dst_reg(uint8_t instruction) {
    return (Register)((instruction >> 3) & 0x07);
}

Register extract_src_reg(uint8_t instruction) {
    return (Register)(instruction & 0x07);
}

typedef struct {
    uint8_t a, b, c, d, e, h, l;

    uint16_t sp, pc;

    uint8_t flag_reg;

    uint8_t* mem;
} CpuState;

uint16_t get_hl(CpuState *cpu) {
    return (((uint16_t)cpu->h << 8) | cpu->l);
}

uint8_t cpu_read_reg(CpuState *cpu, Register r) {
    switch(r) {
        case M: return get_hl(cpu);
        case B: return cpu->b;
        case C: return cpu->c;
        case D: return cpu->d;
        case E: return cpu->e;
        case H: return cpu->h;
        case L: return cpu->l;
        case A: return cpu->a;
    }
}

void cpu_set_reg(CpuState *cpu, Register r, uint8_t val) {
    switch(r) {
        case M: cpu->mem[get_hl(cpu)] = val; break;
        case B: cpu->b = val; break;
        case C: cpu->c = val; break;
        case D: cpu->d = val; break;
        case E: cpu->e = val; break;
        case H: cpu->h = val; break;
        case L: cpu->l = val; break;
        case A: cpu->a = val; break;
    }
}

// MOV 01DDDSSS
void cpu_mov(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    Register src = extract_src_reg(opcode);
    cpu_set_reg(cpu, dst, cpu_read_reg(cpu, src));
}

// MVI 00DDD110 db
void cpu_mvi(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    cpu->pc++;
    uint8_t immediate = cpu->mem[cpu->pc];
    cpu_set_reg(cpu, dst, immediate);
}

int main() {

    uint8_t mem[] = {
        0b01000001, // MOV B C
        0b01110110, // HALT
    };

    CpuState cpu = {0};
    cpu.mem = mem;

    while(cpu.mem[cpu.pc] != HALT) {
        uint8_t opcode = cpu.mem[cpu.pc];

        if ((opcode & 0b11000000) == 0b01000000) {
            cpu_mov(&cpu, opcode);
        }
        
        else if ((opcode & 0b11000111) == 0b00000110) {
            cpu_mvi(&cpu, opcode);
        }

        cpu.pc++;
    }

}
