/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_SURFACE_H
#define U_SURFACE_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"


#ifdef __cplusplus
extern "C" {
#endif


extern void
u_surface_default_template(struct pipe_surface *view,
                           const struct pipe_resource *texture,
                           unsigned bind);

extern boolean
util_create_rgba_surface(struct pipe_context *ctx,
                         uint width, uint height, uint bind,
                         struct pipe_resource **textureOut,
                         struct pipe_surface **surfaceOut);


extern void
util_destroy_rgba_surface(struct pipe_resource *texture,
                          struct pipe_surface *surface);



extern void
util_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst,
                          unsigned dst_level,
                          unsigned dst_x, unsigned dst_y, unsigned dst_z,
                          struct pipe_resource *src,
                          unsigned src_level,
                          const struct pipe_box *src_box);

extern void
util_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const union pipe_color_union *color,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height);

extern void
util_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height);


#ifdef __cplusplus
}
#endif


#endif /* U_SURFACE_H */
