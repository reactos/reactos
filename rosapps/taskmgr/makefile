#
#  ReactOS Task Manager
#
#  Makefile
#

PATH_TO_TOP = ../../reactos

TARGET_TYPE = program

TARGET_APPTYPE = console

TARGET_NAME = taskmgr

#TARGET_CFLAGS = -DDBG -D_WIN32_IE=0x0400
#TARGET_RCFLAGS = -D_WIN32_IE=0x0400

WINE_MODE = yes

WINE_RC = $(TARGET_NAME)

WINE_INCLUDE = ./

TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	$(COMCTL32_TARGET).a

TARGET_GCCLIBS = stdc++

TARGET_OBJECTS = \
	about.o \
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

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
