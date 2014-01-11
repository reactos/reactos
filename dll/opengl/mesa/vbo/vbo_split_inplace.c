
/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include <precomp.h>

#define MAX_PRIM 32

/* Used for splitting without copying. No attempt is made to handle
 * too large indexed vertex buffers: In general you need to copy to do
 * that.
 */
struct split_context {
   struct gl_context *ctx;
   const struct gl_client_array **array;
   const struct _mesa_prim *prim;
   GLuint nr_prims;
   const struct _mesa_index_buffer *ib;
   GLuint min_index;
   GLuint max_index;
   vbo_draw_func draw;

   const struct split_limits *limits;
   GLuint limit;

   struct _mesa_prim dstprim[MAX_PRIM];
   GLuint dstprim_nr;
};




static void flush_vertex( struct split_context *split )
{
   struct _mesa_index_buffer ib;
   GLuint i;

   if (!split->dstprim_nr) 
      return;

   if (split->ib) {
      ib = *split->ib;

      ib.count = split->max_index - split->min_index + 1;
      ib.ptr = (const void *)((const char *)ib.ptr + 
                              split->min_index * _mesa_sizeof_type(ib.type));

      /* Rebase the primitives to save index buffer entries. */
      for (i = 0; i < split->dstprim_nr; i++)
	 split->dstprim[i].start -= split->min_index;
   }

   assert(split->max_index >= split->min_index);

   split->draw(split->ctx,
	       split->array,
	       split->dstprim,
	       split->dstprim_nr,
	       split->ib ? &ib : NULL,
	       !split->ib,
	       split->min_index,
	       split->max_index);

   split->dstprim_nr = 0;
   split->min_index = ~0;
   split->max_index = 0;
}


static struct _mesa_prim *next_outprim( struct split_context *split )
{
   if (split->dstprim_nr == MAX_PRIM-1) {
      flush_vertex(split);
   }

   {
      struct _mesa_prim *prim = &split->dstprim[split->dstprim_nr++];
      memset(prim, 0, sizeof(*prim));
      return prim;
   }
}

static void update_index_bounds(struct split_context *split,
				const struct _mesa_prim *prim)
{
   split->min_index = MIN2(split->min_index, prim->start);
   split->max_index = MAX2(split->max_index, prim->start + prim->count - 1);
}

/* Return the maximum amount of vertices that can be emitted for a
 * primitive starting at 'prim->start', depending on the previous
 * index bounds.
 */
static GLuint get_max_vertices(struct split_context *split,
			       const struct _mesa_prim *prim)
{
   if ((prim->start > split->min_index &&
	prim->start - split->min_index >= split->limit) ||
       (prim->start < split->max_index &&
        split->max_index - prim->start >= split->limit))
      /* "prim" starts too far away from the old range. */
      return 0;

   return MIN2(split->min_index, prim->start) + split->limit - prim->start;
}

/* Break large primitives into smaller ones.  If not possible, convert
 * the primitive to indexed and pass to split_elts().
 */
static void split_prims( struct split_context *split) 
{
   GLuint i;

   for (i = 0; i < split->nr_prims; i++) {
      const struct _mesa_prim *prim = &split->prim[i];
      GLuint first, incr;
      GLboolean split_inplace = split_prim_inplace(prim->mode, &first, &incr);
      GLuint available = get_max_vertices(split, prim);
      GLuint count = prim->count - (prim->count - first) % incr;

      if (prim->count < first)
	 continue;

      if ((available < count && !split_inplace) ||
	  (available < first && split_inplace)) {
	 flush_vertex(split);
	 available = get_max_vertices(split, prim);
      }
      
      if (available >= count) {
	 struct _mesa_prim *outprim = next_outprim(split);

	 *outprim = *prim;
	 update_index_bounds(split, outprim);
      }
      else if (split_inplace) {
	 GLuint j, nr;

	 for (j = 0 ; j < count ; ) {
	    GLuint remaining = count - j;
	    struct _mesa_prim *outprim = next_outprim(split);

	    nr = MIN2( available, remaining );
	    nr -= (nr - first) % incr;
	    
	    outprim->mode = prim->mode;
	    outprim->begin = (j == 0 && prim->begin);
	    outprim->end = (nr == remaining && prim->end);
	    outprim->start = prim->start + j;
	    outprim->count = nr;
            outprim->num_instances = prim->num_instances;

	    update_index_bounds(split, outprim);

	    if (nr == remaining) {
	       /* Finished. 
		*/
	       j += nr;
	    }
	    else {
	       /* Wrapped the primitive: 
		*/
	       j += nr - (first - incr);
	       flush_vertex(split);
	       available = get_max_vertices(split, prim);
	    }
	 }
      }
      else if (split->ib == NULL) {
	 /* XXX: could at least send the first max_verts off from the
	  * inplace buffers.
	  */

	 /* else convert to indexed primitive and pass to split_elts,
	  * which will do the necessary copying and turn it back into a
	  * vertex primitive for rendering...
	  */
	 struct _mesa_index_buffer ib;
	 struct _mesa_prim tmpprim;
	 GLuint *elts = malloc(count * sizeof(GLuint));
	 GLuint j;
	 
	 for (j = 0; j < count; j++)
	    elts[j] = prim->start + j;

	 ib.count = count;
	 ib.type = GL_UNSIGNED_INT;
	 ib.obj = split->ctx->Shared->NullBufferObj;
	 ib.ptr = elts;
	    
	 tmpprim = *prim;
	 tmpprim.indexed = 1;
	 tmpprim.start = 0;
	 tmpprim.count = count;
         tmpprim.num_instances = 1;

	 flush_vertex(split);

	 vbo_split_copy(split->ctx,
			split->array,
			&tmpprim, 1, 
			&ib,
			split->draw,
			split->limits);
	    
	 free(elts);
      }
      else {
	 flush_vertex(split);

	 vbo_split_copy(split->ctx,
			split->array,
			prim, 1, 
			split->ib,
			split->draw,
			split->limits);
      }
   }

   flush_vertex(split);
}


void vbo_split_inplace( struct gl_context *ctx,
			const struct gl_client_array *arrays[],
			const struct _mesa_prim *prim,
			GLuint nr_prims,
			const struct _mesa_index_buffer *ib,
			GLuint min_index,
			GLuint max_index,
			vbo_draw_func draw,
			const struct split_limits *limits )
{
   struct split_context split;

   memset(&split, 0, sizeof(split));

   split.ctx = ctx;
   split.array = arrays;
   split.prim = prim;
   split.nr_prims = nr_prims;
   split.ib = ib;

   /* Empty interval, makes calculations simpler. */
   split.min_index = ~0;
   split.max_index = 0;

   split.draw = draw;
   split.limits = limits;
   split.limit = ib ? limits->max_indices : limits->max_verts;

   split_prims( &split );
}


