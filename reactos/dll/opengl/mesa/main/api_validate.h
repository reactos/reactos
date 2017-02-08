
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef API_VALIDATE_H
#define API_VALIDATE_H


#include "glheader.h"
#include "mfeatures.h"

struct gl_buffer_object;
struct gl_context;
struct gl_transform_feedback_object;


extern GLuint
_mesa_max_buffer_index(struct gl_context *ctx, GLuint count, GLenum type,
                       const void *indices,
                       struct gl_buffer_object *elementBuf);


extern GLboolean
_mesa_valid_prim_mode(const struct gl_context *ctx, GLenum mode);


extern GLboolean
_mesa_validate_DrawArrays(struct gl_context *ctx,
			  GLenum mode, GLint start, GLsizei count);

extern GLboolean
_mesa_validate_DrawElements(struct gl_context *ctx,
			    GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices);

#endif
