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


/**
 * \file swrast/s_span.c
 * \brief Span processing functions used by all rasterization functions.
 * This is where all the per-fragment tests are performed
 * \author Brian Paul
 */

#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/image.h"

#include "s_atifragshader.h"
#include "s_alpha.h"
#include "s_blend.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_fog.h"
#include "s_logic.h"
#include "s_masking.h"
#include "s_fragprog.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texcombine.h"


/**
 * Set default fragment attributes for the span using the
 * current raster values.  Used prior to glDraw/CopyPixels
 * and glBitmap.
 */
void
_swrast_span_default_attribs(GLcontext *ctx, SWspan *span)
{
   /* Z*/
   {
      const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;
      if (ctx->DrawBuffer->Visual.depthBits <= 16)
         span->z = FloatToFixed(ctx->Current.RasterPos[2] * depthMax + 0.5F);
      else {
         GLfloat tmpf = ctx->Current.RasterPos[2] * depthMax; 
         tmpf = MIN2(tmpf, depthMax);
         span->z = (GLint)tmpf;
      }
      span->zStep = 0;
      span->interpMask |= SPAN_Z;
   }

   /* W (for perspective correction) */
   span->attrStart[FRAG_ATTRIB_WPOS][3] = 1.0;
   span->attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0;
   span->attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0;

   /* primary color, or color index */
   if (ctx->Visual.rgbMode) {
      GLchan r, g, b, a;
      UNCLAMPED_FLOAT_TO_CHAN(r, ctx->Current.RasterColor[0]);
      UNCLAMPED_FLOAT_TO_CHAN(g, ctx->Current.RasterColor[1]);
      UNCLAMPED_FLOAT_TO_CHAN(b, ctx->Current.RasterColor[2]);
      UNCLAMPED_FLOAT_TO_CHAN(a, ctx->Current.RasterColor[3]);
#if CHAN_TYPE == GL_FLOAT
      span->red = r;
      span->green = g;
      span->blue = b;
      span->alpha = a;
#else
      span->red   = IntToFixed(r);
      span->green = IntToFixed(g);
      span->blue  = IntToFixed(b);
      span->alpha = IntToFixed(a);
#endif
      span->redStep = 0;
      span->greenStep = 0;
      span->blueStep = 0;
      span->alphaStep = 0;
      span->interpMask |= SPAN_RGBA;

      COPY_4V(span->attrStart[FRAG_ATTRIB_COL0], ctx->Current.RasterColor);
      ASSIGN_4V(span->attrStepX[FRAG_ATTRIB_COL0], 0.0, 0.0, 0.0, 0.0);
      ASSIGN_4V(span->attrStepY[FRAG_ATTRIB_COL0], 0.0, 0.0, 0.0, 0.0);
   }
   else {
      span->index = FloatToFixed(ctx->Current.RasterIndex);
      span->indexStep = 0;
      span->interpMask |= SPAN_INDEX;
   }

   /* Secondary color */
   if (ctx->Visual.rgbMode && (ctx->Light.Enabled || ctx->Fog.ColorSumEnabled))
   {
      COPY_4V(span->attrStart[FRAG_ATTRIB_COL1], ctx->Current.RasterSecondaryColor);
      ASSIGN_4V(span->attrStepX[FRAG_ATTRIB_COL1], 0.0, 0.0, 0.0, 0.0);
      ASSIGN_4V(span->attrStepY[FRAG_ATTRIB_COL1], 0.0, 0.0, 0.0, 0.0);
   }

   /* fog */
   {
      const SWcontext *swrast = SWRAST_CONTEXT(ctx);
      GLfloat fogVal; /* a coord or a blend factor */
      if (swrast->_PreferPixelFog) {
         /* fog blend factors will be computed from fog coordinates per pixel */
         fogVal = ctx->Current.RasterDistance;
      }
      else {
         /* fog blend factor should be computed from fogcoord now */
         fogVal = _swrast_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
      }
      span->attrStart[FRAG_ATTRIB_FOGC][0] = fogVal;
      span->attrStepX[FRAG_ATTRIB_FOGC][0] = 0.0;
      span->attrStepY[FRAG_ATTRIB_FOGC][0] = 0.0;
   }

   /* texcoords */
   {
      GLuint i;
      for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
         const GLuint attr = FRAG_ATTRIB_TEX0 + i;
         const GLfloat *tc = ctx->Current.RasterTexCoords[i];
         if (ctx->FragmentProgram._Current || ctx->ATIFragmentShader._Enabled) {
            COPY_4V(span->attrStart[attr], tc);
         }
         else if (tc[3] > 0.0F) {
            /* use (s/q, t/q, r/q, 1) */
            span->attrStart[attr][0] = tc[0] / tc[3];
            span->attrStart[attr][1] = tc[1] / tc[3];
            span->attrStart[attr][2] = tc[2] / tc[3];
            span->attrStart[attr][3] = 1.0;
         }
         else {
            ASSIGN_4V(span->attrStart[attr], 0.0F, 0.0F, 0.0F, 1.0F);
         }
         ASSIGN_4V(span->attrStepX[attr], 0.0F, 0.0F, 0.0F, 0.0F);
         ASSIGN_4V(span->attrStepY[attr], 0.0F, 0.0F, 0.0F, 0.0F);
      }
   }
}


/**
 * Interpolate the active attributes (and'd with attrMask) to
 * fill in span->array->attribs[].
 * Perspective correction will be done.  The point/line/triangle function
 * should have computed attrStart/Step values for FRAG_ATTRIB_WPOS[3]!
 */
static INLINE void
interpolate_active_attribs(GLcontext *ctx, SWspan *span, GLbitfield attrMask)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /*
    * Don't overwrite existing array values, such as colors that may have
    * been produced by glDraw/CopyPixels.
    */
   attrMask &= ~span->arrayAttribs;

   ATTRIB_LOOP_BEGIN
      if (attrMask & (1 << attr)) {
         const GLfloat dwdx = span->attrStepX[FRAG_ATTRIB_WPOS][3];
         GLfloat w = span->attrStart[FRAG_ATTRIB_WPOS][3];
         const GLfloat dv0dx = span->attrStepX[attr][0];
         const GLfloat dv1dx = span->attrStepX[attr][1];
         const GLfloat dv2dx = span->attrStepX[attr][2];
         const GLfloat dv3dx = span->attrStepX[attr][3];
         GLfloat v0 = span->attrStart[attr][0];
         GLfloat v1 = span->attrStart[attr][1];
         GLfloat v2 = span->attrStart[attr][2];
         GLfloat v3 = span->attrStart[attr][3];
         GLuint k;
         for (k = 0; k < span->end; k++) {
            const GLfloat invW = 1.0f / w;
            span->array->attribs[attr][k][0] = v0 * invW;
            span->array->attribs[attr][k][1] = v1 * invW;
            span->array->attribs[attr][k][2] = v2 * invW;
            span->array->attribs[attr][k][3] = v3 * invW;
            v0 += dv0dx;
            v1 += dv1dx;
            v2 += dv2dx;
            v3 += dv3dx;
            w += dwdx;
         }
         ASSERT((span->arrayAttribs & (1 << attr)) == 0);
         span->arrayAttribs |= (1 << attr);
      }
   ATTRIB_LOOP_END
}


/**
 * Interpolate primary colors to fill in the span->array->rgba8 (or rgb16)
 * color array.
 */
static INLINE void
interpolate_int_colors(GLcontext *ctx, SWspan *span)
{
   const GLuint n = span->end;
   GLuint i;

#if CHAN_BITS != 32
   ASSERT(!(span->arrayMask & SPAN_RGBA));
#endif

   switch (span->array->ChanType) {
#if CHAN_BITS != 32
   case GL_UNSIGNED_BYTE:
      {
         GLubyte (*rgba)[4] = span->array->rgba8;
         if (span->interpMask & SPAN_FLAT) {
            GLubyte color[4];
            color[RCOMP] = FixedToInt(span->red);
            color[GCOMP] = FixedToInt(span->green);
            color[BCOMP] = FixedToInt(span->blue);
            color[ACOMP] = FixedToInt(span->alpha);
            for (i = 0; i < n; i++) {
               COPY_4UBV(rgba[i], color);
            }
         }
         else {
            GLfixed r = span->red;
            GLfixed g = span->green;
            GLfixed b = span->blue;
            GLfixed a = span->alpha;
            GLint dr = span->redStep;
            GLint dg = span->greenStep;
            GLint db = span->blueStep;
            GLint da = span->alphaStep;
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = FixedToChan(r);
               rgba[i][GCOMP] = FixedToChan(g);
               rgba[i][BCOMP] = FixedToChan(b);
               rgba[i][ACOMP] = FixedToChan(a);
               r += dr;
               g += dg;
               b += db;
               a += da;
            }
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort (*rgba)[4] = span->array->rgba16;
         if (span->interpMask & SPAN_FLAT) {
            GLushort color[4];
            color[RCOMP] = FixedToInt(span->red);
            color[GCOMP] = FixedToInt(span->green);
            color[BCOMP] = FixedToInt(span->blue);
            color[ACOMP] = FixedToInt(span->alpha);
            for (i = 0; i < n; i++) {
               COPY_4V(rgba[i], color);
            }
         }
         else {
            GLushort (*rgba)[4] = span->array->rgba16;
            GLfixed r, g, b, a;
            GLint dr, dg, db, da;
            r = span->red;
            g = span->green;
            b = span->blue;
            a = span->alpha;
            dr = span->redStep;
            dg = span->greenStep;
            db = span->blueStep;
            da = span->alphaStep;
            for (i = 0; i < n; i++) {
               rgba[i][RCOMP] = FixedToChan(r);
               rgba[i][GCOMP] = FixedToChan(g);
               rgba[i][BCOMP] = FixedToChan(b);
               rgba[i][ACOMP] = FixedToChan(a);
               r += dr;
               g += dg;
               b += db;
               a += da;
            }
         }
      }
      break;
#endif
   case GL_FLOAT:
      interpolate_active_attribs(ctx, span, FRAG_BIT_COL0);
      break;
   default:
      _mesa_problem(NULL, "bad datatype in interpolate_int_colors");
   }
   span->arrayMask |= SPAN_RGBA;
}


/**
 * Populate the FRAG_ATTRIB_COL0 array.
 */
static INLINE void
interpolate_float_colors(SWspan *span)
{
   GLfloat (*col0)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
   const GLuint n = span->end;
   GLuint i;

   assert(!(span->arrayAttribs & FRAG_BIT_COL0));

   if (span->arrayMask & SPAN_RGBA) {
      /* convert array of int colors */
      for (i = 0; i < n; i++) {
         col0[i][0] = UBYTE_TO_FLOAT(span->array->rgba8[i][0]);
         col0[i][1] = UBYTE_TO_FLOAT(span->array->rgba8[i][1]);
         col0[i][2] = UBYTE_TO_FLOAT(span->array->rgba8[i][2]);
         col0[i][3] = UBYTE_TO_FLOAT(span->array->rgba8[i][3]);
      }
   }
   else {
      /* interpolate red/green/blue/alpha to get float colors */
      ASSERT(span->interpMask & SPAN_RGBA);
      if (span->interpMask & SPAN_FLAT) {
         GLfloat r = FixedToFloat(span->red);
         GLfloat g = FixedToFloat(span->green);
         GLfloat b = FixedToFloat(span->blue);
         GLfloat a = FixedToFloat(span->alpha);
         for (i = 0; i < n; i++) {
            ASSIGN_4V(col0[i], r, g, b, a);
         }
      }
      else {
         GLfloat r = FixedToFloat(span->red);
         GLfloat g = FixedToFloat(span->green);
         GLfloat b = FixedToFloat(span->blue);
         GLfloat a = FixedToFloat(span->alpha);
         GLfloat dr = FixedToFloat(span->redStep);
         GLfloat dg = FixedToFloat(span->greenStep);
         GLfloat db = FixedToFloat(span->blueStep);
         GLfloat da = FixedToFloat(span->alphaStep);
         for (i = 0; i < n; i++) {
            col0[i][0] = r;
            col0[i][1] = g;
            col0[i][2] = b;
            col0[i][3] = a;
            r += dr;
            g += dg;
            b += db;
            a += da;
         }
      }
   }

   span->arrayAttribs |= FRAG_BIT_COL0;
   span->array->ChanType = GL_FLOAT;
}



/* Fill in the span.color.index array from the interpolation values */
static INLINE void
interpolate_indexes(GLcontext *ctx, SWspan *span)
{
   GLfixed index = span->index;
   const GLint indexStep = span->indexStep;
   const GLuint n = span->end;
   GLuint *indexes = span->array->index;
   GLuint i;
   (void) ctx;

   ASSERT(!(span->arrayMask & SPAN_INDEX));

   if ((span->interpMask & SPAN_FLAT) || (indexStep == 0)) {
      /* constant color */
      index = FixedToInt(index);
      for (i = 0; i < n; i++) {
         indexes[i] = index;
      }
   }
   else {
      /* interpolate */
      for (i = 0; i < n; i++) {
         indexes[i] = FixedToInt(index);
         index += indexStep;
      }
   }
   span->arrayMask |= SPAN_INDEX;
   span->interpMask &= ~SPAN_INDEX;
}


/**
 * Fill in the span.zArray array from the span->z, zStep values.
 */
void
_swrast_span_interpolate_z( const GLcontext *ctx, SWspan *span )
{
   const GLuint n = span->end;
   GLuint i;

   ASSERT(!(span->arrayMask & SPAN_Z));

   if (ctx->DrawBuffer->Visual.depthBits <= 16) {
      GLfixed zval = span->z;
      GLuint *z = span->array->z; 
      for (i = 0; i < n; i++) {
         z[i] = FixedToInt(zval);
         zval += span->zStep;
      }
   }
   else {
      /* Deep Z buffer, no fixed->int shift */
      GLuint zval = span->z;
      GLuint *z = span->array->z;
      for (i = 0; i < n; i++) {
         z[i] = zval;
         zval += span->zStep;
      }
   }
   span->interpMask &= ~SPAN_Z;
   span->arrayMask |= SPAN_Z;
}


/**
 * Compute mipmap LOD from partial derivatives.
 * This the ideal solution, as given in the OpenGL spec.
 */
GLfloat
_swrast_compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
                       GLfloat dqdx, GLfloat dqdy, GLfloat texW, GLfloat texH,
                       GLfloat s, GLfloat t, GLfloat q, GLfloat invQ)
{
   GLfloat dudx = texW * ((s + dsdx) / (q + dqdx) - s * invQ);
   GLfloat dvdx = texH * ((t + dtdx) / (q + dqdx) - t * invQ);
   GLfloat dudy = texW * ((s + dsdy) / (q + dqdy) - s * invQ);
   GLfloat dvdy = texH * ((t + dtdy) / (q + dqdy) - t * invQ);
   GLfloat x = SQRTF(dudx * dudx + dvdx * dvdx);
   GLfloat y = SQRTF(dudy * dudy + dvdy * dvdy);
   GLfloat rho = MAX2(x, y);
   GLfloat lambda = LOG2(rho);
   return lambda;
}


/**
 * Compute mipmap LOD from partial derivatives.
 * This is a faster approximation than above function.
 */
#if 0
GLfloat
_swrast_compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
                     GLfloat dqdx, GLfloat dqdy, GLfloat texW, GLfloat texH,
                     GLfloat s, GLfloat t, GLfloat q, GLfloat invQ)
{
   GLfloat dsdx2 = (s + dsdx) / (q + dqdx) - s * invQ;
   GLfloat dtdx2 = (t + dtdx) / (q + dqdx) - t * invQ;
   GLfloat dsdy2 = (s + dsdy) / (q + dqdy) - s * invQ;
   GLfloat dtdy2 = (t + dtdy) / (q + dqdy) - t * invQ;
   GLfloat maxU, maxV, rho, lambda;
   dsdx2 = FABSF(dsdx2);
   dsdy2 = FABSF(dsdy2);
   dtdx2 = FABSF(dtdx2);
   dtdy2 = FABSF(dtdy2);
   maxU = MAX2(dsdx2, dsdy2) * texW;
   maxV = MAX2(dtdx2, dtdy2) * texH;
   rho = MAX2(maxU, maxV);
   lambda = LOG2(rho);
   return lambda;
}
#endif


/**
 * Fill in the span.array->attrib[FRAG_ATTRIB_TEXn] arrays from the
 * using the attrStart/Step values.
 *
 * This function only used during fixed-function fragment processing.
 *
 * Note: in the places where we divide by Q (or mult by invQ) we're
 * really doing two things: perspective correction and texcoord
 * projection.  Remember, for texcoord (s,t,r,q) we need to index
 * texels with (s/q, t/q, r/q).
 */
static void
interpolate_texcoords(GLcontext *ctx, SWspan *span)
{
   const GLuint maxUnit
      = (ctx->Texture._EnabledCoordUnits > 1) ? ctx->Const.MaxTextureUnits : 1;
   GLuint u;

   /* XXX CoordUnits vs. ImageUnits */
   for (u = 0; u < maxUnit; u++) {
      if (ctx->Texture._EnabledCoordUnits & (1 << u)) {
         const GLuint attr = FRAG_ATTRIB_TEX0 + u;
         const struct gl_texture_object *obj = ctx->Texture.Unit[u]._Current;
         GLfloat texW, texH;
         GLboolean needLambda;
         GLfloat (*texcoord)[4] = span->array->attribs[attr];
         GLfloat *lambda = span->array->lambda[u];
         const GLfloat dsdx = span->attrStepX[attr][0];
         const GLfloat dsdy = span->attrStepY[attr][0];
         const GLfloat dtdx = span->attrStepX[attr][1];
         const GLfloat dtdy = span->attrStepY[attr][1];
         const GLfloat drdx = span->attrStepX[attr][2];
         const GLfloat dqdx = span->attrStepX[attr][3];
         const GLfloat dqdy = span->attrStepY[attr][3];
         GLfloat s = span->attrStart[attr][0];
         GLfloat t = span->attrStart[attr][1];
         GLfloat r = span->attrStart[attr][2];
         GLfloat q = span->attrStart[attr][3];

         if (obj) {
            const struct gl_texture_image *img = obj->Image[0][obj->BaseLevel];
            needLambda = (obj->MinFilter != obj->MagFilter)
               || ctx->FragmentProgram._Current;
            texW = img->WidthScale;
            texH = img->HeightScale;
         }
         else {
            /* using a fragment program */
            texW = 1.0;
            texH = 1.0;
            needLambda = GL_FALSE;
         }

         if (needLambda) {
            GLuint i;
            if (ctx->FragmentProgram._Current
                || ctx->ATIFragmentShader._Enabled) {
               /* do perspective correction but don't divide s, t, r by q */
               const GLfloat dwdx = span->attrStepX[FRAG_ATTRIB_WPOS][3];
               GLfloat w = span->attrStart[FRAG_ATTRIB_WPOS][3];
               for (i = 0; i < span->end; i++) {
                  const GLfloat invW = 1.0F / w;
                  texcoord[i][0] = s * invW;
                  texcoord[i][1] = t * invW;
                  texcoord[i][2] = r * invW;
                  texcoord[i][3] = q * invW;
                  lambda[i] = _swrast_compute_lambda(dsdx, dsdy, dtdx, dtdy,
                                                     dqdx, dqdy, texW, texH,
                                                     s, t, q, invW);
                  s += dsdx;
                  t += dtdx;
                  r += drdx;
                  q += dqdx;
                  w += dwdx;
               }
            }
            else {
               for (i = 0; i < span->end; i++) {
                  const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
                  texcoord[i][0] = s * invQ;
                  texcoord[i][1] = t * invQ;
                  texcoord[i][2] = r * invQ;
                  texcoord[i][3] = q;
                  lambda[i] = _swrast_compute_lambda(dsdx, dsdy, dtdx, dtdy,
                                                     dqdx, dqdy, texW, texH,
                                                     s, t, q, invQ);
                  s += dsdx;
                  t += dtdx;
                  r += drdx;
                  q += dqdx;
               }
            }
            span->arrayMask |= SPAN_LAMBDA;
         }
         else {
            GLuint i;
            if (ctx->FragmentProgram._Current ||
                ctx->ATIFragmentShader._Enabled) {
               /* do perspective correction but don't divide s, t, r by q */
               const GLfloat dwdx = span->attrStepX[FRAG_ATTRIB_WPOS][3];
               GLfloat w = span->attrStart[FRAG_ATTRIB_WPOS][3];
               for (i = 0; i < span->end; i++) {
                  const GLfloat invW = 1.0F / w;
                  texcoord[i][0] = s * invW;
                  texcoord[i][1] = t * invW;
                  texcoord[i][2] = r * invW;
                  texcoord[i][3] = q * invW;
                  lambda[i] = 0.0;
                  s += dsdx;
                  t += dtdx;
                  r += drdx;
                  q += dqdx;
                  w += dwdx;
               }
            }
            else if (dqdx == 0.0F) {
               /* Ortho projection or polygon's parallel to window X axis */
               const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
               for (i = 0; i < span->end; i++) {
                  texcoord[i][0] = s * invQ;
                  texcoord[i][1] = t * invQ;
                  texcoord[i][2] = r * invQ;
                  texcoord[i][3] = q;
                  lambda[i] = 0.0;
                  s += dsdx;
                  t += dtdx;
                  r += drdx;
               }
            }
            else {
               for (i = 0; i < span->end; i++) {
                  const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
                  texcoord[i][0] = s * invQ;
                  texcoord[i][1] = t * invQ;
                  texcoord[i][2] = r * invQ;
                  texcoord[i][3] = q;
                  lambda[i] = 0.0;
                  s += dsdx;
                  t += dtdx;
                  r += drdx;
                  q += dqdx;
               }
            }
         } /* lambda */
      } /* if */
   } /* for */
}


/**
 * Fill in the arrays->attribs[FRAG_ATTRIB_WPOS] array.
 */
static INLINE void
interpolate_wpos(GLcontext *ctx, SWspan *span)
{
   GLfloat (*wpos)[4] = span->array->attribs[FRAG_ATTRIB_WPOS];
   GLuint i;
   const GLfloat zScale = 1.0 / ctx->DrawBuffer->_DepthMaxF;
   GLfloat w, dw;

   if (span->arrayMask & SPAN_XY) {
      for (i = 0; i < span->end; i++) {
         wpos[i][0] = (GLfloat) span->array->x[i];
         wpos[i][1] = (GLfloat) span->array->y[i];
      }
   }
   else {
      for (i = 0; i < span->end; i++) {
         wpos[i][0] = (GLfloat) span->x + i;
         wpos[i][1] = (GLfloat) span->y;
      }
   }

   w = span->attrStart[FRAG_ATTRIB_WPOS][3];
   dw = span->attrStepX[FRAG_ATTRIB_WPOS][3];
   for (i = 0; i < span->end; i++) {
      wpos[i][2] = (GLfloat) span->array->z[i] * zScale;
      wpos[i][3] = w;
      w += dw;
   }
}


/**
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static INLINE void
stipple_polygon_span(GLcontext *ctx, SWspan *span)
{
   GLubyte *mask = span->array->mask;

   ASSERT(ctx->Polygon.StippleFlag);

   if (span->arrayMask & SPAN_XY) {
      /* arrays of x/y pixel coords */
      GLuint i;
      for (i = 0; i < span->end; i++) {
         const GLint col = span->array->x[i] % 32;
         const GLint row = span->array->y[i] % 32;
         const GLuint stipple = ctx->PolygonStipple[row];
         if (((1 << col) & stipple) == 0) {
            mask[i] = 0;
         }
      }
   }
   else {
      /* horizontal span of pixels */
      const GLuint highBit = 1 << 31;
      const GLuint stipple = ctx->PolygonStipple[span->y % 32];
      GLuint i, m = highBit >> (GLuint) (span->x % 32);
      for (i = 0; i < span->end; i++) {
         if ((m & stipple) == 0) {
            mask[i] = 0;
         }
         m = m >> 1;
         if (m == 0) {
            m = highBit;
         }
      }
   }
   span->writeAll = GL_FALSE;
}


/**
 * Clip a pixel span to the current buffer/window boundaries:
 * DrawBuffer->_Xmin, _Xmax, _Ymin, _Ymax.  This will accomplish
 * window clipping and scissoring.
 * Return:   GL_TRUE   some pixels still visible
 *           GL_FALSE  nothing visible
 */
static INLINE GLuint
clip_span( GLcontext *ctx, SWspan *span )
{
   const GLint xmin = ctx->DrawBuffer->_Xmin;
   const GLint xmax = ctx->DrawBuffer->_Xmax;
   const GLint ymin = ctx->DrawBuffer->_Ymin;
   const GLint ymax = ctx->DrawBuffer->_Ymax;

   if (span->arrayMask & SPAN_XY) {
      /* arrays of x/y pixel coords */
      const GLint *x = span->array->x;
      const GLint *y = span->array->y;
      const GLint n = span->end;
      GLubyte *mask = span->array->mask;
      GLint i;
      if (span->arrayMask & SPAN_MASK) {
         /* note: using & intead of && to reduce branches */
         for (i = 0; i < n; i++) {
            mask[i] &= (x[i] >= xmin) & (x[i] < xmax)
                     & (y[i] >= ymin) & (y[i] < ymax);
         }
      }
      else {
         /* note: using & intead of && to reduce branches */
         for (i = 0; i < n; i++) {
            mask[i] = (x[i] >= xmin) & (x[i] < xmax)
                    & (y[i] >= ymin) & (y[i] < ymax);
         }
      }
      return GL_TRUE;  /* some pixels visible */
   }
   else {
      /* horizontal span of pixels */
      const GLint x = span->x;
      const GLint y = span->y;
      const GLint n = span->end;

      /* Trivial rejection tests */
      if (y < ymin || y >= ymax || x + n <= xmin || x >= xmax) {
         span->end = 0;
         return GL_FALSE;  /* all pixels clipped */
      }

      /* Clip to the left */
      if (x < xmin) {
         ASSERT(x + n > xmin);
         span->writeAll = GL_FALSE;
         _mesa_bzero(span->array->mask, (xmin - x) * sizeof(GLubyte));
      }

      /* Clip to right */
      if (x + n > xmax) {
         ASSERT(x < xmax);
         span->end = xmax - x;
      }

      return GL_TRUE;  /* some pixels visible */
   }
}


/**
 * Apply all the per-fragment opertions to a span of color index fragments
 * and write them to the enabled color drawbuffers.
 * The 'span' parameter can be considered to be const.  Note that
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_index_span( GLcontext *ctx, SWspan *span)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLbitfield origInterpMask = span->interpMask;
   const GLbitfield origArrayMask = span->arrayMask;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(span->primitive == GL_POINT  ||  span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON  ||  span->primitive == GL_BITMAP);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_INDEX);
   /*
   ASSERT((span->interpMask & span->arrayMask) == 0);
   */

   if (span->arrayMask & SPAN_MASK) {
      /* mask was initialized by caller, probably glBitmap */
      span->writeAll = GL_FALSE;
   }
   else {
      _mesa_memset(span->array->mask, 1, span->end);
      span->writeAll = GL_TRUE;
   }

   /* Clipping */
   if ((swrast->_RasterMask & CLIP_BIT) || (span->primitive != GL_POLYGON)) {
      if (!clip_span(ctx, span)) {
         return;
      }
   }

   /* Depth bounds test */
   if (ctx->Depth.BoundsTest && fb->Visual.depthBits > 0) {
      if (!_swrast_depth_bounds_test(ctx, span)) {
         return;
      }
   }

#ifdef DEBUG
   /* Make sure all fragments are within window bounds */
   if (span->arrayMask & SPAN_XY) {
      GLuint i;
      for (i = 0; i < span->end; i++) {
         if (span->array->mask[i]) {
            assert(span->array->x[i] >= fb->_Xmin);
            assert(span->array->x[i] < fb->_Xmax);
            assert(span->array->y[i] >= fb->_Ymin);
            assert(span->array->y[i] < fb->_Ymax);
         }
      }
   }
#endif

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && span->primitive == GL_POLYGON) {
      stipple_polygon_span(ctx, span);
   }

   /* Stencil and Z testing */
   if (ctx->Depth.Test || ctx->Stencil.Enabled) {
      if (!(span->arrayMask & SPAN_Z))
         _swrast_span_interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled) {
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         if (!_swrast_depth_test_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

#if FEATURE_ARB_occlusion_query
   if (ctx->Query.CurrentOcclusionObject) {
      /* update count of 'passed' fragments */
      struct gl_query_object *q = ctx->Query.CurrentOcclusionObject;
      GLuint i;
      for (i = 0; i < span->end; i++)
         q->Result += span->array->mask[i];
   }
#endif

   /* we have to wait until after occlusion to do this test */
   if (ctx->Color.IndexMask == 0) {
      /* write no pixels */
      span->arrayMask = origArrayMask;
      return;
   }

   /* Interpolate the color indexes if needed */
   if (swrast->_FogEnabled ||
       ctx->Color.IndexLogicOpEnabled ||
       ctx->Color.IndexMask != 0xffffffff ||
       (span->arrayMask & SPAN_COVERAGE)) {
      if (!(span->arrayMask & SPAN_INDEX) /*span->interpMask & SPAN_INDEX*/) {
         interpolate_indexes(ctx, span);
      }
   }

   /* Fog */
   if (swrast->_FogEnabled) {
      _swrast_fog_ci_span(ctx, span);
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      const GLfloat *coverage = span->array->coverage;
      GLuint *index = span->array->index;
      GLuint i;
      for (i = 0; i < span->end; i++) {
         ASSERT(coverage[i] < 16);
         index[i] = (index[i] & ~0xf) | ((GLuint) coverage[i]);
      }
   }

   /*
    * Write to renderbuffers
    */
   {
      const GLuint numBuffers = fb->_NumColorDrawBuffers;
      GLuint buf;

      for (buf = 0; buf < numBuffers; buf++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[buf];
         GLuint indexSave[MAX_WIDTH];

         ASSERT(rb->_BaseFormat == GL_COLOR_INDEX);

         if (numBuffers > 1) {
            /* save indexes for second, third renderbuffer writes */
            _mesa_memcpy(indexSave, span->array->index,
                         span->end * sizeof(indexSave[0]));
         }

         if (ctx->Color.IndexLogicOpEnabled) {
            _swrast_logicop_ci_span(ctx, rb, span);
         }

         if (ctx->Color.IndexMask != 0xffffffff) {
            _swrast_mask_ci_span(ctx, rb, span);
         }

         if (!(span->arrayMask & SPAN_INDEX) && span->indexStep == 0) {
            /* all fragments have same color index */
            GLubyte index8;
            GLushort index16;
            GLuint index32;
            void *value;

            if (rb->DataType == GL_UNSIGNED_BYTE) {
               index8 = FixedToInt(span->index);
               value = &index8;
            }
            else if (rb->DataType == GL_UNSIGNED_SHORT) {
               index16 = FixedToInt(span->index);
               value = &index16;
            }
            else {
               ASSERT(rb->DataType == GL_UNSIGNED_INT);
               index32 = FixedToInt(span->index);
               value = &index32;
            }

            if (span->arrayMask & SPAN_XY) {
               rb->PutMonoValues(ctx, rb, span->end, span->array->x, 
                                 span->array->y, value, span->array->mask);
            }
            else {
               rb->PutMonoRow(ctx, rb, span->end, span->x, span->y,
                              value, span->array->mask);
            }
         }
         else {
            /* each fragment is a different color */
            GLubyte index8[MAX_WIDTH];
            GLushort index16[MAX_WIDTH];
            void *values;

            if (rb->DataType == GL_UNSIGNED_BYTE) {
               GLuint k;
               for (k = 0; k < span->end; k++) {
                  index8[k] = (GLubyte) span->array->index[k];
               }
               values = index8;
            }
            else if (rb->DataType == GL_UNSIGNED_SHORT) {
               GLuint k;
               for (k = 0; k < span->end; k++) {
                  index16[k] = (GLushort) span->array->index[k];
               }
               values = index16;
            }
            else {
               ASSERT(rb->DataType == GL_UNSIGNED_INT);
               values = span->array->index;
            }

            if (span->arrayMask & SPAN_XY) {
               rb->PutValues(ctx, rb, span->end,
                             span->array->x, span->array->y,
                             values, span->array->mask);
            }
            else {
               rb->PutRow(ctx, rb, span->end, span->x, span->y,
                          values, span->array->mask);
            }
         }

         if (buf + 1 < numBuffers) {
            /* restore original span values */
            _mesa_memcpy(span->array->index, indexSave,
                         span->end * sizeof(indexSave[0]));
         }
      } /* for buf */
   }

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}


/**
 * Add specular colors to primary colors.
 * Only called during fixed-function operation.
 * Result is float color array (FRAG_ATTRIB_COL0).
 */
static INLINE void
add_specular(GLcontext *ctx, SWspan *span)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLubyte *mask = span->array->mask;
   GLfloat (*col0)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
   GLfloat (*col1)[4] = span->array->attribs[FRAG_ATTRIB_COL1];
   GLuint i;

   ASSERT(!ctx->FragmentProgram._Current);
   ASSERT(span->arrayMask & SPAN_RGBA);
   ASSERT(swrast->_ActiveAttribMask & FRAG_BIT_COL1);
   (void) swrast; /* silence warning */

   if (span->array->ChanType == GL_FLOAT) {
      if ((span->arrayAttribs & FRAG_BIT_COL0) == 0) {
         interpolate_active_attribs(ctx, span, FRAG_BIT_COL0);
      }
   }
   else {
      /* need float colors */
      if ((span->arrayAttribs & FRAG_BIT_COL0) == 0) {
         interpolate_float_colors(span);
      }
   }

   if ((span->arrayAttribs & FRAG_BIT_COL1) == 0) {
      /* XXX could avoid this and interpolate COL1 in the loop below */
      interpolate_active_attribs(ctx, span, FRAG_BIT_COL1);
   }

   ASSERT(span->arrayAttribs & FRAG_BIT_COL0);
   ASSERT(span->arrayAttribs & FRAG_BIT_COL1);

   for (i = 0; i < span->end; i++) {
      if (mask[i]) {
         col0[i][0] += col1[i][0];
         col0[i][1] += col1[i][1];
         col0[i][2] += col1[i][2];
      }
   }

   span->array->ChanType = GL_FLOAT;
}


/**
 * Apply antialiasing coverage value to alpha values.
 */
static INLINE void
apply_aa_coverage(SWspan *span)
{
   const GLfloat *coverage = span->array->coverage;
   GLuint i;
   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      GLubyte (*rgba)[4] = span->array->rgba8;
      for (i = 0; i < span->end; i++) {
         const GLfloat a = rgba[i][ACOMP] * coverage[i];
         rgba[i][ACOMP] = (GLubyte) CLAMP(a, 0.0, 255.0);
         ASSERT(coverage[i] >= 0.0);
         ASSERT(coverage[i] <= 1.0);
      }
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = span->array->rgba16;
      for (i = 0; i < span->end; i++) {
         const GLfloat a = rgba[i][ACOMP] * coverage[i];
         rgba[i][ACOMP] = (GLushort) CLAMP(a, 0.0, 65535.0);
      }
   }
   else {
      GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * coverage[i];
         /* clamp later */
      }
   }
}


/**
 * Clamp span's float colors to [0,1]
 */
static INLINE void
clamp_colors(SWspan *span)
{
   GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
   GLuint i;
   ASSERT(span->array->ChanType == GL_FLOAT);
   for (i = 0; i < span->end; i++) {
      rgba[i][RCOMP] = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
      rgba[i][GCOMP] = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
      rgba[i][BCOMP] = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
      rgba[i][ACOMP] = CLAMP(rgba[i][ACOMP], 0.0F, 1.0F);
   }
}


/**
 * Convert the span's color arrays to the given type.
 * The only way 'output' can be greater than zero is when we have a fragment
 * program that writes to gl_FragData[1] or higher.
 * \param output  which fragment program color output is being processed
 */
static INLINE void
convert_color_type(SWspan *span, GLenum newType, GLuint output)
{
   GLvoid *src, *dst;

   if (output > 0 || span->array->ChanType == GL_FLOAT) {
      src = span->array->attribs[FRAG_ATTRIB_COL0 + output];
      span->array->ChanType = GL_FLOAT;
   }
   else if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      src = span->array->rgba8;
   }
   else {
      ASSERT(span->array->ChanType == GL_UNSIGNED_SHORT);
      src = span->array->rgba16;
   }

   if (newType == GL_UNSIGNED_BYTE) {
      dst = span->array->rgba8;
   }
   else if (newType == GL_UNSIGNED_SHORT) {
      dst = span->array->rgba16;
   }
   else {
      dst = span->array->attribs[FRAG_ATTRIB_COL0];
   }

   _mesa_convert_colors(span->array->ChanType, src,
                        newType, dst,
                        span->end, span->array->mask);

   span->array->ChanType = newType;
   span->array->rgba = dst;
}



/**
 * Apply fragment shader, fragment program or normal texturing to span.
 */
static INLINE void
shade_texture_span(GLcontext *ctx, SWspan *span)
{
   GLbitfield inputsRead;

   /* Determine which fragment attributes are actually needed */
   if (ctx->FragmentProgram._Current) {
      inputsRead = ctx->FragmentProgram._Current->Base.InputsRead;
   }
   else {
      /* XXX we could be a bit smarter about this */
      inputsRead = ~0;
   }

   if (ctx->FragmentProgram._Current ||
       ctx->ATIFragmentShader._Enabled) {
      /* programmable shading */
      if (span->primitive == GL_BITMAP && span->array->ChanType != GL_FLOAT) {
         convert_color_type(span, GL_FLOAT, 0);
      }
      if (span->primitive != GL_POINT ||
	  (span->interpMask & SPAN_RGBA) ||
	  ctx->Point.PointSprite) {
         /* for single-pixel points, we populated the arrays already */
         interpolate_active_attribs(ctx, span, ~0);
      }
      span->array->ChanType = GL_FLOAT;

      if (!(span->arrayMask & SPAN_Z))
         _swrast_span_interpolate_z (ctx, span);

#if 0
      if (inputsRead & FRAG_BIT_WPOS)
#else
      /* XXX always interpolate wpos so that DDX/DDY work */
#endif
         interpolate_wpos(ctx, span);

      /* Run fragment program/shader now */
      if (ctx->FragmentProgram._Current) {
         _swrast_exec_fragment_program(ctx, span);
      }
      else {
         ASSERT(ctx->ATIFragmentShader._Enabled);
         _swrast_exec_fragment_shader(ctx, span);
      }
   }
   else if (ctx->Texture._EnabledUnits) {
      /* conventional texturing */

#if CHAN_BITS == 32
      if ((span->arrayAttribs & FRAG_BIT_COL0) == 0) {
         interpolate_int_colors(ctx, span);
      }
#else
      if (!(span->arrayMask & SPAN_RGBA))
         interpolate_int_colors(ctx, span);
#endif
      if ((span->arrayAttribs & FRAG_BITS_TEX_ANY) == 0x0)
         interpolate_texcoords(ctx, span);

      _swrast_texture_span(ctx, span);
   }
}



/**
 * Apply all the per-fragment operations to a span.
 * This now includes texturing (_swrast_write_texture_span() is history).
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_rgba_span( GLcontext *ctx, SWspan *span)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   const GLbitfield origInterpMask = span->interpMask;
   const GLbitfield origArrayMask = span->arrayMask;
   const GLbitfield origArrayAttribs = span->arrayAttribs;
   const GLenum origChanType = span->array->ChanType;
   void * const origRgba = span->array->rgba;
   const GLboolean shader = (ctx->FragmentProgram._Current
                             || ctx->ATIFragmentShader._Enabled);
   const GLboolean shaderOrTexture = shader || ctx->Texture._EnabledUnits;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /*
   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__,
          span->interpMask, span->arrayMask);
   */

   ASSERT(span->primitive == GL_POINT ||
          span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON ||
          span->primitive == GL_BITMAP);
   ASSERT(span->end <= MAX_WIDTH);

   /* Fragment write masks */
   if (span->arrayMask & SPAN_MASK) {
      /* mask was initialized by caller, probably glBitmap */
      span->writeAll = GL_FALSE;
   }
   else {
      _mesa_memset(span->array->mask, 1, span->end);
      span->writeAll = GL_TRUE;
   }

   /* Clip to window/scissor box */
   if ((swrast->_RasterMask & CLIP_BIT) || (span->primitive != GL_POLYGON)) {
      if (!clip_span(ctx, span)) {
	 return;
      }
   }

#ifdef DEBUG
   /* Make sure all fragments are within window bounds */
   if (span->arrayMask & SPAN_XY) {
      GLuint i;
      for (i = 0; i < span->end; i++) {
         if (span->array->mask[i]) {
            assert(span->array->x[i] >= fb->_Xmin);
            assert(span->array->x[i] < fb->_Xmax);
            assert(span->array->y[i] >= fb->_Ymin);
            assert(span->array->y[i] < fb->_Ymax);
         }
      }
   }
#endif

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && span->primitive == GL_POLYGON) {
      stipple_polygon_span(ctx, span);
   }

   /* This is the normal place to compute the fragment color/Z
    * from texturing or shading.
    */
   if (shaderOrTexture && !swrast->_DeferredTexture) {
      shade_texture_span(ctx, span);
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (!_swrast_alpha_test(ctx, span)) {
         goto end;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil.Enabled || ctx->Depth.Test) {
      if (!(span->arrayMask & SPAN_Z))
         _swrast_span_interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled && fb->Visual.stencilBits > 0) {
         /* Combined Z/stencil tests */
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            goto end;
         }
      }
      else if (fb->Visual.depthBits > 0) {
         /* Just regular depth testing */
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         if (!_swrast_depth_test_span(ctx, span)) {
            goto end;
         }
      }
   }

#if FEATURE_ARB_occlusion_query
   if (ctx->Query.CurrentOcclusionObject) {
      /* update count of 'passed' fragments */
      struct gl_query_object *q = ctx->Query.CurrentOcclusionObject;
      GLuint i;
      for (i = 0; i < span->end; i++)
         q->Result += span->array->mask[i];
   }
#endif

   /* We had to wait until now to check for glColorMask(0,0,0,0) because of
    * the occlusion test.
    */
   if (colorMask == 0x0) {
      goto end;
   }

   /* If we were able to defer fragment color computation to now, there's
    * a good chance that many fragments will have already been killed by
    * Z/stencil testing.
    */
   if (shaderOrTexture && swrast->_DeferredTexture) {
      shade_texture_span(ctx, span);
   }

#if CHAN_BITS == 32
   if ((span->arrayAttribs & FRAG_BIT_COL0) == 0) {
      interpolate_active_attribs(ctx, span, FRAG_BIT_COL0);
   }
#else
   if ((span->arrayMask & SPAN_RGBA) == 0) {
      interpolate_int_colors(ctx, span);
   }
#endif

   ASSERT(span->arrayMask & SPAN_RGBA);

   if (!shader) {
      /* Add base and specular colors */
      if (ctx->Fog.ColorSumEnabled ||
          (ctx->Light.Enabled &&
           ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
         add_specular(ctx, span);
      }
   }

   /* Fog */
   if (swrast->_FogEnabled) {
      _swrast_fog_rgba_span(ctx, span);
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      apply_aa_coverage(span);
   }

   /* Clamp color/alpha values over the range [0.0, 1.0] before storage */
   if (ctx->Color.ClampFragmentColor == GL_TRUE &&
       span->array->ChanType == GL_FLOAT) {
      clamp_colors(span);
   }

   /*
    * Write to renderbuffers
    */
   {
      const GLuint numBuffers = fb->_NumColorDrawBuffers;
      const GLboolean multiFragOutputs = numBuffers > 1;
      GLuint buf;

      for (buf = 0; buf < numBuffers; buf++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[buf];

         /* color[fragOutput] will be written to buffer[buf] */

         if (rb) {
            GLchan rgbaSave[MAX_WIDTH][4];
            const GLuint fragOutput = multiFragOutputs ? buf : 0;

            if (rb->DataType != span->array->ChanType || fragOutput > 0) {
               convert_color_type(span, rb->DataType, fragOutput);
            }

            if (!multiFragOutputs && numBuffers > 1) {
               /* save colors for second, third renderbuffer writes */
               _mesa_memcpy(rgbaSave, span->array->rgba,
                            4 * span->end * sizeof(GLchan));
            }

            ASSERT(rb->_BaseFormat == GL_RGBA || rb->_BaseFormat == GL_RGB);

            if (ctx->Color._LogicOpEnabled) {
               _swrast_logicop_rgba_span(ctx, rb, span);
            }
            else if (ctx->Color.BlendEnabled) {
               _swrast_blend_span(ctx, rb, span);
            }

            if (colorMask != 0xffffffff) {
               _swrast_mask_rgba_span(ctx, rb, span);
            }

            if (span->arrayMask & SPAN_XY) {
               /* array of pixel coords */
               ASSERT(rb->PutValues);
               rb->PutValues(ctx, rb, span->end,
                             span->array->x, span->array->y,
                             span->array->rgba, span->array->mask);
            }
            else {
               /* horizontal run of pixels */
               ASSERT(rb->PutRow);
               rb->PutRow(ctx, rb, span->end, span->x, span->y,
                          span->array->rgba,
                          span->writeAll ? NULL: span->array->mask);
            }

            if (!multiFragOutputs && numBuffers > 1) {
               /* restore original span values */
               _mesa_memcpy(span->array->rgba, rgbaSave,
                            4 * span->end * sizeof(GLchan));
            }

         } /* if rb */
      } /* for buf */
   }

end:
   /* restore these values before returning */
   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
   span->arrayAttribs = origArrayAttribs;
   span->array->ChanType = origChanType;
   span->array->rgba = origRgba;
}


/**
 * Read RGBA pixels from a renderbuffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 * \param dstType  datatype for returned colors
 * \param rgba  the returned colors
 */
void
_swrast_read_rgba_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y, GLenum dstType,
                        GLvoid *rgba)
{
   const GLint bufWidth = (GLint) rb->Width;
   const GLint bufHeight = (GLint) rb->Height;

   if (y < 0 || y >= bufHeight || x + (GLint) n < 0 || x >= bufWidth) {
      /* completely above, below, or right */
      /* XXX maybe leave rgba values undefined? */
      _mesa_bzero(rgba, 4 * n * sizeof(GLchan));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clipping */
         skip = -x;
         length = (GLint) n - skip;
         if (length < 0) {
            /* completely left of window */
            return;
         }
         if (length > bufWidth) {
            length = bufWidth;
         }
      }
      else if ((GLint) (x + n) > bufWidth) {
         /* right edge clipping */
         skip = 0;
         length = bufWidth - x;
         if (length < 0) {
            /* completely to right of window */
            return;
         }
      }
      else {
         /* no clipping */
         skip = 0;
         length = (GLint) n;
      }

      ASSERT(rb);
      ASSERT(rb->GetRow);
      ASSERT(rb->_BaseFormat == GL_RGB || rb->_BaseFormat == GL_RGBA);

      if (rb->DataType == dstType) {
         rb->GetRow(ctx, rb, length, x + skip, y,
                    (GLubyte *) rgba + skip * RGBA_PIXEL_SIZE(rb->DataType));
      }
      else {
         GLuint temp[MAX_WIDTH * 4];
         rb->GetRow(ctx, rb, length, x + skip, y, temp);
         _mesa_convert_colors(rb->DataType, temp,
                   dstType, (GLubyte *) rgba + skip * RGBA_PIXEL_SIZE(dstType),
                   length, NULL);
      }
   }
}


/**
 * Read CI pixels from a renderbuffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void
_swrast_read_index_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                         GLuint n, GLint x, GLint y, GLuint index[] )
{
   const GLint bufWidth = (GLint) rb->Width;
   const GLint bufHeight = (GLint) rb->Height;

   if (y < 0 || y >= bufHeight || x + (GLint) n < 0 || x >= bufWidth) {
      /* completely above, below, or right */
      _mesa_bzero(index, n * sizeof(GLuint));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clipping */
         skip = -x;
         length = (GLint) n - skip;
         if (length < 0) {
            /* completely left of window */
            return;
         }
         if (length > bufWidth) {
            length = bufWidth;
         }
      }
      else if ((GLint) (x + n) > bufWidth) {
         /* right edge clipping */
         skip = 0;
         length = bufWidth - x;
         if (length < 0) {
            /* completely to right of window */
            return;
         }
      }
      else {
         /* no clipping */
         skip = 0;
         length = (GLint) n;
      }

      ASSERT(rb->GetRow);
      ASSERT(rb->_BaseFormat == GL_COLOR_INDEX);

      if (rb->DataType == GL_UNSIGNED_BYTE) {
         GLubyte index8[MAX_WIDTH];
         GLint i;
         rb->GetRow(ctx, rb, length, x + skip, y, index8);
         for (i = 0; i < length; i++)
            index[skip + i] = index8[i];
      }
      else if (rb->DataType == GL_UNSIGNED_SHORT) {
         GLushort index16[MAX_WIDTH];
         GLint i;
         rb->GetRow(ctx, rb, length, x + skip, y, index16);
         for (i = 0; i < length; i++)
            index[skip + i] = index16[i];
      }
      else if (rb->DataType == GL_UNSIGNED_INT) {
         rb->GetRow(ctx, rb, length, x + skip, y, index + skip);
      }
   }
}


/**
 * Wrapper for gl_renderbuffer::GetValues() which does clipping to avoid
 * reading values outside the buffer bounds.
 * We can use this for reading any format/type of renderbuffer.
 * \param valueSize is the size in bytes of each value (pixel) put into the
 *                  values array.
 */
void
_swrast_get_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, const GLint x[], const GLint y[],
                   void *values, GLuint valueSize)
{
   GLuint i, inCount = 0, inStart = 0;

   for (i = 0; i < count; i++) {
      if (x[i] >= 0 && y[i] >= 0 &&
	  x[i] < (GLint) rb->Width && y[i] < (GLint) rb->Height) {
         /* inside */
         if (inCount == 0)
            inStart = i;
         inCount++;
      }
      else {
         if (inCount > 0) {
            /* read [inStart, inStart + inCount) */
            rb->GetValues(ctx, rb, inCount, x + inStart, y + inStart,
                          (GLubyte *) values + inStart * valueSize);
            inCount = 0;
         }
      }
   }
   if (inCount > 0) {
      /* read last values */
      rb->GetValues(ctx, rb, inCount, x + inStart, y + inStart,
                    (GLubyte *) values + inStart * valueSize);
   }
}


/**
 * Wrapper for gl_renderbuffer::PutRow() which does clipping.
 * \param valueSize  size of each value (pixel) in bytes
 */
void
_swrast_put_row(GLcontext *ctx, struct gl_renderbuffer *rb,
                GLuint count, GLint x, GLint y,
                const GLvoid *values, GLuint valueSize)
{
   GLint skip = 0;

   if (y < 0 || y >= (GLint) rb->Height)
      return; /* above or below */

   if (x + (GLint) count <= 0 || x >= (GLint) rb->Width)
      return; /* entirely left or right */

   if ((GLint) (x + count) > (GLint) rb->Width) {
      /* right clip */
      GLint clip = x + count - rb->Width;
      count -= clip;
   }

   if (x < 0) {
      /* left clip */
      skip = -x;
      x = 0;
      count -= skip;
   }

   rb->PutRow(ctx, rb, count, x, y,
              (const GLubyte *) values + skip * valueSize, NULL);
}


/**
 * Wrapper for gl_renderbuffer::GetRow() which does clipping.
 * \param valueSize  size of each value (pixel) in bytes
 */
void
_swrast_get_row(GLcontext *ctx, struct gl_renderbuffer *rb,
                GLuint count, GLint x, GLint y,
                GLvoid *values, GLuint valueSize)
{
   GLint skip = 0;

   if (y < 0 || y >= (GLint) rb->Height)
      return; /* above or below */

   if (x + (GLint) count <= 0 || x >= (GLint) rb->Width)
      return; /* entirely left or right */

   if (x + count > rb->Width) {
      /* right clip */
      GLint clip = x + count - rb->Width;
      count -= clip;
   }

   if (x < 0) {
      /* left clip */
      skip = -x;
      x = 0;
      count -= skip;
   }

   rb->GetRow(ctx, rb, count, x, y, (GLubyte *) values + skip * valueSize);
}


/**
 * Get RGBA pixels from the given renderbuffer.  Put the pixel colors into
 * the span's specular color arrays.  The specular color arrays should no
 * longer be needed by time this function is called.
 * Used by blending, logicop and masking functions.
 * \return pointer to the colors we read.
 */
void *
_swrast_get_dest_rgba(GLcontext *ctx, struct gl_renderbuffer *rb,
                      SWspan *span)
{
   const GLuint pixelSize = RGBA_PIXEL_SIZE(span->array->ChanType);
   void *rbPixels;

   /*
    * Point rbPixels to a temporary space (use specular color arrays).
    */
   rbPixels = span->array->attribs[FRAG_ATTRIB_COL1];

   /* Get destination values from renderbuffer */
   if (span->arrayMask & SPAN_XY) {
      _swrast_get_values(ctx, rb, span->end, span->array->x, span->array->y,
                         rbPixels, pixelSize);
   }
   else {
      _swrast_get_row(ctx, rb, span->end, span->x, span->y,
                      rbPixels, pixelSize);
   }

   return rbPixels;
}
