CC=/usr/bin/clang
CFLAGS=-g
LDFLAGS=-lpthread

LLVM_CC=llvm-gcc
LLVM_LLC=/home/symbiosis/work/llvm-2.9/Release+Asserts/bin/llc
LLVM_OPT=/home/symbiosis/work/llvm-2.9/Release+Asserts/bin/opt
LLVM_LINK=/home/symbiosis/work/llvm-2.9/Release+Asserts/bin/llvm-link
SYMB_PASS=/home/symbiosis/work/llvm-2.9/Release+Asserts/lib/SymbiosisBBPass.so
SYMB_RUNTIME=/home/symbiosis/work/symbiosis/SymbiosisRuntime

all: plain

plain:
	$(CC) $(CFLAGS) -o Crasher Crasher.c $(LDFLAGS)

RUN:
	$(LLVM_CC) -emit-llvm -c -g Crasher.c -o CrasherRUN.bc
	$(LLVM_OPT) -load $(SYMB_PASS) -symbiosisBB CrasherRUN.bc -o CrasherRUN_inst.bc
	$(LLVM_LLC) CrasherRUN_inst.bc
	clang++ -L$(SYMB_RUNTIME) CrasherRUN_inst.s -o CrasherRUN_inst -pthread -lbz2 -lsymb

KLEE:
	$(LLVM_CC) -emit-llvm -c -g CrasherKLEE.c -o CrasherKLEE.bc
	$(LLVM_OPT) -load $(SYMB_PASS) -symbiosisBB CrasherKLEE.bc -o CrasherKLEE_inst.bc

clean:
	-rm *.o *.s *.bc *.ll
	-rm CrasherRUN_inst 
