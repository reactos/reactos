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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 

#include "main/macros.h"
#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"


/**
 * Scissor depends on the scissor box, and the framebuffer dimensions.
 */
static void
update_scissor( struct st_context *st )
{
   struct pipe_scissor_state scissor;
   const struct gl_context *ctx = st->ctx;
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLint miny, maxy;

   scissor.minx = 0;
   scissor.miny = 0;
   scissor.maxx = fb->Width;
   scissor.maxy = fb->Height;

   if (ctx->Scissor.Enabled) {
      /* need to be careful here with xmax or ymax < 0 */
      GLint xmax = MAX2(0, ctx->Scissor.X + ctx->Scissor.Width);
      GLint ymax = MAX2(0, ctx->Scissor.Y + ctx->Scissor.Height);

      if (ctx->Scissor.X > (GLint)scissor.minx)
         scissor.minx = ctx->Scissor.X;
      if (ctx->Scissor.Y > (GLint)scissor.miny)
         scissor.miny = ctx->Scissor.Y;

      if (xmax < (GLint) scissor.maxx)
         scissor.maxx = xmax;
      if (ymax < (GLint) scissor.maxy)
         scissor.maxy = ymax;

      /* check for null space */
      if (scissor.minx >= scissor.maxx || scissor.miny >= scissor.maxy)
         scissor.minx = scissor.miny = scissor.maxx = scissor.maxy = 0;
   }

   /* Now invert Y if needed.
    * Gallium drivers use the convention Y=0=top for surfaces.
    */
   if (st_fb_orientation(fb) == Y_0_TOP) {
      miny = fb->Height - scissor.maxy;
      maxy = fb->Height - scissor.miny;
      scissor.miny = miny;
      scissor.maxy = maxy;
   }

   if (memcmp(&scissor, &st->state.scissor, sizeof(scissor)) != 0) {
      /* state has changed */
      st->state.scissor = scissor;  /* struct copy */
      st->pipe->set_scissor_state(st->pipe, &scissor); /* activate */
   }
}


const struct st_tracked_state st_update_scissor = {
   "st_update_scissor",					/* name */
   {							/* dirty */
      (_NEW_SCISSOR | _NEW_BUFFERS),			/* mesa */
      0,						/* st */
   },
   update_scissor					/* update */
};
