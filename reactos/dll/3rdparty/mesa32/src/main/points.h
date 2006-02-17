/**
 * \file points.h
 * Point operations.
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


#ifndef POINTS_H
#define POINTS_H


#include "mtypes.h"


extern void GLAPIENTRY
_mesa_PointSize( GLfloat size );

extern void GLAPIENTRY
_mesa_PointParameteriNV( GLenum pname, GLint param );

extern void GLAPIENTRY
_mesa_PointParameterivNV( GLenum pname, const GLint *params );

extern void GLAPIENTRY
_mesa_PointParameterfEXT( GLenum pname, GLfloat param );

extern void GLAPIENTRY
_mesa_PointParameterfvEXT( GLenum pname, const GLfloat *params );

extern void
_mesa_update_point(GLcontext *ctx);

extern void 
_mesa_init_point( GLcontext * ctx );


#endif
