# src/mesa/drivers/dri/r300/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r300_dri.so

MINIGLX_SOURCES = server/radeon_dri.c

ifeq ($(USING_EGL), 1)
EGL_SOURCES = server/radeon_egl.c
endif

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c

DRIVER_SOURCES = \
		 radeon_screen.c \
		 radeon_context.c \
		 radeon_ioctl.c \
		 radeon_lock.c \
		 radeon_span.c \
		 radeon_state.c \
		 r300_mem.c \
		 \
		 r300_context.c \
		 r300_ioctl.c \
		 r300_cmdbuf.c \
		 r300_state.c \
		 r300_render.c \
		 r300_texmem.c \
		 r300_tex.c \
		 r300_texstate.c \
		 r300_vertprog.c \
		 r300_fragprog.c \
		 r300_shader.c \
		 r300_emit.c \
		 $(EGL_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

DRIVER_DEFINES = -DCOMPILE_R300 -DR200_MERGED=0 \
	-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R300

SYMLINKS = \
	server/radeon_dri.c \
	server/radeon_dri.h \
	server/radeon.h \
	server/radeon_macros.h \
	server/radeon_reg.h \
	server/radeon_egl.c

COMMON_SYMLINKS = \
	radeon_chipset.h \
	radeon_screen.c \
	radeon_screen.h \
	radeon_span.h

##### TARGETS #####

include ../Makefile.template

server:
	mkdir -p server

$(SYMLINKS): server
	@[ -e $@ ] || ln -sf ../../radeon/$@ server/

$(COMMON_SYMLINKS):
	@[ -e $@ ] || ln -sf ../radeon/$@ ./

symlinks: $(SYMLINKS) $(COMMON_SYMLINKS)
