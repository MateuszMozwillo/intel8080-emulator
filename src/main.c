#include "cpu.h"

int main() {

    uint8_t mem[] = {
        0b01000001, // MOV B C
        0b01110110, // HALT
    };

    CpuState cpu = {0};
    cpu.mem = mem;

    printf("hello world\n");

    // while(cpu.mem[cpu.pc] != 0b01110110) {
    //     uint8_t opcode = cpu.mem[cpu.pc];

    // }
}
