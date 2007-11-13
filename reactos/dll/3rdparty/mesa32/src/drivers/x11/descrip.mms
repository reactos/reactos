# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 16 June 2003

.first
	define gl [----.include.gl]
	define math [--.math]
	define tnl [--.tnl]
	define swrast [--.swrast]
	define swrast_setup [--.swrast_setup]
	define array_cache [--.array_cache]
	define drivers [-]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[--.main],[--.glapi]
LIBDIR = [----.lib]
CFLAGS =/include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = fakeglx.c glxapi.c xfonts.c xm_api.c xm_dd.c xm_line.c xm_span.c\
	xm_tri.c xm_buffer.c

OBJECTS =fakeglx.obj,glxapi.obj,xfonts.obj,xm_api.obj,xm_dd.obj,xm_line.obj,\
	xm_span.obj,xm_tri.obj,xm_buffer.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

fakeglx.obj : fakeglx.c
glxapi.obj : glxapi.c
xfonts.obj : xfonts.c
xm_api.obj : xm_api.c
xm_buffer.obj : xm_buffer.c
xm_dd.obj : xm_dd.c
xm_line.obj : xm_line.c
xm_span.obj : xm_span.c
xm_tri.obj : xm_tri.c
