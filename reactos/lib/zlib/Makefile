# $Id: Makefile,v 1.4 2003/04/14 01:19:08 hyperion Exp $

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = zlib

TARGET_CFLAGS = \
 -MMD -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
 -Wstrict-prototypes -Wmissing-prototypes

TARGET_OBJECTS = \
 adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o zutil.o \
 inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
