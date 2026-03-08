#include <stdlib.h>
#include "cpu.h"
#include <string.h>
#include <unistd.h>

const int MEM_SIZE = 0x10000;

typedef struct {
    uint8_t *bytes;
    size_t len;
} ByteCode;

int load_bytecode(const char *filename, ByteCode *out_bc) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 0;

    size_t length;
    if (fread(&length, sizeof(size_t), 1, f) != 1) {
        fclose(f);
        return 0;
    }

    out_bc->len = length;

    out_bc->bytes = (uint8_t*)malloc(length);

    if (!out_bc->bytes) {
        fclose(f);
        return 0;
    }

    fread(out_bc->bytes, 1, length, f);

    fclose(f);
    return 1;
}

#define MAX_FILENAME_LEN 100

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("ERROR: invalid argument count specify path to input file\n");
        return 1;
    }

    ByteCode byte_code;
    if (!load_bytecode(argv[1], &byte_code)) {
        printf("ERROR: error while loading bytecode\n");
        return 1;
    }

    CpuState cpu = {0};
    Bus bus = {0};
    bus.rom_size = byte_code.len;
    bus.mem = calloc(MEM_SIZE, 1);
    cpu.bus = &bus;

    memcpy(bus.mem, byte_code.bytes, byte_code.len);
    free(byte_code.bytes);

    while(!cpu.halted) {
        cpu.cycle += cpu_step(&cpu);
    }

    printf("halted: %d\n", cpu.halted);

    free(bus.mem);
}
