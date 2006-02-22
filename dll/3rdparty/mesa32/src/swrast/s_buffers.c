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
#include "colormac.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"
#include "fbobject.h"

#include "s_accum.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_masking.h"
#include "s_stencil.h"


/**
 * Clear the color buffer when glColorMask is in effect.
 */
static void
clear_rgba_buffer_with_masking(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   GLchan clearColor[4];
   GLint i;

   ASSERT(ctx->Visual.rgbMode);
   ASSERT(rb->PutRow);

   CLAMPED_FLOAT_TO_CHAN(clearColor[RCOMP], ctx->Color.ClearColor[0]);
   CLAMPED_FLOAT_TO_CHAN(clearColor[GCOMP], ctx->Color.ClearColor[1]);
   CLAMPED_FLOAT_TO_CHAN(clearColor[BCOMP], ctx->Color.ClearColor[2]);
   CLAMPED_FLOAT_TO_CHAN(clearColor[ACOMP], ctx->Color.ClearColor[3]);

   for (i = 0; i < height; i++) {
      GLchan rgba[MAX_WIDTH][4];
      GLint j;
      for (j = 0; j < width; j++) {
         COPY_CHAN4(rgba[j], clearColor);
      }
      _swrast_mask_rgba_array( ctx, rb, width, x, y + i, rgba );
      rb->PutRow(ctx, rb, width, x, y + i, rgba, NULL);
   }
}


/**
 * Clear color index buffer with masking.
 */
static void
clear_ci_buffer_with_masking(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   GLuint span[MAX_WIDTH];
   GLubyte mask[MAX_WIDTH];
   GLint i, j;

   ASSERT(!ctx->Visual.rgbMode);

   MEMSET( mask, 1, width );
   for (i = 0; i < height;i++) {
      for (j = 0; j < width;j++) {
         span[j] = ctx->Color.ClearIndex;
      }
      _swrast_mask_ci_array(ctx, rb, width, x, y + i, span);
      ASSERT(rb->PutRow);
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      rb->PutRow(ctx, rb, width, x, y + i, span, mask);
   }
}


/**
 * Clear an rgba color buffer without channel masking.
 */
static void
clear_rgba_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   GLubyte clear8[4];
   GLushort clear16[4];
   GLvoid *clearVal;
   GLint i;

   ASSERT(ctx->Visual.rgbMode);

   ASSERT(ctx->Color.ColorMask[0] &&
          ctx->Color.ColorMask[1] &&
          ctx->Color.ColorMask[2] &&
          ctx->Color.ColorMask[3]);             

   ASSERT(rb->PutMonoRow);

   switch (rb->DataType) {
      case GL_UNSIGNED_BYTE:
         clear8[0] = FLOAT_TO_UBYTE(ctx->Color.ClearColor[0]);
         clear8[1] = FLOAT_TO_UBYTE(ctx->Color.ClearColor[1]);
         clear8[2] = FLOAT_TO_UBYTE(ctx->Color.ClearColor[2]);
         clear8[3] = FLOAT_TO_UBYTE(ctx->Color.ClearColor[3]);
         clearVal = clear8;
         break;
      case GL_UNSIGNED_SHORT:
         clear16[0] = FLOAT_TO_USHORT(ctx->Color.ClearColor[0]);
         clear16[1] = FLOAT_TO_USHORT(ctx->Color.ClearColor[1]);
         clear16[2] = FLOAT_TO_USHORT(ctx->Color.ClearColor[2]);
         clear16[3] = FLOAT_TO_USHORT(ctx->Color.ClearColor[3]);
         clearVal = clear16;
         break;
      case GL_FLOAT:
         clearVal = ctx->Color.ClearColor;
         break;
      default:
         _mesa_problem(ctx, "Bad rb DataType in clear_color_buffer");
         return;
   }

   for (i = 0; i < height; i++) {
      rb->PutMonoRow(ctx, rb, width, x, y + i, clearVal, NULL);
   }
}


/**
 * Clear color index buffer without masking.
 */
static void
clear_ci_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   GLubyte clear8;
   GLushort clear16;
   GLuint clear32;
   GLvoid *clearVal;
   GLint i;

   ASSERT(!ctx->Visual.rgbMode);

   ASSERT((ctx->Color.IndexMask & ((1 << ctx->Visual.indexBits) - 1))
          == (GLuint) ((1 << ctx->Visual.indexBits) - 1));

   ASSERT(rb->PutMonoRow);

   /* setup clear value */
   switch (rb->DataType) {
      case GL_UNSIGNED_BYTE:
         clear8 = (GLubyte) ctx->Color.ClearIndex;
         clearVal = &clear8;
         break;
      case GL_UNSIGNED_SHORT:
         clear16 = (GLushort) ctx->Color.ClearIndex;
         clearVal = &clear16;
         break;
      case GL_UNSIGNED_INT:
         clear32 = ctx->Color.ClearIndex;
         clearVal = &clear32;
         break;
      default:
         _mesa_problem(ctx, "Bad rb DataType in clear_color_buffer");
         return;
   }

   for (i = 0; i < height; i++)
      rb->PutMonoRow(ctx, rb, width, x, y + i, clearVal, NULL);
}


/**
 * Clear the front/back/left/right/aux color buffers.
 * This function is usually only called if the device driver can't
 * clear its own color buffers for some reason (such as with masking).
 */
static void
clear_color_buffers(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLboolean masking;
   GLuint i;

   if (ctx->Visual.rgbMode) {
      if (ctx->Color.ColorMask[0] && 
          ctx->Color.ColorMask[1] && 
          ctx->Color.ColorMask[2] && 
          ctx->Color.ColorMask[3]) {
         masking = GL_FALSE;
      }
      else {
         masking = GL_TRUE;
      }
   }
   else {
      const GLuint indexBits = (1 << ctx->Visual.indexBits) - 1;
      if ((ctx->Color.IndexMask & indexBits) == indexBits) {
         masking = GL_FALSE;
      }
      else {
         masking = GL_TRUE;
      }
   }

   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers[0]; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][i];
#if OLD_RENDERBUFFER
      /* SetBuffer will go away */
      if (swrast->Driver.SetBuffer)
         swrast->Driver.SetBuffer(ctx, ctx->DrawBuffer,
                                  ctx->DrawBuffer->_ColorDrawBit[0][i]);
#endif

      if (ctx->Visual.rgbMode) {
         if (masking) {
            clear_rgba_buffer_with_masking(ctx, rb);
         }
         else {
            clear_rgba_buffer(ctx, rb);
         }
      }
      else {
         if (masking) {
            clear_ci_buffer_with_masking(ctx, rb);
         }
         else {
            clear_ci_buffer(ctx, rb);
         }
      }
   }

   /* restore default read/draw buffer */
   _swrast_use_draw_buffer(ctx);
}


/**
 * Called via the device driver's ctx->Driver.Clear() function if the
 * device driver can't clear one or more of the buffers itself.
 * \param mask  bitwise-OR of DD_*_BIT flags.
 * \param all  if GL_TRUE, clear whole buffer, else clear specified region.
 */
void
_swrast_Clear(GLcontext *ctx, GLbitfield mask,
	      GLboolean all, GLint x, GLint y, GLint width, GLint height)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   (void) all; (void) x; (void) y; (void) width; (void) height;

#ifdef DEBUG_FOO
   {
      const GLbitfield legalBits =
         BUFFER_BIT_FRONT_LEFT |
	 BUFFER_BIT_FRONT_RIGHT |
	 BUFFER_BIT_BACK_LEFT |
	 BUFFER_BIT_BACK_RIGHT |
	 BUFFER_BIT_DEPTH |
	 BUFFER_BIT_STENCIL |
	 BUFFER_BIT_ACCUM |
         BUFFER_BIT_AUX0 |
         BUFFER_BIT_AUX1 |
         BUFFER_BIT_AUX2 |
         BUFFER_BIT_AUX3;
      assert((mask & (~legalBits)) == 0);
   }
#endif

   RENDER_START(swrast,ctx);

   /* do software clearing here */
   if (mask) {
      if (mask & ctx->DrawBuffer->_ColorDrawBufferMask[0]) {
         clear_color_buffers(ctx);
      }
      if (mask & BUFFER_BIT_DEPTH) {
         struct gl_renderbuffer *rb
            = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
         _swrast_clear_depth_buffer(ctx, rb);
      }
      if (mask & BUFFER_BIT_ACCUM) {
         struct gl_renderbuffer *rb
            = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;
         _swrast_clear_accum_buffer(ctx, rb);
      }
      if (mask & BUFFER_BIT_STENCIL) {
         struct gl_renderbuffer *rb
            = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
         _swrast_clear_stencil_buffer(ctx, rb);
      }
   }

   RENDER_FINISH(swrast,ctx);
}


/*
 * Fallback for ctx->Driver.DrawBuffer()
 */
void
_swrast_DrawBuffer( GLcontext *ctx, GLenum mode )
{
   (void) mode;
   _swrast_use_draw_buffer(ctx);
}


/*
 * Fallback for ctx->Driver.DrawBuffers()
 */
void
_swrast_DrawBuffers( GLcontext *ctx, GLsizei n, const GLenum *buffers )
{
   _swrast_use_draw_buffer(ctx);
}


/*
 * Setup things so that we read/write spans from the user-designated
 * read buffer (set via glReadPixels).  We usually just have to call
 * this for glReadPixels, glCopyPixels, etc.
 */
void
_swrast_use_read_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* Do this so the software-emulated alpha plane span functions work! */
   swrast->CurrentBufferBit = ctx->ReadBuffer->_ColorReadBufferMask;
   /* Tell the device driver where to read/write spans */
   if (swrast->Driver.SetBuffer)
      swrast->Driver.SetBuffer(ctx, ctx->ReadBuffer, swrast->CurrentBufferBit);
}


/*
 * Setup things so that we read/write spans from the default draw buffer.
 * This is the usual mode that Mesa's software rasterizer operates in.
 */
void
_swrast_use_draw_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* The user can specify rendering to zero, one, two, or four color
    * buffers simultaneously with glDrawBuffer()!
    * We don't expect the span/point/line/triangle functions to deal with
    * that mess so we'll iterate over the multiple buffers as needed.
    * But usually we only render to one color buffer at a time.
    * We set ctx->Color._DriverDrawBuffer to that buffer and tell the
    * device driver to use that buffer.
    * Look in s_span.c's multi_write_rgba_span() function to see how
    * we loop over multiple color buffers when needed.
    */

   if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_FRONT_LEFT)
      swrast->CurrentBufferBit = BUFFER_BIT_FRONT_LEFT;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_BACK_LEFT)
      swrast->CurrentBufferBit = BUFFER_BIT_BACK_LEFT;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_FRONT_RIGHT)
      swrast->CurrentBufferBit = BUFFER_BIT_FRONT_RIGHT;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_BACK_RIGHT)
      swrast->CurrentBufferBit = BUFFER_BIT_BACK_RIGHT;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_AUX0)
      swrast->CurrentBufferBit = BUFFER_BIT_AUX0;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_AUX1)
      swrast->CurrentBufferBit = BUFFER_BIT_AUX1;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_AUX2)
      swrast->CurrentBufferBit = BUFFER_BIT_AUX2;
   else if (ctx->DrawBuffer->_ColorDrawBufferMask[0] & BUFFER_BIT_AUX3)
      swrast->CurrentBufferBit = BUFFER_BIT_AUX3;
   else
      /* glDrawBuffer(GL_NONE) */
      swrast->CurrentBufferBit = BUFFER_BIT_FRONT_LEFT; /* we always have this buffer */

   if (swrast->Driver.SetBuffer)
      swrast->Driver.SetBuffer(ctx, ctx->DrawBuffer, swrast->CurrentBufferBit);
}
