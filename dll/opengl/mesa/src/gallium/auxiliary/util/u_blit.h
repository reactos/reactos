/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_BLIT_H
#define U_BLIT_H


#include "pipe/p_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif

   
struct cso_context;
struct pipe_context;
struct pipe_resource;
struct pipe_sampler_view;
struct pipe_surface;


extern struct blit_state *
util_create_blit(struct pipe_context *pipe, struct cso_context *cso);

extern void
util_destroy_blit(struct blit_state *ctx);

extern void
util_blit_pixels(struct blit_state *ctx,
                 struct pipe_resource *src_tex,
                 unsigned src_level,
                 int srcX0, int srcY0,
                 int srcX1, int srcY1,
                 int srcZ0,
                 struct pipe_surface *dst,
                 int dstX0, int dstY0,
                 int dstX1, int dstY1,
                 float z, uint filter);

void
util_blit_pixels_writemask(struct blit_state *ctx,
                           struct pipe_resource *src_tex,
                           unsigned src_level,
                           int srcX0, int srcY0,
                           int srcX1, int srcY1,
                           int srcZ0,
                           struct pipe_surface *dst,
                           int dstX0, int dstY0,
                           int dstX1, int dstY1,
                           float z, uint filter,
                           uint writemask);

extern void
util_blit_pixels_tex(struct blit_state *ctx,
                     struct pipe_sampler_view *src_sampler_view,
                     int srcX0, int srcY0,
                     int srcX1, int srcY1,
                     struct pipe_surface *dst,
                     int dstX0, int dstY0,
                     int dstX1, int dstY1,
                     float z, uint filter);

/* Call at end of frame to avoid synchronous rendering.
 */
extern void
util_blit_flush( struct blit_state *ctx );

#ifdef __cplusplus
}
#endif

#endif
