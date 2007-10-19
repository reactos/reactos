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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"

#include "s_context.h"
#include "s_fog.h"
#include "s_span.h"


/**
 * Used to convert current raster distance to a fog factor in [0,1].
 */
GLfloat
_swrast_z_to_fogfactor(GLcontext *ctx, GLfloat z)
{
   GLfloat d, f;

   switch (ctx->Fog.Mode) {
   case GL_LINEAR:
      if (ctx->Fog.Start == ctx->Fog.End)
         d = 1.0F;
      else
         d = 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      f = (ctx->Fog.End - z) * d;
      return CLAMP(f, 0.0F, 1.0F);
   case GL_EXP:
      d = ctx->Fog.Density;
      f = (GLfloat) exp(-d * z);
      f = CLAMP(f, 0.0F, 1.0F);
      return f;
   case GL_EXP2:
      d = ctx->Fog.Density;
      f = (GLfloat) exp(-(d * d * z * z));
      f = CLAMP(f, 0.0F, 1.0F);
      return f;
   default:
      _mesa_problem(ctx, "Bad fog mode in _swrast_z_to_fogfactor");
      return 0.0;
   }
}


/**
 * Apply fog to a span of RGBA pixels.
 * The fog value are either in the span->array->fog array or interpolated from
 * the fog/fogStep values.
 * They fog values are either fog coordinates (Z) or fog blend factors.
 * _PreferPixelFog should be in sync with that state!
 */
void
_swrast_fog_rgba_span( const GLcontext *ctx, struct sw_span *span )
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLchan rFog = swrast->_FogColor[RCOMP];
   const GLchan gFog = swrast->_FogColor[GCOMP];
   const GLchan bFog = swrast->_FogColor[BCOMP];
   const GLuint haveW = (span->interpMask & SPAN_W);
   GLchan (*rgba)[4] = (GLchan (*)[4]) span->array->rgba;

   ASSERT(swrast->_FogEnabled);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_FOG);
   ASSERT(span->arrayMask & SPAN_RGBA);

   /* NOTE: if haveW is true, that means the fog start/step values are
    * perspective-corrected and we have to divide each fog coord by W.
    */

   /* we need to compute fog blend factors */
   if (swrast->_PreferPixelFog) {
      /* The span's fog values are fog coordinates, now compute blend factors
       * and blend the fragment colors with the fog color.
       */
      switch (swrast->_FogMode) {
      case GL_LINEAR:
         {
            const GLfloat fogEnd = ctx->Fog.End;
            const GLfloat fogScale = (ctx->Fog.Start == ctx->Fog.End)
               ? 1.0F : 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               GLfloat f, oneMinusF;
               f = (fogEnd - FABSF(fogCoord) / w) * fogScale;
               f = CLAMP(f, 0.0F, 1.0F);
               oneMinusF = 1.0F - f;
               rgba[i][RCOMP] = (GLchan) (f * rgba[i][RCOMP] + oneMinusF * rFog);
               rgba[i][GCOMP] = (GLchan) (f * rgba[i][GCOMP] + oneMinusF * gFog);
               rgba[i][BCOMP] = (GLchan) (f * rgba[i][BCOMP] + oneMinusF * bFog);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      case GL_EXP:
         {
            const GLfloat density = -ctx->Fog.Density;
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               GLfloat f, oneMinusF;
               f = (GLfloat) exp(density * FABSF(fogCoord) / w);
               f = CLAMP(f, 0.0F, 1.0F);
               oneMinusF = 1.0F - f;
               rgba[i][RCOMP] = (GLchan) (f * rgba[i][RCOMP] + oneMinusF * rFog);
               rgba[i][GCOMP] = (GLchan) (f * rgba[i][GCOMP] + oneMinusF * gFog);
               rgba[i][BCOMP] = (GLchan) (f * rgba[i][BCOMP] + oneMinusF * bFog);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      case GL_EXP2:
         {
            const GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               const GLfloat coord = fogCoord / w;
               GLfloat tmp = negDensitySquared * coord * coord;
               GLfloat f, oneMinusF;
#if defined(__alpha__) || defined(__alpha)
               /* XXX this underflow check may be needed for other systems*/
               if (tmp < FLT_MIN_10_EXP)
                  tmp = FLT_MIN_10_EXP;
#endif
               f = (GLfloat) exp(tmp);
               f = CLAMP(f, 0.0F, 1.0F);
               oneMinusF = 1.0F - f;
               rgba[i][RCOMP] = (GLchan) (f * rgba[i][RCOMP] + oneMinusF * rFog);
               rgba[i][GCOMP] = (GLchan) (f * rgba[i][GCOMP] + oneMinusF * gFog);
               rgba[i][BCOMP] = (GLchan) (f * rgba[i][BCOMP] + oneMinusF * bFog);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      default:
         _mesa_problem(ctx, "Bad fog mode in _swrast_fog_rgba_span");
         return;
      }
   }
   else if (span->arrayMask & SPAN_FOG) {
      /* The span's fog array values are blend factors.
       * They were previously computed per-vertex.
       */
      GLuint i;
      for (i = 0; i < span->end; i++) {
         const GLfloat f = span->array->fog[i];
         const GLfloat oneMinusF = 1.0F - f;
         rgba[i][RCOMP] = (GLchan) (f * rgba[i][RCOMP] + oneMinusF * rFog);
         rgba[i][GCOMP] = (GLchan) (f * rgba[i][GCOMP] + oneMinusF * gFog);
         rgba[i][BCOMP] = (GLchan) (f * rgba[i][BCOMP] + oneMinusF * bFog);
      }
   }
   else {
      /* The span's fog start/step values are blend factors.
       * They were previously computed per-vertex.
       */
      const GLfloat fogStep = span->fogStep;
      GLfloat fog = span->fog;
      const GLfloat wStep = haveW ? span->dwdx : 0.0F;
      GLfloat w = haveW ? span->w : 1.0F;
      GLuint i;
      ASSERT(span->interpMask & SPAN_FOG);
      for (i = 0; i < span->end; i++) {
         const GLfloat fact = fog / w;
         const GLfloat oneMinusF = 1.0F - fact;
         rgba[i][RCOMP] = (GLchan) (fact * rgba[i][RCOMP] + oneMinusF * rFog);
         rgba[i][GCOMP] = (GLchan) (fact * rgba[i][GCOMP] + oneMinusF * gFog);
         rgba[i][BCOMP] = (GLchan) (fact * rgba[i][BCOMP] + oneMinusF * bFog);
         fog += fogStep;
         w += wStep;
      }
   }
}


/**
 * As above, but color index mode.
 */
void
_swrast_fog_ci_span( const GLcontext *ctx, struct sw_span *span )
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint haveW = (span->interpMask & SPAN_W);
   const GLuint fogIndex = (GLuint) ctx->Fog.Index;
   GLuint *index = span->array->index;

   ASSERT(swrast->_FogEnabled);
   ASSERT(span->arrayMask & SPAN_INDEX);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_FOG);

   /* we need to compute fog blend factors */
   if (swrast->_PreferPixelFog) {
      /* The span's fog values are fog coordinates, now compute blend factors
       * and blend the fragment colors with the fog color.
       */
      switch (ctx->Fog.Mode) {
      case GL_LINEAR:
         {
            const GLfloat fogEnd = ctx->Fog.End;
            const GLfloat fogScale = (ctx->Fog.Start == ctx->Fog.End)
               ? 1.0F : 1.0F / (ctx->Fog.End - ctx->Fog.Start);
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               GLfloat f = (fogEnd - fogCoord / w) * fogScale;
               f = CLAMP(f, 0.0F, 1.0F);
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      case GL_EXP:
         {
            const GLfloat density = -ctx->Fog.Density;
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               GLfloat f = (GLfloat) exp(density * fogCoord / w);
               f = CLAMP(f, 0.0F, 1.0F);
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      case GL_EXP2:
         {
            const GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            const GLfloat fogStep = span->fogStep;
            GLfloat fogCoord = span->fog;
            const GLfloat wStep = haveW ? span->dwdx : 0.0F;
            GLfloat w = haveW ? span->w : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               const GLfloat coord = fogCoord / w;
               GLfloat tmp = negDensitySquared * coord * coord;
               GLfloat f;
#if defined(__alpha__) || defined(__alpha)
               /* XXX this underflow check may be needed for other systems*/
               if (tmp < FLT_MIN_10_EXP)
                  tmp = FLT_MIN_10_EXP;
#endif
               f = (GLfloat) exp(tmp);
               f = CLAMP(f, 0.0F, 1.0F);
               index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
               fogCoord += fogStep;
               w += wStep;
            }
         }
         break;
      default:
         _mesa_problem(ctx, "Bad fog mode in _swrast_fog_ci_span");
         return;
      }
   }
   else if (span->arrayMask & SPAN_FOG) {
      /* The span's fog array values are blend factors.
       * They were previously computed per-vertex.
       */
      GLuint i;
      for (i = 0; i < span->end; i++) {
         const GLfloat f = span->array->fog[i];
         index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
      }
   }
   else {
      /* The span's fog start/step values are blend factors.
       * They were previously computed per-vertex.
       */
      const GLfloat fogStep = span->fogStep;
      GLfloat fog = span->fog;
      const GLfloat wStep = haveW ? span->dwdx : 0.0F;
      GLfloat w = haveW ? span->w : 1.0F;
      GLuint i;
      ASSERT(span->interpMask & SPAN_FOG);
      for (i = 0; i < span->end; i++) {
         const GLfloat f = fog / w;
         index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
         fog += fogStep;
         w += wStep;
      }
   }
}
