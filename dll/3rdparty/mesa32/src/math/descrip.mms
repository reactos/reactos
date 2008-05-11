# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 16 June 2003

.first
	define gl [---.include.gl]
	define math [-.math]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.main],[-.glapi]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = m_debug_clip.c m_debug_norm.c m_debug_xform.c m_eval.c m_matrix.c\
	m_translate.c m_vector.c m_xform.c

OBJECTS = m_debug_clip.obj,m_debug_norm.obj,m_debug_xform.obj,m_eval.obj,\
	m_matrix.obj,m_translate.obj,m_vector.obj,m_xform.obj
 
##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

m_debug_clip.obj : m_debug_clip.c
m_debug_norm.obj : m_debug_norm.c
m_debug_xform.obj : m_debug_xform.c
m_eval.obj : m_eval.c
m_matrix.obj : m_matrix.c
m_translate.obj : m_translate.c
m_vector.obj : m_vector.c
m_xform.obj : m_xform.c
