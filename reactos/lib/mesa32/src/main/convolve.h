
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


#ifndef CONVOLVE_H
#define CONVOLVE_H


#include "mtypes.h"


#if _HAVE_FULL_GL
extern void GLAPIENTRY
_mesa_ConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width,
                          GLenum format, GLenum type, const GLvoid *image);

extern void GLAPIENTRY
_mesa_ConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width,
                          GLsizei height, GLenum format, GLenum type,
                          const GLvoid *image);

extern void GLAPIENTRY
_mesa_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat params);

extern void GLAPIENTRY
_mesa_ConvolutionParameterfv(GLenum target, GLenum pname,
                             const GLfloat *params);

extern void GLAPIENTRY
_mesa_ConvolutionParameteri(GLenum target, GLenum pname, GLint params);

extern void GLAPIENTRY
_mesa_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params);

extern void GLAPIENTRY
_mesa_CopyConvolutionFilter1D(GLenum target, GLenum internalformat,
                              GLint x, GLint y, GLsizei width);

extern void GLAPIENTRY
_mesa_CopyConvolutionFilter2D(GLenum target, GLenum internalformat,
                              GLint x, GLint y, GLsizei width, GLsizei height);

extern void GLAPIENTRY
_mesa_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
                           GLvoid *image);

extern void GLAPIENTRY
_mesa_GetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params);

extern void GLAPIENTRY
_mesa_GetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
                         GLvoid *row, GLvoid *column, GLvoid *span);

extern void GLAPIENTRY
_mesa_SeparableFilter2D(GLenum target, GLenum internalformat,
                        GLsizei width, GLsizei height,
                        GLenum format, GLenum type,
                        const GLvoid *row, const GLvoid *column);



extern void
_mesa_convolve_1d_image(const GLcontext *ctx, GLsizei *width,
                        const GLfloat *srcImage, GLfloat *dstImage);


extern void
_mesa_convolve_2d_image(const GLcontext *ctx, GLsizei *width, GLsizei *height,
                        const GLfloat *srcImage, GLfloat *dstImage);


extern void
_mesa_convolve_sep_image(const GLcontext *ctx,
                         GLsizei *width, GLsizei *height,
                         const GLfloat *srcImage, GLfloat *dstImage);


extern void
_mesa_adjust_image_for_convolution(const GLcontext *ctx, GLuint dimensions,
                                   GLsizei *width, GLsizei *height);

#else
#define _mesa_adjust_image_for_convolution(c, d, w, h) ((void)0)
#define _mesa_convolve_1d_image(c,w,s,d) ((void)0)
#define _mesa_convolve_2d_image(c,w,h,s,d) ((void)0)
#define _mesa_convolve_sep_image(c,w,h,s,d) ((void)0)
#endif

#endif
