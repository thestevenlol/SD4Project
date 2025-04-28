# C Fuzzer - 2024 Software Development

## C00274246 - Jack Foley

## What is this project?

This project is a coverage-guided fuzzing tool written in the C Programming language. It uses LLVM instrumentation to detect code coverage and implements multiple fuzzing techniques to find bugs and vulnerabilities in C programs. The fuzzer is designed to run on UNIX systems and uses shared memory for efficient coverage tracking.

## Features

- **Multiple Fuzzing Modes**:
  - Random fuzzing mode: Generates purely random inputs
  - Genetic/Grey-box fuzzing mode: Uses evolutionary algorithms to evolve inputs based on coverage feedback
  - File-based fuzzing mode: For targets that consume file inputs rather than stdin inputs

- **Coverage Tracking**:
  - Uses LLVM's sanitizer coverage instrumentation (trace-pc-guard, trace-cmp)
  - Tracks edge coverage via shared memory between the fuzzer and target

- **Mutation Strategies**:
  - Bit flips, byte flips, and arithmetic mutations
  - Havoc mode with multiple stacked mutations
  - Crossover between inputs for genetic evolution

- **Input Corpus Management**:
  - Maintains and evolves a corpus of interesting inputs
  - Saves and minimizes corpus based on coverage efficiency
  - Can restore fuzzing sessions from saved corpus

- **Bug Detection**:
  - Detects and saves crashes with reproduction steps
  - Identifies and tracks timeouts
  - Creates organized crash and timeout directories

## Requirements

Please note: You MUST run this on a UNIX system. It will not work if you do not have a UNIX system (WSL works too!).

### Dependencies:

1. LLVM/Clang (for instrumentation)
```
sudo apt install clang
```

2. LCOV (for coverage visualization)
```
sudo apt install lcov
```

3. Flex (for lexical analysis)
```
sudo apt install flex
```

## Building the Fuzzer

To build the fuzzer:

```
make
```

This will compile the main fuzzer executable and its components.

## Usage

The basic usage of the fuzzer is:

```
./main [options] -i target.c
```

### Command-Line Options:

- `-i FILE` : Specify the target source file to fuzz
- `-r` : Use random fuzzing mode (default is genetic/grey-box)
- `-g` : Use genetic/grey-box fuzzing mode (explicit setting)
- `-f` : Use file-based fuzzing mode for targets that read from files
- `-n NUM` : Set minimum input value range (default: INT_MIN)
- `-x NUM` : Set maximum input value range (default: INT_MAX)

## Fuzzing Modes Explained

### Random Fuzzing Mode

Generates purely random inputs within the specified range without using coverage feedback:

```
./main -r -i target.c
```

### Genetic/Grey-box Fuzzing Mode

Uses evolutionary algorithms and coverage feedback to generate more effective inputs:

```
./main -g -i target.c
```

This is the default and generally most effective mode.

### File-Based Fuzzing Mode

Some targets expect a filename argument and read full file contents rather than integers via stdin. To fuzz such programs, invoke the fuzzer in "file mode" using the `-f` flag:

```
./main -g -f -i target.c
```

This will drive the target by writing each test case to a temporary file and passing its path to the instrumented binary, enabling coverage feedback on file-based inputs.

## Understanding Output

When running the fuzzer, you'll see various outputs:

- **Coverage Information**: Shows how many paths/edges have been discovered
- **Corpus Statistics**: Information about the saved interesting inputs
- **Crash/Timeout Detection**: Alerts when the target crashes or times out

### Output Directories:

- `corpus/`: Contains interesting inputs that discover new coverage
- `crashes/`: Contains inputs that caused the target to crash
- `timeouts/`: Contains inputs that caused the target to time out
- `fuzzing_progress.csv`: CSV file tracking fuzzing progress metrics

## Examples

### Basic Example: Fuzzing Problem10.c

```
./main -i target.c
```

### Random Fuzzing with Custom Range:

```
./main -r -i target.c -n 1 -x 100
```

### File-Based Target Fuzzing:

```
./main -g -f -i target.c
```

## Analyzing Results

After fuzzing completes, you can:

1. Examine crash files to understand what inputs cause problems
2. Analyze the corpus to understand input patterns
3. Check coverage information to see which parts of the code were tested

## Project Structure

- `src/`: Source files implementing the fuzzer
- `headers/`: Header files defining the API
- `problems/`: Example target programs to fuzz
- `main.c`: Main fuzzer entry point
- `coverage_runtime.c`: Runtime support for coverage instrumentation