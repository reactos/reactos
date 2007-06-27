/* $Id: mmx.h,v 1.9 2002/04/19 20:12:30 jrfonseca Exp $ */

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


#ifndef ASM_MMX_H
#define ASM_MMX_H

extern void _ASMAPI
_mesa_mmx_blend_transparency( GLcontext *ctx, GLuint n, const GLubyte mask[],
                              GLubyte rgba[][4], const GLubyte dest[][4] );

extern void _ASMAPI
_mesa_mmx_blend_add( GLcontext *ctx, GLuint n, const GLubyte mask[],
                     GLubyte rgba[][4], const GLubyte dest[][4] );

extern void _ASMAPI
_mesa_mmx_blend_min( GLcontext *ctx, GLuint n, const GLubyte mask[],
                     GLubyte rgba[][4], const GLubyte dest[][4] );

extern void _ASMAPI
_mesa_mmx_blend_max( GLcontext *ctx, GLuint n, const GLubyte mask[],
                     GLubyte rgba[][4], const GLubyte dest[][4] );

extern void _ASMAPI
_mesa_mmx_blend_modulate( GLcontext *ctx, GLuint n, const GLubyte mask[],
                          GLubyte rgba[][4], const GLubyte dest[][4] );

#endif
