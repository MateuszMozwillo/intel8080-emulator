#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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

typedef struct {
    uint8_t mem[0x4000];
} Bus;

typedef struct {
    uint8_t a, b, c, d, e, h, l;

    uint16_t sp, pc;

    bool carry_flag;
    bool parity_flag;
    bool auxilary_flag;
    bool zero_flag;
    bool sign_flag;

    bool halted;
    bool interruptible;

    uint8_t port0;
    uint8_t port1;
    uint8_t port2;
    uint8_t port3;

    uint64_t cycle;

    Bus *bus;
} CpuState;

int cpu_step(CpuState *cpu);
