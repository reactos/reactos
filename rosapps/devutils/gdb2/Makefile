# $Id: Makefile,v 1.1 2001/04/15 23:43:12 narnaoud Exp $
#
#  ReactOS makefile for gdb2
#

include ../../rules.mak

TARGET_NAME=gdb2

all: $(TARGET_NAME)$(EXE_POSTFIX)

gdb2.exe : gdb2.cpp ph.h
	gcc gdb2.cpp -o gdb2.exe
