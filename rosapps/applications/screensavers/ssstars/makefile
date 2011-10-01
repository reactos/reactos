# Makefile for Starfield screensaver
#
# syntax:
#   make <platform> <unicode>
#
# parameters:
#   <platform>
#       x68_64=1 : compile for x86_64
#       undefined: use default tools (typically IA_32)
#   <unicode>
#       unicode=1: compile as UNICODE application
#       undefined: compile as ASCII application

# Makefile for Starfield screensaver

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
LIBS     = -lopengl32 -lglu32 -lscrnsave -lcomctl32 -s -mwindows

# Base files
FILES = stars settings

# Check the UNICODE support
ifeq ($(unicode), 1)
UNICODE=u
DEFS  += -DUNICODE -D_UNICODE
else
UNICODE=a
endif

CFLAGS   = $(DEFS) $(INCLUDE) $(DEBUG) $(OPTIMIZE)

DIR_OBJECTS = obj/$(TOOL_PREFIX)$(UNICODE)/

# target file name
TARGET   = $(DIR_OBJECTS)ssstars.scr

OBJS= $(addprefix $(DIR_OBJECTS), $(addsuffix .o, $(FILES)))
RES = $(DIR_OBJECTS)resource.res

.PHONY: all all-before all-after clean clean-custom

all: all-before $(TARGET) all-after

clean: clean-custom
	$(RM) $(OBJ) $(TARGET)

$(TARGET): $(OBJS) $(RES)
	$(CC) $^ -o $@ $(LIBS)

$(DIR_OBJECTS)%.o: %.c $(DIR_OBJECTS)
	$(CC) -c $< -o $@ $(CFLAGS)

$(RES): resource.rc 
	$(WINDRES) -i $< --input-format=rc -o $@ -O coff 

$(DIR_OBJECTS):
	-mkdir $@
