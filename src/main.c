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

#define HL_VAL (uint16_t)((registers[H] << 8) + registers[L])

Register extract_dst_reg(uint8_t instruction) {
    return (Register)((instruction >> 3) & 0x07);
}

Register extract_src_reg(uint8_t instruction) {
    return (Register)(instruction & 0x07);
}

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;

    uint16_t sp;
    uint16_t pc;

    uint8_t mem[0x10000];
} CpuState;

void cpu_set_reg(CpuState* cpu, Register reg, uint8_t to_set) {

}

int main() {

    CpuState cpu;
    cpu.mem[0] = 0b01000001; // MOV B C
    cpu.mem[1] = 0b01110110; // HALT

    uint8_t mem[] = {
        0b01000001, // MOV B C
        0b01110110, // HALT
    };

    uint8_t registers[8] = {0, 0, 0, 0, 0, 0, -1, 0};

    uint16_t pc = 0;
    uint16_t sp = 0;

    while(mem[pc] != HALT) {
        // MOV 01DDDSSS
        if ((mem[pc] & 0b11000000) == 0b01000000) {
            Register dst = extract_dst_reg(mem[pc]);
            Register src = extract_src_reg(mem[pc]);

            if ((dst != M) && (src != M)) {
                registers[dst] = registers[src];
            } 
            else if ((dst == M) && (src != M)) {
                mem[HL_VAL] = registers[src];
            } 
            else if ((dst != M) && (src == M)) {
                registers[dst] = mem[HL_VAL];
            }
        }

        // MVI 00DDD110 db
        if ((mem[pc] & 0b11000111) == 0b00000110) {
            Register dst = extract_dst_reg(mem[pc]);
            pc++;
            uint8_t immediate = mem[pc];
            if (dst != M) {
                registers[dst] = immediate;
            } else if (dst == M) {
                mem[HL_VAL] = immediate;
            }
        }
        pc++;
    }

}
