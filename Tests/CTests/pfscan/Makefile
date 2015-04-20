# Makefile for pfscan

DESTDIR=/usr/local

BINDIR=$(DESTDIR)/bin
MANDIR=$(DESTDIR)/man
MAN1DIR=$(MANDIR)/man1

TAR=tar
GZIP=gzip
MAKE=make
INSTALL=./install-sh


## Solaris 8 with Gcc 3.0
GSO_CC=gcc -Wall -g -O -pthreads
GSO_LDOPTS=
GSO_LIBS= -lnsl -lsocket

## Solaris 8 with Forte C 6.2
SOL_CC=cc -mt -O
SOL_LDOPTS=
SOL_LIBS= -lpthread -lnsl -lsocket

## Linux 2.4 with Gcc 2.96
LNX_CC=gcc -Wall -g -O
LNX_LDOPTS=-Wl,-s 
LNX_LIBS=-lpthread -lnsl


OBJS = pfscan.o bm.o version.o pqueue.o


default:
	@echo 'Use "make SYSTEM" where SYSTEM may be:'
	@echo '   lnx      (Linux with GCC)'
	@echo '   gso      (Solaris with GCC v3)'
	@echo '   sol      (Solaris with Forte C)'
	@exit 1

lnx linux:
	@$(MAKE) all CC="$(LNX_CC)" LIBS="$(LNX_LIBS)" LDOPTS="$(LNX_LDOPTS)"

gso:
	@$(MAKE) all CC="$(GSO_CC)" LIBS="$(GSO_LIBS)" LDOPTS="$(GSO_LDOPTS)"

sol solaris:
	@$(MAKE) all CC="$(SOL_CC)" LIBS="$(SOL_LIBS)" LDOPTS="$(SOL_LDOPTS)"

sol64:
	@$(MAKE) all CC="$(SOL_CC) -xarch=v9" LIBS="$(SOL_LIBS)" LDOPTS="$(SOL_LDOPTS)"


all: pfscan

man: pfscan.1

pfscan.1:	pfscan.sgml
	docbook2man pfscan.sgml

pfscan: $(OBJS)
	$(CC) $(LDOPTS) -o pfscan $(OBJS) $(LIBS)


RUN1: 
	llvm-gcc -emit-llvm -c -g bm.c -o bm.bc
	llvm-gcc -emit-llvm -c -g version.c -o version.bc
	llvm-gcc -emit-llvm -c -g pqueue.c -o pqueue.bc
	llvm-gcc -emit-llvm -c -g pfscan.c -o pfscan.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-link -S bm.bc version.bc pqueue.bc pfscan.bc -o pfscanRUN.bc

RUN2:	pfscanRUN.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-dis pfscanRUN_inst.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llc pfscanRUN_inst.bc
	clang++ -L/home/ubuntu/symbiosis/Runtime pfscanRUN_inst.s -o pfscanRUN_inst -pthread -lbz2 -lsymb


KLEE: 	version.bc pqueue.bc 
	llvm-gcc -emit-llvm -c -g bmKLEE.c -o bmKLEE.bc
	llvm-gcc -emit-llvm -c -g pfscanKLEE.c -o pfscanKLEE.bc
	/home/ubuntu/CLAP/llvm-2.9/Release+Asserts/bin/llvm-link -S bmKLEE.bc version.bc pqueue.bc pfscanKLEE.bc -o pfscanKLEE.bc

version:
	(PACKNAME=`basename \`pwd\`` ; echo 'char version[] = "'`echo $$PACKNAME | cut -d- -f2`'";' >version.c)

clean distclean:
	-rm -f *.o *~ pfscan core manpage.* \#*

dist:	distclean version
	(PACKNAME=`basename \`pwd\`` ; cd .. ; $(TAR) cf - $$PACKNAME | $(GZIP) -9 >$$PACKNAME.tar.gz)


install: all
	$(INSTALL) -c -m 755 pfscan $(BINDIR)

install-man: man
	$(INSTALL) -c -m 644 pfscan.1 $(MAN1DIR)


install-all install-distribution: install install-man
