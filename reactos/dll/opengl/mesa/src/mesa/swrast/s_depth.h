/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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


#ifndef S_DEPTH_H
#define S_DEPTH_H


#include "main/glheader.h"
#include "s_span.h"

struct gl_context;
struct gl_renderbuffer;


extern GLuint
_swrast_depth_test_span( struct gl_context *ctx, SWspan *span);

extern void
_swrast_depth_clamp_span( struct gl_context *ctx, SWspan *span );

extern GLboolean
_swrast_depth_bounds_test( struct gl_context *ctx, SWspan *span );


extern void
_swrast_read_depth_span_float( struct gl_context *ctx, struct gl_renderbuffer *rb,
                               GLint n, GLint x, GLint y, GLfloat depth[] );

extern void
_swrast_clear_depth_buffer(struct gl_context *ctx);

extern void
_swrast_clear_depth_stencil_buffer(struct gl_context *ctx);


#endif
