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


/*
 * Templates for the span/pixel-array write/read functions called via
 * the gl_renderbuffer's GetRow, GetValues, PutRow, PutMonoRow, PutValues
 * and PutMonoValues functions.
 *
 * Define the following macros before including this file:
 *   NAME(PREFIX)  to generate the function name
 *   FORMAT  must be either GL_RGBA, GL_RGBA8 or GL_COLOR_INDEX8_EXT
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


static void
NAME(get_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y, void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   GLchan (*dest)[4] = (GLchan (*)[4]) values;
#elif FORMAT == GL_RGBA8
   GLubyte (*dest)[4] = (GLubyte (*)[4]) values;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   GLubyte *dest = (GLubyte *) values;
#else
#error FORMAT must be set!!!!
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   for (i = 0; i < count; i++) {
      FETCH_PIXEL(dest[i], pixel);
      INC_PIXEL_PTR(pixel);
   }
}

static void
NAME(get_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[], void *values )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   GLchan (*dest)[4] = (GLchan (*)[4]) values;
#elif FORMAT == GL_RGBA8
   GLubyte (*dest)[4] = (GLubyte (*)[4]) values;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   GLubyte *dest = (GLubyte *) values;
#endif
   GLuint i;
   for (i = 0; i < count; i++) {
      INIT_PIXEL_PTR(pixel, x[i], y[i]);
      FETCH_PIXEL(dest[i], pixel);
   }
}


static void
NAME(put_row)( GLcontext *ctx, struct gl_renderbuffer *rb,
               GLuint count, GLint x, GLint y,
               const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   const GLchan (*src)[4] = (const GLchan (*)[4]) values;
#elif FORMAT == GL_RGBA8
   const GLubyte (*src)[4] = (const GLubyte (*)[4]) values;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   const GLubyte (*src)[1] = (const GLubyte (*)[1]) values;
#endif
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
}

#if (FORMAT == GL_RGBA) || (FORMAT == GL_RGBA8)
static void
NAME(put_row_rgb)( GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, GLint x, GLint y,
                   const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   const GLchan (*src)[3] = (const GLchan (*)[3]) values;
#elif FORMAT == GL_RGBA8
   const GLubyte (*src)[3] = (const GLubyte (*)[3]) values;
#else
#error bad format
#endif
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
#if FORMAT == GL_RGBA
   const GLchan *src = (const GLchan *) value;
#elif FORMAT == GL_RGBA8
   const GLubyte *src = (const GLubyte *) value;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   const GLubyte *src = (const GLubyte *) value;
#endif
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
}


static void
NAME(put_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                  GLuint count, const GLint x[], const GLint y[],
                  const void *values, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   const GLchan (*src)[4] = (const GLchan (*)[4]) values;
#elif FORMAT == GL_RGBA8
   const GLubyte (*src)[4] = (const GLubyte (*)[4]) values;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   const GLubyte (*src)[1] = (const GLubyte (*)[1]) values;
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_PIXEL(pixel, x[i], y[i], src[i]);
      }
   }
}


static void
NAME(put_mono_values)( GLcontext *ctx, struct gl_renderbuffer *rb,
                       GLuint count, const GLint x[], const GLint y[],
                       const void *value, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
#if FORMAT == GL_RGBA
   const GLchan *src = (const GLchan *) value;
#elif FORMAT == GL_RGBA8
   const GLubyte *src = (const GLubyte *) value;
#elif FORMAT == GL_COLOR_INDEX8_EXT
   const GLubyte *src = (const GLubyte *) value;
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < count; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_PIXEL(pixel, x[i], y[i], src);
      }
   }
}


#undef NAME
#undef SPAN_VARS
#undef INIT_PIXEL_PTR
#undef INC_PIXEL_PTR
#undef STORE_PIXEL
#undef STORE_PIXEL_RGB
#undef FETCH_PIXEL
#undef FORMAT
