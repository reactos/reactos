#
#  ReactOS Task Manager
#
#  Makefile
#
#  Copyright (C) 1999 - 2001  Brian Palmer  <brianp@reactos.org>
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


#export CPP = g++
#export LD = ld
#export AR = ar
#export RM = cmd /C del
#export CP = cmd /C copy

FLAGS = -D_WIN32_IE=0x0400 -g

OBJS = about.o affinity.o applpage.o column.o debug.o endproc.o font.o \
		graph.o optnmenu.o perfdata.o perfpage.o priority.o procpage.o \
		proclist.o run.o trayicon.o taskmgr.o
LIBS = -lgdi32 -luser32 -lkernel32 -lcomctl32

all:	taskmgr.exe

taskmgr.exe:	$(OBJS)
	$(CPP) -g -o taskmgr.exe $(OBJS) $(LIBS)

about.o:	about.c about.h
	$(CPP) $(FLAGS) -o about.o -c about.c

affinity.o:	affinity.c affinity.h
	$(CPP) $(FLAGS) -o affinity.o -c affinity.c

applpage.o:	applpage.c applpage.h
	$(CPP) $(FLAGS) -o applpage.o -c applpage.c

column.o:	column.c column.h
	$(CPP) $(FLAGS) -o column.o -c column.c

debug.o:	debug.c debug.h
	$(CPP) $(FLAGS) -o debug.o -c debug.c

endproc.o:	endproc.c endproc.h
	$(CPP) $(FLAGS) -o endproc.o -c endproc.c

font.o:	font.c font.h
	$(CPP) $(FLAGS) -o font.o -c font.c

graph.o:	graph.c graph.h
	$(CPP) $(FLAGS) -o graph.o -c graph.c

optnmenu.o:	optnmenu.c optnmenu.h
	$(CPP) $(FLAGS) -o optnmenu.o -c optnmenu.c

perfdata.o:	perfdata.c perfdata.h
	$(CPP) $(FLAGS) -o perfdata.o -c perfdata.c

perfpage.o:	perfpage.cpp perfpage.h
	$(CPP) $(FLAGS) -o perfpage.o -c perfpage.cpp

priority.o:	priority.c priority.h
	$(CPP) $(FLAGS) -o priority.o -c priority.c

procpage.o:	procpage.c procpage.h
	$(CPP) $(FLAGS) -o procpage.o -c procpage.c

proclist.o:	proclist.c proclist.h
	$(CPP) $(FLAGS) -o proclist.o -c proclist.c

run.o:	run.c run.h
	$(CPP) $(FLAGS) -o run.o -c run.c

trayicon.o:	trayicon.c trayicon.h
	$(CPP) $(FLAGS) -o trayicon.o -c trayicon.c

taskmgr.o:	taskmgr.c taskmgr.h
	$(CPP) $(FLAGS) -o taskmgr.o -c taskmgr.c

clean:
	- $(RM) $(OBJS)
	- $(RM) taskmgr.exe
