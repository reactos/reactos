#
#  ReactOS welcome/autorun
#
#  Makefile
#
#  Copyright (C) 2002 The ReactOS Project
	
PATH_TO_TOP = ..

OBJS =	welcome.o

LIBS = -lkernel32 -lgdi32 -luser32

all: welcome.exe autorun.exe


autorun.exe: welcome.o autorun.coff
	$(CC) welcome.o autorun.coff \
		-Wl,--subsystem,windows \
		-lkernel32 -lgdi32 -luser32 -o autorun.exe

welcome.exe: welcome.o welcome.coff
	$(CC) welcome.o welcome.coff \
		-Wl,--subsystem,windows \
		-lkernel32 -lgdi32 -luser32 -o welcome.exe

clean:
	- $(RM) *.o
	- $(RM) *.coff
	- $(RM) *.exe

include $(PATH_TO_TOP)/rules.mak

# EOF
