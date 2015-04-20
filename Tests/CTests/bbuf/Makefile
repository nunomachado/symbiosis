CC=/usr/bin/clang
CFLAGS=-g
LDFLAGS=-lpthread

LLVM_CC=llvm-gcc
LLVM_LLC=/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llc
LLVM_OPT=/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/opt
LLVM_LINK=/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-link
SYMB_PASS=/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/lib/SymbiosisBBPass.so
SYMB_RUNTIME=/home/ubuntu/symbiosis/Runtime/


all: bbuf

bbuf:
	clang boundedBuffer.c -c -pthread -o bbuf.o
	clang ./bbuf.o -pthread -g -o bbuf


RUN: 
	$(LLVM_CC) -emit-llvm -c -g boundedBuffer.c -o bbufRUN.bc
	$(LLVM_OPT) -load $(SYMB_PASS) -symbiosisBB bbufRUN.bc -o bbufRUN_inst.bc
	$(LLVM_LLC) bbufRUN_inst.bc
	clang++ -L$(SYMB_RUNTIME) bbufRUN_inst.s -o bbufRUN_inst -pthread -lbz2 -lsymb

KLEE:
	$(LLVM_CC) -emit-llvm -c -g boundedBufferKLEE.c -o bbufKLEE.bc
	$(LLVM_OPT) -load $(SYMB_PASS) -symbiosisBB bbufKLEE.bc -o bbufKLEE_inst.bc

clean:
	-rm *.o *.s *.bc *.ll
	-rm bbufRUN_inst bbuf_KLEE_inst bbuf 
