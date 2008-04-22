
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

/* Helper for drivers which find themselves rendering a range of
 * indices starting somewhere above zero.  Typically the application
 * is issuing multiple DrawArrays() or DrawElements() to draw
 * successive primitives layed out linearly in the vertex arrays.
 * Unless the vertex arrays are all in a VBO, the OpenGL semantics
 * imply that we need to re-upload the vertex data on each draw call.
 * In that case, we want to avoid starting the upload at zero, as it
 * will mean every draw call uploads an increasing amount of not-used
 * vertex data.  Worse - in the software tnl module, all those
 * vertices will be transformed and lit.
 *
 * If we just upload the new data, however, the indices will be
 * incorrect as we tend to upload each set of vertex data to a new
 * region.  
 *
 * This file provides a helper to adjust the arrays, primitives and
 * indices of a draw call so that it can be re-issued with a min_index
 * of zero.
 */

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"

#include "vbo.h"


#define REBASE(TYPE) 						\
static void *rebase_##TYPE( const void *ptr,			\
			  GLuint count, 			\
			  TYPE min_index )			\
{								\
   const TYPE *in = (TYPE *)ptr;				\
   TYPE *tmp_indices = malloc(count * sizeof(TYPE));	\
   GLuint i;							\
								\
   for (i = 0; i < count; i++)  				\
      tmp_indices[i] = in[i] - min_index;			\
								\
   return (void *)tmp_indices;					\
}


REBASE(GLuint)
REBASE(GLushort)
REBASE(GLubyte)

GLboolean vbo_all_varyings_in_vbos( const struct gl_client_array *arrays[] )
{
   GLuint i;
   
   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      if (arrays[i]->StrideB &&
	  arrays[i]->BufferObj->Name == 0)
	 return GL_FALSE;

   return GL_TRUE;
}

/* Adjust primitives, indices and vertex definitions so that min_index
 * becomes zero. There are lots of reasons for wanting to do this, eg:
 *
 * Software tnl:
 *    - any time min_index != 0, otherwise unused vertices lower than
 *      min_index will be transformed.
 *
 * Hardware tnl:
 *    - if ib != NULL and min_index != 0, otherwise vertices lower than 
 *      min_index will be uploaded.  Requires adjusting index values.
 *
 *    - if ib == NULL and min_index != 0, just for convenience so this doesn't
 *      have to be handled within the driver.
 *
 * Hardware tnl with VBO support:
 *    - as above, but only when vertices are not (all?) in VBO's.
 *    - can't save time by trying to upload half a vbo - typically it is
 *      all or nothing.
 */
void vbo_rebase_prims( GLcontext *ctx,
		       const struct gl_client_array *arrays[],
		       const struct _mesa_prim *prim,
		       GLuint nr_prims,
		       const struct _mesa_index_buffer *ib,
		       GLuint min_index,
		       GLuint max_index,
		       vbo_draw_func draw )
{
   struct gl_client_array tmp_arrays[VERT_ATTRIB_MAX];
   const struct gl_client_array *tmp_array_pointers[VERT_ATTRIB_MAX];

   struct _mesa_index_buffer tmp_ib;
   struct _mesa_prim *tmp_prims = NULL;
   void *tmp_indices = NULL;
   GLuint i;

   assert(min_index != 0);

   if (0)
      _mesa_printf("%s %d..%d\n", __FUNCTION__, min_index, max_index);

   if (ib) {
      /* Unfortunately need to adjust each index individually.
       */
      GLboolean map_ib = ib->obj->Name && !ib->obj->Pointer;
      void *ptr;

      if (map_ib) 
	 ctx->Driver.MapBuffer(ctx, 
			       GL_ELEMENT_ARRAY_BUFFER,
			       GL_READ_ONLY_ARB,
			       ib->obj);


      ptr = ADD_POINTERS(ib->obj->Pointer, ib->ptr);

      /* Some users might prefer it if we translated elements to
       * GLuints here.  Others wouldn't...
       */
      switch (ib->type) {
      case GL_UNSIGNED_INT: 
	 tmp_indices = rebase_GLuint( ptr, ib->count, min_index );
	 break;
      case GL_UNSIGNED_SHORT: 
	 tmp_indices = rebase_GLushort( ptr, ib->count, min_index );
	 break;
      case GL_UNSIGNED_BYTE: 
	 tmp_indices = rebase_GLubyte( ptr, ib->count, min_index );
	 break;
      }      

      if (map_ib) 
	 ctx->Driver.UnmapBuffer(ctx, 
				 GL_ELEMENT_ARRAY_BUFFER,
				 ib->obj);

      tmp_ib.obj = ctx->Array.NullBufferObj;
      tmp_ib.ptr = tmp_indices;
      tmp_ib.count = ib->count;
      tmp_ib.type = ib->type;

      ib = &tmp_ib;
   }
   else {
      /* Otherwise the primitives need adjustment.
       */
      tmp_prims = (struct _mesa_prim *)_mesa_malloc(sizeof(*prim) * nr_prims);

      for (i = 0; i < nr_prims; i++) {
	 /* If this fails, it could indicate an application error:
	  */
	 assert(prim[i].start >= min_index);

	 tmp_prims[i] = prim[i];
	 tmp_prims[i].start -= min_index;
      }

      prim = tmp_prims;
   }

   /* Just need to adjust the pointer values on each incoming array.
    * This works for VBO and non-vbo rendering and shouldn't pesimize
    * VBO-based upload schemes.  However this may still not be a fast
    * path for hardware tnl for VBO based rendering as most machines
    * will be happier if you just specify a starting vertex value in
    * each primitive.
    *
    * For drivers with hardware tnl, you only want to do this if you
    * are forced to, eg non-VBO indexed rendering with start != 0.
    */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      tmp_arrays[i] = *arrays[i];
      tmp_arrays[i].Ptr += min_index * tmp_arrays[i].StrideB;
      tmp_array_pointers[i] = &tmp_arrays[i];
   }
   
   /* Re-issue the draw call.
    */
   draw( ctx, 
	 tmp_array_pointers, 
	 prim, 
	 nr_prims, 
	 ib, 
	 0, 
	 max_index - min_index );
   
   if (tmp_indices)
      _mesa_free(tmp_indices);
   
   if (tmp_prims)
      _mesa_free(tmp_prims);
}



