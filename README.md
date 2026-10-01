# intel 8080 emulator

https://github.com/user-attachments/assets/4a30852f-9921-408b-91de-8f98d071809a

## Requirements

To build and test the project, you will need:
* A C compiler (e.g., `gcc` or `clang`)
* `make`
* `gcovr` (optional, for test coverage reports)

## Building and Running

To compile the main program:
```
make build
```

To run the emulator:
```
./i8080 <path_to_rom>
```

To run unittests:
```
make test
```

To generate a code coverage report:
```
make coverage
```
The HTML report will be generated in the coverage/ directory.

## CPU test ROMs

| ROM | Result |
|---|---|
| `TST8080.COM` | CPU IS OPERATIONAL |
| `8080PRE.COM` | 8080 Preliminary tests complete |
| `CPUTEST.COM` | CPU TESTS OK |
| `8080EXM.COM` | all 25 instruction groups PASS |

To build the test runner:
```
make cpm
```

To run a test ROM:
```
./cpm_runner roms/TST8080.COM
```
