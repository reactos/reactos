# $Id: Makefile,v 1.3 2002/10/29 04:45:26 rex Exp $
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
