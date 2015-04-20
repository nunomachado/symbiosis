# Symbiosis Tutorial
Symbiosis - Concurrency Debugging via Differential Schedule Projections

## Symbiosis C

### Installation 
* Install LLVM-2.9, STP, and uclibc as described at http://klee.github.io/getting-started/
* Build Symbiosis LLVM Pass as indicated by the instructions in folder **Instrumentation**.
* Build SymbiosisRuntime:
```
$ cd SymbiosisRuntime
$ make
```
* Build Symbiosis Symbolic Execution Engine:
```
$ cd SymbiosisSE
$ make
```
* Build Symbiosis Symbolic Solver:
```
$ cd SymbiosisSolver
$ make
```

### Example: Crasher
* Compile and run instrumented version 
```
$ cd Tests/CTests/crasher 
$ make Run
$ export SYMBTRACE=$PWD/crasher.trace
$ export LD_LIBRARY_PATH=/path/to/SymbiosisRuntime
$ ./CrasherRUN_inst
```
This will create a trace file with extension either **.ok** (in case of a successful execution) or **.fail** (in case of a failing execution). 