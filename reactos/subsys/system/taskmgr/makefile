#
#  ReactOS Task Manager
#
#  Makefile
#

PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = taskmgr

TARGET_CFLAGS = -DDBG -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -D__USE_W32API 
	
TARGET_SDKLIBS = \
	kernel32.a \
	user32.a \
	gdi32.a \
	comctl32.a

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
