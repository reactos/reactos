/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"
#include "sp_context.h"
#include "sp_state.h"


static void *
softpipe_create_blend_state(struct pipe_context *pipe,
                            const struct pipe_blend_state *blend)
{
   return mem_dup(blend, sizeof(*blend));
}


static void
softpipe_bind_blend_state(struct pipe_context *pipe,
                          void *blend)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   draw_flush(softpipe->draw);

   softpipe->blend = (struct pipe_blend_state *)blend;

   softpipe->dirty |= SP_NEW_BLEND;
}


static void
softpipe_delete_blend_state(struct pipe_context *pipe,
                            void *blend)
{
   FREE( blend );
}


static void
softpipe_set_blend_color(struct pipe_context *pipe,
                         const struct pipe_blend_color *blend_color)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   unsigned i;

   draw_flush(softpipe->draw);

   softpipe->blend_color = *blend_color;

   /* save clamped color too */
   for (i = 0; i < 4; i++)
      softpipe->blend_color_clamped.color[i] =
         CLAMP(blend_color->color[i], 0.0f, 1.0f);

   softpipe->dirty |= SP_NEW_BLEND;
}


static void *
softpipe_create_depth_stencil_state(struct pipe_context *pipe,
                                    const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   return mem_dup(depth_stencil, sizeof(*depth_stencil));
}


static void
softpipe_bind_depth_stencil_state(struct pipe_context *pipe,
                                  void *depth_stencil)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->depth_stencil = (struct pipe_depth_stencil_alpha_state *)depth_stencil;

   softpipe->dirty |= SP_NEW_DEPTH_STENCIL_ALPHA;
}


static void
softpipe_delete_depth_stencil_state(struct pipe_context *pipe, void *depth)
{
   FREE( depth );
}


static void
softpipe_set_stencil_ref(struct pipe_context *pipe,
                         const struct pipe_stencil_ref *stencil_ref)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->stencil_ref = *stencil_ref;

   softpipe->dirty |= SP_NEW_DEPTH_STENCIL_ALPHA;
}


static void
softpipe_set_sample_mask(struct pipe_context *pipe,
                         unsigned sample_mask)
{
}


void
softpipe_init_blend_funcs(struct pipe_context *pipe)
{
   pipe->create_blend_state = softpipe_create_blend_state;
   pipe->bind_blend_state   = softpipe_bind_blend_state;
   pipe->delete_blend_state = softpipe_delete_blend_state;

   pipe->set_blend_color = softpipe_set_blend_color;

   pipe->create_depth_stencil_alpha_state = softpipe_create_depth_stencil_state;
   pipe->bind_depth_stencil_alpha_state   = softpipe_bind_depth_stencil_state;
   pipe->delete_depth_stencil_alpha_state = softpipe_delete_depth_stencil_state;

   pipe->set_stencil_ref = softpipe_set_stencil_ref;

   pipe->set_sample_mask = softpipe_set_sample_mask;
}
