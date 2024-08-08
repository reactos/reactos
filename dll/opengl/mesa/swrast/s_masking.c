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

/*
 * Implement the effect of glColorMask and glIndexMask in software.
 */

#include <precomp.h>

/**
 * Apply the color mask to a span of rgba values.
 */
void
_swrast_mask_rgba_span(struct gl_context *ctx, struct gl_renderbuffer *rb,
                       SWspan *span)
{
   const GLuint n = span->end;
   void *rbPixels;

   ASSERT(n < MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);

   rbPixels = _swrast_get_dest_rgba(ctx, rb, span);

   /*
    * Do component masking.
    * Note that we're not using span->array->mask[] here.  We could...
    */
   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      /* treat 4xGLubyte as 1xGLuint */
      const GLuint srcMask = *((GLuint *) ctx->Color.ColorMask);
      const GLuint dstMask = ~srcMask;
      const GLuint *dst = (const GLuint *) rbPixels;
      GLuint *src = (GLuint *) span->array->rgba8;
      GLuint i;
      for (i = 0; i < n; i++) {
         src[i] = (src[i] & srcMask) | (dst[i] & dstMask);
      }
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      /* 2-byte components */
      /* XXX try to use 64-bit arithmetic someday */
      const GLushort rMask = ctx->Color.ColorMask[RCOMP] ? 0xffff : 0x0;
      const GLushort gMask = ctx->Color.ColorMask[GCOMP] ? 0xffff : 0x0;
      const GLushort bMask = ctx->Color.ColorMask[BCOMP] ? 0xffff : 0x0;
      const GLushort aMask = ctx->Color.ColorMask[ACOMP] ? 0xffff : 0x0;
      const GLushort (*dst)[4] = (const GLushort (*)[4]) rbPixels;
      GLushort (*src)[4] = span->array->rgba16;
      GLuint i;
      for (i = 0; i < n; i++) {
         src[i][RCOMP] = (src[i][RCOMP] & rMask) | (dst[i][RCOMP] & ~rMask);
         src[i][GCOMP] = (src[i][GCOMP] & gMask) | (dst[i][GCOMP] & ~gMask);
         src[i][BCOMP] = (src[i][BCOMP] & bMask) | (dst[i][BCOMP] & ~bMask);
         src[i][ACOMP] = (src[i][ACOMP] & aMask) | (dst[i][ACOMP] & ~aMask);
      }
   }
   else {
      /* 4-byte components */
      const GLuint rMask = ctx->Color.ColorMask[RCOMP] ? ~0x0 : 0x0;
      const GLuint gMask = ctx->Color.ColorMask[GCOMP] ? ~0x0 : 0x0;
      const GLuint bMask = ctx->Color.ColorMask[BCOMP] ? ~0x0 : 0x0;
      const GLuint aMask = ctx->Color.ColorMask[ACOMP] ? ~0x0 : 0x0;
      const GLuint (*dst)[4] = (const GLuint (*)[4]) rbPixels;
      GLuint (*src)[4] = (GLuint (*)[4]) span->array->attribs[FRAG_ATTRIB_COL];
      GLuint i;
      for (i = 0; i < n; i++) {
         src[i][RCOMP] = (src[i][RCOMP] & rMask) | (dst[i][RCOMP] & ~rMask);
         src[i][GCOMP] = (src[i][GCOMP] & gMask) | (dst[i][GCOMP] & ~gMask);
         src[i][BCOMP] = (src[i][BCOMP] & bMask) | (dst[i][BCOMP] & ~bMask);
         src[i][ACOMP] = (src[i][ACOMP] & aMask) | (dst[i][ACOMP] & ~aMask);
      }
   }
}
