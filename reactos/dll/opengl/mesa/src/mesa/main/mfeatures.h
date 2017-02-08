/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file mfeatures.h
 * Flags to enable/disable specific parts of the API.
 */

#ifndef FEATURES_H
#define FEATURES_H


#ifndef _HAVE_FULL_GL
#define _HAVE_FULL_GL 1
#endif

/* assert that a feature is disabled and should never be used */
#define ASSERT_NO_FEATURE() ASSERT(0)

/**
 * A feature can be anything.  But most of them share certain characteristics.
 *
 * When a feature defines vtxfmt entries, they can be initialized and
 * installed by
 *   _MESA_INIT_<FEATURE>_VTXFMT
 *   _mesa_install_<feature>_vtxfmt
 *
 * When a feature defines dispatch entries, they are initialized by
 *   _mesa_init_<feature>_dispatch
 *
 * When a feature has states, they are initialized and freed by
 *   _mesa_init_<feature>
 *   _mesa_free_<feature>_data
 *
 * Except for states, the others compile to no-op when a feature is disabled.
 *
 * The GLAPIENTRYs and helper functions defined by a feature should also
 * compile to no-op when it is disabled.  But to save typings and to catch
 * bugs, some of them may be unavailable, or compile to ASSERT_NO_FEATURE()
 * when the feature is disabled.
 *
 * A feature following the conventions may be used without knowing if it is
 * enabled or not.
 */

#ifndef FEATURE_ES1
#define FEATURE_ES1 0
#endif
#ifndef FEATURE_ES2
#define FEATURE_ES2 0
#endif

#define FEATURE_ES (FEATURE_ES1 || FEATURE_ES2)

#ifndef FEATURE_GL
#define FEATURE_GL !FEATURE_ES
#endif

#if defined(IN_DRI_DRIVER) || (FEATURE_GL + FEATURE_ES1 + FEATURE_ES2 > 1)
#define FEATURE_remap_table               1
#else
#define FEATURE_remap_table               0
#endif

#define FEATURE_dispatch                  1
#define FEATURE_texgen                    1
#define FEATURE_userclip                  1

#define FEATURE_accum                     FEATURE_GL
#define FEATURE_arrayelt                  FEATURE_GL
#define FEATURE_attrib_stack              FEATURE_GL
/* this disables vtxfmt, api_loopback, and api_noop completely */
#define FEATURE_beginend                  FEATURE_GL
#define FEATURE_colortable                FEATURE_GL
#define FEATURE_convolve                  FEATURE_GL
#define FEATURE_dlist                     (FEATURE_GL && FEATURE_arrayelt && FEATURE_beginend)
#define FEATURE_draw_read_buffer          FEATURE_GL
#define FEATURE_drawpix                   FEATURE_GL
#define FEATURE_evaluators                FEATURE_GL
#define FEATURE_feedback                  FEATURE_GL
#define FEATURE_pixel_transfer            FEATURE_GL
#define FEATURE_queryobj                  FEATURE_GL
#define FEATURE_rastpos                   FEATURE_GL
#define FEATURE_texture_fxt1              FEATURE_GL
#define FEATURE_texture_s3tc              FEATURE_GL

#define FEATURE_extra_context_init        FEATURE_ES
#define FEATURE_point_size_array          FEATURE_ES

#define FEATURE_es2_glsl                  FEATURE_ES2

#define FEATURE_ARB_fragment_program      1
#define FEATURE_ARB_vertex_program        1
#define FEATURE_ARB_vertex_shader         1
#define FEATURE_ARB_fragment_shader       1
#define FEATURE_ARB_shader_objects        (FEATURE_ARB_vertex_shader || FEATURE_ARB_fragment_shader)
#define FEATURE_ARB_shading_language_100  FEATURE_ARB_shader_objects
#define FEATURE_ARB_geometry_shader4      FEATURE_ARB_shader_objects

#define FEATURE_ARB_framebuffer_object    (FEATURE_GL && FEATURE_EXT_framebuffer_object)
#define FEATURE_ARB_map_buffer_range      FEATURE_GL
#define FEATURE_ARB_pixel_buffer_object   (FEATURE_GL && FEATURE_EXT_pixel_buffer_object)
#define FEATURE_ARB_sampler_objects       FEATURE_GL
#define FEATURE_ARB_sync                  FEATURE_GL

#define FEATURE_EXT_framebuffer_blit      FEATURE_GL
#define FEATURE_EXT_framebuffer_object    1
#define FEATURE_EXT_pixel_buffer_object   1
#define FEATURE_EXT_texture_sRGB          FEATURE_GL
#define FEATURE_EXT_transform_feedback    FEATURE_GL

#define FEATURE_APPLE_object_purgeable    FEATURE_GL
#define FEATURE_ATI_fragment_shader       FEATURE_GL
#define FEATURE_NV_fence                  FEATURE_GL
#define FEATURE_NV_fragment_program       FEATURE_GL
#define FEATURE_NV_vertex_program         FEATURE_GL

#define FEATURE_OES_EGL_image             1
#define FEATURE_OES_draw_texture          FEATURE_ES1
#define FEATURE_OES_framebuffer_object    FEATURE_ES
#define FEATURE_OES_mapbuffer             FEATURE_ES

#endif /* FEATURES_H */
