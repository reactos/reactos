# $Id: makefile,v 1.3 1999/07/17 23:10:12 ea Exp $
# ReactOS Operating System
# LPC test
CC=gcc
LD=ld
CFLAGS=-I../../include


all: conport.exe creport.exe simpless.exe

conport.exe: conport.o dumpinfo.o
	$(CC) -o conport conport.o dumpinfo.o

creport.exe: creport.o dumpinfo.o.
	$(CC) -o creport creport.o dumpinfo.o 

simpless.exe: simpless.o dumpinfo.o.
	$(CC) -o simpless simpless.o dumpinfo.o 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


#EOF
