/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "bufferobj.h"
#include "colormac.h"
#include "histogram.h"
#include "macros.h"
#include "mfeatures.h"
#include "main/dispatch.h"


#if FEATURE_histogram

/**********************************************************************
 * API functions
 */


/* this is defined below */
static void GLAPIENTRY _mesa_ResetMinmax(GLenum target);


static void GLAPIENTRY
_mesa_GetnMinmaxARB(GLenum target, GLboolean reset, GLenum format,
                    GLenum type, GLsizei bufSize, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmax");
}


static void GLAPIENTRY
_mesa_GetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type,
                GLvoid *values)
{
   _mesa_GetnMinmaxARB(target, reset, format, type, INT_MAX, values);
}


static void GLAPIENTRY
_mesa_GetnHistogramARB(GLenum target, GLboolean reset, GLenum format,
                       GLenum type, GLsizei bufSize, GLvoid *values)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogram");
}


static void GLAPIENTRY
_mesa_GetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type,
                   GLvoid *values)
{
   _mesa_GetnHistogramARB(target, reset, format, type, INT_MAX, values);
}


static void GLAPIENTRY
_mesa_GetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameterfv");
}


static void GLAPIENTRY
_mesa_GetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetHistogramParameteriv");
}


static void GLAPIENTRY
_mesa_GetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);

      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameterfv");
}


static void GLAPIENTRY
_mesa_GetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetMinmaxParameteriv");
}


static void GLAPIENTRY
_mesa_Histogram(GLenum target, GLsizei width, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glHistogram");
}


static void GLAPIENTRY
_mesa_Minmax(GLenum target, GLenum internalFormat, GLboolean sink)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glMinmax");
}


static void GLAPIENTRY
_mesa_ResetHistogram(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glResetHistogram");
}


static void GLAPIENTRY
_mesa_ResetMinmax(GLenum target)
{
   GET_CURRENT_CONTEXT(ctx);

   _mesa_error(ctx, GL_INVALID_OPERATION, "glResetMinmax");
}


void
_mesa_init_histogram_dispatch(struct _glapi_table *disp)
{
   SET_GetHistogram(disp, _mesa_GetHistogram);
   SET_GetHistogramParameterfv(disp, _mesa_GetHistogramParameterfv);
   SET_GetHistogramParameteriv(disp, _mesa_GetHistogramParameteriv);
   SET_GetMinmax(disp, _mesa_GetMinmax);
   SET_GetMinmaxParameterfv(disp, _mesa_GetMinmaxParameterfv);
   SET_GetMinmaxParameteriv(disp, _mesa_GetMinmaxParameteriv);
   SET_Histogram(disp, _mesa_Histogram);
   SET_Minmax(disp, _mesa_Minmax);
   SET_ResetHistogram(disp, _mesa_ResetHistogram);
   SET_ResetMinmax(disp, _mesa_ResetMinmax);

   /* GL_ARB_robustness */
   SET_GetnHistogramARB(disp, _mesa_GetnHistogramARB);
   SET_GetnMinmaxARB(disp, _mesa_GetnMinmaxARB);
}

#endif /* FEATURE_histogram */
