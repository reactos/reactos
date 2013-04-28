# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 3 October 2007

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

INCDIR = [---.include],[-.main],[-.glapi],[-.shader],[-.shader.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = s_aaline.c s_aatriangle.c s_accum.c s_alpha.c \
	s_bitmap.c s_blend.c s_blit.c s_buffers.c s_context.c \
	s_copypix.c s_depth.c s_fragprog.c \
        s_drawpix.c s_feedback.c s_fog.c s_imaging.c s_lines.c s_logic.c \
	s_masking.c s_points.c s_readpix.c \
	s_span.c s_stencil.c s_texstore.c s_texcombine.c s_texfilter.c \
	s_triangle.c s_zoom.c s_atifragshader.c
 
OBJECTS = s_aaline.obj,s_aatriangle.obj,s_accum.obj,s_alpha.obj,\
	s_bitmap.obj,s_blend.obj,s_blit.obj,s_fragprog.obj,\
	s_buffers.obj,s_context.obj,s_atifragshader.obj,\
	s_copypix.obj,s_depth.obj,s_drawpix.obj,s_feedback.obj,s_fog.obj,\
	s_imaging.obj,s_lines.obj,s_logic.obj,s_masking.obj,\
	s_points.obj,s_readpix.obj,s_span.obj,s_stencil.obj,\
	s_texstore.obj,s_texcombine.obj,s_texfilter.obj,s_triangle.obj,\
	s_zoom.obj
 
##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

s_atifragshader.obj : s_atifragshader.c
s_aaline.obj : s_aaline.c
s_aatriangle.obj : s_aatriangle.c
s_accum.obj : s_accum.c
s_alpha.obj : s_alpha.c
s_bitmap.obj : s_bitmap.c
s_blend.obj : s_blend.c
s_blit.obj : s_blit.c
s_buffers.obj : s_buffers.c
s_context.obj : s_context.c
s_copypix.obj : s_copypix.c
s_depth.obj : s_depth.c
s_drawpix.obj : s_drawpix.c
s_feedback.obj : s_feedback.c
s_fog.obj : s_fog.c
s_imaging.obj : s_imaging.c
s_lines.obj : s_lines.c
s_logic.obj : s_logic.c
s_masking.obj : s_masking.c
s_points.obj : s_points.c
s_readpix.obj : s_readpix.c
s_span.obj : s_span.c
s_stencil.obj : s_stencil.c
s_texstore.obj : s_texstore.c
s_texcombine.obj : s_texcombine.c
s_texfilter.obj : s_texfilter.c
s_triangle.obj : s_triangle.c
s_zoom.obj : s_zoom.c
s_fragprog.obj : s_fragprog.c
