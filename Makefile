SRC = $(wildcard src/*.c)

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)

run: build
	./i8080

build:
	gcc -Wall $(SDL_CFLAGS) $(SRC) -o i8080 $(SDL_LIBS)
