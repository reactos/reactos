# src/mesa/drivers/dri/radeon/Makefile
# Note, this Makefile requires GNU make

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeon_dri.so

MINIGLX_SOURCES = server/radeon_dri.c 

DRIVER_SOURCES = \
	radeon_context.c \
	radeon_ioctl.c \
	radeon_lock.c \
	radeon_screen.c \
	radeon_state.c \
	radeon_state_init.c \
	radeon_tex.c \
	radeon_texmem.c \
	radeon_texstate.c \
	radeon_tcl.c \
	radeon_swtcl.c \
	radeon_span.c \
	radeon_maos.c \
	radeon_sanity.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

DRIVER_DEFINES = -DRADEON_COMMON=0

X86_SOURCES = 

include ../Makefile.template

symlinks:
