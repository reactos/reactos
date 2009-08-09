# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 3 October 2007

.first
	define gl [----.include.gl]
	define math [--.math]
	define swrast [--.swrast]
	define array_cache [--.array_cache]
	define main [--.main]
	define glapi [--.glapi]
	define shader [--.shader]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[--.main],[--.glapi],[-.slang],[-.grammar],[-]
LIBDIR = [----.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = \
	slang_compile.c,slang_preprocess.c

OBJECTS = slang_builtin.obj,slang_codegen.obj,slang_compile.obj,\
	slang_compile_function.obj,slang_compile_operation.obj,\
	slang_compile_struct.obj,slang_compile_variable.obj,slang_emit.obj,\
	slang_ir.obj,slang_label.obj,slang_library_noise.obj,slang_link.obj,\
	slang_log.obj,slang_mem.obj,slang_preprocess.obj,slang_print.obj,\
	slang_simplify.obj,slang_storage.obj,slang_typeinfo.obj,\
	slang_utility.obj,slang_vartable.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

slang_builtin.obj : slang_builtin.c
slang_codegen.obj : slang_codegen.c
slang_compile.obj : slang_compile.c
slang_compile_function.obj : slang_compile_function.c
slang_compile_operation.obj : slang_compile_operation.c
slang_compile_struct.obj : slang_compile_struct.c
slang_compile_variable.obj : slang_compile_variable.c
slang_emit.obj : slang_emit.c
slang_ir.obj : slang_ir.c
slang_label.obj : slang_label.c
slang_library_noise.obj : slang_library_noise.c
slang_link.obj : slang_link.c
slang_log.obj : slang_log.c
slang_mem.obj : slang_mem.c
slang_preprocess.obj : slang_preprocess.c
slang_print.obj : slang_print.c
slang_simplify.obj : slang_simplify.c
slang_storage.obj : slang_storage.c
slang_typeinfo.obj : slang_typeinfo.c
slang_utility.obj : slang_utility.c
slang_vartable.obj : slang_vartable.c
