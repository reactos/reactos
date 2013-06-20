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

#include "util/u_memory.h"
#include "draw/draw_private.h"
#include "draw/draw_pipe.h"



void
draw_pipe_passthrough_point(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->point(stage->next, header);
}

void
draw_pipe_passthrough_line(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->line(stage->next, header);
}

void
draw_pipe_passthrough_tri(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->tri(stage->next, header);
}





/* This is only used for temporary verts.
 */
#define MAX_VERTEX_SIZE ((2 + PIPE_MAX_SHADER_OUTPUTS) * 4 * sizeof(float))


/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
boolean draw_alloc_temp_verts( struct draw_stage *stage, unsigned nr )
{
   assert(!stage->tmp);

   stage->tmp = NULL;
   stage->nr_tmps = nr;

   if (nr != 0)
   {
      unsigned i;
      ubyte *store = (ubyte *) MALLOC( MAX_VERTEX_SIZE * nr );

      if (store == NULL)
         return FALSE;

      stage->tmp = (struct vertex_header **) MALLOC( sizeof(struct vertex_header *) * nr );
      if (stage->tmp == NULL) {
         FREE(store);
         return FALSE;
      }
         
      for (i = 0; i < nr; i++)
         stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }

   return TRUE;
}


void draw_free_temp_verts( struct draw_stage *stage )
{
   if (stage->tmp) {
      FREE( stage->tmp[0] );
      FREE( stage->tmp );
      stage->tmp = NULL;
   }
}


/* Reset vertex ids.  This is basically a type of flush.
 *
 * Called only from draw_pipe_vbuf.c
 */
void draw_reset_vertex_ids(struct draw_context *draw)
{
   struct draw_stage *stage = draw->pipeline.first;
   
   while (stage) {
      unsigned i;

      for (i = 0; i < stage->nr_tmps; i++)
	 stage->tmp[i]->vertex_id = UNDEFINED_VERTEX_ID;

      stage = stage->next;
   }

   if (draw->pipeline.verts)
   {
      unsigned i;
      char *verts = draw->pipeline.verts;
      unsigned stride = draw->pipeline.vertex_stride;

      for (i = 0; i < draw->pipeline.vertex_count; i++) {
         ((struct vertex_header *)verts)->vertex_id = UNDEFINED_VERTEX_ID;
         verts += stride;
      }
   }
}

