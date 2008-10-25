# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 30 November 2007

.first
	define gl [---.include.gl]
	define math [-.math]
	define vbo [-.vbo]
	define shader [-.shader]
	define swrast [-.swrast]
	define array_cache [-.array_cache]
	define main [-.main]
	define glapi [-.glapi]
	define tnl [-.tnl]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi],[-.shader],[-.shader.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = t_context.c t_draw.c \
	t_pipeline.c t_vb_fog.c \
	t_vb_light.c t_vb_normals.c t_vb_points.c t_vb_program.c \
	t_vb_render.c t_vb_texgen.c t_vb_texmat.c t_vb_vertex.c \
	t_vertex.c \
	t_vertex_generic.c t_vp_build.c

OBJECTS = t_context.obj,t_draw.obj,\
	t_pipeline.obj,t_vb_fog.obj,t_vb_light.obj,t_vb_normals.obj,\
	t_vb_points.obj,t_vb_program.obj,t_vb_render.obj,t_vb_texgen.obj,\
	t_vb_texmat.obj,t_vb_vertex.obj,\
	t_vertex.obj,t_vertex_generic.obj,\
	t_vp_build.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

t_context.obj : t_context.c
t_draw.obj : t_draw.c
t_pipeline.obj : t_pipeline.c
t_vb_fog.obj : t_vb_fog.c
t_vb_light.obj : t_vb_light.c
t_vb_normals.obj : t_vb_normals.c
t_vb_points.obj : t_vb_points.c
t_vb_program.obj : t_vb_program.c
t_vb_render.obj : t_vb_render.c
t_vb_texgen.obj : t_vb_texgen.c
t_vb_texmat.obj : t_vb_texmat.c
t_vb_vertex.obj : t_vb_vertex.c
t_vertex.obj : t_vertex.c
t_vertex_generic.obj : t_vertex_generic.c
t_vp_build.obj : t_vp_build.c
