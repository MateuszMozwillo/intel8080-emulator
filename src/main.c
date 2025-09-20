#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PC program_counter
#define SP stack_pointer

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

int main() {

    uint8_t mem[] = {
        0b01000001, // MOV B C
        0b01110110, // HALT
    };

    uint8_t registers[8] = {0, 0, 0, 0, 0, 0, -1, 0};

    uint16_t program_counter = 0;
    uint16_t stack_pointer = 0;

    while(mem[PC] != HALT) {
        // MOV 01DDDSSS
        if ((mem[PC] & 0b11000000) == 0b01000000) {
            Register destination;
            Register source;
            if ((mem[PC] & 0b00111000) == 0b00111000) { destination = A; };
            if ((mem[PC] & 0b00111000) == 0b00000000) { destination = B; };
            if ((mem[PC] & 0b00111000) == 0b00001000) { destination = C; };
            if ((mem[PC] & 0b00111000) == 0b00010000) { destination = D; };
            if ((mem[PC] & 0b00111000) == 0b00011000) { destination = E; };
            if ((mem[PC] & 0b00111000) == 0b00100000) { destination = H; };
            if ((mem[PC] & 0b00111000) == 0b00101000) { destination = L; };
            if ((mem[PC] & 0b00111000) == 0b00110000) { destination = M; };

            if ((mem[PC] & 0b00000111) == 0b00000111) { source = A; };
            if ((mem[PC] & 0b00000111) == 0b00000000) { source = B; };
            if ((mem[PC] & 0b00000111) == 0b00000001) { source = C; };
            if ((mem[PC] & 0b00000111) == 0b00000010) { source = D; };
            if ((mem[PC] & 0b00000111) == 0b00000011) { source = E; };
            if ((mem[PC] & 0b00000111) == 0b00000100) { source = H; };
            if ((mem[PC] & 0b00000111) == 0b00000101) { source = L; };
            if ((mem[PC] & 0b00000111) == 0b00000110) { source = M; };

            if ((destination != M) && (source != M)) {
                registers[destination] = registers[source];
            }

            if ((destination == M) && (source != M)) {
                
            }

        }
    }

}
