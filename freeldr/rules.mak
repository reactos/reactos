.EXPORT_ALL_VARIABLES:

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = mingw32-
EXE_POSTFIX :=
EXE_PREFIX := ./
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
DOSCLI =
FLOPPY_DIR = /mnt/floppy
SEP := /
PIPE :=
RM			= rm -f
CP			= cp -f 
MKDIR		= mkdir
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX =
EXE_PREFIX :=
EXE_POSTFIX := .exe
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasmw
DOSCLI = yes
FLOPPY_DIR = A:
SEP := \$(EMPTY_VAR)
PIPE := -pipe
RM			= cmd /C del
CP			= copy /Y 
MKDIR		= md
endif


NFLAGS = -fwin32 -dwin32
BIN2C		= ..$(SEP)tools$(SEP)bin2c
TOOLSDIR	= ..$(SEP)tools

CC			= $(PREFIX)gcc
LD			= $(PREFIX)ld
AR			= $(PREFIX)ar
NM			= $(PREFIX)nm
WINDRES  		= $(PREFIX)windres
