# $Id: Makefile,v 1.2 2000/10/11 20:50:33 dwelch Exp $
#
# ReactOS Operating System
#
TARGET=midl

CFLAGS = 

all: midl$(EXE_POSTFIX)

idl.tab.o: idl.y
	bison -t -v -d idl.y
	$(NATIVE_CC) -c idl.tab.c -o idl.tab.o

lex.yy.o: idl.l idl.tab.h
	flex idl.l
	$(NATIVE_CC) -c lex.yy.c -o lex.yy.o

midl.o: midl.c midl.h idl.tab.h
	$(NATIVE_CC) -c midl.c -o midl.o

option.o: option.c midl.h idl.tab.h
	$(NATIVE_CC) -c option.c -o option.o	

interface.o: interface.c midl.h idl.tab.h
	$(NATIVE_CC) -c interface.c -o interface.o

types.o: types.c midl.h idl.tab.h types.h
	$(NATIVE_CC) -c types.c -o types.o

midl$(EXE_POSTFIX): idl.tab.o lex.yy.o midl.o option.o interface.o types.o
	$(NATIVE_CC) midl.o idl.tab.o lex.yy.o option.o interface.o -lfl \
	types.o -o midl$(EXE_POSTFIX)

include ../../../rules.mak
















