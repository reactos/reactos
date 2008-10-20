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
 * \file buffers.c
 * General framebuffer-related functions, like glClear, glScissor, etc.
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

   if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glClear(incomplete framebuffer)");
      return;
   }

   if (ctx->DrawBuffer->Width == 0 || ctx->DrawBuffer->Height == 0 ||
       ctx->DrawBuffer->_Xmin >= ctx->DrawBuffer->_Xmax ||
       ctx->DrawBuffer->_Ymin >= ctx->DrawBuffer->_Ymax)
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
         GLuint i;
         for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
            bufferMask |= (1 << ctx->DrawBuffer->_ColorDrawBufferIndexes[i]);
         }
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



/**
 * Return bitmask of BUFFER_BIT_* flags indicating which color buffers are
 * available to the rendering context (for drawing or reading).
 * This depends on the type of framebuffer.  For window system framebuffers
 * we look at the framebuffer's visual.  But for user-create framebuffers we
 * look at the number of supported color attachments.
 * \param fb  the framebuffer to draw to, or read from
 * \return  bitmask of BUFFER_BIT_* flags
 */
static GLbitfield
supported_buffer_bitmask(const GLcontext *ctx, const struct gl_framebuffer *fb)
{
   GLbitfield mask = 0x0;

   if (fb->Name > 0) {
      /* A user-created renderbuffer */
      GLuint i;
      ASSERT(ctx->Extensions.EXT_framebuffer_object);
      for (i = 0; i < ctx->Const.MaxColorAttachments; i++) {
         mask |= (BUFFER_BIT_COLOR0 << i);
      }
   }
   else {
      /* A window system framebuffer */
      GLint i;
      mask = BUFFER_BIT_FRONT_LEFT; /* always have this */
      if (fb->Visual.stereoMode) {
         mask |= BUFFER_BIT_FRONT_RIGHT;
         if (fb->Visual.doubleBufferMode) {
            mask |= BUFFER_BIT_BACK_LEFT | BUFFER_BIT_BACK_RIGHT;
         }
      }
      else if (fb->Visual.doubleBufferMode) {
         mask |= BUFFER_BIT_BACK_LEFT;
      }

      for (i = 0; i < fb->Visual.numAuxBuffers; i++) {
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
static GLbitfield
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
      case GL_COLOR_ATTACHMENT4_EXT:
         return BUFFER_BIT_COLOR4;
      case GL_COLOR_ATTACHMENT5_EXT:
         return BUFFER_BIT_COLOR5;
      case GL_COLOR_ATTACHMENT6_EXT:
         return BUFFER_BIT_COLOR6;
      case GL_COLOR_ATTACHMENT7_EXT:
         return BUFFER_BIT_COLOR7;
      default:
         /* error */
         return BAD_MASK;
   }
}


/**
 * Helper routine used by glReadBuffer.
 * Given a GLenum naming a color buffer, return the index of the corresponding
 * renderbuffer (a BUFFER_* value).
 * return -1 for an invalid buffer.
 */
static GLint
read_buffer_enum_to_index(GLenum buffer)
{
   switch (buffer) {
      case GL_FRONT:
         return BUFFER_FRONT_LEFT;
      case GL_BACK:
         return BUFFER_BACK_LEFT;
      case GL_RIGHT:
         return BUFFER_FRONT_RIGHT;
      case GL_FRONT_RIGHT:
         return BUFFER_FRONT_RIGHT;
      case GL_BACK_RIGHT:
         return BUFFER_BACK_RIGHT;
      case GL_BACK_LEFT:
         return BUFFER_BACK_LEFT;
      case GL_LEFT:
         return BUFFER_FRONT_LEFT;
      case GL_FRONT_LEFT:
         return BUFFER_FRONT_LEFT;
      case GL_AUX0:
         return BUFFER_AUX0;
      case GL_AUX1:
         return BUFFER_AUX1;
      case GL_AUX2:
         return BUFFER_AUX2;
      case GL_AUX3:
         return BUFFER_AUX3;
      case GL_COLOR_ATTACHMENT0_EXT:
         return BUFFER_COLOR0;
      case GL_COLOR_ATTACHMENT1_EXT:
         return BUFFER_COLOR1;
      case GL_COLOR_ATTACHMENT2_EXT:
         return BUFFER_COLOR2;
      case GL_COLOR_ATTACHMENT3_EXT:
         return BUFFER_COLOR3;
      case GL_COLOR_ATTACHMENT4_EXT:
         return BUFFER_COLOR4;
      case GL_COLOR_ATTACHMENT5_EXT:
         return BUFFER_COLOR5;
      case GL_COLOR_ATTACHMENT6_EXT:
         return BUFFER_COLOR6;
      case GL_COLOR_ATTACHMENT7_EXT:
         return BUFFER_COLOR7;
      default:
         /* error */
         return -1;
   }
}


/**
 * Called by glDrawBuffer().
 * Specify which renderbuffer(s) to draw into for the first color output.
 * <buffer> can name zero, one, two or four renderbuffers!
 * \sa _mesa_DrawBuffersARB
 *
 * \param buffer  buffer token such as GL_LEFT or GL_FRONT_AND_BACK, etc.
 *
 * Note that the behaviour of this function depends on whether the
 * current ctx->DrawBuffer is a window-system framebuffer (Name=0) or
 * a user-created framebuffer object (Name!=0).
 *   In the former case, we update the per-context ctx->Color.DrawBuffer
 *   state var _and_ the FB's ColorDrawBuffer state.
 *   In the later case, we update the FB's ColorDrawBuffer state only.
 *
 * Furthermore, upon a MakeCurrent() or BindFramebuffer() call, if the
 * new FB is a window system FB, we need to re-update the FB's
 * ColorDrawBuffer state to match the context.  This is handled in
 * _mesa_update_framebuffer().
 *
 * See the GL_EXT_framebuffer_object spec for more info.
 */
void GLAPIENTRY
_mesa_DrawBuffer(GLenum buffer)
{
   GLbitfield destMask;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx); /* too complex... */

   if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glDrawBuffer %s\n", _mesa_lookup_enum_by_nr(buffer));
   }

   if (buffer == GL_NONE) {
      destMask = 0x0;
   }
   else {
      const GLbitfield supportedMask
         = supported_buffer_bitmask(ctx, ctx->DrawBuffer);
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

   /*
    * Call device driver function.
    */
   if (ctx->Driver.DrawBuffers)
      ctx->Driver.DrawBuffers(ctx, 1, &buffer);
   else if (ctx->Driver.DrawBuffer)
      ctx->Driver.DrawBuffer(ctx, buffer);
}


/**
 * Called by glDrawBuffersARB; specifies the destination color renderbuffers
 * for N fragment program color outputs.
 * \sa _mesa_DrawBuffer
 * \param n  number of outputs
 * \param buffers  array [n] of renderbuffer names.  Unlike glDrawBuffer, the
 *                 names cannot specify more than one buffer.  For example,
 *                 GL_FRONT_AND_BACK is illegal.
 */
void GLAPIENTRY
_mesa_DrawBuffersARB(GLsizei n, const GLenum *buffers)
{
   GLint output;
   GLbitfield usedBufferMask, supportedMask;
   GLbitfield destMask[MAX_DRAW_BUFFERS];
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

   supportedMask = supported_buffer_bitmask(ctx, ctx->DrawBuffer);
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

   /*
    * Call device driver function.
    */
   if (ctx->Driver.DrawBuffers)
      ctx->Driver.DrawBuffers(ctx, n, buffers);
   else if (ctx->Driver.DrawBuffer)
      ctx->Driver.DrawBuffer(ctx, buffers[0]);
}


/**
 * Helper function to set the GL_DRAW_BUFFER state in the context and
 * current FBO.
 *
 * All error checking will have been done prior to calling this function
 * so nothing should go wrong at this point.
 *
 * \param ctx  current context
 * \param n    number of color outputs to set
 * \param buffers  array[n] of colorbuffer names, like GL_LEFT.
 * \param destMask  array[n] of BUFFER_BIT_* bitmasks which correspond to the
 *                  colorbuffer names.  (i.e. GL_FRONT_AND_BACK =>
 *                  BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT).
 */
void
_mesa_drawbuffers(GLcontext *ctx, GLuint n, const GLenum *buffers,
                  const GLbitfield *destMask)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLbitfield mask[MAX_DRAW_BUFFERS];

   if (!destMask) {
      /* compute destMask values now */
      const GLbitfield supportedMask = supported_buffer_bitmask(ctx, fb);
      GLuint output;
      for (output = 0; output < n; output++) {
         mask[output] = draw_buffer_enum_to_bitmask(buffers[output]);
         ASSERT(mask[output] != BAD_MASK);
         mask[output] &= supportedMask;
      }
      destMask = mask;
   }

   if (n == 1) {
      GLuint buf, count = 0;
      /* init to -1 to help catch errors */
      fb->_ColorDrawBufferIndexes[0] = -1;
      for (buf = 0; buf < BUFFER_COUNT; buf++) {
         if (destMask[0] & (1 << buf)) {
            fb->_ColorDrawBufferIndexes[count] = buf;
            count++;
         }
      }
      fb->ColorDrawBuffer[0] = buffers[0];
      fb->_NumColorDrawBuffers = count;
   }
   else {
      GLuint buf, count = 0;
      for (buf = 0; buf < n; buf++ ) {
         if (destMask[buf]) {
            fb->_ColorDrawBufferIndexes[buf] = _mesa_ffs(destMask[buf]) - 1;
            fb->ColorDrawBuffer[buf] = buffers[buf];
            count = buf + 1;
         }
         else {
            fb->_ColorDrawBufferIndexes[buf] = -1;
         }
      }
      /* set remaining outputs to -1 (GL_NONE) */
      while (buf < ctx->Const.MaxDrawBuffers) {
         fb->_ColorDrawBufferIndexes[buf] = -1;
         fb->ColorDrawBuffer[buf] = GL_NONE;
         buf++;
      }
      fb->_NumColorDrawBuffers = count;
   }

   if (fb->Name == 0) {
      /* also set context drawbuffer state */
      GLuint buf;
      for (buf = 0; buf < ctx->Const.MaxDrawBuffers; buf++) {
         ctx->Color.DrawBuffer[buf] = fb->ColorDrawBuffer[buf];
      }
   }

   ctx->NewState |= _NEW_COLOR;
}


/**
 * Like \sa _mesa_drawbuffers(), this is a helper function for setting
 * GL_READ_BUFFER state in the context and current FBO.
 * \param ctx  the rendering context
 * \param buffer  GL_FRONT, GL_BACK, GL_COLOR_ATTACHMENT0, etc.
 * \param bufferIndex  the numerical index corresponding to 'buffer'
 */
void
_mesa_readbuffer(GLcontext *ctx, GLenum buffer, GLint bufferIndex)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;

   if (fb->Name == 0) {
      /* Only update the per-context READ_BUFFER state if we're bound to
       * a window-system framebuffer.
       */
      ctx->Pixel.ReadBuffer = buffer;
   }

   fb->ColorReadBuffer = buffer;
   fb->_ColorReadBufferIndex = bufferIndex;

   ctx->NewState |= _NEW_PIXEL;
}



/**
 * Called by glReadBuffer to set the source renderbuffer for reading pixels.
 * \param mode color buffer such as GL_FRONT, GL_BACK, etc.
 */
void GLAPIENTRY
_mesa_ReadBuffer(GLenum buffer)
{
   struct gl_framebuffer *fb;
   GLbitfield supportedMask;
   GLint srcBuffer;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadBuffer %s\n", _mesa_lookup_enum_by_nr(buffer));

   fb = ctx->ReadBuffer;

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadBuffer %s\n", _mesa_lookup_enum_by_nr(buffer));

   if (fb->Name > 0 && buffer == GL_NONE) {
      /* This is legal for user-created framebuffer objects */
      srcBuffer = -1;
   }
   else {
      /* general case / window-system framebuffer */
      srcBuffer = read_buffer_enum_to_index(buffer);
      if (srcBuffer == -1) {
         _mesa_error(ctx, GL_INVALID_ENUM,
                     "glReadBuffer(buffer=0x%x)", buffer);
         return;
      }
      supportedMask = supported_buffer_bitmask(ctx, fb);
      if (((1 << srcBuffer) & supportedMask) == 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadBuffer(buffer=0x%x)", buffer);
         return;
      }
   }

   /* OK, all error checking has been completed now */

   _mesa_readbuffer(ctx, buffer, srcBuffer);

   /*
    * Call device driver function.
    */
   if (ctx->Driver.ReadBuffer)
      (*ctx->Driver.ReadBuffer)(ctx, buffer);
}


#if _HAVE_FULL_GL

/**
 * XXX THIS IS OBSOLETE - drivers should take care of detecting window
 * size changes and act accordingly, likely calling _mesa_resize_framebuffer().
 *
 * GL_MESA_resize_buffers extension.
 *
 * When this function is called, we'll ask the window system how large
 * the current window is.  If it's a new size, we'll call the driver's
 * ResizeBuffers function.  The driver will then resize its color buffers
 * as needed, and maybe call the swrast's routine for reallocating
 * swrast-managed depth/stencil/accum/etc buffers.
 * \note This function should only be called through the GL API, not
 * from device drivers (as was done in the past).
 */

void _mesa_resizebuffers( GLcontext *ctx )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx );

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glResizeBuffersMESA\n");

   if (!ctx->Driver.GetBufferSize) {
      return;
   }

   if (ctx->WinSysDrawBuffer) {
      GLuint newWidth, newHeight;
      GLframebuffer *buffer = ctx->WinSysDrawBuffer;

      assert(buffer->Name == 0);

      /* ask device driver for size of output buffer */
      ctx->Driver.GetBufferSize( buffer, &newWidth, &newHeight );

      /* see if size of device driver's color buffer (window) has changed */
      if (buffer->Width != newWidth || buffer->Height != newHeight) {
         if (ctx->Driver.ResizeBuffers)
            ctx->Driver.ResizeBuffers(ctx, buffer, newWidth, newHeight );
      }
   }

   if (ctx->WinSysReadBuffer
       && ctx->WinSysReadBuffer != ctx->WinSysDrawBuffer) {
      GLuint newWidth, newHeight;
      GLframebuffer *buffer = ctx->WinSysReadBuffer;

      assert(buffer->Name == 0);

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
 * XXX THIS IS OBSOLETE
 */
void GLAPIENTRY
_mesa_ResizeBuffersMESA( void )
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->Extensions.MESA_resize_buffers)
      _mesa_resizebuffers( ctx );
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
void
_mesa_set_scissor(GLcontext *ctx, 
                  GLint x, GLint y, GLsizei width, GLsizei height)
{
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

   _mesa_set_scissor(ctx, x, y, width, height);
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
   ctx->Multisample.Enabled = GL_TRUE;
   ctx->Multisample.SampleAlphaToCoverage = GL_FALSE;
   ctx->Multisample.SampleAlphaToOne = GL_FALSE;
   ctx->Multisample.SampleCoverage = GL_FALSE;
   ctx->Multisample.SampleCoverageValue = 1.0;
   ctx->Multisample.SampleCoverageInvert = GL_FALSE;
}

/*@}*/
