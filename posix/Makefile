# $Id: Makefile,v 1.4 2002/10/01 20:21:44 ea Exp $
#
# ReactOS POSIX+ Personality
#
PATH_TO_TOP=../reactos

CFLAGS=-Iinclude

all: lib/crt0w32.o
	make -C tools
	make -C lib
	make -C server
	make -C lib/psxdll
	make -C lib/psxx
	make -C apps/baresh
	make -C apps/posixw32

lib/crt0w32.o: lib/crt0w32.c

clean:
	make -C lib clean
	make -C tools clean
	make -C server clean
	make -C lib/psxdll clean
	make -C lib/psxx clean
	make -C apps/baresh clean
	make -C apps/posixw32 clean
	- $(RM) lib/crt0w32.o


include $(PATH_TO_TOP)/rules.mak

# EOF
