To compile Symbiosis LLVM pass, proceed as follows:

1. Create a folder named "SymbiosisPass" in /path/to/llvm-2.9/lib/Transforms/
2. Change Makefile in /path/to/llvm-2.9/lib/Transforms/Makefile by adding SymbiosisPath to PARALLEL_DIRS (i.e. `PARALLEL_DIRS = Utils Instrumentation Scalar InstCombine IPO Hello SymbiosisPass` )
3. Run `make` in llvm-2.9 home folder
4. Use pass as follows: 
```
opt -load /path/to/llvm-2.9/Release+Asserts/lib/SymbiosisBBPass.so -symbiosisBB file.bc -o file_inst.bc
```