#
#  ReactOS winfile
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

TARGET = winfile

BASE_CFLAGS = -DGCC -D_WIN32_IE=0x0400

RCFLAGS = -DGCC -D_WIN32_IE=0x0400


OBJS =	about.o \
        childwnd.o \
        debug.o \
        dialogs.o \
        drivebar.o \
        draw.o \
        entries.o \
        framewnd.o \
        listview.o \
        main.o \
        network.o \
        settings.o \
        shell.o \
        sort.o \
        splitpath.o \
        run.o \
        trace.o \
        treeview.o \
        utils.o \
        worker.o

LIBS = -lgdi32 -luser32 -lkernel32 -ladvapi32 -lcomctl32 -lcomdlg32 -lversion

all:	$(TARGET).exe

$(TARGET).res:	$(TARGET).rc

$(TARGET).exe:	$(OBJS) $(TARGET).coff
	$(CC) -Wl,--subsystem,windows -o $(TARGET).exe $(OBJS) $(TARGET).coff $(LIBS)
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


about.o:	about.c about.h resource.h

main.o:		main.c main.h childwnd.h framewnd.h resource.h

childwnd.o:	childwnd.c childwnd.h resource.h

framewnd.o:	framewnd.c framewnd.h resource.h

debug.o:	debug.c debug.h

font.o:		font.c font.h

run.o:		run.c run.h

setttings.o:	setttings.c setttings.h resource.h

$(TARGET).o:	$(TARGET).c $(TARGET).h resource.h


clean:
	- $(RM) $(OBJS)
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

include $(PATH_TO_TOP)/rules.mak
