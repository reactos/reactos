# Mesa 3-D graphics library
#
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

LOCAL_PATH := $(call my-dir)

# from Makefile
C_SOURCES = \
	sp_fs_exec.c \
	sp_clear.c \
	sp_fence.c \
	sp_flush.c \
	sp_query.c \
	sp_context.c \
	sp_draw_arrays.c \
	sp_prim_vbuf.c \
	sp_quad_pipe.c \
	sp_quad_stipple.c \
	sp_quad_depth_test.c \
	sp_quad_fs.c \
	sp_quad_blend.c \
	sp_screen.c \
        sp_setup.c \
	sp_state_blend.c \
	sp_state_clip.c \
	sp_state_derived.c \
	sp_state_sampler.c \
	sp_state_shader.c \
	sp_state_so.c \
	sp_state_rasterizer.c \
	sp_state_surface.c \
	sp_state_vertex.c \
	sp_texture.c \
	sp_tex_sample.c \
	sp_tex_tile_cache.c \
	sp_tile_cache.c \
	sp_surface.c

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(C_SOURCES)

LOCAL_MODULE := libmesa_pipe_softpipe

include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
