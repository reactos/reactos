#
# Important
#
.EXPORT_ALL_VARIABLES:


#
# Choose various options
#
ifeq ($(HOST),djgpp-linux)
NASM_FORMAT = coff
PREFIX = dos-
KERNEL_BFD_TARGET = coff-i386
EXE_POSTFIX =
CP = cp
endif

ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i386-mingw32
KERNEL_BFD_TARGET = pe-i386
EXE_POSTFIX
CP = cp
endif

ifeq ($(HOST),djgpp-msdos)
NASM_FORMAT = coff
PREFIX = 
KERNEL_BFD_TARGET = coff-go32
EXE_POSTFIX = .exe
CP = copy
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
KERNEL_BFD_TARGET = pe-i386
EXE_POSTFIX = .exe
CP = copy
endif

#
# Create variables for all the compiler tools 
#
DEFINES = -DCHECKED_BUILD -DWIN32_LEAN_AND_MEAN -DDBG
CC = $(PREFIX)gcc
NATIVE_CC = gcc
CFLAGS = -O2 -I../../include -I../include -fno-builtin $(DEFINES) -Wall -Wstrict-prototypes
CXXFLAGS = $(CFLAGS)
NASM = nasm
NFLAGS = -i../include/ -f$(NASM_FORMAT)
LD = $(PREFIX)ld
NM = $(PREFIX)nm
OBJCOPY = $(PREFIX)objcopy
STRIP = $(PREFIX)strip
AS = $(PREFIX)gcc -c -x assembler-with-cpp
CPP = $(PREFIX)cpp

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM) $(NFLAGS) $< -o $@


RULES_MAK_INCLUDED = 1
