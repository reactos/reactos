#
#  FreeLoader
#  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

PATH_TO_TOP = ../../..

include ../rules.mak

TOOLS	=	deptool$(EXE_POSTFIX)	\
			bin2c$(EXE_POSTFIX)

all : $(TOOLS)
	@echo freeldr: Tools are up to date.

% :: all

%$(EXE_POSTFIX): %.c
	@echo freeldr: Compiling $*
	@$(HOST_CC) -Wall -O3 -o $@ $<


.PHONY : clean
clean:
	@-$(RM) $(TOOLS)
	@echo freeldr: Clean ALL done.
