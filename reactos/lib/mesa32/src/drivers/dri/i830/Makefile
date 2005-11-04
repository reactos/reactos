
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i830_dri.so

DRIVER_SOURCES = \
	i830_context.c \
	i830_debug.c \
	i830_ioctl.c \
	i830_render.c \
	i830_screen.c \
	i830_span.c \
	i830_state.c \
	i830_tex.c \
	i830_texmem.c \
	i830_texstate.c \
	i830_tris.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(MINIGLX_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 



include ../Makefile.template

symlinks:
