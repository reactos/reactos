# $Id: Makefile,v 1.4 2003/01/01 11:11:48 robd Exp $

PATH_TO_TOP = ../../..

TARGET_TYPE = export_driver

TARGET_NAME = unbzip2

TARGET_NORC = yes

TARGET_CFLAGS = \
 -Wall -Winline -Os -fomit-frame-pointer -fno-strength-reduce \
 -DBZ_NO_STDIO -DBZ_DECOMPRESS_ONLY $(BIGFILES) -g

TARGET_OBJECTS = \
	bzlib.o \
	randtable.o \
	crctable.o \
	decompress.o \
	huffman.o \
	dllmain.o

TARGET_GCCLIBS = gcc

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

test.exe: test.o ../../dk/w32/lib/unbzip2.a
	$(CC) -s -Os -o test.exe test.o ../../dk/w32/lib/unbzip2.a

test.o: test.c
	$(CC) -s -Os -c test.c

# EOF
