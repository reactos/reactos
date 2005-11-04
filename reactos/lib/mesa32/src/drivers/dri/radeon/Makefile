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
	radeon_sanity.c \
	radeon_compat.c \
	radeon_vtxfmt.c \
	radeon_vtxfmt_c.c \
	radeon_vtxfmt_sse.c \
	radeon_vtxfmt_x86.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 


X86_SOURCES = \
	radeon_vtxtmp_x86.S 

include ../Makefile.template

symlinks:
