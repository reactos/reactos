/*
 * Mesa 3-D graphics library
 * Version:  7.5
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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

#include <precomp.h>

/**
 * Set default fragment attributes for the span using the
 * current raster values.  Used prior to glDraw/CopyPixels
 * and glBitmap.
 */
void
_swrast_span_default_attribs(struct gl_context *ctx, SWspan *span)
{
   GLchan r, g, b, a;
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

   COPY_4V(span->attrStart[FRAG_ATTRIB_COL], ctx->Current.RasterColor);
   ASSIGN_4V(span->attrStepX[FRAG_ATTRIB_COL], 0.0, 0.0, 0.0, 0.0);
   ASSIGN_4V(span->attrStepY[FRAG_ATTRIB_COL], 0.0, 0.0, 0.0, 0.0);

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
      const GLuint attr = FRAG_ATTRIB_TEX;
      const GLfloat *tc = ctx->Current.RasterTexCoords;
      if (tc[3] > 0.0F) {
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


/**
 * Interpolate the active attributes (and'd with attrMask) to
 * fill in span->array->attribs[].
 * Perspective correction will be done.  The point/line/triangle function
 * should have computed attrStart/Step values for FRAG_ATTRIB_WPOS[3]!
 */
static inline void
interpolate_active_attribs(struct gl_context *ctx, SWspan *span,
                           GLbitfield64 attrMask)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /*
    * Don't overwrite existing array values, such as colors that may have
    * been produced by glDraw/CopyPixels.
    */
   attrMask &= ~span->arrayAttribs;

   ATTRIB_LOOP_BEGIN
      if (attrMask & BITFIELD64_BIT(attr)) {
         const GLfloat dwdx = span->attrStepX[FRAG_ATTRIB_WPOS][3];
         GLfloat w = span->attrStart[FRAG_ATTRIB_WPOS][3];
         const GLfloat dv0dx = span->attrStepX[attr][0];
         const GLfloat dv1dx = span->attrStepX[attr][1];
         const GLfloat dv2dx = span->attrStepX[attr][2];
         const GLfloat dv3dx = span->attrStepX[attr][3];
         GLfloat v0 = span->attrStart[attr][0] + span->leftClip * dv0dx;
         GLfloat v1 = span->attrStart[attr][1] + span->leftClip * dv1dx;
         GLfloat v2 = span->attrStart[attr][2] + span->leftClip * dv2dx;
         GLfloat v3 = span->attrStart[attr][3] + span->leftClip * dv3dx;
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
         ASSERT((span->arrayAttribs & BITFIELD64_BIT(attr)) == 0);
         span->arrayAttribs |= BITFIELD64_BIT(attr);
      }
   ATTRIB_LOOP_END
}


/**
 * Interpolate primary colors to fill in the span->array->rgba8 (or rgb16)
 * color array.
 */
static inline void
interpolate_int_colors(struct gl_context *ctx, SWspan *span)
{
#if CHAN_BITS != 32
   const GLuint n = span->end;
   GLuint i;

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
      interpolate_active_attribs(ctx, span, FRAG_BIT_COL);
      break;
   default:
      _mesa_problem(ctx, "bad datatype 0x%x in interpolate_int_colors",
                    span->array->ChanType);
   }
   span->arrayMask |= SPAN_RGBA;
}

/**
 * Fill in the span.zArray array from the span->z, zStep values.
 */
void
_swrast_span_interpolate_z( const struct gl_context *ctx, SWspan *span )
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
interpolate_texcoords(struct gl_context *ctx, SWspan *span)
{
   if (ctx->Texture._EnabledCoord) {
      const GLuint attr = FRAG_ATTRIB_TEX;
      const struct gl_texture_object *obj = ctx->Texture.Unit._Current;
      GLfloat texW, texH;
      GLboolean needLambda;
      GLfloat (*texcoord)[4] = span->array->attribs[attr];
      GLfloat *lambda = span->array->lambda;
      const GLfloat dsdx = span->attrStepX[attr][0];
      const GLfloat dsdy = span->attrStepY[attr][0];
      const GLfloat dtdx = span->attrStepX[attr][1];
      const GLfloat dtdy = span->attrStepY[attr][1];
      const GLfloat drdx = span->attrStepX[attr][2];
      const GLfloat dqdx = span->attrStepX[attr][3];
      const GLfloat dqdy = span->attrStepY[attr][3];
      GLfloat s = span->attrStart[attr][0] + span->leftClip * dsdx;
      GLfloat t = span->attrStart[attr][1] + span->leftClip * dtdx;
      GLfloat r = span->attrStart[attr][2] + span->leftClip * drdx;
      GLfloat q = span->attrStart[attr][3] + span->leftClip * dqdx;

      if (obj) {
         const struct gl_texture_image *img = obj->Image[0][obj->BaseLevel];
         const struct swrast_texture_image *swImg =
            swrast_texture_image_const(img);

         needLambda = (obj->Sampler.MinFilter != obj->Sampler.MagFilter);
         /* LOD is calculated directly in the ansiotropic filter, we can
          * skip the normal lambda function as the result is ignored.
          */
         if (obj->Sampler.MaxAnisotropy > 1.0 &&
             obj->Sampler.MinFilter == GL_LINEAR_MIPMAP_LINEAR) {
            needLambda = GL_FALSE;
         }
         texW = swImg->WidthScale;
         texH = swImg->HeightScale;
      }
      else {
         /* using a fragment program */
         texW = 1.0;
         texH = 1.0;
         needLambda = GL_FALSE;
      }

      if (needLambda) {
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
         GLuint i;
         if (dqdx == 0.0F) {
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
}

/**
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static inline void
stipple_polygon_span(struct gl_context *ctx, SWspan *span)
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
static inline GLuint
clip_span( struct gl_context *ctx, SWspan *span )
{
   const GLint xmin = ctx->DrawBuffer->_Xmin;
   const GLint xmax = ctx->DrawBuffer->_Xmax;
   const GLint ymin = ctx->DrawBuffer->_Ymin;
   const GLint ymax = ctx->DrawBuffer->_Ymax;

   span->leftClip = 0;

   if (span->arrayMask & SPAN_XY) {
      /* arrays of x/y pixel coords */
      const GLint *x = span->array->x;
      const GLint *y = span->array->y;
      const GLint n = span->end;
      GLubyte *mask = span->array->mask;
      GLint i;
      GLuint passed = 0;
      if (span->arrayMask & SPAN_MASK) {
         /* note: using & intead of && to reduce branches */
         for (i = 0; i < n; i++) {
            mask[i] &= (x[i] >= xmin) & (x[i] < xmax)
                     & (y[i] >= ymin) & (y[i] < ymax);
            passed += mask[i];
         }
      }
      else {
         /* note: using & intead of && to reduce branches */
         for (i = 0; i < n; i++) {
            mask[i] = (x[i] >= xmin) & (x[i] < xmax)
                    & (y[i] >= ymin) & (y[i] < ymax);
            passed += mask[i];
         }
      }
      return passed > 0;
   }
   else {
      /* horizontal span of pixels */
      const GLint x = span->x;
      const GLint y = span->y;
      GLint n = span->end;

      /* Trivial rejection tests */
      if (y < ymin || y >= ymax || x + n <= xmin || x >= xmax) {
         span->end = 0;
         return GL_FALSE;  /* all pixels clipped */
      }

      /* Clip to right */
      if (x + n > xmax) {
         ASSERT(x < xmax);
         n = span->end = xmax - x;
      }

      /* Clip to the left */
      if (x < xmin) {
         const GLint leftClip = xmin - x;
         GLuint i;

         ASSERT(leftClip > 0);
         ASSERT(x + n > xmin);

         /* Clip 'leftClip' pixels from the left side.
          * The span->leftClip field will be applied when we interpolate
          * fragment attributes.
          * For arrays of values, shift them left.
          */
         for (i = 0; i < FRAG_ATTRIB_MAX; i++) {
            if (span->interpMask & (1 << i)) {
               GLuint j;
               for (j = 0; j < 4; j++) {
                  span->attrStart[i][j] += leftClip * span->attrStepX[i][j];
               }
            }
         }

         span->red += leftClip * span->redStep;
         span->green += leftClip * span->greenStep;
         span->blue += leftClip * span->blueStep;
         span->alpha += leftClip * span->alphaStep;
         span->index += leftClip * span->indexStep;
         span->z += leftClip * span->zStep;
         span->intTex[0] += leftClip * span->intTexStep[0];
         span->intTex[1] += leftClip * span->intTexStep[1];

#define SHIFT_ARRAY(ARRAY, SHIFT, LEN) \
         memmove(ARRAY, ARRAY + (SHIFT), (LEN) * sizeof(ARRAY[0]))

         for (i = 0; i < FRAG_ATTRIB_MAX; i++) {
            if (span->arrayAttribs & (1 << i)) {
               /* shift array elements left by 'leftClip' */
               SHIFT_ARRAY(span->array->attribs[i], leftClip, n - leftClip);
            }
         }

         SHIFT_ARRAY(span->array->mask, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->rgba8, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->rgba16, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->x, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->y, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->z, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->index, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->lambda, leftClip, n - leftClip);
         SHIFT_ARRAY(span->array->coverage, leftClip, n - leftClip);

#undef SHIFT_ARRAY

         span->leftClip = leftClip;
         span->x = xmin;
         span->end -= leftClip;
         span->writeAll = GL_FALSE;
      }

      ASSERT(span->x >= xmin);
      ASSERT(span->x + span->end <= xmax);
      ASSERT(span->y >= ymin);
      ASSERT(span->y < ymax);

      return GL_TRUE;  /* some pixels visible */
   }
}


/**
 * Apply antialiasing coverage value to alpha values.
 */
static inline void
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
      GLfloat (*rgba)[4] = span->array->attribs[FRAG_ATTRIB_COL];
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = rgba[i][ACOMP] * coverage[i];
         /* clamp later */
      }
   }
}

/**
 * Convert the span's color arrays to the given type.
 * The only way 'output' can be greater than zero is when we have a fragment
 * program that writes to gl_FragData[1] or higher.
 * \param output  which fragment program color output is being processed
 */
static inline void
convert_color_type(SWspan *span, GLenum newType, GLuint output)
{
   GLvoid *src, *dst;

   if (output > 0 || span->array->ChanType == GL_FLOAT) {
      src = span->array->attribs[FRAG_ATTRIB_COL + output];
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
      dst = span->array->attribs[FRAG_ATTRIB_COL];
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
static inline void
shade_texture_span(struct gl_context *ctx, SWspan *span)
{
   if (ctx->Texture._EnabledCoord) {
      /* conventional texturing */

#if CHAN_BITS == 32
      if ((span->arrayAttribs & FRAG_BIT_COL0) == 0) {
         interpolate_int_colors(ctx, span);
      }
#else
      if (!(span->arrayMask & SPAN_RGBA))
         interpolate_int_colors(ctx, span);
#endif
      if (!(span->arrayAttribs & FRAG_BIT_TEX))
         interpolate_texcoords(ctx, span);

      _swrast_texture_span(ctx, span);
   }
}


/** Put colors at x/y locations into a renderbuffer */
static void
put_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
           GLenum datatype,
           GLuint count, const GLint x[], const GLint y[],
           const void *values, const GLubyte *mask)
{
   gl_pack_ubyte_rgba_func pack_ubyte;
   gl_pack_float_rgba_func pack_float;
   GLuint i;

   if (datatype == GL_UNSIGNED_BYTE)
      pack_ubyte = _mesa_get_pack_ubyte_rgba_function(rb->Format);
   else
      pack_float = _mesa_get_pack_float_rgba_function(rb->Format);

   for (i = 0; i < count; i++) {
      if (mask[i]) {
         GLubyte *dst = _swrast_pixel_address(rb, x[i], y[i]);

         if (datatype == GL_UNSIGNED_BYTE) {
            pack_ubyte((const GLubyte *) values + 4 * i, dst);
         }
         else {
            assert(datatype == GL_FLOAT);
            pack_float((const GLfloat *) values + 4 * i, dst);
         }
      }
   }
}


/** Put row of colors into renderbuffer */
void
_swrast_put_row(struct gl_context *ctx, struct gl_renderbuffer *rb,
                GLenum datatype,
                GLuint count, GLint x, GLint y,
                const void *values, const GLubyte *mask)
{
   GLubyte *dst = _swrast_pixel_address(rb, x, y);

   if (!mask) {
      if (datatype == GL_UNSIGNED_BYTE) {
         _mesa_pack_ubyte_rgba_row(rb->Format, count,
                                   (const GLubyte (*)[4]) values, dst);
      }
      else {
         assert(datatype == GL_FLOAT);
         _mesa_pack_float_rgba_row(rb->Format, count,
                                   (const GLfloat (*)[4]) values, dst);
      }
   }
   else {
      const GLuint bpp = _mesa_get_format_bytes(rb->Format);
      GLuint i, runLen, runStart;
      /* We can't pass a 'mask' array to the _mesa_pack_rgba_row() functions
       * so look for runs where mask=1...
       */
      runLen = runStart = 0;
      for (i = 0; i < count; i++) {
         if (mask[i]) {
            if (runLen == 0)
               runStart = i;
            runLen++;
         }

         if (!mask[i] || i == count - 1) {
            /* might be the end of a run of pixels */
            if (runLen > 0) {
               if (datatype == GL_UNSIGNED_BYTE) {
                  _mesa_pack_ubyte_rgba_row(rb->Format, runLen,
                                     (const GLubyte (*)[4]) values + runStart,
                                     dst + runStart * bpp);
               }
               else {
                  assert(datatype == GL_FLOAT);
                  _mesa_pack_float_rgba_row(rb->Format, runLen,
                                   (const GLfloat (*)[4]) values + runStart,
                                   dst + runStart * bpp);
               }
               runLen = 0;
            }
         }
      }
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
_swrast_write_rgba_span( struct gl_context *ctx, SWspan *span)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint colorMask = *((GLuint *)ctx->Color.ColorMask);
   const GLbitfield origInterpMask = span->interpMask;
   const GLbitfield origArrayMask = span->arrayMask;
   const GLbitfield64 origArrayAttribs = span->arrayAttribs;
   const GLenum origChanType = span->array->ChanType;
   void * const origRgba = span->array->rgba;
   const GLboolean texture = ctx->Texture._EnabledCoord;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /*
   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__,
          span->interpMask, span->arrayMask);
   */

   ASSERT(span->primitive == GL_POINT ||
          span->primitive == GL_LINE ||
	  span->primitive == GL_POLYGON ||
          span->primitive == GL_BITMAP);

   /* Fragment write masks */
   if (span->arrayMask & SPAN_MASK) {
      /* mask was initialized by caller, probably glBitmap */
      span->writeAll = GL_FALSE;
   }
   else {
      memset(span->array->mask, 1, span->end);
      span->writeAll = GL_TRUE;
   }

   /* Clip to window/scissor box */
   if (!clip_span(ctx, span)) {
      return;
   }

   ASSERT(span->end <= MAX_WIDTH);

   /* Depth bounds test */
   if (ctx->Depth.BoundsTest && fb->Visual.depthBits > 0) {
      if (!_swrast_depth_bounds_test(ctx, span)) {
         return;
      }
   }

#ifdef DEBUG
   /* Make sure all fragments are within window bounds */
   if (span->arrayMask & SPAN_XY) {
      /* array of pixel locations */
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
   if (texture && !swrast->_DeferredTexture) {
      shade_texture_span(ctx, span);
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (!_swrast_alpha_test(ctx, span)) {
         /* all fragments failed test */
         goto end;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil._Enabled || ctx->Depth.Test) {
      if (!(span->arrayMask & SPAN_Z))
         _swrast_span_interpolate_z(ctx, span);

      if (ctx->Stencil._Enabled) {
         /* Combined Z/stencil tests */
         if (!_swrast_stencil_and_ztest_span(ctx, span)) {
            /* all fragments failed test */
            goto end;
         }
      }
      else if (fb->Visual.depthBits > 0) {
         /* Just regular depth testing */
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         if (!_swrast_depth_test_span(ctx, span)) {
            /* all fragments failed test */
            goto end;
         }
      }
   }

   /* We had to wait until now to check for glColorMask(0,0,0,0) because of
    * the occlusion test.
    */
   if (colorMask == 0) {
      /* no colors to write */
      goto end;
   }

   /* If we were able to defer fragment color computation to now, there's
    * a good chance that many fragments will have already been killed by
    * Z/stencil testing.
    */
   if (texture && swrast->_DeferredTexture) {
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

   /* Fog */
   if (swrast->_FogEnabled) {
      _swrast_fog_rgba_span(ctx, span);
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      apply_aa_coverage(span);
   }

   /*
    * Write to renderbuffers.
    * Depending on glDrawBuffer() state and the which color outputs are
    * written by the fragment shader, we may either replicate one color to
    * all renderbuffers or write a different color to each renderbuffer.
    * multiFragOutputs=TRUE for the later case.
    */
   {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffer;

      /* color[fragOutput] will be written to buffer */

      if (rb) {
         struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
         GLenum colorType = srb->ColorType;

         assert(colorType == GL_UNSIGNED_BYTE ||
                colorType == GL_FLOAT);

         /* set span->array->rgba to colors for renderbuffer's datatype */
         if (span->array->ChanType != colorType) {
            convert_color_type(span, colorType, 0);
         }
         else {
            if (span->array->ChanType == GL_UNSIGNED_BYTE) {
               span->array->rgba = span->array->rgba8;
            }
            else {
               span->array->rgba = (void *)span->array->attribs[FRAG_ATTRIB_COL];
            }
         }


         ASSERT(rb->_BaseFormat == GL_RGBA ||
                rb->_BaseFormat == GL_RGB ||
                rb->_BaseFormat == GL_RED ||
                rb->_BaseFormat == GL_RG ||
                rb->_BaseFormat == GL_ALPHA);

         if (ctx->Color.ColorLogicOpEnabled) {
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
            put_values(ctx, rb,
                       span->array->ChanType, span->end,
                       span->array->x, span->array->y,
                       span->array->rgba, span->array->mask);
         }
         else {
            /* horizontal run of pixels */
            _swrast_put_row(ctx, rb,
                            span->array->ChanType,
                            span->end, span->x, span->y,
                            span->array->rgba,
                            span->writeAll ? NULL: span->array->mask);
         }

      } /* if rb */
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
 * Read float RGBA pixels from a renderbuffer.  Clipping will be done to
 * prevent reading ouside the buffer's boundaries.
 * \param rgba  the returned colors
 */
void
_swrast_read_rgba_span( struct gl_context *ctx, struct gl_renderbuffer *rb,
                        GLuint n, GLint x, GLint y,
                        GLvoid *rgba)
{
   struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
#if __REACTOS__
   UNREFERENCED_PARAMETER(srb);
#endif
   GLenum dstType = GL_FLOAT;
   const GLint bufWidth = (GLint) rb->Width;
   const GLint bufHeight = (GLint) rb->Height;

   if (y < 0 || y >= bufHeight || x + (GLint) n < 0 || x >= bufWidth) {
      /* completely above, below, or right */
      /* XXX maybe leave rgba values undefined? */
      memset(rgba, 0, 4 * n * sizeof(GLchan));
   }
   else {
      GLint skip, length;
      GLubyte *src;

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
      ASSERT(rb->_BaseFormat == GL_RGBA ||
	     rb->_BaseFormat == GL_RGB ||
	     rb->_BaseFormat == GL_RG ||
	     rb->_BaseFormat == GL_RED ||
	     rb->_BaseFormat == GL_LUMINANCE ||
	     rb->_BaseFormat == GL_INTENSITY ||
	     rb->_BaseFormat == GL_LUMINANCE_ALPHA ||
	     rb->_BaseFormat == GL_ALPHA);

      assert(srb->Map);

      src = _swrast_pixel_address(rb, x + skip, y);

      if (dstType == GL_UNSIGNED_BYTE) {
         _mesa_unpack_ubyte_rgba_row(rb->Format, length, src,
                                     (GLubyte (*)[4]) rgba + skip);
      }
      else if (dstType == GL_FLOAT) {
         _mesa_unpack_rgba_row(rb->Format, length, src,
                               (GLfloat (*)[4]) rgba + skip);
      }
      else {
         _mesa_problem(ctx, "unexpected type in _swrast_read_rgba_span()");
      }
   }
}


/**
 * Get colors at x/y positions with clipping.
 * \param type  type of values to return
 */
static void
get_values(struct gl_context *ctx, struct gl_renderbuffer *rb,
           GLuint count, const GLint x[], const GLint y[],
           void *values, GLenum type)
{
   GLuint i;

   for (i = 0; i < count; i++) {
      if (x[i] >= 0 && y[i] >= 0 &&
	  x[i] < (GLint) rb->Width && y[i] < (GLint) rb->Height) {
         /* inside */
         const GLubyte *src = _swrast_pixel_address(rb, x[i], y[i]);

         if (type == GL_UNSIGNED_BYTE) {
            _mesa_unpack_ubyte_rgba_row(rb->Format, 1, src,
                                        (GLubyte (*)[4]) values + i);
         }
         else if (type == GL_FLOAT) {
            _mesa_unpack_rgba_row(rb->Format, 1, src,
                                  (GLfloat (*)[4]) values + i);
         }
         else {
            _mesa_problem(ctx, "unexpected type in get_values()");
         }
      }
   }
}


/**
 * Get row of colors with clipping.
 * \param type  type of values to return
 */
static void
get_row(struct gl_context *ctx, struct gl_renderbuffer *rb,
        GLuint count, GLint x, GLint y,
        GLvoid *values, GLenum type)
{
   GLint skip = 0;
   GLubyte *src;

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

   src = _swrast_pixel_address(rb, x, y);

   if (type == GL_UNSIGNED_BYTE) {
      _mesa_unpack_ubyte_rgba_row(rb->Format, count, src,
                                  (GLubyte (*)[4]) values + skip);
   }
   else if (type == GL_FLOAT) {
      _mesa_unpack_rgba_row(rb->Format, count, src,
                            (GLfloat (*)[4]) values + skip);
   }
   else {
      _mesa_problem(ctx, "unexpected type in get_row()");
   }
}


/**
 * Get RGBA pixels from the given renderbuffer.
 * Used by blending, logicop and masking functions.
 * \return pointer to the colors we read.
 */
void *
_swrast_get_dest_rgba(struct gl_context *ctx, struct gl_renderbuffer *rb,
                      SWspan *span)
{
   void *rbPixels;

   /* Point rbPixels to a temporary space */
   rbPixels = span->array->attribs[FRAG_ATTRIB_MAX - 1];

   /* Get destination values from renderbuffer */
   if (span->arrayMask & SPAN_XY) {
      get_values(ctx, rb, span->end, span->array->x, span->array->y,
                 rbPixels, span->array->ChanType);
   }
   else {
      get_row(ctx, rb, span->end, span->x, span->y,
              rbPixels, span->array->ChanType);
   }

   return rbPixels;
}
