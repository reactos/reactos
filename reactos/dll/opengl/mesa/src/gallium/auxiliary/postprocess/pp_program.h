/**************************************************************************
 *
 * Copyright 2010 Jakob Bornecrantz
 * Copyright 2011 Lauri Kasanen
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
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef PP_PROGRAM_H
#define PP_PROGRAM_H

#include "pipe/p_state.h"

/**
*	Internal control details.
*/
struct program
{
   struct pipe_screen *screen;
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;   /* bilinear */
   struct pipe_sampler_state sampler_point;     /* point */
   struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state framebuffer;
   struct pipe_vertex_element velem[2];

   union pipe_color_union clear_color;

   void *passvs;

   struct pipe_resource *vbuf;
   struct pipe_surface surf;
   struct pipe_sampler_view *view;

   struct blit_state *blitctx;
};


#endif
