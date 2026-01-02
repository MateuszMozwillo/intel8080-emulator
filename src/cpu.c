#include "cpu.h"
#include <stdint.h>
#include <sys/types.h>

static inline Register extract_dst_reg(uint8_t opcode) {
    return (Register)((opcode >> 3) & 0x07);
}

static inline Register extract_src_reg(uint8_t opcode) {
    return (Register)(opcode & 0x07);
}

static inline RegisterPair extract_reg_pair(uint8_t opcode) {
    return (RegisterPair)((opcode >> 4) & 0x03);
}

static inline ConditionCode extract_condition_code(uint8_t opcode) {
    return (ConditionCode)((opcode >> 3) & 0x07);
}

static inline uint16_t cpu_get_hl(CpuState *cpu) {
    return (((uint16_t)cpu->h << 8) | cpu->l);
}

static inline uint16_t lb_hb_to_uint16(uint8_t low_byte, uint8_t high_byte) {
    return ((uint16_t)high_byte << 8) | low_byte;
}

static inline void cpu_set_reg_pair(CpuState *cpu, RegisterPair rp, uint8_t low_byte, uint8_t high_byte) {
    switch(rp) {
        case RP_BC: cpu->b = high_byte; cpu->c = low_byte; break;
        case RP_DE: cpu->d = high_byte; cpu->e = low_byte; break;
        case RP_HL: cpu->h = high_byte; cpu->l = low_byte; break;
        case RP_SP: cpu->sp = lb_hb_to_uint16(low_byte, high_byte); break;
    }
}

static inline uint16_t cpu_get_reg_pair(CpuState *cpu, RegisterPair rp) {
    switch(rp) {
        case RP_BC: return lb_hb_to_uint16(cpu->c, cpu->b);
        case RP_DE: return lb_hb_to_uint16(cpu->e, cpu->d);
        case RP_HL: return lb_hb_to_uint16(cpu->l, cpu->h);
        case RP_SP: return cpu->sp;
    }
}

static inline uint8_t cpu_read_reg(CpuState *cpu, Register r) {
    switch(r) {
        case REG_M: return cpu->mem[cpu_get_hl(cpu)];
        case REG_B: return cpu->b;
        case REG_C: return cpu->c;
        case REG_D: return cpu->d;
        case REG_E: return cpu->e;
        case REG_H: return cpu->h;
        case REG_L: return cpu->l;
        case REG_A: return cpu->a;
    }
}

static inline void cpu_set_reg(CpuState *cpu, Register r, uint8_t val) {
    switch(r) {
        case REG_M: cpu->mem[cpu_get_hl(cpu)] = val; break;
        case REG_B: cpu->b = val; break;
        case REG_C: cpu->c = val; break;
        case REG_D: cpu->d = val; break;
        case REG_E: cpu->e = val; break;
        case REG_H: cpu->h = val; break;
        case REG_L: cpu->l = val; break;
        case REG_A: cpu->a = val; break;
    }
}

static inline bool check_condition(CpuState *cpu, ConditionCode condition_code) {
    switch(condition_code) {
        case CC_NZ: return !cpu->zero_flag;
        case CC_Z:  return cpu->zero_flag;
        case CC_NC: return !cpu->carry_flag;
        case CC_C:  return cpu->carry_flag;
        case CC_PO: return !cpu->parity_flag;
        case CC_PE: return cpu->parity_flag;
        case CC_P:  return !cpu->sign_flag;
        case CC_M:  return cpu->sign_flag;
    }
}

static inline void cpu_stack_push(CpuState *cpu, uint16_t val) {
    cpu->mem[cpu->sp - 1] = (uint8_t)(val >> 8);
    cpu->mem[cpu->sp - 2] = (uint8_t)(val);
    cpu->sp -= 2;
}

static inline bool bitwise_parity(uint8_t n) {
    return __builtin_parity(n);
}

// returns byte from memory location at program counter + offset
static inline uint8_t read_byte(CpuState *cpu, uint8_t pc_offset) {
    return cpu->mem[cpu->pc+pc_offset];
}

// MOV  01DDDSSS         (moves DDD reg to SSS reg)
static inline void cpu_mov(CpuState *cpu) {
    Register dst = extract_dst_reg(read_byte(cpu, 0));
    Register src = extract_src_reg(read_byte(cpu, 0));
    cpu_set_reg(cpu, dst, cpu_read_reg(cpu, src));
    cpu->pc += 1;
}

// MVI  00DDD110 db      (moves immediate to DDD reg)
static inline void cpu_mvi(CpuState *cpu) {
    Register dst = extract_dst_reg(read_byte(cpu, 0));
    uint8_t immediate = read_byte(cpu, 1);
    cpu_set_reg(cpu, dst, immediate);
    cpu->pc += 2;
}

// LXI  00RP0001 lb hb   (loads 16 bit immediate to register pair)
static inline void cpu_lxi(CpuState *cpu) {
    RegisterPair dst = extract_reg_pair(read_byte(cpu, 0));
    cpu_set_reg_pair(cpu, dst, read_byte(cpu, 1), read_byte(cpu, 2));
    cpu->pc += 3;
}

// LDA  00111010 lb hb   (loads data from address to reg A)
static inline void cpu_lda(CpuState *cpu) {
    cpu->a = cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))];
    cpu->pc += 3;
}

// STA  00110010 lb hb   (stores reg A to address)
static inline void cpu_sta(CpuState *cpu) {
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))] = cpu->a;
    cpu->pc += 3;
}

// LHLD 00101010 lb hb   (load hl pair from mem)
static inline void cpu_lhld(CpuState *cpu) {
    uint16_t addr = lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2));
    cpu->l = cpu->mem[addr];
    cpu->h = cpu->mem[addr + 1];
    cpu->pc += 3;
}

// SHLD 00100010 lb hb   (stores hl to mem)
static inline void cpu_shld(CpuState *cpu) {
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))] = cpu_read_reg(cpu, REG_L);
    cpu->mem[lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2))+1] = cpu_read_reg(cpu, REG_H);
    cpu->pc += 3;
}

// LDAX 00RP1010         (loads value from address from RP to A reg only BC or DE)
static inline void cpu_ldax(CpuState *cpu) {
    cpu->a = cpu->mem[cpu_get_reg_pair(cpu, extract_reg_pair(read_byte(cpu, 0)))];
    cpu->pc += 1;
}

// STAX 00RP0010         (stores value from A reg to adress from RP)
static inline void cpu_stax(CpuState *cpu) {
    cpu->mem[cpu_get_reg_pair(cpu, extract_reg_pair(read_byte(cpu, 0)))] = cpu->a;
    cpu->pc += 1;
}

// XCHG 11101011         (exchanges hl with de)
static inline void cpu_xchg(CpuState *cpu) {
    uint8_t temp_l = cpu-> l;
    uint8_t temp_h = cpu->h;
    cpu->l = cpu->e;
    cpu->h = cpu->d;
    cpu->e = temp_l;
    cpu->d = temp_h;
    cpu->pc += 1;
}

// ADD 10000SSS          (add register to A)
static inline void cpu_add(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));
    uint16_t result = a + b;

    cpu->auxilary_flag = ((a & 0x0F) + (b & 0x0F)) > 0x0F;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = result > 0xFF;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 1;
}

// ADI 10000110  db      (add immidiate to A)
static inline void cpu_adi(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);
    uint16_t result = a + b;

    cpu->auxilary_flag = ((a & 0x0F) + (b & 0x0F)) > 0x0F;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = result > 0xFF;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 2;
}

// ADC 10001SSS          (add register to A with carry)
static inline void cpu_adc(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint16_t result = a + b + cpu->carry_flag;

    cpu->auxilary_flag = ((a & 0x0F) + (b & 0x0F)) + cpu->carry_flag > 0x0F;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = result > 0xFF;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 1;
}

// ACI 11001110 db        (add immediate to A with carry)
static inline void cpu_aci(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint16_t result = a + b + cpu->carry_flag;

    cpu->auxilary_flag = ((a & 0x0F) + (b & 0x0F)) + cpu->carry_flag > 0x0F;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = result > 0xFF;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 2;
}

// SUB 10010SSS             (subtract register from a)
static inline void cpu_sub(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint16_t result = a - b;

    cpu->auxilary_flag = (a & 0x0F) < (b & 0x0F);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) != 0;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 1;
}

// SUI 11010110 db          (subtract immediate from a)
static inline void cpu_sui(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint16_t result = a - b;

    cpu->auxilary_flag = (a & 0x0F) < (b & 0x0F);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) != 0;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 2;
}


// SBB 10011SSS             (subtract register from a with borrow)
static inline void cpu_sbb(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint16_t result = a - b - cpu->carry_flag;

    cpu->auxilary_flag = (a & 0x0F) < ((b & 0x0F) + cpu->carry_flag);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) != 0;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 1;
}

// SBI 11011110 db          (subtract immediate from a with borrow)
static inline void cpu_sbi(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint16_t result = a - b - cpu->carry_flag;

    cpu->auxilary_flag = (a & 0x0F) < ((b & 0x0F) + cpu->carry_flag);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) != 0;

    cpu_set_reg(cpu, REG_A, (uint8_t)result);
    cpu->pc += 2;
}

// INR 00DDD100             (increment register)
static inline void cpu_inr(CpuState *cpu) {
    Register dst_reg = extract_dst_reg(read_byte(cpu, 0));
    uint8_t reg_val = cpu_read_reg(cpu, dst_reg);
    uint16_t result = reg_val + 1;

    cpu->auxilary_flag = (reg_val & 0x0F) == 0x0F;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);

    cpu_set_reg(cpu, dst_reg, (uint8_t)result);
    cpu->pc += 1;
}

// DCR 00DDD101             (decrement register)
static inline void cpu_dcr(CpuState *cpu) {
    Register dst_reg = extract_dst_reg(read_byte(cpu, 0));
    uint8_t reg_val = cpu_read_reg(cpu, dst_reg);
    uint16_t result = reg_val - 1;

    cpu->auxilary_flag = (reg_val & 0x0F) == 0x00;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);

    cpu_set_reg(cpu, dst_reg, (uint8_t)result);
    cpu->pc += 1;
}

// INX 00RP0011             (increment register pair)
static inline void cpu_inx(CpuState *cpu) {
    RegisterPair rp = extract_reg_pair(read_byte(cpu, 0));
    uint16_t rp_val = cpu_get_reg_pair(cpu, rp);
    uint16_t result = rp_val + 1;
    cpu_set_reg_pair(cpu, rp, result & 0x0F, result & 0xF0);
    cpu->pc += 1;
}

// DCX 00RP1011             (decrement register pair)
static inline void cpu_dcx(CpuState *cpu) {
    RegisterPair rp = extract_reg_pair(read_byte(cpu, 0));
    uint16_t rp_val = cpu_get_reg_pair(cpu, rp);
    uint16_t result = rp_val - 1;
    cpu_set_reg_pair(cpu, rp, result & 0x0F, result & 0xF0);
    cpu->pc += 1;
}


// DAD 00RP1001             (Add register pair to HL (16 bit add))
static inline void cpu_dad(CpuState *cpu) {
    RegisterPair rp = extract_reg_pair(read_byte(cpu, 0));
    uint16_t val_to_add = cpu_get_reg_pair(cpu, rp);

    uint16_t hl_val = cpu_get_reg_pair(cpu, RP_HL);

    uint32_t result = (uint32_t)hl_val + (uint32_t)val_to_add;

    cpu->carry_flag = (result > 0xFFFF);

    cpu_set_reg_pair(cpu, RP_HL, (result & 0xFF), (result >> 8) & 0xFF);
    cpu->pc += 1;
}

// DAA 00100111             (Decimal Adjust Accumulator)
static inline void cpu_daa(CpuState *cpu) {
    bool cy = cpu->carry_flag;
    uint8_t correction = 0;
    uint8_t lsb = cpu->a & 0x0F;
    uint8_t msb = cpu->a >> 4;

    if (lsb > 9 || cpu->auxilary_flag) {
        correction += 0x06;
    }

    if (msb > 9 || cy || (msb >= 9 && lsb > 9)) {
        correction += 0x60;
        cy = true;
    }

    uint16_t result = (uint16_t)cpu->a + correction;

    cpu->auxilary_flag = ((cpu->a & 0x0F) + (correction & 0x0F)) > 0x0F;
    cpu->carry_flag = cy;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result & 0x80) != 0;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->a = (uint8_t)result;
    cpu->pc += 1;
}

// ANA 10100SSS             (and register with A)
static inline void cpu_ana(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint8_t result = a & b;

    cpu->auxilary_flag = ((a | b) & 0x08) != 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 1;
}

// ANI 11100110  db         (and immediate with a)
static inline void cpu_ani(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint8_t result = a & b;

    cpu->auxilary_flag = ((a | b) & 0x08) != 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 2;
}

// ORA 10110SSS            (or reg with A)
static inline void cpu_ora(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint8_t result = a | b;

    cpu->auxilary_flag = 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 1;
}

// ORI 11110110 DB           (or immediate with A)
static inline void cpu_ori(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint8_t result = a | b;

    cpu->auxilary_flag = 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 2;
}

// XRA 10101SSS           (xor reg with A)
static inline void cpu_xra(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint8_t result = a ^ b;

    cpu->auxilary_flag = 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 1;
}

// XRI 11101110 DB           (xor immediate with A)
static inline void cpu_xri(CpuState *cpu) {

    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint8_t result = a ^ b;

    cpu->auxilary_flag = 0;
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = 0;

    cpu_set_reg(cpu, REG_A, result);
    cpu->pc += 2;
}

// CMP 10111SSS           (compare reg with A)
static inline void cpu_cmp(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = cpu_read_reg(cpu, extract_src_reg(read_byte(cpu, 0)));

    uint16_t result = a - b;

    cpu->auxilary_flag = (a & 0x0F) < (b & 0x0F);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) == 0x0100;

    cpu->pc += 1;
}

// CPI 11111110 DB          (compare immediate with A)
static inline void cpu_cpi(CpuState *cpu) {
    uint8_t a = cpu_read_reg(cpu, REG_A);
    uint8_t b = read_byte(cpu, 1);

    uint16_t result = a - b;

    cpu->auxilary_flag = (a & 0x0F) < (b & 0x0F);
    cpu->zero_flag = (uint8_t)result == 0x00;
    cpu->sign_flag = ((uint8_t)result >> 7) == 0x01;
    cpu->parity_flag = bitwise_parity((uint8_t)result);
    cpu->carry_flag = (result & 0xFF00) != 0;

    cpu->pc += 2;
}

// RLC 00000111             (rotate A left)
static inline void cpu_rlc(CpuState *cpu) {
    uint8_t val = cpu_read_reg(cpu, REG_A);
    uint8_t msb = (val & 0x80) >> 7;
    cpu->carry_flag = msb;
    cpu_set_reg(cpu, REG_A, (val << 1) | msb);
    cpu->pc += 1;
}

// RRC 00001111             (rotate A right)
static inline void cpu_rrc(CpuState *cpu) {
    uint8_t val = cpu_read_reg(cpu, REG_A);
    uint8_t lsb = (val & 0x01) << 7;
    cpu->carry_flag = (val & 0x01);
    cpu_set_reg(cpu, REG_A, (val >> 1) | lsb);
    cpu->pc += 1;
}

// RAL 000010111            (rotate A left through carry)
static inline void cpu_ral(CpuState *cpu) {
    uint8_t val = cpu_read_reg(cpu, REG_A);
    uint8_t msb = (val & 0x80) >> 7;
    bool current_carry = cpu->carry_flag;
    cpu->carry_flag = msb;
    cpu_set_reg(cpu, REG_A, (val << 1) | current_carry);
    cpu->pc += 1;
}

// RAR 000011111             (rotate A right through carry)
static inline void cpu_rar(CpuState *cpu) {
    uint8_t val = cpu_read_reg(cpu, REG_A);
    bool current_carry = cpu->carry_flag;
    cpu->carry_flag = (val & 0x01);
    cpu_set_reg(cpu, REG_A, (val >> 1) | (current_carry << 7));
    cpu->pc += 1;
}

// CMA 00101111              (compliment A)
static inline void cpu_cma(CpuState *cpu) {
    uint8_t val = cpu_read_reg(cpu, REG_A);
    cpu_set_reg(cpu, REG_A, ~val);
    cpu->pc += 1;
}

// CMC 00111111              (compliment carry flag)
static inline void cpu_cmc(CpuState *cpu) {
    cpu->carry_flag = !cpu->carry_flag;
    cpu->pc += 1;
}

// STC 00110111              (set carry flag)
static inline void cpu_stc(CpuState *cpu) {
    cpu->carry_flag = 1;
    cpu->pc += 1;
}

// JMP 11000011 lb hb        (unconditional jump)
static inline void cpu_jmp(CpuState *cpu) {
    cpu->pc = lb_hb_to_uint16(read_byte(cpu, 1), read_byte(cpu, 2));
}

// Jccc 11CCC010 lb hb       (conditional jump)
static inline void cpu_jccc(CpuState *cpu) {
    if (check_condition(cpu, extract_condition_code(read_byte(cpu, 0)))) {
        cpu_jmp(cpu);
    } else {
        cpu->pc += 3;
    }
}

// CALL 11001101 lb hb       (unconditional subrutine call)
static inline void cpu_call(CpuState *cpu) {
    uint16_t return_addr = cpu->pc + 3;
    cpu_stack_push(cpu, return_addr);
    cpu_jmp(cpu);
}

// Cccc 11CCC100 lb hb       (conditional subrutine call)
static inline void cpu_Cccc(CpuState *cpu) {
    if (check_condition(cpu, extract_condition_code(read_byte(cpu, 0)))) {
        cpu_call(cpu);
    } else {
        cpu->pc += 3;
    }
}

// RET 11001001              (unconditional return from subrutine)
static inline void cpu_ret(CpuState *cpu) {
    uint8_t lo = cpu->mem[cpu->sp];
    uint8_t hi = cpu->mem[cpu->sp + 1];
    cpu->pc = lb_hb_to_uint16(lo, hi);
    cpu->sp += 2;
}

// Rccc 11CCC000             (Conditional return from subrutine)
static inline void cpu_Rccc(CpuState *cpu) {
    if (check_condition(cpu, extract_condition_code(read_byte(cpu, 0)))) {
        cpu_ret(cpu);
    } else {
        cpu->pc += 1;
    }
}

// RST 11NNN111              (Restart / Call to address N * 8)
static inline void cpu_rst(CpuState *cpu) {
    uint16_t return_addr = cpu->pc + 1;
    cpu_stack_push(cpu, return_addr);
    cpu->pc = (uint16_t)(read_byte(cpu, 0) & 0x38);
}

// PCHL 11101001             (Jump to address in HL)
static inline void cpu_pchl(CpuState *cpu) {
    cpu->pc = cpu_get_reg_pair(cpu, RP_HL);
}

// PUSH 11RP0101             (push register pair on the stack)
static inline void cpu_push(CpuState *cpu) {
    RegisterPair rp = extract_reg_pair(read_byte(cpu, 0));
    uint16_t val_to_push;
    if (rp == RP_SP) {
        uint8_t flags = 0x02;
        if (cpu->sign_flag)     flags |= 0x80;
        if (cpu->zero_flag)     flags |= 0x40;
        if (cpu->auxilary_flag) flags |= 0x10;
        if (cpu->parity_flag)   flags |= 0x04;
        if (cpu->carry_flag)    flags |= 0x01;
        val_to_push = ((uint16_t)cpu->a << 8) | flags;
    } else {
        val_to_push = cpu_get_reg_pair(cpu, rp);
    }
    cpu_stack_push(cpu, val_to_push);
    cpu->pc += 1;
}

// POP 11RP0001          (Pop register pair from stack)
static inline void cpu_pop(CpuState *cpu) {
    RegisterPair rp = extract_reg_pair(read_byte(cpu, 0));

    uint8_t low_byte = cpu->mem[cpu->sp];
    uint8_t high_byte = cpu->mem[cpu->sp + 1];

    cpu->sp += 2;

    if (rp == RP_SP) {

        cpu->a = high_byte;

        cpu->sign_flag = (low_byte & 0x80) != 0;
        cpu->zero_flag = (low_byte & 0x40) != 0;
        cpu->auxilary_flag = (low_byte & 0x10) != 0;
        cpu->parity_flag = (low_byte & 0x04) != 0;
        cpu->carry_flag = (low_byte & 0x01) != 0;
    } else {
        cpu_set_reg_pair(cpu, rp, low_byte, high_byte);
    }

    cpu->pc += 1;
}

// XTHL 11100011             (Exchange top of stack with HL)
static inline void cpu_xthl(CpuState *cpu) {
    uint8_t stack_lo = cpu->mem[cpu->sp];
    uint8_t stack_hi = cpu->mem[cpu->sp + 1];

    cpu->mem[cpu->sp] = cpu->l;
    cpu->mem[cpu->sp + 1] = cpu->h;

    cpu->l = stack_lo;
    cpu->h = stack_hi;

    cpu->pc += 1;
}

// SPHL 11111001             (Set SP to content of HL)
static inline void cpu_sphl(CpuState *cpu) {
    cpu->sp = cpu_get_reg_pair(cpu, RP_HL);
    cpu->pc += 1;
}

// IN 11011011 pa            (read input port into A)
static inline void cpu_in(CpuState *cpu) {
    uint8_t port = read_byte(cpu, 1);

    // TODO: do something with this

    cpu->pc += 2;
}

// OUT 11010011 pa           (Write A to output port)
static inline void cpu_out(CpuState *cpu) {
    uint8_t port = read_byte(cpu, 1);
    uint8_t data = cpu->a;

    // TODO: do something with this

    cpu->pc += 2;
}

// EI 11111011               (Enable interrupts)
static inline void cpu_ei(CpuState *cpu) {
    cpu->interruptible = true;
    cpu->pc += 1;
}

// DI 11110011               (Disable interrupts)
static inline void cpu_di(CpuState *cpu) {
    cpu->interruptible = false;
    cpu->pc += 1;
}

// HLT 01110110              (Halt processor)
static inline void cpu_hlt(CpuState *cpu) {
    cpu->halted = true;
    cpu->pc += 1;
}

// NOP 00000000              (No operation)
static inline void cpu_nop(CpuState *cpu) {
    cpu->pc += 1;
}

// returns number of cycles consumed by instruction
static inline int cpu_step(CpuState *cpu) {
    switch (read_byte(cpu, 0)) {
        case 0x00: case 0x10: case 0x20: case 0x30:
        case 0x08: case 0x18: case 0x28: case 0x38:
            cpu_nop(cpu); return 4;

        case 0x01: case 0x11: case 0x21: case 0x31:
            cpu_lxi(cpu); return 10;

        case 0x02: case 0x12:
            cpu_stax(cpu); return 7;

        case 0x03: case 0x13: case 0x23: case 0x33:
            cpu_inx(cpu); return 5;

        case 0x04: case 0x14: case 0x24:
            cpu_inr(cpu); return 5;

        case 0x34:
            cpu_inr(cpu); return 10;

        case 0x05: case 0x15: case 0x25:
            cpu_dcr(cpu); return 5;

        case 0x35:
            cpu_dcr(cpu); return 10;

        case 0x06: case 0x16: case 0x26:
        case 0x0E: case 0x1E: case 0x2E: case 0x3E:
            cpu_mvi(cpu); return 7;

        case 0x36:
            cpu_mvi(cpu); return 10;

        case 0x07:
            cpu_rlc(cpu); return 4;

        case 0x09: case 0x19: case 0x29: case 0x39:
            cpu_dad(cpu); return 10;
    }
}
