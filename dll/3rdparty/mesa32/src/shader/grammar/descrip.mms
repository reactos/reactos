# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 1 June 2005

.first
	define gl [----.include.gl]
	define math [--.math]
	define swrast [--.swrast]
	define array_cache [--.array_cache]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[],[--.main],[--.glapi],[-.slang]
LIBDIR = [----.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES = grammar_mesa.c

OBJECTS = grammar_mesa.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
all : 
	$(MMS)$(MMSQUALIFIERS) $(LIBDIR)$(GL_LIB)

# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

grammar_mesa.obj : grammar_mesa.c grammar.c
