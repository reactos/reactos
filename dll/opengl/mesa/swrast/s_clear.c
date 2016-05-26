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

#include <precomp.h>

#include <main/accum.h>

/**
 * Clear an rgba color buffer with masking if needed.
 */
static void
clear_rgba_buffer(struct gl_context *ctx, struct gl_renderbuffer *rb,
                  const GLubyte colorMask[4])
{
   const GLint x = ctx->DrawBuffer->_Xmin;
   const GLint y = ctx->DrawBuffer->_Ymin;
   const GLint height = ctx->DrawBuffer->_Ymax - ctx->DrawBuffer->_Ymin;
   const GLint width  = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
   const GLuint pixelSize = _mesa_get_format_bytes(rb->Format);
   const GLboolean doMasking = (colorMask[0] == 0 ||
                                colorMask[1] == 0 ||
                                colorMask[2] == 0 ||
                                colorMask[3] == 0);
   const GLfloat (*clearColor)[4] =
      (const GLfloat (*)[4]) ctx->Color.ClearColor.f;
   GLbitfield mapMode = GL_MAP_WRITE_BIT;
   GLubyte *map;
   GLint rowStride;
   GLint i, j;

   if (doMasking) {
      /* we'll need to read buffer values too */
      mapMode |= GL_MAP_READ_BIT;
   }

   /* map dest buffer */
   ctx->Driver.MapRenderbuffer(ctx, rb, x, y, width, height,
                               mapMode, &map, &rowStride);
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClear(color)");
      return;
   }

   /* for 1, 2, 4-byte clearing */
#define SIMPLE_TYPE_CLEAR(TYPE)                                         \
   do {                                                                 \
      TYPE pixel, pixelMask;                                            \
      _mesa_pack_float_rgba_row(rb->Format, 1, clearColor, &pixel);     \
      if (doMasking) {                                                  \
         _mesa_pack_colormask(rb->Format, colorMask, &pixelMask);       \
         pixel &= pixelMask;                                            \
         pixelMask = ~pixelMask;                                        \
      }                                                                 \
      for (i = 0; i < height; i++) {                                    \
         TYPE *row = (TYPE *) map;                                      \
         if (doMasking) {                                               \
            for (j = 0; j < width; j++) {                               \
               row[j] = (row[j] & pixelMask) | pixel;                   \
            }                                                           \
         }                                                              \
         else {                                                         \
            for (j = 0; j < width; j++) {                               \
               row[j] = pixel;                                          \
            }                                                           \
         }                                                              \
         map += rowStride;                                              \
      }                                                                 \
   } while (0)


   /* for 3, 6, 8, 12, 16-byte clearing */
#define MULTI_WORD_CLEAR(TYPE, N)                                       \
   do {                                                                 \
      TYPE pixel[N], pixelMask[N];                                      \
      GLuint k;                                                         \
      _mesa_pack_float_rgba_row(rb->Format, 1, clearColor, pixel);      \
      if (doMasking) {                                                  \
         _mesa_pack_colormask(rb->Format, colorMask, pixelMask);        \
         for (k = 0; k < N; k++) {                                      \
            pixel[k] &= pixelMask[k];                                   \
            pixelMask[k] = ~pixelMask[k];                               \
         }                                                              \
      }                                                                 \
      for (i = 0; i < height; i++) {                                    \
         TYPE *row = (TYPE *) map;                                      \
         if (doMasking) {                                               \
            for (j = 0; j < width; j++) {                               \
               for (k = 0; k < N; k++) {                                \
                  row[j * N + k] =                                      \
                     (row[j * N + k] & pixelMask[k]) | pixel[k];        \
               }                                                        \
            }                                                           \
         }                                                              \
         else {                                                         \
            for (j = 0; j < width; j++) {                               \
               for (k = 0; k < N; k++) {                                \
                  row[j * N + k] = pixel[k];                            \
               }                                                        \
            }                                                           \
         }                                                              \
         map += rowStride;                                              \
      }                                                                 \
   } while(0)

   switch (pixelSize) {
   case 1:
      SIMPLE_TYPE_CLEAR(GLubyte);
      break;
   case 2:
      SIMPLE_TYPE_CLEAR(GLushort);
      break;
   case 3:
      MULTI_WORD_CLEAR(GLubyte, 3);
      break;
   case 4:
      SIMPLE_TYPE_CLEAR(GLuint);
      break;
   case 6:
      MULTI_WORD_CLEAR(GLushort, 3);
      break;
   case 8:
      MULTI_WORD_CLEAR(GLuint, 2);
      break;
   case 12:
      MULTI_WORD_CLEAR(GLuint, 3);
      break;
   case 16:
      MULTI_WORD_CLEAR(GLuint, 4);
      break;
   default:
      _mesa_problem(ctx, "bad pixel size in clear_rgba_buffer()");
   }

   /* unmap buffer */
   ctx->Driver.UnmapRenderbuffer(ctx, rb);
}


/**
 * Clear the front/back/left/right/aux color buffers.
 * This function is usually only called if the device driver can't
 * clear its own color buffers for some reason (such as with masking).
 */
static void
clear_color_buffers(struct gl_context *ctx)
{
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffer;

   clear_rgba_buffer(ctx, rb, ctx->Color.ColorMask);
}


/**
 * Called via the device driver's ctx->Driver.Clear() function if the
 * device driver can't clear one or more of the buffers itself.
 * \param buffers  bitfield of BUFFER_BIT_* values indicating which
 *                 renderbuffers are to be cleared.
 * \param all  if GL_TRUE, clear whole buffer, else clear specified region.
 */
void
_swrast_Clear(struct gl_context *ctx, GLbitfield buffers)
{
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
         BUFFER_BIT_AUX0;
      assert((buffers & (~legalBits)) == 0);
   }
#endif

   if (SWRAST_CONTEXT(ctx)->NewState)
      _swrast_validate_derived(ctx);

   if (buffers & BUFFER_BITS_COLOR) {
      clear_color_buffers(ctx);
   }

   if (buffers & BUFFER_BIT_ACCUM) {
      _mesa_clear_accum_buffer(ctx);
   }

   if (buffers & BUFFER_BIT_DEPTH) {
      _swrast_clear_depth_buffer(ctx);
   }
   if (buffers & BUFFER_BIT_STENCIL) {
      _swrast_clear_stencil_buffer(ctx);
   }
}
