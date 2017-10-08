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

#include <precomp.h>

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
blend_noop(struct gl_context *ctx, GLuint n, const GLubyte mask[],
           GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLint bytes;

   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_ZERO);
   ASSERT(ctx->Color.DstRGB == GL_ONE);
   (void) ctx;

   /* just memcpy */
   if (chanType == GL_UNSIGNED_BYTE)
      bytes = 4 * n * sizeof(GLubyte);
   else if (chanType == GL_UNSIGNED_SHORT)
      bytes = 4 * n * sizeof(GLushort);
   else
      bytes = 4 * n * sizeof(GLfloat);

   memcpy(src, dst, bytes);
}


/**
 * Special case for glBlendFunc(GL_ONE, GL_ZERO)
 * Any chanType ok.
 */
static void _BLENDAPI
blend_replace(struct gl_context *ctx, GLuint n, const GLubyte mask[],
              GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_ONE);
   ASSERT(ctx->Color.DstRGB == GL_ZERO);
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
blend_transparency_ubyte(struct gl_context *ctx, GLuint n, const GLubyte mask[],
                         GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLubyte (*rgba)[4] = (GLubyte (*)[4]) src;
   const GLubyte (*dest)[4] = (const GLubyte (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.SrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.DstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.DstA == GL_ONE_MINUS_SRC_ALPHA);
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
blend_transparency_ushort(struct gl_context *ctx, GLuint n, const GLubyte mask[],
                          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLushort (*rgba)[4] = (GLushort (*)[4]) src;
   const GLushort (*dest)[4] = (const GLushort (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.SrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.DstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.DstA == GL_ONE_MINUS_SRC_ALPHA);
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
blend_transparency_float(struct gl_context *ctx, GLuint n, const GLubyte mask[],
                         GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLfloat (*rgba)[4] = (GLfloat (*)[4]) src;
   const GLfloat (*dest)[4] = (const GLfloat (*)[4]) dst;
   GLuint i;

   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_SRC_ALPHA);
   ASSERT(ctx->Color.SrcA == GL_SRC_ALPHA);
   ASSERT(ctx->Color.DstRGB == GL_ONE_MINUS_SRC_ALPHA);
   ASSERT(ctx->Color.DstA == GL_ONE_MINUS_SRC_ALPHA);
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
blend_add(struct gl_context *ctx, GLuint n, const GLubyte mask[],
          GLvoid *src, const GLvoid *dst, GLenum chanType)
{
   GLuint i;

   ASSERT(ctx->Color.EquationRGB == GL_FUNC_ADD);
   ASSERT(ctx->Color.EquationA == GL_FUNC_ADD);
   ASSERT(ctx->Color.SrcRGB == GL_ONE);
   ASSERT(ctx->Color.DstRGB == GL_ONE);
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
 * Modulate:  result = src * dest
 * Any chanType ok.
 */
static void _BLENDAPI
blend_modulate(struct gl_context *ctx, GLuint n, const GLubyte mask[],
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
blend_general_float(struct gl_context *ctx, GLuint n, const GLubyte mask[],
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
         switch (ctx->Color.SrcFactor) {
            case GL_ZERO:
               sR = sG = sB = sA = 0.0F;
               break;
            case GL_ONE:
               sR = sG = sB = sA = 1.0F;
               break;
            case GL_DST_COLOR:
               sR = Rd;
               sG = Gd;
               sB = Bd;
               sA = Ad;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               sR = 1.0F - Rd;
               sG = 1.0F - Gd;
               sB = 1.0F - Bd;
               sA = 1.0f - Ad;
               break;
            case GL_SRC_ALPHA:
               sR = sG = sB = sA = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               sR = sG = sB = sA = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               sR = sG = sB = sA = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               sR = sG = sB = sA = 1.0F - Ad;
               break;
            case GL_SRC_ALPHA_SATURATE:
               if (As < 1.0F - Ad) {
                  sR = sG = sB = sA = As;
               }
               else {
                  sR = sG = sB = sA = 1.0F - Ad;
               }
               break;
            case GL_SRC_COLOR:
               sR = Rs;
               sG = Gs;
               sB = Bs;
               sA = As;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               sR = 1.0F - Rs;
               sG = 1.0F - Gs;
               sB = 1.0F - Bs;
               sA = 1.0F - As;
               break;
            default:
               /* this should never happen */
               _mesa_problem(ctx, "Bad blend source RGB factor in blend_general_float");
               return;
         }

         /* Dest factor */
         switch (ctx->Color.DstFactor) {
            case GL_ZERO:
               dR = dG = dB = dA = 0.0F;
               break;
            case GL_ONE:
               dR = dG = dB = dA = 1.0F;
               break;
            case GL_SRC_COLOR:
               dR = Rs;
               dG = Gs;
               dB = Bs;
               dA = As;
               break;
            case GL_ONE_MINUS_SRC_COLOR:
               dR = 1.0F - Rs;
               dG = 1.0F - Gs;
               dB = 1.0F - Bs;
               dA = 1.0F - As;
               break;
            case GL_SRC_ALPHA:
               dR = dG = dB = dA = As;
               break;
            case GL_ONE_MINUS_SRC_ALPHA:
               dR = dG = dB = dA = 1.0F - As;
               break;
            case GL_DST_ALPHA:
               dR = dG = dB = dA = Ad;
               break;
            case GL_ONE_MINUS_DST_ALPHA:
               dR = dG = dB = dA = 1.0F - Ad;
               break;
            case GL_DST_COLOR:
               dR = Rd;
               dG = Gd;
               dB = Bd;
               dA = Ad;
               break;
            case GL_ONE_MINUS_DST_COLOR:
               dR = 1.0F - Rd;
               dG = 1.0F - Gd;
               dB = 1.0F - Bd;
               dA = 1.0F - Ad;
               break;
            default:
               /* this should never happen */
               dR = dG = dB = dA = 0.0F;
               _mesa_problem(ctx, "Bad blend dest factor in blend_general_float");
               return;
         }

         /* compute the blended RGBA */
         r = Rs * sR + Rd * dR;
         g = Gs * sG + Gd * dG;
         b = Bs * sB + Bd * dB;
         a = As * sA + Ad * dA;

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
blend_general(struct gl_context *ctx, GLuint n, const GLubyte mask[],
              void *src, const void *dst, GLenum chanType)
{
   GLfloat (*rgbaF)[4], (*destF)[4];

   rgbaF = (GLfloat (*)[4]) malloc(4 * n * sizeof(GLfloat));
   destF = (GLfloat (*)[4]) malloc(4 * n * sizeof(GLfloat));
   if (!rgbaF || !destF) {
      free(rgbaF);
      free(destF);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "blending");
      return;
   }

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
         if (mask[i])
	   _mesa_unclamped_float_rgba_to_ubyte(rgba[i], rgbaF[i]);
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

   free(rgbaF);
   free(destF);
}



/**
 * Analyze current blending parameters to pick fastest blending function.
 * Result: the ctx->Color.BlendFunc pointer is updated.
 */
void
_swrast_choose_blend_func(struct gl_context *ctx, GLenum chanType)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLenum srcFactor = ctx->Color.SrcFactor;
   const GLenum dstFactor = ctx->Color.DstFactor;

   if (srcFactor == GL_SRC_ALPHA && dstFactor == GL_ONE_MINUS_SRC_ALPHA) {
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
   else if (srcFactor == GL_ONE && dstFactor == GL_ONE) {
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_add;
      }
      else
#endif
         swrast->BlendFunc = blend_add;
   }
   else if ((srcFactor == GL_ZERO && dstFactor == GL_SRC_COLOR)
	    || (srcFactor == GL_DST_COLOR && dstFactor == GL_ZERO)) {
#if defined(USE_MMX_ASM)
      if (cpu_has_mmx && chanType == GL_UNSIGNED_BYTE) {
         swrast->BlendFunc = _mesa_mmx_blend_modulate;
      }
      else
#endif
         swrast->BlendFunc = blend_modulate;
   }
   else if (srcFactor == GL_ZERO && dstFactor == GL_ONE) {
      swrast->BlendFunc = blend_noop;
   }
   else if (srcFactor == GL_ONE && dstFactor == GL_ZERO) {
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
_swrast_blend_span(struct gl_context *ctx, struct gl_renderbuffer *rb, SWspan *span)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   void *rbPixels;

   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(span->arrayMask & SPAN_RGBA);
   ASSERT(!ctx->Color.ColorLogicOpEnabled);

   rbPixels = _swrast_get_dest_rgba(ctx, rb, span);

   swrast->BlendFunc(ctx, span->end, span->array->mask,
                     span->array->rgba, rbPixels, span->array->ChanType);
}
