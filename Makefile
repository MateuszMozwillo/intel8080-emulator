SRC = $(wildcard src/*.c)

SDL_CFLAGS = $(shell sdl2-config --cflags)
SDL_LIBS = $(shell sdl2-config --libs)

CORE_SRC = src/cpu.c

build:
	gcc -Wall $(SDL_CFLAGS) $(SRC) -o i8080 $(SDL_LIBS)

TEST_SRC = tests/test_main.c
TEST_BIN = run_tests

test:
	$(CC) $(CFLAGS) $(CORE_SRC) $(TEST_SRC) -o $(TEST_BIN)
	./$(TEST_BIN)

.PHONY: build test
