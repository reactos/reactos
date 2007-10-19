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

#include "s_atifragshader.h"
#include "s_alpha.h"
#include "s_blend.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_fog.h"
#include "s_logic.h"
#include "s_masking.h"
#include "s_nvfragprog.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texture.h"


/**
 * Init span's Z interpolation values to the RasterPos Z.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_z( GLcontext *ctx, struct sw_span *span )
{
   const GLfloat depthMax = ctx->DrawBuffer->_DepthMaxF;
   if (ctx->Visual.depthBits <= 16)
      span->z = FloatToFixed(ctx->Current.RasterPos[2] * depthMax + 0.5F);
   else
      span->z = (GLint) (ctx->Current.RasterPos[2] * depthMax + 0.5F);
   span->zStep = 0;
   span->interpMask |= SPAN_Z;
}


/**
 * Init span's fog interpolation values to the RasterPos fog.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_fog( GLcontext *ctx, struct sw_span *span )
{
   span->fog = _swrast_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
   span->fogStep = span->dfogdx = span->dfogdy = 0.0F;
   span->interpMask |= SPAN_FOG;
}


/**
 * Init span's rgba or index interpolation values to the RasterPos color.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_color( GLcontext *ctx, struct sw_span *span )
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
 * Init span's texcoord interpolation values to the RasterPos texcoords.
 * Used during setup for glDraw/CopyPixels.
 */
void
_swrast_span_default_texcoords( GLcontext *ctx, struct sw_span *span )
{
   GLuint i;
   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      const GLfloat *tc = ctx->Current.RasterTexCoords[i];
      if (ctx->FragmentProgram._Active || ctx->ATIFragmentShader._Enabled) {
         COPY_4V(span->tex[i], tc);
      }
      else if (tc[3] > 0.0F) {
         /* use (s/q, t/q, r/q, 1) */
         span->tex[i][0] = tc[0] / tc[3];
         span->tex[i][1] = tc[1] / tc[3];
         span->tex[i][2] = tc[2] / tc[3];
         span->tex[i][3] = 1.0;
      }
      else {
         ASSIGN_4V(span->tex[i], 0.0F, 0.0F, 0.0F, 1.0F);
      }
      ASSIGN_4V(span->texStepX[i], 0.0F, 0.0F, 0.0F, 0.0F);
      ASSIGN_4V(span->texStepY[i], 0.0F, 0.0F, 0.0F, 0.0F);
   }
   span->interpMask |= SPAN_TEXTURE;
}


/* Fill in the span.color.rgba array from the interpolation values */
static void
interpolate_colors(GLcontext *ctx, struct sw_span *span)
{
   const GLuint n = span->end;
   GLchan (*rgba)[4] = span->array->rgba;
   GLuint i;
   (void) ctx;

   ASSERT((span->interpMask & SPAN_RGBA)  &&
	  !(span->arrayMask & SPAN_RGBA));

   if (span->interpMask & SPAN_FLAT) {
      /* constant color */
      GLchan color[4];
      color[RCOMP] = FixedToChan(span->red);
      color[GCOMP] = FixedToChan(span->green);
      color[BCOMP] = FixedToChan(span->blue);
      color[ACOMP] = FixedToChan(span->alpha);
      for (i = 0; i < n; i++) {
         COPY_CHAN4(span->array->rgba[i], color);
      }
   }
   else {
      /* interpolate */
#if CHAN_TYPE == GL_FLOAT
      GLfloat r = span->red;
      GLfloat g = span->green;
      GLfloat b = span->blue;
      GLfloat a = span->alpha;
      const GLfloat dr = span->redStep;
      const GLfloat dg = span->greenStep;
      const GLfloat db = span->blueStep;
      const GLfloat da = span->alphaStep;
#else
      GLfixed r = span->red;
      GLfixed g = span->green;
      GLfixed b = span->blue;
      GLfixed a = span->alpha;
      const GLint dr = span->redStep;
      const GLint dg = span->greenStep;
      const GLint db = span->blueStep;
      const GLint da = span->alphaStep;
#endif
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
   span->arrayMask |= SPAN_RGBA;
}


/* Fill in the span.color.index array from the interpolation values */
static void
interpolate_indexes(GLcontext *ctx, struct sw_span *span)
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


/* Fill in the span.->array->spec array from the interpolation values */
static void
interpolate_specular(GLcontext *ctx, struct sw_span *span)
{
   (void) ctx;
   if (span->interpMask & SPAN_FLAT) {
      /* constant color */
      const GLchan r = FixedToChan(span->specRed);
      const GLchan g = FixedToChan(span->specGreen);
      const GLchan b = FixedToChan(span->specBlue);
      GLuint i;
      for (i = 0; i < span->end; i++) {
         span->array->spec[i][RCOMP] = r;
         span->array->spec[i][GCOMP] = g;
         span->array->spec[i][BCOMP] = b;
      }
   }
   else {
      /* interpolate */
#if CHAN_TYPE == GL_FLOAT
      GLfloat r = span->specRed;
      GLfloat g = span->specGreen;
      GLfloat b = span->specBlue;
#else
      GLfixed r = span->specRed;
      GLfixed g = span->specGreen;
      GLfixed b = span->specBlue;
#endif
      GLuint i;
      for (i = 0; i < span->end; i++) {
         span->array->spec[i][RCOMP] = FixedToChan(r);
         span->array->spec[i][GCOMP] = FixedToChan(g);
         span->array->spec[i][BCOMP] = FixedToChan(b);
         r += span->specRedStep;
         g += span->specGreenStep;
         b += span->specBlueStep;
      }
   }
   span->arrayMask |= SPAN_SPEC;
}


/* Fill in the span.array.fog values from the interpolation values */
static void
interpolate_fog(const GLcontext *ctx, struct sw_span *span)
{
   GLfloat *fog = span->array->fog;
   const GLfloat fogStep = span->fogStep;
   GLfloat fogCoord = span->fog;
   const GLuint haveW = (span->interpMask & SPAN_W);
   const GLfloat wStep = haveW ? span->dwdx : 0.0F;
   GLfloat w = haveW ? span->w : 1.0F;
   GLuint i;
   for (i = 0; i < span->end; i++) {
      fog[i] = fogCoord / w;
      fogCoord += fogStep;
      w += wStep;
   }
   span->arrayMask |= SPAN_FOG;
}


/* Fill in the span.zArray array from the interpolation values */
void
_swrast_span_interpolate_z( const GLcontext *ctx, struct sw_span *span )
{
   const GLuint n = span->end;
   GLuint i;

   ASSERT((span->interpMask & SPAN_Z)  &&
	  !(span->arrayMask & SPAN_Z));

   if (ctx->Visual.depthBits <= 16) {
      GLfixed zval = span->z;
      GLdepth *z = span->array->z;
      for (i = 0; i < n; i++) {
         z[i] = FixedToInt(zval);
         zval += span->zStep;
      }
   }
   else {
      /* Deep Z buffer, no fixed->int shift */
      GLfixed zval = span->z;
      GLdepth *z = span->array->z;
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
interpolate_texcoords(GLcontext *ctx, struct sw_span *span)
{
   ASSERT(span->interpMask & SPAN_TEXTURE);
   ASSERT(!(span->arrayMask & SPAN_TEXTURE));

   if (ctx->Texture._EnabledCoordUnits > 1) {
      /* multitexture */
      GLuint u;
      span->arrayMask |= SPAN_TEXTURE;
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (ctx->Texture._EnabledCoordUnits & (1 << u)) {
            const struct gl_texture_object *obj =ctx->Texture.Unit[u]._Current;
            GLfloat texW, texH;
            GLboolean needLambda;
            if (obj) {
               const struct gl_texture_image *img = obj->Image[0][obj->BaseLevel];
               needLambda = (obj->MinFilter != obj->MagFilter)
                  || ctx->FragmentProgram._Active;
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
               GLfloat (*texcoord)[4] = span->array->texcoords[u];
               GLfloat *lambda = span->array->lambda[u];
               const GLfloat dsdx = span->texStepX[u][0];
               const GLfloat dsdy = span->texStepY[u][0];
               const GLfloat dtdx = span->texStepX[u][1];
               const GLfloat dtdy = span->texStepY[u][1];
               const GLfloat drdx = span->texStepX[u][2];
               const GLfloat dqdx = span->texStepX[u][3];
               const GLfloat dqdy = span->texStepY[u][3];
               GLfloat s = span->tex[u][0];
               GLfloat t = span->tex[u][1];
               GLfloat r = span->tex[u][2];
               GLfloat q = span->tex[u][3];
               GLuint i;
               if (ctx->FragmentProgram._Active) {
                  /* do perspective correction but don't divide s, t, r by q */
                  const GLfloat dwdx = span->dwdx;
                  GLfloat w = span->w;
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
               GLfloat (*texcoord)[4] = span->array->texcoords[u];
               GLfloat *lambda = span->array->lambda[u];
               const GLfloat dsdx = span->texStepX[u][0];
               const GLfloat dtdx = span->texStepX[u][1];
               const GLfloat drdx = span->texStepX[u][2];
               const GLfloat dqdx = span->texStepX[u][3];
               GLfloat s = span->tex[u][0];
               GLfloat t = span->tex[u][1];
               GLfloat r = span->tex[u][2];
               GLfloat q = span->tex[u][3];
               GLuint i;
               if (ctx->FragmentProgram._Active) {
                  /* do perspective correction but don't divide s, t, r by q */
                  const GLfloat dwdx = span->dwdx;
                  GLfloat w = span->w;
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
   else {
      /* single texture */
      const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;
      GLfloat texW, texH;
      GLboolean needLambda;
      if (obj) {
         const struct gl_texture_image *img = obj->Image[0][obj->BaseLevel];
         needLambda = (obj->MinFilter != obj->MagFilter)
            || ctx->FragmentProgram._Active;
         texW = (GLfloat) img->WidthScale;
         texH = (GLfloat) img->HeightScale;
      }
      else {
         needLambda = GL_FALSE;
         texW = texH = 1.0;
      }
      span->arrayMask |= SPAN_TEXTURE;
      if (needLambda) {
         /* just texture unit 0, with lambda */
         GLfloat (*texcoord)[4] = span->array->texcoords[0];
         GLfloat *lambda = span->array->lambda[0];
         const GLfloat dsdx = span->texStepX[0][0];
         const GLfloat dsdy = span->texStepY[0][0];
         const GLfloat dtdx = span->texStepX[0][1];
         const GLfloat dtdy = span->texStepY[0][1];
         const GLfloat drdx = span->texStepX[0][2];
         const GLfloat dqdx = span->texStepX[0][3];
         const GLfloat dqdy = span->texStepY[0][3];
         GLfloat s = span->tex[0][0];
         GLfloat t = span->tex[0][1];
         GLfloat r = span->tex[0][2];
         GLfloat q = span->tex[0][3];
         GLuint i;
         if (ctx->FragmentProgram._Active) {
            /* do perspective correction but don't divide s, t, r by q */
            const GLfloat dwdx = span->dwdx;
            GLfloat w = span->w;
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
            /* tex.c */
            for (i = 0; i < span->end; i++) {
               const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
               lambda[i] = _swrast_compute_lambda(dsdx, dsdy, dtdx, dtdy,
                                                dqdx, dqdy, texW, texH,
                                                s, t, q, invQ);
               texcoord[i][0] = s * invQ;
               texcoord[i][1] = t * invQ;
               texcoord[i][2] = r * invQ;
               texcoord[i][3] = q;
               s += dsdx;
               t += dtdx;
               r += drdx;
               q += dqdx;
            }
         }
         span->arrayMask |= SPAN_LAMBDA;
      }
      else {
         /* just texture 0, without lambda */
         GLfloat (*texcoord)[4] = span->array->texcoords[0];
         const GLfloat dsdx = span->texStepX[0][0];
         const GLfloat dtdx = span->texStepX[0][1];
         const GLfloat drdx = span->texStepX[0][2];
         const GLfloat dqdx = span->texStepX[0][3];
         GLfloat s = span->tex[0][0];
         GLfloat t = span->tex[0][1];
         GLfloat r = span->tex[0][2];
         GLfloat q = span->tex[0][3];
         GLuint i;
         if (ctx->FragmentProgram._Active) {
            /* do perspective correction but don't divide s, t, r by q */
            const GLfloat dwdx = span->dwdx;
            GLfloat w = span->w;
            for (i = 0; i < span->end; i++) {
               const GLfloat invW = 1.0F / w;
               texcoord[i][0] = s * invW;
               texcoord[i][1] = t * invW;
               texcoord[i][2] = r * invW;
               texcoord[i][3] = q * invW;
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
               s += dsdx;
               t += dtdx;
               r += drdx;
               q += dqdx;
            }
         }
      }
   }
}


/**
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static void
stipple_polygon_span( GLcontext *ctx, struct sw_span *span )
{
   const GLuint highbit = 0x80000000;
   const GLuint stipple = ctx->PolygonStipple[span->y % 32];
   GLubyte *mask = span->array->mask;
   GLuint i, m;

   ASSERT(ctx->Polygon.StippleFlag);
   ASSERT((span->arrayMask & SPAN_XY) == 0);

   m = highbit >> (GLuint) (span->x % 32);

   for (i = 0; i < span->end; i++) {
      if ((m & stipple) == 0) {
	 mask[i] = 0;
      }
      m = m >> 1;
      if (m == 0) {
         m = highbit;
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
static GLuint
clip_span( GLcontext *ctx, struct sw_span *span )
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
_swrast_write_index_span( GLcontext *ctx, struct sw_span *span)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLuint output = 0;
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;
   GLuint buf;

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
   if (ctx->Depth.BoundsTest && ctx->Visual.depthBits > 0) {
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

   /* if we get here, something passed the depth test */
   if (ctx->Depth.OcclusionTest) {
      ctx->OcclusionResult = GL_TRUE;
   }

#if FEATURE_ARB_occlusion_query
   if (ctx->Occlusion.Active) {
      /* update count of 'passed' fragments */
      GLuint i;
      for (i = 0; i < span->end; i++)
         ctx->Occlusion.PassedCounter += span->array->mask[i];
   }
#endif

   /* we have to wait until after occlusion to do this test */
   if (ctx->Color.DrawBuffer == GL_NONE || ctx->Color.IndexMask == 0) {
      /* write no pixels */
      span->arrayMask = origArrayMask;
      return;
   }

   /* Interpolate the color indexes if needed */
   if (ctx->Fog.Enabled ||
       ctx->Color.IndexLogicOpEnabled ||
       ctx->Color.IndexMask != 0xffffffff ||
       (span->arrayMask & SPAN_COVERAGE)) {
      if (span->interpMask & SPAN_INDEX) {
         interpolate_indexes(ctx, span);
      }
   }

   /* Fog */
   if (ctx->Fog.Enabled) {
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

   /* Loop over drawing buffers */
   for (buf = 0; buf < fb->_NumColorDrawBuffers[output]; buf++) {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[output][buf];
      GLuint indexTemp[MAX_WIDTH], *index32;

      ASSERT(rb->_BaseFormat == GL_COLOR_INDEX);

      if (ctx->Color.IndexLogicOpEnabled ||
          ctx->Color.IndexMask != 0xffffffff) {
         /* make copy of incoming indexes */
         MEMCPY(indexTemp, span->array->index, span->end * sizeof(GLuint));

         if (ctx->Color.IndexLogicOpEnabled) {
            _swrast_logicop_ci_span(ctx, rb, span, indexTemp);
         }

         if (ctx->Color.IndexMask != 0xffffffff) {
            _swrast_mask_ci_span(ctx, rb, span, indexTemp);
         }
         index32 = indexTemp;
      }
      else {
         index32 = span->array->index;
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
               index8[k] = (GLubyte) index32[k];
            }
            values = index8;
         }
         else if (rb->DataType == GL_UNSIGNED_SHORT) {
            GLuint k;
            for (k = 0; k < span->end; k++) {
               index16[k] = (GLushort) index32[k];
            }
            values = index16;
         }
         else {
            ASSERT(rb->DataType == GL_UNSIGNED_INT);
            values = index32;
         }

         if (span->arrayMask & SPAN_XY) {
            rb->PutValues(ctx, rb, span->end, span->array->x, span->array->y,
                          values, span->array->mask);
         }
         else {
            rb->PutRow(ctx, rb, span->end, span->x, span->y,
                       values, span->array->mask);
         }
      }
   }

#if OLD_RENDERBUFFER
   /* restore default dest buffer */
   _swrast_use_draw_buffer(ctx);
#endif

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}


/**
 * Add specular color to base color.  This is used only when
 * GL_LIGHT_MODEL_COLOR_CONTROL = GL_SEPARATE_SPECULAR_COLOR.
 */
static void
add_colors(GLuint n, GLchan rgba[][4], GLchan specular[][4] )
{
   GLuint i;
   for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
      /* no clamping */
      rgba[i][RCOMP] += specular[i][RCOMP];
      rgba[i][GCOMP] += specular[i][GCOMP];
      rgba[i][BCOMP] += specular[i][BCOMP];
#else
      GLint r = rgba[i][RCOMP] + specular[i][RCOMP];
      GLint g = rgba[i][GCOMP] + specular[i][GCOMP];
      GLint b = rgba[i][BCOMP] + specular[i][BCOMP];
      rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
      rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
      rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
   }
}


/**
 * XXX merge this code into the _swrast_write_rgba_span() routine!
 *
 * Draw to more than one RGBA color buffer (or none).
 * All fragment operations, up to (but not) blending/logicop should
 * have been done first.
 */
static void
multi_write_rgba_span( GLcontext *ctx, struct sw_span *span )
{
#if OLD_RENDERBUFFER
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
#endif
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLuint output = 0;
   GLuint i;

   ASSERT(span->end < MAX_WIDTH);
   ASSERT(colorMask != 0x0);

   for (i = 0; i < fb->_NumColorDrawBuffers[output]; i++) {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[output][i];
      GLchan rgbaTmp[MAX_WIDTH][4];

#if OLD_RENDERBUFFER
      /* obsolete code */
      GLuint bufferBit = fb->_ColorDrawBit[output][i];
      /* Set the current read/draw buffer */
      swrast->CurrentBufferBit = bufferBit;
      if (swrast->Driver.SetBuffer)
         (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, bufferBit);
#endif

      /* make copy of incoming colors */
      MEMCPY( rgbaTmp, span->array->rgba, 4 * span->end * sizeof(GLchan) );

      if (ctx->Color._LogicOpEnabled) {
         _swrast_logicop_rgba_span(ctx, rb, span, rgbaTmp);
      }
      else if (ctx->Color.BlendEnabled) {
         _swrast_blend_span(ctx, rb, span, rgbaTmp);
      }

      if (colorMask != 0xffffffff) {
         _swrast_mask_rgba_span(ctx, rb, span, rgbaTmp);
      }

      if (span->arrayMask & SPAN_XY) {
         /* array of pixel coords */
         ASSERT(rb->PutValues);
         rb->PutValues(ctx, rb, span->end, span->array->x,
                       span->array->y, rgbaTmp, span->array->mask);
      }
      else {
         /* horizontal run of pixels */
         ASSERT(rb->PutRow);
         rb->PutRow(ctx, rb, span->end, span->x, span->y, rgbaTmp,
                    span->array->mask);
      }
   }

#if OLD_RENDERBUFFER
   /* restore default dest buffer */
   _swrast_use_draw_buffer(ctx);
#endif
}


/**
 * Apply all the per-fragment operations to a span.
 * This now includes texturing (_swrast_write_texture_span() is history).
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_rgba_span( GLcontext *ctx, struct sw_span *span)
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;
   const GLboolean deferredTexture = !(ctx->Color.AlphaEnabled ||
                                       ctx->FragmentProgram._Active ||
                                       ctx->ATIFragmentShader._Enabled);

   ASSERT(span->primitive == GL_POINT  ||  span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON  ||  span->primitive == GL_BITMAP);
   ASSERT(span->end <= MAX_WIDTH);
   ASSERT((span->interpMask & span->arrayMask) == 0);

   /*
   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__,
          span->interpMask, span->arrayMask);
   */

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

   /* Interpolate texcoords? */
   if (ctx->Texture._EnabledCoordUnits
       && (span->interpMask & SPAN_TEXTURE)
       && (span->arrayMask & SPAN_TEXTURE) == 0) {
      interpolate_texcoords(ctx, span);
   }

   /* This is the normal place to compute the resulting fragment color/Z.
    * As an optimization, we try to defer this until after Z/stencil
    * testing in order to try to avoid computing colors that we won't
    * actually need.
    */
   if (!deferredTexture) {
      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      if (span->interpMask & SPAN_SPEC)
         interpolate_specular(ctx, span);

      if (span->interpMask & SPAN_FOG)
         interpolate_fog(ctx, span);

      /* Compute fragment colors with fragment program or texture lookups */
      if (ctx->FragmentProgram._Active) {
         /* frag prog may need Z values */
         if (span->interpMask & SPAN_Z)
            _swrast_span_interpolate_z(ctx, span);
         _swrast_exec_fragment_program( ctx, span );
      }
      else if (ctx->ATIFragmentShader._Enabled)
         _swrast_exec_fragment_shader( ctx, span );
      else if (ctx->Texture._EnabledUnits && (span->arrayMask & SPAN_TEXTURE))
         _swrast_texture_span( ctx, span );

      /* Do the alpha test */
      if (!_swrast_alpha_test(ctx, span)) {
         span->arrayMask = origArrayMask;
	 return;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil.Enabled || ctx->Depth.Test) {
      if (span->interpMask & SPAN_Z)
         _swrast_span_interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled && ctx->DrawBuffer->Visual.stencilBits > 0) {
         /* Combined Z/stencil tests */
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else if (ctx->DrawBuffer->Visual.depthBits > 0) {
         /* Just regular depth testing */
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         if (!_swrast_depth_test_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, some fragments passed the depth test */
   if (ctx->Depth.OcclusionTest) {
      ctx->OcclusionResult = GL_TRUE;
   }

#if FEATURE_ARB_occlusion_query
   if (ctx->Occlusion.Active) {
      /* update count of 'passed' fragments */
      GLuint i;
      for (i = 0; i < span->end; i++)
         ctx->Occlusion.PassedCounter += span->array->mask[i];
   }
#endif

   /* We had to wait until now to check for glColorMask(0,0,0,0) because of
    * the occlusion test.
    */
   if (colorMask == 0x0) {
      span->interpMask = origInterpMask;
      span->arrayMask = origArrayMask;
      return;
   }

   /* If we were able to defer fragment color computation to now, there's
    * a good chance that many fragments will have already been killed by
    * Z/stencil testing.
    */
   if (deferredTexture) {
      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      if (span->interpMask & SPAN_SPEC)
         interpolate_specular(ctx, span);

      if (span->interpMask & SPAN_FOG)
         interpolate_fog(ctx, span);

      if (ctx->FragmentProgram._Active)
         _swrast_exec_fragment_program( ctx, span );
      else if (ctx->ATIFragmentShader._Enabled)
         _swrast_exec_fragment_shader( ctx, span );
      else if (ctx->Texture._EnabledUnits && (span->arrayMask & SPAN_TEXTURE))
         _swrast_texture_span( ctx, span );
   }

   ASSERT(span->arrayMask & SPAN_RGBA);

   if (!ctx->FragmentProgram._Enabled) {
      /* Add base and specular colors */
      if (ctx->Fog.ColorSumEnabled ||
          (ctx->Light.Enabled &&
           ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
         if (span->interpMask & SPAN_SPEC) {
            interpolate_specular(ctx, span);
         }
         if (span->arrayMask & SPAN_SPEC) {
            add_colors( span->end, span->array->rgba, span->array->spec );
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
      GLchan (*rgba)[4] = span->array->rgba;
      GLfloat *coverage = span->array->coverage;
      GLuint i;
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      /* need to do blend/logicop separately for each color buffer */
      multi_write_rgba_span(ctx, span);
   }
   else {
      /* normal: write to exactly one buffer */
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[0][0];

      if (ctx->Color._LogicOpEnabled) {
         _swrast_logicop_rgba_span(ctx, rb, span, span->array->rgba);
      }
      else if (ctx->Color.BlendEnabled) {
         _swrast_blend_span(ctx, rb, span, span->array->rgba);
      }

      /* Color component masking */
      if (colorMask != 0xffffffff) {
         _swrast_mask_rgba_span(ctx, rb, span, span->array->rgba);
      }

      /* Finally, write the pixels to a color buffer */
      if (span->arrayMask & SPAN_XY) {
         /* array of pixel coords */
         ASSERT(rb->PutValues);
         ASSERT(rb->_BaseFormat == GL_RGB || rb->_BaseFormat == GL_RGBA);
         /* XXX check datatype */
         rb->PutValues(ctx, rb, span->end, span->array->x, span->array->y,
                       span->array->rgba, span->array->mask);
      }
      else {
         /* horizontal run of pixels */
         ASSERT(rb->PutRow);
         ASSERT(rb->_BaseFormat == GL_RGB || rb->_BaseFormat == GL_RGBA);
         /* XXX check datatype */
         rb->PutRow(ctx, rb, span->end, span->x, span->y, span->array->rgba,
                    span->writeAll ? NULL : span->array->mask);
      }
   }

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}



/**
 * Read RGBA pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void
_swrast_read_rgba_span( GLcontext *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y, GLchan rgba[][4] )
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
      ASSERT(rb->DataType == GL_UNSIGNED_BYTE);
      rb->GetRow(ctx, rb, length, x + skip, y, rgba + skip);
   }
}


/**
 * Read CI pixels from frame buffer.  Clipping will be done to prevent
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
 * \param valueSize is the size in bytes of each value put into the
 *                  values array.
 */
void
_swrast_get_values(GLcontext *ctx, struct gl_renderbuffer *rb,
                   GLuint count, const GLint x[], const GLint y[],
                   void *values, GLuint valueSize)
{
   GLuint i, inCount = 0, inStart = 0;

   for (i = 0; i < count; i++) {
      if (x[i] >= 0 && y[i] >= 0 && x[i] < rb->Width && y[i] < rb->Height) {
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
