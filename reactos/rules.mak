#
# Important
#
.EXPORT_ALL_VARIABLES:

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif

# uncomment if you use bochs and it displays only 30 rows
# BOCHS_30ROWS = yes

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
#PREFIX = i586-mingw32-
PREFIX = /usr/mingw32-2.95.3-fc/bin/mingw32-pc-
EXE_POSTFIX := 
EXE_PREFIX := ./
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
DOSCLI =
FLOPPY_DIR = /mnt/floppy
SEP := /
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
endif


# Set INSTALL_DIR to default value if not already set
# ifeq ($(INSTALL_DIR),)
INSTALL_DIR = $(PATH_TO_TOP)/reactos
# endif

# Set DIST_DIR to default value if not already set
# ifeq ($(DIST_DIR),)
DIST_DIR = $(PATH_TO_TOP)/dist
# endif


CC = $(PREFIX)gcc
CXX = $(PREFIX)g++
HOST_CC = gcc
HOST_NM = nm
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp 
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres
RCINC = --include-dir $(PATH_TO_TOP)/include
OBJCOPY = $(PREFIX)objcopy
TOOLS_PATH = $(PATH_TO_TOP)/tools
CP = $(TOOLS_PATH)/rcopy
RM = $(TOOLS_PATH)/rdel
RMDIR = $(TOOLS_PATH)/rrmdir
RMKDIR = $(TOOLS_PATH)/rmkdir
MC = $(TOOLS_PATH)/wmc/wmc


# Maybe we can delete these soon
CFLAGS := $(CFLAGS) -I$(PATH_TO_TOP)/include -pipe -m386
CXXFLAGS = $(CFLAGS)
NFLAGS = -i$(PATH_TO_TOP)/include/ -f$(NASM_FORMAT) -d$(NASM_FORMAT)
ASFLAGS := $(ASFLAGS) -I$(PATH_TO_TOP)/include -D__ASM__


# Developer Kits
DK_PATH=$(PATH_TO_TOP)/dk
# Native and kernel mode
DDK_PATH=$(DK_PATH)/nkm
DDK_PATH_LIB=$(DDK_PATH)/lib
DDK_PATH_INC=$(PATH_TO_TOP)/include
# Win32
SDK_PATH=$(DK_PATH)/w32
SDK_PATH_LIB=$(SDK_PATH)/lib
SDK_PATH_INC=$(PATH_TO_TOP)/include
# POSIX+
XDK_PATH=$(DK_PATH)/psx
XDK_PATH_LIB=$(XDK_PATH)/lib
XDK_PATH_INC=$(XDK_PATH)/include

# Wine Integration
WINE_PATH=$(PATH_TO_TOP)/../wine
WINE_PATH_LIB=$(WINE_PATH)/lib
WINE_PATH_INC=$(WINE_PATH)/include
