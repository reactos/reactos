#
#  ReactOS Task Manager
#
#  Makefile
#
#  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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

TARGET = taskmgr

BASE_CFLAGS = -DGCC -D_WIN32_IE=0x0400

RCFLAGS = -DGCC -D_WIN32_IE=0x0400


OBJS =	about.o \
	affinity.o \
	applpage.o \
	column.o \
	debug.o \
	endproc.o \
	font.o \
	graph.o \
	optnmenu.o \
	perfdata.o \
	perfpage.o \
	priority.o \
	procpage.o \
	proclist.o \
	run.o \
	trayicon.o \
	taskmgr.o \
	graphctl.o

LIBS = -lgdi32 -luser32 -lkernel32 -lcomctl32

all:	taskmgr.exe

taskmgr.res:	taskmgr.rc

taskmgr.exe:	$(OBJS) taskmgr.coff
	$(CPP) -Wl,--subsystem,windows -o taskmgr.exe $(OBJS) taskmgr.coff $(LIBS)
	$(NM) --numeric-sort taskmgr.exe > taskmgr.sym


about.o:	about.c about.h resource.h

affinity.o:	affinity.c affinity.h

applpage.o:	applpage.c applpage.h procpage.h taskmgr.h resource.h

column.o:	column.c column.h resource.h

debug.o:	debug.c debug.h

endproc.o:	endproc.c endproc.h

font.o:		font.c font.h

graph.o:	graph.c graph.h resource.h

graphctl.o:	graphctl.cpp graphctl.h resource.h

optnmenu.o:	optnmenu.c optnmenu.h resource.h

perfdata.o:	perfdata.c perfdata.h

perfpage.o:	perfpage.cpp perfpage.h perfdata.h graphctl.h graph.h taskmgr.h resource.h

priority.o:	priority.c priority.h

procpage.o:	procpage.c procpage.h perfdata.h column.h proclist.h taskmgr.h resource.h

proclist.o:	proclist.c proclist.h

run.o:		run.c run.h

trayicon.o:	trayicon.c trayicon.h resource.h

taskmgr.o:	taskmgr.c taskmgr.h resource.h

clean:
	- $(RM) $(OBJS)
	- $(RM) taskmgr.exe
	- $(RM) taskmgr.sym
	- $(RM) taskmgr.coff

include $(PATH_TO_TOP)/rules.mak
