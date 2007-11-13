/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
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


/*
 * Templates for the span/pixel-array write/read functions called via
 * the gl_renderbuffer's GetRow, GetValues, PutRow, PutMonoRow, PutValues
 * and PutMonoValues functions.
 *
 * Define the following macros before including this file:
 *   NAME(BASE)  to generate the function name (i.e. add prefix or suffix)
 *   RB_TYPE  the renderbuffer DataType
 *   CI_MODE  if set, color index mode, else RGBA
 *   SPAN_VARS  to declare any local variables
 *   INIT_PIXEL_PTR(P, X, Y)  to initialize a pointer to a pixel
 *   INC_PIXEL_PTR(P)  to increment a pixel pointer by one pixel
 *   STORE_PIXEL(DST, X, Y, VALUE)  to store pixel values in buffer
 *   FETCH_PIXEL(DST, SRC)  to fetch pixel values from buffer
 *
 * Note that in the STORE_PIXEL macros, we also pass in the (X,Y) coordinates
 * for the pixels to be stored.  This is useful when dithering and probably
 * ignored otherwise.
 */

#include "macros.h"


#ifdef CI_MODE
#define RB_COMPONENTS 1
#elif !defined(RB_COMPONENTS)
#define RB_COMPONENTS 4
#endif


static void
NAME(get_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y, void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#ifdef CI_MODE
   RB_TYPE *dest = (RB_TYPE *) values;
#else
   RB_TYPE (*dest)[RB_COMPONENTS] = (RB_TYPE (*)[RB_COMPONENTS]) values;
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   for (i = 0; i < count; i++) {
      FETCH_PIXEL(dest[i], pixel);
      INC_PIXEL_PTR(pixel);
   }
   (void) rb;
}


static void
NAME(get_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[], void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#ifdef CI_MODE
   RB_TYPE *dest = (RB_TYPE *) values;
#else
   RB_TYPE (*dest)[RB_COMPONENTS] = (RB_TYPE (*)[RB_COMPONENTS]) values;
#endif
   GLuint i;
   for (i = 0; i < count; i++) {
      INIT_PIXEL_PTR(pixel, x[i], y[i]);
      FETCH_PIXEL(dest[i], pixel);
   }
   (void) rb;
}


static void
NAME(put_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y,
               const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[RB_COMPONENTS] = (const RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            STORE_PIXEL(pixel, x + i, y, src[i]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < count; i++) {
         STORE_PIXEL(pixel, x + i, y, src[i]);
         INC_PIXEL_PTR(pixel);
      }
   }
   (void) rb;
}


#if !defined(CI_MODE)
static void
NAME(put_row_rgb)( GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, GLint x, GLint y,
                   const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[3] = (const RB_TYPE (*)[3]) values;
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   for (i = 0; i < count; i++) {
      if (!mask || mask[i]) {
#ifdef STORE_PIXEL_RGB
         STORE_PIXEL_RGB(pixel, x + i, y, src[i]);
#else
         STORE_PIXEL(pixel, x + i, y, src[i]);
#endif
      }
      INC_PIXEL_PTR(pixel);
   }
   (void) rb;
}
#endif


static void
NAME(put_mono_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
                    GLuint count, GLint x, GLint y,
                    const void *value, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE *src = (const RB_TYPE *) value;
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            STORE_PIXEL(pixel, x + i, y, src);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < count; i++) {
         STORE_PIXEL(pixel, x + i, y, src);
         INC_PIXEL_PTR(pixel);
      }
   }
   (void) rb;
}


static void
NAME(put_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[],
                  const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE (*src)[RB_COMPONENTS] = (const RB_TYPE (*)[RB_COMPONENTS]) values;
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_PIXEL(pixel, x[i], y[i], src[i]);
      }
   }
   (void) rb;
}


static void
NAME(put_mono_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   const RB_TYPE *src = (const RB_TYPE *) value;
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_PIXEL(pixel, x[i], y[i], src);
      }
   }
   (void) rb;
}


#undef NAME
#undef RB_TYPE
#undef RB_COMPONENTS
#undef CI_MODE
#undef SPAN_VARS
#undef INIT_PIXEL_PTR
#undef INC_PIXEL_PTR
#undef STORE_PIXEL
#undef STORE_PIXEL_RGB
#undef FETCH_PIXEL
