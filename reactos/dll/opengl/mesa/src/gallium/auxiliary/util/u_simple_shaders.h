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


#ifndef U_SIMPLE_SHADERS_H
#define U_SIMPLE_SHADERS_H


#include "pipe/p_compiler.h"


struct pipe_context;
struct pipe_shader_state;
struct pipe_stream_output_info;


#ifdef __cplusplus
extern "C" {
#endif


extern void *
util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes);

extern void *
util_make_vertex_passthrough_shader_with_so(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes,
                                    const struct pipe_stream_output_info *so);


extern void *
util_make_fragment_tex_shader_writemask(struct pipe_context *pipe, 
                                        unsigned tex_target,
                                        unsigned interp_mode,
                                        unsigned writemask);

extern void *
util_make_fragment_tex_shader(struct pipe_context *pipe, unsigned tex_target,
                              unsigned interp_mode);


extern void *
util_make_fragment_tex_shader_writedepth(struct pipe_context *pipe,
                                         unsigned tex_target,
                                         unsigned interp_mode);


extern void *
util_make_fragment_passthrough_shader(struct pipe_context *pipe);


extern void *
util_make_fragment_cloneinput_shader(struct pipe_context *pipe, int num_cbufs,
                                     int input_semantic,
                                     int input_interpolate);

#ifdef __cplusplus
}
#endif


#endif
