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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"

#include "s_context.h"
#include "s_fog.h"


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
      f = EXPF(-d * z);
      f = CLAMP(f, 0.0F, 1.0F);
      return f;
   case GL_EXP2:
      d = ctx->Fog.Density;
      f = EXPF(-(d * d * z * z));
      f = CLAMP(f, 0.0F, 1.0F);
      return f;
   default:
      _mesa_problem(ctx, "Bad fog mode in _swrast_z_to_fogfactor");
      return 0.0; 
   }
}


/**
 * Template code for computing fog blend factor and applying it to colors.
 * \param TYPE  either GLubyte, GLushort or GLfloat.
 * \param COMPUTE_F  code to compute the fog blend factor, f.
 */
#define FOG_LOOP(TYPE, COMPUTE_F)					\
do {									\
   const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];	\
   GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];		\
   const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;\
   GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;	\
   GLuint i;								\
   for (i = 0; i < span->end; i++) {					\
      GLfloat f, oneMinusF;						\
      COMPUTE_F;							\
      f = CLAMP(f, 0.0F, 1.0F);						\
      oneMinusF = 1.0F - f;						\
      rgba[i][RCOMP] = (TYPE) (f * rgba[i][RCOMP] + oneMinusF * rFog);	\
      rgba[i][GCOMP] = (TYPE) (f * rgba[i][GCOMP] + oneMinusF * gFog);	\
      rgba[i][BCOMP] = (TYPE) (f * rgba[i][BCOMP] + oneMinusF * bFog);	\
      fogCoord += fogStep;						\
      w += wStep;							\
   }									\
} while (0)



/**
 * Apply fog to a span of RGBA pixels.
 * The fog value are either in the span->array->fog array or interpolated from
 * the fog/fogStep values.
 * They fog values are either fog coordinates (Z) or fog blend factors.
 * _PreferPixelFog should be in sync with that state!
 */
void
_swrast_fog_rgba_span( const GLcontext *ctx, SWspan *span )
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat rFog, gFog, bFog;
   const GLuint haveW = (span->interpMask & SPAN_W);

   ASSERT(swrast->_FogEnabled);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_FOG);
   ASSERT(span->arrayMask & SPAN_RGBA);

   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      rFog = ctx->Fog.Color[RCOMP] * 255.0;
      gFog = ctx->Fog.Color[GCOMP] * 255.0;
      bFog = ctx->Fog.Color[BCOMP] * 255.0;
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      rFog = ctx->Fog.Color[RCOMP] * 65535.0;
      gFog = ctx->Fog.Color[GCOMP] * 65535.0;
      bFog = ctx->Fog.Color[BCOMP] * 65535.0;
   }
   else {
      rFog = ctx->Fog.Color[RCOMP];
      gFog = ctx->Fog.Color[GCOMP];
      bFog = ctx->Fog.Color[BCOMP];
   }


   /* NOTE: if haveW is true, that means the fog start/step values are
    * perspective-corrected and we have to divide each fog coord by W.
    */

   /* we need to compute fog blend factors */
   if (swrast->_PreferPixelFog) {
      /* The span's fog values are fog coordinates, now compute blend factors
       * and blend the fragment colors with the fog color.
       */
      const GLfloat fogEnd = ctx->Fog.End;
      const GLfloat fogScale = (ctx->Fog.Start == ctx->Fog.End)
         ? 1.0F : 1.0F / (ctx->Fog.End - ctx->Fog.Start);
      const GLfloat density = -ctx->Fog.Density;
      const GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;

      switch (swrast->_FogMode) {
      case GL_LINEAR:
#define COMPUTE_F  f = (fogEnd - FABSF(fogCoord) / w) * fogScale;
         if (span->array->ChanType == GL_UNSIGNED_BYTE) {
            GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
            FOG_LOOP(GLubyte, COMPUTE_F);
         }
         else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
            GLushort (*rgba)[4] = span->array->color.sz2.rgba;
            FOG_LOOP(GLushort, COMPUTE_F);
         }
         else {
            GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
            ASSERT(span->array->ChanType == GL_FLOAT);
            FOG_LOOP(GLfloat, COMPUTE_F);
         }
#undef COMPUTE_F
         break;

      case GL_EXP:
#define COMPUTE_F  f = EXPF(density * FABSF(fogCoord) / w);
         if (span->array->ChanType == GL_UNSIGNED_BYTE) {
            GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
            FOG_LOOP(GLubyte, COMPUTE_F);
         }
         else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
            GLushort (*rgba)[4] = span->array->color.sz2.rgba;
            FOG_LOOP(GLushort, COMPUTE_F);
         }
         else {
            GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
            ASSERT(span->array->ChanType == GL_FLOAT);
            FOG_LOOP(GLfloat, COMPUTE_F);
         }
#undef COMPUTE_F
         break;

      case GL_EXP2:
#define COMPUTE_F  const GLfloat coord = fogCoord / w; \
                   GLfloat tmp = negDensitySquared * coord * coord; \
                   if (tmp < FLT_MIN_10_EXP) \
                      tmp = FLT_MIN_10_EXP; \
                   f = EXPF(tmp);
         if (span->array->ChanType == GL_UNSIGNED_BYTE) {
            GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
            FOG_LOOP(GLubyte, COMPUTE_F);
         }
         else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
            GLushort (*rgba)[4] = span->array->color.sz2.rgba;
            FOG_LOOP(GLushort, COMPUTE_F);
         }
         else {
            GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
            ASSERT(span->array->ChanType == GL_FLOAT);
            FOG_LOOP(GLfloat, COMPUTE_F);
         }
#undef COMPUTE_F
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
      if (span->array->ChanType == GL_UNSIGNED_BYTE) {
         GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
         for (i = 0; i < span->end; i++) {
            const GLfloat f = span->array->attribs[FRAG_ATTRIB_FOGC][i][0];
            const GLfloat oneMinusF = 1.0F - f;
            rgba[i][RCOMP] = (GLubyte) (f * rgba[i][RCOMP] + oneMinusF * rFog);
            rgba[i][GCOMP] = (GLubyte) (f * rgba[i][GCOMP] + oneMinusF * gFog);
            rgba[i][BCOMP] = (GLubyte) (f * rgba[i][BCOMP] + oneMinusF * bFog);
         }
      }
      else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
         GLushort (*rgba)[4] = span->array->color.sz2.rgba;
         for (i = 0; i < span->end; i++) {
            const GLfloat f = span->array->attribs[FRAG_ATTRIB_FOGC][i][0];
            const GLfloat oneMinusF = 1.0F - f;
            rgba[i][RCOMP] = (GLushort) (f * rgba[i][RCOMP] + oneMinusF * rFog);
            rgba[i][GCOMP] = (GLushort) (f * rgba[i][GCOMP] + oneMinusF * gFog);
            rgba[i][BCOMP] = (GLushort) (f * rgba[i][BCOMP] + oneMinusF * bFog);
         }
      }
      else {
         GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
         ASSERT(span->array->ChanType == GL_FLOAT);
         for (i = 0; i < span->end; i++) {
            const GLfloat f = span->array->attribs[FRAG_ATTRIB_FOGC][i][0];
            const GLfloat oneMinusF = 1.0F - f;
            rgba[i][RCOMP] = f * rgba[i][RCOMP] + oneMinusF * rFog;
            rgba[i][GCOMP] = f * rgba[i][GCOMP] + oneMinusF * gFog;
            rgba[i][BCOMP] = f * rgba[i][BCOMP] + oneMinusF * bFog;
         }
      }

   }
   else {
      /* The span's fog start/step values are blend factors.
       * They were previously computed per-vertex.
       */
#define COMPUTE_F f = fogCoord / w;
      if (span->array->ChanType == GL_UNSIGNED_BYTE) {
         GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
         FOG_LOOP(GLubyte, COMPUTE_F);
      }
      else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
         GLushort (*rgba)[4] = span->array->color.sz2.rgba;
         FOG_LOOP(GLushort, COMPUTE_F);
      }
      else {
         GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
         ASSERT(span->array->ChanType == GL_FLOAT);
         FOG_LOOP(GLfloat, COMPUTE_F);
      }
#undef COMPUTE_F
   }
}


/**
 * As above, but color index mode.
 */
void
_swrast_fog_ci_span( const GLcontext *ctx, SWspan *span )
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
            const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];
            GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];
            const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;
            GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;
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
            const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];
            GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];
            const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;
            GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;
            GLuint i;
            for (i = 0; i < span->end; i++) {
               GLfloat f = EXPF(density * fogCoord / w);
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
            const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];
            GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];
            const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;
            GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;
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
               f = EXPF(tmp);
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
         const GLfloat f = span->array->attribs[FRAG_ATTRIB_FOGC][i][0];
         index[i] = (GLuint) ((GLfloat) index[i] + (1.0F - f) * fogIndex);
      }
   }
   else {
      /* The span's fog start/step values are blend factors.
       * They were previously computed per-vertex.
       */
      const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];
      GLfloat fog = span->attrStart[FRAG_ATTRIB_FOGC][0];
      const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;
      GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;
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
