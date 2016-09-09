
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

/* Deal with hardware and/or swtnl maximums:
 * - maximum number of vertices in buffer
 * - maximum number of elements (maybe zero)
 *
 * The maximums may vary with opengl state (eg if a larger hardware
 * vertex is required in this state, the maximum number of vertices
 * may be smaller than in another state).
 *
 * We want buffer splitting to be a convenience function for the code
 * actually drawing the primitives rather than a system-wide maximum,
 * otherwise it is hard to avoid pessimism.  
 *
 * For instance, if a driver has no hardware limits on vertex buffer
 * dimensions, it would not ordinarily want to split vbos.  But if
 * there is an unexpected fallback, eg memory manager fails to upload
 * textures, it will want to pass the drawing commands onto swtnl,
 * which does have limitations.  A convenience function allows swtnl
 * to split the drawing and vbos internally without imposing its
 * limitations on drivers which want to use it as a fallback path.
 */

#include <precomp.h>

/* True if a primitive can be split without copying of vertices, false
 * otherwise.
 */
GLboolean split_prim_inplace(GLenum mode, GLuint *first, GLuint *incr)
{
   switch (mode) {
   case GL_POINTS:
      *first = 1;
      *incr = 1;
      return GL_TRUE;
   case GL_LINES:
      *first = 2;
      *incr = 2;
      return GL_TRUE;
   case GL_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return GL_TRUE;
   case GL_TRIANGLES:
      *first = 3;
      *incr = 3;
      return GL_TRUE;
   case GL_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return GL_TRUE;
   case GL_QUADS:
      *first = 4;
      *incr = 4;
      return GL_TRUE;
   case GL_QUAD_STRIP:
      *first = 4;
      *incr = 2;
      return GL_TRUE;
   default:
      *first = 0;
      *incr = 1;		/* so that count % incr works */
      return GL_FALSE;
   }
}



void vbo_split_prims( struct gl_context *ctx,
		      const struct gl_client_array *arrays[],
		      const struct _mesa_prim *prim,
		      GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLuint min_index,
		      GLuint max_index,
		      vbo_draw_func draw,
		      const struct split_limits *limits )
{
   if (ib) {
      if (limits->max_indices == 0) {
	 /* Could traverse the indices, re-emitting vertices in turn.
	  * But it's hard to see why this case would be needed - for
	  * software tnl, it is better to convert to non-indexed
	  * rendering after transformation is complete.  Are there any devices
	  * with hardware tnl that cannot do indexed rendering?
	  *
	  * For now, this path is disabled.
	  */
	 assert(0);
      }
      else if (max_index - min_index >= limits->max_verts) {
	 /* The vertex buffers are too large for hardware (or the
	  * swtnl module).  Traverse the indices, re-emitting vertices
	  * in turn.  Use a vertex cache to preserve some of the
	  * sharing from the original index list.
	  */
	 vbo_split_copy(ctx, arrays, prim, nr_prims, ib,
			draw, limits );
      }
      else if (ib->count > limits->max_indices) {
	 /* The index buffer is too large for hardware.  Try to split
	  * on whole-primitive boundaries, otherwise try to split the
	  * individual primitives.
	  */
	 vbo_split_inplace(ctx, arrays, prim, nr_prims, ib,
			   min_index, max_index, draw, limits );
      }
      else {
	 /* Why were we called? */
	 assert(0);
      }
   }
   else {
      if (max_index - min_index >= limits->max_verts) {
	 /* The vertex buffer is too large for hardware (or the swtnl
	  * module).  Try to split on whole-primitive boundaries,
	  * otherwise try to split the individual primitives.
	  */
	 vbo_split_inplace(ctx, arrays, prim, nr_prims, ib,
			   min_index, max_index, draw, limits );
      }
      else {
	 /* Why were we called? */
	 assert(0);
      }
   }
}

