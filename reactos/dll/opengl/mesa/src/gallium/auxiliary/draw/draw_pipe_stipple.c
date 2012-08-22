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

/* Implement line stipple by cutting lines up into smaller lines.
 * There are hundreds of ways to implement line stipple, this is one
 * choice that should work in all situations, requires no state
 * manipulations, but with a penalty in terms of large amounts of
 * generated geometry.
 */


#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "draw/draw_pipe.h"


/** Subclass of draw_stage */
struct stipple_stage {
   struct draw_stage stage;
   float counter;
   uint pattern;
   uint factor;
};


static INLINE struct stipple_stage *
stipple_stage(struct draw_stage *stage)
{
   return (struct stipple_stage *) stage;
}


/**
 * Compute interpolated vertex attributes for 'dst' at position 't' 
 * between 'v0' and 'v1'.
 * XXX using linear interpolation for all attribs at this time.
 */
static void
screen_interp( struct draw_context *draw,
               struct vertex_header *dst,
               float t,
               const struct vertex_header *v0, 
               const struct vertex_header *v1 )
{
   uint attr;
   int num_outputs = draw_current_shader_outputs(draw);
   for (attr = 0; attr < num_outputs; attr++) {
      const float *val0 = v0->data[attr];
      const float *val1 = v1->data[attr];
      float *newv = dst->data[attr];
      uint i;
      for (i = 0; i < 4; i++) {
         newv[i] = val0[i] + t * (val1[i] - val0[i]);
      }
   }
}


static void
emit_segment(struct draw_stage *stage, struct prim_header *header,
             float t0, float t1)
{
   struct vertex_header *v0new = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1new = dup_vert(stage, header->v[1], 1);
   struct prim_header newprim = *header;

   if (t0 > 0.0) {
      screen_interp( stage->draw, v0new, t0, header->v[0], header->v[1] );
      newprim.v[0] = v0new;
   }

   if (t1 < 1.0) {
      screen_interp( stage->draw, v1new, t1, header->v[0], header->v[1] );
      newprim.v[1] = v1new;
   }

   stage->next->line( stage->next, &newprim );
}


static INLINE unsigned
stipple_test(int counter, ushort pattern, int factor)
{
   int b = (counter / factor) & 0xf;
   return (1 << b) & pattern;
}


static void
stipple_line(struct draw_stage *stage, struct prim_header *header)
{
   struct stipple_stage *stipple = stipple_stage(stage);
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   const unsigned pos = draw_current_shader_position_output(stage->draw);
   const float *pos0 = v0->data[pos];
   const float *pos1 = v1->data[pos];
   float start = 0;
   int state = 0;

   float x0 = pos0[0];
   float x1 = pos1[0];
   float y0 = pos0[1];
   float y1 = pos1[1];

   float dx = x0 > x1 ? x0 - x1 : x1 - x0;
   float dy = y0 > y1 ? y0 - y1 : y1 - y0;

   float length = MAX2(dx, dy);
   int i;

   if (header->flags & DRAW_PIPE_RESET_STIPPLE)
      stipple->counter = 0;


   /* XXX ToDo: intead of iterating pixel-by-pixel, use a look-up table.
    */
   for (i = 0; i < length; i++) {
      int result = stipple_test( (int) stipple->counter+i,
                                 (ushort) stipple->pattern, stipple->factor );
      if (result != state) {
         /* changing from "off" to "on" or vice versa */
	 if (state) {
	    if (start != i) {
               /* finishing an "on" segment */
	       emit_segment( stage, header, start / length, i / length );
            }
	 }
	 else {
            /* starting an "on" segment */
	    start = (float) i;
	 }
	 state = result;	   
      }
   }

   if (state && start < length)
      emit_segment( stage, header, start / length, 1.0 );

   stipple->counter += length;
}


static void
reset_stipple_counter(struct draw_stage *stage)
{
   struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
   stage->next->reset_stipple_counter( stage->next );
}

static void
stipple_reset_point(struct draw_stage *stage, struct prim_header *header)
{
   struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
   stage->next->point(stage->next, header);
}

static void
stipple_reset_tri(struct draw_stage *stage, struct prim_header *header)
{
   struct stipple_stage *stipple = stipple_stage(stage);
   stipple->counter = 0;
   stage->next->tri(stage->next, header);
}


static void
stipple_first_line(struct draw_stage *stage, 
		   struct prim_header *header)
{
   struct stipple_stage *stipple = stipple_stage(stage);
   struct draw_context *draw = stage->draw;

   stipple->pattern = draw->rasterizer->line_stipple_pattern;
   stipple->factor = draw->rasterizer->line_stipple_factor + 1;

   stage->line = stipple_line;
   stage->line( stage, header );
}


static void
stipple_flush(struct draw_stage *stage, unsigned flags)
{
   stage->line = stipple_first_line;
   stage->next->flush( stage->next, flags );
}




static void 
stipple_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


/**
 * Create line stippler stage
 */
struct draw_stage *draw_stipple_stage( struct draw_context *draw )
{
   struct stipple_stage *stipple = CALLOC_STRUCT(stipple_stage);
   if (stipple == NULL)
      goto fail;

   stipple->stage.draw = draw;
   stipple->stage.name = "stipple";
   stipple->stage.next = NULL;
   stipple->stage.point = stipple_reset_point;
   stipple->stage.line = stipple_first_line;
   stipple->stage.tri = stipple_reset_tri;
   stipple->stage.reset_stipple_counter = reset_stipple_counter;
   stipple->stage.flush = stipple_flush;
   stipple->stage.destroy = stipple_destroy;

   if (!draw_alloc_temp_verts( &stipple->stage, 2 ))
      goto fail;

   return &stipple->stage;

fail:
   if (stipple)
      stipple->stage.destroy( &stipple->stage );

   return NULL;
}
