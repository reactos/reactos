/*
 * Mesa 3-D graphics library
 * Version:  6.5.2
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
#include "fbobject.h"

#include "s_accum.h"
#include "s_context.h"
#include "s_masking.h"
#include "s_span.h"


/* XXX this would have to change for accum buffers with more or less
 * than 16 bits per color channel.
 */
#define ACCUM_SCALE16 32767.0


/*
 * Accumulation buffer notes
 *
 * Normally, accumulation buffer values are GLshorts with values in
 * [-32767, 32767] which represent floating point colors in [-1, 1],
 * as defined by the OpenGL specification.
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
 * values by n (n=1/w).
 * This lets us avoid _many_ int->float->int conversions.
 */


#if CHAN_BITS == 8
/* enable the optimization */
#define USE_OPTIMIZED_ACCUM  1
#else
#define USE_OPTIMIZED_ACCUM  0
#endif


/**
 * This is called when we fall out of optimized/unscaled accum buffer mode.
 * That is, we convert each unscaled accum buffer value into a scaled value
 * representing the range[-1, 1].
 */
static void
rescale_accum( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb
      = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;
   const GLfloat s = swrast->_IntegerAccumScaler * (32767.0F / CHAN_MAXF);

   assert(rb);
   assert(rb->_BaseFormat == GL_RGBA);
   /* add other types in future? */
   assert(rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT);
   assert(swrast->_IntegerAccumMode);

   if (rb->GetPointer(ctx, rb, 0, 0)) {
      /* directly-addressable memory */
      GLuint y;
      for (y = 0; y < rb->Height; y++) {
         GLuint i;
         GLshort *acc = (GLshort *) rb->GetPointer(ctx, rb, 0, y);
         for (i = 0; i < 4 * rb->Width; i++) {
            acc[i] = (GLshort) (acc[i] * s);
         }
      }
   }
   else {
      /* use get/put row funcs */
      GLuint y;
      for (y = 0; y < rb->Height; y++) {
         GLshort accRow[MAX_WIDTH * 4];
         GLuint i;
         rb->GetRow(ctx, rb, rb->Width, 0, y, accRow);
         for (i = 0; i < 4 * rb->Width; i++) {
            accRow[i] = (GLshort) (accRow[i] * s);
         }
         rb->PutRow(ctx, rb, rb->Width, 0, y, accRow, NULL);
      }
   }

   swrast->_IntegerAccumMode = GL_FALSE;
}



/**
 * Clear the accumulation Buffer.
 */
void
_swrast_clear_accum_buffer( GLcontext *ctx, struct gl_renderbuffer *rb )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint x, y, width, height;

   if (ctx->Visual.accumRedBits == 0) {
      /* No accumulation buffer! Not an error. */
      return;
   }

   if (!rb || !rb->Data)
      return;

   assert(rb->_BaseFormat == GL_RGBA);
   /* add other types in future? */
   assert(rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT);

   /* bounds, with scissor */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   if (rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT) {
      const GLfloat accScale = 32767.0;
      GLshort clearVal[4];
      GLuint i;

      clearVal[0] = (GLshort) (ctx->Accum.ClearColor[0] * accScale);
      clearVal[1] = (GLshort) (ctx->Accum.ClearColor[1] * accScale);
      clearVal[2] = (GLshort) (ctx->Accum.ClearColor[2] * accScale);
      clearVal[3] = (GLshort) (ctx->Accum.ClearColor[3] * accScale);

      for (i = 0; i < height; i++) {
         rb->PutMonoRow(ctx, rb, width, x, y + i, clearVal, NULL);
      }
   }
   else {
      /* someday support other sizes */
   }

   /* update optimized accum state vars */
   if (ctx->Accum.ClearColor[0] == 0.0 && ctx->Accum.ClearColor[1] == 0.0 &&
       ctx->Accum.ClearColor[2] == 0.0 && ctx->Accum.ClearColor[3] == 0.0) {
#if USE_OPTIMIZED_ACCUM
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


static void
accum_add(GLcontext *ctx, GLfloat value,
          GLint xpos, GLint ypos, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb
      = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;

   assert(rb);

   /* Leave optimized accum buffer mode */
   if (swrast->_IntegerAccumMode)
      rescale_accum(ctx);

   if (rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT) {
      const GLshort incr = (GLshort) (value * ACCUM_SCALE16);
      if (rb->GetPointer(ctx, rb, 0, 0)) {
         GLint i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) rb->GetPointer(ctx, rb, xpos, ypos + i);
            for (j = 0; j < 4 * width; j++) {
               acc[j] += incr;
            }
         }
      }
      else {
         GLint i, j;
         for (i = 0; i < height; i++) {
            GLshort accRow[4 * MAX_WIDTH];
            rb->GetRow(ctx, rb, width, xpos, ypos + i, accRow);
            for (j = 0; j < 4 * width; j++) {
               accRow[j] += incr;
            }
            rb->PutRow(ctx, rb, width, xpos, ypos + i, accRow, NULL);
         }
      }
   }
   else {
      /* other types someday */
   }
}


static void
accum_mult(GLcontext *ctx, GLfloat mult,
           GLint xpos, GLint ypos, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb
      = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;

   assert(rb);

   /* Leave optimized accum buffer mode */
   if (swrast->_IntegerAccumMode)
      rescale_accum(ctx);

   if (rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT) {
      if (rb->GetPointer(ctx, rb, 0, 0)) {
         GLint i, j;
         for (i = 0; i < height; i++) {
            GLshort *acc = (GLshort *) rb->GetPointer(ctx, rb, xpos, ypos + i);
            for (j = 0; j < 4 * width; j++) {
               acc[j] = (GLshort) (acc[j] * mult);
            }
         }
      }
      else {
         GLint i, j;
         for (i = 0; i < height; i++) {
            GLshort accRow[4 * MAX_WIDTH];
            rb->GetRow(ctx, rb, width, xpos, ypos + i, accRow);
            for (j = 0; j < 4 * width; j++) {
               accRow[j] = (GLshort) (accRow[j] * mult);
            }
            rb->PutRow(ctx, rb, width, xpos, ypos + i, accRow, NULL);
         }
      }
   }
   else {
      /* other types someday */
   }
}



static void
accum_accum(GLcontext *ctx, GLfloat value,
            GLint xpos, GLint ypos, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb
      = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;
   const GLboolean directAccess = (rb->GetPointer(ctx, rb, 0, 0) != NULL);

   assert(rb);

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no read buffer - OK */
      return;
   }

   /* May have to leave optimized accum buffer mode */
   if (swrast->_IntegerAccumScaler == 0.0 && value > 0.0 && value <= 1.0)
      swrast->_IntegerAccumScaler = value;
   if (swrast->_IntegerAccumMode && value != swrast->_IntegerAccumScaler)
      rescale_accum(ctx);

   if (rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT) {
      const GLfloat scale = value * ACCUM_SCALE16 / CHAN_MAXF;
      GLshort accumRow[4 * MAX_WIDTH];
      GLchan rgba[MAX_WIDTH][4];
      GLint i;

      for (i = 0; i < height; i++) {
         GLshort *acc;
         if (directAccess) {
            acc = (GLshort *) rb->GetPointer(ctx, rb, xpos, ypos + i);
         }
         else {
            rb->GetRow(ctx, rb, width, xpos, ypos + i, accumRow);
            acc = accumRow;
         }

         /* read colors from color buffer */
         _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer, width,
                                xpos, ypos + i, CHAN_TYPE, rgba);

         /* do accumulation */
         if (swrast->_IntegerAccumMode) {
            /* simply add integer color values into accum buffer */
            GLint j;
            for (j = 0; j < width; j++) {
               acc[j * 4 + 0] += rgba[j][RCOMP];
               acc[j * 4 + 1] += rgba[j][GCOMP];
               acc[j * 4 + 2] += rgba[j][BCOMP];
               acc[j * 4 + 3] += rgba[j][ACOMP];
            }
         }
         else {
            /* scaled integer (or float) accum buffer */
            GLint j;
            for (j = 0; j < width; j++) {
               acc[j * 4 + 0] += (GLshort) ((GLfloat) rgba[j][RCOMP] * scale);
               acc[j * 4 + 1] += (GLshort) ((GLfloat) rgba[j][GCOMP] * scale);
               acc[j * 4 + 2] += (GLshort) ((GLfloat) rgba[j][BCOMP] * scale);
               acc[j * 4 + 3] += (GLshort) ((GLfloat) rgba[j][ACOMP] * scale);
            }
         }

         if (!directAccess) {
            rb->PutRow(ctx, rb, width, xpos, ypos + i, accumRow, NULL);
         }
      }
   }
   else {
      /* other types someday */
   }
}



static void
accum_load(GLcontext *ctx, GLfloat value,
           GLint xpos, GLint ypos, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb
      = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;
   const GLboolean directAccess = (rb->GetPointer(ctx, rb, 0, 0) != NULL);

   assert(rb);

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no read buffer - OK */
      return;
   }

   /* This is a change to go into optimized accum buffer mode */
   if (value > 0.0 && value <= 1.0) {
#if USE_OPTIMIZED_ACCUM
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

   if (rb->DataType == GL_SHORT || rb->DataType == GL_UNSIGNED_SHORT) {
      const GLfloat scale = value * ACCUM_SCALE16 / CHAN_MAXF;
      GLshort accumRow[4 * MAX_WIDTH];
      GLchan rgba[MAX_WIDTH][4];
      GLint i;

      for (i = 0; i < height; i++) {
         GLshort *acc;
         if (directAccess) {
            acc = (GLshort *) rb->GetPointer(ctx, rb, xpos, ypos + i);
         }
         else {
            rb->GetRow(ctx, rb, width, xpos, ypos + i, accumRow);
            acc = accumRow;
         }

         /* read colors from color buffer */
         _swrast_read_rgba_span(ctx, ctx->ReadBuffer->_ColorReadBuffer, width,
                                xpos, ypos + i, CHAN_TYPE, rgba);

         /* do load */
         if (swrast->_IntegerAccumMode) {
            /* just copy values in */
            GLint j;
            assert(swrast->_IntegerAccumScaler > 0.0);
            assert(swrast->_IntegerAccumScaler <= 1.0);
            for (j = 0; j < width; j++) {
               acc[j * 4 + 0] = rgba[j][RCOMP];
               acc[j * 4 + 1] = rgba[j][GCOMP];
               acc[j * 4 + 2] = rgba[j][BCOMP];
               acc[j * 4 + 3] = rgba[j][ACOMP];
            }
         }
         else {
            /* scaled integer (or float) accum buffer */
            GLint j;
            for (j = 0; j < width; j++) {
               acc[j * 4 + 0] = (GLshort) ((GLfloat) rgba[j][RCOMP] * scale);
               acc[j * 4 + 1] = (GLshort) ((GLfloat) rgba[j][GCOMP] * scale);
               acc[j * 4 + 2] = (GLshort) ((GLfloat) rgba[j][BCOMP] * scale);
               acc[j * 4 + 3] = (GLshort) ((GLfloat) rgba[j][ACOMP] * scale);
            }
         }

         if (!directAccess) {
            rb->PutRow(ctx, rb, width, xpos, ypos + i, accumRow, NULL);
         }
      }
   }
}


static void
accum_return(GLcontext *ctx, GLfloat value,
             GLint xpos, GLint ypos, GLint width, GLint height )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *accumRb = fb->Attachment[BUFFER_ACCUM].Renderbuffer;
   const GLboolean directAccess
      = (accumRb->GetPointer(ctx, accumRb, 0, 0) != NULL);
   const GLboolean masking = (!ctx->Color.ColorMask[RCOMP] ||
                              !ctx->Color.ColorMask[GCOMP] ||
                              !ctx->Color.ColorMask[BCOMP] ||
                              !ctx->Color.ColorMask[ACOMP]);

   static GLchan multTable[32768];
   static GLfloat prevMult = 0.0;
   const GLfloat mult = swrast->_IntegerAccumScaler;
   const GLint max = MIN2((GLint) (256 / mult), 32767);

   /* May have to leave optimized accum buffer mode */
   if (swrast->_IntegerAccumMode && value != 1.0)
      rescale_accum(ctx);

   if (swrast->_IntegerAccumMode && swrast->_IntegerAccumScaler > 0) {
      /* build lookup table to avoid many floating point multiplies */
      GLint j;
      assert(swrast->_IntegerAccumScaler <= 1.0);
      if (mult != prevMult) {
         for (j = 0; j < max; j++)
            multTable[j] = IROUND((GLfloat) j * mult);
         prevMult = mult;
      }
   }

   if (accumRb->DataType == GL_SHORT ||
       accumRb->DataType == GL_UNSIGNED_SHORT) {
      const GLfloat scale = value * CHAN_MAXF / ACCUM_SCALE16;
      GLuint buffer;
      GLint i;

      /* XXX maybe transpose the 'i' and 'buffer' loops??? */
      for (i = 0; i < height; i++) {
         GLshort accumRow[4 * MAX_WIDTH];
         GLshort *acc;
         SWspan span;

         /* init color span */
         INIT_SPAN(span, GL_BITMAP, width, 0, SPAN_RGBA);
         span.x = xpos;
         span.y = ypos + i;

         if (directAccess) {
            acc = (GLshort *) accumRb->GetPointer(ctx, accumRb, xpos, ypos +i);
         }
         else {
            accumRb->GetRow(ctx, accumRb, width, xpos, ypos + i, accumRow);
            acc = accumRow;
         }

         /* get the colors to return */
         if (swrast->_IntegerAccumMode) {
            GLint j;
            for (j = 0; j < width; j++) {
               ASSERT(acc[j * 4 + 0] < max);
               ASSERT(acc[j * 4 + 1] < max);
               ASSERT(acc[j * 4 + 2] < max);
               ASSERT(acc[j * 4 + 3] < max);
               span.array->rgba[j][RCOMP] = multTable[acc[j * 4 + 0]];
               span.array->rgba[j][GCOMP] = multTable[acc[j * 4 + 1]];
               span.array->rgba[j][BCOMP] = multTable[acc[j * 4 + 2]];
               span.array->rgba[j][ACOMP] = multTable[acc[j * 4 + 3]];
            }
         }
         else {
            /* scaled integer (or float) accum buffer */
            GLint j;
            for (j = 0; j < width; j++) {
#if CHAN_BITS==32
               GLchan r = acc[j * 4 + 0] * scale;
               GLchan g = acc[j * 4 + 1] * scale;
               GLchan b = acc[j * 4 + 2] * scale;
               GLchan a = acc[j * 4 + 3] * scale;
#else
               GLint r = IROUND( (GLfloat) (acc[j * 4 + 0]) * scale );
               GLint g = IROUND( (GLfloat) (acc[j * 4 + 1]) * scale );
               GLint b = IROUND( (GLfloat) (acc[j * 4 + 2]) * scale );
               GLint a = IROUND( (GLfloat) (acc[j * 4 + 3]) * scale );
#endif
               span.array->rgba[j][RCOMP] = CLAMP( r, 0, CHAN_MAX );
               span.array->rgba[j][GCOMP] = CLAMP( g, 0, CHAN_MAX );
               span.array->rgba[j][BCOMP] = CLAMP( b, 0, CHAN_MAX );
               span.array->rgba[j][ACOMP] = CLAMP( a, 0, CHAN_MAX );
            }
         }

         /* store colors */
         for (buffer = 0; buffer < fb->_NumColorDrawBuffers[0]; buffer++) {
            struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[0][buffer];
            if (masking) {
               _swrast_mask_rgba_span(ctx, rb, &span);
            }
            rb->PutRow(ctx, rb, width, xpos, ypos + i, span.array->rgba, NULL);
         }
      }
   }
   else {
      /* other types someday */
   }
}



/**
 * Software fallback for glAccum.
 */
void
_swrast_Accum(GLcontext *ctx, GLenum op, GLfloat value)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLint xpos, ypos, width, height;

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived( ctx );

   if (!ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer) {
      _mesa_warning(ctx, "Calling glAccum() without an accumulation buffer");
      return;
   }

   RENDER_START(swrast, ctx);

   /* Compute region after calling RENDER_START so that we know the
    * drawbuffer's size/bounds are up to date.
    */
   xpos = ctx->DrawBuffer->_Xmin;
   ypos = ctx->DrawBuffer->_Ymin;
   width =  ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   switch (op) {
      case GL_ADD:
         if (value != 0.0F) {
            accum_add(ctx, value, xpos, ypos, width, height);
	 }
	 break;
      case GL_MULT:
         if (value != 1.0F) {
            accum_mult(ctx, value, xpos, ypos, width, height);
	 }
	 break;
      case GL_ACCUM:
         if (value != 0.0F) {
            accum_accum(ctx, value, xpos, ypos, width, height);
         }
	 break;
      case GL_LOAD:
         accum_load(ctx, value, xpos, ypos, width, height);
	 break;
      case GL_RETURN:
         accum_return(ctx, value, xpos, ypos, width, height);
	 break;
      default:
         _mesa_problem(ctx, "invalid mode in _swrast_Accum()");
         break;
   }

   RENDER_FINISH(swrast, ctx);
}
