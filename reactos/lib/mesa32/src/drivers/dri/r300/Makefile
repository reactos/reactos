# src/mesa/drivers/dri/r300/Makefile


TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r300_dri.so
DEFINES += -DCOMPILE_R300 -DGLX_DIRECT_RENDERING -DR200_MERGED=0

MINIGLX_SOURCES = server/radeon_dri.c

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
		 \
		 r300_context.c \
		 r300_ioctl.c \
		 r300_cmdbuf.c \
		 r300_state.c \
		 r300_render.c \
		 r300_texmem.c \
		 r300_tex.c \
		 r300_texstate.c \
		 r300_texprog.c \
		 r300_vertexprog.c \
		 r300_fragprog.c \
		 r300_shader.c \
		 r300_maos.c 
#		 \
#		 r200_context.c \
#		 r200_ioctl.c \
#		 r200_state.c \
#		 r200_state_init.c \
#		 r200_cmdbuf.c \
#		 r200_pixel.c \
#		 r200_tex.c \
#		 r200_texmem.c \
#		 r200_texstate.c \
#		 r200_swtcl.c \
#		 r200_maos.c \
#		 r200_sanity.c \
#		 r200_vtxfmt.c \
#		 r200_vtxfmt_c.c \
#		 r200_vtxfmt_sse.c \
#		 r200_vtxfmt_x86.c


C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

X86_SOURCES = 
#r200_vtxtmp_x86.S

SYMLINKS = \
	server/radeon_dri.c \
	server/radeon_dri.h \
	server/radeon.h \
	server/radeon_macros.h \
	server/radeon_reg.h

##### TARGETS #####


include ../Makefile.template

$(SYMLINKS):
	mkdir -p server
	for i in $(SYMLINKS) ; do rm -f $$i && test -f ../radeon/$$i && ln -s ../../radeon/$$i $$i ; done

symlinks: $(SYMLINKS)

