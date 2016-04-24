
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

/* Split indexed primitives with per-vertex copying.
 */

#include <precomp.h>

#define ELT_TABLE_SIZE 16

/**
 * Used for vertex-level splitting of indexed buffers.  Note that
 * non-indexed primitives may be converted to indexed in some cases
 * (eg loops, fans) in order to use this splitting path.
 */
struct copy_context {

   struct gl_context *ctx;
   const struct gl_client_array **array;
   const struct _mesa_prim *prim;
   GLuint nr_prims;
   const struct _mesa_index_buffer *ib;
   vbo_draw_func draw;

   const struct split_limits *limits;

   struct {
      GLuint attr;
      GLuint size;
      const struct gl_client_array *array;
      const GLubyte *src_ptr;

      struct gl_client_array dstarray;

   } varying[VBO_ATTRIB_MAX];
   GLuint nr_varying;

   const struct gl_client_array *dstarray_ptr[VBO_ATTRIB_MAX];
   struct _mesa_index_buffer dstib;

   GLuint *translated_elt_buf;
   const GLuint *srcelt;

   /** A baby hash table to avoid re-emitting (some) duplicate
    * vertices when splitting indexed primitives.
    */
   struct { 
      GLuint in;
      GLuint out;
   } vert_cache[ELT_TABLE_SIZE];

   GLuint vertex_size;
   GLubyte *dstbuf;
   GLubyte *dstptr;     /**< dstptr == dstbuf + dstelt_max * vertsize */
   GLuint dstbuf_size;  /**< in vertices */
   GLuint dstbuf_nr;    /**< count of emitted vertices, also the largest value
                         * in dstelt.  Our MaxIndex.
                         */

   GLuint *dstelt;
   GLuint dstelt_nr;
   GLuint dstelt_size;

#define MAX_PRIM 32
   struct _mesa_prim dstprim[MAX_PRIM];
   GLuint dstprim_nr;

};


static GLuint attr_size( const struct gl_client_array *array )
{
   return array->Size * _mesa_sizeof_type(array->Type);
}


/**
 * Starts returning true slightly before the buffer fills, to ensure
 * that there is sufficient room for any remaining vertices to finish
 * off the prim:
 */
static GLboolean
check_flush( struct copy_context *copy )
{
   GLenum mode = copy->dstprim[copy->dstprim_nr].mode; 

   if (GL_TRIANGLE_STRIP == mode &&
       copy->dstelt_nr & 1) { /* see bug9962 */
       return GL_FALSE;
   }

   if (copy->dstbuf_nr + 4 > copy->dstbuf_size)
      return GL_TRUE;

   if (copy->dstelt_nr + 4 > copy->dstelt_size)
      return GL_TRUE;

   return GL_FALSE;
}


/**
 * Dump the parameters/info for a vbo->draw() call.
 */
static void
dump_draw_info(struct gl_context *ctx,
               const struct gl_client_array **arrays,
               const struct _mesa_prim *prims,
               GLuint nr_prims,
               const struct _mesa_index_buffer *ib,
               GLuint min_index,
               GLuint max_index)
{
   GLuint i, j;

   printf("VBO Draw:\n");
   for (i = 0; i < nr_prims; i++) {
      printf("Prim %u of %u\n", i, nr_prims);
      printf("  Prim mode 0x%x\n", prims[i].mode);
      printf("  IB: %p\n", (void*) ib);
      for (j = 0; j < VBO_ATTRIB_MAX; j++) {
         printf("    array %d at %p:\n", j, (void*) arrays[j]);
         printf("      enabled %d, ptr %p, size %d, type 0x%x, stride %d\n",
		arrays[j]->Enabled, arrays[j]->Ptr,
		arrays[j]->Size, arrays[j]->Type, arrays[j]->StrideB);
         if (0) {
            GLint k = prims[i].start + prims[i].count - 1;
            GLfloat *last = (GLfloat *) (arrays[j]->Ptr + arrays[j]->Stride * k);
            printf("        last: %f %f %f\n",
		   last[0], last[1], last[2]);
         }
      }
   }
}


static void
flush( struct copy_context *copy )
{
   GLuint i;

   /* Set some counters: 
    */
   copy->dstib.count = copy->dstelt_nr;

#if 0
   dump_draw_info(copy->ctx,
                  copy->dstarray_ptr,
                  copy->dstprim,
                  copy->dstprim_nr,
                  &copy->dstib,
                  0,
                  copy->dstbuf_nr);
#else
   (void) dump_draw_info;
#endif

   copy->draw( copy->ctx,
	       copy->dstarray_ptr,
	       copy->dstprim,
	       copy->dstprim_nr,
	       &copy->dstib,
	       GL_TRUE,
	       0,
	       copy->dstbuf_nr - 1);

   /* Reset all pointers: 
    */
   copy->dstprim_nr = 0;
   copy->dstelt_nr = 0;
   copy->dstbuf_nr = 0;
   copy->dstptr = copy->dstbuf;

   /* Clear the vertex cache:
    */
   for (i = 0; i < ELT_TABLE_SIZE; i++)
      copy->vert_cache[i].in = ~0;
}


/**
 * Called at begin of each primitive during replay.
 */
static void
begin( struct copy_context *copy, GLenum mode, GLboolean begin_flag )
{
   struct _mesa_prim *prim = &copy->dstprim[copy->dstprim_nr];

   prim->mode = mode;
   prim->begin = begin_flag;
   prim->num_instances = 1;
}


/**
 * Use a hashtable to attempt to identify recently-emitted vertices
 * and avoid re-emitting them.
 */
static GLuint
elt(struct copy_context *copy, GLuint elt_idx)
{
   GLuint elt = copy->srcelt[elt_idx];
   GLuint slot = elt & (ELT_TABLE_SIZE-1);

/*    printf("elt %d\n", elt); */

   /* Look up the incoming element in the vertex cache.  Re-emit if
    * necessary.   
    */
   if (copy->vert_cache[slot].in != elt) {
      GLubyte *csr = copy->dstptr;
      GLuint i;

/*       printf("  --> emit to dstelt %d\n", copy->dstbuf_nr); */

      for (i = 0; i < copy->nr_varying; i++) {
	 const struct gl_client_array *srcarray = copy->varying[i].array;
	 const GLubyte *srcptr = copy->varying[i].src_ptr + elt * srcarray->StrideB;

	 memcpy(csr, srcptr, copy->varying[i].size);
	 csr += copy->varying[i].size;

#ifdef NAN_CHECK
         if (srcarray->Type == GL_FLOAT) {
            GLuint k;
            GLfloat *f = (GLfloat *) srcptr;
            for (k = 0; k < srcarray->Size; k++) {
               assert(!IS_INF_OR_NAN(f[k]));
               assert(f[k] <= 1.0e20 && f[k] >= -1.0e20);
            }
         }
#endif

	 if (0) 
	 {
	    const GLuint *f = (const GLuint *)srcptr;
	    GLuint j;
	    printf("  varying %d: ", i);
	    for(j = 0; j < copy->varying[i].size / 4; j++)
	       printf("%x ", f[j]);
	    printf("\n");
	 }
      }

      copy->vert_cache[slot].in = elt;
      copy->vert_cache[slot].out = copy->dstbuf_nr++;
      copy->dstptr += copy->vertex_size;

      assert(csr == copy->dstptr);
      assert(copy->dstptr == (copy->dstbuf +
                              copy->dstbuf_nr * copy->vertex_size));
   }
/*    else */
/*       printf("  --> reuse vertex\n"); */
   
/*    printf("  --> emit %d\n", copy->vert_cache[slot].out); */
   copy->dstelt[copy->dstelt_nr++] = copy->vert_cache[slot].out;
   return check_flush(copy);
}


/**
 * Called at end of each primitive during replay.
 */
static void
end( struct copy_context *copy, GLboolean end_flag )
{
   struct _mesa_prim *prim = &copy->dstprim[copy->dstprim_nr];

/*    printf("end (%d)\n", end_flag); */

   prim->end = end_flag;
   prim->count = copy->dstelt_nr - prim->start;

   if (++copy->dstprim_nr == MAX_PRIM ||
       check_flush(copy)) 
      flush(copy);
}


static void
replay_elts( struct copy_context *copy )
{
   GLuint i, j, k;
   GLboolean split;

   for (i = 0; i < copy->nr_prims; i++) {
      const struct _mesa_prim *prim = &copy->prim[i];
      const GLuint start = prim->start;
      GLuint first, incr;

      switch (prim->mode) {
	 
      case GL_LINE_LOOP:
	 /* Convert to linestrip and emit the final vertex explicitly,
	  * but only in the resultant strip that requires it.
	  */
	 j = 0;
	 while (j != prim->count) {
	    begin(copy, GL_LINE_STRIP, prim->begin && j == 0);

	    for (split = GL_FALSE; j != prim->count && !split; j++)
	       split = elt(copy, start + j);

	    if (j == prim->count) {
	       /* Done, emit final line.  Split doesn't matter as
		* it is always raised a bit early so we can emit
		* the last verts if necessary!
		*/
	       if (prim->end) 
		  (void)elt(copy, start + 0);

	       end(copy, prim->end);
	    }
	    else {
	       /* Wrap
		*/
	       assert(split);
	       end(copy, 0);
	       j--;
	    }
	 }
	 break;

      case GL_TRIANGLE_FAN:
      case GL_POLYGON:
	 j = 2;
	 while (j != prim->count) {
	    begin(copy, prim->mode, prim->begin && j == 0);

	    split = elt(copy, start+0); 
	    assert(!split);

	    split = elt(copy, start+j-1); 
	    assert(!split);

	    for (; j != prim->count && !split; j++)
	       split = elt(copy, start+j);

	    end(copy, prim->end && j == prim->count);

	    if (j != prim->count) {
	       /* Wrapped the primitive, need to repeat some vertices:
		*/
	       j -= 1;
	    }
	 }
	 break;

      default:
	 (void)split_prim_inplace(prim->mode, &first, &incr);
	 
	 j = 0;
	 while (j != prim->count) {

	    begin(copy, prim->mode, prim->begin && j == 0);

	    split = 0;
	    for (k = 0; k < first; k++, j++)
	       split |= elt(copy, start+j);

	    assert(!split);

	    for (; j != prim->count && !split; )
	       for (k = 0; k < incr; k++, j++)
		  split |= elt(copy, start+j);

	    end(copy, prim->end && j == prim->count);

	    if (j != prim->count) {
	       /* Wrapped the primitive, need to repeat some vertices:
		*/
	       assert(j > first - incr);
	       j -= (first - incr);
	    }
	 }
	 break;
      }
   }

   if (copy->dstprim_nr)
      flush(copy);
}


static void
replay_init( struct copy_context *copy )
{
   struct gl_context *ctx = copy->ctx;
   GLuint i;
   GLuint offset;
   const GLvoid *srcptr;

   /* Make a list of varying attributes and their vbo's.  Also
    * calculate vertex size.
    */
   copy->vertex_size = 0;
   for (i = 0; i < VBO_ATTRIB_MAX; i++) {
      struct gl_buffer_object *vbo = copy->array[i]->BufferObj;

      if (copy->array[i]->StrideB == 0) {
	 copy->dstarray_ptr[i] = copy->array[i];
      }
      else {
	 GLuint j = copy->nr_varying++;
	 
	 copy->varying[j].attr = i;
	 copy->varying[j].array = copy->array[i];
	 copy->varying[j].size = attr_size(copy->array[i]);
	 copy->vertex_size += attr_size(copy->array[i]);
      
	 if (_mesa_is_bufferobj(vbo) && !_mesa_bufferobj_mapped(vbo)) 
	    ctx->Driver.MapBufferRange(ctx, 0, vbo->Size, GL_MAP_READ_BIT, vbo);

	 copy->varying[j].src_ptr = ADD_POINTERS(vbo->Pointer,
						 copy->array[i]->Ptr);

	 copy->dstarray_ptr[i] = &copy->varying[j].dstarray;
      }
   }

   /* There must always be an index buffer.  Currently require the
    * caller convert non-indexed prims to indexed.  Could alternately
    * do it internally.
    */
   if (_mesa_is_bufferobj(copy->ib->obj) &&
       !_mesa_bufferobj_mapped(copy->ib->obj)) 
      ctx->Driver.MapBufferRange(ctx, 0, copy->ib->obj->Size, GL_MAP_READ_BIT,
				 copy->ib->obj);

   srcptr = (const GLubyte *) ADD_POINTERS(copy->ib->obj->Pointer,
                                           copy->ib->ptr);

   switch (copy->ib->type) {
   case GL_UNSIGNED_BYTE:
      copy->translated_elt_buf = malloc(sizeof(GLuint) * copy->ib->count);
      copy->srcelt = copy->translated_elt_buf;

      for (i = 0; i < copy->ib->count; i++)
	 copy->translated_elt_buf[i] = ((const GLubyte *)srcptr)[i];
      break;

   case GL_UNSIGNED_SHORT:
      copy->translated_elt_buf = malloc(sizeof(GLuint) * copy->ib->count);
      copy->srcelt = copy->translated_elt_buf;

      for (i = 0; i < copy->ib->count; i++)
	 copy->translated_elt_buf[i] = ((const GLushort *)srcptr)[i];
      break;

   case GL_UNSIGNED_INT:
      copy->translated_elt_buf = NULL;
      copy->srcelt = (const GLuint *)srcptr;
      break;
   }

   /* Figure out the maximum allowed vertex buffer size:
    */
   if (copy->vertex_size * copy->limits->max_verts <= copy->limits->max_vb_size) {
      copy->dstbuf_size = copy->limits->max_verts;
   }
   else {
      copy->dstbuf_size = copy->limits->max_vb_size / copy->vertex_size;
   }

   /* Allocate an output vertex buffer:
    *
    * XXX:  This should be a VBO!
    */
   copy->dstbuf = malloc(copy->dstbuf_size * copy->vertex_size);   
   copy->dstptr = copy->dstbuf;

   /* Setup new vertex arrays to point into the output buffer: 
    */
   for (offset = 0, i = 0; i < copy->nr_varying; i++) {
      const struct gl_client_array *src = copy->varying[i].array;
      struct gl_client_array *dst = &copy->varying[i].dstarray;

      dst->Size = src->Size;
      dst->Type = src->Type;
      dst->Stride = copy->vertex_size;
      dst->StrideB = copy->vertex_size;
      dst->Ptr = copy->dstbuf + offset;
      dst->Enabled = GL_TRUE;
      dst->Normalized = src->Normalized; 
      dst->Integer = src->Integer;
      dst->BufferObj = ctx->Shared->NullBufferObj;
      dst->_ElementSize = src->_ElementSize;
      dst->_MaxElement = copy->dstbuf_size; /* may be less! */

      offset += copy->varying[i].size;
   }

   /* Allocate an output element list:
    */
   copy->dstelt_size = MIN2(65536,
			    copy->ib->count * 2 + 3);
   copy->dstelt_size = MIN2(copy->dstelt_size,
			    copy->limits->max_indices);
   copy->dstelt = malloc(sizeof(GLuint) * copy->dstelt_size);
   copy->dstelt_nr = 0;

   /* Setup the new index buffer to point to the allocated element
    * list:
    */
   copy->dstib.count = 0;	/* duplicates dstelt_nr */
   copy->dstib.type = GL_UNSIGNED_INT;
   copy->dstib.obj = ctx->Shared->NullBufferObj;
   copy->dstib.ptr = copy->dstelt;
}


/**
 * Free up everything allocated during split/replay.
 */
static void
replay_finish( struct copy_context *copy )
{
   struct gl_context *ctx = copy->ctx;
   GLuint i;

   /* Free our vertex and index buffers: 
    */
   free(copy->translated_elt_buf);
   free(copy->dstbuf);
   free(copy->dstelt);

   /* Unmap VBO's 
    */
   for (i = 0; i < copy->nr_varying; i++) {
      struct gl_buffer_object *vbo = copy->varying[i].array->BufferObj;
      if (_mesa_is_bufferobj(vbo) && _mesa_bufferobj_mapped(vbo)) 
	 ctx->Driver.UnmapBuffer(ctx, vbo);
   }

   /* Unmap index buffer:
    */
   if (_mesa_is_bufferobj(copy->ib->obj) &&
       _mesa_bufferobj_mapped(copy->ib->obj)) {
      ctx->Driver.UnmapBuffer(ctx, copy->ib->obj);
   }
}


/**
 * Split VBO into smaller pieces, draw the pieces.
 */
void vbo_split_copy( struct gl_context *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     vbo_draw_func draw,
		     const struct split_limits *limits )
{
   struct copy_context copy;
   GLuint i;

   memset(&copy, 0, sizeof(copy));

   /* Require indexed primitives:
    */
   assert(ib);

   copy.ctx = ctx;
   copy.array = arrays;
   copy.prim = prim;
   copy.nr_prims = nr_prims;
   copy.ib = ib;
   copy.draw = draw;
   copy.limits = limits;

   /* Clear the vertex cache:
    */
   for (i = 0; i < ELT_TABLE_SIZE; i++)
      copy.vert_cache[i].in = ~0;

   replay_init(&copy);
   replay_elts(&copy);
   replay_finish(&copy);
}
