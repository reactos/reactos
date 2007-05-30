/**
 * \file get.h
 * State query functions.
 */

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


#ifndef GET_H
#define GET_H


#include "mtypes.h"


extern void GLAPIENTRY
_mesa_GetBooleanv( GLenum pname, GLboolean *params );

extern void GLAPIENTRY
_mesa_GetDoublev( GLenum pname, GLdouble *params );

extern void GLAPIENTRY
_mesa_GetFloatv( GLenum pname, GLfloat *params );

extern void GLAPIENTRY
_mesa_GetIntegerv( GLenum pname, GLint *params );

extern void GLAPIENTRY
_mesa_GetPointerv( GLenum pname, GLvoid **params );

extern const GLubyte * GLAPIENTRY
_mesa_GetString( GLenum name );

extern GLenum GLAPIENTRY
_mesa_GetError( void );

#endif
