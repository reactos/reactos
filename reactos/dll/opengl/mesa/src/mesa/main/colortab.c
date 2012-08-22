/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
#include "colortab.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "mfeatures.h"
#include "mtypes.h"
#include "pack.h"
#include "pbo.h"
#include "state.h"
#include "teximage.h"
#include "texstate.h"
#include "main/dispatch.h"


#if FEATURE_colortable

void GLAPIENTRY
_mesa_ColorTable( GLenum target, GLenum internalFormat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
}



void GLAPIENTRY
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
}



static void GLAPIENTRY
_mesa_CopyColorTable(GLenum target, GLenum internalformat,
                     GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyColorTable(target)");
}



static void GLAPIENTRY
_mesa_CopyColorSubTable(GLenum target, GLsizei start,
                        GLint x, GLint y, GLsizei width)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyColorSubTable(target)");
}



static void GLAPIENTRY
_mesa_GetnColorTableARB( GLenum target, GLenum format, GLenum type,
                         GLsizei bufSize, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetnColorTableARB(target)");
}


static void GLAPIENTRY
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
}


static void GLAPIENTRY
_mesa_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   /* no extensions use this function */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(target)");
}



static void GLAPIENTRY
_mesa_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   /* no extensions use this function */
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glColorTableParameteriv(target)");
}



static void GLAPIENTRY
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(target)");
}



static void GLAPIENTRY
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(target)");
}


void
_mesa_init_colortable_dispatch(struct _glapi_table *disp)
{
   SET_ColorSubTable(disp, _mesa_ColorSubTable);
   SET_ColorTable(disp, _mesa_ColorTable);
   SET_ColorTableParameterfv(disp, _mesa_ColorTableParameterfv);
   SET_ColorTableParameteriv(disp, _mesa_ColorTableParameteriv);
   SET_CopyColorSubTable(disp, _mesa_CopyColorSubTable);
   SET_CopyColorTable(disp, _mesa_CopyColorTable);
   SET_GetColorTable(disp, _mesa_GetColorTable);
   SET_GetColorTableParameterfv(disp, _mesa_GetColorTableParameterfv);
   SET_GetColorTableParameteriv(disp, _mesa_GetColorTableParameteriv);

   /* GL_ARB_robustness */
   SET_GetnColorTableARB(disp, _mesa_GetnColorTableARB);
}


#endif /* FEATURE_colortable */
