CC=/home/ubuntu/clang+llvm-3.4/bin/clang
OBJS = pbzip2.o cvector.o
<<<<<<< HEAD
OBJSA = pbzip2Aviso.o cvectorAviso.o

CFLAGS= -Wno-strict-aliasing -Dassert=IR_Assert -DAVISO_Synthetic_Event=IR_SyntheticEvent -Dpthread_exit=IR_ThreadExit -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -Dpthread_mutex_init=IR_LockInit -g -O0 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer -finstrument-functions

LDFLAGS= -pthread -L${AVISOHOME}/Runtime -lIRPTR 
LIBS= -L${AVISOHOME}/Runtime -lpthread -Dassert=IR_Assert -L${AVISOHOME}/Runtime -lIRPTR -fno-omit-frame-pointer
LOADPASS= -Xclang -load -Xclang /home/ubuntu/Aviso/Compiler/Aviso/Aviso.so

all: pbzip2Aviso

#Aviso compilation
pbzip2Aviso.o: pbzip2.c
	$(CC) $(CFLAGS) $(LOADPASS) -g -c -o $@ $<  

cvectorAviso.o: cvector.c
	$(CC) $(CFLAGS) $(LOADPASS) -g -c -o $@ $< 

pbzip2Aviso: $(OBJSA)
	$(CC) $(OBJSA) -o pbzip2Aviso -pthread -lbz2 $(LDFLAGS) $(LIBS)

# Default compilation
pbzip2.o: pbzip2.c
	$(CC) -c -o $@ $< -g

cvector.o: cvector.c
	$(CC) -c -o $@ $< -g

plain:	$(OBJS)
	$(CC) -g $(OBJS) -o pbzip2 -pthread -lbz2

=======

CFLAGS= -Wno-strict-aliasing -Dassert=IR_Assert -DAVISO_Synthetic_Event=IR_SyntheticEvent -Dpthread_exit=IR_ThreadExit -Dpthread_mutex_lock=IR_Lock -Dpthread_mutex_unlock=IR_Unlock -Dpthread_mutex_init=IR_LockInit -g -O0 -funroll-loops -fprefetch-loop-arrays -fno-omit-frame-pointer -finstrument-functions

DUMPER=../../scripts/dump-with-stats.sh

LDFLAGS= -pthread -L${AVISOHOME}/Runtime -lIRPTR 
LIBS= -L${AVISOHOME}/Runtime -lpthread -Dassert=IR_Assert -L${AVISOHOME}/Runtime -lIRPTR -fno-omit-frame-pointer
LOADPASS= -Xclang -load -Xclang /home/ubuntu/Aviso/Compiler/Aviso/Aviso.so

.PHONY: cord

all: pbzip2

ciltrees:
	$(DUMPER)

pbzip2.o: pbzip2.c
	$(CC) $(CFLAGS) $(LOADPASS) -g -c -o $@ $<  

cvector.o: cvector.c
	$(CC) $(CFLAGS) $(LOADPASS) -g -c -o $@ $< 

# Default compilation
plain:	$(OBJS)
	$(CC) $(OBJS) -o pbzip2 -pthread -lbz2 $(LDFLAGS) $(LIBS)

#Aviso compilation
pbzip2: $(OBJS)
	$(CC) $(LOADPASS) $(OBJS) -o pbzip2Aviso -pthread -lbz2 $(LDFLAGS) $(LIBS)
>>>>>>> 4091b55444c5678c7a1239d92152fde801288247

clean:
	rm -f *~ *\# *.bc *.s *.ll *.o pbzip2 pbzip2Aviso runtime pbzip2-inst pbzip2-uninst pbzip2-indices.txt
