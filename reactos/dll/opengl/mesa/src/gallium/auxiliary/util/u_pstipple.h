/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2010 VMware, Inc.
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

#ifndef U_PSTIPPLE_H
#define U_PSTIPPLE_H

#include "pipe/p_compiler.h"

struct pipe_context;
struct pipe_resource;
struct pipe_shader_state;


extern struct pipe_resource *
util_pstipple_create_stipple_texture(struct pipe_context *pipe,
                                     const uint32_t pattern[32]);

extern struct pipe_sampler_view *
util_pstipple_create_sampler_view(struct pipe_context *pipe,
                                  struct pipe_resource *tex);

extern void *
util_pstipple_create_sampler(struct pipe_context *pipe);

extern struct pipe_shader_state *
util_pstipple_create_fragment_shader(struct pipe_context *pipe,
                                     struct pipe_shader_state *fs,
                                     unsigned *samplerUnitOut);


#endif
