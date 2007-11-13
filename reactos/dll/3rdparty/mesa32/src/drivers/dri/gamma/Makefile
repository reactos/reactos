# src/mesa/drivers/dri/gamma/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = gamma_dri.so

# Not yet
# MINIGLX_SOURCES = server/gamma_dri.c 

DRIVER_SOURCES = \
	gamma_context.c \
	gamma_dd.c \
	gamma_inithw.c \
	gamma_lock.c \
	gamma_render.c \
	gamma_screen.c \
	gamma_span.c \
	gamma_state.c \
	gamma_tex.c \
	gamma_texmem.c \
	gamma_texstate.c \
	gamma_tris.c \
	gamma_vb.c \
	gamma_xmesa.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
