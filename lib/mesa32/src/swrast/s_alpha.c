
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

/**
 * \file swrast/s_alpha.c
 * \brief Functions to apply alpha test.
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"

#include "s_alpha.h"
#include "s_context.h"


/**
 * \fn GLint _swrast_alpha_test( const GLcontext *ctx, struct sw_span *span )
 * \brief Apply the alpha test to a span of pixels.
 * \return
 *      - "0" = all pixels in the span failed the alpha test.
 *      - "1" = one or more pixels passed the alpha test.
 */
GLint
_swrast_alpha_test( const GLcontext *ctx, struct sw_span *span )
{
   const GLchan (*rgba)[4] = (const GLchan (*)[4]) span->array->rgba;
   GLchan ref;
   const GLuint n = span->end;
   GLubyte *mask = span->array->mask;
   GLuint i;

   CLAMPED_FLOAT_TO_CHAN(ref, ctx->Color.AlphaRef);

   if (span->arrayMask & SPAN_RGBA) {
      /* Use the array values */
      switch (ctx->Color.AlphaFunc) {
         case GL_LESS:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] < ref);
            break;
         case GL_LEQUAL:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] <= ref);
            break;
         case GL_GEQUAL:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] >= ref);
            break;
         case GL_GREATER:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] > ref);
            break;
         case GL_NOTEQUAL:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] != ref);
            break;
         case GL_EQUAL:
            for (i = 0; i < n; i++)
               mask[i] &= (rgba[i][ACOMP] == ref);
            break;
         case GL_ALWAYS:
            /* do nothing */
            return 1;
         case GL_NEVER:
            /* caller should check for zero! */
            span->writeAll = GL_FALSE;
            return 0;
         default:
            _mesa_problem( ctx, "Invalid alpha test in _swrast_alpha_test" );
            return 0;
      }
   }
   else {
      /* Use the interpolation values */
#if CHAN_TYPE == GL_FLOAT
      const GLfloat alphaStep = span->alphaStep;
      GLfloat alpha = span->alpha;
      ASSERT(span->interpMask & SPAN_RGBA);
      switch (ctx->Color.AlphaFunc) {
         case GL_LESS:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha < ref);
               alpha += alphaStep;
            }
            break;
         case GL_LEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha <= ref);
               alpha += alphaStep;
            }
            break;
         case GL_GEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha >= ref);
               alpha += alphaStep;
            }
            break;
         case GL_GREATER:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha > ref);
               alpha += alphaStep;
            }
            break;
         case GL_NOTEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha != ref);
               alpha += alphaStep;
            }
            break;
         case GL_EQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (alpha == ref);
               alpha += alphaStep;
            }
            break;
         case GL_ALWAYS:
            /* do nothing */
            return 1;
         case GL_NEVER:
            /* caller should check for zero! */
            span->writeAll = GL_FALSE;
            return 0;
         default:
            _mesa_problem( ctx, "Invalid alpha test in gl_alpha_test" );
            return 0;
      }
#else
      /* 8 or 16-bit channel interpolation */
      const GLfixed alphaStep = span->alphaStep;
      GLfixed alpha = span->alpha;
      ASSERT(span->interpMask & SPAN_RGBA);
      switch (ctx->Color.AlphaFunc) {
         case GL_LESS:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) < ref);
               alpha += alphaStep;
            }
            break;
         case GL_LEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) <= ref);
               alpha += alphaStep;
            }
            break;
         case GL_GEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) >= ref);
               alpha += alphaStep;
            }
            break;
         case GL_GREATER:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) > ref);
               alpha += alphaStep;
            }
            break;
         case GL_NOTEQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) != ref);
               alpha += alphaStep;
            }
            break;
         case GL_EQUAL:
            for (i = 0; i < n; i++) {
               mask[i] &= (FixedToChan(alpha) == ref);
               alpha += alphaStep;
            }
            break;
         case GL_ALWAYS:
            /* do nothing */
            return 1;
         case GL_NEVER:
            /* caller should check for zero! */
            span->writeAll = GL_FALSE;
            return 0;
         default:
            _mesa_problem( ctx, "Invalid alpha test in gl_alpha_test" );
            return 0;
      }
#endif /* CHAN_TYPE */
   }

#if 0
   /* XXXX This causes conformance failures!!!! */
   while ((span->start <= span->end)  &&
          (mask[span->start] == 0))
     span->start ++;

   while ((span->end >= span->start)  &&
          (mask[span->end] == 0))
     span->end --;
#endif

   span->writeAll = GL_FALSE;

   if (span->start >= span->end)
     return 0;
   else
     return 1;
}
