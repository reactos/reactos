#
# Important
#
.EXPORT_ALL_VARIABLES:

TOPDIR := $(shell if [ "$$PWD" != "" ]; then echo $$PWD; else pwd; fi)

#
# Choose various options
#
ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i586-mingw32-
EXE_POSTFIX = 
CP = cp
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
NASM_CMD = nasm
KM_SPECS = $(TOPDIR)/specs
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
EXE_POSTFIX = .exe
CP = copy
endif

#
# Create variables for all the compiler tools 
#
ifeq ($(WITH_DEBUGGING),yes)
DEBUGGING_CFLAGS = -g
else
DEBUGGING_CFLAGS = 
endif

DEFINES = -DDBG -DCOMPILER_LARGE_INTEGERS

ifeq ($(WIN32_LEAN_AND_MEAN),yes)
LEAN_AND_MEAN_DEFINE = -DWIN32_LEAN_AND_MEAN
else
LEAN_AND_MEAN_DEFINE = 
endif 

CC = $(PREFIX)gcc
NATIVE_CC = gcc
CFLAGS = -O2 -I../../../include -I../../include  \
         -I../include -fno-builtin $(LEAN_AND_MEAN_DEFINE)  \
         $(DEFINES) -Wall -Wstrict-prototypes $(DEBUGGING_CFLAGS)
CXXFLAGS = $(CFLAGS)
NFLAGS = -i../../include/ -i../include/ -pinternal/asm.inc -f$(NASM_FORMAT) -d$(NASM_FORMAT)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp
CPP = $(PREFIX)cpp
AR = $(PREFIX)ar

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@


RULES_MAK_INCLUDED = 1
