#
# MinGW Makefile for expat
#
# Martin Fuchs, 24.03.2003
#

PATH_TO_TOP = ../..
TOOLS_PATH = $(PATH_TO_TOP)/tools

CC = gcc
LINK = gcc

CFLAGS	= -O2 -Wall -Wmissing-prototypes -Wstrict-prototypes -fexceptions -DWIN32 -DHAVE_EXPAT_CONFIG_H
LFLAGS	= -shared

ifdef DEBUG
CFLAGS	+= -g
LFLAGS	+= -g
else
CFLAGS	+= -Os
LFLAGS	+= -s
endif

VPATH = lib

DLL_SUFIX = .dll

LIBRARY = libexpat$(DLL_SUFIX)

LIB_OBJS = \
	xmlparse.o \
	xmlrole.o \
	xmltok.o

all: $(LIBRARY)

$(LIBRARY): $(LIB_OBJS) libexpat.def
	$(LINK) $(LFLAGS) -o $@ $^

clean:
	$(TOOLS_PATH)/rdel $(LIBRARY) $(LIB_OBJS)

implib:

install:
	-$(TOOLS_PATH)/rcopy $(LIBRARY) $(INSTALL_DIR)/system32/$(LIBRARY)

bootcd:
