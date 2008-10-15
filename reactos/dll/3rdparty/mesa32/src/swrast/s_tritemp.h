/*
 * Mesa 3-D graphics library
 * Version:  7.0
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

/*
 * Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom triangle rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interpolated across the triangle:
 *    INTERP_Z        - if defined, interpolate integer Z values
 *    INTERP_RGB      - if defined, interpolate integer RGB values
 *    INTERP_ALPHA    - if defined, interpolate integer Alpha values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_INT_TEX  - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping, without
 *                         perspective correction)
 *    INTERP_ATTRIBS  - if defined, interpolate arbitrary attribs (texcoords,
 *                         varying vars, etc)  This also causes W to be
 *                         computed for perspective correction).
 *
 * When one can directly address pixels in the color buffer the following
 * macros can be defined and used to compute pixel addresses during
 * rasterization (see pRow):
 *    PIXEL_TYPE          - the datatype of a pixel (GLubyte, GLushort, GLuint)
 *    BYTES_PER_ROW       - number of bytes per row in the color buffer
 *    PIXEL_ADDRESS(X,Y)  - returns the address of pixel at (X,Y) where
 *                          Y==0 at bottom of screen and increases upward.
 *
 * Similarly, for direct depth buffer access, this type is used for depth
 * buffer addressing (see zRow):
 *    DEPTH_TYPE          - either GLushort or GLuint
 *
 * Optionally, one may provide one-time setup code per triangle:
 *    SETUP_CODE    - code which is to be executed once per triangle
 *
 * The following macro MUST be defined:
 *    RENDER_SPAN(span) - code to write a span of pixels.
 *
 * This code was designed for the origin to be in the lower-left corner.
 *
 * Inspired by triangle rasterizer code written by Allen Akin.  Thanks Allen!
 *
 *
 * Some notes on rasterization accuracy:
 *
 * This code uses fixed point arithmetic (the GLfixed type) to iterate
 * over the triangle edges and interpolate ancillary data (such as Z,
 * color, secondary color, etc).  The number of fractional bits in
 * GLfixed and the value of SUB_PIXEL_BITS has a direct bearing on the
 * accuracy of rasterization.
 *
 * If SUB_PIXEL_BITS=4 then we'll snap the vertices to the nearest
 * 1/16 of a pixel.  If we're walking up a long, nearly vertical edge
 * (dx=1/16, dy=1024) we'll need 4 + 10 = 14 fractional bits in
 * GLfixed to walk the edge without error.  If the maximum viewport
 * height is 4K pixels, then we'll need 4 + 12 = 16 fractional bits.
 *
 * Historically, Mesa has used 11 fractional bits in GLfixed, snaps
 * vertices to 1/16 pixel and allowed a maximum viewport height of 2K
 * pixels.  11 fractional bits is actually insufficient for accurately
 * rasterizing some triangles.  More recently, the maximum viewport
 * height was increased to 4K pixels.  Thus, Mesa should be using 16
 * fractional bits in GLfixed.  Unfortunately, there may be some issues
 * with setting FIXED_FRAC_BITS=16, such as multiplication overflow.
 * This will have to be examined in some detail...
 *
 * For now, if you find rasterization errors, particularly with tall,
 * sliver triangles, try increasing FIXED_FRAC_BITS and/or decreasing
 * SUB_PIXEL_BITS.
 */


/*
 * Some code we unfortunately need to prevent negative interpolated colors.
 */
#ifndef CLAMP_INTERPOLANT
#define CLAMP_INTERPOLANT(CHANNEL, CHANNELSTEP, LEN)		\
do {								\
   GLfixed endVal = span.CHANNEL + (LEN) * span.CHANNELSTEP;	\
   if (endVal < 0) {						\
      span.CHANNEL -= endVal;					\
   }								\
   if (span.CHANNEL < 0) {					\
      span.CHANNEL = 0;						\
   }								\
} while (0)
#endif


static void NAME(GLcontext *ctx, const SWvertex *v0,
                                 const SWvertex *v1,
                                 const SWvertex *v2 )
{
   typedef struct {
      const SWvertex *v0, *v1;   /* Y(v0) < Y(v1) */
      GLfloat dx;	/* X(v1) - X(v0) */
      GLfloat dy;	/* Y(v1) - Y(v0) */
      GLfloat dxdy;	/* dx/dy */
      GLfixed fdxdy;	/* dx/dy in fixed-point */
      GLfloat adjy;	/* adjust from v[0]->fy to fsy, scaled */
      GLfixed fsx;	/* first sample point x coord */
      GLfixed fsy;
      GLfixed fx0;	/* fixed pt X of lower endpoint */
      GLint lines;	/* number of lines to be sampled on this edge */
   } EdgeT;

   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
#ifdef INTERP_Z
   const GLint depthBits = ctx->DrawBuffer->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
   const GLfloat maxDepth = ctx->DrawBuffer->_DepthMaxF;
#define FixedToDepth(F)  ((F) >> fixedToDepthShift)
#endif
   EdgeT eMaj, eTop, eBot;
   GLfloat oneOverArea;
   const SWvertex *vMin, *vMid, *vMax;  /* Y(vMin)<=Y(vMid)<=Y(vMax) */
   GLfloat bf = SWRAST_CONTEXT(ctx)->_BackfaceSign;
   const GLint snapMask = ~((FIXED_ONE / (1 << SUB_PIXEL_BITS)) - 1); /* for x/y coord snapping */
   GLfixed vMin_fx, vMin_fy, vMid_fx, vMid_fy, vMax_fx, vMax_fy;

   SWspan span;

   (void) swrast;

   INIT_SPAN(span, GL_POLYGON);
   span.y = 0; /* silence warnings */

#ifdef INTERP_Z
   (void) fixedToDepthShift;
#endif

   /*
   printf("%s()\n", __FUNCTION__);
   printf("  %g, %g, %g\n",
          v0->attrib[FRAG_ATTRIB_WPOS][0],
          v0->attrib[FRAG_ATTRIB_WPOS][1],
          v0->attrib[FRAG_ATTRIB_WPOS][2]);
   printf("  %g, %g, %g\n",
          v1->attrib[FRAG_ATTRIB_WPOS][0],
          v1->attrib[FRAG_ATTRIB_WPOS][1],
          v1->attrib[FRAG_ATTRIB_WPOS][2]);
   printf("  %g, %g, %g\n",
          v2->attrib[FRAG_ATTRIB_WPOS][0],
          v2->attrib[FRAG_ATTRIB_WPOS][1],
          v2->attrib[FRAG_ATTRIB_WPOS][2]);
   */

   /* Compute fixed point x,y coords w/ half-pixel offsets and snapping.
    * And find the order of the 3 vertices along the Y axis.
    */
   {
      const GLfixed fy0 = FloatToFixed(v0->attrib[FRAG_ATTRIB_WPOS][1] - 0.5F) & snapMask;
      const GLfixed fy1 = FloatToFixed(v1->attrib[FRAG_ATTRIB_WPOS][1] - 0.5F) & snapMask;
      const GLfixed fy2 = FloatToFixed(v2->attrib[FRAG_ATTRIB_WPOS][1] - 0.5F) & snapMask;
      if (fy0 <= fy1) {
         if (fy1 <= fy2) {
            /* y0 <= y1 <= y2 */
            vMin = v0;   vMid = v1;   vMax = v2;
            vMin_fy = fy0;  vMid_fy = fy1;  vMax_fy = fy2;
         }
         else if (fy2 <= fy0) {
            /* y2 <= y0 <= y1 */
            vMin = v2;   vMid = v0;   vMax = v1;
            vMin_fy = fy2;  vMid_fy = fy0;  vMax_fy = fy1;
         }
         else {
            /* y0 <= y2 <= y1 */
            vMin = v0;   vMid = v2;   vMax = v1;
            vMin_fy = fy0;  vMid_fy = fy2;  vMax_fy = fy1;
            bf = -bf;
         }
      }
      else {
         if (fy0 <= fy2) {
            /* y1 <= y0 <= y2 */
            vMin = v1;   vMid = v0;   vMax = v2;
            vMin_fy = fy1;  vMid_fy = fy0;  vMax_fy = fy2;
            bf = -bf;
         }
         else if (fy2 <= fy1) {
            /* y2 <= y1 <= y0 */
            vMin = v2;   vMid = v1;   vMax = v0;
            vMin_fy = fy2;  vMid_fy = fy1;  vMax_fy = fy0;
            bf = -bf;
         }
         else {
            /* y1 <= y2 <= y0 */
            vMin = v1;   vMid = v2;   vMax = v0;
            vMin_fy = fy1;  vMid_fy = fy2;  vMax_fy = fy0;
         }
      }

      /* fixed point X coords */
      vMin_fx = FloatToFixed(vMin->attrib[FRAG_ATTRIB_WPOS][0] + 0.5F) & snapMask;
      vMid_fx = FloatToFixed(vMid->attrib[FRAG_ATTRIB_WPOS][0] + 0.5F) & snapMask;
      vMax_fx = FloatToFixed(vMax->attrib[FRAG_ATTRIB_WPOS][0] + 0.5F) & snapMask;
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[upper] - vertex[lower] */
   eMaj.dx = FixedToFloat(vMax_fx - vMin_fx);
   eMaj.dy = FixedToFloat(vMax_fy - vMin_fy);
   eTop.dx = FixedToFloat(vMax_fx - vMid_fx);
   eTop.dy = FixedToFloat(vMax_fy - vMid_fy);
   eBot.dx = FixedToFloat(vMid_fx - vMin_fx);
   eBot.dy = FixedToFloat(vMid_fy - vMin_fy);

   /* compute area, oneOverArea and perform backface culling */
   {
      const GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;

      if (IS_INF_OR_NAN(area) || area == 0.0F)
         return;

      if (area * bf * swrast->_BackfaceCullSign < 0.0)
         return;

      oneOverArea = 1.0F / area;

      /* 0 = front, 1 = back */
      span.facing = oneOverArea * bf > 0.0F;
   }

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(FixedCeil(vMax_fy - eMaj.fsy));
      if (eMaj.lines > 0) {
         eMaj.dxdy = eMaj.dx / eMaj.dy;
         eMaj.fdxdy = SignedFloatToFixed(eMaj.dxdy);
         eMaj.adjy = (GLfloat) (eMaj.fsy - vMin_fy);  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (GLfixed) (eMaj.adjy * eMaj.dxdy);
      }
      else {
         return;  /*CULLED*/
      }

      eTop.fsy = FixedCeil(vMid_fy);
      eTop.lines = FixedToInt(FixedCeil(vMax_fy - eTop.fsy));
      if (eTop.lines > 0) {
         eTop.dxdy = eTop.dx / eTop.dy;
         eTop.fdxdy = SignedFloatToFixed(eTop.dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * eTop.dxdy);
      }

      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(FixedCeil(vMid_fy - eBot.fsy));
      if (eBot.lines > 0) {
         eBot.dxdy = eBot.dx / eBot.dy;
         eBot.fdxdy = SignedFloatToFixed(eBot.dxdy);
         eBot.adjy = (GLfloat) (eBot.fsy - vMin_fy);  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (GLfixed) (eBot.adjy * eBot.dxdy);
      }
   }

   /*
    * Conceptually, we view a triangle as two subtriangles
    * separated by a perfectly horizontal line.  The edge that is
    * intersected by this line is one with maximal absolute dy; we
    * call it a ``major'' edge.  The other two edges are the
    * ``top'' edge (for the upper subtriangle) and the ``bottom''
    * edge (for the lower subtriangle).  If either of these two
    * edges is horizontal or very close to horizontal, the
    * corresponding subtriangle might cover zero sample points;
    * we take care to handle such cases, for performance as well
    * as correctness.
    *
    * By stepping rasterization parameters along the major edge,
    * we can avoid recomputing them at the discontinuity where
    * the top and bottom edges meet.  However, this forces us to
    * be able to scan both left-to-right and right-to-left.
    * Also, we must determine whether the major edge is at the
    * left or right side of the triangle.  We do this by
    * computing the magnitude of the cross-product of the major
    * and top edges.  Since this magnitude depends on the sine of
    * the angle between the two edges, its sign tells us whether
    * we turn to the left or to the right when travelling along
    * the major edge to the top edge, and from this we infer
    * whether the major edge is on the left or the right.
    *
    * Serendipitously, this cross-product magnitude is also a
    * value we need to compute the iteration parameter
    * derivatives for the triangle, and it can be used to perform
    * backface culling because its sign tells us whether the
    * triangle is clockwise or counterclockwise.  In this code we
    * refer to it as ``area'' because it's also proportional to
    * the pixel area of the triangle.
    */

   {
      GLint scan_from_left_to_right;  /* true if scanning left-to-right */
#ifdef INTERP_INDEX
      GLfloat didx, didy;
#endif

      /*
       * Execute user-supplied setup code
       */
#ifdef SETUP_CODE
      SETUP_CODE
#endif

      scan_from_left_to_right = (oneOverArea < 0.0F);


      /* compute d?/dx and d?/dy derivatives */
#ifdef INTERP_Z
      span.interpMask |= SPAN_Z;
      {
         GLfloat eMaj_dz = vMax->attrib[FRAG_ATTRIB_WPOS][2] - vMin->attrib[FRAG_ATTRIB_WPOS][2];
         GLfloat eBot_dz = vMid->attrib[FRAG_ATTRIB_WPOS][2] - vMin->attrib[FRAG_ATTRIB_WPOS][2];
         span.attrStepX[FRAG_ATTRIB_WPOS][2] = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (span.attrStepX[FRAG_ATTRIB_WPOS][2] > maxDepth ||
             span.attrStepX[FRAG_ATTRIB_WPOS][2] < -maxDepth) {
            /* probably a sliver triangle */
            span.attrStepX[FRAG_ATTRIB_WPOS][2] = 0.0;
            span.attrStepY[FRAG_ATTRIB_WPOS][2] = 0.0;
         }
         else {
            span.attrStepY[FRAG_ATTRIB_WPOS][2] = oneOverArea * (eMaj.dx * eBot_dz - eMaj_dz * eBot.dx);
         }
         if (depthBits <= 16)
            span.zStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_WPOS][2]);
         else
            span.zStep = (GLint) span.attrStepX[FRAG_ATTRIB_WPOS][2];
      }
#endif
#ifdef INTERP_RGB
      span.interpMask |= SPAN_RGBA;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_dr = (GLfloat) (vMax->color[RCOMP] - vMin->color[RCOMP]);
         GLfloat eBot_dr = (GLfloat) (vMid->color[RCOMP] - vMin->color[RCOMP]);
         GLfloat eMaj_dg = (GLfloat) (vMax->color[GCOMP] - vMin->color[GCOMP]);
         GLfloat eBot_dg = (GLfloat) (vMid->color[GCOMP] - vMin->color[GCOMP]);
         GLfloat eMaj_db = (GLfloat) (vMax->color[BCOMP] - vMin->color[BCOMP]);
         GLfloat eBot_db = (GLfloat) (vMid->color[BCOMP] - vMin->color[BCOMP]);
#  ifdef INTERP_ALPHA
         GLfloat eMaj_da = (GLfloat) (vMax->color[ACOMP] - vMin->color[ACOMP]);
         GLfloat eBot_da = (GLfloat) (vMid->color[ACOMP] - vMin->color[ACOMP]);
#  endif
         span.attrStepX[FRAG_ATTRIB_COL0][0] = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         span.attrStepY[FRAG_ATTRIB_COL0][0] = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL0][1] = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         span.attrStepY[FRAG_ATTRIB_COL0][1] = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL0][2] = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         span.attrStepY[FRAG_ATTRIB_COL0][2] = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
         span.redStep   = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][0]);
         span.greenStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][1]);
         span.blueStep  = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][2]);
#  ifdef INTERP_ALPHA
         span.attrStepX[FRAG_ATTRIB_COL0][3] = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         span.attrStepY[FRAG_ATTRIB_COL0][3] = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
         span.alphaStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][3]);
#  endif /* INTERP_ALPHA */
      }
      else {
         ASSERT(ctx->Light.ShadeModel == GL_FLAT);
         span.interpMask |= SPAN_FLAT;
         span.attrStepX[FRAG_ATTRIB_COL0][0] = span.attrStepY[FRAG_ATTRIB_COL0][0] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL0][1] = span.attrStepY[FRAG_ATTRIB_COL0][1] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL0][2] = span.attrStepY[FRAG_ATTRIB_COL0][2] = 0.0F;
	 span.redStep   = 0;
	 span.greenStep = 0;
	 span.blueStep  = 0;
#  ifdef INTERP_ALPHA
         span.attrStepX[FRAG_ATTRIB_COL0][3] = span.attrStepY[FRAG_ATTRIB_COL0][3] = 0.0F;
	 span.alphaStep = 0;
#  endif
      }
#endif /* INTERP_RGB */
#ifdef INTERP_INDEX
      span.interpMask |= SPAN_INDEX;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_di = vMax->attrib[FRAG_ATTRIB_CI][0] - vMin->attrib[FRAG_ATTRIB_CI][0];
         GLfloat eBot_di = vMid->attrib[FRAG_ATTRIB_CI][0] - vMin->attrib[FRAG_ATTRIB_CI][0];
         didx = oneOverArea * (eMaj_di * eBot.dy - eMaj.dy * eBot_di);
         didy = oneOverArea * (eMaj.dx * eBot_di - eMaj_di * eBot.dx);
         span.indexStep = SignedFloatToFixed(didx);
      }
      else {
         span.interpMask |= SPAN_FLAT;
         didx = didy = 0.0F;
         span.indexStep = 0;
      }
#endif
#ifdef INTERP_INT_TEX
      {
         GLfloat eMaj_ds = (vMax->attrib[FRAG_ATTRIB_TEX0][0] - vMin->attrib[FRAG_ATTRIB_TEX0][0]) * S_SCALE;
         GLfloat eBot_ds = (vMid->attrib[FRAG_ATTRIB_TEX0][0] - vMin->attrib[FRAG_ATTRIB_TEX0][0]) * S_SCALE;
         GLfloat eMaj_dt = (vMax->attrib[FRAG_ATTRIB_TEX0][1] - vMin->attrib[FRAG_ATTRIB_TEX0][1]) * T_SCALE;
         GLfloat eBot_dt = (vMid->attrib[FRAG_ATTRIB_TEX0][1] - vMin->attrib[FRAG_ATTRIB_TEX0][1]) * T_SCALE;
         span.attrStepX[FRAG_ATTRIB_TEX0][0] = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         span.attrStepY[FRAG_ATTRIB_TEX0][0] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_TEX0][1] = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         span.attrStepY[FRAG_ATTRIB_TEX0][1] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
         span.intTexStep[0] = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_TEX0][0]);
         span.intTexStep[1] = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_TEX0][1]);
      }
#endif
#ifdef INTERP_ATTRIBS
      {
         /* attrib[FRAG_ATTRIB_WPOS][3] is 1/W */
         const GLfloat wMax = vMax->attrib[FRAG_ATTRIB_WPOS][3];
         const GLfloat wMin = vMin->attrib[FRAG_ATTRIB_WPOS][3];
         const GLfloat wMid = vMid->attrib[FRAG_ATTRIB_WPOS][3];
         {
            const GLfloat eMaj_dw = wMax - wMin;
            const GLfloat eBot_dw = wMid - wMin;
            span.attrStepX[FRAG_ATTRIB_WPOS][3] = oneOverArea * (eMaj_dw * eBot.dy - eMaj.dy * eBot_dw);
            span.attrStepY[FRAG_ATTRIB_WPOS][3] = oneOverArea * (eMaj.dx * eBot_dw - eMaj_dw * eBot.dx);
         }
         ATTRIB_LOOP_BEGIN
            if (swrast->_InterpMode[attr] == GL_FLAT) {
               ASSIGN_4V(span.attrStepX[attr], 0.0, 0.0, 0.0, 0.0);
               ASSIGN_4V(span.attrStepY[attr], 0.0, 0.0, 0.0, 0.0);
            }
            else {
               GLuint c;
               for (c = 0; c < 4; c++) {
                  GLfloat eMaj_da = vMax->attrib[attr][c] * wMax - vMin->attrib[attr][c] * wMin;
                  GLfloat eBot_da = vMid->attrib[attr][c] * wMid - vMin->attrib[attr][c] * wMin;
                  span.attrStepX[attr][c] = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
                  span.attrStepY[attr][c] = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
               }
            }
         ATTRIB_LOOP_END
      }
#endif

      /*
       * We always sample at pixel centers.  However, we avoid
       * explicit half-pixel offsets in this code by incorporating
       * the proper offset in each of x and y during the
       * transformation to window coordinates.
       *
       * We also apply the usual rasterization rules to prevent
       * cracks and overlaps.  A pixel is considered inside a
       * subtriangle if it meets all of four conditions: it is on or
       * to the right of the left edge, strictly to the left of the
       * right edge, on or below the top edge, and strictly above
       * the bottom edge.  (Some edges may be degenerate.)
       *
       * The following discussion assumes left-to-right scanning
       * (that is, the major edge is on the left); the right-to-left
       * case is a straightforward variation.
       *
       * We start by finding the half-integral y coordinate that is
       * at or below the top of the triangle.  This gives us the
       * first scan line that could possibly contain pixels that are
       * inside the triangle.
       *
       * Next we creep down the major edge until we reach that y,
       * and compute the corresponding x coordinate on the edge.
       * Then we find the half-integral x that lies on or just
       * inside the edge.  This is the first pixel that might lie in
       * the interior of the triangle.  (We won't know for sure
       * until we check the other edges.)
       *
       * As we rasterize the triangle, we'll step down the major
       * edge.  For each step in y, we'll move an integer number
       * of steps in x.  There are two possible x step sizes, which
       * we'll call the ``inner'' step (guaranteed to land on the
       * edge or inside it) and the ``outer'' step (guaranteed to
       * land on the edge or outside it).  The inner and outer steps
       * differ by one.  During rasterization we maintain an error
       * term that indicates our distance from the true edge, and
       * select either the inner step or the outer step, whichever
       * gets us to the first pixel that falls inside the triangle.
       *
       * All parameters (z, red, etc.) as well as the buffer
       * addresses for color and z have inner and outer step values,
       * so that we can increment them appropriately.  This method
       * eliminates the need to adjust parameters by creeping a
       * sub-pixel amount into the triangle at each scanline.
       */

      {
         GLint subTriangle;
         GLfixed fxLeftEdge = 0, fxRightEdge = 0;
         GLfixed fdxLeftEdge = 0, fdxRightEdge = 0;
         GLfixed fError = 0, fdError = 0;
#ifdef PIXEL_ADDRESS
         PIXEL_TYPE *pRow = NULL;
         GLint dPRowOuter = 0, dPRowInner;  /* offset in bytes */
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
         struct gl_renderbuffer *zrb
            = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
         DEPTH_TYPE *zRow = NULL;
         GLint dZRowOuter = 0, dZRowInner;  /* offset in bytes */
#  endif
         GLuint zLeft = 0;
         GLfixed fdzOuter = 0, fdzInner;
#endif
#ifdef INTERP_RGB
         GLint rLeft = 0, fdrOuter = 0, fdrInner;
         GLint gLeft = 0, fdgOuter = 0, fdgInner;
         GLint bLeft = 0, fdbOuter = 0, fdbInner;
#endif
#ifdef INTERP_ALPHA
         GLint aLeft = 0, fdaOuter = 0, fdaInner;
#endif
#ifdef INTERP_INDEX
         GLfixed iLeft=0, diOuter=0, diInner;
#endif
#ifdef INTERP_INT_TEX
         GLfixed sLeft=0, dsOuter=0, dsInner;
         GLfixed tLeft=0, dtOuter=0, dtInner;
#endif
#ifdef INTERP_ATTRIBS
         GLfloat wLeft = 0, dwOuter = 0, dwInner;
         GLfloat attrLeft[FRAG_ATTRIB_MAX][4];
         GLfloat daOuter[FRAG_ATTRIB_MAX][4], daInner[FRAG_ATTRIB_MAX][4];
#endif

         for (subTriangle=0; subTriangle<=1; subTriangle++) {
            EdgeT *eLeft, *eRight;
            int setupLeft, setupRight;
            int lines;

            if (subTriangle==0) {
               /* bottom half */
               if (scan_from_left_to_right) {
                  eLeft = &eMaj;
                  eRight = &eBot;
                  lines = eRight->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
               else {
                  eLeft = &eBot;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 1;
               }
            }
            else {
               /* top half */
               if (scan_from_left_to_right) {
                  eLeft = &eMaj;
                  eRight = &eTop;
                  lines = eRight->lines;
                  setupLeft = 0;
                  setupRight = 1;
               }
               else {
                  eLeft = &eTop;
                  eRight = &eMaj;
                  lines = eLeft->lines;
                  setupLeft = 1;
                  setupRight = 0;
               }
               if (lines == 0)
                  return;
            }

            if (setupLeft && eLeft->lines > 0) {
               const SWvertex *vLower = eLeft->v0;
               const GLfixed fsy = eLeft->fsy;
               const GLfixed fsx = eLeft->fsx;  /* no fractional part */
               const GLfixed fx = FixedCeil(fsx);  /* no fractional part */
               const GLfixed adjx = (GLfixed) (fx - eLeft->fx0); /* SCALED! */
               const GLfixed adjy = (GLfixed) eLeft->adjy;      /* SCALED! */
               GLint idxOuter;
               GLfloat dxOuter;
               GLfixed fdxOuter;

               fError = fx - fsx - FIXED_ONE;
               fxLeftEdge = fsx - FIXED_EPSILON;
               fdxLeftEdge = eLeft->fdxdy;
               fdxOuter = FixedFloor(fdxLeftEdge - FIXED_EPSILON);
               fdError = fdxOuter - fdxLeftEdge + FIXED_ONE;
               idxOuter = FixedToInt(fdxOuter);
               dxOuter = (GLfloat) idxOuter;
               span.y = FixedToInt(fsy);

               /* silence warnings on some compilers */
               (void) dxOuter;
               (void) adjx;
               (void) adjy;
               (void) vLower;

#ifdef PIXEL_ADDRESS
               {
                  pRow = (PIXEL_TYPE *) PIXEL_ADDRESS(FixedToInt(fxLeftEdge), span.y);
                  dPRowOuter = -((int)BYTES_PER_ROW) + idxOuter * sizeof(PIXEL_TYPE);
                  /* negative because Y=0 at bottom and increases upward */
               }
#endif
               /*
                * Now we need the set of parameter (z, color, etc.) values at
                * the point (fx, fsy).  This gives us properly-sampled parameter
                * values that we can step from pixel to pixel.  Furthermore,
                * although we might have intermediate results that overflow
                * the normal parameter range when we step temporarily outside
                * the triangle, we shouldn't overflow or underflow for any
                * pixel that's actually inside the triangle.
                */

#ifdef INTERP_Z
               {
                  GLfloat z0 = vLower->attrib[FRAG_ATTRIB_WPOS][2];
                  if (depthBits <= 16) {
                     /* interpolate fixed-pt values */
                     GLfloat tmp = (z0 * FIXED_SCALE
                                    + span.attrStepX[FRAG_ATTRIB_WPOS][2] * adjx
                                    + span.attrStepY[FRAG_ATTRIB_WPOS][2] * adjy) + FIXED_HALF;
                     if (tmp < MAX_GLUINT / 2)
                        zLeft = (GLfixed) tmp;
                     else
                        zLeft = MAX_GLUINT / 2;
                     fdzOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_WPOS][2] +
                                                   dxOuter * span.attrStepX[FRAG_ATTRIB_WPOS][2]);
                  }
                  else {
                     /* interpolate depth values w/out scaling */
                     zLeft = (GLuint) (z0 + span.attrStepX[FRAG_ATTRIB_WPOS][2] * FixedToFloat(adjx)
                                          + span.attrStepY[FRAG_ATTRIB_WPOS][2] * FixedToFloat(adjy));
                     fdzOuter = (GLint) (span.attrStepY[FRAG_ATTRIB_WPOS][2] +
                                         dxOuter * span.attrStepX[FRAG_ATTRIB_WPOS][2]);
                  }
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *)
                    zrb->GetPointer(ctx, zrb, FixedToInt(fxLeftEdge), span.y);
                  dZRowOuter = (ctx->DrawBuffer->Width + idxOuter) * sizeof(DEPTH_TYPE);
#  endif
               }
#endif
#ifdef INTERP_RGB
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
                  rLeft = (GLint)(ChanToFixed(vLower->color[RCOMP])
                                  + span.attrStepX[FRAG_ATTRIB_COL0][0] * adjx
                                  + span.attrStepY[FRAG_ATTRIB_COL0][0] * adjy) + FIXED_HALF;
                  gLeft = (GLint)(ChanToFixed(vLower->color[GCOMP])
                                  + span.attrStepX[FRAG_ATTRIB_COL0][1] * adjx
                                  + span.attrStepY[FRAG_ATTRIB_COL0][1] * adjy) + FIXED_HALF;
                  bLeft = (GLint)(ChanToFixed(vLower->color[BCOMP])
                                  + span.attrStepX[FRAG_ATTRIB_COL0][2] * adjx
                                  + span.attrStepY[FRAG_ATTRIB_COL0][2] * adjy) + FIXED_HALF;
                  fdrOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][0]
                                                + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][0]);
                  fdgOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][1]
                                                + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][1]);
                  fdbOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][2]
                                                + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][2]);
#  ifdef INTERP_ALPHA
                  aLeft = (GLint)(ChanToFixed(vLower->color[ACOMP])
                                  + span.attrStepX[FRAG_ATTRIB_COL0][3] * adjx
                                  + span.attrStepY[FRAG_ATTRIB_COL0][3] * adjy) + FIXED_HALF;
                  fdaOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][3]
                                                + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][3]);
#  endif
               }
               else {
                  ASSERT(ctx->Light.ShadeModel == GL_FLAT);
                  rLeft = ChanToFixed(v2->color[RCOMP]);
                  gLeft = ChanToFixed(v2->color[GCOMP]);
                  bLeft = ChanToFixed(v2->color[BCOMP]);
                  fdrOuter = fdgOuter = fdbOuter = 0;
#  ifdef INTERP_ALPHA
                  aLeft = ChanToFixed(v2->color[ACOMP]);
                  fdaOuter = 0;
#  endif
               }
#endif /* INTERP_RGB */


#ifdef INTERP_INDEX
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
                  iLeft = (GLfixed)(vLower->attrib[FRAG_ATTRIB_CI][0] * FIXED_SCALE
                                 + didx * adjx + didy * adjy) + FIXED_HALF;
                  diOuter = SignedFloatToFixed(didy + dxOuter * didx);
               }
               else {
                  ASSERT(ctx->Light.ShadeModel == GL_FLAT);
                  iLeft = FloatToFixed(v2->attrib[FRAG_ATTRIB_CI][0]);
                  diOuter = 0;
               }
#endif
#ifdef INTERP_INT_TEX
               {
                  GLfloat s0, t0;
                  s0 = vLower->attrib[FRAG_ATTRIB_TEX0][0] * S_SCALE;
                  sLeft = (GLfixed)(s0 * FIXED_SCALE + span.attrStepX[FRAG_ATTRIB_TEX0][0] * adjx
                                 + span.attrStepY[FRAG_ATTRIB_TEX0][0] * adjy) + FIXED_HALF;
                  dsOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_TEX0][0]
                                               + dxOuter * span.attrStepX[FRAG_ATTRIB_TEX0][0]);

                  t0 = vLower->attrib[FRAG_ATTRIB_TEX0][1] * T_SCALE;
                  tLeft = (GLfixed)(t0 * FIXED_SCALE + span.attrStepX[FRAG_ATTRIB_TEX0][1] * adjx
                                 + span.attrStepY[FRAG_ATTRIB_TEX0][1] * adjy) + FIXED_HALF;
                  dtOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_TEX0][1]
                                               + dxOuter * span.attrStepX[FRAG_ATTRIB_TEX0][1]);
               }
#endif
#ifdef INTERP_ATTRIBS
               {
                  const GLuint attr = FRAG_ATTRIB_WPOS;
                  wLeft = vLower->attrib[FRAG_ATTRIB_WPOS][3]
                        + (span.attrStepX[attr][3] * adjx
                           + span.attrStepY[attr][3] * adjy) * (1.0F/FIXED_SCALE);
                  dwOuter = span.attrStepY[attr][3] + dxOuter * span.attrStepX[attr][3];
               }
               ATTRIB_LOOP_BEGIN
                  const GLfloat invW = vLower->attrib[FRAG_ATTRIB_WPOS][3];
                  if (swrast->_InterpMode[attr] == GL_FLAT) {
                     GLuint c;
                     for (c = 0; c < 4; c++) {
                        attrLeft[attr][c] = v2->attrib[attr][c] * invW;
                        daOuter[attr][c] = 0.0;
                     }
                  }
                  else {
                     GLuint c;
                     for (c = 0; c < 4; c++) {
                        const GLfloat a = vLower->attrib[attr][c] * invW;
                        attrLeft[attr][c] = a + (  span.attrStepX[attr][c] * adjx
                                                 + span.attrStepY[attr][c] * adjy) * (1.0F/FIXED_SCALE);
                        daOuter[attr][c] = span.attrStepY[attr][c] + dxOuter * span.attrStepX[attr][c];
                     }
                  }
               ATTRIB_LOOP_END
#endif
            } /*if setupLeft*/


            if (setupRight && eRight->lines>0) {
               fxRightEdge = eRight->fsx - FIXED_EPSILON;
               fdxRightEdge = eRight->fdxdy;
            }

            if (lines==0) {
               continue;
            }


            /* Rasterize setup */
#ifdef PIXEL_ADDRESS
            dPRowInner = dPRowOuter + sizeof(PIXEL_TYPE);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
            dZRowInner = dZRowOuter + sizeof(DEPTH_TYPE);
#  endif
            fdzInner = fdzOuter + span.zStep;
#endif
#ifdef INTERP_RGB
            fdrInner = fdrOuter + span.redStep;
            fdgInner = fdgOuter + span.greenStep;
            fdbInner = fdbOuter + span.blueStep;
#endif
#ifdef INTERP_ALPHA
            fdaInner = fdaOuter + span.alphaStep;
#endif
#ifdef INTERP_INDEX
            diInner = diOuter + span.indexStep;
#endif
#ifdef INTERP_INT_TEX
            dsInner = dsOuter + span.intTexStep[0];
            dtInner = dtOuter + span.intTexStep[1];
#endif
#ifdef INTERP_ATTRIBS
            dwInner = dwOuter + span.attrStepX[FRAG_ATTRIB_WPOS][3];
            ATTRIB_LOOP_BEGIN
               GLuint c;
               for (c = 0; c < 4; c++) {
                  daInner[attr][c] = daOuter[attr][c] + span.attrStepX[attr][c];
               }
            ATTRIB_LOOP_END
#endif

            while (lines > 0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
               const GLint right = FixedToInt(fxRightEdge);
               span.x = FixedToInt(fxLeftEdge);
               if (right <= span.x)
                  span.end = 0;
               else
                  span.end = right - span.x;

#ifdef INTERP_Z
               span.z = zLeft;
#endif
#ifdef INTERP_RGB
               span.red = rLeft;
               span.green = gLeft;
               span.blue = bLeft;
#endif
#ifdef INTERP_ALPHA
               span.alpha = aLeft;
#endif
#ifdef INTERP_INDEX
               span.index = iLeft;
#endif
#ifdef INTERP_INT_TEX
               span.intTex[0] = sLeft;
               span.intTex[1] = tLeft;
#endif

#ifdef INTERP_ATTRIBS
               span.attrStart[FRAG_ATTRIB_WPOS][3] = wLeft;
               ATTRIB_LOOP_BEGIN
                  GLuint c;
                  for (c = 0; c < 4; c++) {
                     span.attrStart[attr][c] = attrLeft[attr][c];
                  }
               ATTRIB_LOOP_END
#endif

               /* This is where we actually generate fragments */
               /* XXX the test for span.y > 0 _shouldn't_ be needed but
                * it fixes a problem on 64-bit Opterons (bug 4842).
                */
               if (span.end > 0 && span.y >= 0) {
                  const GLint len = span.end - 1;
                  (void) len;
#ifdef INTERP_RGB
                  CLAMP_INTERPOLANT(red, redStep, len);
                  CLAMP_INTERPOLANT(green, greenStep, len);
                  CLAMP_INTERPOLANT(blue, blueStep, len);
#endif
#ifdef INTERP_ALPHA
                  CLAMP_INTERPOLANT(alpha, alphaStep, len);
#endif
#ifdef INTERP_INDEX
                  CLAMP_INTERPOLANT(index, indexStep, len);
#endif
                  {
                     RENDER_SPAN( span );
                  }
               }

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               span.y++;
               lines--;

               fxLeftEdge += fdxLeftEdge;
               fxRightEdge += fdxRightEdge;

               fError += fdError;
               if (fError >= 0) {
                  fError -= FIXED_ONE;

#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte *) pRow + dPRowOuter);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte *) zRow + dZRowOuter);
#  endif
                  zLeft += fdzOuter;
#endif
#ifdef INTERP_RGB
                  rLeft += fdrOuter;
                  gLeft += fdgOuter;
                  bLeft += fdbOuter;
#endif
#ifdef INTERP_ALPHA
                  aLeft += fdaOuter;
#endif
#ifdef INTERP_INDEX
                  iLeft += diOuter;
#endif
#ifdef INTERP_INT_TEX
                  sLeft += dsOuter;
                  tLeft += dtOuter;
#endif
#ifdef INTERP_ATTRIBS
                  wLeft += dwOuter;
                  ATTRIB_LOOP_BEGIN
                     GLuint c;
                     for (c = 0; c < 4; c++) {
                        attrLeft[attr][c] += daOuter[attr][c];
                     }
                  ATTRIB_LOOP_END
#endif
               }
               else {
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte *) pRow + dPRowInner);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte *) zRow + dZRowInner);
#  endif
                  zLeft += fdzInner;
#endif
#ifdef INTERP_RGB
                  rLeft += fdrInner;
                  gLeft += fdgInner;
                  bLeft += fdbInner;
#endif
#ifdef INTERP_ALPHA
                  aLeft += fdaInner;
#endif
#ifdef INTERP_INDEX
                  iLeft += diInner;
#endif
#ifdef INTERP_INT_TEX
                  sLeft += dsInner;
                  tLeft += dtInner;
#endif
#ifdef INTERP_ATTRIBS
                  wLeft += dwInner;
                  ATTRIB_LOOP_BEGIN
                     GLuint c;
                     for (c = 0; c < 4; c++) {
                        attrLeft[attr][c] += daInner[attr][c];
                     }
                  ATTRIB_LOOP_END
#endif
               }
            } /*while lines>0*/

         } /* for subTriangle */

      }
   }
}

#undef SETUP_CODE
#undef RENDER_SPAN

#undef PIXEL_TYPE
#undef BYTES_PER_ROW
#undef PIXEL_ADDRESS
#undef DEPTH_TYPE

#undef INTERP_Z
#undef INTERP_RGB
#undef INTERP_ALPHA
#undef INTERP_INDEX
#undef INTERP_INT_TEX
#undef INTERP_ATTRIBS

#undef S_SCALE
#undef T_SCALE

#undef FixedToDepth

#undef NAME
