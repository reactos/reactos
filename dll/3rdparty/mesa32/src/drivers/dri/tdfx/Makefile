# src/mesa/drivers/dri/tdfx/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = tdfx_dri.so

# not yet
# MINIGLX_SOURCES = server/tdfx_dri.c 

DRIVER_SOURCES = \
	tdfx_context.c \
	tdfx_dd.c \
	tdfx_lock.c \
	tdfx_pixels.c \
	tdfx_render.c \
	tdfx_screen.c \
	tdfx_span.c \
	tdfx_state.c \
	tdfx_tex.c \
	tdfx_texman.c \
	tdfx_texstate.c \
	tdfx_tris.c \
	tdfx_vb.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)


ASM_SOURCES = 

include ../Makefile.template

symlinks:


