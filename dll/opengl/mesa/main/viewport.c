/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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


/**
 * \file viewport.c
 * glViewport and glDepthRange functions.
 */

#include <precomp.h>

/**
 * Set the viewport.
 * \sa Called via glViewport() or display list execution.
 *
 * Flushes the vertices and calls _mesa_set_viewport() with the given
 * parameters.
 */
void GLAPIENTRY
_mesa_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);
   _mesa_set_viewport(ctx, x, y, width, height);
}


/**
 * Set new viewport parameters and update derived state (the _WindowMap
 * matrix).  Usually called from _mesa_Viewport().
 * 
 * \param ctx GL context.
 * \param x, y coordinates of the lower left corner of the viewport rectangle.
 * \param width width of the viewport rectangle.
 * \param height height of the viewport rectangle.
 */
void
_mesa_set_viewport(struct gl_context *ctx, GLint x, GLint y,
                    GLsizei width, GLsizei height)
{
   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewport %d %d %d %d\n", x, y, width, height);

   if (width < 0 || height < 0) {
      _mesa_error(ctx,  GL_INVALID_VALUE,
                   "glViewport(%d, %d, %d, %d)", x, y, width, height);
      return;
   }

   /* clamp width and height to the implementation dependent range */
   width  = MIN2(width, (GLsizei) ctx->Const.MaxViewportWidth);
   height = MIN2(height, (GLsizei) ctx->Const.MaxViewportHeight);

   ctx->Viewport.X = x;
   ctx->Viewport.Width = width;
   ctx->Viewport.Y = y;
   ctx->Viewport.Height = height;
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif

   if (ctx->Driver.Viewport) {
      /* Many drivers will use this call to check for window size changes
       * and reallocate the z/stencil/accum/etc buffers if needed.
       */
      ctx->Driver.Viewport(ctx, x, y, width, height);
   }
}


/**
 * Called by glDepthRange
 *
 * \param nearval  specifies the Z buffer value which should correspond to
 *                 the near clip plane
 * \param farval  specifies the Z buffer value which should correspond to
 *                the far clip plane
 */
void GLAPIENTRY
_mesa_DepthRange(GLclampd nearval, GLclampd farval)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glDepthRange %f %f\n", nearval, farval);

   if (ctx->Viewport.Near == nearval &&
       ctx->Viewport.Far == farval)
      return;

   ctx->Viewport.Near = (GLfloat) CLAMP(nearval, 0.0, 1.0);
   ctx->Viewport.Far = (GLfloat) CLAMP(farval, 0.0, 1.0);
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->Viewport._WindowMap,
                         ctx->Viewport.X, ctx->Viewport.Y,
                         ctx->Viewport.Width, ctx->Viewport.Height,
                         ctx->Viewport.Near, ctx->Viewport.Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif

   if (ctx->Driver.DepthRange) {
      ctx->Driver.DepthRange(ctx, nearval, farval);
   }
}

/** 
 * Initialize the context viewport attribute group.
 * \param ctx  the GL context.
 */
void _mesa_init_viewport(struct gl_context *ctx)
{
   GLfloat depthMax = 65535.0F; /* sorf of arbitrary */

   /* Viewport group */
   ctx->Viewport.X = 0;
   ctx->Viewport.Y = 0;
   ctx->Viewport.Width = 0;
   ctx->Viewport.Height = 0;
   ctx->Viewport.Near = 0.0;
   ctx->Viewport.Far = 1.0;
   _math_matrix_ctr(&ctx->Viewport._WindowMap);

   _math_matrix_viewport(&ctx->Viewport._WindowMap, 0, 0, 0, 0,
                         0.0F, 1.0F, depthMax);
}


/** 
 * Free the context viewport attribute group data.
 * \param ctx  the GL context.
 */
void _mesa_free_viewport_data(struct gl_context *ctx)
{
   _math_matrix_dtr(&ctx->Viewport._WindowMap);
}

