# Makefile for the StringBuffer bug

CXX=llvm-g++
CXXFLAGS=-g
LDFLAGS=-lpthread

all: main

main: stringbuffer.o main.cpp
	$(CXX) $(CXXFLAGS) -o main main.cpp stringbuffer.o $(LDFLAGS)

stringbuffer.o: stringbuffer.cpp stringbuffer.hpp
	$(CXX) $(CXXFLAGS) -c -o stringbuffer.o stringbuffer.cpp 


RUN1: main.cpp stringbuffer.cpp stringbuffer.hpp
	$(CXX) -emit-llvm -c -g stringbuffer.cpp -o stringbuffer.bc 
	$(CXX) -emit-llvm -c -g main.cpp -o main.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-link -S stringbuffer.bc main.bc -o mainRUN.bc


RUN2:	mainRUN.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-dis mainRUN_inst.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llc mainRUN_inst.bc
	clang++ -L/home/ubuntu/symbiosis/Runtime mainRUN_inst.s -o mainRUN_inst -pthread -lbz2 -lsymb


KLEE: 	 
	llvm-gcc -emit-llvm -c -g stringbufferKLEE.cpp -o stringbufferKLEE.bc
	llvm-gcc -emit-llvm -c -g mainKLEE.cpp -o mainKLEE.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-link -S stringbufferKLEE.bc mainKLEE.bc -o mainKLEE.bc


clean:
	rm -f stringbuffer.o main *.bc


