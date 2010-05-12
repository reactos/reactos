# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 3 October 2007

.first
	define gl [---.include.gl]
	define math [-.math]
	define tnl [-.tnl]
	define vbo [-.vbo]
	define swrast [-.swrast]
	define array_cache [-.array_cache]
	define glapi [-.glapi]
	define main [-.main]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = ss_context.c ss_triangle.c

OBJECTS =  ss_context.obj,ss_triangle.obj
##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

ss_context.obj : ss_context.c
ss_triangle.obj : ss_triangle.c
