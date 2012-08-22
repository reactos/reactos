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

#ifndef U_GENMIPMAP_H
#define U_GENMIPMAP_H

#include "pipe/p_state.h"


#ifdef __cplusplus
extern "C" {
#endif

   
struct pipe_context;
struct pipe_resource;
struct cso_context;

struct gen_mipmap_state;


extern struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe, struct cso_context *cso);


extern void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx);

/* Release vertex buffer at end of frame to avoid synchronous
 * rendering.
 */
extern void 
util_gen_mipmap_flush( struct gen_mipmap_state *ctx );


extern void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_sampler_view *psv,
                uint layer, uint baseLevel, uint lastLevel, uint filter);


#ifdef __cplusplus
}
#endif

#endif
