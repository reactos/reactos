#
#  ReactOS regedt32
#
#  Makefile
#
#  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
	
PATH_TO_TOP = ..

TARGET = regedt32

BASE_CFLAGS = -DGCC -D_WIN32_IE=0x0400

RCFLAGS = -DGCC -D_WIN32_IE=0x0400


OBJS =	about.o \
	debug.o \
	$(TARGET).o

LIBS = -lgdi32 -luser32 -lkernel32 -lcomctl32

all:	$(TARGET).exe

$(TARGET).res:	$(TARGET).rc

$(TARGET).exe:	$(OBJS) $(TARGET).coff
	$(CC) -Wl,--subsystem,windows -o $(TARGET).exe $(OBJS) $(TARGET).coff $(LIBS)
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


about.o:	about.cpp about.h resource.h

affinity.o:	affinity.cpp affinity.h

applicationpage.o:	applicationpage.cpp applicationpage.h processpage.h $(TARGET).h resource.h

column.o:	column.cpp column.h resource.h

debug.o:	debug.cpp debug.h

endproc.o:	endproc.cpp endproc.h

font.o:		font.cpp font.h

graph.o:	graph.cpp graph.h resource.h

graphctrl.o:	graphctrl.cpp graphctrl.h resource.h

optnmenu.o:	optnmenu.cpp optnmenu.h resource.h

perfdata.o:	perfdata.cpp perfdata.h

performancepage.o:	performancepage.cpp performancepage.h perfdata.h graphctrl.h graph.h $(TARGET).h resource.h

priority.o:	priority.cpp priority.h

processpage.o:	processpage.cpp processpage.h perfdata.h column.h proclist.h $(TARGET).h resource.h

proclist.o:	proclist.cpp proclist.h

run.o:		run.cpp run.h

trayicon.o:	trayicon.cpp trayicon.h resource.h

$(TARGET).o:	$(TARGET).cpp $(TARGET).h resource.h

clean:
	- $(RM) $(OBJS)
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

include ../rules.mak
