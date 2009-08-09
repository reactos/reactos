/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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


#include "glheader.h"
#include "context.h"
#include "depth.h"
#include "lines.h"
#include "macros.h"
#include "texstate.h"
#include "mtypes.h"


/**
 * Set the line width.
 *
 * \param width line width in pixels.
 *
 * \sa glLineWidth().
 */
void GLAPIENTRY
_mesa_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (width<=0.0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glLineWidth" );
      return;
   }

   if (ctx->Line.Width == width)
      return;

   FLUSH_VERTICES(ctx, _NEW_LINE);
   ctx->Line.Width = width;

   if (width != 1.0F)
      ctx->_TriangleCaps |= DD_LINE_WIDTH;
   else
      ctx->_TriangleCaps &= ~DD_LINE_WIDTH;

   if (ctx->Driver.LineWidth)
      ctx->Driver.LineWidth(ctx, width);
}


/**
 * Set the line stipple pattern.
 *
 * \param factor pattern scale factor.
 * \param pattern bit pattern.
 * 
 * \sa glLineStipple().
 *
 * Updates gl_line_attrib::StippleFactor and gl_line_attrib::StipplePattern. On
 * change flushes the vertices and notifies the driver via
 * the dd_function_table::LineStipple callback.
 */
void GLAPIENTRY
_mesa_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   factor = CLAMP( factor, 1, 256 );

   if (ctx->Line.StippleFactor == factor &&
       ctx->Line.StipplePattern == pattern)
      return;

   FLUSH_VERTICES(ctx, _NEW_LINE);
   ctx->Line.StippleFactor = factor;
   ctx->Line.StipplePattern = pattern;

   if (ctx->Driver.LineStipple)
      ctx->Driver.LineStipple( ctx, factor, pattern );
}


/**
 * Initialize the context line state.
 *
 * \param ctx GL context.
 *
 * Initializes __GLcontextRec::Line and line related constants in
 * __GLcontextRec::Const.
 */
void GLAPIENTRY
_mesa_init_line( GLcontext * ctx )
{
   ctx->Line.SmoothFlag = GL_FALSE;
   ctx->Line.StippleFlag = GL_FALSE;
   ctx->Line.Width = 1.0;
   ctx->Line.StipplePattern = 0xffff;
   ctx->Line.StippleFactor = 1;
}
