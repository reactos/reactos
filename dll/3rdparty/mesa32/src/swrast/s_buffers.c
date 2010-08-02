/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

/** XXX This file should be named s_clear.c */

#include "main/glheader.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/mtypes.h"

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
   SWspan span;
   GLint i;

   ASSERT(ctx->Visual.rgbMode);
   ASSERT(rb->PutRow);

   /* Initialize color span with clear color */
   /* XXX optimize for clearcolor == black/zero (bzero) */
   INIT_SPAN(span, GL_BITMAP);
   span.end = width;
   span.arrayMask = SPAN_RGBA;
   span.array->ChanType = rb->DataType;
   if (span.array->ChanType == GL_UNSIGNED_BYTE) {
      GLubyte clearColor[4];
      UNCLAMPED_FLOAT_TO_UBYTE(clearColor[RCOMP], ctx->Color.ClearColor[0]);
      UNCLAMPED_FLOAT_TO_UBYTE(clearColor[GCOMP], ctx->Color.ClearColor[1]);
      UNCLAMPED_FLOAT_TO_UBYTE(clearColor[BCOMP], ctx->Color.ClearColor[2]);
      UNCLAMPED_FLOAT_TO_UBYTE(clearColor[ACOMP], ctx->Color.ClearColor[3]);
      for (i = 0; i < width; i++) {
         COPY_4UBV(span.array->rgba[i], clearColor);
      }
   }
   else if (span.array->ChanType == GL_UNSIGNED_SHORT) {
      GLushort clearColor[4];
      UNCLAMPED_FLOAT_TO_USHORT(clearColor[RCOMP], ctx->Color.ClearColor[0]);
      UNCLAMPED_FLOAT_TO_USHORT(clearColor[GCOMP], ctx->Color.ClearColor[1]);
      UNCLAMPED_FLOAT_TO_USHORT(clearColor[BCOMP], ctx->Color.ClearColor[2]);
      UNCLAMPED_FLOAT_TO_USHORT(clearColor[ACOMP], ctx->Color.ClearColor[3]);
      for (i = 0; i < width; i++) {
         COPY_4V(span.array->rgba[i], clearColor);
      }
   }
   else {
      ASSERT(span.array->ChanType == GL_FLOAT);
      for (i = 0; i < width; i++) {
         CLAMPED_FLOAT_TO_CHAN(span.array->rgba[i][0], ctx->Color.ClearColor[0]);
         CLAMPED_FLOAT_TO_CHAN(span.array->rgba[i][1], ctx->Color.ClearColor[1]);
         CLAMPED_FLOAT_TO_CHAN(span.array->rgba[i][2], ctx->Color.ClearColor[2]);
         CLAMPED_FLOAT_TO_CHAN(span.array->rgba[i][3], ctx->Color.ClearColor[3]);
      }
   }

   /* Note that masking will change the color values, but only the
    * channels for which the write mask is GL_FALSE.  The channels
    * which which are write-enabled won't get modified.
    */
   for (i = 0; i < height; i++) {
      span.x = x;
      span.y = y + i;
      _swrast_mask_rgba_span(ctx, rb, &span);
      /* write masked row */
      rb->PutRow(ctx, rb, width, x, y + i, span.array->rgba, NULL);
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
   SWspan span;
   GLint i;

   ASSERT(!ctx->Visual.rgbMode);
   ASSERT(rb->PutRow);
   ASSERT(rb->DataType == GL_UNSIGNED_INT);

   /* Initialize index span with clear index */
   INIT_SPAN(span, GL_BITMAP);
   span.end = width;
   span.arrayMask = SPAN_INDEX;
   for (i = 0; i < width;i++) {
      span.array->index[i] = ctx->Color.ClearIndex;
   }

   /* Note that masking will change the color indexes, but only the
    * bits for which the write mask is GL_FALSE.  The bits
    * which are write-enabled won't get modified.
    */
   for (i = 0; i < height;i++) {
      span.x = x;
      span.y = y + i;
      _swrast_mask_ci_span(ctx, rb, &span);
      /* write masked row */
      rb->PutRow(ctx, rb, width, x, y + i, span.array->index, NULL);
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
         UNCLAMPED_FLOAT_TO_UBYTE(clear8[0], ctx->Color.ClearColor[0]);
         UNCLAMPED_FLOAT_TO_UBYTE(clear8[1], ctx->Color.ClearColor[1]);
         UNCLAMPED_FLOAT_TO_UBYTE(clear8[2], ctx->Color.ClearColor[2]);
         UNCLAMPED_FLOAT_TO_UBYTE(clear8[3], ctx->Color.ClearColor[3]);
         clearVal = clear8;
         break;
      case GL_UNSIGNED_SHORT:
         UNCLAMPED_FLOAT_TO_USHORT(clear16[0], ctx->Color.ClearColor[0]);
         UNCLAMPED_FLOAT_TO_USHORT(clear16[1], ctx->Color.ClearColor[1]);
         UNCLAMPED_FLOAT_TO_USHORT(clear16[2], ctx->Color.ClearColor[2]);
         UNCLAMPED_FLOAT_TO_USHORT(clear16[3], ctx->Color.ClearColor[3]);
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

   ASSERT((ctx->Color.IndexMask & ((1 << rb->IndexBits) - 1))
          == (GLuint) ((1 << rb->IndexBits) - 1));

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
   GLboolean masking;
   GLuint buf;

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
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0];
      const GLuint indexBits = (1 << rb->IndexBits) - 1;
      if ((ctx->Color.IndexMask & indexBits) == indexBits) {
         masking = GL_FALSE;
      }
      else {
         masking = GL_TRUE;
      }
   }

   for (buf = 0; buf < ctx->DrawBuffer->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[buf];
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
}


/**
 * Called via the device driver's ctx->Driver.Clear() function if the
 * device driver can't clear one or more of the buffers itself.
 * \param buffers  bitfield of BUFFER_BIT_* values indicating which
 *                 renderbuffers are to be cleared.
 * \param all  if GL_TRUE, clear whole buffer, else clear specified region.
 */
void
_swrast_Clear(GLcontext *ctx, GLbitfield buffers)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

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
      assert((buffers & (~legalBits)) == 0);
   }
#endif

   RENDER_START(swrast,ctx);

   /* do software clearing here */
   if (buffers) {
      if ((buffers & BUFFER_BITS_COLOR)
          && (ctx->DrawBuffer->_NumColorDrawBuffers > 0)) {
         clear_color_buffers(ctx);
      }
      if (buffers & BUFFER_BIT_DEPTH) {
         _swrast_clear_depth_buffer(ctx, ctx->DrawBuffer->_DepthBuffer);
      }
      if (buffers & BUFFER_BIT_ACCUM) {
         _swrast_clear_accum_buffer(ctx,
                       ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
      }
      if (buffers & BUFFER_BIT_STENCIL) {
         _swrast_clear_stencil_buffer(ctx, ctx->DrawBuffer->_StencilBuffer);
      }
   }

   RENDER_FINISH(swrast,ctx);
}
