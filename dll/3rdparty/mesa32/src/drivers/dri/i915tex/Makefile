
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915tex_dri.so

MINIGLX_SOURCES = server/intel_dri.c

DRIVER_SOURCES = \
	i830_context.c \
	i830_metaops.c \
	i830_state.c \
	i830_texblend.c \
	i830_tex.c \
	i830_texstate.c \
	i830_vtbl.c \
	intel_render.c \
	intel_regions.c \
	intel_buffer_objects.c \
	intel_batchbuffer.c \
	intel_mipmap_tree.c \
	i915_tex_layout.c \
	intel_tex_layout.c \
	intel_tex_image.c \
	intel_tex_subimage.c \
	intel_tex_copy.c \
	intel_tex_validate.c \
	intel_tex_format.c \
	intel_tex.c \
	intel_pixel.c \
	intel_pixel_copy.c \
	intel_pixel_read.c \
	intel_pixel_draw.c \
	intel_buffers.c \
	intel_blit.c \
	i915_tex.c \
	i915_texstate.c \
	i915_context.c \
	i915_debug.c \
	i915_fragprog.c \
	i915_metaops.c \
	i915_program.c \
	i915_state.c \
	i915_vtbl.c \
	intel_context.c \
	intel_ioctl.c \
	intel_rotate.c \
	intel_screen.c \
	intel_span.c \
	intel_state.c \
	intel_tris.c \
	intel_fbo.c \
	intel_depthstencil.c \
	intel_batchpool.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(COMMON_BM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I../intel $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../Makefile.template

intel_tex_layout.o: ../intel/intel_tex_layout.c

symlinks:
