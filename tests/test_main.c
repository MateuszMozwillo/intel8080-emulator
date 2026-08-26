#define UNITTEST_IMPLEMENTATION
#include "unittest.h"

#include "../src/cpu.h"

static uint8_t mem[0x10000];

static void reset_mem(void) {
    memset(mem, 0, sizeof(mem));
}

static CpuState create_cpu_state(Bus *bus, uint8_t *mem, size_t rom_size) {
    *bus = (Bus){.mem = mem, .rom_size = rom_size};
    return (CpuState){.bus = bus};
}

#define SETUP_TEST_CPU() \
    reset_mem(); \
    Bus bus; \
    CpuState cpu = create_cpu_state(&bus, mem, 8)

#define EXPECT_FLAGS(cpu, z, s, p, cy, ac) do { \
    EXPECT_EQ(z,  cpu.zero_flag); \
    EXPECT_EQ(s,  cpu.sign_flag); \
    EXPECT_EQ(p,  cpu.parity_flag); \
    EXPECT_EQ(cy, cpu.carry_flag); \
    EXPECT_EQ(ac, cpu.auxilary_flag); \
} while(0)

#define POISON_REGISTERS(cpu) do { \
    cpu.a = 0x11; cpu.b = 0x22; cpu.c = 0x33; cpu.d = 0x44; \
    cpu.e = 0x55; cpu.h = 0x66; cpu.l = 0x77; \
} while(0)

#define POISON_FLAGS(cpu) do { \
    cpu.zero_flag = 1; cpu.sign_flag = 0; cpu.parity_flag = 1; \
    cpu.carry_flag = 0; cpu.auxilary_flag = 1; \
} while(0)

TEST(data_transfer_instructions) {
    {
        SETUP_TEST_CPU();

        // MOV B, A
        POISON_REGISTERS(cpu);
        POISON_FLAGS(cpu);
        mem[0] = 0b01000111;
        cpu.a = 0x54;
        cpu.b = 0x45;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0x54, cpu.b);
        EXPECT_EQ(0x54, cpu.a);
        EXPECT_EQ(0x33, cpu.c);
        EXPECT_EQ(0x44, cpu.d);
        EXPECT_EQ(0x55, cpu.e);
        EXPECT_EQ(0x66, cpu.h);
        EXPECT_EQ(0x77, cpu.l);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // MOV M, A
        mem[0] = 0b01110111;
        cpu.h = 0x00;
        cpu.l = 0x10;
        cpu.a = 0xAB;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0xAB, mem[0x0010]);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // MVI A, 0xF8
        POISON_REGISTERS(cpu);
        POISON_FLAGS(cpu);
        mem[0] = 0x3E;
        mem[1] = 0xF8;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0xF8, cpu.a);
        EXPECT_EQ(0x22, cpu.b);
        EXPECT_EQ(0x33, cpu.c);
        EXPECT_EQ(0x44, cpu.d);
        EXPECT_EQ(0x55, cpu.e);
        EXPECT_EQ(0x66, cpu.h);
        EXPECT_EQ(0x77, cpu.l);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // MVI M, 0x3C
        mem[0] = 0x36;
        mem[1] = 0x3C;
        cpu.h = 0x00;
        cpu.l = 0x22;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x3C, mem[0x22]);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // LXI B, 0x1234
        POISON_FLAGS(cpu);
        mem[0] = 0x01;
        mem[1] = 0x34;
        mem[2] = 0x12;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x12, cpu.b);
        EXPECT_EQ(0x34, cpu.c);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // LXI SP, 0xBEEF
        mem[0] = 0x31;
        mem[1] = 0xEF;
        mem[2] = 0xBE;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0xBEEF, cpu.sp);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // STA 0x0040
        mem[0] = 0x32;
        mem[1] = 0x40;
        mem[2] = 0x00;
        cpu.a = 0xAA;

        EXPECT_EQ(13, cpu_step(&cpu));
        EXPECT_EQ(0xAA, mem[0x40]);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // LDA 0x0050
        mem[0] = 0x3A;
        mem[1] = 0x50;
        mem[2] = 0x00;
        mem[0x50] = 0xBC;

        EXPECT_EQ(13, cpu_step(&cpu));
        EXPECT_EQ(0xBC, cpu.a);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // SHLD 0x0060
        mem[0] = 0x22;
        mem[1] = 0x60;
        mem[2] = 0x00;
        cpu.h = 0x12;
        cpu.l = 0x34;

        EXPECT_EQ(16, cpu_step(&cpu));
        EXPECT_EQ(0x34, mem[0x60]);
        EXPECT_EQ(0x12, mem[0x61]);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // LHLD 0x0070
        mem[0] = 0x2A;
        mem[1] = 0x70;
        mem[2] = 0x00;
        mem[0x70] = 0x78;
        mem[0x71] = 0x56;

        EXPECT_EQ(16, cpu_step(&cpu));
        EXPECT_EQ(0x78, cpu.l);
        EXPECT_EQ(0x56, cpu.h);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // STAX B
        mem[0] = 0x02;
        cpu.b = 0x00;
        cpu.c = 0x80;
        cpu.a = 0xD1;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0xD1, mem[0x80]);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // LDAX D
        mem[0] = 0x1A;
        cpu.d = 0x00;
        cpu.e = 0x81;
        mem[0x81] = 0x6E;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x6E, cpu.a);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // XCHG
        mem[0] = 0xEB;
        cpu.h = 0x12;
        cpu.l = 0x34;
        cpu.d = 0xAB;
        cpu.e = 0xCD;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0xAB, cpu.h);
        EXPECT_EQ(0xCD, cpu.l);
        EXPECT_EQ(0x12, cpu.d);
        EXPECT_EQ(0x34, cpu.e);
        EXPECT_EQ(1, cpu.pc);
    }
}

TEST(arithmetic_and_compare_instructions) {
    {
        SETUP_TEST_CPU();

        // ADD B
        mem[0] = 0x80;
        cpu.a = 0x8F;
        cpu.b = 0x81;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x10, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 0, 1, 1);
    }

    {
        SETUP_TEST_CPU();

        // ADC B
        POISON_REGISTERS(cpu);
        mem[0] = 0x88;
        cpu.a = 0x01;
        cpu.b = 0x01;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x03, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 1, 0, 0);
        EXPECT_EQ(0x33, cpu.c);
        EXPECT_EQ(0x44, cpu.d);
    }

    {
        SETUP_TEST_CPU();

        // ADI 0x01
        mem[0] = 0xC6;
        mem[1] = 0x01;
        cpu.a = 0xFF;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // ACI 0x00
        mem[0] = 0xCE;
        mem[1] = 0x00;
        cpu.a = 0x0F;
        cpu.carry_flag = 1;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x10, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 0, 0, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // SUB B
        POISON_REGISTERS(cpu);
        mem[0] = 0x90;
        cpu.a = 0x10;
        cpu.b = 0x20;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0xF0, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 1, 0);
    }

    {
        SETUP_TEST_CPU();

        // SBB B
        mem[0] = 0x98;
        cpu.a = 0x10;
        cpu.b = 0x0F;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
    }

    {
        SETUP_TEST_CPU();

        // SUI 0x01
        POISON_REGISTERS(cpu);
        mem[0] = 0xD6;
        mem[1] = 0x01;
        cpu.a = 0x00;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0xFF, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 1, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // SBI 0x0F
        POISON_REGISTERS(cpu);
        mem[0] = 0xDE;
        mem[1] = 0x0F;
        cpu.a = 0x10;
        cpu.carry_flag = 1;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // CMP B
        POISON_REGISTERS(cpu);
        mem[0] = 0xB8;
        cpu.a = 0x42;
        cpu.b = 0x42;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x42, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 0);
    }

    {
        SETUP_TEST_CPU();

        // CMP C
        POISON_REGISTERS(cpu);
        mem[0] = 0xB9;
        cpu.a = 0x10;
        cpu.c = 0x20;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x10, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 1, 0);
    }

    {
        SETUP_TEST_CPU();

        // CPI 0x0F
        POISON_REGISTERS(cpu);
        mem[0] = 0xFE;
        mem[1] = 0x0F;
        cpu.a = 0x10;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x10, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 0, 0, 1);
        EXPECT_EQ(2, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // ADI 0x01 ; DAA
        mem[0] = 0xC6;
        mem[1] = 0x01;
        mem[2] = 0x27;
        cpu.a = 0x09;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x0A, cpu.a);

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x10, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 0, 0, 1);
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // ADI 0x01 ; DAA
        mem[0] = 0xC6;
        mem[1] = 0x01;
        mem[2] = 0x27;
        cpu.a = 0x99;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x9A, cpu.a);

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(3, cpu.pc);
    }
}

TEST(increment_and_register_pair_instructions) {
    {
        SETUP_TEST_CPU();

        // INR B
        mem[0] = 0x04;
        cpu.b = 0xFF;
        cpu.carry_flag = 1;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.b);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
    }

    {
        SETUP_TEST_CPU();

        // DCR C
        POISON_REGISTERS(cpu);
        mem[0] = 0x0D;
        cpu.c = 0x00;
        cpu.carry_flag = 0;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0xFF, cpu.c);
        EXPECT_FLAGS(cpu, 0, 1, 1, 0, 1);
        EXPECT_EQ(0x22, cpu.b);
        EXPECT_EQ(0x11, cpu.a);
    }

    {
        SETUP_TEST_CPU();

        // INX B
        mem[0] = 0x03;
        cpu.b = 0x12;
        cpu.c = 0xFF;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0x13, cpu.b);
        EXPECT_EQ(0x00, cpu.c);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // DCX H
        mem[0] = 0x2B;
        cpu.h = 0x34;
        cpu.l = 0x00;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0x33, cpu.h);
        EXPECT_EQ(0xFF, cpu.l);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // DAD D
        mem[0] = 0x19;
        cpu.h = 0xFF;
        cpu.l = 0xFF;
        cpu.d = 0x00;
        cpu.e = 0x01;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.h);
        EXPECT_EQ(0x00, cpu.l);
        EXPECT_EQ(1, cpu.carry_flag);
        EXPECT_EQ(1, cpu.pc);
    }
}

TEST(logical_instructions) {
    {
        SETUP_TEST_CPU();

        // ANA B
        POISON_REGISTERS(cpu);
        mem[0] = 0xA0;
        cpu.a = 0xF0;
        cpu.b = 0x0F;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x00, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
    }

    {
        SETUP_TEST_CPU();

        // ANI 0x0F
        mem[0] = 0xE6;
        mem[1] = 0x0F;
        cpu.a = 0x3C;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x0C, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 1, 0, 1);
    }

    {
        SETUP_TEST_CPU();

        // ORA B
        mem[0] = 0xB0;
        cpu.a = 0x10;
        cpu.b = 0x01;
        cpu.carry_flag = 1;
        cpu.auxilary_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x11, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 1, 0, 0);
    }

    {
        SETUP_TEST_CPU();

        // ORI 0x0F
        mem[0] = 0xF6;
        mem[1] = 0x0F;
        cpu.a = 0x10;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0x1F, cpu.a);
        EXPECT_FLAGS(cpu, 0, 0, 0, 0, 0);
    }

    {
        SETUP_TEST_CPU();

        // XRA B
        mem[0] = 0xA8;
        cpu.a = 0xFF;
        cpu.b = 0x0F;
        cpu.carry_flag = 1;
        cpu.auxilary_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0xF0, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 0, 0);
    }

    {
        SETUP_TEST_CPU();

        // XRI 0xFF
        mem[0] = 0xEE;
        mem[1] = 0xFF;
        cpu.a = 0x0F;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(0xF0, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 0, 0);
        EXPECT_EQ(2, cpu.pc);
    }
}

TEST(rotate_and_flag_instructions) {
    {
        SETUP_TEST_CPU();

        // RLC
        POISON_FLAGS(cpu);
        mem[0] = 0x07;
        cpu.a = 0x81;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x03, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // RRC
        POISON_FLAGS(cpu);
        mem[0] = 0x0F;
        cpu.a = 0x01;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x80, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // RAL
        POISON_FLAGS(cpu);
        mem[0] = 0x17;
        cpu.a = 0x80;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x01, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // RAR
        POISON_FLAGS(cpu);
        mem[0] = 0x1F;
        cpu.a = 0x01;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0x80, cpu.a);
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // CMA
        mem[0] = 0x2F;
        cpu.a = 0x55;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0xAA, cpu.a);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // CMC
        POISON_FLAGS(cpu);
        mem[0] = 0x3F;
        cpu.carry_flag = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_FLAGS(cpu, 1, 0, 1, 0, 1);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // STC
        POISON_FLAGS(cpu);
        mem[0] = 0x37;
        cpu.carry_flag = 0;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_FLAGS(cpu, 1, 0, 1, 1, 1);
        EXPECT_EQ(1, cpu.pc);
    }
}

TEST(stack_and_flow_control_instructions) {
    {
        SETUP_TEST_CPU();

        // JMP 0x0030
        mem[0] = 0xC3;
        mem[1] = 0x30;
        mem[2] = 0x00;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x0030, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // JZ 0x0040, taken
        mem[0] = 0xCA;
        mem[1] = 0x40;
        mem[2] = 0x00;
        cpu.zero_flag = 1;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x0040, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // JZ 0x0040, not taken
        mem[0] = 0xCA;
        mem[1] = 0x40;
        mem[2] = 0x00;
        cpu.zero_flag = 0;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(3, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // CZ 0x0008, taken
        mem[0] = 0xCC;
        mem[1] = 0x08;
        mem[2] = 0x00;
        cpu.zero_flag = 1;
        cpu.sp = 0x0100;

        EXPECT_EQ(17, cpu_step(&cpu));
        EXPECT_EQ(0x0008, cpu.pc);
        EXPECT_EQ(0x00FE, cpu.sp);
        EXPECT_EQ(0x03, mem[0x00FE]);
        EXPECT_EQ(0x00, mem[0x00FF]);
    }

    {
        SETUP_TEST_CPU();

        // CNZ 0x0008, not taken because Z is set
        mem[0] = 0xC4;
        mem[1] = 0x08;
        mem[2] = 0x00;
        cpu.zero_flag = 1;
        cpu.sp = 0x0100;

        EXPECT_EQ(11, cpu_step(&cpu));
        EXPECT_EQ(3, cpu.pc);
        EXPECT_EQ(0x0100, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // CALL 0x0005 ; RET
        mem[0] = 0xCD;
        mem[1] = 0x05;
        mem[2] = 0x00;
        mem[5] = 0xC9;
        cpu.sp = 0x0100;

        EXPECT_EQ(17, cpu_step(&cpu));
        EXPECT_EQ(0x0005, cpu.pc);
        EXPECT_EQ(0x00FE, cpu.sp);
        EXPECT_EQ(0x03, mem[0x00FE]);
        EXPECT_EQ(0x00, mem[0x00FF]);

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x0003, cpu.pc);
        EXPECT_EQ(0x0100, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // RZ, taken
        mem[0] = 0xC8;
        cpu.zero_flag = 1;
        cpu.sp = 0x00FE;
        mem[0x00FE] = 0x34;
        mem[0x00FF] = 0x12;

        EXPECT_EQ(11, cpu_step(&cpu));
        EXPECT_EQ(0x1234, cpu.pc);
        EXPECT_EQ(0x0100, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // RZ, not taken
        mem[0] = 0xC8;
        cpu.zero_flag = 0;
        cpu.sp = 0x00FE;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(1, cpu.pc);
        EXPECT_EQ(0x00FE, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // RST 5
        mem[0] = 0xEF;
        cpu.sp = 0x0100;

        EXPECT_EQ(11, cpu_step(&cpu));
        EXPECT_EQ(0x0028, cpu.pc);
        EXPECT_EQ(0x00FE, cpu.sp);
        EXPECT_EQ(0x01, mem[0x00FE]);
        EXPECT_EQ(0x00, mem[0x00FF]);
    }

    {
        SETUP_TEST_CPU();

        // PCHL
        mem[0] = 0xE9;
        cpu.h = 0x12;
        cpu.l = 0x34;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0x1234, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // PUSH B ; POP D
        mem[0] = 0xC5;
        mem[1] = 0xD1;
        cpu.b = 0x12;
        cpu.c = 0x34;
        cpu.sp = 0x0100;

        EXPECT_EQ(11, cpu_step(&cpu));
        EXPECT_EQ(0x00FE, cpu.sp);
        EXPECT_EQ(0x34, mem[0x00FE]);
        EXPECT_EQ(0x12, mem[0x00FF]);

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0x12, cpu.d);
        EXPECT_EQ(0x34, cpu.e);
        EXPECT_EQ(0x0100, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // PUSH PSW ; POP PSW
        mem[0] = 0xF5;
        mem[1] = 0xF1;
        cpu.a = 0xA5;
        cpu.sign_flag = 1;
        cpu.zero_flag = 0;
        cpu.auxilary_flag = 1;
        cpu.parity_flag = 1;
        cpu.carry_flag = 1;
        cpu.sp = 0x0100;

        EXPECT_EQ(11, cpu_step(&cpu));
        EXPECT_EQ(0x00FE, cpu.sp);
        EXPECT_EQ(0x97, mem[0x00FE]);
        EXPECT_EQ(0xA5, mem[0x00FF]);

        cpu.a = 0x00;
        cpu.sign_flag = 0;
        cpu.zero_flag = 1;
        cpu.auxilary_flag = 0;
        cpu.parity_flag = 0;
        cpu.carry_flag = 0;

        EXPECT_EQ(10, cpu_step(&cpu));
        EXPECT_EQ(0xA5, cpu.a);
        EXPECT_FLAGS(cpu, 0, 1, 1, 1, 1);
        EXPECT_EQ(0x0100, cpu.sp);
    }

    {
        SETUP_TEST_CPU();

        // XTHL
        mem[0] = 0xE3;
        cpu.sp = 0x0080;
        cpu.h = 0x12;
        cpu.l = 0x34;
        mem[0x80] = 0x78;
        mem[0x81] = 0x56;

        EXPECT_EQ(18, cpu_step(&cpu));
        EXPECT_EQ(0x78, cpu.l);
        EXPECT_EQ(0x56, cpu.h);
        EXPECT_EQ(0x34, mem[0x80]);
        EXPECT_EQ(0x12, mem[0x81]);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // SPHL
        mem[0] = 0xF9;
        cpu.h = 0xBE;
        cpu.l = 0xEF;

        EXPECT_EQ(5, cpu_step(&cpu));
        EXPECT_EQ(0xBEEF, cpu.sp);
        EXPECT_EQ(1, cpu.pc);
    }
}

TEST(system_instructions) {
    {
        SETUP_TEST_CPU();

        // EI
        mem[0] = 0xFB;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(1, cpu.interruptible);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // DI
        mem[0] = 0xF3;
        cpu.interruptible = 1;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(0, cpu.interruptible);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // HLT
        mem[0] = 0x76;

        EXPECT_EQ(7, cpu_step(&cpu));
        EXPECT_EQ(1, cpu.halted);
        EXPECT_EQ(1, cpu.pc);
    }

    {
        SETUP_TEST_CPU();

        // NOP
        mem[0] = 0x00;

        EXPECT_EQ(4, cpu_step(&cpu));
        EXPECT_EQ(1, cpu.pc);
    }
}

int main() {
    return run_all_tests();
}
