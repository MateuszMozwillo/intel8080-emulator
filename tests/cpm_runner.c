#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/cpu.h"

#define MEM_SIZE    0x10000
#define LOAD_ADDR   0x0100
#define BDOS_ADDR   0x0005

static uint8_t mem[MEM_SIZE];

static size_t load_com(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;
    size_t len = fread(&mem[LOAD_ADDR], 1, MEM_SIZE - LOAD_ADDR, f);
    fclose(f);
    return len;
}

static void bdos_call(CpuState *cpu) {
    switch (cpu->c) {
        case 2:
            putchar(cpu->e);
            break;
        case 9: {
            uint16_t addr = ((uint16_t)cpu->d << 8) | cpu->e;
            while (mem[addr] != '$') {
                putchar(mem[addr++]);
            }
            break;
        }
        default:
            fprintf(stderr, "ERROR: unsupported BDOS call C=%d\n", cpu->c);
            break;
    }
    fflush(stdout);

    cpu->pc = mem[cpu->sp] | ((uint16_t)mem[(uint16_t)(cpu->sp + 1)] << 8);
    cpu->sp += 2;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "ERROR: usage: %s <program.com>\n", argv[0]);
        return 1;
    }

    if (!load_com(argv[1])) {
        fprintf(stderr, "ERROR: could not load %s\n", argv[1]);
        return 1;
    }

    Bus bus = {.mem = mem, .rom_size = 0};
    CpuState cpu = {.bus = &bus, .pc = LOAD_ADDR};

    uint64_t instructions = 0;

    while (!cpu.halted) {
        if (cpu.pc == BDOS_ADDR) {
            bdos_call(&cpu);
            continue;
        }
        if (cpu.pc == 0x0000) {
            break;
        }

        int cycles = cpu_step(&cpu);
        if (cycles < 0) {
            fprintf(stderr, "\nERROR: unknown opcode 0x%02X at 0x%04X\n", mem[cpu.pc], cpu.pc);
            return 1;
        }
        cpu.cycle += cycles;
        instructions++;
    }

    printf("\n\n%s after %llu instructions, %llu cycles\n",
           cpu.halted ? "HALTED" : "WARM BOOT",
           (unsigned long long)instructions, (unsigned long long)cpu.cycle);
    return 0;
}
