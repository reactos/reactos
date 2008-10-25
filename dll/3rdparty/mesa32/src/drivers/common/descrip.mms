# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 3 October 2007

.first
	define gl [----.include.gl]
	define math [--.math]
	define tnl [--.tnl]
	define swrast [--.swrast]
	define glapi [--.glapi]
	define shader [--.shader]
	define main [--.main]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[--.main],[--.glapi],[--.shader]
LIBDIR = [----.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = driverfuncs.c

OBJECTS =driverfuncs.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

driverfuncs.obj : driverfuncs.c
