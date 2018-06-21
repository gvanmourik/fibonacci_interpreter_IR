# Fibonacci LLVM IR Solver

Two different methods by which the Nth Fibonacci number can be found. One is recursive and the other iterative. Both use the LLVM library to generate the IR, which is then run with lli.  

## Getting Started

Follow these instructions to use and experiment with the Fibonacci IR code.

### Prerequisites

LLVM is required in order to run this code. The code was written and tested on the following:

```
user$ llvm-g++ --version
Apple LLVM version 9.0.0 (clang-900.0.39.2)
Target: x86_64-apple-darwin16.7.0
Thread model: posix
InstalledDir: /Library/Developer/CommandLineTools/usr/bin
```
Where Apple LLVM version 9.0.0 uses LLVM 4.0.

### Installing

To clone the repo:

```
git clone https://github.com/gvanmourik/fibonacci_interpreter_IR
```

## Running the code

To run the two Fibonacci solvers with the provided bash script from the fibonacci_interpreter_IR home directory, perform the following steps.

### Recursive

For the recursive method (where N is some integer value):

```
./lli_compile_and_run fibonacci_recursive N
```

### Iterative

For the iterative method (where N is some integer value):

```
./lli_compile_and_run fibonacci_loop N
```

### Viewing the control flow graphs

To generate and view the cfg's for each method run the following (this will take a few seconds). Note that these commands must be run following the above compile and run commands, and that DestDirName/ will be placed in the home directory of fibonacci_interpreter_IR.

Recursive:
```
./cfg_move_convert_to_png fibonacci_recursive.ll DestDirName
```

Iterative:
```
./cfg_move_convert_to_png fibonacci_loop.ll DestDirName
```

## Built With

* [LLVM 4.0](https://releases.llvm.org/4.0.1/docs/ReleaseNotes.html) - The compiler infrastructure used

## Acknowledgments

* LLVM Team

