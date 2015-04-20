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
