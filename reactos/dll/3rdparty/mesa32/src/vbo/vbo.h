/*
 * mesa 3-D graphics library
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
 */

/**
 * \file vbo_context.h
 * \brief VBO builder module datatypes and definitions.
 * \author Keith Whitwell
 */


#ifndef _VBO_H
#define _VBO_H

#include "mtypes.h"

struct _mesa_prim {
   GLuint mode:8;
   GLuint indexed:1;
   GLuint begin:1;
   GLuint end:1;
   GLuint weak:1;
   GLuint pad:20;

   GLuint start;
   GLuint count;
};

/* Would like to call this a "vbo_index_buffer", but this would be
 * confusing as the indices are not neccessarily yet in a non-null
 * buffer object.
 */
struct _mesa_index_buffer {
   GLuint count;
   GLenum type;
   struct gl_buffer_object *obj;
   const void *ptr;
};



GLboolean _vbo_CreateContext( GLcontext *ctx );
void _vbo_DestroyContext( GLcontext *ctx );
void _vbo_InvalidateState( GLcontext *ctx, GLuint new_state );


typedef void (*vbo_draw_func)( GLcontext *ctx,
			       const struct gl_client_array **arrays,
			       const struct _mesa_prim *prims,
			       GLuint nr_prims,
			       const struct _mesa_index_buffer *ib,
			       GLuint min_index,
			       GLuint max_index );




/* Utility function to cope with various constraints on tnl modules or
 * hardware.  This can be used to split an incoming set of arrays and
 * primitives against the following constraints:
 *    - Maximum number of indices in index buffer.
 *    - Maximum number of vertices referenced by index buffer.
 *    - Maximum hardware vertex buffer size.
 */
struct split_limits {
   GLuint max_verts;
   GLuint max_indices;
   GLuint max_vb_size;		/* bytes */
};


void vbo_split_prims( GLcontext *ctx,
		      const struct gl_client_array *arrays[],
		      const struct _mesa_prim *prim,
		      GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLuint min_index,
		      GLuint max_index,
		      vbo_draw_func draw,
		      const struct split_limits *limits );


/* Helpers for dealing translating away non-zero min_index.
 */
GLboolean vbo_all_varyings_in_vbos( const struct gl_client_array *arrays[] );

void vbo_rebase_prims( GLcontext *ctx,
		       const struct gl_client_array *arrays[],
		       const struct _mesa_prim *prim,
		       GLuint nr_prims,
		       const struct _mesa_index_buffer *ib,
		       GLuint min_index,
		       GLuint max_index,
		       vbo_draw_func draw );


#endif
