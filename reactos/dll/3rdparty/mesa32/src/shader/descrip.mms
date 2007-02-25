# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 1 June 2005

.first
	define gl [---.include.gl]
	define math [-.math]
	define swrast [-.swrast]
	define array_cache [-.array_cache]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[.grammar],[-.main],[-.glapi],[.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES = \
	atifragshader.c \
	arbfragparse.c \
	arbprogparse.c \
	arbprogram.c \
	arbvertparse.c \
	nvfragparse.c \
	nvprogram.c \
	nvvertexec.c \
	nvvertparse.c \
	program.c \
	shaderobjects.c \
	shaderobjects_3dlabs.c

OBJECTS = \
	atifragshader.obj,\
	arbfragparse.obj,\
	arbprogparse.obj,\
	arbprogram.obj,\
	arbvertparse.obj,\
	nvfragparse.obj,\
	nvprogram.obj,\
	nvvertexec.obj,\
	nvvertparse.obj,\
	program.obj,\
	shaderobjects.obj,\
	shaderobjects_3dlabs.obj


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
arbfragparse.obj : arbfragparse.c
arbprogparse.obj : arbprogparse.c
arbprogram.obj : arbprogram.c
arbvertparse.obj : arbvertparse.c
nvfragparse.obj : nvfragparse.c
nvprogram.obj : nvprogram.c
nvvertexec.obj : nvvertexec.c
nvvertparse.obj : nvvertparse.c
program.obj : program.c
shaderobjects.obj : shaderobjects.c
	cc$(CFLAGS)/nowarn shaderobjects.c
shaderobjects_3dlabs.obj : shaderobjects_3dlabs.c
