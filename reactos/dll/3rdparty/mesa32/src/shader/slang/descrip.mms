# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 17 March 2006

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
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = \
	slang_compile.c,slang_preprocess.c

OBJECTS = \
	slang_compile.obj,slang_preprocess.obj,slang_utility.obj,\
	slang_execute.obj,slang_assemble.obj,slang_assemble_conditional.obj,\
	slang_assemble_constructor.obj,slang_assemble_typeinfo.obj,\
	slang_storage.obj,slang_assemble_assignment.obj,\
	slang_compile_function.obj,slang_compile_struct.obj,\
	slang_compile_variable.obj,slang_compile_operation.obj,\
	slang_library_noise.obj,slang_link.obj,slang_export.obj,\
	slang_analyse.obj,slang_library_texsample.obj

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
slang_compile_function.obj : slang_compile_function.c
slang_compile_struct.obj : slang_compile_struct.c
slang_compile_variable.obj : slang_compile_variable.c
slang_compile_operation.obj : slang_compile_operation.c
slang_library_noise.obj : slang_library_noise.c
slang_link.obj : slang_link.c
slang_export.obj : slang_export.c
slang_analyse.obj : slang_analyse.c
slang_library_texsample.obj : slang_library_texsample.c
