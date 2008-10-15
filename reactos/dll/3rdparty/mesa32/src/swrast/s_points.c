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


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "texstate.h"
#include "s_context.h"
#include "s_feedback.h"
#include "s_points.h"
#include "s_span.h"


/**
 * Used to cull points with invalid coords
 */
#define CULL_INVALID(V)                              \
   do {                                              \
      float tmp = (V)->attrib[FRAG_ATTRIB_WPOS][0]   \
                + (V)->attrib[FRAG_ATTRIB_WPOS][1];  \
      if (IS_INF_OR_NAN(tmp))                        \
	 return;                                     \
   } while(0)



/**
 * Get/compute the point size.
 * The size may come from a vertex shader, or computed with attentuation
 * or just the glPointSize value.
 * Must also clamp to user-defined range and implmentation limits.
 */
static INLINE GLfloat
get_size(const GLcontext *ctx, const SWvertex *vert, GLboolean smoothed)
{
   GLfloat size;

   if (ctx->Point._Attenuated || ctx->VertexProgram.PointSizeEnabled) {
      /* use vertex's point size */
      size = vert->pointSize;
   }
   else {
      /* use constant point size */
      size = ctx->Point.Size;
   }
   /* always clamp to user-specified limits */
   size = CLAMP(size, ctx->Point.MinSize, ctx->Point.MaxSize);
   /* clamp to implementation limits */
   if (smoothed)
      size = CLAMP(size, ctx->Const.MinPointSizeAA, ctx->Const.MaxPointSizeAA);
   else
      size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);

   return size;
}


/**
 * Draw a point sprite
 */
static void
sprite_point(GLcontext *ctx, const SWvertex *vert)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   SWspan span;
   GLfloat size;
   GLuint tCoords[MAX_TEXTURE_COORD_UNITS + 1];
   GLuint numTcoords = 0;
   GLfloat t0, dtdy;

   CULL_INVALID(vert);

   /* z coord */
   if (ctx->DrawBuffer->Visual.depthBits <= 16)
      span.z = FloatToFixed(vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   else
      span.z = (GLuint) (vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   span.zStep = 0;

   size = get_size(ctx, vert, GL_FALSE);

   /* span init */
   INIT_SPAN(span, GL_POINT);
   span.interpMask = SPAN_Z | SPAN_RGBA;

   span.facing = swrast->PointLineFacing;

   span.red   = ChanToFixed(vert->color[0]);
   span.green = ChanToFixed(vert->color[1]);
   span.blue  = ChanToFixed(vert->color[2]);
   span.alpha = ChanToFixed(vert->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;

   /* need these for fragment programs */
   span.attrStart[FRAG_ATTRIB_WPOS][3] = 1.0F;
   span.attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0F;
   span.attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0F;

   {
      GLfloat s, r, dsdx;

      /* texcoord / pointcoord interpolants */
      s = 0.0;
      dsdx = 1.0 / size;
      if (ctx->Point.SpriteOrigin == GL_LOWER_LEFT) {
         t0 = 0.0;
         dtdy = 1.0 / size;
      }
      else {
         /* GL_UPPER_LEFT */
         t0 = 1.0;
         dtdy = -1.0 / size;
      }

      ATTRIB_LOOP_BEGIN
         if (attr >= FRAG_ATTRIB_TEX0 && attr < FRAG_ATTRIB_VAR0) {
            const GLuint u = attr - FRAG_ATTRIB_TEX0;
            /* a texcoord */
            if (ctx->Point.CoordReplace[u]) {
               tCoords[numTcoords++] = attr;

               if (ctx->Point.SpriteRMode == GL_ZERO)
                  r = 0.0F;
               else if (ctx->Point.SpriteRMode == GL_S)
                  r = vert->attrib[attr][0];
               else /* GL_R */
                  r = vert->attrib[attr][2];

               span.attrStart[attr][0] = s;
               span.attrStart[attr][1] = 0.0; /* overwritten below */
               span.attrStart[attr][2] = r;
               span.attrStart[attr][3] = 1.0;

               span.attrStepX[attr][0] = dsdx;
               span.attrStepX[attr][1] = 0.0;
               span.attrStepX[attr][2] = 0.0;
               span.attrStepX[attr][3] = 0.0;

               span.attrStepY[attr][0] = 0.0;
               span.attrStepY[attr][1] = dtdy;
               span.attrStepY[attr][2] = 0.0;
               span.attrStepY[attr][3] = 0.0;

               continue;
            }
         }
         else if (attr == FRAG_ATTRIB_FOGC) {
            /* GLSL gl_PointCoord is stored in fog.zw */
            span.attrStart[FRAG_ATTRIB_FOGC][2] = 0.0;
            span.attrStart[FRAG_ATTRIB_FOGC][3] = 0.0; /* t0 set below */
            span.attrStepX[FRAG_ATTRIB_FOGC][2] = dsdx;
            span.attrStepX[FRAG_ATTRIB_FOGC][3] = 0.0;
            span.attrStepY[FRAG_ATTRIB_FOGC][2] = 0.0;
            span.attrStepY[FRAG_ATTRIB_FOGC][3] = dtdy;
            tCoords[numTcoords++] = FRAG_ATTRIB_FOGC;
            continue;
         }
         /* use vertex's texcoord/attrib */
         COPY_4V(span.attrStart[attr], vert->attrib[attr]);
         ASSIGN_4V(span.attrStepX[attr], 0, 0, 0, 0);
         ASSIGN_4V(span.attrStepY[attr], 0, 0, 0, 0);
      ATTRIB_LOOP_END;
   }

   /* compute pos, bounds and render */
   {
      const GLfloat x = vert->attrib[FRAG_ATTRIB_WPOS][0];
      const GLfloat y = vert->attrib[FRAG_ATTRIB_WPOS][1];
      GLint iSize = (GLint) (size + 0.5F);
      GLint xmin, xmax, ymin, ymax, iy;
      GLint iRadius;
      GLfloat tcoord = t0;

      iSize = MAX2(1, iSize);
      iRadius = iSize / 2;

      if (iSize & 1) {
         /* odd size */
         xmin = (GLint) (x - iRadius);
         xmax = (GLint) (x + iRadius);
         ymin = (GLint) (y - iRadius);
         ymax = (GLint) (y + iRadius);
      }
      else {
         /* even size */
         /* 0.501 factor allows conformance to pass */
         xmin = (GLint) (x + 0.501) - iRadius;
         xmax = xmin + iSize - 1;
         ymin = (GLint) (y + 0.501) - iRadius;
         ymax = ymin + iSize - 1;
      }

      /* render spans */
      for (iy = ymin; iy <= ymax; iy++) {
         GLuint i;
         /* setup texcoord T for this row */
         for (i = 0; i < numTcoords; i++) {
            if (tCoords[i] == FRAG_ATTRIB_FOGC)
               span.attrStart[FRAG_ATTRIB_FOGC][3] = tcoord;
            else
               span.attrStart[tCoords[i]][1] = tcoord;
         }

         /* these might get changed by span clipping */
         span.x = xmin;
         span.y = iy;
         span.end = xmax - xmin + 1;

         _swrast_write_rgba_span(ctx, &span);

         tcoord += dtdy;
      }
   }
}


/**
 * Draw smooth/antialiased point.  RGB or CI mode.
 */
static void
smooth_point(GLcontext *ctx, const SWvertex *vert)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean ciMode = !ctx->Visual.rgbMode;
   SWspan span;
   GLfloat size, alphaAtten;

   CULL_INVALID(vert);

   /* z coord */
   if (ctx->DrawBuffer->Visual.depthBits <= 16)
      span.z = FloatToFixed(vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   else
      span.z = (GLuint) (vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   span.zStep = 0;

   size = get_size(ctx, vert, GL_TRUE);

   /* alpha attenuation / fade factor */
   if (ctx->Multisample._Enabled) {
      if (vert->pointSize >= ctx->Point.Threshold) {
         alphaAtten = 1.0F;
      }
      else {
         GLfloat dsize = vert->pointSize / ctx->Point.Threshold;
         alphaAtten = dsize * dsize;
      }
   }
   else {
      alphaAtten = 1.0;
   }
   (void) alphaAtten; /* not used */

   /* span init */
   INIT_SPAN(span, GL_POINT);
   span.interpMask = SPAN_Z | SPAN_RGBA;
   span.arrayMask = SPAN_COVERAGE | SPAN_MASK;

   span.facing = swrast->PointLineFacing;

   span.red   = ChanToFixed(vert->color[0]);
   span.green = ChanToFixed(vert->color[1]);
   span.blue  = ChanToFixed(vert->color[2]);
   span.alpha = ChanToFixed(vert->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;

   /* need these for fragment programs */
   span.attrStart[FRAG_ATTRIB_WPOS][3] = 1.0F;
   span.attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0F;
   span.attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0F;

   ATTRIB_LOOP_BEGIN
      COPY_4V(span.attrStart[attr], vert->attrib[attr]);
      ASSIGN_4V(span.attrStepX[attr], 0, 0, 0, 0);
      ASSIGN_4V(span.attrStepY[attr], 0, 0, 0, 0);
   ATTRIB_LOOP_END

   /* compute pos, bounds and render */
   {
      const GLfloat x = vert->attrib[FRAG_ATTRIB_WPOS][0];
      const GLfloat y = vert->attrib[FRAG_ATTRIB_WPOS][1];
      const GLfloat radius = 0.5F * size;
      const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
      const GLfloat rmax = radius + 0.7071F;
      const GLfloat rmin2 = MAX2(0.0F, rmin * rmin);
      const GLfloat rmax2 = rmax * rmax;
      const GLfloat cscale = 1.0F / (rmax2 - rmin2);
      const GLint xmin = (GLint) (x - radius);
      const GLint xmax = (GLint) (x + radius);
      const GLint ymin = (GLint) (y - radius);
      const GLint ymax = (GLint) (y + radius);
      GLint ix, iy;

      for (iy = ymin; iy <= ymax; iy++) {

         /* these might get changed by span clipping */
         span.x = xmin;
         span.y = iy;
         span.end = xmax - xmin + 1;

         /* compute coverage for each pixel in span */
         for (ix = xmin; ix <= xmax; ix++) {
            const GLfloat dx = ix - x + 0.5F;
            const GLfloat dy = iy - y + 0.5F;
            const GLfloat dist2 = dx * dx + dy * dy;
            GLfloat coverage;

            if (dist2 < rmax2) {
               if (dist2 >= rmin2) {
                  /* compute partial coverage */
                  coverage = 1.0F - (dist2 - rmin2) * cscale;
                  if (ciMode) {
                     /* coverage in [0,15] */
                     coverage *= 15.0;
                  }
               }
               else {
                  /* full coverage */
                  coverage = 1.0F;
               }
               span.array->mask[ix - xmin] = 1;
            }
            else {
               /* zero coverage - fragment outside the radius */
               coverage = 0.0;
               span.array->mask[ix - xmin] = 0;
            }
            span.array->coverage[ix - xmin] = coverage;
         }

         /* render span */
         _swrast_write_rgba_span(ctx, &span);

      }
   }
}


/**
 * Draw large (size >= 1) non-AA point.  RGB or CI mode.
 */
static void
large_point(GLcontext *ctx, const SWvertex *vert)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean ciMode = !ctx->Visual.rgbMode;
   SWspan span;
   GLfloat size;

   CULL_INVALID(vert);

   /* z coord */
   if (ctx->DrawBuffer->Visual.depthBits <= 16)
      span.z = FloatToFixed(vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   else
      span.z = (GLuint) (vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);
   span.zStep = 0;

   size = get_size(ctx, vert, GL_FALSE);

   /* span init */
   INIT_SPAN(span, GL_POINT);
   span.arrayMask = SPAN_XY;
   span.facing = swrast->PointLineFacing;

   if (ciMode) {
      span.interpMask = SPAN_Z | SPAN_INDEX;
      span.index = FloatToFixed(vert->attrib[FRAG_ATTRIB_CI][0]);
      span.indexStep = 0;
   }
   else {
      span.interpMask = SPAN_Z | SPAN_RGBA;
      span.red   = ChanToFixed(vert->color[0]);
      span.green = ChanToFixed(vert->color[1]);
      span.blue  = ChanToFixed(vert->color[2]);
      span.alpha = ChanToFixed(vert->color[3]);
      span.redStep = 0;
      span.greenStep = 0;
      span.blueStep = 0;
      span.alphaStep = 0;
   }

   /* need these for fragment programs */
   span.attrStart[FRAG_ATTRIB_WPOS][3] = 1.0F;
   span.attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0F;
   span.attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0F;

   ATTRIB_LOOP_BEGIN
      COPY_4V(span.attrStart[attr], vert->attrib[attr]);
      ASSIGN_4V(span.attrStepX[attr], 0, 0, 0, 0);
      ASSIGN_4V(span.attrStepY[attr], 0, 0, 0, 0);
   ATTRIB_LOOP_END

   /* compute pos, bounds and render */
   {
      const GLfloat x = vert->attrib[FRAG_ATTRIB_WPOS][0];
      const GLfloat y = vert->attrib[FRAG_ATTRIB_WPOS][1];
      GLint iSize = (GLint) (size + 0.5F);
      GLint xmin, xmax, ymin, ymax, ix, iy;
      GLint iRadius;

      iSize = MAX2(1, iSize);
      iRadius = iSize / 2;

      if (iSize & 1) {
         /* odd size */
         xmin = (GLint) (x - iRadius);
         xmax = (GLint) (x + iRadius);
         ymin = (GLint) (y - iRadius);
         ymax = (GLint) (y + iRadius);
      }
      else {
         /* even size */
         /* 0.501 factor allows conformance to pass */
         xmin = (GLint) (x + 0.501) - iRadius;
         xmax = xmin + iSize - 1;
         ymin = (GLint) (y + 0.501) - iRadius;
         ymax = ymin + iSize - 1;
      }

      /* generate fragments */
      span.end = 0;
      for (iy = ymin; iy <= ymax; iy++) {
         for (ix = xmin; ix <= xmax; ix++) {
            span.array->x[span.end] = ix;
            span.array->y[span.end] = iy;
            span.end++;
         }
      }
      assert(span.end <= MAX_WIDTH);
      _swrast_write_rgba_span(ctx, &span);
   }
}


/**
 * Draw size=1, single-pixel point
 */
static void
pixel_point(GLcontext *ctx, const SWvertex *vert)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean ciMode = !ctx->Visual.rgbMode;
   /*
    * Note that unlike the other functions, we put single-pixel points
    * into a special span array in order to render as many points as
    * possible with a single _swrast_write_rgba_span() call.
    */
   SWspan *span = &(swrast->PointSpan);
   GLuint count;

   CULL_INVALID(vert);

   /* Span init */
   span->interpMask = 0;
   span->arrayMask = SPAN_XY | SPAN_Z;
   if (ciMode)
      span->arrayMask |= SPAN_INDEX;
   else
      span->arrayMask |= SPAN_RGBA;
   /*span->arrayMask |= SPAN_LAMBDA;*/
   span->arrayAttribs = swrast->_ActiveAttribMask; /* we'll produce these vals */

   /* need these for fragment programs */
   span->attrStart[FRAG_ATTRIB_WPOS][3] = 1.0F;
   span->attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0F;
   span->attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0F;

   /* check if we need to flush */
   if (span->end >= MAX_WIDTH ||
       (swrast->_RasterMask & (BLEND_BIT | LOGIC_OP_BIT | MASKING_BIT)) ||
       span->facing != swrast->PointLineFacing) {
      if (span->end > 0) {
         if (ciMode)
            _swrast_write_index_span(ctx, span);
         else
            _swrast_write_rgba_span(ctx, span);
         span->end = 0;
      }
   }

   count = span->end;

   span->facing = swrast->PointLineFacing;

   /* fragment attributes */
   if (ciMode) {
      span->array->index[count] = (GLuint) vert->attrib[FRAG_ATTRIB_CI][0];
   }
   else {
      span->array->rgba[count][RCOMP] = vert->color[0];
      span->array->rgba[count][GCOMP] = vert->color[1];
      span->array->rgba[count][BCOMP] = vert->color[2];
      span->array->rgba[count][ACOMP] = vert->color[3];
   }
   ATTRIB_LOOP_BEGIN
      COPY_4V(span->array->attribs[attr][count], vert->attrib[attr]);
   ATTRIB_LOOP_END

   /* fragment position */
   span->array->x[count] = (GLint) vert->attrib[FRAG_ATTRIB_WPOS][0];
   span->array->y[count] = (GLint) vert->attrib[FRAG_ATTRIB_WPOS][1];
   span->array->z[count] = (GLint) (vert->attrib[FRAG_ATTRIB_WPOS][2] + 0.5F);

   span->end = count + 1;
   ASSERT(span->end <= MAX_WIDTH);
}


/**
 * Add specular color to primary color, draw point, restore original
 * primary color.
 */
void
_swrast_add_spec_terms_point(GLcontext *ctx, const SWvertex *v0)
{
   SWvertex *ncv0 = (SWvertex *) v0; /* cast away const */
   GLfloat rSum, gSum, bSum;
   GLchan cSave[4];

   /* save */
   COPY_CHAN4(cSave, ncv0->color);
   /* sum */
   rSum = CHAN_TO_FLOAT(ncv0->color[0]) + ncv0->attrib[FRAG_ATTRIB_COL1][0];
   gSum = CHAN_TO_FLOAT(ncv0->color[1]) + ncv0->attrib[FRAG_ATTRIB_COL1][1];
   bSum = CHAN_TO_FLOAT(ncv0->color[2]) + ncv0->attrib[FRAG_ATTRIB_COL1][2];
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[0], rSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[1], gSum);
   UNCLAMPED_FLOAT_TO_CHAN(ncv0->color[2], bSum);
   /* draw */
   SWRAST_CONTEXT(ctx)->SpecPoint(ctx, ncv0);
   /* restore */
   COPY_CHAN4(ncv0->color, cSave);
}


/**
 * Examine current state to determine which point drawing function to use.
 */
void
_swrast_choose_point(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Point.PointSprite) {
         swrast->Point = sprite_point;
      }
      else if (ctx->Point.SmoothFlag) {
         swrast->Point = smooth_point;
      }
      else if (ctx->Point.Size > 1.0 ||
               ctx->Point._Attenuated ||
               ctx->VertexProgram.PointSizeEnabled) {
         swrast->Point = large_point;
      }
      else {
         swrast->Point = pixel_point;
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      swrast->Point = _swrast_feedback_point;
   }
   else {
      /* GL_SELECT mode */
      swrast->Point = _swrast_select_point;
   }
}
