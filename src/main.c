#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define HALT 0b01110110

// sources: http://dunfield.classiccmp.org/r/8080.txt, https://pastraiser.com/cpu/i8080/i8080_opcodes.html

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
    RP_BC,
    RP_DE,
    RP_HL,
    RP_SP
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

static inline Register extract_dst_reg(uint8_t opcode) {
    return (Register)((opcode >> 3) & 0x07);
}

static inline Register extract_src_reg(uint8_t opcode) {
    return (Register)(opcode & 0x07);
}

static inline RegisterPair extract_reg_pair(uint8_t opcode) {
    return (RegisterPair)((opcode >> 4) & 0x03);
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

static inline uint16_t lb_hb_to_uint16(uint8_t low_byte, uint8_t high_byte) {
    return ((uint16_t)high_byte << 8) | low_byte;
}

static inline void cpu_set_reg_pair(CpuState *cpu, RegisterPair rp, uint8_t low_byte, uint8_t high_byte) {
    switch(rp) {
        case RP_BC: cpu->b = high_byte; cpu->c = low_byte; break;
        case RP_DE: cpu->d = high_byte; cpu->e = low_byte; break;
        case RP_HL: cpu->h = high_byte; cpu->l = low_byte; break;
        case RP_SP: cpu->sp = lb_hb_to_uint16(low_byte, high_byte); break;
    }
}

static inline uint16_t cpu_get_reg_pair(CpuState *cpu, RegisterPair rp) {
    switch(rp) {
        case RP_BC: return lb_hb_to_uint16(cpu->c, cpu->b);
        case RP_DE: return lb_hb_to_uint16(cpu->e, cpu->d);
        case RP_HL: return lb_hb_to_uint16(cpu->l, cpu->h);
        case RP_SP: return cpu->sp;
    }
}

static inline uint8_t cpu_read_reg(CpuState *cpu, Register r) {
    switch(r) {
        case REG_M: return cpu->mem[cpu_get_hl(cpu)];
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

// returns byte from memory location at program counter + offset
static inline uint8_t read_byte(CpuState *cpu, uint8_t pc_offset) {
    return cpu->mem[cpu->pc+pc_offset];
}

// MOV  01DDDSSS         (moves DDD reg to SSS reg)
static inline void cpu_mov(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    Register src = extract_src_reg(opcode);
    cpu_set_reg(cpu, dst, cpu_read_reg(cpu, src));
    cpu->pc += 1;
}

// MVI  00DDD110 db      (moves immediate to DDD reg)
static inline void cpu_mvi(CpuState *cpu, uint8_t opcode) {
    Register dst = extract_dst_reg(opcode);
    uint8_t immediate = read_byte(cpu, 1);
    cpu_set_reg(cpu, dst, immediate);
    cpu->pc += 2;
}

// LXI  00RP0001 lb hb   (loads 16 bit immediate to register pair)
static inline void cpu_lxi(CpuState *cpu, uint8_t opcode) {
    RegisterPair dst = extract_reg_pair(opcode);
    cpu_set_reg_pair(cpu, dst, read_byte(cpu, 1), read_byte(cpu, 2));
    cpu->pc += 3;
}

// LDA  00111010 lb hb   (loads data from address to reg A)
static inline void cpu_lda(CpuState *cpu, uint8_t opcode) {
    cpu->a = cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))];
    cpu->pc += 3;
}

// STA  00110010 lb hb   (stores reg A to address)
static inline void cpu_sta(CpuState *cpu, uint8_t opcode) {
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))] = cpu->a;
    cpu->pc += 3;
}

// LHLD 00101010 lb hb   (load hl pair from mem)
static inline void cpu_lhld(CpuState *cpu, uint8_t opcode) {
    cpu_set_reg_pair(cpu, RP_HL, read_byte(cpu, 1), read_byte(cpu, 2));
    cpu->pc += 3;
}

// SHLD 00100010 lb hb   (stores hl to mem)
static inline void cpu_shld(CpuState *cpu, uint8_t opcode) {
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))] = cpu_read_reg(cpu, REG_L);
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))+1] = cpu_read_reg(cpu, REG_H);
    cpu->pc += 3;
}

// LDAX 00RP1010         (loads value from address from RP to A reg only BC or DE)
static inline void cpu_ldax(CpuState *cpu, uint8_t opcode) {
    cpu->a = cpu->mem[cpu_get_reg_pair(cpu, extract_reg_pair(opcode))];
    cpu->pc += 1;
}

// STAX 00RP0010         (stores value from A reg to adress from RP)
static inline void cpu_stax(CpuState *cpu, uint8_t opcode) {
    cpu->mem[cpu_get_reg_pair(cpu, extract_reg_pair(opcode))] = cpu->a;
    cpu->pc += 1;
}

// XCHG 11101011         (exchanges hl with de)
static inline void cpu_xchg(CpuState *cpu, uint8_t opcode) {
    uint8_t temp_l = cpu->l;
    uint8_t temp_h = cpu->h;
    cpu->l = cpu->e;
    cpu->h = cpu->d;
    cpu->e = temp_l;
    cpu->d = temp_h;
    cpu->pc += 1;
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

    }
}
