#
# Important
#
.EXPORT_ALL_VARIABLES:


#
# Choose various options
#
ifeq ($(HOST),elf-linux)
NASM_FORMAT = elf
PREFIX = 
KERNEL_BFD_TARGET = --oformat=elf32-i386
EXE_POSTFIX = 
CP = cp
endif

ifeq ($(HOST),djgpp-linux)
NASM_FORMAT = coff
PREFIX = dos-
KERNEL_BFD_TARGET = --oformat=coff-i386
EXE_POSTFIX =
CP = cp
LIBGCC = ./libgcc.a
endif

ifeq ($(HOST),mingw32-linux)
NASM_FORMAT = win32
PREFIX = i386-mingw32-
#KERNEL_BFD_TARGET = pe-i386
KERNEL_BFD_TARGET = 
EXE_POSTFIX = 
CP = cp
DLLTOOL = $(PREFIX)dlltool --as=$(PREFIX)as
LIBGCC = /usr/lib/gcc-lib/i386-mingw32/2.8.1/libgcc.a
NASM_CMD = nasm
endif

ifeq ($(HOST),djgpp-msdos)
NASM_FORMAT = coff
PREFIX = 
KERNEL_BFD_TARGET = --oformat=coff-go32
EXE_POSTFIX = .exe
CP = COPY
RM = DELETE
LIBGCC = libgcc.a
NASM_CMD = nasm
endif

ifeq ($(HOST),mingw32-windows)
NASM_FORMAT = win32
PREFIX = 
KERNEL_BFD_TARGET = --oformat=pe-i386
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

DEFINES = -DDBG -DCHECKED -DCOMPILER_LARGE_INTEGERS

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

%.o: %.cc
	$(CC) $(CFLAGS) -c $< -o $@
%.o: %.asm
	$(NASM_CMD) $(NFLAGS) $< -o $@


RULES_MAK_INCLUDED = 1
