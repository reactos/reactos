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

/*
 * Init the mask[] array to implement a line stipple.
 */
static void
compute_stipple_mask( struct gl_context *ctx, GLuint len, GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;

   for (i = 0; i < len; i++) {
      GLuint bit = (swrast->StippleCounter / ctx->Line.StippleFactor) & 0xf;
      if ((1 << bit) & ctx->Line.StipplePattern) {
         mask[i] = GL_TRUE;
      }
      else {
         mask[i] = GL_FALSE;
      }
      swrast->StippleCounter++;
   }
}


/*
 * To draw a wide line we can simply redraw the span N times, side by side.
 */
static void
draw_wide_line( struct gl_context *ctx, SWspan *span, GLboolean xMajor )
{
   const GLint width = (GLint) CLAMP(ctx->Line.Width,
                                     ctx->Const.MinLineWidth,
                                     ctx->Const.MaxLineWidth);
   GLint start;

   ASSERT(span->end < MAX_WIDTH);

   if (width & 1)
      start = width / 2;
   else
      start = width / 2 - 1;

   if (xMajor) {
      GLint *y = span->array->y;
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               y[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               y[i]++;
         }
	 _swrast_write_rgba_span(ctx, span);
      }
   }
   else {
      GLint *x = span->array->x;
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               x[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               x[i]++;
         }
	 _swrast_write_rgba_span(ctx, span);
      }
   }
}



/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/

/* Simple RGBA index line (no stipple, width=1, no Z, no fog, no tex)*/
#define NAME simple_no_z_rgba_line
#define INTERP_RGBA
#define RENDER_SPAN(span) _swrast_write_rgba_span(ctx, &span);
#include "s_linetemp.h"


/* Z, fog, wide, stipple RGBA line */
#define NAME rgba_line
#define INTERP_RGBA
#define INTERP_Z
#define RENDER_SPAN(span)					\
   if (ctx->Line.StippleFlag) {					\
      span.arrayMask |= SPAN_MASK;				\
      compute_stipple_mask(ctx, span.end, span.array->mask);	\
   }								\
   if (ctx->Line.Width > 1.0) {					\
      draw_wide_line(ctx, &span, (GLboolean)(dx > dy));		\
   }								\
   else {							\
      _swrast_write_rgba_span(ctx, &span);			\
   }
#include "s_linetemp.h"


/* General-purpose line (any/all features). */
#define NAME general_line
#define INTERP_RGBA
#define INTERP_Z
#define INTERP_ATTRIBS
#define RENDER_SPAN(span)					\
   if (ctx->Line.StippleFlag) {					\
      span.arrayMask |= SPAN_MASK;				\
      compute_stipple_mask(ctx, span.end, span.array->mask);	\
   }								\
   if (ctx->Line.Width > 1.0) {					\
      draw_wide_line(ctx, &span, (GLboolean)(dx > dy));		\
   }								\
   else {							\
      _swrast_write_rgba_span(ctx, &span);			\
   }
#include "s_linetemp.h"



#ifdef DEBUG

/* record the current line function name */
static const char *lineFuncName = NULL;

#define USE(lineFunc)                   \
do {                                    \
    lineFuncName = #lineFunc;           \
    /*printf("%s\n", lineFuncName);*/   \
    swrast->Line = lineFunc;            \
} while (0)

#else

#define USE(lineFunc)  swrast->Line = lineFunc

#endif



/**
 * Determine which line drawing function to use given the current
 * rendering context.
 *
 * Please update the summary flag _SWRAST_NEW_LINE if you add or remove
 * tests to this code.
 */
void
_swrast_choose_line( struct gl_context *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Line.SmoothFlag) {
         /* antialiased lines */
         _swrast_choose_aa_line_function(ctx);
         ASSERT(swrast->Line);
      }
      else if (ctx->Texture._EnabledCoord
               || swrast->_FogEnabled) {
         USE(general_line);
      }
      else if (ctx->Depth.Test
               || ctx->Line.Width != 1.0
               || ctx->Line.StippleFlag) {
         /* no texture, but Z, fog, width>1, stipple, etc. */
#if CHAN_BITS == 32
         USE(general_line);
#else
         USE(rgba_line);
#endif
      }
      else {
         ASSERT(!ctx->Depth.Test);
         ASSERT(ctx->Line.Width == 1.0);
         /* simple lines */
         USE(simple_no_z_rgba_line);
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      USE(_swrast_feedback_line);
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      USE(_swrast_select_line);
   }
}
