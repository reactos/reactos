# $Id$

PATH_TO_TOP = ../..

TARGET_TYPE = library

TARGET_NAME = zlib

TARGET_CFLAGS = \
 -O3 -Wall -Wwrite-strings -Wpointer-arith -Wconversion \
 -Wstrict-prototypes -Wmissing-prototypes -Werror

# require os code to explicitly request A/W version of structs/functions
TARGET_CFLAGS += -D_DISABLE_TIDENTS

TARGET_OBJECTS = \
 adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o zutil.o \
 inflate.o infblock.o inftrees.o infcodes.o infutil.o inffast.o
 
DEP_OBJECTS = $(TARGET_OBJECTS)

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

include $(TOOLS_PATH)/depend.mk

# EOF
