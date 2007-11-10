# Texture compression OpenWatcom makefile
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
#	all:		build dynamic module
#	clean:		remove object files
#	realclean:	remove all generated files
#


.PHONY: all clean realclean
.SUFFIXES: .c .obj

DLLNAME = dxtn.dll

CC = wcl386
CFLAGS = -wx -zq
CFLAGS += -ox -6s

LD = wcl386
LDFLAGS = -zq -bd
LDLIBS =

RM = del

SOURCES = \
	fxt1.c \
	dxtn.c \
	wrapper.c \
	texstore.c

OBJECTS = $(SOURCES:.c=.obj)

.c.obj:
	$(CC) -fo=$@ $(CFLAGS) -c $<

all: $(DLLNAME)

$(DLLNAME): $(OBJECTS)
	$(LD) -fe=$@ $(LDFLAGS) $^ $(LDLIBS)

clean:
	$(RM) *.obj
	$(RM) *.err

realclean: clean
	$(RM) $(DLLNAME)

-include depend
