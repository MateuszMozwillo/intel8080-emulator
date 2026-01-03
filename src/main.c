#include <SDL2/SDL.h>
#include <stdlib.h>
#include "cpu.h"
#include <string.h>

const int MEM_SIZE = 65536;

const int SCREEN_SIZE = 64;

const int SCREEN_VRAM_START = MEM_SIZE - (SCREEN_SIZE * SCREEN_SIZE);

int main() {

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "i8080 emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_SIZE*10,
        SCREEN_SIZE*10,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, 64, 64);

    SDL_Event e;

    const uint8_t program[] = {
        0x06, // MVI B, d8
        0x0A, // 10
        0x3E, // MVI A, d8
        0x01, // 1
        0x80, // ADD B
        0xC3, // JMP
        0x00,
        0x00,
        0x76, // HALT
    };

    uint8_t *mem = calloc(MEM_SIZE, 1);

    CpuState cpu = {0};
    cpu.mem = mem;

    memcpy(mem, program, sizeof(program));

    while(!cpu.halted) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                goto exit;
            }
        }

        cpu.cycle += cpu_step(&cpu);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        for (int i = SCREEN_VRAM_START; i < MEM_SIZE; i++) {
            uint8_t pixel = cpu.mem[i];
            if (pixel != 0) {
                int pixel_index = i - SCREEN_VRAM_START;

                int x = pixel_index % SCREEN_SIZE;
                int y = pixel_index / SCREEN_SIZE;
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }

        SDL_RenderPresent(renderer);
    }

exit:
    SDL_DestroyWindow(window);
    SDL_Quit();

    free(mem);
}
