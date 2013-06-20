/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
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


/*
 * Image convolution functions.
 *
 * Notes: filter kernel elements are indexed by <n> and <m> as in
 * the GL spec.
 */


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "convolve.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "main/dispatch.h"


#if FEATURE_convolve

static void GLAPIENTRY
_mesa_ConvolutionFilter1D(GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter1D");
}

static void GLAPIENTRY
_mesa_ConvolutionFilter2D(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionFilter2D");
}


static void GLAPIENTRY
_mesa_ConvolutionParameterf(GLenum target, GLenum pname, GLfloat param)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterf");
}


static void GLAPIENTRY
_mesa_ConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameterfv");
}


static void GLAPIENTRY
_mesa_ConvolutionParameteri(GLenum target, GLenum pname, GLint param)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteri");
}


static void GLAPIENTRY
_mesa_ConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glConvolutionParameteriv");
}


static void GLAPIENTRY
_mesa_CopyConvolutionFilter1D(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter1D");
}


static void GLAPIENTRY
_mesa_CopyConvolutionFilter2D(GLenum target, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyConvolutionFilter2D");
}


static void GLAPIENTRY
_mesa_GetnConvolutionFilterARB(GLenum target, GLenum format, GLenum type,
                               GLsizei bufSize, GLvoid *image)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetConvolutionFilter");
}


static void GLAPIENTRY
_mesa_GetConvolutionFilter(GLenum target, GLenum format, GLenum type,
                           GLvoid *image)
{
   _mesa_GetnConvolutionFilterARB(target, format, type, INT_MAX, image);
}


static void GLAPIENTRY
_mesa_GetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameterfv");
}


static void GLAPIENTRY
_mesa_GetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glGetConvolutionParameteriv");
}


static void GLAPIENTRY
_mesa_GetnSeparableFilterARB(GLenum target, GLenum format, GLenum type,
                             GLsizei rowBufSize, GLvoid *row,
                             GLsizei columnBufSize,  GLvoid *column,
                             GLvoid *span)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glGetSeparableFilter");
}


static void GLAPIENTRY
_mesa_GetSeparableFilter(GLenum target, GLenum format, GLenum type,
                         GLvoid *row, GLvoid *column, GLvoid *span)
{
   _mesa_GetnSeparableFilterARB(target, format, type, INT_MAX, row,
                                INT_MAX, column, span);
}


static void GLAPIENTRY
_mesa_SeparableFilter2D(GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_ENUM, "glSeparableFilter2D");
}

void
_mesa_init_convolve_dispatch(struct _glapi_table *disp)
{
   SET_ConvolutionFilter1D(disp, _mesa_ConvolutionFilter1D);
   SET_ConvolutionFilter2D(disp, _mesa_ConvolutionFilter2D);
   SET_ConvolutionParameterf(disp, _mesa_ConvolutionParameterf);
   SET_ConvolutionParameterfv(disp, _mesa_ConvolutionParameterfv);
   SET_ConvolutionParameteri(disp, _mesa_ConvolutionParameteri);
   SET_ConvolutionParameteriv(disp, _mesa_ConvolutionParameteriv);
   SET_CopyConvolutionFilter1D(disp, _mesa_CopyConvolutionFilter1D);
   SET_CopyConvolutionFilter2D(disp, _mesa_CopyConvolutionFilter2D);
   SET_GetConvolutionFilter(disp, _mesa_GetConvolutionFilter);
   SET_GetConvolutionParameterfv(disp, _mesa_GetConvolutionParameterfv);
   SET_GetConvolutionParameteriv(disp, _mesa_GetConvolutionParameteriv);
   SET_SeparableFilter2D(disp, _mesa_SeparableFilter2D);
   SET_GetSeparableFilter(disp, _mesa_GetSeparableFilter);

   /* GL_ARB_robustness */
   SET_GetnConvolutionFilterARB(disp, _mesa_GetnConvolutionFilterARB);
   SET_GetnSeparableFilterARB(disp, _mesa_GetnSeparableFilterARB);
}


#endif /* FEATURE_convolve */
