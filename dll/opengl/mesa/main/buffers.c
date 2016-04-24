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
 * glReadBuffer, DrawBuffer functions.
 */

#include <precomp.h>

#define BAD_MASK ~0u

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
supported_buffer_bitmask(const struct gl_context *ctx, const struct gl_framebuffer *fb)
{
   GLbitfield mask = 0x0;

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
      case GL_AUX2:
      case GL_AUX3:
         return 1 << BUFFER_COUNT; /* invalid, but not BAD_MASK */
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
      case GL_AUX2:
      case GL_AUX3:
         return BUFFER_COUNT; /* invalid, but not -1 */
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
         _mesa_error(ctx, GL_INVALID_ENUM, "glDrawBuffer(buffer=0x%x)", buffer);
         return;
      }
      destMask &= supportedMask;
      if (destMask == 0x0) {
         /* none of the named color buffers exist! */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glDrawBuffer(buffer=0x%x)", buffer);
         return;
      }
   }

   /* if we get here, there's no error so set new state */
   _mesa_drawbuffer(ctx, buffer, destMask);

   /*
    * Call device driver function.
    */
   if (ctx->Driver.DrawBuffer)
      ctx->Driver.DrawBuffer(ctx, buffer);
}

/**
 * Performs necessary state updates when _mesa_drawbuffers makes an
 * actual change.
 */
static void
updated_drawbuffers(struct gl_context *ctx)
{
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);
}

/**
 * Helper function to set the GL_DRAW_BUFFER state in the context and
 * current FBO.  Called via glDrawBuffer(), glDrawBuffersARB()
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
_mesa_drawbuffer(struct gl_context *ctx, const GLenum buffers, GLbitfield destMask)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLint bufIndex;

   if (!destMask) {
      /* compute destMask values now */
      const GLbitfield supportedMask = supported_buffer_bitmask(ctx, fb);
      destMask = draw_buffer_enum_to_bitmask(buffers);
      ASSERT(destmask != BAD_MASK);
      destMask &= supportedMask;
   }
   
   bufIndex = _mesa_ffs(destMask) - 1;

   if (fb->_ColorDrawBufferIndex != bufIndex) {
      updated_drawbuffers(ctx);
      fb->_ColorDrawBufferIndex = bufIndex;
   }
   fb->ColorDrawBuffer = buffers;

   if (ctx->Color.DrawBuffer != fb->ColorDrawBuffer) {
      updated_drawbuffers(ctx);
      ctx->Color.DrawBuffer = fb->ColorDrawBuffer;
   }
}


/**
 * Update the current drawbuffer's _ColorDrawBufferIndex[] list, etc.
 * from the context's Color.DrawBuffer[] state.
 * Use when changing contexts.
 */
void
_mesa_update_draw_buffer(struct gl_context *ctx)
{
   _mesa_drawbuffer(ctx, ctx->Color.DrawBuffer, 0);
}


/**
 * Like \sa _mesa_drawbuffers(), this is a helper function for setting
 * GL_READ_BUFFER state in the context and current FBO.
 * \param ctx  the rendering context
 * \param buffer  GL_FRONT, GL_BACK, GL_COLOR_ATTACHMENT0, etc.
 * \param bufferIndex  the numerical index corresponding to 'buffer'
 */
void
_mesa_readbuffer(struct gl_context *ctx, GLenum buffer, GLint bufferIndex)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;

   ctx->Pixel.ReadBuffer = buffer;

   fb->ColorReadBuffer = buffer;
   fb->_ColorReadBufferIndex = bufferIndex;

   ctx->NewState |= _NEW_BUFFERS;
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

   /* OK, all error checking has been completed now */

   _mesa_readbuffer(ctx, buffer, srcBuffer);
   ctx->NewState |= _NEW_BUFFERS;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.ReadBuffer)
      (*ctx->Driver.ReadBuffer)(ctx, buffer);
}
