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


/**
 * \mainpage The VBO splitter
 *
 * This is the private data used internally to the vbo_split_prims()
 * helper function.  Nobody outside the vbo_split* files needs to
 * include or know about this structure.
 */


#ifndef _VBO_SPLIT_H
#define _VBO_SPLIT_H

#include "vbo.h"


/* True if a primitive can be split without copying of vertices, false
 * otherwise.
 */
GLboolean split_prim_inplace(GLenum mode, GLuint *first, GLuint *incr);

void vbo_split_inplace( struct gl_context *ctx,
			const struct gl_client_array *arrays[],
			const struct _mesa_prim *prim,
			GLuint nr_prims,
			const struct _mesa_index_buffer *ib,
			GLuint min_index,
			GLuint max_index,
			vbo_draw_func draw,
			const struct split_limits *limits );

/* Requires ib != NULL:
 */
void vbo_split_copy( struct gl_context *ctx,
		     const struct gl_client_array *arrays[],
		     const struct _mesa_prim *prim,
		     GLuint nr_prims,
		     const struct _mesa_index_buffer *ib,
		     vbo_draw_func draw,
		     const struct split_limits *limits );

#endif
