# Makefile for ReactOS Calc
#
# syntax:
#   make <platform> <precision>
#
# parameters:
#   <platform>
#       x68_64=1 : compile for x86_64
#       undefined: use default tools (typically IA_32)
#   <precision>
#       gnu-mp=1 : compile with GNU multi precision library
#       undefined: use standard IEEE precision

# Default tools
TOOL_PREFIX=

ifdef x86-64
TOOL_PREFIX=x86_64-pc-mingw32-
endif

CPP     = $(TOOL_PREFIX)g++.exe
CC      = $(TOOL_PREFIX)gcc.exe
WINDRES = $(TOOL_PREFIX)windres.exe

# Define some variables
INCLUDE  = 
DEBUG    = -Wall
OPTIMIZE = -O2 -fomit-frame-pointer

ifdef x86-64
# nothing here
else
OPTIMIZE+= -mpreferred-stack-boundary=2
endif

# Strip, typical mingw flag, help library
LIBS     = -s -mwindows -lhtmlhelp

# Check if precision is specified
ifndef gnu-mp
gnu_mp=1
endif

# Check the precision flags
ifeq ($(gnu-mp), 1)
PRECISION=gmp
DEFS  += -DENABLE_MULTI_PRECISION
FILES  = rpn_mpfr about fun_mpfr utl_mpfr winmain
LIBS  += -lgmp -lmpfr
else
PRECISION=ieee
DEFS  +=
FILES  = rpn about function utl winmain
LIBS  += -lm
endif

CFLAGS   = $(DEFS) $(INCLUDE) $(DEBUG) $(OPTIMIZE)

DIR_OBJECTS = obj/$(TOOL_PREFIX)$(PRECISION)/

# target file name
TARGET   = $(DIR_OBJECTS)calc.exe


OBJS= $(addprefix $(DIR_OBJECTS), $(addsuffix .o, $(FILES)))
RES = $(DIR_OBJECTS)resource.res

.PHONY: all all-before all-after clean clean-custom

all: all-before $(TARGET) all-after

clean: clean-custom
	$(RM) $(OBJ) $(TARGET)

$(TARGET): $(OBJS) $(RES)
	$(CC) $^ -o $@ $(LIBS)

$(DIR_OBJECTS)%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(RES): resource.rc 
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff 
