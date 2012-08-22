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
 

#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"
#include "st_program.h"
#include "util/u_debug.h"
#include "cso_cache/cso_context.h"


/* Second state atom for user clip planes:
 */
static void update_clip( struct st_context *st )
{
   struct pipe_clip_state clip;
   const struct gl_context *ctx = st->ctx;
   bool use_eye = FALSE;

   assert(sizeof(clip.ucp) <= sizeof(ctx->Transform._ClipUserPlane));

   /* if we have a vertex shader that writes clip vertex we need to pass
      the pre-projection transformed coordinates into the driver. */
   if (st->vp) {
      if (ctx->Shader.CurrentVertexProgram)
         use_eye = TRUE;
   }

   memcpy(clip.ucp,
          use_eye ? ctx->Transform.EyeUserPlane
                  : ctx->Transform._ClipUserPlane, sizeof(clip.ucp));
   st->state.clip = clip;
   cso_set_clip(st->cso_context, &clip);
}


const struct st_tracked_state st_update_clip = {
   "st_update_clip",					/* name */
   {							/* dirty */
      (_NEW_TRANSFORM | _NEW_PROGRAM),			/* mesa */
      0,						/* st */
   },
   update_clip						/* update */
};
