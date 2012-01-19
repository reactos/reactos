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

#ifndef _API_NOOP_H
#define _API_NOOP_H

#include "mtypes.h"
#include "context.h"

extern void GLAPIENTRY
_mesa_noop_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);

extern void GLAPIENTRY
_mesa_noop_EvalMesh1(GLenum mode, GLint i1, GLint i2);

extern void GLAPIENTRY
_mesa_noop_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);

extern void GLAPIENTRY
_mesa_noop_Materialfv(GLenum face, GLenum pname, const GLfloat *param);

extern void
_mesa_noop_vtxfmt_init(GLvertexformat *vfmt);

#endif
