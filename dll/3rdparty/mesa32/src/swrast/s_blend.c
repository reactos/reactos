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


/**
 * \file swrast/s_blend.c
 * \brief software blending.
 * \author Brian Paul
 *
 * Only a few blend modes have been optimized (min, max, transparency, add)
 * more optimized cases can easily be added if needed.
 * Celestia uses glBlendFunc(GL_SRC_ALPHA, GL_ONE), for example.
 */



#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"

#include "s_blend.h"
#include "s_context.h"
#include "s_span.h"


#if defined(USE_MMX_ASM)
#include "x86/mmx.h"
#include "x86/common_x86_asm.h"
#define _BLENDAPI _ASMAPI
#else
#define _BLENDAPI
#endif


/**
 * Integer divide by 255
 * Declare "int divtemp" before using.
 * This satisfies Glean and should be reasonably fast.
 * Contributed by Nathan Hand.
 */
#define DIV255(X)  (divtemp = (X), ((divtemp << 8) + divtemp + 256) >> 16)



/**
 * Special case for glBlendFunc(GL_ZERO, GL_ONE).
 * No-op means the framebuffer values remain unchanged.
 * Any chanType ok.
 */
static void _BLENDAPI
blend_noop(GLcontext *ctx, GLuint n, const GLubyte mask[],
           GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLint bytes;

   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_ZERO);
   ASSERT(ctx->Color.BlendDstRGB == GL_ONE);
   (void) ctx;

   /* just memcpy */
   if (chanType == GL_UNSIGNED_BYTE)
      bytes = 4 * n * sizeof(GLubyte);
   else if (chanType == GL_UNSIGNED_SHORT)
      bytes = 4 * n * sizeof(GLushort);
   else
      bytes = 4 * n * sizeof(GLfloat);

   _mesa_memcpy(src, dst, bytes);
}


/**
 * Special case for glBlendFunc(GL_ONE, GL_ZERO)
 * Any chanType ok.
 */
static void _BLENDAPI
blend_replace(GLcontext *ctx, GLuint n, const GLubyte mask[],
              GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_ONE);
   ASSERT(ctx->Color.BlendDstRGB == GL_ZERO);
   (void) ctx;
   (void) n;
   (void) mask;
   (void) src;
   (void) dst;
}


/**
 * Common transparency blending mode:
 * glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA).
 */
static void _BLENDAPI
blend_transparency_ubyte(GLcontext *ctx, GLuint n, const GLubyte mask[],
                         GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
   const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendSrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstA == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(chanType == GL_UNSIGNED_BYTE);

   (void) ctx;

   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLint t = rgba[i][ACOMP];  /* t is in [0, 255] */
         if (t == 0) {
            /* 0% alpha */
            COPY_4UBV(rgba[i], dest[i]);
         }
         else if (t != 255) {
	    GLint divtemp;
            const GLint r = DIV255((rgba[i][RCOMP] - dest[i][RCOMP]) * t) + dest[i][RCOMP];
            const GLint g = DIV255((rgba[i][GCOMP] - dest[i][GCOMP]) * t) + dest[i][GCOMP];
            const GLint b = DIV255((rgba[i][BCOMP] - dest[i][BCOMP]) * t) + dest[i][BCOMP];
            const GLint a = DIV255((rgba[i][ACOMP] - dest[i][ACOMP]) * t) + dest[i][ACOMP]; 
            ASSERT(r <= 255);
            ASSERT(g <= 255);
            ASSERT(b <= 255);
            ASSERT(a <= 255);
            rgba[i][RCOMP] = (GLubyte) r;
            rgba[i][GCOMP] = (GLubyte) g;
            rgba[i][BCOMP] = (GLubyte) b;
            rgba[i][ACOMP] = (GLubyte) a;
         }
      }
   }
}


static void _BLENDAPI
blend_transparency_ushort(GLcontext *ctx, GLuint n, const GLubyte mask[],
                          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLushort (*rgba)[4] = (GLushort (*)[4]) src;
   const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendSrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstA == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(chanType == GL_UNSIGNED_SHORT);

   (void) ctx;

   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLint t = rgba[i][ACOMP];
         if (t == 0) {
            /* 0% alpha */
            COPY_4V(rgba[i], dest[i]);
         }
         else if (t != 65535) {
            const GLfloat tt = (GLfloat) t / 65535.0F;
            GLushort r = (GLushort) ((rgba[i][RCOMP] - dest[i][RCOMP]) * tt + dest[i][RCOMP]);
            GLushort g = (GLushort) ((rgba[i][GCOMP] - dest[i][GCOMP]) * tt + dest[i][GCOMP]);
            GLushort b = (GLushort) ((rgba[i][BCOMP] - dest[i][BCOMP]) * tt + dest[i][BCOMP]);
            GLushort a = (GLushort) ((rgba[i][ACOMP] - dest[i][ACOMP]) * tt + dest[i][ACOMP]);
            ASSIGN_4V(rgba[i], r, g, b, a);
         }
      }
   }
}


static void _BLENDAPI
blend_transparency_float(GLcontext *ctx, GLuint n, const GLubyte mask[],
                         GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
   const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendSrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.BlendDstA == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(chanType == GL_FLOAT);

   (void) ctx;

   for (i = 0; i < n; i++) {
      if (mask[i]) {
         const GLfloat t = rgba[i][ACOMP];  /* t in [0, 1] */
         if (t == 0.0F) {
            /* 0% alpha */
            COPY_4V(rgba[i], dest[i]);
         }
         else if (t != 1.0F) {
            GLfloat r = (rgba[i][RCOMP] - dest[i][RCOMP]) * t + dest[i][RCOMP];
            GLfloat g = (rgba[i][GCOMP] - dest[i][GCOMP]) * t + dest[i][GCOMP];
            GLfloat b = (rgba[i][BCOMP] - dest[i][BCOMP]) * t + dest[i][BCOMP];
            GLfloat a = (rgba[i][ACOMP] - dest[i][ACOMP]) * t + dest[i][ACOMP];
            ASSIGN_4V(rgba[i], r, g, b, a);
         }
      }
   }
}



/**
 * Add src and dest: glBlendFunc(GL_ONE, GL_ONE).
 * Any chanType ok.
 */
static void _BLENDAPI
blend_add(GLcontext *ctx, GLuint n, const GLubyte mask[],
          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLuint i;

   ASSERT(ctx->Color.BlendEquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendEquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.BlendSrcRGB == GL_ONE);
   ASSERT(ctx->Color.BlendDstRGB == GL_ONE);
   (void) ctx;

   if (chanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
      const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLint r = rgba[i][RCOMP] + dest[i][RCOMP];
            GLint g = rgba[i][GCOMP] + dest[i][GCOMP];
            GLint b = rgba[i][BCOMP] + dest[i][BCOMP];
            GLint a = rgba[i][ACOMP] + dest[i][ACOMP];
            rgba[i][RCOMP] = (GLubyte) MIN2( r, 255 );
            rgba[i][GCOMP] = (GLubyte) MIN2( g, 255 );
            rgba[i][BCOMP] = (GLubyte) MIN2( b, 255 );
            rgba[i][ACOMP] = (GLubyte) MIN2( a, 255 );
         }
      }
   }
   else if (chanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = (GLushort (*)[4]) src;
      const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLint r = rgba[i][RCOMP] + dest[i][RCOMP];
            GLint g = rgba[i][GCOMP] + dest[i][GCOMP];
            GLint b = rgba[i][BCOMP] + dest[i][BCOMP];
            GLint a = rgba[i][ACOMP] + dest[i][ACOMP];
            rgba[i][RCOMP] = (GLshort) MIN2( r, 255 );
            rgba[i][GCOMP] = (GLshort) MIN2( g, 255 );
            rgba[i][BCOMP] = (GLshort) MIN2( b, 255 );
            rgba[i][ACOMP] = (GLshort) MIN2( a, 255 );
         }
      }
   }
   else {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
      const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
      ASSERT(chanType == GL_FLOAT);
      for (i=0;i<n;i++) {
         if (mask[i]) {
            /* don't RGB clamp to max */
            rgba[i][RCOMP] += dest[i][RCOMP];
            rgba[i][GCOMP] += dest[i][GCOMP];
            rgba[i][BCOMP] += dest[i][BCOMP];
            rgba[i][ACOMP] += dest[i][ACOMP];
         }
      }
   }
}



/**
 * Blend min function.
 * Any chanType ok.
 */
static void _BLENDAPI
blend_min(GLcontext *ctx, GLuint n, const GLubyte mask[],
          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLuint i;
   ASSERT(ctx->Color.BlendEquationRGB == GL_MIN);
   ASSERT(ctx->Color.BlendEquationA == GL_MIN);
   (void) ctx;

   if (chanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
      const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MIN2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MIN2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MIN2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MIN2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
   else if (chanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = (GLushort (*)[4]) src;
      const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MIN2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MIN2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MIN2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MIN2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
   else {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
      const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
      ASSERT(chanType == GL_FLOAT);
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MIN2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MIN2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MIN2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MIN2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
}


/**
 * Blend max function.
 * Any chanType ok.
 */
static void _BLENDAPI
blend_max(GLcontext *ctx, GLuint n, const GLubyte mask[],
          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLuint i;
   ASSERT(ctx->Color.BlendEquationRGB == GL_MAX);
   ASSERT(ctx->Color.BlendEquationA == GL_MAX);
   (void) ctx;

   if (chanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
      const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MAX2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MAX2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MAX2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MAX2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
   else if (chanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = (GLushort (*)[4]) src;
      const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MAX2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MAX2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MAX2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MAX2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
   else {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
      const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
      ASSERT(chanType == GL_FLOAT);
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = MAX2( rgba[i][RCOMP], dest[i][RCOMP] );
            rgba[i][GCOMP] = MAX2( rgba[i][GCOMP], dest[i][GCOMP] );
            rgba[i][BCOMP] = MAX2( rgba[i][BCOMP], dest[i][BCOMP] );
            rgba[i][ACOMP] = MAX2( rgba[i][ACOMP], dest[i][ACOMP] );
         }
      }
   }
}



/**
 * Modulate:  result = src * dest
 * Any chanType ok.
 */
static void _BLENDAPI
blend_modulate(GLcontext *ctx, GLuint n, const GLubyte mask[],
               GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLuint i;
   (void) ctx;

   if (chanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
      const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
	    GLint divtemp;
            rgba[i][RCOMP] = DIV255(rgba[i][RCOMP] * dest[i][RCOMP]);
            rgba[i][GCOMP] = DIV255(rgba[i][GCOMP] * dest[i][GCOMP]);
            rgba[i][BCOMP] = DIV255(rgba[i][BCOMP] * dest[i][BCOMP]);
            rgba[i][ACOMP] = DIV255(rgba[i][ACOMP] * dest[i][ACOMP]);
         }
      }
   }
   else if (chanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = (GLushort (*)[4]) src;
      const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = (rgba[i][RCOMP] * dest[i][RCOMP] + 65535) >> 16;
            rgba[i][GCOMP] = (rgba[i][GCOMP] * dest[i][GCOMP] + 65535) >> 16;
            rgba[i][BCOMP] = (rgba[i][BCOMP] * dest[i][BCOMP] + 65535) >> 16;
            rgba[i][ACOMP] = (rgba[i][ACOMP] * dest[i][ACOMP] + 65535) >> 16;
         }
      }
   }
   else {
      GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
      const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
      ASSERT(chanType == GL_FLOAT);
      for (i=0;i<n;i++) {
         if (mask[i]) {
            rgba[i][RCOMP] = rgba[i][RCOMP] * dest[i][RCOMP];
            rgba[i][GCOMP] = rgba[i][GCOMP] * dest[i][GCOMP];
            rgba[i][BCOMP] = rgba[i][BCOMP] * dest[i][BCOMP];
            rgba[i][ACOMP] = rgba[i][ACOMP] * dest[i][ACOMP];
         }
      }
   }
}


/**
 * Do any blending operation, using floating point.
 * \param n  number of pixels
 * \param mask  fragment writemask array
 * \param rgba  array of incoming (and modified) pixels
 * \param dest  array of pixels from the dest color buffer
 */
static void
blend_general_float(GLcontext *ctx, GLuint n, const GLubyte mask[],
                    GLfloat rgba[][4], GLfloat dest[][4],
                    GLenum chanType)
{
   GLuint i;

   for (i = 0; i < n; i++) {
      if (mask[i]) {
         /* Incoming/source Color */
         const GLfloat Rs = rgba[i][RCOMP];
         const GLfloat Gs = rgba[i][GCOMP];
         const GLfloat Bs = rgba[i][BCOMP];
         const GLfloat As = rgba[i][ACOMP];

         /* Frame buffer/dest color */
         const GLfloat Rd = dest[i][RCOMP];
         const GLfloat Gd = dest[i][GCOMP];
         const GLfloat Bd = dest[i][BCOMP];
         const GLfloat Ad = dest[i][ACOMP];

         GLfloat sR, sG, sB, sA;  /* Source factor */
         GLfloat dR, dG, dB, dA;  /* Dest factor */
         GLfloat r, g, b, a;      /* result color */

         /* XXX for the case of constant blend terms we could init
          * the sX and dX variables just once before the loop.
          */

         /* Source RGB factor */
         switch (ctx->Color.BlendSrcRGB) {
            case GL_ZERO:
               sR = sG = sB = 0.0F;
               break;
            case GL_ONE:
               sR = sG = sB = 1.0F;
               break;
            case GL_DST_COLOR:
               sR = Rd;
               sG = Gd;
               sB = Bd;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               sR = 1.0F - Rd;
               sG = 1.0F - Gd;
               sB = 1.0F - Bd;
               break;
            case GL_SRC_ALPHA:
               sR = sG = sB = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               sR = sG = sB = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               sR = sG = sB = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               sR = sG = sB = 1.0F - Ad;
               break;
            case GL_SRC_ALPHA_SATURATE:
               if (As < 1.0F - Ad) {
                  sR = sG = sB = As;
               }
               else {
                  sR = sG = sB = 1.0F - Ad;
               }
               break;
            case GL_CONSTANT_COLOR:
               sR = ctx->Color.BlendColor[0];
               sG = ctx->Color.BlendColor[1];
               sB = ctx->Color.BlendColor[2];
               break;
            case GL_ONE_MINUS_CONSTANT_COLOR:
               sR = 1.0F - ctx->Color.BlendColor[0];
               sG = 1.0F - ctx->Color.BlendColor[1];
               sB = 1.0F - ctx->Color.BlendColor[2];
               break;
            case GL_CONSTANT_ALPHA:
               sR = sG = sB = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_ALPHA:
               sR = sG = sB = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_SRC_COLOR:
               sR = Rs;
               sG = Gs;
               sB = Bs;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               sR = 1.0F - Rs;
               sG = 1.0F - Gs;
               sB = 1.0F - Bs;
               break;
            default:
               /* this should never happen */
               _mesa_problem(ctx, "Bad blend source RGB factor in blend_general_float");
               return;
         }

         /* Source Alpha factor */
         switch (ctx->Color.BlendSrcA) {
            case GL_ZERO:
               sA = 0.0F;
               break;
            case GL_ONE:
               sA = 1.0F;
               break;
            case GL_DST_COLOR:
               sA = Ad;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               sA = 1.0F - Ad;
               break;
            case GL_SRC_ALPHA:
               sA = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               sA = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               sA = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               sA = 1.0F - Ad;
               break;
            case GL_SRC_ALPHA_SATURATE:
               sA = 1.0;
               break;
            case GL_CONSTANT_COLOR:
               sA = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_COLOR:
               sA = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_CONSTANT_ALPHA:
               sA = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_ALPHA:
               sA = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_SRC_COLOR:
               sA = As;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               sA = 1.0F - As;
               break;
            default:
               /* this should never happen */
               sA = 0.0F;
               _mesa_problem(ctx, "Bad blend source A factor in blend_general_float");
               return;
         }

         /* Dest RGB factor */
         switch (ctx->Color.BlendDstRGB) {
            case GL_ZERO:
               dR = dG = dB = 0.0F;
               break;
            case GL_ONE:
               dR = dG = dB = 1.0F;
               break;
            case GL_SRC_COLOR:
               dR = Rs;
               dG = Gs;
               dB = Bs;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               dR = 1.0F - Rs;
               dG = 1.0F - Gs;
               dB = 1.0F - Bs;
               break;
            case GL_SRC_ALPHA:
               dR = dG = dB = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               dR = dG = dB = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               dR = dG = dB = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               dR = dG = dB = 1.0F - Ad;
               break;
            case GL_CONSTANT_COLOR:
               dR = ctx->Color.BlendColor[0];
               dG = ctx->Color.BlendColor[1];
               dB = ctx->Color.BlendColor[2];
               break;
            case GL_ONE_MINUS_CONSTANT_COLOR:
               dR = 1.0F - ctx->Color.BlendColor[0];
               dG = 1.0F - ctx->Color.BlendColor[1];
               dB = 1.0F - ctx->Color.BlendColor[2];
               break;
            case GL_CONSTANT_ALPHA:
               dR = dG = dB = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_ALPHA:
               dR = dG = dB = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_DST_COLOR:
               dR = Rd;
               dG = Gd;
               dB = Bd;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               dR = 1.0F - Rd;
               dG = 1.0F - Gd;
               dB = 1.0F - Bd;
               break;
            default:
               /* this should never happen */
               dR = dG = dB = 0.0F;
               _mesa_problem(ctx, "Bad blend dest RGB factor in blend_general_float");
               return;
         }

         /* Dest Alpha factor */
         switch (ctx->Color.BlendDstA) {
            case GL_ZERO:
               dA = 0.0F;
               break;
            case GL_ONE:
               dA = 1.0F;
               break;
            case GL_SRC_COLOR:
               dA = As;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               dA = 1.0F - As;
               break;
            case GL_SRC_ALPHA:
               dA = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               dA = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               dA = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               dA = 1.0F - Ad;
               break;
            case GL_CONSTANT_COLOR:
               dA = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_COLOR:
               dA = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_CONSTANT_ALPHA:
               dA = ctx->Color.BlendColor[3];
               break;
            case GL_ONE_MINUS_CONSTANT_ALPHA:
               dA = 1.0F - ctx->Color.BlendColor[3];
               break;
            case GL_DST_COLOR:
               dA = Ad;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               dA = 1.0F - Ad;
               break;
            default:
               /* this should never happen */
               dA = 0.0F;
               _mesa_problem(ctx, "Bad blend dest A factor in blend_general_float");
               return;
         }

         /* compute the blended RGB */
         switch (ctx->Color.BlendEquationRGB) {
         case GL_FUNC_ADD:
            r = Rs * sR + Rd * dR;
            g = Gs * sG + Gd * dG;
            b = Bs * sB + Bd * dB;
            a = As * sA + Ad * dA;
            break;
         case GL_FUNC_SUBTRACT:
            r = Rs * sR - Rd * dR;
            g = Gs * sG - Gd * dG;
            b = Bs * sB - Bd * dB;
            a = As * sA - Ad * dA;
            break;
         case GL_FUNC_REVERSE_SUBTRACT:
            r = Rd * dR - Rs * sR;
            g = Gd * dG - Gs * sG;
            b = Bd * dB - Bs * sB;
            a = Ad * dA - As * sA;
            break;
         case GL_MIN:
	    r = MIN2( Rd, Rs );
	    g = MIN2( Gd, Gs );
	    b = MIN2( Bd, Bs );
            break;
         case GL_MAX:
	    r = MAX2( Rd, Rs );
	    g = MAX2( Gd, Gs );
	    b = MAX2( Bd, Bs );
            break;
	 default:
            /* should never get here */
            r = g = b = 0.0F;  /* silence uninitialized var warning */
            _mesa_problem(ctx, "unexpected BlendEquation in blend_general()");
            return;
         }

         /* compute the blended alpha */
         switch (ctx->Color.BlendEquationA) {
         case GL_FUNC_ADD:
            a = As * sA + Ad * dA;
            break;
         case GL_FUNC_SUBTRACT:
            a = As * sA - Ad * dA;
            break;
         case GL_FUNC_REVERSE_SUBTRACT:
            a = Ad * dA - As * sA;
            break;
         case GL_MIN:
	    a = MIN2( Ad, As );
            break;
         case GL_MAX:
	    a = MAX2( Ad, As );
            break;
         default:
            /* should never get here */
            a = 0.0F;  /* silence uninitialized var warning */
            _mesa_problem(ctx, "unexpected BlendEquation in blend_general()");
            return;
         }

         /* final clamping */
#if 0
         rgba[i][RCOMP] = MAX2( r, 0.0F );
         rgba[i][GCOMP] = MAX2( g, 0.0F );
         rgba[i][BCOMP] = MAX2( b, 0.0F );
         rgba[i][ACOMP] = CLAMP( a, 0.0F, 1.0F );
#else
         ASSIGN_4V(rgba[i], r, g, b, a);
#endif
      }
   }
}


/**
 * Do any blending operation, any chanType.
 */
static void
blend_general(GLcontext *ctx, GLuint n, const GLubyte mask[],
              void *src, const void *dst, GLenum chanType)
{
   GLfloat rgbaF[MAX_WIDTH][4], destF[MAX_WIDTH][4];

   if (chanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
      const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
      GLuint i;
      /* convert ubytes to floats */
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            rgbaF[i][RCOMP] = UBYTE_TO_FLOAT(rgba[i][RCOMP]);
            rgbaF[i][GCOMP] = UBYTE_TO_FLOAT(rgba[i][GCOMP]);
            rgbaF[i][BCOMP] = UBYTE_TO_FLOAT(rgba[i][BCOMP]);
            rgbaF[i][ACOMP] = UBYTE_TO_FLOAT(rgba[i][ACOMP]);
            destF[i][RCOMP] = UBYTE_TO_FLOAT(dest[i][RCOMP]);
            destF[i][GCOMP] = UBYTE_TO_FLOAT(dest[i][GCOMP]);
            destF[i][BCOMP] = UBYTE_TO_FLOAT(dest[i][BCOMP]);
            destF[i][ACOMP] = UBYTE_TO_FLOAT(dest[i][ACOMP]);
         }
      }
      /* do blend */
      blend_general_float(ctx, n, mask, rgbaF, destF, chanType);
      /* convert back to ubytes */
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            UNCLAMPED_FLOAT_TO_UBYTE(rgba[i][RCOMP], rgbaF[i][RCOMP]);
            UNCLAMPED_FLOAT_TO_UBYTE(rgba[i][GCOMP], rgbaF[i][GCOMP]);
            UNCLAMPED_FLOAT_TO_UBYTE(rgba[i][BCOMP], rgbaF[i][BCOMP]);
            UNCLAMPED_FLOAT_TO_UBYTE(rgba[i][ACOMP], rgbaF[i][ACOMP]);
         }
      }
   }
   else if (chanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = (GLushort (*)[4]) src;
      const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
      GLuint i;
      /* convert ushorts to floats */
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            rgbaF[i][RCOMP] = USHORT_TO_FLOAT(rgba[i][RCOMP]);
            rgbaF[i][GCOMP] = USHORT_TO_FLOAT(rgba[i][GCOMP]);
            rgbaF[i][BCOMP] = USHORT_TO_FLOAT(rgba[i][BCOMP]);
            rgbaF[i][ACOMP] = USHORT_TO_FLOAT(rgba[i][ACOMP]);
            destF[i][RCOMP] = USHORT_TO_FLOAT(dest[i][RCOMP]);
            destF[i][GCOMP] = USHORT_TO_FLOAT(dest[i][GCOMP]);
            destF[i][BCOMP] = USHORT_TO_FLOAT(dest[i][BCOMP]);
            destF[i][ACOMP] = USHORT_TO_FLOAT(dest[i][ACOMP]);
         }
      }
      /* do blend */
      blend_general_float(ctx, n, mask, rgbaF, destF, chanType);
      /* convert back to ushorts */
      for (i = 0; i < n; i++) {
         if (mask[i]) {
            UNCLAMPED_FLOAT_TO_USHORT(rgba[i][RCOMP], rgbaF[i][RCOMP]);
            UNCLAMPED_FLOAT_TO_USHORT(rgba[i][GCOMP], rgbaF[i][GCOMP]);
            UNCLAMPED_FLOAT_TO_USHORT(rgba[i][BCOMP], rgbaF[i][BCOMP]);
            UNCLAMPED_FLOAT_TO_USHORT(rgba[i][ACOMP], rgbaF[i][ACOMP]);
         }
      }
   }
   else {
      blend_general_float(ctx, n, mask, (GLfloat (*)[4]) src,
                          (GLfloat (*)[4]) dst, chanType);
   }
}



/**
 * Analyze current blending parameters to pick fastest blending function.
 * Result: the ctx->Color.BlendFunc pointer is updated.
 */
void
_swrast_choose_blend_func(GLcontext *ctx, GLenum chanType)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLenum eq = ctx->Color.BlendEquationRGB;
   const GLenum srcRGB = ctx->Color.BlendSrcRGB;
   const GLenum dstRGB = ctx->Color.BlendDstRGB;
   const GLenum srcA = ctx->Color.BlendSrcA;
   const GLenum dstA = ctx->Color.BlendDstA;

   if (ctx->Color.BlendEquationRGB != ctx->Color.BlendEquationA) {
      swrast->BlendFunc = blend_general;
   }
   else if (eq == GL_MIN) {
      /* Note: GL_MIN ignores the blending weight factors */
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_min;
      }
      else
#endif
         swrast->BlendFunc = blend_min;
   }
   else if (eq == GL_MAX) {
      /* Note: GL_MAX ignores the blending weight factors */
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_max;
      }
      else
#endif
         swrast->BlendFunc = blend_max;
   }
   else if (srcRGB != srcA || dstRGB != dstA) {
      swrast->BlendFunc = blend_general;
   }
   else if (eq == GL_FUNC_ADD && srcRGB == GL_SRC_ALPHA
            && dstRGB == GL_ONE_MINUS_SRC_ALPHA) {
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_transparency;
      }
      else
#endif
      {
         if (chanType == GL_UNSIGNED_BYTE)
            swrast->BlendFunc = blend_transparency_ubyte;
         else if (chanType == GL_UNSIGNED_SHORT)
            swrast->BlendFunc = blend_transparency_ushort;
         else
            swrast->BlendFunc = blend_transparency_float;
      }
   }
   else if (eq == GL_FUNC_ADD && srcRGB == GL_ONE && dstRGB == GL_ONE) {
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_add;
      }
      else
#endif
         swrast->BlendFunc = blend_add;
   }
   else if (((eq == GL_FUNC_ADD || eq == GL_FUNC_REVERSE_SUBTRACT)
	     && (srcRGB == GL_ZERO && dstRGB == GL_SRC_COLOR))
	    ||
	    ((eq == GL_FUNC_ADD || eq == GL_FUNC_SUBTRACT)
	     && (srcRGB == GL_DST_COLOR && dstRGB == GL_ZERO))) {
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_modulate;
      }
      else
#endif
         swrast->BlendFunc = blend_modulate;
   }
   else if (eq == GL_FUNC_ADD && srcRGB == GL_ZERO && dstRGB == GL_ONE) {
      swrast->BlendFunc = blend_noop;
   }
   else if (eq == GL_FUNC_ADD && srcRGB == GL_ONE && dstRGB == GL_ZERO) {
      swrast->BlendFunc = blend_replace;
   }
   else {
      swrast->BlendFunc = blend_general;
   }
}



/**
 * Apply the blending operator to a span of pixels.
 * We can handle horizontal runs of pixels (spans) or arrays of x/y
 * pixel coordinates.
 */
void
_swrast_blend_span(GLcontext *ctx, struct gl_renderbuffer *rb, SWspan *span)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   void *rbPixels;

   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);
   ASSERT(rb->DataType == span->array->ChanType);
   ASSERT(!ctx->Color._LogicOpEnabled);

   rbPixels = _swrast_get_dest_rgba(ctx, rb, span);

   swrast->BlendFunc(ctx, span->end, span->array->mask,
                     span->array->rgba, rbPixels, span->array->ChanType);
}
