# src/mesa/drivers/dri/sis/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = sis_dri.so


# Not yet
# MINIGLX_SOURCES = server/sis_dri.c 

DRIVER_SOURCES = \
	sis6326_state.c \
	sis6326_clear.c \
	sis_alloc.c \
	sis_clear.c \
	sis_context.c \
	sis_dd.c \
	sis_fog.c \
	sis_lock.c \
	sis_screen.c \
	sis_span.c \
	sis_state.c \
	sis_stencil.c \
	sis_tex.c \
	sis_texstate.c \
	sis_tris.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
