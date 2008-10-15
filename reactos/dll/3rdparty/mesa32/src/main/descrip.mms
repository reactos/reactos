# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.nano.tudelft.nl
# Last revision : 2 October 2007

.first
	define gl [---.include.gl]
	define math [-.math]
	define shader [-.shader]
	define glapi [-.glapi]
	define main [-.main]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.glapi],[-.shader]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)/float=ieee/ieee=denorm

SOURCES =accum.c \
	api_arrayelt.c \
	api_loopback.c \
	api_noop.c \
	api_validate.c \
 	attrib.c \
	arrayobj.c \
	blend.c \
	bufferobj.c \
	buffers.c \
	clip.c \
	colortab.c \
	context.c \
	convolve.c \
	debug.c \
	depth.c \
	depthstencil.c \
	dispatch.c \
	dlist.c \
	drawpix.c \
	enable.c \
	enums.c \
	eval.c \
	execmem.c \
	extensions.c \
	fbobject.c \
	feedback.c \
	fog.c \
	framebuffer.c \
	get.c \
	getstring.c \
	hash.c \
	hint.c \
	histogram.c \
	image.c \
	imports.c \
	light.c \
	lines.c \
	matrix.c \
	mipmap.c \
	mm.c \
	pixel.c \
	points.c \
	polygon.c \
	rastpos.c \
	rbadaptors.c \
	renderbuffer.c \
	shaders.c \
	state.c \
	stencil.c \
	texcompress.c \
	texcompress_fxt1.c \
	texcompress_s3tc.c \
	texenvprogram.c \
	texformat.c \
	teximage.c \
	texobj.c \
	texrender.c \
	texstate.c \
	texstore.c \
	varray.c \
	vtxfmt.c \
	queryobj.c \
	rbadaptors.c

OBJECTS=accum.obj,\
api_arrayelt.obj,\
api_loopback.obj,\
api_noop.obj,\
api_validate.obj,\
arrayobj.obj,\
attrib.obj,\
blend.obj,\
bufferobj.obj,\
buffers.obj,\
clip.obj,\
colortab.obj,\
context.obj,\
convolve.obj,\
debug.obj,\
depth.obj,\
depthstencil.obj,\
dispatch.obj,\
dlist.obj,\
drawpix.obj,\
enable.obj,\
enums.obj,\
eval.obj,\
execmem.obj,\
extensions.obj,\
fbobject.obj,\
feedback.obj,\
fog.obj,\
framebuffer.obj,\
get.obj,\
getstring.obj,\
hash.obj,\
hint.obj,\
histogram.obj,\
image.obj,\
imports.obj,\
light.obj,\
lines.obj,\
matrix.obj,\
mipmap.obj,\
mm.obj,\
pixel.obj,\
points.obj,\
polygon.obj,\
rastpos.obj,\
renderbuffer.obj,\
shaders.obj,\
state.obj,\
stencil.obj,\
texcompress.obj,\
texcompress_fxt1.obj,\
texcompress_s3tc.obj,\
texenvprogram.obj,\
texformat.obj,\
teximage.obj,\
texobj.obj,\
texrender.obj,\
texstate.obj,\
texstore.obj,\
varray.obj,\
vtxfmt.obj,\
queryobj.obj,\
rbadaptors.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ $(MAKELIB) $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

accum.obj : accum.c
api_arrayelt.obj : api_arrayelt.c
api_loopback.obj : api_loopback.c
api_noop.obj : api_noop.c
api_validate.obj : api_validate.c
arrayobj.obj : arrayobj.c
attrib.obj : attrib.c
blend.obj : blend.c
bufferobj.obj : bufferobj.c
buffers.obj : buffers.c
clip.obj : clip.c
colortab.obj : colortab.c
context.obj : context.c
convolve.obj : convolve.c
debug.obj : debug.c
depth.obj : depth.c
depthstencil.obj : depthstencil.c
dispatch.obj : dispatch.c
dlist.obj : dlist.c
drawpix.obj : drawpix.c
enable.obj : enable.c
enums.obj : enums.c
eval.obj : eval.c
execmem.obj : execmem.c
extensions.obj : extensions.c
fbobject.obj : fbobject.c
feedback.obj : feedback.c
fog.obj : fog.c
framebuffer.obj : framebuffer.c
get.obj : get.c
getstring.obj : getstring.c
hash.obj : hash.c
hint.obj : hint.c
histogram.obj : histogram.c
image.obj : image.c
imports.obj : imports.c vsnprintf.c
light.obj : light.c
lines.obj : lines.c
matrix.obj : matrix.c
mipmap.obj : mipmap.c
mm.obj : mm.c
pixel.obj : pixel.c
points.obj : points.c
polygon.obj : polygon.c
rastpos.obj : rastpos.c
rbadaptors.obj : rbadaptors.c
renderbuffer.obj : renderbuffer.c
state.obj : state.c
stencil.obj : stencil.c
texcompress.obj : texcompress.c
texcompress_fxt1.obj : texcompress_fxt1.c
	cc$(CFLAGS)/warn=(disable=SHIFTCOUNT) texcompress_fxt1.c
texcompress_s3tc.obj : texcompress_s3tc.c
texenvprogram.obj : texenvprogram.c
texformat.obj : texformat.c
teximage.obj : teximage.c
texobj.obj : texobj.c
texrender.obj : texrender.c
texstate.obj : texstate.c
texstore.obj : texstore.c
varray.obj : varray.c
vtxfmt.obj : vtxfmt.c
shaders.obj : shaders.c
queryobj.obj : queryobj.c
rbadaptors.obj : rbadaptors.c
