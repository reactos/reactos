# $Id: Makefile,v 1.1 2002/10/01 20:21:45 ea Exp $
# posix/lib/Makefile
#
PATH_TO_TOP=../../reactos

PATH_TO_PSX_TOP = ../..

all:
	make -C psxdll psxdll.a
	make -C psxx   psxx.a

clean:
	- $(RM) *.a

include $(PATH_TO_TOP)/rules.mak

# EOF
