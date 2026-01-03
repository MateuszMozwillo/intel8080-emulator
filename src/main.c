#include "cpu.h"

int main() {

    uint8_t mem[] = {
        0x06, // MVI B, d8
        0x0A, // 10
        0x3E, // MVI A, d8
        0x01, // 1
        0x80, // ADD B
        0x32, // STA a 16
        0x01, // low byte 1
        0x00, // high byte 0
        0x76, // HALT
    };



    CpuState cpu = {0};
    cpu.mem = mem;

    while(!cpu.halted) {
        cpu.cycle += cpu_step(&cpu);
    }
    printf("%d %d\n", mem[1], cpu.parity_flag);
}
