
/*
 * Mesa 3-D graphics library
 * Version:  5.0.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


/*
 * Software alpha planes.  Many frame buffers don't have alpha bits so
 * we simulate them in software.
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "imports.h"

#include "s_context.h"
#include "s_alphabuf.h"


/*
 * Allocate a new front and back alpha buffer.
 */
void
_swrast_alloc_alpha_buffers( GLframebuffer *buffer )
{
   const GLint bytes = buffer->Width * buffer->Height * sizeof(GLchan);

   ASSERT(buffer->UseSoftwareAlphaBuffers);

   if (buffer->FrontLeftAlpha) {
      MESA_PBUFFER_FREE( buffer->FrontLeftAlpha );
   }
   buffer->FrontLeftAlpha = MESA_PBUFFER_ALLOC( bytes );
   if (!buffer->FrontLeftAlpha) {
      /* out of memory */
      _mesa_error( NULL, GL_OUT_OF_MEMORY,
                   "Couldn't allocate front-left alpha buffer" );
   }

   if (buffer->Visual.doubleBufferMode) {
      if (buffer->BackLeftAlpha) {
         MESA_PBUFFER_FREE( buffer->BackLeftAlpha );
      }
      buffer->BackLeftAlpha = MESA_PBUFFER_ALLOC( bytes );
      if (!buffer->BackLeftAlpha) {
         /* out of memory */
         _mesa_error( NULL, GL_OUT_OF_MEMORY,
                      "Couldn't allocate back-left alpha buffer" );
      }
   }

   if (buffer->Visual.stereoMode) {
      if (buffer->FrontRightAlpha) {
         MESA_PBUFFER_FREE( buffer->FrontRightAlpha );
      }
      buffer->FrontRightAlpha = MESA_PBUFFER_ALLOC( bytes );
      if (!buffer->FrontRightAlpha) {
         /* out of memory */
         _mesa_error( NULL, GL_OUT_OF_MEMORY,
                      "Couldn't allocate front-right alpha buffer" );
      }

      if (buffer->Visual.doubleBufferMode) {
         if (buffer->BackRightAlpha) {
            MESA_PBUFFER_FREE( buffer->BackRightAlpha );
         }
         buffer->BackRightAlpha = MESA_PBUFFER_ALLOC( bytes );
         if (!buffer->BackRightAlpha) {
            /* out of memory */
            _mesa_error( NULL, GL_OUT_OF_MEMORY,
                         "Couldn't allocate back-right alpha buffer" );
         }
      }
   }
}


/*
 * Clear all the alpha buffers
 */
void
_swrast_clear_alpha_buffers( GLcontext *ctx )
{
   GLchan aclear;
   GLuint bufferBit;

   CLAMPED_FLOAT_TO_CHAN(aclear, ctx->Color.ClearColor[3]);

   ASSERT(ctx->DrawBuffer->UseSoftwareAlphaBuffers);
   ASSERT(ctx->Color.ColorMask[ACOMP]);

   /* loop over four possible alpha buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color._DrawDestMask) {
         GLchan *buffer;
         if (bufferBit == FRONT_LEFT_BIT) {
            buffer = (GLchan *) ctx->DrawBuffer->FrontLeftAlpha;
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            buffer = (GLchan *) ctx->DrawBuffer->FrontRightAlpha;
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            buffer = (GLchan *) ctx->DrawBuffer->BackLeftAlpha;
         }
         else {
            buffer = (GLchan *) ctx->DrawBuffer->BackRightAlpha;
         }

         if (ctx->Scissor.Enabled) {
            /* clear scissor region */
            GLint j;
            GLint rowLen = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
            GLint rows = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
            GLint width = ctx->DrawBuffer->Width;
            GLchan *aptr = buffer
                          + ctx->DrawBuffer->_Ymin * ctx->DrawBuffer->Width
                          + ctx->DrawBuffer->_Xmin;
            for (j = 0; j < rows; j++) {
#if CHAN_BITS == 8
               MEMSET( aptr, aclear, rowLen );
#elif CHAN_BITS == 16
               MEMSET16( aptr, aclear, rowLen );
#else
               GLint i;
               for (i = 0; i < rowLen; i++) {
                  aptr[i] = aclear;
               }
#endif
               aptr += width;
            }
         }
         else {
            /* clear whole buffer */
            GLuint pixels = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;
#if CHAN_BITS == 8
            MEMSET(buffer, aclear, pixels);
#elif CHAN_BITS == 16
            MEMSET16(buffer, aclear, pixels);
#else
            GLuint i;
            for (i = 0; i < pixels; i++) {
               buffer[i] = aclear;
            }
#endif
         }
      }
   }
}



static INLINE
GLchan *get_alpha_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   switch (swrast->CurrentBuffer) {
   case FRONT_LEFT_BIT:
      return (GLchan *) ctx->DrawBuffer->FrontLeftAlpha;
      break;
   case BACK_LEFT_BIT:
      return (GLchan *) ctx->DrawBuffer->BackLeftAlpha;
      break;
   case FRONT_RIGHT_BIT:
      return (GLchan *) ctx->DrawBuffer->FrontRightAlpha;
      break;
   case BACK_RIGHT_BIT:
      return (GLchan *) ctx->DrawBuffer->BackRightAlpha;
      break;
   default:
      _mesa_problem(ctx, "Bad CurrentBuffer in get_alpha_buffer()");
      return (GLchan *) ctx->DrawBuffer->FrontLeftAlpha;
   }
}


void
_swrast_write_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        CONST GLchan rgba[][4], const GLubyte mask[] )
{
   GLchan *buffer, *aptr;
   GLuint i;

   buffer = get_alpha_buffer(ctx);
   aptr = buffer + y * ctx->DrawBuffer->Width + x;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = rgba[i][ACOMP];
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = rgba[i][ACOMP];
      }
   }
}


void
_swrast_write_mono_alpha_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                             GLchan alpha, const GLubyte mask[] )
{
   GLchan *buffer, *aptr;
   GLuint i;

   buffer = get_alpha_buffer(ctx);
   aptr = buffer + y * ctx->DrawBuffer->Width + x;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = alpha;
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = alpha;
      }
   }
}


void
_swrast_write_alpha_pixels( GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          CONST GLchan rgba[][4], const GLubyte mask[] )
{
   GLchan *buffer;
   GLuint i;

   buffer = get_alpha_buffer(ctx);

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLchan *aptr = buffer + y[i] * ctx->DrawBuffer->Width + x[i];
            *aptr = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLchan *aptr = buffer + y[i] * ctx->DrawBuffer->Width + x[i];
         *aptr = rgba[i][ACOMP];
      }
   }
}


void
_swrast_write_mono_alpha_pixels( GLcontext *ctx,
                               GLuint n, const GLint x[], const GLint y[],
                               GLchan alpha, const GLubyte mask[] )
{
   GLchan *buffer;
   GLuint i;

   buffer = get_alpha_buffer(ctx);

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLchan *aptr = buffer + y[i] * ctx->DrawBuffer->Width + x[i];
            *aptr = alpha;
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLchan *aptr = buffer + y[i] * ctx->DrawBuffer->Width + x[i];
         *aptr = alpha;
      }
   }
}



void
_swrast_read_alpha_span( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   const GLchan *buffer, *aptr;
   GLuint i;

   buffer = get_alpha_buffer(ctx);
   aptr = buffer + y * ctx->DrawBuffer->Width + x;

   for (i = 0; i < n; i++)
      rgba[i][ACOMP] = *aptr++;
}


void
_swrast_read_alpha_pixels( GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLchan rgba[][4], const GLubyte mask[] )
{
   const GLchan *buffer;
   GLuint i;

   buffer = get_alpha_buffer(ctx);

   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLchan *aptr = buffer + y[i] * ctx->DrawBuffer->Width + x[i];
         rgba[i][ACOMP] = *aptr;
      }
   }
}
