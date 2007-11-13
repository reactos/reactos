# src/mesa/drivers/dri/s3v/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = s3v_dri.so

# Doesn't exist yet.
#MINIGLX_SOURCES = server/savage_dri.c 

DRIVER_SOURCES = \
	s3v_context.c \
        s3v_dd.c \
        s3v_inithw.c \
        s3v_lock.c \
        s3v_render.c \
        s3v_screen.c \
        s3v_span.c \
        s3v_state.c \
        s3v_tex.c \
        s3v_texmem.c \
        s3v_texstate.c \
        s3v_tris.c \
        s3v_vb.c \
        s3v_xmesa.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.template

symlinks:
