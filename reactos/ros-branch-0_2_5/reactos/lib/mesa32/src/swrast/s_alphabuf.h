
/*
 * Mesa 3-D graphics library
 * Version:  4.0.2
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


#ifndef S_ALPHABUF_H
#define S_ALPHABUF_H


#include "mtypes.h"
#include "swrast.h"


extern void
_swrast_alloc_alpha_buffers( GLframebuffer *buffer );


extern void
_swrast_clear_alpha_buffers( GLcontext *ctx );


extern void
_swrast_write_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        CONST GLchan rgba[][4], const GLubyte mask[] );


extern void
_swrast_write_mono_alpha_span( GLcontext *ctx,
                             GLuint n, GLint x, GLint y,
                             GLchan alpha, const GLubyte mask[] );



extern void
_swrast_write_alpha_pixels( GLcontext* ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          CONST GLchan rgba[][4],
                          const GLubyte mask[] );


extern void
_swrast_write_mono_alpha_pixels( GLcontext* ctx,
                               GLuint n, const GLint x[],
                               const GLint y[], GLchan alpha,
                               const GLubyte mask[] );


extern void
_swrast_read_alpha_span( GLcontext* ctx,
                       GLuint n, GLint x, GLint y, GLchan rgba[][4] );


extern void
_swrast_read_alpha_pixels( GLcontext* ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLchan rgba[][4], const GLubyte mask[] );


#endif
