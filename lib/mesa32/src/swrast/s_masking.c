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
 * Implement the effect of glColorMask and glIndexMask in software.
 */


#include "glheader.h"
#include "enums.h"
#include "macros.h"

#include "s_context.h"
#include "s_masking.h"
#include "s_span.h"



void
_swrast_mask_rgba_span(GLcontext *ctx, struct gl_renderbuffer *rb,
                       const struct sw_span *span, GLchan rgba[][4])
{
   GLchan dest[MAX_WIDTH][4];
#if CHAN_BITS == 8
   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;
#else
   const GLboolean rMask = ctx->Color.ColorMask[RCOMP];
   const GLboolean gMask = ctx->Color.ColorMask[GCOMP];
   const GLboolean bMask = ctx->Color.ColorMask[BCOMP];
   const GLboolean aMask = ctx->Color.ColorMask[ACOMP];
#endif
   const GLuint n = span->end;
   GLuint i;

   ASSERT(n < MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);

   if (span->arrayMask & SPAN_XY) {
      _swrast_get_values(ctx, rb, n, span->array->x, span->array->y,
                         dest, 4 * sizeof(GLchan));
   }
   else {
      _swrast_read_rgba_span(ctx, rb, n, span->x, span->y, dest);
   }

#if CHAN_BITS == 8
   for (i = 0; i < n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }
#else
   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }
#endif
}


/*
 * Apply glColorMask to a span of RGBA pixels.
 */
void
_swrast_mask_rgba_array(GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y, GLchan rgba[][4])
{
   GLchan dest[MAX_WIDTH][4];
   GLuint i;

#if CHAN_BITS == 8

   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;

   _swrast_read_rgba_span( ctx, rb, n, x, y, dest );
   for (i = 0; i < n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }

#else

   const GLint rMask = ctx->Color.ColorMask[RCOMP];
   const GLint gMask = ctx->Color.ColorMask[GCOMP];
   const GLint bMask = ctx->Color.ColorMask[BCOMP];
   const GLint aMask = ctx->Color.ColorMask[ACOMP];

   _swrast_read_rgba_span( ctx, rb, n, x, y, dest );
   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }

#endif
}



void
_swrast_mask_ci_span(GLcontext *ctx, struct gl_renderbuffer *rb,
                     const struct sw_span *span, GLuint index[])
{
   const GLuint srcMask = ctx->Color.IndexMask;
   const GLuint dstMask = ~srcMask;
   GLuint dest[MAX_WIDTH];
   GLuint i;

   ASSERT(span->arrayMask & SPAN_INDEX);
   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(rb->DataType == GL_UNSIGNED_INT);

   if (span->arrayMask & SPAN_XY) {
      _swrast_get_values(ctx, rb, span->end, span->array->x, span->array->y,
                         dest, sizeof(GLuint));
   }
   else {
      _swrast_read_index_span(ctx, rb, span->end, span->x, span->y, dest);
   }

   for (i = 0; i < span->end; i++) {
      index[i] = (index[i] & srcMask) | (dest[i] & dstMask);
   }
}


/*
 * Apply glIndexMask to an array of CI pixels.
 */
void
_swrast_mask_ci_array(GLcontext *ctx, struct gl_renderbuffer *rb,
                      GLuint n, GLint x, GLint y, GLuint index[])
{
   const GLuint srcMask = ctx->Color.IndexMask;
   const GLuint dstMask = ~srcMask;
   GLuint dest[MAX_WIDTH];
   GLuint i;

   _swrast_read_index_span(ctx, rb, n, x, y, dest);

   for (i=0;i<n;i++) {
      index[i] = (index[i] & srcMask) | (dest[i] & dstMask);
   }
}
