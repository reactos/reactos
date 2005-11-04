# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 23 March 2004

.first
	define gl [---.include.gl]
	define math [-.math]
	define swrast [-.swrast]
	define array_cache [-.array_cache]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES = \
	arbfragparse.c \
	arbprogparse.c \
	arbprogram.c \
	arbvertparse.c \
	grammar_mesa.c \
	nvfragparse.c \
	nvprogram.c \
	nvvertexec.c \
	nvvertparse.c \
	program.c

OBJECTS = \
	arbfragparse.obj,\
	arbprogparse.obj,\
	arbprogram.obj,\
	arbvertparse.obj,\
	grammar_mesa.obj,\
	nvfragparse.obj,\
	nvprogram.obj,\
	nvvertexec.obj,\
	nvvertparse.obj,\
	program.obj


##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*


arbfragparse.obj : arbfragparse.c
arbprogparse.obj : arbprogparse.c
arbprogram.obj : arbprogram.c
arbvertparse.obj : arbvertparse.c
grammar_mesa.obj : grammar_mesa.c
nvfragparse.obj : nvfragparse.c
nvprogram.obj : nvprogram.c
nvvertexec.obj : nvvertexec.c
nvvertparse.obj : nvvertparse.c
program.obj : program.c
