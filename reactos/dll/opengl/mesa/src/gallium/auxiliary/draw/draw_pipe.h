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

#ifndef DRAW_PIPE_H
#define DRAW_PIPE_H

#include "pipe/p_compiler.h"
#include "draw_private.h"       /* for sizeof(vertex_header) */


/**
 * Basic info for a point/line/triangle primitive.
 */
struct prim_header {
   float det;                 /**< front/back face determinant */
   ushort flags;
   ushort pad;
   struct vertex_header *v[3];  /**< 1 to 3 vertex pointers */
};



/**
 * Base class for all primitive drawing stages.
 */
struct draw_stage
{
   struct draw_context *draw;   /**< parent context */

   struct draw_stage *next;     /**< next stage in pipeline */
   const char *name;            /**< for debugging  */

   struct vertex_header **tmp;  /**< temp vert storage, such as for clipping */
   unsigned nr_tmps;

   void (*point)( struct draw_stage *,
		  struct prim_header * );

   void (*line)( struct draw_stage *,
		 struct prim_header * );

   void (*tri)( struct draw_stage *,
		struct prim_header * );

   void (*flush)( struct draw_stage *,
		  unsigned flags );

   void (*reset_stipple_counter)( struct draw_stage * );

   void (*destroy)( struct draw_stage * );
};


extern struct draw_stage *draw_unfilled_stage( struct draw_context *context );
extern struct draw_stage *draw_twoside_stage( struct draw_context *context );
extern struct draw_stage *draw_offset_stage( struct draw_context *context );
extern struct draw_stage *draw_clip_stage( struct draw_context *context );
extern struct draw_stage *draw_flatshade_stage( struct draw_context *context );
extern struct draw_stage *draw_cull_stage( struct draw_context *context );
extern struct draw_stage *draw_stipple_stage( struct draw_context *context );
extern struct draw_stage *draw_wide_line_stage( struct draw_context *context );
extern struct draw_stage *draw_wide_point_stage( struct draw_context *context );
extern struct draw_stage *draw_validate_stage( struct draw_context *context );


extern void draw_free_temp_verts( struct draw_stage *stage );
extern boolean draw_alloc_temp_verts( struct draw_stage *stage, unsigned nr );

extern void draw_reset_vertex_ids( struct draw_context *draw );

void draw_pipe_passthrough_tri(struct draw_stage *stage, struct prim_header *header);
void draw_pipe_passthrough_line(struct draw_stage *stage, struct prim_header *header);
void draw_pipe_passthrough_point(struct draw_stage *stage, struct prim_header *header);



/**
 * Get a writeable copy of a vertex.
 * \param stage  drawing stage info
 * \param vert  the vertex to copy (source)
 * \param idx  index into stage's tmp[] array to put the copy (dest)
 * \return  pointer to the copied vertex
 */
static INLINE struct vertex_header *
dup_vert( struct draw_stage *stage,
	  const struct vertex_header *vert,
	  unsigned idx )
{   
   struct vertex_header *tmp = stage->tmp[idx];
   const uint vsize = sizeof(struct vertex_header)
      + stage->draw->vs.num_vs_outputs * 4 * sizeof(float);
   memcpy(tmp, vert, vsize);
   tmp->vertex_id = UNDEFINED_VERTEX_ID;
   return tmp;
}

#endif
