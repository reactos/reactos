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

#include <precomp.h>

/**
 * Used to convert current raster distance to a fog factor in [0,1].
 */
GLfloat
_swrast_z_to_fogfactor(struct gl_context *ctx, GLfloat z)
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


#define LINEAR_FOG(f, coord)  f = (fogEnd - coord) * fogScale

#define EXP_FOG(f, coord)  f = EXPF(density * coord)

#define EXP2_FOG(f, coord)				\
do {							\
   GLfloat tmp = negDensitySquared * coord * coord;	\
   if (tmp < FLT_MIN_10_EXP)				\
      tmp = FLT_MIN_10_EXP;				\
   f = EXPF(tmp);					\
 } while(0)


#define BLEND_FOG(f, coord)  f = coord



/**
 * Template code for computing fog blend factor and applying it to colors.
 * \param TYPE  either GLubyte, GLushort or GLfloat.
 * \param COMPUTE_F  code to compute the fog blend factor, f.
 */
#define FOG_LOOP(TYPE, FOG_FUNC)						\
if (span->arrayAttribs & FRAG_BIT_FOGC) {					\
   GLuint i;									\
   for (i = 0; i < span->end; i++) {						\
      const GLfloat fogCoord = span->array->attribs[FRAG_ATTRIB_FOGC][i][0];	\
      const GLfloat c = FABSF(fogCoord);					\
      GLfloat f, oneMinusF;							\
      FOG_FUNC(f, c);								\
      f = CLAMP(f, 0.0F, 1.0F);							\
      oneMinusF = 1.0F - f;							\
      rgba[i][RCOMP] = (TYPE) (f * rgba[i][RCOMP] + oneMinusF * rFog);		\
      rgba[i][GCOMP] = (TYPE) (f * rgba[i][GCOMP] + oneMinusF * gFog);		\
      rgba[i][BCOMP] = (TYPE) (f * rgba[i][BCOMP] + oneMinusF * bFog);		\
   }										\
}										\
else {										\
   const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];		\
   GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];			\
   const GLfloat wStep = span->attrStepX[FRAG_ATTRIB_WPOS][3];			\
   GLfloat w = span->attrStart[FRAG_ATTRIB_WPOS][3];				\
   GLuint i;									\
   for (i = 0; i < span->end; i++) {						\
      const GLfloat c = FABSF(fogCoord) / w;					\
      GLfloat f, oneMinusF;							\
      FOG_FUNC(f, c);								\
      f = CLAMP(f, 0.0F, 1.0F);							\
      oneMinusF = 1.0F - f;							\
      rgba[i][RCOMP] = (TYPE) (f * rgba[i][RCOMP] + oneMinusF * rFog);		\
      rgba[i][GCOMP] = (TYPE) (f * rgba[i][GCOMP] + oneMinusF * gFog);		\
      rgba[i][BCOMP] = (TYPE) (f * rgba[i][BCOMP] + oneMinusF * bFog);		\
      fogCoord += fogStep;							\
      w += wStep;								\
   }										\
}

/**
 * Apply fog to a span of RGBA pixels.
 * The fog value are either in the span->array->fog array or interpolated from
 * the fog/fogStep values.
 * They fog values are either fog coordinates (Z) or fog blend factors.
 * _PreferPixelFog should be in sync with that state!
 */
void
_swrast_fog_rgba_span( const struct gl_context *ctx, SWspan *span )
{
   const SWcontext *swrast = CONST_SWRAST_CONTEXT(ctx);
   GLfloat rFog, gFog, bFog;

   ASSERT(swrast->_FogEnabled);
   ASSERT(span->arrayMask & SPAN_RGBA);

   /* compute (scaled) fog color */
   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      rFog = ctx->Fog.Color[RCOMP] * 255.0F;
      gFog = ctx->Fog.Color[GCOMP] * 255.0F;
      bFog = ctx->Fog.Color[BCOMP] * 255.0F;
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      rFog = ctx->Fog.Color[RCOMP] * 65535.0F;
      gFog = ctx->Fog.Color[GCOMP] * 65535.0F;
      bFog = ctx->Fog.Color[BCOMP] * 65535.0F;
   }
   else {
      rFog = ctx->Fog.Color[RCOMP];
      gFog = ctx->Fog.Color[GCOMP];
      bFog = ctx->Fog.Color[BCOMP];
   }

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
            if (span->array->ChanType == GL_UNSIGNED_BYTE) {
               GLubyte (*rgba)[4] = span->array->rgba8;
               FOG_LOOP(GLubyte, LINEAR_FOG);
            }
            else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
               GLushort (*rgba)[4] = span->array->rgba16;
               FOG_LOOP(GLushort, LINEAR_FOG);
            }
            else {
               GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL];
               ASSERT(span->array->ChanType == GL_FLOAT);
               FOG_LOOP(GLfloat, LINEAR_FOG);
            }
         }
         break;

      case GL_EXP:
         {
            const GLfloat density = -ctx->Fog.Density;
            if (span->array->ChanType == GL_UNSIGNED_BYTE) {
               GLubyte (*rgba)[4] = span->array->rgba8;
               FOG_LOOP(GLubyte, EXP_FOG);
            }
            else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
               GLushort (*rgba)[4] = span->array->rgba16;
               FOG_LOOP(GLushort, EXP_FOG);
            }
            else {
               GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL];
               ASSERT(span->array->ChanType == GL_FLOAT);
               FOG_LOOP(GLfloat, EXP_FOG);
            }
         }
         break;

      case GL_EXP2:
         {
            const GLfloat negDensitySquared = -ctx->Fog.Density * ctx->Fog.Density;
            if (span->array->ChanType == GL_UNSIGNED_BYTE) {
               GLubyte (*rgba)[4] = span->array->rgba8;
               FOG_LOOP(GLubyte, EXP2_FOG);
            }
            else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
               GLushort (*rgba)[4] = span->array->rgba16;
               FOG_LOOP(GLushort, EXP2_FOG);
            }
            else {
               GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL];
               ASSERT(span->array->ChanType == GL_FLOAT);
               FOG_LOOP(GLfloat, EXP2_FOG);
            }
         }
         break;

      default:
         _mesa_problem(ctx, "Bad fog mode in _swrast_fog_rgba_span");
         return;
      }
   }
   else {
      /* The span's fog start/step/array values are blend factors in [0,1].
       * They were previously computed per-vertex.
       */
      if (span->array->ChanType == GL_UNSIGNED_BYTE) {
         GLubyte (*rgba)[4] = span->array->rgba8;
         FOG_LOOP(GLubyte, BLEND_FOG);
      }
      else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
         GLushort (*rgba)[4] = span->array->rgba16;
         FOG_LOOP(GLushort, BLEND_FOG);
      }
      else {
         GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL];
         ASSERT(span->array->ChanType == GL_FLOAT);
         FOG_LOOP(GLfloat, BLEND_FOG);
      }
   }
}
