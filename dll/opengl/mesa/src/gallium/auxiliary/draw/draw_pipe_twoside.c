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
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "draw_vs.h"
#include "draw_pipe.h"

struct twoside_stage {
   struct draw_stage stage;
   float sign;         /**< +1 or -1 */
   int attrib_front0, attrib_back0;
   int attrib_front1, attrib_back1;
};


static INLINE struct twoside_stage *twoside_stage( struct draw_stage *stage )
{
   return (struct twoside_stage *)stage;
}

/**
 * Copy back color(s) to front color(s).
 */
static INLINE struct vertex_header *
copy_bfc( struct twoside_stage *twoside, 
          const struct vertex_header *v,
          unsigned idx )
{   
   struct vertex_header *tmp = dup_vert( &twoside->stage, v, idx );

   if (twoside->attrib_back0 >= 0 && twoside->attrib_front0 >= 0) {
      COPY_4FV(tmp->data[twoside->attrib_front0],
               tmp->data[twoside->attrib_back0]);
   }
   if (twoside->attrib_back1 >= 0 && twoside->attrib_front1 >= 0) {
      COPY_4FV(tmp->data[twoside->attrib_front1],
               tmp->data[twoside->attrib_back1]);
   }

   return tmp;
}


/* Twoside tri:
 */
static void twoside_tri( struct draw_stage *stage,
			 struct prim_header *header )
{
   struct twoside_stage *twoside = twoside_stage(stage);

   if (header->det * twoside->sign < 0.0) {
      /* this is a back-facing triangle */
      struct prim_header tmp;

      tmp.det = header->det;
      tmp.flags = header->flags;
      tmp.pad = header->pad;
      /* copy back attribs to front attribs */
      tmp.v[0] = copy_bfc(twoside, header->v[0], 0);
      tmp.v[1] = copy_bfc(twoside, header->v[1], 1);
      tmp.v[2] = copy_bfc(twoside, header->v[2], 2);

      stage->next->tri( stage->next, &tmp );
   }
   else {
      stage->next->tri( stage->next, header );
   }
}



static void twoside_first_tri( struct draw_stage *stage, 
			       struct prim_header *header )
{
   struct twoside_stage *twoside = twoside_stage(stage);
   const struct draw_vertex_shader *vs = stage->draw->vs.vertex_shader;
   uint i;

   twoside->attrib_front0 = -1;
   twoside->attrib_front1 = -1;
   twoside->attrib_back0 = -1;
   twoside->attrib_back1 = -1;

   /* Find which vertex shader outputs are front/back colors */
   for (i = 0; i < vs->info.num_outputs; i++) {
      if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_COLOR) {
         if (vs->info.output_semantic_index[i] == 0)
            twoside->attrib_front0 = i;
         else
            twoside->attrib_front1 = i;
      }
      if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_BCOLOR) {
         if (vs->info.output_semantic_index[i] == 0)
            twoside->attrib_back0 = i;
         else
            twoside->attrib_back1 = i;
      }
   }

   /*
    * We'll multiply the primitive's determinant by this sign to determine
    * if the triangle is back-facing (negative).
    * sign = -1 for CCW, +1 for CW
    */
   twoside->sign = stage->draw->rasterizer->front_ccw ? -1.0f : 1.0f;

   stage->tri = twoside_tri;
   stage->tri( stage, header );
}


static void twoside_flush( struct draw_stage *stage, unsigned flags )
{
   stage->tri = twoside_first_tri;
   stage->next->flush( stage->next, flags );
}


static void twoside_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void twoside_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


/**
 * Create twoside pipeline stage.
 */
struct draw_stage *draw_twoside_stage( struct draw_context *draw )
{
   struct twoside_stage *twoside = CALLOC_STRUCT(twoside_stage);
   if (twoside == NULL)
      goto fail;

   twoside->stage.draw = draw;
   twoside->stage.name = "twoside";
   twoside->stage.next = NULL;
   twoside->stage.point = draw_pipe_passthrough_point;
   twoside->stage.line = draw_pipe_passthrough_line;
   twoside->stage.tri = twoside_first_tri;
   twoside->stage.flush = twoside_flush;
   twoside->stage.reset_stipple_counter = twoside_reset_stipple_counter;
   twoside->stage.destroy = twoside_destroy;

   if (!draw_alloc_temp_verts( &twoside->stage, 3 ))
      goto fail;

   return &twoside->stage;

 fail:
   if (twoside)
      twoside->stage.destroy( &twoside->stage );

   return NULL;
}
