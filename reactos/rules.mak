#
# Important
#
.EXPORT_ALL_VARIABLES:

ifeq ($(HOST),mingw32-linux)
TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)
endif

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i586-mingw32-
EXE_POSTFIX = 
EXE_PREFIX = ./
CP = cp
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
KM_SPECS = $(TOPDIR)/specs
FLOPPY_DIR = /a
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
EXE_POSTFIX = .exe
CP = copy
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
RM = del
KM_SPECS = specs
DOSCLI = yes
FLOPPY_DIR = A:
# DIST_DIR should be relative from the top of the tree
DIST_DIR = dist
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

CC = $(PREFIX)gcc
NATIVE_CC = gcc
NATIVE_NM = nm
CFLAGS = $(BASE_CFLAGS) \
	-pipe \
	-O2 \
	-Iinclude \
	-fno-builtin $(LEAN_AND_MEAN_DEFINE)  \
	$(DEFINES) -Wall \
	-Wstrict-prototypes $(DEBUGGING_CFLAGS) \
	$(EXTRA_CFLAGS)
CXXFLAGS = $(CFLAGS)
NFLAGS = -i../../include/ -i../include/ -pinternal/asm.inc -f$(NASM_FORMAT) -d$(NASM_FORMAT)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar
RC = $(PREFIX)windres
RCINC = --include-dir ../include --include-dir ../../include --include-dir ../../../include

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@
%.coff: %.rc
	$(RC) $(RCINC) $< $@


RULES_MAK_INCLUDED = 1
