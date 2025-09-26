#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HALT 0b01110110

typedef enum {
    REG_B,
    REG_C,
    REG_D,
    REG_E,
    REG_H,
    REG_L,
    REG_M,
    REG_A
} Register;

typedef enum {
    REG_P_BC,
    REG_P_DE,
    REG_P_HL,
    REG_P_SP
} RegisterPair;

typedef enum {
    CC_NZ,
    CC_Z,
    CC_NC,
    CC_C,
    CC_PO,
    CC_PE,
    CC_P,
    CC_M
} ConditionCode;

static inline Register extract_dst_reg(uint8_t instruction) {
    return (Register)((instruction >> 3) & 0x07);
}

static inline Register extract_src_reg(uint8_t instruction) {
    return (Register)(instruction & 0x07);
}

typedef struct {
    uint8_t a, b, c, d, e, h, l;

    uint16_t sp, pc;

    uint8_t flag_reg;

    uint64_t cycle;

    uint8_t* mem;
} CpuState;

static inline uint16_t cpu_get_hl(CpuState *cpu) {
    return (((uint16_t)cpu->h << 8) | cpu->l);
}

static inline uint8_t cpu_read_reg(CpuState *cpu, Register r) {
    switch(r) {
        case REG_M: return cpu_get_hl(cpu);
        case REG_B: return cpu->b;
        case REG_C: return cpu->c;
        case REG_D: return cpu->d;
        case REG_E: return cpu->e;
        case REG_H: return cpu->h;
        case REG_L: return cpu->l;
        case REG_A: return cpu->a;
    }
}

static inline void cpu_set_reg(CpuState *cpu, Register r, uint8_t val) {
    switch(r) {
        case REG_M: cpu->mem[cpu_get_hl(cpu)] = val; break;
        case REG_B: cpu->b = val; break;
        case REG_C: cpu->c = val; break;
        case REG_D: cpu->d = val; break;
        case REG_E: cpu->e = val; break;
        case REG_H: cpu->h = val; break;
        case REG_L: cpu->l = val; break;
        case REG_A: cpu->a = val; break;
    }
}

// MOV 01DDDSSS         (moves DDD reg to SSS reg)
static inline void cpu_mov(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    Register src = extract_src_reg(opcode);
    cpu_set_reg(cpu, dst, cpu_read_reg(cpu, src));
    cpu->pc++;
}

// MVI 00DDD110 db      (moves immediate to DDD reg)
static inline void cpu_mvi(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    uint8_t immediate = cpu->mem[cpu->pc+1];
    cpu_set_reg(cpu, dst, immediate);
    cpu->pc += 2;
}

// LXI 00RP0001 lb hb   (loads 16 bit immediate to register pair)
static inline void cpu_lxi(CpuState *cpu, uint8_t opcode) {

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

    }
}
