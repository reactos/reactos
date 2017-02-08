/**************************************************************************
 * 
 * Copyright 2004 Tungsten Graphics, Inc., Cedar Park, Texas.
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
    

#ifndef ST_DRAW_H
#define ST_DRAW_H

#include "main/compiler.h"
#include "main/glheader.h"

struct _mesa_index_buffer;
struct _mesa_prim;
struct gl_client_array;
struct gl_context;
struct st_context;

void st_init_draw( struct st_context *st );

void st_destroy_draw( struct st_context *st );

extern void
st_draw_vbo(struct gl_context *ctx,
            const struct gl_client_array **arrays,
            const struct _mesa_prim *prims,
            GLuint nr_prims,
            const struct _mesa_index_buffer *ib,
	    GLboolean index_bounds_valid,
            GLuint min_index,
            GLuint max_index,
            struct gl_transform_feedback_object *tfb_vertcount);

extern void
st_feedback_draw_vbo(struct gl_context *ctx,
                     const struct gl_client_array **arrays,
                     const struct _mesa_prim *prims,
                     GLuint nr_prims,
                     const struct _mesa_index_buffer *ib,
		     GLboolean index_bounds_valid,
                     GLuint min_index,
                     GLuint max_index,
                     struct gl_transform_feedback_object *tfb_vertcount);

/* Internal function:
 */
extern enum pipe_format
st_pipe_vertex_format(GLenum type, GLuint size, GLenum format,
                      GLboolean normalized, GLboolean integer);


/**
 * When drawing with VBOs, the addresses specified with
 * glVertex/Color/TexCoordPointer() are really offsets into the VBO, not real
 * addresses.  At some point we need to convert those pointers to offsets.
 * This function is basically a cast wrapper to avoid warnings when building
 * in 64-bit mode.
 */
static INLINE unsigned
pointer_to_offset(const void *ptr)
{
   return (unsigned) (((unsigned long) ptr) & 0xffffffffUL);
}


#endif
