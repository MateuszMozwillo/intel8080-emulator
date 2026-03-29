#define UNITTEST_IMPLEMENTATION
#include "unittest.h"

#include "../src/cpu.h"

TEST(mov_instrucion) {
    {
        uint8_t mem[256] = {0};
        Bus bus = {.mem = mem, .rom_size = 8};
        CpuState cpu = {.bus = &bus};

        // MOV B A
        mem[0] = 0b01000111;

        cpu.a = 0x54;
        cpu.b = 0x45;

        EXPECT_EQ(5, cpu_step(&cpu));

        EXPECT_EQ(0x54, cpu.b);
        EXPECT_EQ(0x54, cpu.a);
    }

    {
        uint8_t mem[256] = {0};
        Bus bus = {.mem = mem, .rom_size = 8};
        CpuState cpu = {.bus = &bus};

        cpu.h = 0x00;
        cpu.l = 0x10;

        cpu.a = 0xAB;

        // MOV M A
        mem[0] = 0b01110111;

        EXPECT_EQ(7, cpu_step(&cpu));

        EXPECT_EQ(0xAB, mem[0x0010]);
    }
}

TEST(mvi_instruction) {
    {
        uint8_t mem[256] = {0};
        Bus bus = {.mem = mem, .rom_size = 8};
        CpuState cpu = {.bus = &bus};

        // MVI A 0xF8
        mem[0] = 0b00111110;
        mem[1] = 0xF8;

        EXPECT_EQ(7, cpu_step(&cpu));

        EXPECT_EQ(0xF8, cpu.a);
    }

    {

    }
}

int main() {
    return run_all_tests();
}
