#
#  ReactOS hcalc.
#
#  Makefile
#
#   Copyright 1998 DJ Delorie <dj@delorie.com>
#   Distributed under the terms of the GNU GPL
#   http://www.delorie.com/store/hcalc/
#
#  Copyright (C) 2002 The ReactOS Project
	
PATH_TO_TOP = ..

TARGET = hcalc

BASE_CFLAGS = # -DGCC -D_WIN32_IE=0x0400

RCFLAGS = # -DGCC -D_WIN32_IE=0x0400


OBJS =	hcalc.o \
        input.o

LIBS = -lgdi32 -luser32 -lkernel32

all:	$(TARGET).exe

$(TARGET).res:	$(TARGET).rc

$(TARGET).exe:	$(OBJS) $(TARGET).coff
	$(CC) -Wl,--subsystem,windows -o $(TARGET).exe $(OBJS) $(TARGET).coff $(LIBS)
	$(NM) --numeric-sort $(TARGET).exe > $(TARGET).sym


# main.h:		resource.h

# about.o:	about.c about.h main.h

# main.o:		main.c main.h framewnd.h

# framewnd.o:	framewnd.c framewnd.h listview.h treeview.h main.h

# listview.o:	listview.c listview.h main.h 

# treeview.o:	treeview.c treeview.h main.h 
 
# debug.o:	debug.c debug.h main.h


clean:
	- $(RM) $(OBJS)
	- $(RM) $(TARGET).exe
	- $(RM) $(TARGET).sym
	- $(RM) $(TARGET).coff

include $(PATH_TO_TOP)/rules.mak
