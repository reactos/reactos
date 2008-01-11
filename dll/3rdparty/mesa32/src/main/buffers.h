/**
 * \file buffers.h
 * Frame buffer management functions declarations.
 */

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



#ifndef BUFFERS_H
#define BUFFERS_H


#include "mtypes.h"


extern void GLAPIENTRY
_mesa_ClearIndex( GLfloat c );

extern void GLAPIENTRY
_mesa_ClearColor( GLclampf red, GLclampf green,
                  GLclampf blue, GLclampf alpha );

extern void GLAPIENTRY
_mesa_Clear( GLbitfield mask );

extern void GLAPIENTRY
_mesa_DrawBuffer( GLenum mode );

extern void GLAPIENTRY
_mesa_DrawBuffersARB(GLsizei n, const GLenum *buffers);

extern void
_mesa_drawbuffers(GLcontext *ctx, GLuint n, const GLenum *buffers,
                  const GLbitfield *destMask);

extern void GLAPIENTRY
_mesa_ReadBuffer( GLenum mode );

extern void GLAPIENTRY
_mesa_ResizeBuffersMESA( void );

extern void GLAPIENTRY
_mesa_Scissor( GLint x, GLint y, GLsizei width, GLsizei height );

extern void GLAPIENTRY
_mesa_SampleCoverageARB(GLclampf value, GLboolean invert);

extern void 
_mesa_init_scissor(GLcontext *ctx);

extern void 
_mesa_init_multisample(GLcontext *ctx);

extern void
_mesa_set_scissor(GLcontext *ctx, 
                  GLint x, GLint y, GLsizei width, GLsizei height);

extern void _mesa_resizebuffers( GLcontext *ctx );

#endif
