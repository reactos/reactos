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
  * \brief polygon stipple state
  *
  * Authors:
  *   Brian Paul
  */
 

#include <assert.h>

#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/**
 * OpenGL's polygon stipple is indexed with window coordinates in which
 * the origin (0,0) is the lower-left corner of the window.
 * With Gallium, the origin is the upper-left corner of the window.
 * To convert GL's polygon stipple to what gallium expects we need to
 * invert the pattern vertically and rotate the stipple rows according
 * to the window height.
 */
static void
invert_stipple(GLuint dest[32], const GLuint src[32], GLuint winHeight)
{
   GLuint i;

   for (i = 0; i < 32; i++) {
      dest[i] = src[(winHeight - 1 - i) & 0x1f];
   }
}



static void 
update_stipple( struct st_context *st )
{
   const struct gl_context *ctx = st->ctx;
   const GLuint sz = sizeof(st->state.poly_stipple);
   assert(sz == sizeof(ctx->PolygonStipple));

   if (memcmp(st->state.poly_stipple, ctx->PolygonStipple, sz)) {
      /* state has changed */
      struct pipe_poly_stipple newStipple;

      memcpy(st->state.poly_stipple, ctx->PolygonStipple, sz);

      invert_stipple(newStipple.stipple, ctx->PolygonStipple,
                     ctx->DrawBuffer->Height);

      st->pipe->set_polygon_stipple(st->pipe, &newStipple);
   }
}


/** Update the stipple when the pattern or window height changes */
const struct st_tracked_state st_update_polygon_stipple = {
   "st_update_polygon_stipple",				/* name */
   {							/* dirty */
      (_NEW_POLYGONSTIPPLE |
       _NEW_BUFFERS),					/* mesa */
      0,						/* st */
   },
   update_stipple					/* update */
};
