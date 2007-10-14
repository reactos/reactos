# Texture compression
# Version:  1.1
#
# Copyright (C) 2004  Daniel Borca   All Rights Reserved.
#
# this is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# this is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU Make; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.	


#
#  Available options:
#
#    Environment variables:
#
#    Targets:
#	all:		build codec
#	clean:		remove object files
#	realclean:	remove all generated files
#


.PHONY: all clean realclean

EXENAME = codec

CC = gcc
CFLAGS = -Wall -W -pedantic -ansi
CFLAGS += -O2 -ffast-math -funroll-loops
#CFLAGS += -fomit-frame-pointer -fexpensive-optimizations
CFLAGS += -DVERBOSE=1

LD = gcc
LDFLAGS = -s
LDLIBS =

SOURCES = \
	main.c \
	tga.c \
	util.c

SOURCES += \
	../fxt1.c \
	../dxtn.c \
	../wrapper.c \
	../texstore.c

OBJECTS = $(SOURCES:.c=.o)

.c.o:
	$(CC) -o $@ $(CFLAGS) -c $<

all: $(EXENAME)

$(EXENAME): $(OBJECTS)
	$(LD) -o $@ $(LDFLAGS) $^

clean:
	-$(RM) $(OBJECTS)

realclean: clean
	-$(RM) $(EXENAME)
