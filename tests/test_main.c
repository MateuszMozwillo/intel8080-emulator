#define UNITTEST_IMPLEMENTATION
#include "unittest.h"

#include "../src/cpu.h"

TEST(mov_instrucion) {
    uint8_t mem[256] = {0};
    Bus bus = {.mem = mem, .rom_size = 128};
    CpuState cpu = {.bus = &bus};

    // MOV B A
    mem[0] = 0b01111000;

    cpu.a = 0x54;
    cpu.b = 0x45;

    cpu_step(&cpu);

    EXPECT_EQ(0x45, cpu.b);
    EXPECT_EQ(0x45, cpu.a);
}

TEST(mvi_instruction) {
    uint8_t mem[256] = {0};
    Bus bus = {.mem = mem, .rom_size = 128};
    CpuState cpu = {.bus = &bus};

    // MVI A 0xF8
    mem[0] = 0b00111110;
    mem[1] = 0xF8;

    cpu_step(&cpu);

    EXPECT_EQ(0xF8, cpu.a);
}

int main() {
    return run_all_tests();
}
