# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 1 June 2005

.first
	define gl [----.include.gl]
	define math [--.math]
	define swrast [--.swrast]
	define array_cache [--.array_cache]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[--.main],[--.glapi],[-.slang],[-.grammar],[-]
LIBDIR = [----.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES = \
	slang_compile.c,slang_preprocess.c

OBJECTS = \
	slang_compile.obj,slang_preprocess.obj,slang_utility.obj,\
	slang_execute.obj,slang_assemble.obj,slang_assemble_conditional.obj,\
	slang_assemble_constructor.obj,slang_assemble_typeinfo.obj,\
	slang_storage.obj,slang_assemble_assignment.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

slang_compile.obj : slang_compile.c
slang_preprocess.obj : slang_preprocess.c
slang_utility.obj : slang_utility.c
slang_execute.obj : slang_execute.c
slang_assemble.obj : slang_assemble.c
slang_assemble_conditional.obj : slang_assemble_conditional.c
slang_assemble_constructor.obj : slang_assemble_constructor.c
slang_assemble_typeinfo.obj : slang_assemble_typeinfo.c
slang_storage.obj : slang_storage.c
slang_assemble_assignment.obj : slang_assemble_assignment.c
