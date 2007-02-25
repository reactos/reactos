
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


#ifndef _M_TRANSLATE_H_
#define _M_TRANSLATE_H_

#include "config.h"
#include "mtypes.h"		/* hack for GLchan */



extern void _math_trans_1f(GLfloat *to,
			   CONST void *ptr,
			   GLuint stride,
			   GLenum type,
			   GLuint start,
			   GLuint n );

extern void _math_trans_1ui(GLuint *to,
			    CONST void *ptr,
			    GLuint stride,
			    GLenum type,
			    GLuint start,
			    GLuint n );

extern void _math_trans_1ub(GLubyte *to,
			    CONST void *ptr,
			    GLuint stride,
			    GLenum type,
			    GLuint start,
			    GLuint n );

extern void _math_trans_4ub(GLubyte (*to)[4],
			    CONST void *ptr,
			    GLuint stride,
			    GLenum type,
			    GLuint size,
			    GLuint start,
			    GLuint n );

extern void _math_trans_4chan( GLchan (*to)[4],
			       CONST void *ptr,
			       GLuint stride,
			       GLenum type,
			       GLuint size,
			       GLuint start,
			       GLuint n );

extern void _math_trans_4us(GLushort (*to)[4],
			    CONST void *ptr,
			    GLuint stride,
			    GLenum type,
			    GLuint size,
			    GLuint start,
			    GLuint n );

extern void _math_trans_4f(GLfloat (*to)[4],
			   CONST void *ptr,
			   GLuint stride,
			   GLenum type,
			   GLuint size,
			   GLuint start,
			   GLuint n );

extern void _math_trans_4fc(GLfloat (*to)[4],
			    CONST void *ptr,
			    GLuint stride,
			    GLenum type,
			    GLuint size,
			    GLuint start,
			    GLuint n );

extern void _math_trans_3f(GLfloat (*to)[3],
			   CONST void *ptr,
			   GLuint stride,
			   GLenum type,
			   GLuint start,
			   GLuint n );

extern void _math_init_translate( void );


#endif
