
/*
 * Mesa 3-D graphics library
 * Version:  4.1
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
 * Implement the effect of glColorMask and glIndexMask in software.
 */


#include "glheader.h"
#include "enums.h"
#include "macros.h"

#include "s_alphabuf.h"
#include "s_context.h"
#include "s_masking.h"
#include "s_span.h"



void
_swrast_mask_rgba_span( GLcontext *ctx, const struct sw_span *span,
                     GLchan rgba[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
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
      (*swrast->Driver.ReadRGBAPixels)(ctx, n, span->array->x, span->array->y,
                                       dest, span->array->mask);
      if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
         _swrast_read_alpha_pixels(ctx, n, span->array->x, span->array->y,
                                 dest, span->array->mask);
      }
   }
   else {
      _swrast_read_rgba_span(ctx, ctx->DrawBuffer, n, span->x, span->y, dest);
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
_swrast_mask_rgba_array( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   GLchan dest[MAX_WIDTH][4];
   GLuint i;

#if CHAN_BITS == 8

   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;

   _swrast_read_rgba_span( ctx, ctx->DrawBuffer, n, x, y, dest );
   for (i = 0; i < n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }

#else

   const GLint rMask = ctx->Color.ColorMask[RCOMP];
   const GLint gMask = ctx->Color.ColorMask[GCOMP];
   const GLint bMask = ctx->Color.ColorMask[BCOMP];
   const GLint aMask = ctx->Color.ColorMask[ACOMP];

   _swrast_read_rgba_span( ctx, ctx->DrawBuffer, n, x, y, dest );
   for (i = 0; i < n; i++) {
      if (!rMask)  rgba[i][RCOMP] = dest[i][RCOMP];
      if (!gMask)  rgba[i][GCOMP] = dest[i][GCOMP];
      if (!bMask)  rgba[i][BCOMP] = dest[i][BCOMP];
      if (!aMask)  rgba[i][ACOMP] = dest[i][ACOMP];
   }

#endif
}



void
_swrast_mask_index_span( GLcontext *ctx, const struct sw_span *span,
                       GLuint index[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint msrc = ctx->Color.IndexMask;
   const GLuint mdest = ~msrc;
   GLuint fbindexes[MAX_WIDTH];
   GLuint i;

   ASSERT(span->arrayMask & SPAN_INDEX);
   ASSERT(span->end < MAX_WIDTH);

   if (span->arrayMask & SPAN_XY) {

      (*swrast->Driver.ReadCI32Pixels)(ctx, span->end, span->array->x,
                                       span->array->y, fbindexes,
                                       span->array->mask);

      for (i = 0; i < span->end; i++) {
         index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
      }
   }
   else {
      _swrast_read_index_span(ctx, ctx->DrawBuffer, span->end, span->x, span->y,
                            fbindexes );

      for (i = 0; i < span->end; i++) {
         index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
      }
   }
}



/*
 * Apply glIndexMask to a span of CI pixels.
 */
void
_swrast_mask_index_array( GLcontext *ctx,
                        GLuint n, GLint x, GLint y, GLuint index[] )
{
   GLuint i;
   GLuint fbindexes[MAX_WIDTH];
   GLuint msrc, mdest;

   _swrast_read_index_span( ctx, ctx->DrawBuffer, n, x, y, fbindexes );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}
