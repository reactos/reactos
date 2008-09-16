# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 7 March 2007

.first
	define gl [---.include.gl]
	define math [-.math]
	define vbo [-.vbo]
	define tnl [-.tnl]
	define shader [-.shader]
	define swrast [-.swrast]
	define swrast_setup [-.swrast_setup]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi],[-.shader],[-.shader.slang]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES =vbo_context.c,vbo_exec.c,vbo_exec_api.c,vbo_exec_array.c,\
	vbo_exec_draw.c,vbo_exec_eval.c,vbo_rebase.c,vbo_save.c,\
	vbo_save_api.c,vbo_save_draw.c,vbo_save_loopback.c,\
	vbo_split.c,vbo_split_copy.c,vbo_split_inplace.c

OBJECTS =vbo_context.obj,vbo_exec.obj,vbo_exec_api.obj,vbo_exec_array.obj,\
	vbo_exec_draw.obj,vbo_exec_eval.obj,vbo_rebase.obj,vbo_save.obj,\
	vbo_save_api.obj,vbo_save_draw.obj,vbo_save_loopback.obj,\
	vbo_split.obj,vbo_split_copy.obj,vbo_split_inplace.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

vbo_context.obj : vbo_context.c
vbo_exec.obj : vbo_exec.c
vbo_exec_api.obj : vbo_exec_api.c
vbo_exec_array.obj : vbo_exec_array.c
vbo_exec_draw.obj : vbo_exec_draw.c
vbo_exec_eval.obj : vbo_exec_eval.c
vbo_rebase.obj : vbo_rebase.c
vbo_save.obj : vbo_save.c
vbo_save_api.obj : vbo_save_api.c
vbo_save_draw.obj : vbo_save_draw.c
vbo_save_loopback.obj : vbo_save_loopback.c
vbo_split.obj : vbo_split.c
vbo_split_copy.obj : vbo_split_copy.c
vbo_split_inplace.obj : vbo_split_inplace.c
