/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#include "s_alpha.h"
#include "s_alphabuf.h"
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
   if (ctx->Visual.depthBits <= 16)
      span->z = FloatToFixed(ctx->Current.RasterPos[2] * ctx->DepthMax + 0.5F);
   else
      span->z = (GLint) (ctx->Current.RasterPos[2] * ctx->DepthMax + 0.5F);
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
 * Init span's color or index interpolation values to the RasterPos color.
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
      COPY_4V(span->tex[i], ctx->Current.RasterTexCoords[i]);
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
}


/* Fill in the span.->array->spec array from the interpolation values */
static void
interpolate_specular(GLcontext *ctx, struct sw_span *span)
{
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

/*
 * Fill in the span.texcoords array from the interpolation values.
 * XXX We could optimize here for the case when dq = 0.  That would
 * usually be the case when using an orthographic projection.
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
               const struct gl_texture_image *img = obj->Image[obj->BaseLevel];
               needLambda = (obj->MinFilter != obj->MagFilter)
                  || ctx->FragmentProgram.Enabled;
               texW = img->WidthScale;
               texH = img->HeightScale;
            }
            else {
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
               if (dqdx == 0.0) {
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
         const struct gl_texture_image *img = obj->Image[obj->BaseLevel];
         needLambda = (obj->MinFilter != obj->MagFilter)
            || ctx->FragmentProgram.Enabled;
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
         if (dqdx == 0.0) {
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
 * Draw to more than one color buffer (or none).
 */
static void
multi_write_index_span( GLcontext *ctx, struct sw_span *span )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint bufferBit;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit <<= 1) {
      if (bufferBit & ctx->Color._DrawDestMask) {
         GLuint indexTmp[MAX_WIDTH];
         ASSERT(span->end < MAX_WIDTH);

         /* Set the current read/draw buffer */
         swrast->CurrentBuffer = bufferBit;
         (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, bufferBit);

         /* make copy of incoming indexes */
         MEMCPY( indexTmp, span->array->index, span->end * sizeof(GLuint) );

         if (ctx->Color.IndexLogicOpEnabled) {
            _swrast_logicop_ci_span(ctx, span, indexTmp);
         }

         if (ctx->Color.IndexMask != 0xffffffff) {
            _swrast_mask_index_span(ctx, span, indexTmp);
         }

         if (span->arrayMask & SPAN_XY) {
            /* array of pixel coords */
            (*swrast->Driver.WriteCI32Pixels)(ctx, span->end,
                                              span->array->x, span->array->y,
                                              indexTmp, span->array->mask);
         }
         else {
            /* horizontal run of pixels */
            (*swrast->Driver.WriteCI32Span)(ctx, span->end, span->x, span->y,
                                            indexTmp, span->array->mask);
         }
      }
   }

   /* restore default dest buffer */
   _swrast_use_draw_buffer(ctx);
}


/**
 * Draw to more than one RGBA color buffer (or none).
 * All fragment operations, up to (but not) blending/logicop should
 * have been done first.
 */
static void
multi_write_rgba_span( GLcontext *ctx, struct sw_span *span )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   GLuint bufferBit;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   ASSERT(colorMask != 0x0);

   if (ctx->Color.DrawBuffer == GL_NONE)
      return;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit <<= 1) {
      if (bufferBit & ctx->Color._DrawDestMask) {
         GLchan rgbaTmp[MAX_WIDTH][4];
         ASSERT(span->end < MAX_WIDTH);

         /* Set the current read/draw buffer */
         swrast->CurrentBuffer = bufferBit;
         (*swrast->Driver.SetBuffer)(ctx, ctx->DrawBuffer, bufferBit);

         /* make copy of incoming colors */
         MEMCPY( rgbaTmp, span->array->rgba, 4 * span->end * sizeof(GLchan) );

         if (ctx->Color._LogicOpEnabled) {
            _swrast_logicop_rgba_span(ctx, span, rgbaTmp);
         }
         else if (ctx->Color.BlendEnabled) {
            _swrast_blend_span(ctx, span, rgbaTmp);
         }

         if (colorMask != 0xffffffff) {
            _swrast_mask_rgba_span(ctx, span, rgbaTmp);
         }

         if (span->arrayMask & SPAN_XY) {
            /* array of pixel coords */
            (*swrast->Driver.WriteRGBAPixels)(ctx, span->end,
                                              span->array->x, span->array->y,
                                              (const GLchan (*)[4]) rgbaTmp,
                                              span->array->mask);
            if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
               _swrast_write_alpha_pixels(ctx, span->end,
                                        span->array->x, span->array->y,
                                        (const GLchan (*)[4]) rgbaTmp,
                                        span->array->mask);
            }
         }
         else {
            /* horizontal run of pixels */
            (*swrast->Driver.WriteRGBASpan)(ctx, span->end, span->x, span->y,
                                            (const GLchan (*)[4]) rgbaTmp,
                                            span->array->mask);
            if (swrast->_RasterMask & ALPHABUF_BIT) {
               _swrast_write_alpha_span(ctx, span->end, span->x, span->y,
                                      (const GLchan (*)[4]) rgbaTmp,
                                      span->array->mask);
            }
         }
      }
   }

   /* restore default dest buffer */
   _swrast_use_draw_buffer(ctx);
}



/**
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_index_span( GLcontext *ctx, struct sw_span *span)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;

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
      MEMSET(span->array->mask, 1, span->end);
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

   /* Depth test and stencil */
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
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

#if FEATURE_ARB_occlusion_query
   if (ctx->Occlusion.Active) {
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
   if (span->interpMask & SPAN_INDEX) {
      interpolate_indexes(ctx, span);
      /* clear the bit - this allows the WriteMonoCISpan optimization below */
      span->interpMask &= ~SPAN_INDEX;
   }

   /* Fog */
   if (ctx->Fog.Enabled) {
      _swrast_fog_ci_span(ctx, span);
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      GLuint i;
      GLuint *index = span->array->index;
      GLfloat *coverage = span->array->coverage;
      for (i = 0; i < span->end; i++) {
         ASSERT(coverage[i] < 16);
         index[i] = (index[i] & ~0xf) | ((GLuint) coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      /* draw to zero or two or more buffers */
      multi_write_index_span(ctx, span);
   }
   else {
      /* normal situation: draw to exactly one buffer */
      if (ctx->Color.IndexLogicOpEnabled) {
         _swrast_logicop_ci_span(ctx, span, span->array->index);
      }

      if (ctx->Color.IndexMask != 0xffffffff) {
         _swrast_mask_index_span(ctx, span, span->array->index);
      }

      /* write pixels */
      if (span->arrayMask & SPAN_XY) {
         /* array of pixel coords */
         if ((span->interpMask & SPAN_INDEX) && span->indexStep == 0) {
            /* all pixels have same color index */
            (*swrast->Driver.WriteMonoCIPixels)(ctx, span->end,
                                                span->array->x, span->array->y,
                                                FixedToInt(span->index),
                                                span->array->mask);
         }
         else {
            (*swrast->Driver.WriteCI32Pixels)(ctx, span->end, span->array->x,
                                              span->array->y, span->array->index,
                                              span->array->mask );
         }
      }
      else {
         /* horizontal run of pixels */
         if ((span->interpMask & SPAN_INDEX) && span->indexStep == 0) {
            /* all pixels have same color index */
            (*swrast->Driver.WriteMonoCISpan)(ctx, span->end, span->x, span->y,
                                              FixedToInt(span->index),
                                              span->array->mask);
         }
         else {
            (*swrast->Driver.WriteCI32Span)(ctx, span->end, span->x, span->y,
                                            span->array->index,
                                            span->array->mask);
         }
      }
   }

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}


/**
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_rgba_span( GLcontext *ctx, struct sw_span *span)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;
   GLboolean monoColor;

   ASSERT(span->end <= MAX_WIDTH);
   ASSERT(span->primitive == GL_POINT  ||  span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON  ||  span->primitive == GL_BITMAP);
   ASSERT((span->interpMask & span->arrayMask) == 0);
   ASSERT((span->interpMask | span->arrayMask) & SPAN_RGBA);
#ifdef DEBUG
   if (ctx->Fog.Enabled)
      ASSERT((span->interpMask | span->arrayMask) & SPAN_FOG);
   if (ctx->Depth.Test)
      ASSERT((span->interpMask | span->arrayMask) & SPAN_Z);
#endif

   if (span->arrayMask & SPAN_MASK) {
      /* mask was initialized by caller, probably glBitmap */
      span->writeAll = GL_FALSE;
   }
   else {
      MEMSET(span->array->mask, 1, span->end);
      span->writeAll = GL_TRUE;
   }

   /* Determine if we have mono-chromatic colors */
   monoColor = (span->interpMask & SPAN_RGBA) &&
      span->redStep == 0 && span->greenStep == 0 &&
      span->blueStep == 0 && span->alphaStep == 0;

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

   /* Fragment program */
   if (ctx->FragmentProgram.Enabled) {
      /* Now we may need to interpolate the colors */
      if ((span->interpMask & SPAN_RGBA) &&
          (span->arrayMask & SPAN_RGBA) == 0) {
         interpolate_colors(ctx, span);
         span->interpMask &= ~SPAN_RGBA;
      }
      if (span->interpMask & SPAN_SPEC) {
         interpolate_specular(ctx, span);
      }
      _swrast_exec_nv_fragment_program(ctx, span);
      monoColor = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (!_swrast_alpha_test(ctx, span)) {
         span->interpMask = origInterpMask;
         span->arrayMask = origArrayMask;
	 return;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil.Enabled || ctx->Depth.Test) {
      if (span->interpMask & SPAN_Z)
         _swrast_span_interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled) {
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         /* regular depth testing */
         if (!_swrast_depth_test_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

#if FEATURE_ARB_occlusion_query
   if (ctx->Occlusion.Active) {
      GLuint i;
      for (i = 0; i < span->end; i++)
         ctx->Occlusion.PassedCounter += span->array->mask[i];
   }
#endif

   /* can't abort span-writing until after occlusion testing */
   if (colorMask == 0x0) {
      span->interpMask = origInterpMask;
      span->arrayMask = origArrayMask;
      return;
   }

   /* Now we may need to interpolate the colors */
   if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0) {
      interpolate_colors(ctx, span);
      /* clear the bit - this allows the WriteMonoCISpan optimization below */
      span->interpMask &= ~SPAN_RGBA;
   }

   /* Fog */
   if (ctx->Fog.Enabled) {
      _swrast_fog_rgba_span(ctx, span);
      monoColor = GL_FALSE;
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      GLchan (*rgba)[4] = span->array->rgba;
      GLfloat *coverage = span->array->coverage;
      GLuint i;
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
      monoColor = GL_FALSE;
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span(ctx, span);
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color._LogicOpEnabled) {
         _swrast_logicop_rgba_span(ctx, span, span->array->rgba);
         monoColor = GL_FALSE;
      }
      else if (ctx->Color.BlendEnabled) {
         _swrast_blend_span(ctx, span, span->array->rgba);
         monoColor = GL_FALSE;
      }

      /* Color component masking */
      if (colorMask != 0xffffffff) {
         _swrast_mask_rgba_span(ctx, span, span->array->rgba);
         monoColor = GL_FALSE;
      }

      /* write pixels */
      if (span->arrayMask & SPAN_XY) {
         /* array of pixel coords */
         /* XXX test for mono color */
         (*swrast->Driver.WriteRGBAPixels)(ctx, span->end, span->array->x,
             span->array->y, (const GLchan (*)[4]) span->array->rgba, span->array->mask);
         if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
            _swrast_write_alpha_pixels(ctx, span->end,
                                     span->array->x, span->array->y,
                                     (const GLchan (*)[4]) span->array->rgba,
                                     span->array->mask);
         }
      }
      else {
         /* horizontal run of pixels */
         if (monoColor) {
            /* all pixels have same color */
            GLchan color[4];
            color[RCOMP] = FixedToChan(span->red);
            color[GCOMP] = FixedToChan(span->green);
            color[BCOMP] = FixedToChan(span->blue);
            color[ACOMP] = FixedToChan(span->alpha);
            (*swrast->Driver.WriteMonoRGBASpan)(ctx, span->end, span->x,
                                                span->y, color, span->array->mask);
            if (swrast->_RasterMask & ALPHABUF_BIT) {
               _swrast_write_mono_alpha_span(ctx, span->end, span->x, span->y,
                      color[ACOMP],
                      span->writeAll ? ((const GLubyte *) NULL) : span->array->mask);
            }
         }
         else {
            /* each pixel is a different color */
            (*swrast->Driver.WriteRGBASpan)(ctx, span->end, span->x, span->y,
                      (const GLchan (*)[4]) span->array->rgba,
                      span->writeAll ? ((const GLubyte *) NULL) : span->array->mask);
            if (swrast->_RasterMask & ALPHABUF_BIT) {
               _swrast_write_alpha_span(ctx, span->end, span->x, span->y,
                      (const GLchan (*)[4]) span->array->rgba,
                      span->writeAll ? ((const GLubyte *) NULL) : span->array->mask);
            }
         }
      }
   }

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
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_swrast_write_texture_span( GLcontext *ctx, struct sw_span *span)
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;

   ASSERT(span->primitive == GL_POINT  ||  span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON  ||  span->primitive == GL_BITMAP);
   ASSERT(span->end <= MAX_WIDTH);
   ASSERT((span->interpMask & span->arrayMask) == 0);
   ASSERT(ctx->Texture._EnabledCoordUnits || ctx->FragmentProgram.Enabled);

   /*
   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__, span->interpMask, span->arrayMask);
   */

   if (span->arrayMask & SPAN_MASK) {
      /* mask was initialized by caller, probably glBitmap */
      span->writeAll = GL_FALSE;
   }
   else {
      MEMSET(span->array->mask, 1, span->end);
      span->writeAll = GL_TRUE;
   }

   /* Clipping */
   if ((swrast->_RasterMask & CLIP_BIT) || (span->primitive != GL_POLYGON)) {
      if (!clip_span(ctx, span)) {
	 return;
      }
   }

#ifdef DEBUG
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

   /* Need texture coordinates now */
   if ((span->interpMask & SPAN_TEXTURE)
       && (span->arrayMask & SPAN_TEXTURE) == 0)
      interpolate_texcoords(ctx, span);

   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {

      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      if (span->interpMask & SPAN_SPEC) {
         interpolate_specular(ctx, span);
      }

      /* Texturing without alpha is done after depth-testing which
       * gives a potential speed-up.
       */
      if (ctx->FragmentProgram.Enabled)
         _swrast_exec_nv_fragment_program( ctx, span );
      else
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

      if (ctx->Stencil.Enabled) {
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         /* regular depth testing */
         if (!_swrast_depth_test_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, some fragments passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

#if FEATURE_ARB_occlusion_query
   if (ctx->Occlusion.Active) {
      GLuint i;
      for (i = 0; i < span->end; i++)
         ctx->Occlusion.PassedCounter += span->array->mask[i];
   }
#endif

   /* We had to wait until now to check for glColorMask(F,F,F,F) because of
    * the occlusion test.
    */
   if (colorMask == 0x0) {
      span->interpMask = origInterpMask;
      span->arrayMask = origArrayMask;
      return;
   }

   /* Texture without alpha test */
   if (!ctx->Color.AlphaEnabled) {

      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      if (span->interpMask & SPAN_SPEC) {
         interpolate_specular(ctx, span);
      }

      if (ctx->FragmentProgram.Enabled)
         _swrast_exec_nv_fragment_program( ctx, span );
      else
         _swrast_texture_span( ctx, span );
   }

   ASSERT(span->arrayMask & SPAN_RGBA);

   /* Add base and specular colors */
   if (ctx->Fog.ColorSumEnabled ||
       (ctx->Light.Enabled &&
        ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
      if (span->interpMask & SPAN_SPEC) {
         interpolate_specular(ctx, span);
      }
      ASSERT(span->arrayMask & SPAN_SPEC);
      add_colors( span->end, span->array->rgba, span->array->spec );
   }

   /* Fog */
   if (ctx->Fog.Enabled) {
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
      multi_write_rgba_span(ctx, span);
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color._LogicOpEnabled) {
         _swrast_logicop_rgba_span(ctx, span, span->array->rgba);
      }
      else if (ctx->Color.BlendEnabled) {
         _swrast_blend_span(ctx, span, span->array->rgba);
      }

      /* Color component masking */
      if (colorMask != 0xffffffff) {
         _swrast_mask_rgba_span(ctx, span, span->array->rgba);
      }

      /* write pixels */
      if (span->arrayMask & SPAN_XY) {
         /* array of pixel coords */
         (*swrast->Driver.WriteRGBAPixels)(ctx, span->end, span->array->x,
             span->array->y, (const GLchan (*)[4]) span->array->rgba, span->array->mask);
         if (SWRAST_CONTEXT(ctx)->_RasterMask & ALPHABUF_BIT) {
            _swrast_write_alpha_pixels(ctx, span->end,
                                     span->array->x, span->array->y,
                                     (const GLchan (*)[4]) span->array->rgba,
                                     span->array->mask);
         }
      }
      else {
         /* horizontal run of pixels */
         (*swrast->Driver.WriteRGBASpan)(ctx, span->end, span->x, span->y,
                                       (const GLchan (*)[4]) span->array->rgba,
                                       span->writeAll ? NULL : span->array->mask);
         if (swrast->_RasterMask & ALPHABUF_BIT) {
            _swrast_write_alpha_span(ctx, span->end, span->x, span->y,
                                   (const GLchan (*)[4]) span->array->rgba,
                                   span->writeAll ? NULL : span->array->mask);
         }
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
_swrast_read_rgba_span( GLcontext *ctx, GLframebuffer *buffer,
                      GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLint bufWidth = (GLint) buffer->Width;
   const GLint bufHeight = (GLint) buffer->Height;

   if (y < 0 || y >= bufHeight || x + (GLint) n < 0 || x >= bufWidth) {
      /* completely above, below, or right */
      /* XXX maybe leave undefined? */
      _mesa_bzero(rgba, 4 * n * sizeof(GLchan));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clippping */
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

      (*swrast->Driver.ReadRGBASpan)( ctx, length, x + skip, y, rgba + skip );
      if (buffer->UseSoftwareAlphaBuffers) {
         _swrast_read_alpha_span(ctx, length, x + skip, y, rgba + skip);
      }
   }
}


/**
 * Read CI pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void
_swrast_read_index_span( GLcontext *ctx, GLframebuffer *buffer,
                       GLuint n, GLint x, GLint y, GLuint indx[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLint bufWidth = (GLint) buffer->Width;
   const GLint bufHeight = (GLint) buffer->Height;

   if (y < 0 || y >= bufHeight || x + (GLint) n < 0 || x >= bufWidth) {
      /* completely above, below, or right */
      _mesa_bzero(indx, n * sizeof(GLuint));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clippping */
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

      (*swrast->Driver.ReadCI32Span)( ctx, length, skip + x, y, indx + skip );
   }
}
