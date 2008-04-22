/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "image.h"

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
 * Init span's Z interpolation values to the RasterPos Z.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_z( GLcontext *ctx, SWspan *span )
{
   const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;
   if (ctx->DrawBuffer->Visual.depthBits <= 16)
      span->z = FloatToFixed(ctx->Current.RasterPos[2] * depthMax + 0.5F);
   else {
      GLfloat tmpf = ctx->Current.RasterPos[2] * depthMax; 
      tmpf = MIN2(tmpf, depthMax);
      span->z = (GLint) tmpf;
   }
   span->zStep = 0;
   span->interpMask |= SPAN_Z;
}


/**
 * Init span's fogcoord interpolation values to the RasterPos fog.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_fog( GLcontext *ctx, SWspan *span )
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
   span->interpMask |= SPAN_FOG;
}


/**
 * Init span's rgba or index interpolation values to the RasterPos color.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_color( GLcontext *ctx, SWspan *span )
{
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
   }
   else {
      span->index = FloatToFixed(ctx->Current.RasterIndex);
      span->indexStep = 0;
      span->interpMask |= SPAN_INDEX;
   }
}


/**
 * Set the span's secondary color info to the current raster position's
 * secondary color, when needed (lighting enabled or colorsum enabled).
 */
void
_swrast_span_default_secondary_color(GLcontext *ctx, SWspan *span)
{
   if (ctx->Visual.rgbMode && (ctx->Light.Enabled || ctx->Fog.ColorSumEnabled))
   {
      GLchan r, g, b, a;
      UNCLAMPED_FLOAT_TO_CHAN(r, ctx->Current.RasterSecondaryColor[0]);
      UNCLAMPED_FLOAT_TO_CHAN(g, ctx->Current.RasterSecondaryColor[1]);
      UNCLAMPED_FLOAT_TO_CHAN(b, ctx->Current.RasterSecondaryColor[2]);
      UNCLAMPED_FLOAT_TO_CHAN(a, ctx->Current.RasterSecondaryColor[3]);
#if CHAN_TYPE == GL_FLOAT
      span->specRed = r;
      span->specGreen = g;
      span->specBlue = b;
      /*span->specAlpha = a;*/
#else
      span->specRed   = IntToFixed(r);
      span->specGreen = IntToFixed(g);
      span->specBlue  = IntToFixed(b);
      /*span->specAlpha = IntToFixed(a);*/
#endif
      span->specRedStep = 0;
      span->specGreenStep = 0;
      span->specBlueStep = 0;
      /*span->specAlphaStep = 0;*/
      span->interpMask |= SPAN_SPEC;
   }
}


/**
 * Init span's texcoord interpolation values to the RasterPos texcoords.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_texcoords( GLcontext *ctx, SWspan *span )
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
   span->interpMask |= SPAN_TEXTURE;
}


/**
 * Interpolate primary colors to fill in the span->array->color array.
 */
static INLINE void
interpolate_colors(SWspan *span)
{
   const GLuint n = span->end;
   GLuint i;

   ASSERT((span->interpMask & SPAN_RGBA)  &&
          !(span->arrayMask & SPAN_RGBA));

   switch (span->array->ChanType) {
#if CHAN_BITS != 32
   case GL_UNSIGNED_BYTE:
      {
         GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
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
         GLushort (*rgba)[4] = span->array->color.sz2.rgba;
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
            GLushort (*rgba)[4] = span->array->color.sz2.rgba;
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
      {
         GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
         GLfloat r, g, b, a, dr, dg, db, da;
         r = span->red;
         g = span->green;
         b = span->blue;
         a = span->alpha;
         if (span->interpMask & SPAN_FLAT) {
            dr = dg = db = da = 0.0;
         }
         else {
            dr = span->redStep;
            dg = span->greenStep;
            db = span->blueStep;
            da = span->alphaStep;
         }
         for (i = 0; i < n; i++) {
            rgba[i][RCOMP] = r;
            rgba[i][GCOMP] = g;
            rgba[i][BCOMP] = b;
            rgba[i][ACOMP] = a;
            r += dr;
            g += dg;
            b += db;
            a += da;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "bad datatype in interpolate_colors");
   }
   span->arrayMask |= SPAN_RGBA;
}


/**
 * Interpolate specular/secondary colors.
 */
static INLINE void
interpolate_specular(SWspan *span)
{
   const GLuint n = span->end;
   GLuint i;

   switch (span->array->ChanType) {
#if CHAN_BITS != 32
   case GL_UNSIGNED_BYTE:
      {
         GLubyte (*spec)[4] = span->array->color.sz1.spec;
         if (span->interpMask & SPAN_FLAT) {
            GLubyte color[4];
            color[RCOMP] = FixedToInt(span->specRed);
            color[GCOMP] = FixedToInt(span->specGreen);
            color[BCOMP] = FixedToInt(span->specBlue);
            color[ACOMP] = 0;
            for (i = 0; i < n; i++) {
               COPY_4UBV(spec[i], color);
            }
         }
         else {
            GLfixed r = span->specRed;
            GLfixed g = span->specGreen;
            GLfixed b = span->specBlue;
            GLint dr = span->specRedStep;
            GLint dg = span->specGreenStep;
            GLint db = span->specBlueStep;
            for (i = 0; i < n; i++) {
               spec[i][RCOMP] = CLAMP(FixedToChan(r), 0, 255);
               spec[i][GCOMP] = CLAMP(FixedToChan(g), 0, 255);
               spec[i][BCOMP] = CLAMP(FixedToChan(b), 0, 255);
               spec[i][ACOMP] = 0;
               r += dr;
               g += dg;
               b += db;
            }
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort (*spec)[4] = span->array->color.sz2.spec;
         if (span->interpMask & SPAN_FLAT) {
            GLushort color[4];
            color[RCOMP] = FixedToInt(span->specRed);
            color[GCOMP] = FixedToInt(span->specGreen);
            color[BCOMP] = FixedToInt(span->specBlue);
            color[ACOMP] = 0;
            for (i = 0; i < n; i++) {
               COPY_4V(spec[i], color);
            }
         }
         else {
            GLfixed r = FloatToFixed(span->specRed);
            GLfixed g = FloatToFixed(span->specGreen);
            GLfixed b = FloatToFixed(span->specBlue);
            GLint dr = FloatToFixed(span->specRedStep);
            GLint dg = FloatToFixed(span->specGreenStep);
            GLint db = FloatToFixed(span->specBlueStep);
            for (i = 0; i < n; i++) {
               spec[i][RCOMP] = FixedToInt(r);
               spec[i][GCOMP] = FixedToInt(g);
               spec[i][BCOMP] = FixedToInt(b);
               spec[i][ACOMP] = 0;
               r += dr;
               g += dg;
               b += db;
            }
         }
      }
      break;
#endif
   case GL_FLOAT:
      {
         GLfloat (*spec)[4] = span->array->attribs[FRAG_ATTRIB_COL1];
#if CHAN_BITS <= 16
         GLfloat r = CHAN_TO_FLOAT(FixedToChan(span->specRed));
         GLfloat g = CHAN_TO_FLOAT(FixedToChan(span->specGreen));
         GLfloat b = CHAN_TO_FLOAT(FixedToChan(span->specBlue));
#else
         GLfloat r = span->specRed;
         GLfloat g = span->specGreen;
         GLfloat b = span->specBlue;
#endif
         GLfloat dr, dg, db;
         if (span->interpMask & SPAN_FLAT) {
            dr = dg = db = 0.0;
         }
         else {
#if CHAN_BITS <= 16
            dr = CHAN_TO_FLOAT(FixedToChan(span->specRedStep));
            dg = CHAN_TO_FLOAT(FixedToChan(span->specGreenStep));
            db = CHAN_TO_FLOAT(FixedToChan(span->specBlueStep));
#else
            dr = span->specRedStep;
            dg = span->specGreenStep;
            db = span->specBlueStep;
#endif
         }
         for (i = 0; i < n; i++) {
            spec[i][RCOMP] = r;
            spec[i][GCOMP] = g;
            spec[i][BCOMP] = b;
            spec[i][ACOMP] = 0.0F;
            r += dr;
            g += dg;
            b += db;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "bad datatype in interpolate_specular");
   }
   span->arrayMask |= SPAN_SPEC;
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
   ASSERT((span->interpMask & SPAN_INDEX)  &&
	  !(span->arrayMask & SPAN_INDEX));

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


/* Fill in the span.array.fog values from the interpolation values */
static INLINE void
interpolate_fog(const GLcontext *ctx, SWspan *span)
{
   GLfloat (*fog)[4] = span->array->attribs[FRAG_ATTRIB_FOGC];
   const GLfloat fogStep = span->attrStepX[FRAG_ATTRIB_FOGC][0];
   GLfloat fogCoord = span->attrStart[FRAG_ATTRIB_FOGC][0];
   const GLuint haveW = (span->interpMask & SPAN_W);
   const GLfloat wStep = haveW ? span->attrStepX[FRAG_ATTRIB_WPOS][3] : 0.0F;
   GLfloat w = haveW ? span->attrStart[FRAG_ATTRIB_WPOS][3] : 1.0F;
   GLuint i;
   for (i = 0; i < span->end; i++) {
      fog[i][0] = fogCoord / w;
      fogCoord += fogStep;
      w += wStep;
   }
   span->arrayMask |= SPAN_FOG;
}


/* Fill in the span.zArray array from the interpolation values */
void
_swrast_span_interpolate_z( const GLcontext *ctx, SWspan *span )
{
   const GLuint n = span->end;
   GLuint i;

   ASSERT((span->interpMask & SPAN_Z)  &&
	  !(span->arrayMask & SPAN_Z));

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


/*
 * This the ideal solution, as given in the OpenGL spec.
 */
#if 0
static GLfloat
compute_lambda(GLfloat dsdx, GLfloat dsdy, GLfloat dtdx, GLfloat dtdy,
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
#endif


/*
 * This is a faster approximation
 */
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


/**
 * Fill in the span.texcoords array from the interpolation values.
 * Note: in the places where we divide by Q (or mult by invQ) we're
 * really doing two things: perspective correction and texcoord
 * projection.  Remember, for texcoord (s,t,r,q) we need to index
 * texels with (s/q, t/q, r/q).
 * If we're using a fragment program, we never do the division
 * for texcoord projection.  That's done by the TXP instruction
 * or user-written code.
 */
static void
interpolate_texcoords(GLcontext *ctx, SWspan *span)
{
   const GLuint maxUnit
      = (ctx->Texture._EnabledCoordUnits > 1) ? ctx->Const.MaxTextureUnits : 1;
   GLuint u;

   ASSERT(span->interpMask & SPAN_TEXTURE);
   ASSERT(!(span->arrayMask & SPAN_TEXTURE));

   span->arrayMask |= SPAN_TEXTURE;

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
 * Fill in the arrays->attribs[FRAG_ATTRIB_VARx] arrays from the
 * interpolation values.
 * XXX since interpolants/arrays are getting uniformed, we might merge
 * this with interpolate_texcoords(), interpolate_Fog(), etc. someday.
 */
static INLINE void
interpolate_varying(GLcontext *ctx, SWspan *span)
{
   GLuint var;
   const GLbitfield inputsUsed = ctx->FragmentProgram._Current->Base.InputsRead;

   ASSERT(span->interpMask & SPAN_VARYING);
   ASSERT(!(span->arrayMask & SPAN_VARYING));

   span->arrayMask |= SPAN_VARYING;

   for (var = 0; var < MAX_VARYING; var++) {
      if (inputsUsed & FRAG_BIT_VAR(var)) {
         const GLuint attr = FRAG_ATTRIB_VAR0 + var;
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
            GLfloat invW = 1.0f / w;
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
      }
   }
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

   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(span->primitive == GL_POINT  ||  span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON  ||  span->primitive == GL_BITMAP);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_INDEX);
   ASSERT((span->interpMask & span->arrayMask) == 0);

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
   if (ctx->Depth.BoundsTest && ctx->DrawBuffer->Visual.depthBits > 0) {
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
            assert(span->array->x[i] >= ctx->DrawBuffer->_Xmin);
            assert(span->array->x[i] < ctx->DrawBuffer->_Xmax);
            assert(span->array->y[i] >= ctx->DrawBuffer->_Ymin);
            assert(span->array->y[i] < ctx->DrawBuffer->_Ymax);
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
      if (span->interpMask & SPAN_Z)
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
   if (ctx->Color.DrawBuffer == GL_NONE || ctx->Color.IndexMask == 0) {
      /* write no pixels */
      span->arrayMask = origArrayMask;
      return;
   }

   /* Interpolate the color indexes if needed */
   if (swrast->_FogEnabled ||
       ctx->Color.IndexLogicOpEnabled ||
       ctx->Color.IndexMask != 0xffffffff ||
       (span->arrayMask & SPAN_COVERAGE)) {
      if (span->interpMask & SPAN_INDEX) {
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
      struct gl_framebuffer *fb = ctx->DrawBuffer;
      const GLuint output = 0; /* only frag progs can write to other outputs */
      const GLuint numDrawBuffers = fb->_NumColorDrawBuffers[output];
      GLuint indexSave[MAX_WIDTH];
      GLuint buf;

      if (numDrawBuffers > 1) {
         /* save indexes for second, third renderbuffer writes */
         _mesa_memcpy(indexSave, span->array->index,
                      span->end * sizeof(indexSave[0]));
      }

      for (buf = 0; buf < fb->_NumColorDrawBuffers[output]; buf++) {
         struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[output][buf];
         ASSERT(rb->_BaseFormat == GL_COLOR_INDEX);

         if (ctx->Color.IndexLogicOpEnabled) {
            _swrast_logicop_ci_span(ctx, rb, span);
         }

         if (ctx->Color.IndexMask != 0xffffffff) {
            _swrast_mask_ci_span(ctx, rb, span);
         }

         if ((span->interpMask & SPAN_INDEX) && span->indexStep == 0) {
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

         if (buf + 1 < numDrawBuffers) {
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
 * Add specular color to base color.  This is used only when
 * GL_LIGHT_MODEL_COLOR_CONTROL = GL_SEPARATE_SPECULAR_COLOR.
 */
static INLINE void
add_specular(GLcontext *ctx, SWspan *span)
{
   switch (span->array->ChanType) {
   case GL_UNSIGNED_BYTE:
      {
         GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
         GLubyte (*spec)[4] = span->array->color.sz1.spec;
         GLuint i;
         for (i = 0; i < span->end; i++) {
            GLint r = rgba[i][RCOMP] + spec[i][RCOMP];
            GLint g = rgba[i][GCOMP] + spec[i][GCOMP];
            GLint b = rgba[i][BCOMP] + spec[i][BCOMP];
            GLint a = rgba[i][ACOMP] + spec[i][ACOMP];
            rgba[i][RCOMP] = MIN2(r, 255);
            rgba[i][GCOMP] = MIN2(g, 255);
            rgba[i][BCOMP] = MIN2(b, 255);
            rgba[i][ACOMP] = MIN2(a, 255);
         }
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         GLushort (*rgba)[4] = span->array->color.sz2.rgba;
         GLushort (*spec)[4] = span->array->color.sz2.spec;
         GLuint i;
         for (i = 0; i < span->end; i++) {
            GLint r = rgba[i][RCOMP] + spec[i][RCOMP];
            GLint g = rgba[i][GCOMP] + spec[i][GCOMP];
            GLint b = rgba[i][BCOMP] + spec[i][BCOMP];
            GLint a = rgba[i][ACOMP] + spec[i][ACOMP];
            rgba[i][RCOMP] = MIN2(r, 65535);
            rgba[i][GCOMP] = MIN2(g, 65535);
            rgba[i][BCOMP] = MIN2(b, 65535);
            rgba[i][ACOMP] = MIN2(a, 65535);
         }
      }
      break;
   case GL_FLOAT:
      {
         GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
         GLfloat (*spec)[4] = span->array->attribs[FRAG_ATTRIB_COL1];
         GLuint i;
         for (i = 0; i < span->end; i++) {
            rgba[i][RCOMP] += spec[i][RCOMP];
            rgba[i][GCOMP] += spec[i][GCOMP];
            rgba[i][BCOMP] += spec[i][BCOMP];
            rgba[i][ACOMP] += spec[i][ACOMP];
         }
      }
      break;
   default:
      _mesa_problem(ctx, "Invalid datatype in add_specular");
   }
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
      GLubyte (*rgba)[4] = span->array->color.sz1.rgba;
      for (i = 0; i < span->end; i++) {
         const GLfloat a = rgba[i][ACOMP] * coverage[i];
         rgba[i][ACOMP] = (GLubyte) CLAMP(a, 0.0, 255.0);
         ASSERT(coverage[i] >= 0.0);
         ASSERT(coverage[i] <= 1.0);
      }
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      GLushort (*rgba)[4] = span->array->color.sz2.rgba;
      for (i = 0; i < span->end; i++) {
         const GLfloat a = rgba[i][ACOMP] * coverage[i];
         rgba[i][ACOMP] = (GLushort) CLAMP(a, 0.0, 65535.0);
      }
   }
   else {
      GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL0];
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * coverage[i];
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
 * The only way 'output' can be greater than one is when we have a fragment
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
      src = span->array->color.sz1.rgba;
   }
   else {
      ASSERT(span->array->ChanType == GL_UNSIGNED_SHORT);
      src = span->array->color.sz2.rgba;
   }

   if (newType == GL_UNSIGNED_BYTE) {
      dst = span->array->color.sz1.rgba;
   }
   else if (newType == GL_UNSIGNED_SHORT) {
      dst = span->array->color.sz2.rgba;
   }
   else {
      dst = span->array->attribs[FRAG_ATTRIB_COL0];
   }

   _mesa_convert_colors(span->array->ChanType, src,
                        newType, dst,
                        span->end, span->array->mask);

   span->array->ChanType = newType;
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

   if ((inputsRead & FRAG_BIT_COL0) && (span->interpMask & SPAN_RGBA))
      interpolate_colors(span);

   if (ctx->Texture._EnabledCoordUnits && (span->interpMask & SPAN_TEXTURE))
      interpolate_texcoords(ctx, span);

   if (ctx->FragmentProgram._Current ||
       ctx->ATIFragmentShader._Enabled) {
      /* use float colors if running a fragment program or shader */
      const GLenum oldType = span->array->ChanType;
      const GLenum newType = GL_FLOAT;

      if ((inputsRead & FRAG_BIT_COL0) && (oldType != newType)) {
         GLvoid *src = (oldType == GL_UNSIGNED_BYTE)
            ? (GLvoid *) span->array->color.sz1.rgba
            : (GLvoid *) span->array->color.sz2.rgba;
         assert(span->arrayMask & SPAN_RGBA);
         _mesa_convert_colors(oldType, src,
                              newType, span->array->attribs[FRAG_ATTRIB_COL0],
                              span->end, span->array->mask);
      }
      span->array->ChanType = newType;

      /* fragment programs/shaders may need specular, fog and Z coords */
      if ((inputsRead & FRAG_BIT_COL1) && (span->interpMask & SPAN_SPEC))
         interpolate_specular(span);

      if ((inputsRead & FRAG_BIT_FOGC) && (span->interpMask & SPAN_FOG))
         interpolate_fog(ctx, span);

      if (span->interpMask & SPAN_Z)
         _swrast_span_interpolate_z (ctx, span);

      if ((inputsRead >= FRAG_BIT_VAR0) && (span->interpMask & SPAN_VARYING))
         interpolate_varying(ctx, span);

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
   else if (ctx->Texture._EnabledUnits && (span->arrayMask & SPAN_TEXTURE)) {
      /* conventional texturing */
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
   const GLenum chanType = span->array->ChanType;
   const GLboolean shader = (ctx->FragmentProgram._Current
                             || ctx->ATIFragmentShader._Enabled);
   const GLboolean shaderOrTexture = shader || ctx->Texture._EnabledUnits;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint output;
   GLboolean deferredTexture;

   /*
   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__,
          span->interpMask, span->arrayMask);
   */

   ASSERT(span->primitive == GL_POINT ||
          span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON ||
          span->primitive == GL_BITMAP);
   ASSERT(span->end <= MAX_WIDTH);
   ASSERT((span->interpMask & span->arrayMask) == 0);
   ASSERT((span->interpMask & SPAN_RGBA) ^ (span->arrayMask & SPAN_RGBA));

   /* check for conditions that prevent deferred shading (doing shading
    * after stencil/ztest).
    * XXX move this code into state validation.
    */
   if (ctx->Color.AlphaEnabled) {
      /* alpha test depends on post-texture/shader colors */
      deferredTexture = GL_FALSE;
   }
   else if (shaderOrTexture) {
      if (ctx->FragmentProgram._Current) {
         if (ctx->FragmentProgram._Current->Base.OutputsWritten
             & (1 << FRAG_RESULT_DEPR)) {
            /* Z comes from fragment program/shader */
            deferredTexture = GL_FALSE;
         }
         else if (ctx->Query.CurrentOcclusionObject) {
            /* occlusion query depends on shader discard/kill results */
            deferredTexture = GL_FALSE;
         }
         else {
            deferredTexture = GL_TRUE;
         }
      }
      else {
         /* ATI frag shader or conventional texturing */
         deferredTexture = GL_TRUE;
      }
   }
   else {
      /* no texturing or shadering */
      deferredTexture = GL_FALSE;
   }

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

   /* This is the normal place to compute the resulting fragment color/Z.
    * As an optimization, we try to defer this until after Z/stencil
    * testing in order to try to avoid computing colors that we won't
    * actually need.
    */
   if (shaderOrTexture && !deferredTexture) {
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
      if (span->interpMask & SPAN_Z)
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
   if (deferredTexture) {
      ASSERT(shaderOrTexture);
      shade_texture_span(ctx, span);
   }

   if ((span->arrayMask & SPAN_RGBA) == 0) {
      interpolate_colors(span);
   }

   ASSERT(span->arrayMask & SPAN_RGBA);

   if (!shader) {
      /* Add base and specular colors */
      if (ctx->Fog.ColorSumEnabled ||
          (ctx->Light.Enabled &&
           ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
         if (span->interpMask & SPAN_SPEC) {
            interpolate_specular(span);
         }
         if (span->arrayMask & SPAN_SPEC) {
            add_specular(ctx, span);
         }
         else {
            /* We probably added the base/specular colors during the
             * vertex stage!
             */
         }
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
   /* Loop over color outputs (GL_ARB_draw_buffers) written by frag prog */
   for (output = 0; output < swrast->_NumColorOutputs; output++) {
      if (swrast->_ColorOutputsMask & (1 << output)) {
        const GLuint numDrawBuffers = fb->_NumColorDrawBuffers[output];
        GLchan rgbaSave[MAX_WIDTH][4];
        GLuint buf;

        ASSERT(numDrawBuffers > 0);

        if (fb->_ColorDrawBuffers[output][0]->DataType
            != span->array->ChanType || output > 0) {
           convert_color_type(span,
                              fb->_ColorDrawBuffers[output][0]->DataType,
                              output);
        }

        if (numDrawBuffers > 1) {
           /* save colors for second, third renderbuffer writes */
           _mesa_memcpy(rgbaSave, span->array->rgba,
                        4 * span->end * sizeof(GLchan));
        }

        /* Loop over renderbuffers (i.e. GL_FRONT_AND_BACK) */
        for (buf = 0; buf < numDrawBuffers; buf++) {
           struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[output][buf];
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

           if (buf + 1 < numDrawBuffers) {
              /* restore original span values */
              _mesa_memcpy(span->array->rgba, rgbaSave,
                           4 * span->end * sizeof(GLchan));
           }
        } /* for buf */
      } /* if output is written to */
   } /* for output */

end:
   /* restore these values before returning */
   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
   span->array->ChanType = chanType;
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
    * Determine pixel size (in bytes).
    * Point rbPixels to a temporary space (use specular color arrays).
    */
   if (span->array->ChanType == GL_UNSIGNED_BYTE) {
      rbPixels = span->array->color.sz1.spec;
   }
   else if (span->array->ChanType == GL_UNSIGNED_SHORT) {
      rbPixels = span->array->color.sz2.spec;
   }
   else {
      rbPixels = span->array->attribs[FRAG_ATTRIB_COL1];
   }

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
