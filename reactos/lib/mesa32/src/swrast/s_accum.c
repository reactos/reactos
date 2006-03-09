/*
 * Mesa 3-D graphics library
 * Version:  6.0.1
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
#include "context.h"
#include "macros.h"
#include "imports.h"

#include "s_accum.h"
#include "s_alphabuf.h"
#include "s_context.h"
#include "s_masking.h"
#include "s_span.h"


/*
 * Accumulation buffer notes
 *
 * Normally, accumulation buffer values are GLshorts with values in
 * [-32767, 32767] which represent floating point colors in [-1, 1],
 * as suggested by the OpenGL specification.
 *
 * We optimize for the common case used for full-scene antialiasing:
 *    // start with accum buffer cleared to zero
 *    glAccum(GL_LOAD, w);   // or GL_ACCUM the first image
 *    glAccum(GL_ACCUM, w);
 *    ...
 *    glAccum(GL_ACCUM, w);
 *    glAccum(GL_RETURN, 1.0);
 * That is, we start with an empty accumulation buffer and accumulate
 * n images, each with weight w = 1/n.
 * In this scenario, we can simply store unscaled integer values in
 * the accum buffer instead of scaled integers.  We'll also keep track
 * of the w value so when we do GL_RETURN we simply divide the accumulated
 * values by n (=1/w).
 * This lets us avoid _many_ int->float->int conversions.
 */


#if CHAN_BITS == 8 && ACCUM_BITS < 32
#define USE_OPTIMIZED_ACCUM   /* enable the optimization */
#endif


void
_swrast_alloc_accum_buffer( GLframebuffer *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint n;

   if (buffer->Accum) {
      MESA_PBUFFER_FREE( buffer->Accum );
      buffer->Accum = NULL;
   }

   /* allocate accumulation buffer if not already present */
   n = buffer->Width * buffer->Height * 4 * sizeof(GLaccum);
   buffer->Accum = (GLaccum *) MESA_PBUFFER_ALLOC( n );
   if (!buffer->Accum) {
      /* unable to setup accumulation buffer */
      _mesa_error( NULL, GL_OUT_OF_MEMORY, "glAccum" );
   }

   if (ctx) {
      SWcontext *swrast = SWRAST_CONTEXT(ctx);
      /* XXX these fields should probably be in the GLframebuffer */
#ifdef USE_OPTIMIZED_ACCUM
      swrast->_IntegerAccumMode = GL_TRUE;
#else
      swrast->_IntegerAccumMode = GL_FALSE;
#endif
      swrast->_IntegerAccumScaler = 0.0;
   }
}


/*
 * This is called when we fall out of optimized/unscaled accum buffer mode.
 * That is, we convert each unscaled accum buffer value into a scaled value
 * representing the range[-1, 1].
 */
static void rescale_accum( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint n = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height * 4;
   const GLfloat s = swrast->_IntegerAccumScaler * (32767.0F / CHAN_MAXF);
   GLaccum *accum = ctx->DrawBuffer->Accum;
   GLuint i;

   assert(swrast->_IntegerAccumMode);
   assert(accum);

   for (i = 0; i < n; i++) {
      accum[i] = (GLaccum) (accum[i] * s);
   }

   swrast->_IntegerAccumMode = GL_FALSE;
}






/*
 * Clear the accumulation Buffer.
 */
void
_swrast_clear_accum_buffer( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint buffersize;
   GLfloat acc_scale;

   if (ctx->Visual.accumRedBits==0) {
      /* No accumulation buffer! */
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      acc_scale = 1.0F;
   }

   /* number of pixels */
   buffersize = ctx->DrawBuffer->Width * ctx->DrawBuffer->Height;

   if (!ctx->DrawBuffer->Accum) {
      /* try to alloc accumulation buffer */
      ctx->DrawBuffer->Accum = (GLaccum *)
	                   MALLOC( buffersize * 4 * sizeof(GLaccum) );
   }

   if (ctx->DrawBuffer->Accum) {
      if (ctx->Scissor.Enabled) {
	 /* Limit clear to scissor box */
	 const GLaccum r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	 const GLaccum g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	 const GLaccum b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	 const GLaccum a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
	 GLint i, j;
         GLint width, height;
         GLaccum *row;
         /* size of region to clear */
         width = 4 * (ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin);
         height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
         /* ptr to first element to clear */
         row = ctx->DrawBuffer->Accum
               + 4 * (ctx->DrawBuffer->_Ymin * ctx->DrawBuffer->Width
                      + ctx->DrawBuffer->_Xmin);
         for (j=0;j<height;j++) {
            for (i=0;i<width;i+=4) {
               row[i+0] = r;
               row[i+1] = g;
               row[i+2] = b;
               row[i+3] = a;
	    }
            row += 4 * ctx->DrawBuffer->Width;
	 }
      }
      else {
	 /* clear whole buffer */
	 if (ctx->Accum.ClearColor[0]==0.0 &&
	     ctx->Accum.ClearColor[1]==0.0 &&
	     ctx->Accum.ClearColor[2]==0.0 &&
	     ctx->Accum.ClearColor[3]==0.0) {
	    /* Black */
	    _mesa_bzero( ctx->DrawBuffer->Accum,
                         buffersize * 4 * sizeof(GLaccum) );
	 }
	 else {
	    /* Not black */
	    const GLaccum r = (GLaccum) (ctx->Accum.ClearColor[0] * acc_scale);
	    const GLaccum g = (GLaccum) (ctx->Accum.ClearColor[1] * acc_scale);
	    const GLaccum b = (GLaccum) (ctx->Accum.ClearColor[2] * acc_scale);
	    const GLaccum a = (GLaccum) (ctx->Accum.ClearColor[3] * acc_scale);
	    GLaccum *acc = ctx->DrawBuffer->Accum;
	    GLuint i;
	    for (i=0;i<buffersize;i++) {
	       *acc++ = r;
	       *acc++ = g;
	       *acc++ = b;
	       *acc++ = a;
	    }
	 }
      }

      /* update optimized accum state vars */
      if (ctx->Accum.ClearColor[0] == 0.0 && ctx->Accum.ClearColor[1] == 0.0 &&
          ctx->Accum.ClearColor[2] == 0.0 && ctx->Accum.ClearColor[3] == 0.0) {
#ifdef USE_OPTIMIZED_ACCUM
         swrast->_IntegerAccumMode = GL_TRUE;
#else
         swrast->_IntegerAccumMode = GL_FALSE;
#endif
         swrast->_IntegerAccumScaler = 0.0;  /* denotes empty accum buffer */
      }
      else {
         swrast->_IntegerAccumMode = GL_FALSE;
      }
   }
}


void
_swrast_Accum( GLcontext *ctx, GLenum op, GLfloat value,
	       GLint xpos, GLint ypos,
	       GLint width, GLint height )

{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint width4;
   GLfloat acc_scale;
   GLchan rgba[MAX_WIDTH][4];
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);


   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived( ctx );

   if (!ctx->DrawBuffer->Accum) {
      _mesa_warning(ctx,
		    "Calling glAccum() without an accumulation "
		    "buffer (low memory?)");
      return;
   }

   if (sizeof(GLaccum)==1) {
      acc_scale = 127.0;
   }
   else if (sizeof(GLaccum)==2) {
      acc_scale = 32767.0;
   }
   else {
      acc_scale = 1.0F;
   }

   width4 = 4 * width;

   switch (op) {
      case GL_ADD:
         if (value != 0.0F) {
	    const GLaccum val = (GLaccum) (value * acc_scale);
	    GLint j;
            /* Leave optimized accum buffer mode */
            if (swrast->_IntegerAccumMode)
               rescale_accum(ctx);
	    for (j = 0; j < height; j++) {
	       GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + 4*xpos;
               GLuint i;
	       for (i = 0; i < width4; i++) {
                  acc[i] += val;
	       }
	       ypos++;
	    }
	 }
	 break;

      case GL_MULT:
         if (value != 1.0F) {
	    GLint j;
            /* Leave optimized accum buffer mode */
            if (swrast->_IntegerAccumMode)
               rescale_accum(ctx);
	    for (j = 0; j < height; j++) {
	       GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + 4 * xpos;
               GLuint i;
	       for (i = 0; i < width4; i++) {
                  acc[i] = (GLaccum) ( (GLfloat) acc[i] * value );
	       }
	       ypos++;
	    }
	 }
	 break;

      case GL_ACCUM:
         if (value == 0.0F)
            return;

         _swrast_use_read_buffer(ctx);

         /* May have to leave optimized accum buffer mode */
         if (swrast->_IntegerAccumScaler == 0.0 && value > 0.0 && value <= 1.0)
            swrast->_IntegerAccumScaler = value;
         if (swrast->_IntegerAccumMode && value != swrast->_IntegerAccumScaler)
            rescale_accum(ctx);

         RENDER_START(swrast,ctx);

         if (swrast->_IntegerAccumMode) {
            /* simply add integer color values into accum buffer */
            GLint j;
            GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
            assert(swrast->_IntegerAccumScaler > 0.0);
            assert(swrast->_IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {

               GLint i, i4;
               _swrast_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i = i4 = 0; i < width; i++, i4+=4) {
                  acc[i4+0] += rgba[i][RCOMP];
                  acc[i4+1] += rgba[i][GCOMP];
                  acc[i4+2] += rgba[i][BCOMP];
                  acc[i4+3] += rgba[i][ACOMP];
               }
               acc += width4;
               ypos++;
            }
         }
         else {
            /* scaled integer (or float) accum buffer */
            const GLfloat rscale = value * acc_scale / CHAN_MAXF;
            const GLfloat gscale = value * acc_scale / CHAN_MAXF;
            const GLfloat bscale = value * acc_scale / CHAN_MAXF;
            const GLfloat ascale = value * acc_scale / CHAN_MAXF;
            GLint j;
            for (j=0;j<height;j++) {
               GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
               GLint i;
               _swrast_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i=0;i<width;i++) {
                  acc[0] += (GLaccum) ( (GLfloat) rgba[i][RCOMP] * rscale );
                  acc[1] += (GLaccum) ( (GLfloat) rgba[i][GCOMP] * gscale );
                  acc[2] += (GLaccum) ( (GLfloat) rgba[i][BCOMP] * bscale );
                  acc[3] += (GLaccum) ( (GLfloat) rgba[i][ACOMP] * ascale );
                  acc += 4;
               }
               ypos++;
            }
         }
         /* restore read buffer = draw buffer (the default) */
         _swrast_use_draw_buffer(ctx);

         RENDER_FINISH(swrast,ctx);
	 break;

      case GL_LOAD:
         _swrast_use_read_buffer(ctx);

         /* This is a change to go into optimized accum buffer mode */
         if (value > 0.0 && value <= 1.0) {
#ifdef USE_OPTIMIZED_ACCUM
            swrast->_IntegerAccumMode = GL_TRUE;
#else
            swrast->_IntegerAccumMode = GL_FALSE;
#endif
            swrast->_IntegerAccumScaler = value;
         }
         else {
            swrast->_IntegerAccumMode = GL_FALSE;
            swrast->_IntegerAccumScaler = 0.0;
         }

         RENDER_START(swrast,ctx);
         if (swrast->_IntegerAccumMode) {
            /* just copy values into accum buffer */
            GLint j;
            GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
            assert(swrast->_IntegerAccumScaler > 0.0);
            assert(swrast->_IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {
               GLint i, i4;
               _swrast_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i = i4 = 0; i < width; i++, i4 += 4) {
                  acc[i4+0] = rgba[i][RCOMP];
                  acc[i4+1] = rgba[i][GCOMP];
                  acc[i4+2] = rgba[i][BCOMP];
                  acc[i4+3] = rgba[i][ACOMP];
               }
               acc += width4;
               ypos++;
            }
         }
         else {
            /* scaled integer (or float) accum buffer */
            const GLfloat rscale = value * acc_scale / CHAN_MAXF;
            const GLfloat gscale = value * acc_scale / CHAN_MAXF;
            const GLfloat bscale = value * acc_scale / CHAN_MAXF;
            const GLfloat ascale = value * acc_scale / CHAN_MAXF;
#if 0
            const GLfloat d = 3.0 / acc_scale;  /* XXX what's this? */
#endif
            GLint i, j;
            for (j = 0; j < height; j++) {
               GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos * 4;
               _swrast_read_rgba_span(ctx, ctx->DrawBuffer, width, xpos, ypos, rgba);
               for (i=0;i<width;i++) {
#if 0
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][RCOMP] * rscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][GCOMP] * gscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][BCOMP] * bscale + d);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][ACOMP] * ascale + d);
#else
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][RCOMP] * rscale);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][GCOMP] * gscale);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][BCOMP] * bscale);
                  *acc++ = (GLaccum) ((GLfloat) rgba[i][ACOMP] * ascale);
#endif
               }
               ypos++;
            }
         }

         /* restore read buffer = draw buffer (the default) */
         _swrast_use_draw_buffer(ctx);

         RENDER_FINISH(swrast,ctx);
	 break;

      case GL_RETURN:
         /* May have to leave optimized accum buffer mode */
         if (swrast->_IntegerAccumMode && value != 1.0)
            rescale_accum(ctx);

         RENDER_START(swrast,ctx);
#ifdef USE_OPTIMIZED_ACCUM
         if (swrast->_IntegerAccumMode && swrast->_IntegerAccumScaler > 0) {
            /* build lookup table to avoid many floating point multiplies */
            static GLchan multTable[32768];
            static GLfloat prevMult = 0.0;
            const GLfloat mult = swrast->_IntegerAccumScaler;
            const GLint max = MIN2((GLint) (256 / mult), 32767);
            GLint j;
            if (mult != prevMult) {
               for (j = 0; j < max; j++)
                  multTable[j] = IROUND((GLfloat) j * mult);
               prevMult = mult;
            }

            assert(swrast->_IntegerAccumScaler > 0.0);
            assert(swrast->_IntegerAccumScaler <= 1.0);
            for (j = 0; j < height; j++) {
               const GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos*4;
               GLint i, i4;
               for (i = i4 = 0; i < width; i++, i4 += 4) {
                  ASSERT(acc[i4+0] < max);
                  ASSERT(acc[i4+1] < max);
                  ASSERT(acc[i4+2] < max);
                  ASSERT(acc[i4+3] < max);
                  rgba[i][RCOMP] = multTable[acc[i4+0]];
                  rgba[i][GCOMP] = multTable[acc[i4+1]];
                  rgba[i][BCOMP] = multTable[acc[i4+2]];
                  rgba[i][ACOMP] = multTable[acc[i4+3]];
               }
               if (colorMask != 0xffffffff) {
                  _swrast_mask_rgba_array( ctx, width, xpos, ypos, rgba );
               }
               (*swrast->Driver.WriteRGBASpan)( ctx, width, xpos, ypos,
                                             (const GLchan (*)[4])rgba, NULL );
               if (ctx->DrawBuffer->UseSoftwareAlphaBuffers
                   && ctx->Color.ColorMask[ACOMP]) {
                  _swrast_write_alpha_span(ctx, width, xpos, ypos,
                                         (CONST GLchan (*)[4]) rgba, NULL);
               }
               ypos++;
            }
         }
         else
#endif /* USE_OPTIMIZED_ACCUM */
         {
            /* scaled integer (or float) accum buffer */
            const GLfloat rscale = value / acc_scale * CHAN_MAXF;
            const GLfloat gscale = value / acc_scale * CHAN_MAXF;
            const GLfloat bscale = value / acc_scale * CHAN_MAXF;
            const GLfloat ascale = value / acc_scale * CHAN_MAXF;
            GLint i, j;
            for (j=0;j<height;j++) {
               const GLaccum *acc = ctx->DrawBuffer->Accum + ypos * width4 + xpos*4;
               for (i=0;i<width;i++) {
                  GLint r = IROUND( (GLfloat) (acc[0]) * rscale );
                  GLint g = IROUND( (GLfloat) (acc[1]) * gscale );
                  GLint b = IROUND( (GLfloat) (acc[2]) * bscale );
                  GLint a = IROUND( (GLfloat) (acc[3]) * ascale );
                  acc += 4;
                  rgba[i][RCOMP] = CLAMP( r, 0, CHAN_MAX );
                  rgba[i][GCOMP] = CLAMP( g, 0, CHAN_MAX );
                  rgba[i][BCOMP] = CLAMP( b, 0, CHAN_MAX );
                  rgba[i][ACOMP] = CLAMP( a, 0, CHAN_MAX );
               }
               if (colorMask != 0xffffffff) {
                  _swrast_mask_rgba_array( ctx, width, xpos, ypos, rgba );
               }
               (*swrast->Driver.WriteRGBASpan)( ctx, width, xpos, ypos,
                                             (const GLchan (*)[4])rgba, NULL );
               if (ctx->DrawBuffer->UseSoftwareAlphaBuffers
                   && ctx->Color.ColorMask[ACOMP]) {
                  _swrast_write_alpha_span(ctx, width, xpos, ypos,
                                         (CONST GLchan (*)[4]) rgba, NULL);
               }
               ypos++;
            }
	 }
         RENDER_FINISH(swrast,ctx);
	 break;

      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glAccum" );
   }
}
