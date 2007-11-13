/**
 * \file rastpos.h
 * Raster position operations.
 */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
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


#ifndef RASTPOS_H
#define RASTPOS_H


#include "glheader.h"


extern void GLAPIENTRY
_mesa_RasterPos2d(GLdouble x, GLdouble y);

extern void GLAPIENTRY
_mesa_RasterPos2f(GLfloat x, GLfloat y);

extern void GLAPIENTRY
_mesa_RasterPos2i(GLint x, GLint y);

extern void GLAPIENTRY
_mesa_RasterPos2s(GLshort x, GLshort y);

extern void GLAPIENTRY
_mesa_RasterPos3d(GLdouble x, GLdouble y, GLdouble z);

extern void GLAPIENTRY
_mesa_RasterPos3f(GLfloat x, GLfloat y, GLfloat z);

extern void GLAPIENTRY
_mesa_RasterPos3i(GLint x, GLint y, GLint z);

extern void GLAPIENTRY
_mesa_RasterPos3s(GLshort x, GLshort y, GLshort z);

extern void GLAPIENTRY
_mesa_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void GLAPIENTRY
_mesa_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void GLAPIENTRY
_mesa_RasterPos4i(GLint x, GLint y, GLint z, GLint w);

extern void GLAPIENTRY
_mesa_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);

extern void GLAPIENTRY
_mesa_RasterPos2dv(const GLdouble *v);

extern void GLAPIENTRY
_mesa_RasterPos2fv(const GLfloat *v);

extern void GLAPIENTRY
_mesa_RasterPos2iv(const GLint *v);

extern void GLAPIENTRY
_mesa_RasterPos2sv(const GLshort *v);

extern void GLAPIENTRY
_mesa_RasterPos3dv(const GLdouble *v);

extern void GLAPIENTRY
_mesa_RasterPos3fv(const GLfloat *v);

extern void GLAPIENTRY
_mesa_RasterPos3iv(const GLint *v);

extern void GLAPIENTRY
_mesa_RasterPos3sv(const GLshort *v);

extern void GLAPIENTRY
_mesa_RasterPos4dv(const GLdouble *v);

extern void GLAPIENTRY
_mesa_RasterPos4fv(const GLfloat *v);

extern void GLAPIENTRY
_mesa_RasterPos4iv(const GLint *v);

extern void GLAPIENTRY
_mesa_RasterPos4sv(const GLshort *v);


/**********************************************************************/
/** \name GL_MESA_window_pos                                          */
/**********************************************************************/
/*@{*/

extern void GLAPIENTRY
_mesa_WindowPos2dMESA(GLdouble x, GLdouble y);

extern void GLAPIENTRY
_mesa_WindowPos2fMESA(GLfloat x, GLfloat y);

extern void GLAPIENTRY
_mesa_WindowPos2iMESA(GLint x, GLint y);

extern void GLAPIENTRY
_mesa_WindowPos2sMESA(GLshort x, GLshort y);

extern void GLAPIENTRY
_mesa_WindowPos3dMESA(GLdouble x, GLdouble y, GLdouble z);

extern void GLAPIENTRY
_mesa_WindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z);

extern void GLAPIENTRY
_mesa_WindowPos3iMESA(GLint x, GLint y, GLint z);

extern void GLAPIENTRY
_mesa_WindowPos3sMESA(GLshort x, GLshort y, GLshort z);

extern void GLAPIENTRY
_mesa_WindowPos4dMESA(GLdouble x, GLdouble y, GLdouble z, GLdouble w);

extern void GLAPIENTRY
_mesa_WindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

extern void GLAPIENTRY
_mesa_WindowPos4iMESA(GLint x, GLint y, GLint z, GLint w);

extern void GLAPIENTRY
_mesa_WindowPos4sMESA(GLshort x, GLshort y, GLshort z, GLshort w);

extern void GLAPIENTRY
_mesa_WindowPos2dvMESA(const GLdouble *v);

extern void GLAPIENTRY
_mesa_WindowPos2fvMESA(const GLfloat *v);

extern void GLAPIENTRY
_mesa_WindowPos2ivMESA(const GLint *v);

extern void GLAPIENTRY
_mesa_WindowPos2svMESA(const GLshort *v);

extern void GLAPIENTRY
_mesa_WindowPos3dvMESA(const GLdouble *v);

extern void GLAPIENTRY
_mesa_WindowPos3fvMESA(const GLfloat *v);

extern void GLAPIENTRY
_mesa_WindowPos3ivMESA(const GLint *v);

extern void GLAPIENTRY
_mesa_WindowPos3svMESA(const GLshort *v);

extern void GLAPIENTRY
_mesa_WindowPos4dvMESA(const GLdouble *v);

extern void GLAPIENTRY
_mesa_WindowPos4fvMESA(const GLfloat *v);

extern void GLAPIENTRY
_mesa_WindowPos4ivMESA(const GLint *v);

extern void GLAPIENTRY
_mesa_WindowPos4svMESA(const GLshort *v);

extern void 
_mesa_init_rastpos( GLcontext * ctx );

/*@}*/

#endif
