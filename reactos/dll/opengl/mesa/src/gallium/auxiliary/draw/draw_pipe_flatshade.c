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

#include "pipe/p_shader_tokens.h"
#include "draw_vs.h"
#include "draw_pipe.h"


/** subclass of draw_stage */
struct flat_stage
{
   struct draw_stage stage;

   uint num_color_attribs;
   uint color_attribs[2];  /* front/back primary colors */

   uint num_spec_attribs;
   uint spec_attribs[2];  /* front/back secondary colors */
};

#define COPY_3FV( DST, SRC )         \
do {                                \
   (DST)[0] = (SRC)[0];             \
   (DST)[1] = (SRC)[1];             \
   (DST)[2] = (SRC)[2];             \
} while (0)


static INLINE struct flat_stage *
flat_stage(struct draw_stage *stage)
{
   return (struct flat_stage *) stage;
}


/** Copy all the color attributes from 'src' vertex to 'dst' vertex */
static INLINE void copy_colors( struct draw_stage *stage,
                                struct vertex_header *dst,
                                const struct vertex_header *src )
{
   const struct flat_stage *flat = flat_stage(stage);
   uint i;

   for (i = 0; i < flat->num_color_attribs; i++) {
      const uint attr = flat->color_attribs[i];
      COPY_4FV(dst->data[attr], src->data[attr]);
   }

   for (i = 0; i < flat->num_spec_attribs; i++) {
      const uint attr = flat->spec_attribs[i];
      COPY_3FV(dst->data[attr], src->data[attr]);
   }
}


/** Copy all the color attributes from src vertex to dst0 & dst1 vertices */
static INLINE void copy_colors2( struct draw_stage *stage,
                                 struct vertex_header *dst0,
                                 struct vertex_header *dst1,
                                 const struct vertex_header *src )
{
   const struct flat_stage *flat = flat_stage(stage);
   uint i;
   for (i = 0; i < flat->num_color_attribs; i++) {
      const uint attr = flat->color_attribs[i];
      COPY_4FV(dst0->data[attr], src->data[attr]);
      COPY_4FV(dst1->data[attr], src->data[attr]);
   }

   for (i = 0; i < flat->num_spec_attribs; i++) {
      const uint attr = flat->spec_attribs[i];
      COPY_3FV(dst0->data[attr], src->data[attr]);
      COPY_3FV(dst1->data[attr], src->data[attr]);
   }
}


/**
 * Flatshade tri.  Required for clipping and when unfilled tris are
 * active, otherwise handled by hardware.
 */
static void flatshade_tri_0( struct draw_stage *stage,
                             struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
   tmp.flags = header->flags;
   tmp.pad = header->pad;
   tmp.v[0] = header->v[0];
   tmp.v[1] = dup_vert(stage, header->v[1], 0);
   tmp.v[2] = dup_vert(stage, header->v[2], 1);

   copy_colors2(stage, tmp.v[1], tmp.v[2], tmp.v[0]);
   
   stage->next->tri( stage->next, &tmp );
}


static void flatshade_tri_2( struct draw_stage *stage,
                             struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
   tmp.flags = header->flags;
   tmp.pad = header->pad;
   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = dup_vert(stage, header->v[1], 1);
   tmp.v[2] = header->v[2];

   copy_colors2(stage, tmp.v[0], tmp.v[1], tmp.v[2]);
   
   stage->next->tri( stage->next, &tmp );
}





/**
 * Flatshade line.  Required for clipping.
 */
static void flatshade_line_0( struct draw_stage *stage,
                              struct prim_header *header )
{
   struct prim_header tmp;

   tmp.v[0] = header->v[0];
   tmp.v[1] = dup_vert(stage, header->v[1], 0);

   copy_colors(stage, tmp.v[1], tmp.v[0]);
   
   stage->next->line( stage->next, &tmp );
}

static void flatshade_line_1( struct draw_stage *stage,
                              struct prim_header *header )
{
   struct prim_header tmp;

   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = header->v[1];

   copy_colors(stage, tmp.v[0], tmp.v[1]);
   
   stage->next->line( stage->next, &tmp );
}




static void flatshade_init_state( struct draw_stage *stage )
{
   struct flat_stage *flat = flat_stage(stage);
   const struct draw_vertex_shader *vs = stage->draw->vs.vertex_shader;
   uint i;

   /* Find which vertex shader outputs are colors, make a list */
   flat->num_color_attribs = 0;
   flat->num_spec_attribs = 0;
   for (i = 0; i < vs->info.num_outputs; i++) {
      if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_COLOR ||
          vs->info.output_semantic_name[i] == TGSI_SEMANTIC_BCOLOR) {
         if (vs->info.output_semantic_index[i] == 0)
            flat->color_attribs[flat->num_color_attribs++] = i;
         else
            flat->spec_attribs[flat->num_spec_attribs++] = i;
      }
   }

   /* Choose flatshade routine according to provoking vertex:
    */
   if (stage->draw->rasterizer->flatshade_first) {
      stage->line = flatshade_line_0;
      stage->tri = flatshade_tri_0;
   }
   else {
      stage->line = flatshade_line_1;
      stage->tri = flatshade_tri_2;
   }
}

static void flatshade_first_tri( struct draw_stage *stage,
				 struct prim_header *header )
{
   flatshade_init_state( stage );
   stage->tri( stage, header );
}

static void flatshade_first_line( struct draw_stage *stage,
				  struct prim_header *header )
{
   flatshade_init_state( stage );
   stage->line( stage, header );
}


static void flatshade_flush( struct draw_stage *stage, 
			     unsigned flags )
{
   stage->tri = flatshade_first_tri;
   stage->line = flatshade_first_line;
   stage->next->flush( stage->next, flags );
}


static void flatshade_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void flatshade_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


/**
 * Create flatshading drawing stage.
 */
struct draw_stage *draw_flatshade_stage( struct draw_context *draw )
{
   struct flat_stage *flatshade = CALLOC_STRUCT(flat_stage);
   if (flatshade == NULL)
      goto fail;

   flatshade->stage.draw = draw;
   flatshade->stage.name = "flatshade";
   flatshade->stage.next = NULL;
   flatshade->stage.point = draw_pipe_passthrough_point;
   flatshade->stage.line = flatshade_first_line;
   flatshade->stage.tri = flatshade_first_tri;
   flatshade->stage.flush = flatshade_flush;
   flatshade->stage.reset_stipple_counter = flatshade_reset_stipple_counter;
   flatshade->stage.destroy = flatshade_destroy;

   if (!draw_alloc_temp_verts( &flatshade->stage, 2 ))
      goto fail;

   return &flatshade->stage;

 fail:
   if (flatshade)
      flatshade->stage.destroy( &flatshade->stage );

   return NULL;
}


