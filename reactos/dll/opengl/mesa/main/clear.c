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

/**
 * \file clear.c
 * glClearColor, glClearIndex, glClear() functions.
 */

#include <precomp.h>

#if _HAVE_FULL_GL
void GLAPIENTRY
_mesa_ClearIndex( GLfloat c )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->Color.ClearIndex == (GLuint) c)
      return;

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   ctx->Color.ClearIndex = (GLuint) c;
}
#endif


/**
 * Specify the clear values for the color buffers.
 *
 * \param red red color component.
 * \param green green color component.
 * \param blue blue color component.
 * \param alpha alpha component.
 *
 * \sa glClearColor().
 *
 * Clamps the parameters and updates gl_colorbuffer_attrib::ClearColor.  On a
 * change, flushes the vertices and notifies the driver via the
 * dd_function_table::ClearColor callback.
 */
void GLAPIENTRY
_mesa_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
   GLfloat tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = red;
   tmp[1] = green;
   tmp[2] = blue;
   tmp[3] = alpha;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.f))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.f, tmp);

   if (ctx->Driver.ClearColor) {
      /* it's OK to call glClearColor in CI mode but it should be a NOP */
      /* we pass the clamped color, since all drivers that need this don't
       * support GL_ARB_color_buffer_float
       */
      (*ctx->Driver.ClearColor)(ctx, ctx->Color.ClearColor);
   }
}


/**
 * GL_EXT_texture_integer
 */
void GLAPIENTRY
_mesa_ClearColorIiEXT(GLint r, GLint g, GLint b, GLint a)
{
   GLint tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = r;
   tmp[1] = g;
   tmp[2] = b;
   tmp[3] = a;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.i))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.i, tmp);

   /* these should be NOP calls for drivers supporting EXT_texture_integer */
   if (ctx->Driver.ClearColor) {
      ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
   }
}


/**
 * GL_EXT_texture_integer
 */
void GLAPIENTRY
_mesa_ClearColorIuiEXT(GLuint r, GLuint g, GLuint b, GLuint a)
{
   GLuint tmp[4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   tmp[0] = r;
   tmp[1] = g;
   tmp[2] = b;
   tmp[3] = a;

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor.ui))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor.ui, tmp);

   /* these should be NOP calls for drivers supporting EXT_texture_integer */
   if (ctx->Driver.ClearColor) {
      ctx->Driver.ClearColor(ctx, ctx->Color.ClearColor);
   }
}


/**
 * Clear buffers.
 * 
 * \param mask bit-mask indicating the buffers to be cleared.
 *
 * Flushes the vertices and verifies the parameter. If __struct gl_contextRec::NewState
 * is set then calls _mesa_update_state() to update gl_frame_buffer::_Xmin,
 * etc. If the rasterization mode is set to GL_RENDER then requests the driver
 * to clear the buffers, via the dd_function_table::Clear callback.
 */ 
void GLAPIENTRY
_mesa_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glClear 0x%x\n", mask);

   if (mask & ~(GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT |
                GL_ACCUM_BUFFER_BIT)) {
      /* invalid bit set */
      _mesa_error( ctx, GL_INVALID_VALUE, "glClear(0x%x)", mask);
      return;
   }

   if (ctx->NewState) {
      _mesa_update_state( ctx );	/* update _Xmin, etc */
   }

   if (ctx->DrawBuffer->Width == 0 || ctx->DrawBuffer->Height == 0 ||
       ctx->DrawBuffer->_Xmin >= ctx->DrawBuffer->_Xmax ||
       ctx->DrawBuffer->_Ymin >= ctx->DrawBuffer->_Ymax)
      return;

   if (ctx->RasterDiscard)
      return;

   if (ctx->RenderMode == GL_RENDER) {
      GLbitfield bufferMask;

      /* don't clear depth buffer if depth writing disabled */
      if (!ctx->Depth.Mask)
         mask &= ~GL_DEPTH_BUFFER_BIT;

      /* Build the bitmask to send to device driver's Clear function.
       * Note that the GL_COLOR_BUFFER_BIT flag will expand to 0, 1, 2 or 4
       * of the BUFFER_BIT_FRONT/BACK_LEFT/RIGHT flags, or one of the
       * BUFFER_BIT_COLORn flags.
       */
      bufferMask = 0;
      if (mask & GL_COLOR_BUFFER_BIT) {
         bufferMask |= (1 << ctx->DrawBuffer->_ColorDrawBufferIndex);
      }

      if ((mask & GL_DEPTH_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveDepthBuffer) {
         bufferMask |= BUFFER_BIT_DEPTH;
      }

      if ((mask & GL_STENCIL_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveStencilBuffer) {
         bufferMask |= BUFFER_BIT_STENCIL;
      }

      if ((mask & GL_ACCUM_BUFFER_BIT)
          && ctx->DrawBuffer->Visual.haveAccumBuffer) {
         bufferMask |= BUFFER_BIT_ACCUM;
      }

      ASSERT(ctx->Driver.Clear);
      ctx->Driver.Clear(ctx, bufferMask);
   }
}
