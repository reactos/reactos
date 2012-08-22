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

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_state.h"
#include "draw/draw_context.h"



static void *
softpipe_create_rasterizer_state(struct pipe_context *pipe,
                                 const struct pipe_rasterizer_state *rast)
{
   return mem_dup(rast, sizeof(*rast));
}


static void
softpipe_bind_rasterizer_state(struct pipe_context *pipe,
                               void *rasterizer)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   if (softpipe->rasterizer == rasterizer)
      return;

   /* pass-through to draw module */
   draw_set_rasterizer_state(softpipe->draw, rasterizer, rasterizer);

   softpipe->rasterizer = rasterizer;

   softpipe->dirty |= SP_NEW_RASTERIZER;
}


static void
softpipe_delete_rasterizer_state(struct pipe_context *pipe,
                                 void *rasterizer)
{
   FREE( rasterizer );
}


void
softpipe_init_rasterizer_funcs(struct pipe_context *pipe)
{
   pipe->create_rasterizer_state = softpipe_create_rasterizer_state;
   pipe->bind_rasterizer_state   = softpipe_bind_rasterizer_state;
   pipe->delete_rasterizer_state = softpipe_delete_rasterizer_state;
}
