
/*
 * Mesa 3-D graphics library
 * Version:  5.1
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
 * Templates for the span/pixel-array write/read functions called via
 * swrast.  This is intended for memory-based framebuffers (like OSMesa).
 *
 * Define the following macros before including this file:
 *   NAME(PREFIX)  to generate the function name
 *   SPAN_VARS  to declare any local variables
 *   INIT_PIXEL_PTR(P, X, Y)  to initialize a pointer to a pixel
 *   INC_PIXEL_PTR(P)  to increment a pixel pointer by one pixel
 *   STORE_RGB_PIXEL(P, X, Y, R, G, B)  to store RGB values in  pixel P
 *   STORE_RGBA_PIXEL(P, X, Y, R, G, B, A)  to store RGBA values in pixel P
 *   FETCH_RGBA_PIXEL(R, G, B, A, P)  to fetch RGBA values from pixel P
 *
 * Note that in the above STORE_RGBx_PIXEL macros, we also pass in the (X,Y)
 * coordinates for the pixels to be stored, which enables dithering in 8-bit
 * and 15/16-bit display modes. Most undithered modes or 24/32-bit display
 * modes will simply ignore the passed in (X,Y) values.
 *
 * For color index mode:
 *   STORE_CI_PIXEL(P, CI)  to store a color index in pixel P
 *   FETCH_CI_PIXEL(CI, P)  to fetch a pixel index from pixel P
 */


#ifdef STORE_RGBA_PIXEL

static void
NAME(write_rgba_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                       CONST GLchan rgba[][4], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_RGBA_PIXEL(pixel, x+i, y, rgba[i][RCOMP], rgba[i][GCOMP],
                             rgba[i][BCOMP], rgba[i][ACOMP]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         STORE_RGBA_PIXEL(pixel, x+i, y, rgba[i][RCOMP], rgba[i][GCOMP],
                          rgba[i][BCOMP], rgba[i][ACOMP]);
         INC_PIXEL_PTR(pixel);
      }
   }
}

static void
NAME(write_rgb_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      CONST GLchan rgb[][3], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_RGB_PIXEL(pixel, x+i, y, rgb[i][RCOMP], rgb[i][GCOMP],
                            rgb[i][BCOMP]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         STORE_RGB_PIXEL(pixel, x+i, y, rgb[i][RCOMP], rgb[i][GCOMP],
                         rgb[i][BCOMP]);
         INC_PIXEL_PTR(pixel);
      }
   }
}

static void
NAME(write_monorgba_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                           const GLchan color[4], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_RGBA_PIXEL(pixel, x+i, y, color[RCOMP], color[GCOMP],
                             color[BCOMP], color[ACOMP]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         STORE_RGBA_PIXEL(pixel, x+i, y, color[RCOMP], color[GCOMP],
                          color[BCOMP], color[ACOMP]);
         INC_PIXEL_PTR(pixel);
      }
   }
}

static void
NAME(write_rgba_pixels)( const GLcontext *ctx, GLuint n,
                         const GLint x[], const GLint y[],
                         CONST GLchan rgba[][4], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_RGBA_PIXEL(pixel, x[i], y[i], rgba[i][RCOMP], rgba[i][GCOMP],
                          rgba[i][BCOMP], rgba[i][ACOMP]);
      }
   }
}

static void
NAME(write_monorgba_pixels)( const GLcontext *ctx,
                             GLuint n, const GLint x[], const GLint y[],
                             const GLchan color[4], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_RGBA_PIXEL(pixel, x[i], y[i], color[RCOMP], color[GCOMP],
                          color[BCOMP], color[ACOMP]);
      }
   }
}

static void
NAME(read_rgba_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      GLchan rgba[][4] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   for (i = 0; i < n; i++) {
      FETCH_RGBA_PIXEL(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP],
                       rgba[i][ACOMP], pixel);
      INC_PIXEL_PTR(pixel);
   }
}

static void
NAME(read_rgba_pixels)( const GLcontext *ctx,
                        GLuint n, const GLint x[], const GLint y[],
                        GLchan rgba[][4], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         FETCH_RGBA_PIXEL(rgba[i][RCOMP], rgba[i][GCOMP], rgba[i][BCOMP],
                          rgba[i][ACOMP], pixel);
      }
   }
}


#endif /* STORE_RGBA_PIXEL */



#ifdef STORE_CI_PIXEL

static void
NAME(write_index32_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLuint index[], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_CI_PIXEL(pixel, index[i]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         STORE_CI_PIXEL(pixel, index[i]);
         INC_PIXEL_PTR(pixel);
      }
   }
}


static void
NAME(write_index8_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLubyte index[], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_CI_PIXEL(pixel, index[i]);
         }
         INC_PIXEL_PTR(pixel);
      }
   }
   else {
      for (i = 0; i < n; i++) {
         STORE_CI_PIXEL(pixel, index[i]);
         INC_PIXEL_PTR(pixel);
      }
   }
}


static void
NAME(write_monoindex_span)( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            GLuint colorIndex, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   if (mask) {
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            STORE_CI_PIXEL(pixel, colorIndex);
         }
         INC_PIXEL_PTR(pixel);
      }
   }      
   else {
      for (i = 0; i < n; i++) {
         STORE_CI_PIXEL(pixel, colorIndex);
         INC_PIXEL_PTR(pixel);
      }
   }      
}


static void
NAME(write_index_pixels)( const GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          const GLuint index[], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_CI_PIXEL(pixel, index[i]);
      }
   }
}


static void
NAME(write_monoindex_pixels)( const GLcontext *ctx,
                              GLuint n, const GLint x[], const GLint y[],
                              GLuint colorIndex, const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i]) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         STORE_CI_PIXEL(pixel, colorIndex);
      }
   }
}


static void
NAME(read_index_span)( const GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLuint index[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   INIT_PIXEL_PTR(pixel, x, y);
   for (i = 0; i < n; i++) {
      FETCH_CI_PIXEL(index[i], pixel);
      INC_PIXEL_PTR(pixel);
   }
}


static void
NAME(read_index_pixels)( const GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         GLuint index[], const GLubyte mask[] )
{
#ifdef SPAN_VARS
   SPAN_VARS
#endif
   GLuint i;
   ASSERT(mask);
   for (i = 0; i < n; i++) {
      if (mask[i] ) {
         INIT_PIXEL_PTR(pixel, x[i], y[i]);
         FETCH_CI_PIXEL(index[i], pixel);
      }
   }
}

#endif /* STORE_CI_PIXEL */



#undef NAME
#undef SPAN_VARS
#undef INIT_PIXEL_PTR
#undef INC_PIXEL_PTR
#undef STORE_RGB_PIXEL
#undef STORE_RGBA_PIXEL
#undef FETCH_RGBA_PIXEL
#undef STORE_CI_PIXEL
#undef FETCH_CI_PIXEL
