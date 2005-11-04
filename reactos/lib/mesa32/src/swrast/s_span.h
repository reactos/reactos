/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef S_SPAN_H
#define S_SPAN_H


#include "mtypes.h"
#include "swrast.h"


extern void
_swrast_span_default_z( GLcontext *ctx, struct sw_span *span );

extern void
_swrast_span_interpolate_z( const GLcontext *ctx, struct sw_span *span );

extern void
_swrast_span_default_fog( GLcontext *ctx, struct sw_span *span );

extern void
_swrast_span_default_color( GLcontext *ctx, struct sw_span *span );

extern void
_swrast_span_default_texcoords( GLcontext *ctx, struct sw_span *span );

extern GLfloat
_swrast_compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
                       GLfloat dqdx, GLfloat dqdy, GLfloat texW, GLfloat texH,
                       GLfloat s, GLfloat t, GLfloat q, GLfloat invQ);

extern void
_swrast_write_index_span( GLcontext *ctx, struct sw_span *span);


extern void
_swrast_write_rgba_span( GLcontext *ctx, struct sw_span *span);


extern void
_swrast_read_rgba_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y, GLchan rgba[][4] );

extern void
_swrast_read_index_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y, GLuint indx[] );

extern void
_swrast_get_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, const GLint x[], const GLint y[],
                   void *values, GLuint valueSize);

#endif
