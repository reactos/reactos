#
# Important
#
.EXPORT_ALL_VARIABLES:

#
# Select your host
#HOST = mingw32-windows
#HOST = mingw32-linux
#

# Windows is default host environment
ifeq ($(HOST),)
HOST = mingw32-windows
endif


ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
PREFIX = mingw32-
EXE_POSTFIX = 
CP = cp
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
RM = rm
DOSCLI = no
FLOPPY_DIR = /mnt/floppy
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
SEP = /
endif


ifeq ($(HOST),mingw32-windows)
PREFIX = 
EXE_POSTFIX = .exe
CP = copy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
RM = del
DOSCLI = yes
FLOPPY_DIR = A:
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
SEP = \$($EMPTY_VAR)
endif

#
# Create variables for all the compiler tools 
#
ifeq ($(WITH_DEBUGGING),yes)
DEBUGGING_CFLAGS = -g
else
DEBUGGING_CFLAGS = 
endif

ifeq ($(WARNINGS_ARE_ERRORS),yes)
EXTRA_CFLAGS = -Werror
endif

DEFINES = -DDBG

ifeq ($(WIN32_LEAN_AND_MEAN),yes)
LEAN_AND_MEAN_DEFINE = -DWIN32_LEAN_AND_MEAN
else
LEAN_AND_MEAN_DEFINE = 
endif 

CPP = $(PREFIX)g++
CC = $(PREFIX)gcc
NATIVE_CC = gcc
CFLAGS = \
	$(BASE_CFLAGS) \
	-pipe \
	-O2 \
	-Wall \
	-Wstrict-prototypes \
	-fno-builtin \
	$(LEAN_AND_MEAN_DEFINE) \
	$(DEFINES) \
	$(DEBUGGING_CFLAGS) \
	$(EXTRA_CFLAGS)
CXXFLAGS = $(CFLAGS)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres
RCINC = --include-dir $(PATH_TO_TOP)/../reactos/include
TOOLS_PATH = $(PATH_TO_TOP)/../reactos/tools
RSYM = $(TOOLS_PATH)/rsym

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@
%.o: %.cc
	$(CPP) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(RCFLAGS) $(RCINC) $< $@



RULES_MAK_INCLUDED = 1
