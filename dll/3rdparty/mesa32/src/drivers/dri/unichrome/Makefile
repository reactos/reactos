# src/mesa/drivers/dri/unichrome/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = unichrome_dri.so

MINIGLX_SOURCES = server/via_dri.c 

DRIVER_SOURCES = \
	via_context.c \
	via_fb.c \
	via_tex.c \
	via_ioctl.c \
	via_memcpy.c \
	via_render.c \
	via_screen.c \
	via_span.c \
	via_state.c \
	via_texcombine.c \
	via_tris.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 


include ../Makefile.template

symlinks:
