SRC = $(wildcard src/*.c)

CORE_SRC = src/cpu.c

build:
	gcc -Wall $(SRC) -o i8080

TEST_SRC = tests/test_main.c
TEST_BIN = run_tests

test:
	$(CC) $(CFLAGS) $(CORE_SRC) $(TEST_SRC) -o $(TEST_BIN)
	./$(TEST_BIN)

CPM_SRC = tests/cpm_runner.c
CPM_BIN = cpm_runner

cpm:
	$(CC) -Wall -O2 $(CORE_SRC) $(CPM_SRC) -o $(CPM_BIN)

coverage:
	mkdir -p coverage
	gcc -Wall -g --coverage $(CORE_SRC) $(TEST_SRC) -o $(TEST_BIN)
	./$(TEST_BIN)
	gcovr --html-details -o coverage/coverage.html
	rm -f *.gcno *.gcda

.PHONY: build test cpm coverage
