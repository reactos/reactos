/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


#ifndef TEXGEN_H
#define TEXGEN_H


#include "compiler.h"
#include "glheader.h"
#include "mfeatures.h"

struct _glapi_table;


#if FEATURE_texgen

extern void GLAPIENTRY
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params );

extern void GLAPIENTRY
_mesa_TexGenf( GLenum coord, GLenum pname, GLfloat param );

extern void GLAPIENTRY
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param );

extern void GLAPIENTRY
_mesa_GetTexGenfv( GLenum coord, GLenum pname, GLfloat *params );

extern void
_mesa_init_texgen_dispatch(struct _glapi_table *disp);


extern void GLAPIENTRY
_es_GetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);

extern void GLAPIENTRY
_es_TexGenf(GLenum coord, GLenum pname, GLfloat param);

extern void GLAPIENTRY
_es_TexGenfv(GLenum coord, GLenum pname, const GLfloat *params);


#else /* FEATURE_texgen */

static void
_mesa_TexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
}

static void inline
_mesa_TexGeni( GLenum coord, GLenum pname, GLint param )
{
}

static inline void
_mesa_init_texgen_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_texgen */


#endif /* TEXGEN_H */
