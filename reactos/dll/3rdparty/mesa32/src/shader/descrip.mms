# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 27 May 2008
.first
	define gl [---.include.gl]
	define math [-.math]
	define swrast [-.swrast]
	define array_cache [-.array_cache]
	define glapi [-.glapi]
	define main [-.main]
	define shader [-.shader]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[.grammar],[-.main],[-.glapi],[.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1,"__extension__=")/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = \
	atifragshader.c \
	arbprogparse.c \
	arbprogram.c \
	nvfragparse.c \
	nvprogram.c \
	nvvertparse.c \
	program.c \
	programopt.c \
	prog_debug.c \
	prog_execute.c \
	prog_instruction.c \
	prog_parameter.c \
	prog_print.c \
	prog_statevars.c \
	shader_api.c prog_uniform.c

OBJECTS = \
	atifragshader.obj,\
	arbprogparse.obj,\
	arbprogram.obj,\
	nvfragparse.obj,\
	nvprogram.obj,\
	nvvertparse.obj,\
	program.obj,\
	programopt.obj,\
	prog_debug.obj,\
	prog_execute.obj,\
	prog_instruction.obj,\
	prog_parameter.obj,\
	prog_print.obj,\
	prog_statevars.obj,\
	shader_api.obj,prog_uniform.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
all : 
	$(MMS)$(MMSQUALIFIERS) $(LIBDIR)$(GL_LIB)
	set def [.slang]
	$(MMS)$(MMSQUALIFIERS)
	set def [-.grammar]
	$(MMS)$(MMSQUALIFIERS)
	set def [-]

# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

atifragshader.obj : atifragshader.c
arbprogparse.obj : arbprogparse.c
arbprogram.obj : arbprogram.c
nvfragparse.obj : nvfragparse.c
nvprogram.obj : nvprogram.c
nvvertparse.obj : nvvertparse.c
program.obj : program.c
programopt. obj : programopt.c
prog_debug.obj : prog_debug.c
prog_execute.obj : prog_execute.c
prog_instruction.obj : prog_instruction.c
prog_parameter.obj : prog_parameter.c
prog_print.obj : prog_print.c
prog_statevars.obj : prog_statevars.c
shader_api.obj : shader_api.c
prog_uniform.obj : prog_uniform.c
