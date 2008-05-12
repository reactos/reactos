
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


#include "mtypes.h"
#include "macros.h"
#include "enums.h"
#include "vbo_split.h"


#define MAX_PRIM 32

/* Used for splitting without copying.
 */
struct split_context {
   GLcontext *ctx;
   const struct gl_client_array **array;
   const struct _mesa_prim *prim;
   GLuint nr_prims;
   const struct _mesa_index_buffer *ib;
   GLuint min_index;
   GLuint max_index;
   vbo_draw_func draw;

   const struct split_limits *limits;

   struct _mesa_prim dstprim[MAX_PRIM];
   GLuint dstprim_nr;
};




static void flush_vertex( struct split_context *split )
{
   GLint min_index, max_index;

   if (!split->dstprim_nr) 
      return;

   if (split->ib) {
      /* This should basically be multipass rendering over the same
       * unchanging set of VBO's.  Would like the driver not to
       * re-upload the data, or swtnl not to re-transform the
       * vertices.
       */
      assert(split->max_index - split->min_index < split->limits->max_verts);
      min_index = split->min_index;
      max_index = split->max_index;
   }
   else {
      /* Non-indexed rendering.  Cannot assume that the primitives are
       * ordered by increasing vertex, because of entrypoints like
       * MultiDrawArrays.
       */
      GLuint i;
      min_index = split->dstprim[0].start;
      max_index = min_index + split->dstprim[0].count - 1;

      for (i = 1; i < split->dstprim_nr; i++) {
	 GLuint tmp_min = split->dstprim[i].start;
	 GLuint tmp_max = tmp_min + split->dstprim[i].count - 1;

	 if (tmp_min < min_index) 
	    min_index = tmp_min;

	 if (tmp_max > max_index) 
	    max_index = tmp_max;
      }
   }

   assert(max_index >= min_index);

   split->draw( split->ctx, 
		split->array, 
		split->dstprim,
		split->dstprim_nr,
		NULL,
		min_index,
		max_index);

   split->dstprim_nr = 0;
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

static int align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}



/* Break large primitives into smaller ones.  If not possible, convert
 * the primitive to indexed and pass to split_elts().
 */
static void split_prims( struct split_context *split) 
{
   GLuint csr = 0;
   GLuint i;

   for (i = 0; i < split->nr_prims; i++) {
      const struct _mesa_prim *prim = &split->prim[i];
      GLuint first, incr;
      GLboolean split_inplace = split_prim_inplace(prim->mode, &first, &incr);
      GLuint count;

      /* Always wrap on an even numbered vertex to avoid problems with
       * triangle strips.  
       */
      GLuint available = align(split->limits->max_verts - csr - 1, 2); 
      assert(split->limits->max_verts >= csr);

      if (prim->count < first)
	 continue;
      
      count = prim->count - (prim->count - first) % incr; 


      if ((available < count && !split_inplace) || 
	  (available < first && split_inplace)) {
	 flush_vertex(split);
	 csr = 0;
	 available = align(split->limits->max_verts - csr - 1, 2);
      }
      
      if (available >= count) {
	 struct _mesa_prim *outprim = next_outprim(split);
	 *outprim = *prim;
	 csr += prim->count;
	 available = align(split->limits->max_verts - csr - 1, 2); 
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
	    
	    if (nr == remaining) {
	       /* Finished. 
		*/
	       j += nr;		
	       csr += nr;
	       available = align(split->limits->max_verts - csr - 1, 2); 
	    }
	    else {
	       /* Wrapped the primitive: 
		*/
	       j += nr - (first - incr);
	       flush_vertex(split);
	       csr = 0;
	       available = align(split->limits->max_verts - csr - 1, 2); 
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
	 ib.obj = split->ctx->Array.NullBufferObj;
	 ib.ptr = elts;
	    
	 tmpprim = *prim;
	 tmpprim.indexed = 1;
	 tmpprim.start = 0;
	 tmpprim.count = count;

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


void vbo_split_inplace( GLcontext *ctx,
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
   split.min_index = min_index;
   split.max_index = max_index;
   split.draw = draw;
   split.limits = limits;

   split_prims( &split );
}


