# $Id: Makefile,v 1.1 2002/04/10 21:30:20 ea Exp $
#
# ReactOS POSIX+ Personality
#
PATH_TO_TOP=../..

CFLAGS=-Iinclude

all: lib/crt0w32.o
	make -C tools
	make -C server
	make -C lib/psxdll
	make -C lib/psxx
	make -C apps/baresh
	make -C apps/csrterm

lib/crt0w32.o: lib/crt0w32.c

clean:
	make -C tools clean
	make -C server clean
	make -C lib/psxdll clean
	make -C lib/psxx clean
	make -C apps/baresh clean
	make -C apps/csrterm clean
	- $(RM) lib/crt0w32.o

include $(PATH_TO_TOP)/rules.mak

# EOF
