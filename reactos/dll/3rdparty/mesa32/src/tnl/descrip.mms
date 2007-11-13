# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 21 February 2006

.first
	define gl [---.include.gl]
	define math [-.math]
	define shader [-.shader]
	define array_cache [-.array_cache]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi],[-.shader],[-.shader.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = t_array_api.c t_array_import.c t_context.c \
	t_pipeline.c t_vb_fog.c t_save_api.c t_vtx_api.c \
	t_vb_light.c t_vb_normals.c t_vb_points.c t_vb_program.c \
	t_vb_render.c t_vb_texgen.c t_vb_texmat.c t_vb_vertex.c \
	t_vtx_eval.c t_vtx_exec.c t_save_playback.c t_save_loopback.c \
	t_vertex.c t_vtx_generic.c t_vtx_x86.c t_vertex_generic.c \
	t_vb_arbprogram.c t_vp_build.c t_vb_arbshader.c

OBJECTS = t_array_api.obj,t_array_import.obj,t_context.obj,\
	t_pipeline.obj,t_vb_fog.obj,t_vb_light.obj,t_vb_normals.obj,\
	t_vb_points.obj,t_vb_program.obj,t_vb_render.obj,t_vb_texgen.obj,\
	t_vb_texmat.obj,t_vb_vertex.obj,t_save_api.obj,t_vtx_api.obj,\
	t_vtx_eval.obj,t_vtx_exec.obj,t_save_playback.obj,t_save_loopback.obj,\
	t_vertex.obj,t_vtx_generic.obj,t_vtx_x86.obj,t_vertex_generic.obj,\
	t_vb_arbprogram.obj,t_vp_build.obj,t_vb_arbshader.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

t_array_api.obj : t_array_api.c
t_array_import.obj : t_array_import.c
t_context.obj : t_context.c
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
t_save_api.obj : t_save_api.c
t_vtx_api.obj : t_vtx_api.c
t_vtx_eval.obj : t_vtx_eval.c
t_vtx_exec.obj : t_vtx_exec.c
t_save_playback.obj : t_save_playback.c
t_save_loopback.obj : t_save_loopback.c
t_vertex.obj : t_vertex.c
t_vtx_x86.obj : t_vtx_x86.c
t_vtx_generic.obj : t_vtx_generic.c
t_vertex_generic.obj : t_vertex_generic.c
t_vb_arbprogram.obj : t_vb_arbprogram.c
t_vp_build.obj : t_vp_build.c
t_vb_arbshader.obj : t_vb_arbshader.c
