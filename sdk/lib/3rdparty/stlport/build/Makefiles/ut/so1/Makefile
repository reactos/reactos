# -*- Makefile -*- Time-stamp: <06/11/10 16:23:01 ptr>

SRCROOT := ../../..
EXTRA_POST := checks

include Makefile.inc
include ${SRCROOT}/Makefiles/gmake/top.mak

INCLUDES += -I./include

checks:
	file ${SO_NAME_OUTxxx} | grep ELF >/dev/null || exit 1
	file ${SO_NAME_OUTxx} | grep link >/dev/null || exit 1
	file ${SO_NAME_OUTx} | grep link >/dev/null || exit 1
	file ${SO_NAME_OUT} | grep link >/dev/null || exit 1

PHONY += checks

