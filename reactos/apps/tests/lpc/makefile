# $Id: makefile,v 1.2 1999/07/04 22:04:31 ea Exp $
# ReactOS Operating System
# LPC test
CC=gcc
LD=ld
CFLAGS=-I../../include


all: conport.exe creport.exe

conport.exe: conport.o dumpinfo.o
	$(CC) -o conport conport.o dumpinfo.o

creport.exe: creport.o dumpinfo.o.
	$(CC) -o creport creport.o dumpinfo.o 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


#EOF
