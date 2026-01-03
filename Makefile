SRC = $(wildcard src/*.c)

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)

run: build
	./main

build:
	gcc -Wall $(SDL_CFLAGS) $(SRC) -o main $(SDL_LIBS)

clean:
	rm -f main
