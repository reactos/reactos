# $Id: Makefile,v 1.1 2003/03/24 01:28:35 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = zlib

TARGET_NORC = yes

TARGET_CFLAGS = \
 -MMD -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
 -Wstrict-prototypes -Wmissing-prototypes

TARGET_OBJECTS = \
 adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o zutil.o \
 inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o
 
include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

zlib.sym:
	@echo FIXME!>$@

# FIXME: this rule should be defined in helper.mk
$(TARGET_NAME).nostrip.a:
	@echo FIXME!
	@echo FIXME!>$@

# EOF
