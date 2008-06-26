
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i965_dri.so

DRIVER_SOURCES = \
	bufmgr_fake.c \
	intel_batchbuffer.c \
	intel_blit.c \
	intel_buffer_objects.c \
	intel_buffers.c \
	intel_context.c \
	intel_ioctl.c \
	intel_mipmap_tree.c \
	intel_regions.c \
	intel_screen.c \
	intel_span.c \
	intel_pixel_copy.c \
	intel_pixel_bitmap.c \
	intel_state.c \
	intel_tex.c \
	intel_tex_layout.c \
	intel_tex_validate.c \
	brw_aub.c \
	brw_aub_playback.c \
	brw_cc.c \
	brw_clip.c \
	brw_clip_line.c \
	brw_clip_point.c \
	brw_clip_state.c \
	brw_clip_tri.c \
	brw_clip_unfilled.c \
	brw_clip_util.c \
	brw_context.c \
	brw_curbe.c \
	brw_draw.c \
	brw_draw_upload.c \
	brw_eu.c \
	brw_eu_debug.c \
	brw_eu_emit.c \
	brw_eu_util.c \
	brw_fallback.c \
	brw_gs.c \
	brw_gs_emit.c \
	brw_gs_state.c \
	brw_hal.c \
	brw_metaops.c \
	brw_misc_state.c \
	brw_program.c \
	brw_sf.c \
	brw_sf_emit.c \
	brw_sf_state.c \
	brw_state_batch.c \
	brw_state_cache.c \
	brw_state_pool.c \
	brw_state_upload.c \
	brw_tex.c \
	brw_tex_layout.c \
	brw_urb.c \
	brw_util.c \
	brw_vs.c \
	brw_vs_constval.c \
	brw_vs_emit.c \
	brw_vs_state.c \
	brw_vs_tnl.c \
	brw_vtbl.c \
	brw_wm.c \
	brw_wm_debug.c \
	brw_wm_emit.c \
	brw_wm_fp.c \
	brw_wm_iz.c \
	brw_wm_pass0.c \
	brw_wm_pass1.c \
	brw_wm_pass2.c \
	brw_wm_sampler_state.c \
	brw_wm_state.c \
	brw_wm_surface_state.c 

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(MINIGLX_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I../intel

include ../Makefile.template

intel_tex_layout.o: ../intel/intel_tex_layout.c

symlinks:
