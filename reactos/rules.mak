#
# Important
#
.EXPORT_ALL_VARIABLES:

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif

# Build map files which includes source and asm code
# FULL_MAP = yes

# Default to no PCH support
ifeq ($(ROS_USE_PCH),)
ROS_USE_PCH = no
endif

# uncomment if you use bochs and it displays only 30 rows
# BOCHS_30ROWS = yes

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
else
TOPDIR := $(shell cd)
endif

TOPDIR := $(TOPDIR)/$(PATH_TO_TOP)

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
endif


# Set INSTALL_DIR to default value if not already set
# ifeq ($(INSTALL_DIR),)
INSTALL_DIR = $(PATH_TO_TOP)/reactos
# endif

# Set DIST_DIR to default value if not already set
# ifeq ($(DIST_DIR),)
DIST_DIR = $(PATH_TO_TOP)/dist
# endif

# Directory to build a bootable CD image in
BOOTCD_DIR=$(TOPDIR)/../bootcd/disk

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
WRC = $(WINE_TOP)/tools/wrc/wrc
RCINC = --include-dir $(PATH_TO_TOP)/include
OBJCOPY = $(PREFIX)objcopy
OBJDUMP =$(PREFIX)objdump
TOOLS_PATH = $(PATH_TO_TOP)/tools
CP = $(TOOLS_PATH)/rcopy
RM = $(TOOLS_PATH)/rdel
RLINE = $(TOOLS_PATH)/rline
RMDIR = $(TOOLS_PATH)/rrmdir
RMKDIR = $(TOOLS_PATH)/rmkdir
RSYM = $(TOOLS_PATH)/rsym
RTOUCH = $(TOOLS_PATH)/rtouch
MC = $(TOOLS_PATH)/wmc/wmc


# Maybe we can delete these soon

ifeq ($(HOST),mingw32-linux)
CFLAGS := $(CFLAGS) -I$(PATH_TO_TOP)/include -pipe -march=i386 -D_M_IX86
endif

ifeq ($(HOST),mingw32-windows)
CFLAGS := $(CFLAGS) -I$(PATH_TO_TOP)/include -pipe -march=i386 -D_M_IX86
endif

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

# Posix+ Integration
POSIX_PATH=$(PATH_TO_TOP)/../posix
POSIX_PATH_LIB=$(POSIX_PATH)/lib
POSIX_PATH_INC=$(POSIX_PATH)/include

# OS/2 Integration
OS2_PATH=$(PATH_TO_TOP)/../os2
OS2_PATH_LIB=$(OS2_PATH)/lib
OS2_PATH_INC=$(OS2_PATH)/include

# Other systems integration
ROOT_PATH=$(PATH_TO_TOP)/..

COMCTL32_TARGET = comctl23

SHELL32_TARGET = shell23

COMDLG32_TARGET = comdlg23


