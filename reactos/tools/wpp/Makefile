# $Id: Makefile,v 1.1 2003/12/14 19:53:39 sedwards Exp $
PATH_TO_TOP = ../..

TARGET = wpp.a

HOST_CFLAGS = -I$(PATH_TO_TOP)/include/wine -I$(PATH_TO_TOP)/include -D__USE_W32API

OBJECTS = \
	lex.yy.o \
	y.tab.o \
	mkstemps.o \
	preproc.o \
	wpp.o

all: $(TARGET)

%.o: %.c
	$(HOST_CC) $(HOST_CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(HOST_AR) -r $(TARGET) $^

clean:
	- $(RM) *.o
	- $(RM) $(TARGET)

.PHONY: all clean

include $(PATH_TO_TOP)/rules.mak
