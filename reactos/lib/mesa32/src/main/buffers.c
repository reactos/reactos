/**
 * \file buffers.c
 * Frame buffer management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.1
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
#include "imports.h"
#include "buffers.h"
#include "colormac.h"
#include "context.h"
#include "depth.h"
#include "enums.h"
#include "stencil.h"
#include "state.h"
#include "mtypes.h"


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

   if (!ctx->Visual.rgbMode && ctx->Driver.ClearIndex) {
      /* it's OK to call glClearIndex in RGBA mode but it should be a NOP */
      (*ctx->Driver.ClearIndex)( ctx, ctx->Color.ClearIndex );
   }
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

   tmp[0] = CLAMP(red,   0.0F, 1.0F);
   tmp[1] = CLAMP(green, 0.0F, 1.0F);
   tmp[2] = CLAMP(blue,  0.0F, 1.0F);
   tmp[3] = CLAMP(alpha, 0.0F, 1.0F);

   if (TEST_EQ_4V(tmp, ctx->Color.ClearColor))
      return; /* no change */

   FLUSH_VERTICES(ctx, _NEW_COLOR);
   COPY_4V(ctx->Color.ClearColor, tmp);

   if (ctx->Visual.rgbMode && ctx->Driver.ClearColor) {
      /* it's OK to call glClearColor in CI mode but it should be a NOP */
      (*ctx->Driver.ClearColor)(ctx, ctx->Color.ClearColor);
   }
}


/**
 * Clear buffers.
 * 
 * \param mask bit-mask indicating the buffers to be cleared.
 *
 * Flushes the vertices and verifies the parameter. If __GLcontextRec::NewState
 * is set then calls _mesa_update_state() to update gl_frame_buffer::_Xmin,
 * etc. If the rasterization mode is set to GL_RENDER then requests the driver
 * to clear the buffers, via the dd_function_table::Clear callback.
 */ 
void GLAPIENTRY
_mesa_Clear( GLbitfield mask )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

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

   if (ctx->RenderMode==GL_RENDER) {
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint y = ctx->DrawBuffer->_Ymin;
      const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
      const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
      GLbitfield ddMask;

      /* don't clear depth buffer if depth writing disabled */
      if (!ctx->Depth.Mask)
         mask &= ~GL_DEPTH_BUFFER_BIT;

      /* Build the bitmask to send to device driver's Clear function.
       * Note that the GL_COLOR_BUFFER_BIT flag will expand to 0, 1, 2 or 4
       * of the FRONT/BACK_LEFT/RIGHT_BIT flags.
       */
      ddMask = 0;
      if (mask & GL_COLOR_BUFFER_BIT)
         ddMask |= ctx->Color._DrawDestMask;
      if ((mask & GL_DEPTH_BUFFER_BIT) && ctx->Visual.depthBits > 0)
         ddMask |= GL_DEPTH_BUFFER_BIT;
      if ((mask & GL_STENCIL_BUFFER_BIT) && ctx->Visual.stencilBits > 0)
         ddMask |= GL_STENCIL_BUFFER_BIT;
      if ((mask & GL_ACCUM_BUFFER_BIT) && ctx->Visual.accumRedBits > 0)
         ddMask |= GL_ACCUM_BUFFER_BIT;

      ASSERT(ctx->Driver.Clear);
      ctx->Driver.Clear( ctx, ddMask, (GLboolean) !ctx->Scissor.Enabled,
			 x, y, width, height );
   }
}


/**
 * Specify which color buffers to draw into.
 *
 * \param mode color buffer combination.
 *
 * \sa glDrawBuffer().
 *
 * Flushes the vertices and verifies the parameter and updates the
 * gl_colorbuffer_attrib::_DrawDestMask bitfield. Marks new color state in
 * __GLcontextRec::NewState and notifies the driver via the
 * dd_function_table::DrawBuffer callback.
 */
void GLAPIENTRY
_mesa_DrawBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex... */

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDrawBuffer %s\n", _mesa_lookup_enum_by_nr(mode));

   /*
    * Do error checking and compute the _DrawDestMask bitfield.
    */
   switch (mode) {
      case GL_FRONT:
         /* never an error */
         if (ctx->Visual.stereoMode)
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT | DD_FRONT_RIGHT_BIT;
         else
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT;
         break;
      case GL_BACK:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_BACK)");
            return;
         }
         if (ctx->Visual.stereoMode)
            ctx->Color._DrawDestMask = DD_BACK_LEFT_BIT | DD_BACK_RIGHT_BIT;
         else
            ctx->Color._DrawDestMask = DD_BACK_LEFT_BIT;
         break;
      case GL_NONE:
         /* never an error */
         ctx->Color._DrawDestMask = 0;
         break;
#if _HAVE_FULL_GL
      case GL_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_RIGHT)");
            return;}
         if (ctx->Visual.doubleBufferMode)
            ctx->Color._DrawDestMask = DD_FRONT_RIGHT_BIT | DD_BACK_RIGHT_BIT;
         else
            ctx->Color._DrawDestMask = DD_FRONT_RIGHT_BIT;
         break;
      case GL_FRONT_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_FRONT_RIGHT)");
            return;
         }
         ctx->Color._DrawDestMask = DD_FRONT_RIGHT_BIT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual.stereoMode || !ctx->Visual.doubleBufferMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_BACK_RIGHT)");
            return;
         }
         ctx->Color._DrawDestMask = DD_BACK_RIGHT_BIT;
         break;
      case GL_BACK_LEFT:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_BACK_LEFT)");
            return;
         }
         ctx->Color._DrawDestMask = DD_BACK_LEFT_BIT;
         break;
      case GL_FRONT_AND_BACK:
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_FRONT_AND_BACK)");
            return;
         }
         if (ctx->Visual.stereoMode)
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT
                                    | DD_FRONT_RIGHT_BIT | DD_BACK_RIGHT_BIT;
         else
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT;
         break;
      case GL_LEFT:
         /* never an error */
         if (ctx->Visual.doubleBufferMode)
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT;
         else
            ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT;
         break;
      case GL_FRONT_LEFT:
         /* never an error */
         ctx->Color._DrawDestMask = DD_FRONT_LEFT_BIT;
         break;
      case GL_AUX0:
         if (ctx->Visual.numAuxBuffers >= 1) {
            ctx->Color._DrawDestMask = DD_AUX0_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_AUX0)" );
            return;
         }
         break;
      case GL_AUX1:
         if (ctx->Visual.numAuxBuffers >= 2) {
            ctx->Color._DrawDestMask = DD_AUX1_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_AUX1)" );
            return;
         }
         break;
      case GL_AUX2:
         if (ctx->Visual.numAuxBuffers >= 3) {
            ctx->Color._DrawDestMask = DD_AUX2_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_AUX2)" );
            return;
         }
         break;
      case GL_AUX3:
         if (ctx->Visual.numAuxBuffers >= 4) {
            ctx->Color._DrawDestMask = DD_AUX3_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glDrawBuffer(GL_AUX3)" );
            return;
         }
         break;
#endif
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glDrawBuffer" );
         return;
   }

   ctx->Color.DrawBuffer = mode;
   ctx->NewState |= _NEW_COLOR;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.DrawBuffer)
      (*ctx->Driver.DrawBuffer)(ctx, mode);
}


/**
 * Set the color buffer source for reading pixels.
 *
 * \param mode color buffer.
 *
 * \sa glReadBuffer().
 *
 * Verifies the parameter and updates gl_pixel_attrib::_ReadSrcMask.  Marks
 * new pixel state in __GLcontextRec::NewState and notifies the driver via
 * dd_function_table::ReadBuffer.
 */
void GLAPIENTRY
_mesa_ReadBuffer( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadBuffer %s\n", _mesa_lookup_enum_by_nr(mode));

   /*
    * Do error checking and compute ctx->Pixel._ReadSrcMask.
    */
   switch (mode) {
      case GL_LEFT:
      case GL_FRONT:
      case GL_FRONT_LEFT:
         /* Front-Left buffer, always exists */
         ctx->Pixel._ReadSrcMask = DD_FRONT_LEFT_BIT;
         break;
      case GL_BACK:
      case GL_BACK_LEFT:
         /* Back-Left buffer, requires double buffering */
         if (!ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel._ReadSrcMask = DD_BACK_LEFT_BIT;
         break;
#if _HAVE_FULL_GL
      case GL_FRONT_RIGHT:
      case GL_RIGHT:
         if (!ctx->Visual.stereoMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel._ReadSrcMask = DD_FRONT_RIGHT_BIT;
         break;
      case GL_BACK_RIGHT:
         if (!ctx->Visual.stereoMode || !ctx->Visual.doubleBufferMode) {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer" );
            return;
         }
         ctx->Pixel._ReadSrcMask = DD_BACK_RIGHT_BIT;
         break;
      case GL_AUX0:
         if (ctx->Visual.numAuxBuffers >= 1) {
            ctx->Pixel._ReadSrcMask = DD_AUX0_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer(GL_AUX0)" );
            return;
         }
         break;
      case GL_AUX1:
         if (ctx->Visual.numAuxBuffers >= 2) {
            ctx->Pixel._ReadSrcMask = DD_AUX1_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer(GL_AUX1)" );
            return;
         }
         break;
      case GL_AUX2:
         if (ctx->Visual.numAuxBuffers >= 3) {
            ctx->Pixel._ReadSrcMask = DD_AUX2_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer(GL_AUX2)" );
            return;
         }
         break;
      case GL_AUX3:
         if (ctx->Visual.numAuxBuffers >= 4) {
            ctx->Pixel._ReadSrcMask = DD_AUX3_BIT;
         }
         else {
            _mesa_error( ctx, GL_INVALID_OPERATION, "glReadBuffer(GL_AUX3)" );
            return;
         }
         break;
#endif
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glReadBuffer" );
         return;
   }

   ctx->Pixel.ReadBuffer = mode;
   ctx->NewState |= _NEW_PIXEL;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.ReadBuffer)
      (*ctx->Driver.ReadBuffer)(ctx, mode);
}

#if _HAVE_FULL_GL

/**
 * GL_MESA_resize_buffers extension.
 *
 * When this function is called, we'll ask the window system how large
 * the current window is.  If it's a new size, we'll call the driver's
 * ResizeBuffers function.  The driver will then resize its color buffers
 * as needed, and maybe call the swrast's routine for reallocating
 * swrast-managed depth/stencil/accum/etc buffers.
 * \note This function may be called from within Mesa or called by the
 * user directly (see the GL_MESA_resize_buffers extension).
 */
void GLAPIENTRY
_mesa_ResizeBuffersMESA( void )
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glResizeBuffersMESA\n");

   if (ctx) {
      ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );

      if (ctx->DrawBuffer) {
         GLuint buf_width, buf_height;
         GLframebuffer *buffer = ctx->DrawBuffer;

         /* ask device driver for size of output buffer */
         (*ctx->Driver.GetBufferSize)( buffer, &buf_width, &buf_height );

         /* see if size of device driver's color buffer (window) has changed */
         if (buffer->Width == buf_width && buffer->Height == buf_height)
            return; /* size is as expected */

         buffer->Width = buf_width;
         buffer->Height = buf_height;

         ctx->Driver.ResizeBuffers( buffer );
      }

      if (ctx->ReadBuffer && ctx->ReadBuffer != ctx->DrawBuffer) {
         GLuint buf_width, buf_height;
         GLframebuffer *buffer = ctx->ReadBuffer;

         /* ask device driver for size of read buffer */
         (*ctx->Driver.GetBufferSize)( buffer, &buf_width, &buf_height );

         /* see if size of device driver's color buffer (window) has changed */
         if (buffer->Width == buf_width && buffer->Height == buf_height)
            return; /* size is as expected */

         buffer->Width = buf_width;
         buffer->Height = buf_height;

         ctx->Driver.ResizeBuffers( buffer );
      }

      ctx->NewState |= _NEW_BUFFERS;  /* to update scissor / window bounds */
   }
}

/*
 * XXX move somewhere else someday?
 */
void GLAPIENTRY
_mesa_SampleCoverageARB(GLclampf value, GLboolean invert)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.ARB_multisample) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleCoverageARB");
      return;
   }

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );
   ctx->Multisample.SampleCoverageValue = (GLfloat) CLAMP(value, 0.0, 1.0);
   ctx->Multisample.SampleCoverageInvert = invert;
   ctx->NewState |= _NEW_MULTISAMPLE;
}

#endif


/**
 * Define the scissor box.
 *
 * \param x, y coordinates of the scissor box lower-left corner.
 * \param width width of the scissor box.
 * \param height height of the scissor box.
 *
 * \sa glScissor().
 *
 * Verifies the parameters and updates __GLcontextRec::Scissor. On a
 * change flushes the vertices and notifies the driver via
 * the dd_function_table::Scissor callback.
 */
void GLAPIENTRY
_mesa_Scissor( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glScissor" );
      return;
   }

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glScissor %d %d %d %d\n", x, y, width, height);

   if (x == ctx->Scissor.X &&
       y == ctx->Scissor.Y &&
       width == ctx->Scissor.Width &&
       height == ctx->Scissor.Height)
      return;

   FLUSH_VERTICES(ctx, _NEW_SCISSOR);
   ctx->Scissor.X = x;
   ctx->Scissor.Y = y;
   ctx->Scissor.Width = width;
   ctx->Scissor.Height = height;

   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor( ctx, x, y, width, height );
}

/**********************************************************************/
/** \name State management */
/*@{*/

/**
 * Update screen bounds.
 *
 * \param ctx GL context.
 *
 * Update gl_frame_buffer::_Xmin, and etc.
 */
void _mesa_update_buffers( GLcontext *ctx )
{
   ctx->DrawBuffer->_Xmin = 0;
   ctx->DrawBuffer->_Ymin = 0;
   ctx->DrawBuffer->_Xmax = ctx->DrawBuffer->Width;
   ctx->DrawBuffer->_Ymax = ctx->DrawBuffer->Height;
   if (ctx->Scissor.Enabled) {
      if (ctx->Scissor.X > ctx->DrawBuffer->_Xmin) {
	 ctx->DrawBuffer->_Xmin = ctx->Scissor.X;
      }
      if (ctx->Scissor.Y > ctx->DrawBuffer->_Ymin) {
	 ctx->DrawBuffer->_Ymin = ctx->Scissor.Y;
      }
      if (ctx->Scissor.X + ctx->Scissor.Width < ctx->DrawBuffer->_Xmax) {
	 ctx->DrawBuffer->_Xmax = ctx->Scissor.X + ctx->Scissor.Width;
      }
      if (ctx->Scissor.Y + ctx->Scissor.Height < ctx->DrawBuffer->_Ymax) {
	 ctx->DrawBuffer->_Ymax = ctx->Scissor.Y + ctx->Scissor.Height;
      }
   }
}

/*@}*/

			   
/**********************************************************************/
/** \name Initialization */
/*@{*/

/**
 * Initialize the context scissor data.
 *
 * \param ctx GL context.
 * 
 * Initializes the __GLcontextRec::Scissor and __GLcontextRec::Multisample
 * attribute groups, and related constants in __GLcontextRec::Const.
 */
void _mesa_init_buffers( GLcontext * ctx )
{
   /* Scissor group */
   ctx->Scissor.Enabled = GL_FALSE;
   ctx->Scissor.X = 0;
   ctx->Scissor.Y = 0;
   ctx->Scissor.Width = 0;
   ctx->Scissor.Height = 0;

   /* Multisample */
   ctx->Multisample.Enabled = GL_FALSE;
   ctx->Multisample.SampleAlphaToCoverage = GL_FALSE;
   ctx->Multisample.SampleAlphaToOne = GL_FALSE;
   ctx->Multisample.SampleCoverage = GL_FALSE;
   ctx->Multisample.SampleCoverageValue = 1.0;
   ctx->Multisample.SampleCoverageInvert = GL_FALSE;

}

/*@}*/
