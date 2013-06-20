/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef DRAWTEX_H
#define DRAWTEX_H


#include "glheader.h"
#include "mfeatures.h"


#if FEATURE_OES_draw_texture

extern void GLAPIENTRY
_mesa_DrawTexf(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height);

extern void GLAPIENTRY
_mesa_DrawTexfv(const GLfloat *coords);

extern void GLAPIENTRY
_mesa_DrawTexi(GLint x, GLint y, GLint z, GLint width, GLint height);

extern void GLAPIENTRY
_mesa_DrawTexiv(const GLint *coords);

extern void GLAPIENTRY
_mesa_DrawTexs(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height);

extern void GLAPIENTRY
_mesa_DrawTexsv(const GLshort *coords);

extern void GLAPIENTRY
_mesa_DrawTexx(GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height);

extern void GLAPIENTRY
_mesa_DrawTexxv(const GLfixed *coords);

#endif /* FEATURE_OES_draw_texture */


#endif /* DRAWTEX_H */
