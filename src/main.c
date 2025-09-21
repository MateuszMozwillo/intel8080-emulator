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

#define HL_VAL ((registers[H] << 8) + registers[L])

Register extract_dst_reg(uint8_t instruction) {
    switch (instruction & 0x38) {
    case 0x38: return A;
    case 0x00: return B;
    case 0x08: return C;
    case 0x10: return D;
    case 0x18: return E;
    case 0x20: return H;
    case 0x28: return L;
    case 0x39: return M;
    }
}

Register extract_src_reg(uint8_t instruction) {
    switch (instruction & 0x07) {
    case 0x07: return A;
    case 0x00: return B;
    case 0x01: return C;
    case 0x02: return D;
    case 0x03: return E;
    case 0x04: return H;
    case 0x05: return L;
    case 0x06: return M;
    }
}

int main() {

    uint8_t mem[] = {
        0b01000001, // MOV B C
        0b01110110, // HALT
    };

    uint8_t registers[8] = {0, 0, 0, 0, 0, 0, -1, 0};

    uint16_t pc = 0;
    uint16_t sp = 0;

    printf("%d\n", HL_VAL);

    while(mem[pc] != HALT) {
        // MOV 01DDDSSS
        if ((mem[pc] & 0b11000000) == 0b01000000) {
            Register dst = extract_dst_reg(mem[pc]);
            Register src = extract_src_reg(mem[pc]);

            if ((dst != M) && (src != M)) {
                registers[dst] = registers[src];
            }

            if ((dst == M) && (src != M)) {
                mem[HL_VAL] = registers[src];
            }

            if ((dst != M) && (src == M)) {
                registers[dst] = mem[HL_VAL];
            }
        }

        // MVI 00DDD110 db
        if ((mem[pc] & 0b11000111) == 0b00000110) {
            
        }
        pc++;
    }

}
