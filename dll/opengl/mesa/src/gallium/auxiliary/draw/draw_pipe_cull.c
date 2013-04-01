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

/**
 * \brief  Drawing stage for polygon culling
 */

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */


#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "draw_pipe.h"


struct cull_stage {
   struct draw_stage stage;
   unsigned cull_face;  /**< which face(s) to cull (one of PIPE_FACE_x) */
   unsigned front_ccw;
};


static INLINE struct cull_stage *cull_stage( struct draw_stage *stage )
{
   return (struct cull_stage *)stage;
}


static void cull_tri( struct draw_stage *stage,
		      struct prim_header *header )
{
   const unsigned pos = draw_current_shader_position_output(stage->draw);

   /* Window coords: */
   const float *v0 = header->v[0]->data[pos];
   const float *v1 = header->v[1]->data[pos];
   const float *v2 = header->v[2]->data[pos];

   /* edge vectors: e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0] - v2[0];
   const float ey = v0[1] - v2[1];
   const float fx = v1[0] - v2[0];
   const float fy = v1[1] - v2[1];
   
   /* det = cross(e,f).z */
   header->det = ex * fy - ey * fx;

   if (header->det != 0) {
      /* if det < 0 then Z points toward the camera and the triangle is 
       * counter-clockwise winding.
       */
      unsigned ccw = (header->det < 0);
      unsigned face = ((ccw == cull_stage(stage)->front_ccw) ?
                       PIPE_FACE_FRONT :
                       PIPE_FACE_BACK);

      if ((face & cull_stage(stage)->cull_face) == 0) {
         /* triangle is not culled, pass to next stage */
	 stage->next->tri( stage->next, header );
      }
   }
}


static void cull_first_tri( struct draw_stage *stage, 
			    struct prim_header *header )
{
   struct cull_stage *cull = cull_stage(stage);

   cull->cull_face = stage->draw->rasterizer->cull_face;
   cull->front_ccw = stage->draw->rasterizer->front_ccw;

   stage->tri = cull_tri;
   stage->tri( stage, header );
}


static void cull_flush( struct draw_stage *stage, unsigned flags )
{
   stage->tri = cull_first_tri;
   stage->next->flush( stage->next, flags );
}


static void cull_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void cull_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


/**
 * Create a new polygon culling stage.
 */
struct draw_stage *draw_cull_stage( struct draw_context *draw )
{
   struct cull_stage *cull = CALLOC_STRUCT(cull_stage);
   if (cull == NULL)
      goto fail;

   cull->stage.draw = draw;
   cull->stage.name = "cull";
   cull->stage.next = NULL;
   cull->stage.point = draw_pipe_passthrough_point;
   cull->stage.line = draw_pipe_passthrough_line;
   cull->stage.tri = cull_first_tri;
   cull->stage.flush = cull_flush;
   cull->stage.reset_stipple_counter = cull_reset_stipple_counter;
   cull->stage.destroy = cull_destroy;

   if (!draw_alloc_temp_verts( &cull->stage, 0 ))
      goto fail;

   return &cull->stage;

fail:
   if (cull)
      cull->stage.destroy( &cull->stage );

   return NULL;
}
