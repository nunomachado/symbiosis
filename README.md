# Symbiosis Tutorial

#### Consider using [Cortex](https://github.com/nunomachado/cortex-tool), a tool that extends Symbiosis to expose concurrency bugs (that may depend on both the path and the schedule), in addition to finding their root cause.####

Symbiosis is a tool to help developers diagnose concurrency bugs by computing differential schedule projections. For more information check our [PLDI'15 paper](http://www.gsd.inesc-id.pt/~nmachado/papers/pldi15-nmachado.pdf).

## Symbiosis for C/C++

Download VM with everything already set up here: http://www.gsd.inesc-id.pt/~nmachado/software/Symbiosis_Tutorial.html 

### Installation 
* Install **LLVM-2.9**, **STP**, and **uclibc** as described at http://klee.github.io/getting-started/
* Download the Z3 solver version corresponding to your OS from: https://github.com/Z3Prover/z3/wiki
* Download and install Grphviz from: http://www.graphviz.org/Download..php
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
* Compile and run instrumented version: 
```
$ cd Tests/CTests/crasher 
$ make Run
$ export SYMBTRACE=$PWD/crasher.trace
$ export LD_LIBRARY_PATH=/path/to/SymbiosisRuntime
$ ./CrasherRUN_inst
```
This will create an execution path trace with extension **.ok** (in case of a successful execution) or **.fail** (in case of a failing execution). 

* Run symbolic execution with the generated path trace:
```
$ make KLEE
$ /path/to/SymbiosisSE/Release+Asserts/bin/symbiosisse --allow-external-sym-calls --bb-trace=$PWD/crasher.trace.fail CrasherKLEE_inst.bc
```
This will generate the symbolic trace files into folder **klee-last**.

* Run symbiosis solver to find the failing schedule:
```
$ cd /path/to/SymbiosisSolver
$ ./symbiosisSolver --trace-folder=/path/to/Tests/CTests/crasher/klee-last --model=$PWD/tmp/modelCrasher.txt --solution=$PWD/tmp/failCrasher.txt --with-solver=/path/to/z3-folder/bin/z3
```
If the solver yields satisfiable, it will store the failing scheduel into **failCrasher.txt**.

* Run symbiosis solver to find an alternate non-failing schedule and generate a differential schedule projection (DSP):
```
$ ./symbiosisSolver --model=$PWD/tmp/modelCrasher.txt --solution=$PWD/tmp/failCrasher.txt --with-solver=/path/to/z3-folder/bin/z3 --source=/path/to/Tests/CTests/crasher/ --fix-mode
```
If the solver finds a valid alternate schedule, it will output a graphviz file containing the DSP (extension **.gv**) into folder **SymbiosisSolver/tmp/DSP**.
* **(Optional)** Create a **.ps** version of the DSP:
```
$ cd ./tmp/DSP
$ dot -Tps dsp_failCrasher_Alt0.gv -o dsp_failCrasher_Alt0.ps
```
