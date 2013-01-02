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


#include "main/context.h"
#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"

/**
 * Update the viewport transformation matrix.  Depends on:
 *  - viewport pos/size
 *  - depthrange
 *  - window pos/size or FBO size
 */
static void
update_viewport( struct st_context *st )
{
   struct gl_context *ctx = st->ctx;
   GLfloat yScale, yBias;

   /* _NEW_BUFFERS
    */
   if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
      /* Drawing to a window.  The corresponding gallium surface uses
       * Y=0=TOP but OpenGL is Y=0=BOTTOM.  So we need to invert the viewport.
       */
      yScale = -1;
      yBias = (GLfloat)ctx->DrawBuffer->Height;
   }
   else {
      /* Drawing to an FBO where Y=0=BOTTOM, like OpenGL - don't invert */
      yScale = 1.0;
      yBias = 0.0;
   }

   /* _NEW_VIEWPORT 
    */
   {
      GLfloat x = (GLfloat)ctx->Viewport.X;
      GLfloat y = (GLfloat)ctx->Viewport.Y;
      GLfloat z = ctx->Viewport.Near;
      GLfloat half_width = (GLfloat)ctx->Viewport.Width * 0.5f;
      GLfloat half_height = (GLfloat)ctx->Viewport.Height * 0.5f;
      GLfloat half_depth = (GLfloat)(ctx->Viewport.Far - ctx->Viewport.Near) * 0.5f;
      
      st->state.viewport.scale[0] = half_width;
      st->state.viewport.scale[1] = half_height * yScale;
      st->state.viewport.scale[2] = half_depth;
      st->state.viewport.scale[3] = 1.0;

      st->state.viewport.translate[0] = half_width + x;
      st->state.viewport.translate[1] = (half_height + y) * yScale + yBias;
      st->state.viewport.translate[2] = half_depth + z;
      st->state.viewport.translate[3] = 0.0;

      cso_set_viewport(st->cso_context, &st->state.viewport);
   }
}


const struct st_tracked_state st_update_viewport = {
   "st_update_viewport",				/* name */
   {							/* dirty */
      _NEW_BUFFERS | _NEW_VIEWPORT,			/* mesa */
      0,						/* st */
   },
   update_viewport					/* update */
};
