PATH_TO_TOP = ../..
TARGET_TYPE = dynlink
TARGET_NAME = unbzip2
TARGET_NORC = yes
TARGET_LFLAGS = -nostartfiles -ffreestanding

TARGET_CFLAGS=-Wall -Winline -Os -fomit-frame-pointer -fno-strength-reduce -DBZ_NO_STDIO -DBZ_DECOMPRESS_ONLY $(BIGFILES)

TARGET_OBJECTS = bzlib.o randtable.o crctable.o decompress.o huffman.o dllmain.o

include $(PATH_TO_TOP)/rules.mak
include $(TOOLS_PATH)/helper.mk

test.exe: test.o ../../dk/w32/lib/unbzip2.a
	$(CC) -g -o test.exe test.o ../../dk/w32/lib/unbzip2.a
test.o: test.c
	$(CC) -g -c test.c
