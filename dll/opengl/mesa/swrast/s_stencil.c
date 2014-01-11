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

#include <precomp.h>

/* Stencil Logic:

IF stencil test fails THEN
   Apply fail-op to stencil value
   Don't write the pixel (RGBA,Z)
ELSE
   IF doing depth test && depth test fails THEN
      Apply zfail-op to stencil value
      Write RGBA and Z to appropriate buffers
   ELSE
      Apply zpass-op to stencil value
ENDIF

*/



/**
 * Compute/return the offset of the stencil value in a pixel.
 * For example, if the format is Z24+S8, the position of the stencil bits
 * within the 4-byte pixel will be either 0 or 3.
 */
static GLint
get_stencil_offset(gl_format format)
{
   const GLubyte one = 1;
   GLubyte pixel[MAX_PIXEL_BYTES];
   GLint bpp = _mesa_get_format_bytes(format);
   GLint i;

   assert(_mesa_get_format_bits(format, GL_STENCIL_BITS) == 8);
   memset(pixel, 0, sizeof(pixel));
   _mesa_pack_ubyte_stencil_row(format, 1, &one, pixel);

   for (i = 0; i < bpp; i++) {
      if (pixel[i])
         return i;
   }

   _mesa_problem(NULL, "get_stencil_offset() failed\n");
   return 0;
}


/** Clamp the stencil value to [0, 255] */
static inline GLubyte
clamp(GLint val)
{
   if (val < 0)
      return 0;
   else if (val > 255)
      return 255;
   else
      return val;
}


#define STENCIL_OP(NEW_VAL)                                                 \
   if (invmask == 0) {                                                      \
      for (i = j = 0; i < n; i++, j += stride) {                            \
         if (mask[i]) {                                                     \
            GLubyte s = stencil[j];                                         \
            (void) s;                                                       \
            stencil[j] = (GLubyte) (NEW_VAL);                               \
         }                                                                  \
      }                                                                     \
   }                                                                        \
   else {                                                                   \
      for (i = j = 0; i < n; i++, j += stride) {                            \
         if (mask[i]) {                                                     \
            GLubyte s = stencil[j];                                         \
            stencil[j] = (GLubyte) ((invmask & s) | (wrtmask & (NEW_VAL))); \
         }                                                                  \
      }                                                                     \
   }


/**
 * Apply the given stencil operator to the array of stencil values.
 * Don't touch stencil[i] if mask[i] is zero.
 * @param n   number of stencil values
 * @param oper  the stencil buffer operator
 * @param face  0 or 1 for front or back face operation
 * @param stencil  array of stencil values (in/out)
 * @param mask  array [n] of flag:  1=apply operator, 0=don't apply operator
 * @param stride  stride between stencil values
 */
static void
apply_stencil_op(const struct gl_context *ctx, GLenum oper,
                 GLuint n, GLubyte stencil[], const GLubyte mask[],
                 GLint stride)
{
   const GLubyte ref = ctx->Stencil.Ref;
   const GLubyte wrtmask = ctx->Stencil.WriteMask;
   const GLubyte invmask = (GLubyte) (~wrtmask);
   GLuint i, j;

   switch (oper) {
   case GL_KEEP:
      /* do nothing */
      break;
   case GL_ZERO:
      /* replace stencil buf values with zero */
      STENCIL_OP(0);
      break;
   case GL_REPLACE:
      /* replace stencil buf values with ref value */
      STENCIL_OP(ref);
      break;
   case GL_INCR:
      /* increment stencil buf values, with clamping */
      STENCIL_OP(clamp(s + 1));
      break;
   case GL_DECR:
      /* increment stencil buf values, with clamping */
      STENCIL_OP(clamp(s - 1));
      break;
   case GL_INCR_WRAP_EXT:
      /* increment stencil buf values, without clamping */
      STENCIL_OP(s + 1);
      break;
   case GL_DECR_WRAP_EXT:
      /* increment stencil buf values, without clamping */
      STENCIL_OP(s - 1);
      break;
   case GL_INVERT:
      /* replace stencil buf values with inverted value */
      STENCIL_OP(~s);
      break;
   default:
      _mesa_problem(ctx, "Bad stencil op in apply_stencil_op");
   }
}



#define STENCIL_TEST(FUNC)                        \
   for (i = j = 0; i < n; i++, j += stride) {     \
      if (mask[i]) {                              \
         s = (GLubyte) (stencil[j] & valueMask);  \
         if (FUNC) {                              \
            /* stencil pass */                    \
            fail[i] = 0;                          \
         }                                        \
         else {                                   \
            /* stencil fail */                    \
            fail[i] = 1;                          \
            mask[i] = 0;                          \
         }                                        \
      }                                           \
      else {                                      \
         fail[i] = 0;                             \
      }                                           \
   }



/**
 * Apply stencil test to an array of stencil values (before depth buffering).
 * For the values that fail, we'll apply the GL_STENCIL_FAIL operator to
 * the stencil values.
 *
 * @param face  0 or 1 for front or back-face polygons
 * @param n  number of pixels in the array
 * @param stencil  array of [n] stencil values (in/out)
 * @param mask  array [n] of flag:  0=skip the pixel, 1=stencil the pixel,
 *              values are set to zero where the stencil test fails.
 * @param stride  stride between stencil values
 * @return GL_FALSE = all pixels failed, GL_TRUE = zero or more pixels passed.
 */
static GLboolean
do_stencil_test(struct gl_context *ctx, GLuint n,
                GLubyte stencil[], GLubyte mask[], GLint stride)
{
   GLubyte fail[MAX_WIDTH];
   GLboolean allfail = GL_FALSE;
   GLuint i, j;
   const GLuint valueMask = ctx->Stencil.ValueMask;
   const GLubyte ref = (GLubyte) (ctx->Stencil.Ref & valueMask);
   GLubyte s;

   /*
    * Perform stencil test.  The results of this operation are stored
    * in the fail[] array:
    *   IF fail[i] is non-zero THEN
    *       the stencil fail operator is to be applied
    *   ELSE
    *       the stencil fail operator is not to be applied
    *   ENDIF
    */
   switch (ctx->Stencil.Function) {
   case GL_NEVER:
      STENCIL_TEST(0);
      allfail = GL_TRUE;
      break;
   case GL_LESS:
      STENCIL_TEST(ref < s);
      break;
   case GL_LEQUAL:
      STENCIL_TEST(ref <= s);
      break;
   case GL_GREATER:
      STENCIL_TEST(ref > s);
      break;
   case GL_GEQUAL:
      STENCIL_TEST(ref >= s);
      break;
   case GL_EQUAL:
      STENCIL_TEST(ref == s);
      break;
   case GL_NOTEQUAL:
      STENCIL_TEST(ref != s);
      break;
   case GL_ALWAYS:
      STENCIL_TEST(1);
      break;
   default:
      _mesa_problem(ctx, "Bad stencil func in gl_stencil_span");
      return 0;
   }

   if (ctx->Stencil.FailFunc != GL_KEEP) {
      apply_stencil_op(ctx, ctx->Stencil.FailFunc, n, stencil,
                       fail, stride);
   }

   return !allfail;
}


/**
 * Compute the zpass/zfail masks by comparing the pre- and post-depth test
 * masks.
 */
static inline void
compute_pass_fail_masks(GLuint n, const GLubyte origMask[],
                        const GLubyte newMask[],
                        GLubyte passMask[], GLubyte failMask[])
{
   GLuint i;
   for (i = 0; i < n; i++) {
      ASSERT(newMask[i] == 0 || newMask[i] == 1);
      passMask[i] = origMask[i] & newMask[i];
      failMask[i] = origMask[i] & (newMask[i] ^ 1);
   }
}


/**
 * Get 8-bit stencil values from random locations in the stencil buffer.
 */
static void
get_s8_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
              GLuint count, const GLint x[], const GLint y[],
              GLubyte stencil[])
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
   const GLint w = rb->Width, h = rb->Height;
   const GLubyte *map = _swrast_pixel_address(rb, 0, 0);
   GLuint i;

   if (rb->Format == MESA_FORMAT_S8) {
      const GLint rowStride = srb->RowStride;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            stencil[i] = *(map + y[i] * rowStride + x[i]);
         }
      }
   }
   else {
      const GLint bpp = _mesa_get_format_bytes(rb->Format);
      const GLint rowStride = srb->RowStride;
      for (i = 0; i < count; i++) {
         if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
            const GLubyte *src = map + y[i] * rowStride + x[i] * bpp;
            _mesa_unpack_ubyte_stencil_row(rb->Format, 1, src, &stencil[i]);
         }
      }
   }
}


/**
 * Put 8-bit stencil values at random locations into the stencil buffer.
 */
static void
put_s8_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
              GLuint count, const GLint x[], const GLint y[],
              const GLubyte stencil[])
{
   const GLint w = rb->Width, h = rb->Height;
   gl_pack_ubyte_stencil_func pack_stencil =
      _mesa_get_pack_ubyte_stencil_func(rb->Format);
   GLuint i;

   for (i = 0; i < count; i++) {
      if (x[i] >= 0 && y[i] >= 0 && x[i] < w && y[i] < h) {
         GLubyte *dst = _swrast_pixel_address(rb, x[i], y[i]);
         pack_stencil(&stencil[i], dst);
      }
   }
}


/**
 * /return GL_TRUE = one or more fragments passed,
 * GL_FALSE = all fragments failed.
 */
GLboolean
_swrast_stencil_and_ztest_span(struct gl_context *ctx, SWspan *span)
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   const GLint stencilOffset = get_stencil_offset(rb->Format);
   const GLint stencilStride = _mesa_get_format_bytes(rb->Format);
   const GLuint count = span->end;
   GLubyte *mask = span->array->mask;
   GLubyte stencilTemp[MAX_WIDTH];
   GLubyte *stencilBuf;

   if (span->arrayMask & SPAN_XY) {
      /* read stencil values from random locations */
      get_s8_values(ctx, rb, count, span->array->x, span->array->y,
                    stencilTemp);
      stencilBuf = stencilTemp;
   }
   else {
      /* Processing a horizontal run of pixels.  Since stencil is always
       * 8 bits for all MESA_FORMATs, we just need to use the right offset
       * and stride to access them.
       */
      stencilBuf = _swrast_pixel_address(rb, span->x, span->y) + stencilOffset;
   }

   /*
    * Apply the stencil test to the fragments.
    * failMask[i] is 1 if the stencil test failed.
    */
   if (!do_stencil_test(ctx, count, stencilBuf, mask, stencilStride)) {
      /* all fragments failed the stencil test, we're done. */
      span->writeAll = GL_FALSE;
      if (span->arrayMask & SPAN_XY) {
         /* need to write the updated stencil values back to the buffer */
         put_s8_values(ctx, rb, count, span->array->x, span->array->y,
                       stencilTemp);
      }
      return GL_FALSE;
   }

   /*
    * Some fragments passed the stencil test, apply depth test to them
    * and apply Zpass and Zfail stencil ops.
    */
   if (ctx->Depth.Test == GL_FALSE ||
       ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer == NULL) {
      /*
       * No depth buffer, just apply zpass stencil function to active pixels.
       */
      apply_stencil_op(ctx, ctx->Stencil.ZPassFunc, count,
                       stencilBuf, mask, stencilStride);
   }
   else {
      /*
       * Perform depth buffering, then apply zpass or zfail stencil function.
       */
      GLubyte passMask[MAX_WIDTH], failMask[MAX_WIDTH], origMask[MAX_WIDTH];

      /* save the current mask bits */
      memcpy(origMask, mask, count * sizeof(GLubyte));

      /* apply the depth test */
      _swrast_depth_test_span(ctx, span);

      compute_pass_fail_masks(count, origMask, mask, passMask, failMask);

      /* apply the pass and fail operations */
      if (ctx->Stencil.ZFailFunc != GL_KEEP) {
         apply_stencil_op(ctx, ctx->Stencil.ZFailFunc,
                          count, stencilBuf, failMask, stencilStride);
      }
      if (ctx->Stencil.ZPassFunc != GL_KEEP) {
         apply_stencil_op(ctx, ctx->Stencil.ZPassFunc,
                          count, stencilBuf, passMask, stencilStride);
      }
   }

   /* Write updated stencil values back into hardware stencil buffer */
   if (span->arrayMask & SPAN_XY) {
      put_s8_values(ctx, rb, count, span->array->x, span->array->y,
                    stencilBuf);
   }
   
   span->writeAll = GL_FALSE;
   
   return GL_TRUE;  /* one or more fragments passed both tests */
}




/**
 * Return a span of stencil values from the stencil buffer.
 * Used for glRead/CopyPixels
 * Input:  n - how many pixels
 *         x,y - location of first pixel
 * Output:  stencil - the array of stencil values
 */
void
_swrast_read_stencil_span(struct gl_context *ctx, struct gl_renderbuffer *rb,
                          GLint n, GLint x, GLint y, GLubyte stencil[])
{
   GLubyte *src;

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }

   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   src = _swrast_pixel_address(rb, x, y);
   _mesa_unpack_ubyte_stencil_row(rb->Format, n, src, stencil);
}



/**
 * Write a span of stencil values to the stencil buffer.  This function
 * applies the stencil write mask when needed.
 * Used for glDraw/CopyPixels
 * Input:  n - how many pixels
 *         x, y - location of first pixel
 *         stencil - the array of stencil values
 */
void
_swrast_write_stencil_span(struct gl_context *ctx, GLint n, GLint x, GLint y,
                           const GLubyte stencil[] )
{
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   const GLuint stencilMax = (1 << fb->Visual.stencilBits) - 1;
   const GLuint stencilMask = ctx->Stencil.WriteMask;
   GLubyte *stencilBuf;

   if (y < 0 || y >= (GLint) rb->Height ||
       x + n <= 0 || x >= (GLint) rb->Width) {
      /* span is completely outside framebuffer */
      return; /* undefined values OK */
   }
   if (x < 0) {
      GLint dx = -x;
      x = 0;
      n -= dx;
      stencil += dx;
   }
   if (x + n > (GLint) rb->Width) {
      GLint dx = x + n - rb->Width;
      n -= dx;
   }
   if (n <= 0) {
      return;
   }

   stencilBuf = _swrast_pixel_address(rb, x, y);

   if ((stencilMask & stencilMax) != stencilMax) {
      /* need to apply writemask */
      GLubyte destVals[MAX_WIDTH], newVals[MAX_WIDTH];
      GLint i;

      _mesa_unpack_ubyte_stencil_row(rb->Format, n, stencilBuf, destVals);
      for (i = 0; i < n; i++) {
         newVals[i]
            = (stencil[i] & stencilMask) | (destVals[i] & ~stencilMask);
      }
      _mesa_pack_ubyte_stencil_row(rb->Format, n, newVals, stencilBuf);
   }
   else {
      _mesa_pack_ubyte_stencil_row(rb->Format, n, stencil, stencilBuf);
   }
}



/**
 * Clear the stencil buffer.  If the buffer is a combined
 * depth+stencil buffer, only the stencil bits will be touched.
 */
void
_swrast_clear_stencil_buffer(struct gl_context *ctx)
{
   struct gl_renderbuffer *rb =
      ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   const GLubyte stencilBits = ctx->DrawBuffer->Visual.stencilBits;
   const GLuint writeMask = ctx->Stencil.WriteMask;
   const GLuint stencilMax = (1 << stencilBits) - 1;
   GLint x, y, width, height;
   GLubyte *map;
   GLint rowStride, i, j;
   GLbitfield mapMode;

   if (!rb || writeMask == 0)
      return;

   /* compute region to clear */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;

   mapMode = GL_MAP_WRITE_BIT;
   if ((writeMask & stencilMax) != stencilMax) {
      /* need to mask stencil values */
      mapMode |= GL_MAP_READ_BIT;
   }
   else if (_mesa_get_format_bits(rb->Format, GL_DEPTH_BITS) > 0) {
      /* combined depth+stencil, need to mask Z values */
      mapMode |= GL_MAP_READ_BIT;
   }

   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               mapMode, &map, &rowStride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClear(stencil)");
      return;
   }

   switch (rb->Format) {
   case MESA_FORMAT_S8:
      {
         GLubyte clear = ctx->Stencil.Clear & writeMask & 0xff;
         GLubyte mask = (~writeMask) & 0xff;
         if (mask != 0) {
            /* masked clear */
            for (i = 0; i < height; i++) {
               GLubyte *row = map;
               for (j = 0; j < width; j++) {
                  row[j] = (row[j] & mask) | clear;
               }
               map += rowStride;
            }
         }
         else if (rowStride == width) {
            /* clear whole buffer */
            memset(map, clear, width * height);
         }
         else {
            /* clear scissored */
            for (i = 0; i < height; i++) {
               memset(map, clear, width);
               map += rowStride;
            }
         }
      }
      break;
   default:
      _mesa_problem(ctx, "Unexpected stencil buffer format %s"
                    " in _swrast_clear_stencil_buffer()",
                    _mesa_get_format_name(rb->Format));
   }

   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}
