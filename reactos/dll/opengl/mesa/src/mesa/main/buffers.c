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



#include "glheader.h"
#include "buffers.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "mtypes.h"


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
      case GL_AUX2:
      case GL_AUX3:
         return 1 << BUFFER_COUNT; /* invalid, but not BAD_MASK */
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
      case GL_AUX2:
      case GL_AUX3:
         return BUFFER_COUNT; /* invalid, but not -1 */
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

   /* Turns out n==0 is a valid input that should not produce an error.
    * The remaining code below correctly handles the n==0 case.
    */
   if (n < 0 || n > (GLsizei) ctx->Const.MaxDrawBuffers) {
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
    * Call device driver function.  Note that n can be equal to 0,
    * in which case we don't want to reference buffers[0], which
    * may not be valid.
    */
   if (ctx->Driver.DrawBuffers)
      ctx->Driver.DrawBuffers(ctx, n, buffers);
   else if (ctx->Driver.DrawBuffer)
      ctx->Driver.DrawBuffer(ctx, n > 0 ? buffers[0] : GL_NONE);
}

/**
 * Performs necessary state updates when _mesa_drawbuffers makes an
 * actual change.
 */
static void
updated_drawbuffers(struct gl_context *ctx)
{
   FLUSH_VERTICES(ctx, _NEW_BUFFERS);

#if FEATURE_GL
   if (ctx->API == API_OPENGL && !ctx->Extensions.ARB_ES2_compatibility) {
      struct gl_framebuffer *fb = ctx->DrawBuffer;

      /* Flag the FBO as requiring validation. */
      if (fb->Name != 0) {
	 fb->_Status = 0;
      }
   }
#endif
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
_mesa_drawbuffers(struct gl_context *ctx, GLuint n, const GLenum *buffers,
                  const GLbitfield *destMask)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLbitfield mask[MAX_DRAW_BUFFERS];
   GLuint buf;

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

   /*
    * If n==1, destMask[0] may have up to four bits set.
    * Otherwise, destMask[x] can only have one bit set.
    */
   if (n == 1) {
      GLuint count = 0, destMask0 = destMask[0];
      while (destMask0) {
         GLint bufIndex = _mesa_ffs(destMask0) - 1;
         if (fb->_ColorDrawBufferIndexes[count] != bufIndex) {
            updated_drawbuffers(ctx);
            fb->_ColorDrawBufferIndexes[count] = bufIndex;
         }
         count++;
         destMask0 &= ~(1 << bufIndex);
      }
      fb->ColorDrawBuffer[0] = buffers[0];
      fb->_NumColorDrawBuffers = count;
   }
   else {
      GLuint count = 0;
      for (buf = 0; buf < n; buf++ ) {
         if (destMask[buf]) {
            GLint bufIndex = _mesa_ffs(destMask[buf]) - 1;
            /* only one bit should be set in the destMask[buf] field */
            ASSERT(_mesa_bitcount(destMask[buf]) == 1);
            if (fb->_ColorDrawBufferIndexes[buf] != bufIndex) {
	       updated_drawbuffers(ctx);
               fb->_ColorDrawBufferIndexes[buf] = bufIndex;
            }
            count = buf + 1;
         }
         else {
            if (fb->_ColorDrawBufferIndexes[buf] != -1) {
	       updated_drawbuffers(ctx);
               fb->_ColorDrawBufferIndexes[buf] = -1;
            }
         }
         fb->ColorDrawBuffer[buf] = buffers[buf];
      }
      fb->_NumColorDrawBuffers = count;
   }

   /* set remaining outputs to -1 (GL_NONE) */
   for (buf = fb->_NumColorDrawBuffers; buf < ctx->Const.MaxDrawBuffers; buf++) {
      if (fb->_ColorDrawBufferIndexes[buf] != -1) {
         updated_drawbuffers(ctx);
         fb->_ColorDrawBufferIndexes[buf] = -1;
      }
   }
   for (buf = n; buf < ctx->Const.MaxDrawBuffers; buf++) {
      fb->ColorDrawBuffer[buf] = GL_NONE;
   }

   if (fb->Name == 0) {
      /* also set context drawbuffer state */
      for (buf = 0; buf < ctx->Const.MaxDrawBuffers; buf++) {
         if (ctx->Color.DrawBuffer[buf] != fb->ColorDrawBuffer[buf]) {
	    updated_drawbuffers(ctx);
            ctx->Color.DrawBuffer[buf] = fb->ColorDrawBuffer[buf];
         }
      }
   }
}


/**
 * Update the current drawbuffer's _ColorDrawBufferIndex[] list, etc.
 * from the context's Color.DrawBuffer[] state.
 * Use when changing contexts.
 */
void
_mesa_update_draw_buffers(struct gl_context *ctx)
{
   GLenum buffers[MAX_DRAW_BUFFERS];
   GLuint i;

   /* should be a window system FBO */
   assert(ctx->DrawBuffer->Name == 0);

   for (i = 0; i < ctx->Const.MaxDrawBuffers; i++)
      buffers[i] = ctx->Color.DrawBuffer[i];

   _mesa_drawbuffers(ctx, ctx->Const.MaxDrawBuffers, buffers, NULL);
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

   if (fb->Name == 0) {
      /* Only update the per-context READ_BUFFER state if we're bound to
       * a window-system framebuffer.
       */
      ctx->Pixel.ReadBuffer = buffer;
   }

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
   ctx->NewState |= _NEW_BUFFERS;

   /*
    * Call device driver function.
    */
   if (ctx->Driver.ReadBuffer)
      (*ctx->Driver.ReadBuffer)(ctx, buffer);
}
