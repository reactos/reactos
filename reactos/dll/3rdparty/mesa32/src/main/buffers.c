/**
 * \file buffers.c
 * Frame buffer management.
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
#include "buffers.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "fbobject.h"
#include "state.h"


#define BAD_MASK ~0u


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

   if (ctx->RenderMode == GL_RENDER) {
      const GLint x = ctx->DrawBuffer->_Xmin;
      const GLint y = ctx->DrawBuffer->_Ymin;
      const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
      const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
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
         bufferMask |= ctx->DrawBuffer->_ColorDrawBufferMask[0];
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
      ctx->Driver.Clear( ctx, bufferMask, (GLboolean) !ctx->Scissor.Enabled,
			 x, y, width, height );
   }
}



/**
 * Return bitmask of BUFFER_BIT_* flags indicating which color buffers are
 * available to the rendering context.
 * This depends on the framebuffer we're writing to.  For window system
 * framebuffers we look at the framebuffer's visual.  But for user-
 * create framebuffers we look at the number of supported color attachments.
 */
static GLuint
supported_buffer_bitmask(const GLcontext *ctx, GLuint framebufferID)
{
   GLuint mask = 0x0;
   GLint i;

   if (framebufferID > 0) {
      /* A user-created renderbuffer */
      ASSERT(ctx->Extensions.EXT_framebuffer_object);
      for (i = 0; i < ctx->Const.MaxColorAttachments; i++) {
         mask |= (BUFFER_BIT_COLOR0 << i);
      }
   }
   else {
      /* A window system renderbuffer */
      mask = BUFFER_BIT_FRONT_LEFT; /* always have this */
      if (ctx->Visual.stereoMode) {
         mask |= BUFFER_BIT_FRONT_RIGHT;
         if (ctx->Visual.doubleBufferMode) {
            mask |= BUFFER_BIT_BACK_LEFT | BUFFER_BIT_BACK_RIGHT;
         }
      }
      else if (ctx->Visual.doubleBufferMode) {
         mask |= BUFFER_BIT_BACK_LEFT;
      }

      for (i = 0; i < ctx->Visual.numAuxBuffers; i++) {
         mask |= (BUFFER_BIT_AUX0 << i);
      }
   }

   return mask;
}


/**
 * Helper routine used by glDrawBuffer and glDrawBuffersARB.
 * Given a GLenum naming one or more color buffers (such as
 * GL_FRONT_AND_BACK), return the corresponding bitmask of BUFFER_BIT_* flags.
 */
static GLuint
draw_buffer_enum_to_bitmask(GLenum buffer)
{
   switch (buffer) {
      case GL_NONE:
         return 0;
      case GL_FRONT:
         return BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_FRONT_RIGHT;
      case GL_BACK:
         return BUFFER_BIT_BACK_LEFT | BUFFER_BIT_BACK_RIGHT;
      case GL_RIGHT:
         return BUFFER_BIT_FRONT_RIGHT | BUFFER_BIT_BACK_RIGHT;
      case GL_FRONT_RIGHT:
         return BUFFER_BIT_FRONT_RIGHT;
      case GL_BACK_RIGHT:
         return BUFFER_BIT_BACK_RIGHT;
      case GL_BACK_LEFT:
         return BUFFER_BIT_BACK_LEFT;
      case GL_FRONT_AND_BACK:
         return BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT
              | BUFFER_BIT_FRONT_RIGHT | BUFFER_BIT_BACK_RIGHT;
      case GL_LEFT:
         return BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT;
      case GL_FRONT_LEFT:
         return BUFFER_BIT_FRONT_LEFT;
      case GL_AUX0:
         return BUFFER_BIT_AUX0;
      case GL_AUX1:
         return BUFFER_BIT_AUX1;
      case GL_AUX2:
         return BUFFER_BIT_AUX2;
      case GL_AUX3:
         return BUFFER_BIT_AUX3;
      case GL_COLOR_ATTACHMENT0_EXT:
         return BUFFER_BIT_COLOR0;
      case GL_COLOR_ATTACHMENT1_EXT:
         return BUFFER_BIT_COLOR1;
      case GL_COLOR_ATTACHMENT2_EXT:
         return BUFFER_BIT_COLOR2;
      case GL_COLOR_ATTACHMENT3_EXT:
         return BUFFER_BIT_COLOR3;
      default:
         /* error */
         return BAD_MASK;
   }
}


/**
 * Helper routine used by glReadBuffer.
 * Given a GLenum naming (a) color buffer(s), return the corresponding
 * bitmask of DD_* flags.
 */
static GLuint
read_buffer_enum_to_bitmask(GLenum buffer)
{
   switch (buffer) {
      case GL_FRONT:
         return BUFFER_BIT_FRONT_LEFT;
      case GL_BACK:
         return BUFFER_BIT_BACK_LEFT;
      case GL_RIGHT:
         return BUFFER_BIT_FRONT_RIGHT;
      case GL_FRONT_RIGHT:
         return BUFFER_BIT_FRONT_RIGHT;
      case GL_BACK_RIGHT:
         return BUFFER_BIT_BACK_RIGHT;
      case GL_BACK_LEFT:
         return BUFFER_BIT_BACK_LEFT;
      case GL_LEFT:
         return BUFFER_BIT_FRONT_LEFT;
      case GL_FRONT_LEFT:
         return BUFFER_BIT_FRONT_LEFT;
      case GL_AUX0:
         return BUFFER_BIT_AUX0;
      case GL_AUX1:
         return BUFFER_BIT_AUX1;
      case GL_AUX2:
         return BUFFER_BIT_AUX2;
      case GL_AUX3:
         return BUFFER_BIT_AUX3;
      case GL_COLOR_ATTACHMENT0_EXT:
         return BUFFER_BIT_COLOR0;
      case GL_COLOR_ATTACHMENT1_EXT:
         return BUFFER_BIT_COLOR1;
      case GL_COLOR_ATTACHMENT2_EXT:
         return BUFFER_BIT_COLOR2;
      case GL_COLOR_ATTACHMENT3_EXT:
         return BUFFER_BIT_COLOR3;
      default:
         /* error */
         return BAD_MASK;
   }
}


/**
 * Specify which color buffers to draw into.
 *
 * \param buffer  color buffer, such as GL_LEFT or GL_FRONT_AND_BACK.
 *
 * Flushes the vertices and verifies the parameter and updates the
 * gl_colorbuffer_attrib::_DrawDestMask bitfield. Marks new color state in
 * __GLcontextRec::NewState and notifies the driver via the
 * dd_function_table::DrawBuffer callback.
 */
void GLAPIENTRY
_mesa_DrawBuffer(GLenum buffer)
{
   GLuint bufferID;
   GLuint destMask;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex... */

   if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glDrawBuffer %s\n", _mesa_lookup_enum_by_nr(buffer));
   }

   bufferID = ctx->DrawBuffer->Name;

   if (buffer == GL_NONE) {
      destMask = 0x0;
   }
   else {
      const GLuint supportedMask = supported_buffer_bitmask(ctx, bufferID);
      destMask = draw_buffer_enum_to_bitmask(buffer);
      if (destMask == BAD_MASK) {
         /* totally bogus buffer */
         _mesa_error(ctx, GL_INVALID_ENUM, "glDrawBuffer(buffer)");
         return;
      }
      destMask &= supportedMask;
      if (destMask == 0x0) {
         /* none of the named color buffers exist! */
         _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffer(buffer)");
         return;
      }
   }

   /* if we get here, there's no error so set new state */
   _mesa_drawbuffers(ctx, 1, &buffer, &destMask);
}


/**
 * Called by glDrawBuffersARB; specifies the destination color buffers
 * for N fragment program color outputs.
 *
 * XXX This function is called by _mesa_PopAttrib() and we need to do
 * some more work to deal with the current framebuffer binding state!
 */
void GLAPIENTRY
_mesa_DrawBuffersARB(GLsizei n, const GLenum *buffers)
{
   GLint output;
   GLuint usedBufferMask, supportedMask;
   GLuint bufferID;
   GLuint destMask[MAX_DRAW_BUFFERS];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!ctx->Extensions.ARB_draw_buffers) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffersARB");
      return;
   }
   if (n < 1 || n > (GLsizei) ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawBuffersARB(n)");
      return;
   }

   bufferID = ctx->DrawBuffer->Name;

   supportedMask = supported_buffer_bitmask(ctx, bufferID);
   usedBufferMask = 0x0;

   /* complicated error checking... */
   for (output = 0; output < n; output++) {
      if (buffers[output] == GL_NONE) {
         destMask[output] = 0x0;
      }
      else {
         destMask[output] = draw_buffer_enum_to_bitmask(buffers[output]);
         if (destMask[output] == BAD_MASK
             || _mesa_bitcount(destMask[output]) > 1) {
            _mesa_error(ctx, GL_INVALID_ENUM, "glDrawBuffersARB(buffer)");
            return;
         }
         destMask[output] &= supportedMask;
         if (destMask[output] == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawBuffersARB(unsupported buffer)");
            return;
         }
         if (destMask[output] & usedBufferMask) {
            /* can't specify a dest buffer more than once! */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glDrawBuffersARB(duplicated buffer)");
            return;
         }

         /* update bitmask */
         usedBufferMask |= destMask[output];
      }
   }

   /* OK, if we get here, there were no errors so set the new state */
   _mesa_drawbuffers(ctx, n, buffers, destMask);
}


/**
 * Set color output state.  Traditionally, there was only one color
 * output, but fragment programs can now have several distinct color
 * outputs (see GL_ARB_draw_buffers).  This function sets the state
 * for one such color output.
 */
static void
set_color_output(GLcontext *ctx, GLuint output, GLenum buffer, GLuint destMask)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   ASSERT(output < ctx->Const.MaxDrawBuffers);

   fb->ColorDrawBuffer[output] = buffer;
   fb->_ColorDrawBufferMask[output] = destMask;

   if (fb->Name == 0) {
      /* Set traditional state var */
      ctx->Color.DrawBuffer[output] = buffer;
   }

   /* not really needed, will be set later */
   fb->_NumColorDrawBuffers[output] = 0;
}


/**
 * Helper routine used by _mesa_DrawBuffer, _mesa_DrawBuffersARB and
 * _mesa_PopAttrib to set drawbuffer state.
 */
void
_mesa_drawbuffers(GLcontext *ctx, GLsizei n, const GLenum *buffers,
                  const GLuint *destMask)
{
   GLuint mask[MAX_DRAW_BUFFERS];
   GLint output;

   if (!destMask) {
      /* compute destMask values now */
      const GLuint bufferID = ctx->DrawBuffer->Name;
      const GLuint supportedMask = supported_buffer_bitmask(ctx, bufferID);
      for (output = 0; output < n; output++) {
         mask[output] = draw_buffer_enum_to_bitmask(buffers[output]);
         ASSERT(mask[output] != BAD_MASK);
         mask[output] &= supportedMask;
      }
      destMask = mask;
   }

   for (output = 0; output < n; output++) {
      set_color_output(ctx, output, buffers[output], destMask[output]);
   }

   /* set remaining color outputs to NONE */
   for (output = n; output < ctx->Const.MaxDrawBuffers; output++) {
      set_color_output(ctx, output, GL_NONE, 0x0);
   }

   ctx->NewState |= _NEW_COLOR;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.DrawBuffers)
      ctx->Driver.DrawBuffers(ctx, n, buffers);
   else if (ctx->Driver.DrawBuffer)
      ctx->Driver.DrawBuffer(ctx, buffers[0]);
}



/**
 * Set the color buffer source for reading pixels.
 *
 * \param mode color buffer.
 *
 * \sa glReadBuffer().
 *
 */
void GLAPIENTRY
_mesa_ReadBuffer(GLenum buffer)
{
   struct gl_framebuffer *fb;
   GLuint srcMask, supportedMask;
   GLuint bufferID;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   fb = ctx->ReadBuffer;
   bufferID = fb->Name;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadBuffer %s\n", _mesa_lookup_enum_by_nr(buffer));

   if (bufferID > 0 && buffer == GL_NONE) {
      /* legal! */
      srcMask = 0x0;
   }
   else {
      /* general case */
      srcMask = read_buffer_enum_to_bitmask(buffer);
      if (srcMask == BAD_MASK) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glReadBuffer(buffer)");
         return;
      }
      supportedMask = supported_buffer_bitmask(ctx, bufferID);
      if ((srcMask & supportedMask) == 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "glReadBuffer(buffer)");
         return;
      }
   }

   if (bufferID == 0) {
      ctx->Pixel.ReadBuffer = buffer;
   }
   fb->ColorReadBuffer = buffer;
   fb->_ColorReadBufferMask = srcMask;

   ctx->NewState |= _NEW_PIXEL;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.ReadBuffer)
      (*ctx->Driver.ReadBuffer)(ctx, buffer);
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

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glResizeBuffersMESA\n");

   if (ctx->DrawBuffer && ctx->DrawBuffer->Name == 0) {
      GLuint newWidth, newHeight;
      GLframebuffer *buffer = ctx->DrawBuffer;

      /* ask device driver for size of output buffer */
      ctx->Driver.GetBufferSize( buffer, &newWidth, &newHeight );

      /* see if size of device driver's color buffer (window) has changed */
      if (buffer->Width != newWidth || buffer->Height != newHeight) {
         if (ctx->Driver.ResizeBuffers)
            ctx->Driver.ResizeBuffers(ctx, buffer, newWidth, newHeight );
      }
   }

   if (ctx->ReadBuffer && ctx->ReadBuffer != ctx->DrawBuffer
       && ctx->ReadBuffer->Name == 0) {
      GLuint newWidth, newHeight;
      GLframebuffer *buffer = ctx->ReadBuffer;

      /* ask device driver for size of read buffer */
      ctx->Driver.GetBufferSize( buffer, &newWidth, &newHeight );

      /* see if size of device driver's color buffer (window) has changed */
      if (buffer->Width != newWidth || buffer->Height != newHeight) {
         if (ctx->Driver.ResizeBuffers)
            ctx->Driver.ResizeBuffers(ctx, buffer, newWidth, newHeight );
      }
   }

   ctx->NewState |= _NEW_BUFFERS;  /* to update scissor / window bounds */
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

#endif /* _HAVE_FULL_GL */



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
/** \name Initialization */
/*@{*/

/**
 * Initialize the context's scissor state.
 * \param ctx  the GL context.
 */
void
_mesa_init_scissor(GLcontext *ctx)
{
   /* Scissor group */
   ctx->Scissor.Enabled = GL_FALSE;
   ctx->Scissor.X = 0;
   ctx->Scissor.Y = 0;
   ctx->Scissor.Width = 0;
   ctx->Scissor.Height = 0;
}


/**
 * Initialize the context's multisample state.
 * \param ctx  the GL context.
 */
void
_mesa_init_multisample(GLcontext *ctx)
{
   ctx->Multisample.Enabled = GL_FALSE;
   ctx->Multisample.SampleAlphaToCoverage = GL_FALSE;
   ctx->Multisample.SampleAlphaToOne = GL_FALSE;
   ctx->Multisample.SampleCoverage = GL_FALSE;
   ctx->Multisample.SampleCoverageValue = 1.0;
   ctx->Multisample.SampleCoverageInvert = GL_FALSE;
}

/*@}*/
