# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 29 September 2008

.first
	define gl [---.include.gl]
	define main [-.main]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = glapi.c glthread.c glapi_getproc.c

OBJECTS =  glapi.obj,glthread.obj,glapi_getproc.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

glapi.obj : glapi.c

glthread.obj : glthread.c
glapi_getproc.obj : glapi_getproc.c
