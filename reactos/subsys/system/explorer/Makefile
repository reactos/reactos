#
#  ReactOS explorer
#
#  Makefile
#

ifeq ($(HOST),mingw32-linux)
CC = mingw32-gcc
CXX = mingw32-g++
LINK = mingw32-g++
RC = mingw32-windres
CD_PREFIX := ./
else
CC = gcc
CXX = g++
LINK = g++
RC = windres
CD_PREFIX := 
endif

ifndef UNICODE
UNICODE = 1
endif

TOOLS_PATH = ../../../tools

CFLAGS	= -DWIN32 -D_ROS_ -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501 -fexceptions -Wall
RCFLAGS	= -DWIN32 -D_ROS_
LFLAGS	= -Wl,--subsystem,windows

ifdef DEBUG
CFLAGS	+= -D_DEBUG -g
RCFLAGS	+= -D_DEBUG
LFLAGS	+= -g
else
CFLAGS	+= -DNDEBUG -Os
RCFLAGS	+= -DNDEBUG
LFLAGS	+= -s
endif

ifeq ($(ifdef UNICODE),1)
CFLAGS	+= -DUNICODE
# LFLAGS+= -Wl,--entry,_wWinMain@16
endif

CXXFLAGS = $(CFLAGS)

EXEC_SUFFIX = .exe
RES_SUFFIX = .coff

VPATH = shell utility taskbar desktop dialogs

PROGRAM = explorer

TARGET = $(PROGRAM)$(EXEC_SUFFIX)

OBJECTS = \
	startup.o \
	shellclasses.o \
	utility.o \
	window.o \
	dragdropimpl.o \
	shellbrowserimpl.o \
	explorer.o \
	entries.o \
	winfs.o \
	unixfs.o \
	shellfs.o \
	mainframe.o \
	filechild.o \
	pane.o \
	shellbrowser.o \
	desktop.o \
	desktopbar.o \
	taskbar.o \
	startmenu.o \
	traynotify.o \
	quicklaunch.o \
	searchprogram.o \
	i386-stub-win32.o

LIBS = gdi32 comctl32 ole32 uuid wsock32

all: buildno.h $(TARGET)

docu: doxy-footer.html
	doxygen Doxyfile

full-docu: docu
	-cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
	cmd /c move /y doxy-doc\html\index.chm ros-explorer.chm
	doxygen Doxyfile-all
	-cmd /c start /b /low /wait hhc doxy-doc\html\index.hhp
	cmd /c move /y doxy-doc\html\index.chm ros-explorer-full.chm

doxy-footer.html: ever
	updatefooter

ever:

buildno.h: ../../../include/reactos/buildno.h
	$(CD_PREFIX)updatebuildno

$(TARGET): $(OBJECTS) $(PROGRAM)$(RES_SUFFIX)
	$(LINK) $(LFLAGS) -o $@ $^ $(addprefix -l,$(LIBS))

explorer$(RES_SUFFIX): $(PROGRAM)_intres.rc
	$(RC) $(RCFLAGS) -o $@ $^

clean:
	$(TOOLS_PATH)/rdel $(TARGET) $(OBJECTS) $(PROGRAM)$(RES_SUFFIX)

# Use environment var ROS_INSTALL to override default install dir
ifeq ($(ROS_INSTALL),)
ifeq ($(HOST),mingw32-windows)
INSTALL_DIR = C:/reactos
else
INSTALL_DIR = $(PATH_TO_TOP)/reactos
endif
else
INSTALL_DIR = $(ROS_INSTALL)
endif

implib:

install:
	-$(TOOLS_PATH)/rcopy $(TARGET) $(INSTALL_DIR)/$(TARGET)

bootcd:
