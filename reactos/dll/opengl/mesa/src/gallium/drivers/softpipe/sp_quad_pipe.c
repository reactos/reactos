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


#include "sp_context.h"
#include "sp_state.h"
#include "pipe/p_shader_tokens.h"


static void
insert_stage_at_head(struct softpipe_context *sp, struct quad_stage *quad)
{
   quad->next = sp->quad.first;
   sp->quad.first = quad;
}


void
sp_build_quad_pipeline(struct softpipe_context *sp)
{
   boolean early_depth_test =
      sp->depth_stencil->depth.enabled &&
      sp->framebuffer.zsbuf &&
      !sp->depth_stencil->alpha.enabled &&
      !sp->fs_variant->info.uses_kill &&
      !sp->fs_variant->info.writes_z &&
      !sp->fs_variant->info.writes_stencil;

   sp->quad.first = sp->quad.blend;

   if (early_depth_test) {
      insert_stage_at_head( sp, sp->quad.shade );
      insert_stage_at_head( sp, sp->quad.depth_test );
   }
   else {
      insert_stage_at_head( sp, sp->quad.depth_test );
      insert_stage_at_head( sp, sp->quad.shade );
   }

#if !DO_PSTIPPLE_IN_DRAW_MODULE && !DO_PSTIPPLE_IN_HELPER_MODULE
   if (sp->rasterizer->poly_stipple_enable)
      insert_stage_at_head( sp, sp->quad.pstipple );
#endif
}

