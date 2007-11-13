# src/mesa/drivers/dri/mach64/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = mach64_dri.so

# Not yet
# MINIGLX_SOURCES = server/mach64_dri.c 

DRIVER_SOURCES = \
	mach64_context.c \
	mach64_ioctl.c \
	mach64_screen.c \
	mach64_span.c \
	mach64_state.c \
	mach64_tex.c \
	mach64_texmem.c \
	mach64_texstate.c \
	mach64_tris.c \
	mach64_vb.c \
	mach64_dd.c \
	mach64_lock.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
