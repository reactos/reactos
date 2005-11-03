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

#include "s_auxbuffer.h"
#include "s_context.h"



/**
 * Allocate memory for the software auxillary buffers associated with
 * the given GLframebuffer.  Free any currently allocated aux buffers
 * first.
 */
void
_swrast_alloc_aux_buffers( GLframebuffer *buffer )
{
   GLint i;

   for (i = 0; i < buffer->Visual.numAuxBuffers; i++) {
      if (buffer->AuxBuffers[i]) {
         _mesa_free(buffer->AuxBuffers[i]);
         buffer->AuxBuffers[i] = NULL;
      }

      buffer->AuxBuffers[i] = (GLchan *) _mesa_malloc(buffer->Width
                                        * buffer->Height * 4 * sizeof(GLchan));
   }
}



/* RGBA */
#define NAME(PREFIX) PREFIX##_aux
#define SPAN_VARS \
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
#define INIT_PIXEL_PTR(P, X, Y) \
   GLchan *P = swrast->CurAuxBuffer + ((Y) * ctx->DrawBuffer->Width + (X)) * 4; \
   assert(swrast->CurAuxBuffer);

#define INC_PIXEL_PTR(P) P += 4
#if CHAN_TYPE == GL_FLOAT
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = MAX2((R), 0.0F); \
   P[1] = MAX2((G), 0.0F); \
   P[2] = MAX2((B), 0.0F); \
   P[3] = CHAN_MAXF
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = MAX2((R), 0.0F); \
   P[1] = MAX2((G), 0.0F); \
   P[2] = MAX2((B), 0.0F); \
   P[3] = CLAMP((A), 0.0F, CHAN_MAXF)
#else
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = R;  P[1] = G;  P[2] = B;  P[3] = CHAN_MAX
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = R;  P[1] = G;  P[2] = B;  P[3] = A
#endif
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[0];  G = P[1];  B = P[2];  A = P[3]
#include "swrast/s_spantemp.h"



/**
 * Called from driver's SetBuffer() function to choose an aux buffer.
 */
void
_swrast_use_aux_buffer(GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   (void) buffer;

   switch (bufferBit) {
   case DD_AUX0_BIT:
      ASSERT(buffer->Visual.numAuxBuffers >= 1);
      swrast->CurAuxBuffer = ctx->DrawBuffer->AuxBuffers[0];
      break;
   case DD_AUX1_BIT:
      ASSERT(buffer->Visual.numAuxBuffers >= 2);
      swrast->CurAuxBuffer = ctx->DrawBuffer->AuxBuffers[1];
      break;
   case DD_AUX2_BIT:
      ASSERT(buffer->Visual.numAuxBuffers >= 3);
      swrast->CurAuxBuffer = ctx->DrawBuffer->AuxBuffers[2];
      break;
   case DD_AUX3_BIT:
      ASSERT(buffer->Visual.numAuxBuffers >= 4);
      swrast->CurAuxBuffer = ctx->DrawBuffer->AuxBuffers[3];
      break;
   default:
      swrast->CurAuxBuffer = NULL;
   }

   swrast->Driver.WriteRGBASpan = write_rgba_span_aux;
   swrast->Driver.WriteRGBSpan = write_rgb_span_aux;
   swrast->Driver.WriteMonoRGBASpan = write_monorgba_span_aux;
   swrast->Driver.WriteRGBAPixels = write_rgba_pixels_aux;
   swrast->Driver.WriteMonoRGBAPixels = write_monorgba_pixels_aux;
   swrast->Driver.ReadRGBASpan = read_rgba_span_aux;
   swrast->Driver.ReadRGBAPixels = read_rgba_pixels_aux;
}

