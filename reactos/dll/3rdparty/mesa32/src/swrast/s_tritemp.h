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
 *    INTERP_Z        - if defined, interpolate vertex Z values
 *    INTERP_W        - if defined, interpolate vertex W values
 *    INTERP_FOG      - if defined, interpolate fog values
 *    INTERP_RGB      - if defined, interpolate RGB values
 *    INTERP_ALPHA    - if defined, interpolate Alpha values (req's INTERP_RGB)
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_INT_TEX  - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping)
 *    INTERP_ATTRIBS  - if defined, interpolate arbitrary attribs (texcoords,
 *                         varying vars, etc)
 *                         NOTE:  OpenGL STRQ = Mesa STUV (R was taken for red)
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
 * buffer addressing:
 *    DEPTH_TYPE          - either GLushort or GLuint
 *
 * Optionally, one may provide one-time setup code per triangle:
 *    SETUP_CODE    - code which is to be executed once per triangle
 *    CLEANUP_CODE    - code to execute at end of triangle
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
 * ColorTemp is used for intermediate color values.
 */
#if CHAN_TYPE == GL_FLOAT
#define ColorTemp GLfloat
#else
#define ColorTemp GLint  /* same as GLfixed */
#endif


/*
 * Walk triangle edges with GLfixed or GLdouble
 */
#if TRIANGLE_WALK_DOUBLE
#define GLinterp        GLdouble
#define InterpToInt(X)  ((GLint) (X))
#define INTERP_ONE      1.0
#else
#define GLinterp        GLfixed
#define InterpToInt(X)  FixedToInt(X)
#define INTERP_ONE      FIXED_ONE
#endif


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
#if TRIANGLE_WALK_DOUBLE
      GLdouble dx;	/* X(v1) - X(v0) */
      GLdouble dy;	/* Y(v1) - Y(v0) */
      GLdouble dxdy;	/* dx/dy */
      GLdouble adjy;	/* adjust from v[0]->fy to fsy, scaled */
      GLdouble fsx;	/* first sample point x coord */
      GLdouble fsy;
      GLdouble fx0;	/*X of lower endpoint */
#else
      GLfloat dx;	/* X(v1) - X(v0) */
      GLfloat dy;	/* Y(v1) - Y(v0) */
      GLfloat dxdy;	/* dx/dy */
      GLfixed fdxdy;	/* dx/dy in fixed-point */
      GLfloat adjy;	/* adjust from v[0]->fy to fsy, scaled */
      GLfixed fsx;	/* first sample point x coord */
      GLfixed fsy;
      GLfixed fx0;	/* fixed pt X of lower endpoint */
#endif
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
#if !TRIANGLE_WALK_DOUBLE
   const GLint snapMask = ~((FIXED_ONE / (1 << SUB_PIXEL_BITS)) - 1); /* for x/y coord snapping */
#endif
   GLinterp vMin_fx, vMin_fy, vMid_fx, vMid_fy, vMax_fx, vMax_fy;

   SWspan span;

   (void) swrast;

   INIT_SPAN(span, GL_POLYGON, 0, 0, 0);
   span.y = 0; /* silence warnings */

#ifdef INTERP_Z
   (void) fixedToDepthShift;
#endif

   /*
   printf("%s()\n", __FUNCTION__);
   printf("  %g, %g, %g\n", v0->win[0], v0->win[1], v0->win[2]);
   printf("  %g, %g, %g\n", v1->win[0], v1->win[1], v1->win[2]);
   printf("  %g, %g, %g\n", v2->win[0], v2->win[1], v2->win[2]);
   */
   /*
   ASSERT(v0->win[2] >= 0.0);
   ASSERT(v1->win[2] >= 0.0);
   ASSERT(v2->win[2] >= 0.0);
   */
   /* Compute fixed point x,y coords w/ half-pixel offsets and snapping.
    * And find the order of the 3 vertices along the Y axis.
    */
   {
#if TRIANGLE_WALK_DOUBLE
      const GLdouble fy0 = v0->win[1] - 0.5;
      const GLdouble fy1 = v1->win[1] - 0.5;
      const GLdouble fy2 = v2->win[1] - 0.5;
#else
      const GLfixed fy0 = FloatToFixed(v0->win[1] - 0.5F) & snapMask;
      const GLfixed fy1 = FloatToFixed(v1->win[1] - 0.5F) & snapMask;
      const GLfixed fy2 = FloatToFixed(v2->win[1] - 0.5F) & snapMask;
#endif
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
#if TRIANGLE_WALK_DOUBLE
      vMin_fx = vMin->win[0] + 0.5;
      vMid_fx = vMid->win[0] + 0.5;
      vMax_fx = vMax->win[0] + 0.5;
#else
      vMin_fx = FloatToFixed(vMin->win[0] + 0.5F) & snapMask;
      vMid_fx = FloatToFixed(vMid->win[0] + 0.5F) & snapMask;
      vMax_fx = FloatToFixed(vMax->win[0] + 0.5F) & snapMask;
#endif
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[upper] - vertex[lower] */
#if TRIANGLE_WALK_DOUBLE
   eMaj.dx = vMax_fx - vMin_fx;
   eMaj.dy = vMax_fy - vMin_fy;
   eTop.dx = vMax_fx - vMid_fx;
   eTop.dy = vMax_fy - vMid_fy;
   eBot.dx = vMid_fx - vMin_fx;
   eBot.dy = vMid_fy - vMin_fy;
#else
   eMaj.dx = FixedToFloat(vMax_fx - vMin_fx);
   eMaj.dy = FixedToFloat(vMax_fy - vMin_fy);
   eTop.dx = FixedToFloat(vMax_fx - vMid_fx);
   eTop.dy = FixedToFloat(vMax_fy - vMid_fy);
   eBot.dx = FixedToFloat(vMid_fx - vMin_fx);
   eBot.dy = FixedToFloat(vMid_fy - vMin_fy);
#endif

   /* compute area, oneOverArea and perform backface culling */
   {
#if TRIANGLE_WALK_DOUBLE
      const GLdouble area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;
#else
      const GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;
#endif
      /* Do backface culling */
      if (area * bf < 0.0)
         return;

      if (IS_INF_OR_NAN(area) || area == 0.0F)
         return;

      oneOverArea = 1.0F / area;
   }


   span.facing = ctx->_Facing; /* for 2-sided stencil test */

   /* Edge setup.  For a triangle strip these could be reused... */
   {
#if TRIANGLE_WALK_DOUBLE
      eMaj.fsy = CEILF(vMin_fy);
      eMaj.lines = (GLint) CEILF(vMax_fy - eMaj.fsy);
#else
      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(FixedCeil(vMax_fy - eMaj.fsy));
#endif
      if (eMaj.lines > 0) {
         eMaj.dxdy = eMaj.dx / eMaj.dy;
#if TRIANGLE_WALK_DOUBLE
         eMaj.adjy = (eMaj.fsy - vMin_fy) * FIXED_SCALE;  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (eMaj.adjy * eMaj.dxdy) / (GLdouble) FIXED_SCALE;
#else
         eMaj.fdxdy = SignedFloatToFixed(eMaj.dxdy);
         eMaj.adjy = (GLfloat) (eMaj.fsy - vMin_fy);  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (GLfixed) (eMaj.adjy * eMaj.dxdy);
#endif
      }
      else {
         return;  /*CULLED*/
      }

#if TRIANGLE_WALK_DOUBLE
      eTop.fsy = CEILF(vMid_fy);
      eTop.lines = (GLint) CEILF(vMax_fy - eTop.fsy);
#else
      eTop.fsy = FixedCeil(vMid_fy);
      eTop.lines = FixedToInt(FixedCeil(vMax_fy - eTop.fsy));
#endif
      if (eTop.lines > 0) {
         eTop.dxdy = eTop.dx / eTop.dy;
#if TRIANGLE_WALK_DOUBLE
         eTop.adjy = (eTop.fsy - vMid_fy) * FIXED_SCALE; /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (eTop.adjy * eTop.dxdy) / (GLdouble) FIXED_SCALE;
#else
         eTop.fdxdy = SignedFloatToFixed(eTop.dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * eTop.dxdy);
#endif
      }

#if TRIANGLE_WALK_DOUBLE
      eBot.fsy = CEILF(vMin_fy);
      eBot.lines = (GLint) CEILF(vMid_fy - eBot.fsy);
#else
      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(FixedCeil(vMid_fy - eBot.fsy));
#endif
      if (eBot.lines > 0) {
         eBot.dxdy = eBot.dx / eBot.dy;
#if TRIANGLE_WALK_DOUBLE
         eBot.adjy = (eBot.fsy - vMin_fy) * FIXED_SCALE;  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (eBot.adjy * eBot.dxdy) / (GLdouble) FIXED_SCALE;
#else
         eBot.fdxdy = SignedFloatToFixed(eBot.dxdy);
         eBot.adjy = (GLfloat) (eBot.fsy - vMin_fy);  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (GLfixed) (eBot.adjy * eBot.dxdy);
#endif
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
         GLfloat eMaj_dz = vMax->win[2] - vMin->win[2];
         GLfloat eBot_dz = vMid->win[2] - vMin->win[2];
         span.attrStepX[FRAG_ATTRIB_WPOS][2] = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (span.attrStepX[FRAG_ATTRIB_WPOS][2] > maxDepth || span.attrStepX[FRAG_ATTRIB_WPOS][2] < -maxDepth) {
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
#ifdef INTERP_W
      span.interpMask |= SPAN_W;
      {
         const GLfloat eMaj_dw = vMax->win[3] - vMin->win[3];
         const GLfloat eBot_dw = vMid->win[3] - vMin->win[3];
         span.attrStepX[FRAG_ATTRIB_WPOS][3] = oneOverArea * (eMaj_dw * eBot.dy - eMaj.dy * eBot_dw);
         span.attrStepY[FRAG_ATTRIB_WPOS][3] = oneOverArea * (eMaj.dx * eBot_dw - eMaj_dw * eBot.dx);
      }
#endif
#ifdef INTERP_FOG
      span.interpMask |= SPAN_FOG;
      {
#  ifdef INTERP_W
         const GLfloat wMax = vMax->win[3], wMin = vMin->win[3], wMid = vMid->win[3];
         const GLfloat eMaj_dfog = vMax->attrib[FRAG_ATTRIB_FOGC][0] * wMax - vMin->attrib[FRAG_ATTRIB_FOGC][0] * wMin;
         const GLfloat eBot_dfog = vMid->attrib[FRAG_ATTRIB_FOGC][0] * wMid - vMin->attrib[FRAG_ATTRIB_FOGC][0] * wMin;
#  else
         const GLfloat eMaj_dfog = vMax->attrib[FRAG_ATTRIB_FOGC][0] - vMin->attrib[FRAG_ATTRIB_FOGC][0];
         const GLfloat eBot_dfog = vMid->attrib[FRAG_ATTRIB_FOGC][0] - vMin->attrib[FRAG_ATTRIB_FOGC][0];
#  endif
         span.attrStepX[FRAG_ATTRIB_FOGC][0] = oneOverArea * (eMaj_dfog * eBot.dy - eMaj.dy * eBot_dfog);
         span.attrStepY[FRAG_ATTRIB_FOGC][0] = oneOverArea * (eMaj.dx * eBot_dfog - eMaj_dfog * eBot.dx);
      }
#endif
#ifdef INTERP_RGB
      span.interpMask |= SPAN_RGBA;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_dr = (GLfloat) ((ColorTemp) vMax->color[RCOMP] - (ColorTemp) vMin->color[RCOMP]);
         GLfloat eBot_dr = (GLfloat) ((ColorTemp) vMid->color[RCOMP] - (ColorTemp) vMin->color[RCOMP]);
         GLfloat eMaj_dg = (GLfloat) ((ColorTemp) vMax->color[GCOMP] - (ColorTemp) vMin->color[GCOMP]);
         GLfloat eBot_dg = (GLfloat) ((ColorTemp) vMid->color[GCOMP] - (ColorTemp) vMin->color[GCOMP]);
         GLfloat eMaj_db = (GLfloat) ((ColorTemp) vMax->color[BCOMP] - (ColorTemp) vMin->color[BCOMP]);
         GLfloat eBot_db = (GLfloat) ((ColorTemp) vMid->color[BCOMP] - (ColorTemp) vMin->color[BCOMP]);
#  ifdef INTERP_ALPHA
         GLfloat eMaj_da = (GLfloat) ((ColorTemp) vMax->color[ACOMP] - (ColorTemp) vMin->color[ACOMP]);
         GLfloat eBot_da = (GLfloat) ((ColorTemp) vMid->color[ACOMP] - (ColorTemp) vMin->color[ACOMP]);
#  endif
         span.attrStepX[FRAG_ATTRIB_COL0][0] = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         span.attrStepY[FRAG_ATTRIB_COL0][0] = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL0][1] = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         span.attrStepY[FRAG_ATTRIB_COL0][1] = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL0][2] = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         span.attrStepY[FRAG_ATTRIB_COL0][2] = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
#  if CHAN_TYPE == GL_FLOAT
         span.redStep   = span.attrStepX[FRAG_ATTRIB_COL0][0];
         span.greenStep = span.attrStepX[FRAG_ATTRIB_COL0][1];
         span.blueStep  = span.attrStepX[FRAG_ATTRIB_COL0][2];
#  else
         span.redStep   = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][0]);
         span.greenStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][1]);
         span.blueStep  = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][2]);
#  endif /* GL_FLOAT */
#  ifdef INTERP_ALPHA
         span.attrStepX[FRAG_ATTRIB_COL0][3] = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         span.attrStepY[FRAG_ATTRIB_COL0][3] = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
#    if CHAN_TYPE == GL_FLOAT
         span.alphaStep = span.attrStepX[FRAG_ATTRIB_COL0][3];
#    else
         span.alphaStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL0][3]);
#    endif /* GL_FLOAT */
#  endif /* INTERP_ALPHA */
      }
      else {
         ASSERT(ctx->Light.ShadeModel == GL_FLAT);
         span.interpMask |= SPAN_FLAT;
         span.attrStepX[FRAG_ATTRIB_COL0][0] = span.attrStepY[FRAG_ATTRIB_COL0][0] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL0][1] = span.attrStepY[FRAG_ATTRIB_COL0][1] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL0][2] = span.attrStepY[FRAG_ATTRIB_COL0][2] = 0.0F;
#    if CHAN_TYPE == GL_FLOAT
	 span.redStep   = 0.0F;
	 span.greenStep = 0.0F;
	 span.blueStep  = 0.0F;
#    else
	 span.redStep   = 0;
	 span.greenStep = 0;
	 span.blueStep  = 0;
#    endif /* GL_FLOAT */
#  ifdef INTERP_ALPHA
         span.attrStepX[FRAG_ATTRIB_COL0][3] = span.attrStepY[FRAG_ATTRIB_COL0][3] = 0.0F;
#    if CHAN_TYPE == GL_FLOAT
	 span.alphaStep = 0.0F;
#    else
	 span.alphaStep = 0;
#    endif /* GL_FLOAT */
#  endif
      }
#endif /* INTERP_RGB */
#ifdef INTERP_SPEC
      span.interpMask |= SPAN_SPEC;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_dsr = (GLfloat) ((ColorTemp) vMax->specular[RCOMP] - (ColorTemp) vMin->specular[RCOMP]);
         GLfloat eBot_dsr = (GLfloat) ((ColorTemp) vMid->specular[RCOMP] - (ColorTemp) vMin->specular[RCOMP]);
         GLfloat eMaj_dsg = (GLfloat) ((ColorTemp) vMax->specular[GCOMP] - (ColorTemp) vMin->specular[GCOMP]);
         GLfloat eBot_dsg = (GLfloat) ((ColorTemp) vMid->specular[GCOMP] - (ColorTemp) vMin->specular[GCOMP]);
         GLfloat eMaj_dsb = (GLfloat) ((ColorTemp) vMax->specular[BCOMP] - (ColorTemp) vMin->specular[BCOMP]);
         GLfloat eBot_dsb = (GLfloat) ((ColorTemp) vMid->specular[BCOMP] - (ColorTemp) vMin->specular[BCOMP]);
         span.attrStepX[FRAG_ATTRIB_COL1][0] = oneOverArea * (eMaj_dsr * eBot.dy - eMaj.dy * eBot_dsr);
         span.attrStepY[FRAG_ATTRIB_COL1][0] = oneOverArea * (eMaj.dx * eBot_dsr - eMaj_dsr * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL1][1] = oneOverArea * (eMaj_dsg * eBot.dy - eMaj.dy * eBot_dsg);
         span.attrStepY[FRAG_ATTRIB_COL1][1] = oneOverArea * (eMaj.dx * eBot_dsg - eMaj_dsg * eBot.dx);
         span.attrStepX[FRAG_ATTRIB_COL1][2] = oneOverArea * (eMaj_dsb * eBot.dy - eMaj.dy * eBot_dsb);
         span.attrStepY[FRAG_ATTRIB_COL1][2] = oneOverArea * (eMaj.dx * eBot_dsb - eMaj_dsb * eBot.dx);
#  if CHAN_TYPE == GL_FLOAT
         span.specRedStep   = span.attrStepX[FRAG_ATTRIB_COL1][0];
         span.specGreenStep = span.attrStepX[FRAG_ATTRIB_COL1][1];
         span.specBlueStep  = span.attrStepX[FRAG_ATTRIB_COL1][2];
#  else
         span.specRedStep   = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL1][0]);
         span.specGreenStep = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL1][1]);
         span.specBlueStep  = SignedFloatToFixed(span.attrStepX[FRAG_ATTRIB_COL1][2]);
#  endif
      }
      else {
         span.attrStepX[FRAG_ATTRIB_COL1][0] = span.attrStepY[FRAG_ATTRIB_COL1][0] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL1][1] = span.attrStepY[FRAG_ATTRIB_COL1][1] = 0.0F;
         span.attrStepX[FRAG_ATTRIB_COL1][2] = span.attrStepY[FRAG_ATTRIB_COL1][2] = 0.0F;
#  if CHAN_TYPE == GL_FLOAT
	 span.specRedStep   = 0.0F;
	 span.specGreenStep = 0.0F;
	 span.specBlueStep  = 0.0F;
#  else
	 span.specRedStep   = 0;
	 span.specGreenStep = 0;
	 span.specBlueStep  = 0;
#  endif
      }
#endif /* INTERP_SPEC */
#ifdef INTERP_INDEX
      span.interpMask |= SPAN_INDEX;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_di = vMax->index - vMin->index;
         GLfloat eBot_di = vMid->index - vMin->index;
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
      span.interpMask |= SPAN_INT_TEXTURE;
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
      span.interpMask |= (SPAN_TEXTURE | SPAN_VARYING);
      {
         /* win[3] is 1/W */
         const GLfloat wMax = vMax->win[3], wMin = vMin->win[3], wMid = vMid->win[3];
         ATTRIB_LOOP_BEGIN
            GLfloat eMaj_ds = vMax->attrib[attr][0] * wMax - vMin->attrib[attr][0] * wMin;
            GLfloat eBot_ds = vMid->attrib[attr][0] * wMid - vMin->attrib[attr][0] * wMin;
            GLfloat eMaj_dt = vMax->attrib[attr][1] * wMax - vMin->attrib[attr][1] * wMin;
            GLfloat eBot_dt = vMid->attrib[attr][1] * wMid - vMin->attrib[attr][1] * wMin;
            GLfloat eMaj_du = vMax->attrib[attr][2] * wMax - vMin->attrib[attr][2] * wMin;
            GLfloat eBot_du = vMid->attrib[attr][2] * wMid - vMin->attrib[attr][2] * wMin;
            GLfloat eMaj_dv = vMax->attrib[attr][3] * wMax - vMin->attrib[attr][3] * wMin;
            GLfloat eBot_dv = vMid->attrib[attr][3] * wMid - vMin->attrib[attr][3] * wMin;
            span.attrStepX[attr][0] = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
            span.attrStepY[attr][0] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
            span.attrStepX[attr][1] = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
            span.attrStepY[attr][1] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
            span.attrStepX[attr][2] = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
            span.attrStepY[attr][2] = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);
            span.attrStepX[attr][3] = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
            span.attrStepY[attr][3] = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
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
         GLinterp fxLeftEdge = 0, fxRightEdge = 0;
         GLinterp fdxLeftEdge = 0, fdxRightEdge = 0;
         GLinterp fError = 0, fdError = 0;
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
#ifdef INTERP_W
         GLfloat wLeft = 0, dwOuter = 0, dwInner;
#endif
#ifdef INTERP_FOG
         GLfloat fogLeft = 0, dfogOuter = 0, dfogInner;
#endif
#ifdef INTERP_RGB
         ColorTemp rLeft = 0, fdrOuter = 0, fdrInner;
         ColorTemp gLeft = 0, fdgOuter = 0, fdgInner;
         ColorTemp bLeft = 0, fdbOuter = 0, fdbInner;
#endif
#ifdef INTERP_ALPHA
         ColorTemp aLeft = 0, fdaOuter = 0, fdaInner;
#endif
#ifdef INTERP_SPEC
         ColorTemp srLeft=0, dsrOuter=0, dsrInner;
         ColorTemp sgLeft=0, dsgOuter=0, dsgInner;
         ColorTemp sbLeft=0, dsbOuter=0, dsbInner;
#endif
#ifdef INTERP_INDEX
         GLfixed iLeft=0, diOuter=0, diInner;
#endif
#ifdef INTERP_INT_TEX
         GLfixed sLeft=0, dsOuter=0, dsInner;
         GLfixed tLeft=0, dtOuter=0, dtInner;
#endif
#ifdef INTERP_ATTRIBS
         GLfloat sLeft[FRAG_ATTRIB_MAX];
         GLfloat tLeft[FRAG_ATTRIB_MAX];
         GLfloat uLeft[FRAG_ATTRIB_MAX];
         GLfloat vLeft[FRAG_ATTRIB_MAX];
         GLfloat dsOuter[FRAG_ATTRIB_MAX], dsInner[FRAG_ATTRIB_MAX];
         GLfloat dtOuter[FRAG_ATTRIB_MAX], dtInner[FRAG_ATTRIB_MAX];
         GLfloat duOuter[FRAG_ATTRIB_MAX], duInner[FRAG_ATTRIB_MAX];
         GLfloat dvOuter[FRAG_ATTRIB_MAX], dvInner[FRAG_ATTRIB_MAX];
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
#if TRIANGLE_WALK_DOUBLE
               const GLdouble fsy = eLeft->fsy;
               const GLdouble fsx = eLeft->fsx;
               const GLdouble fx = CEILF(fsx);
               const GLdouble adjx = (fx - eLeft->fx0) * FIXED_SCALE;  /* SCALED! */
#else
               const GLfixed fsy = eLeft->fsy;
               const GLfixed fsx = eLeft->fsx;  /* no fractional part */
               const GLfixed fx = FixedCeil(fsx);  /* no fractional part */
               const GLfixed adjx = (GLinterp) (fx - eLeft->fx0); /* SCALED! */
#endif
               const GLinterp adjy = (GLinterp) eLeft->adjy;      /* SCALED! */
               GLint idxOuter;
#if TRIANGLE_WALK_DOUBLE
               GLdouble dxOuter;

               fError = fx - fsx - 1.0;
               fxLeftEdge = fsx;
               fdxLeftEdge = eLeft->dxdy;
               dxOuter = FLOORF(fdxLeftEdge);
               fdError = dxOuter - fdxLeftEdge + 1.0;
               idxOuter = (GLint) dxOuter;
               span.y = (GLint) fsy;
#else
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
#endif

               /* silence warnings on some compilers */
               (void) dxOuter;
               (void) adjx;
               (void) adjy;
               (void) vLower;

#ifdef PIXEL_ADDRESS
               {
                  pRow = (PIXEL_TYPE *) PIXEL_ADDRESS(InterpToInt(fxLeftEdge), span.y);
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
                  GLfloat z0 = vLower->win[2];
                  if (depthBits <= 16) {
                     /* interpolate fixed-pt values */
                     GLfloat tmp = (z0 * FIXED_SCALE
                                    + span.attrStepX[FRAG_ATTRIB_WPOS][2] * adjx
                                    + span.attrStepY[FRAG_ATTRIB_WPOS][2] * adjy) + FIXED_HALF;
                     if (tmp < MAX_GLUINT / 2)
                        zLeft = (GLfixed) tmp;
                     else
                        zLeft = MAX_GLUINT / 2;
                     fdzOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_WPOS][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_WPOS][2]);
                  }
                  else {
                     /* interpolate depth values w/out scaling */
                     zLeft = (GLuint) (z0 + span.attrStepX[FRAG_ATTRIB_WPOS][2] * FixedToFloat(adjx)
                                          + span.attrStepY[FRAG_ATTRIB_WPOS][2] * FixedToFloat(adjy));
                     fdzOuter = (GLint) (span.attrStepY[FRAG_ATTRIB_WPOS][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_WPOS][2]);
                  }
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *)
                    zrb->GetPointer(ctx, zrb, InterpToInt(fxLeftEdge), span.y);
                  dZRowOuter = (ctx->DrawBuffer->Width + idxOuter) * sizeof(DEPTH_TYPE);
#  endif
               }
#endif
#ifdef INTERP_W
               wLeft = vLower->win[3] + (span.attrStepX[FRAG_ATTRIB_WPOS][3] * adjx + span.attrStepY[FRAG_ATTRIB_WPOS][3] * adjy) * (1.0F/FIXED_SCALE);
               dwOuter = span.attrStepY[FRAG_ATTRIB_WPOS][3] + dxOuter * span.attrStepX[FRAG_ATTRIB_WPOS][3];
#endif
#ifdef INTERP_FOG
#  ifdef INTERP_W
               fogLeft = vLower->attrib[FRAG_ATTRIB_FOGC][0] * vLower->win[3] + (span.attrStepX[FRAG_ATTRIB_FOGC][0] * adjx + span.attrStepY[FRAG_ATTRIB_FOGC][0] * adjy) * (1.0F/FIXED_SCALE);
#  else
               fogLeft = vLower->attrib[FRAG_ATTRIB_FOGC][0] + (span.attrStepX[FRAG_ATTRIB_FOGC][0] * adjx + span.attrStepY[FRAG_ATTRIB_FOGC][0] * adjy) * (1.0F/FIXED_SCALE);
#  endif
               dfogOuter = span.attrStepY[FRAG_ATTRIB_FOGC][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_FOGC][0];
#endif
#ifdef INTERP_RGB
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
#  if CHAN_TYPE == GL_FLOAT
                  rLeft = vLower->color[RCOMP] + (span.attrStepX[FRAG_ATTRIB_COL0][0] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][0] * adjy) * (1.0F / FIXED_SCALE);
                  gLeft = vLower->color[GCOMP] + (span.attrStepX[FRAG_ATTRIB_COL0][1] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][1] * adjy) * (1.0F / FIXED_SCALE);
                  bLeft = vLower->color[BCOMP] + (span.attrStepX[FRAG_ATTRIB_COL0][2] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][2] * adjy) * (1.0F / FIXED_SCALE);
                  fdrOuter = span.attrStepY[FRAG_ATTRIB_COL0][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][0];
                  fdgOuter = span.attrStepY[FRAG_ATTRIB_COL0][1] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][1];
                  fdbOuter = span.attrStepY[FRAG_ATTRIB_COL0][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][2];
#  else
                  rLeft = (GLint)(ChanToFixed(vLower->color[RCOMP]) + span.attrStepX[FRAG_ATTRIB_COL0][0] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][0] * adjy) + FIXED_HALF;
                  gLeft = (GLint)(ChanToFixed(vLower->color[GCOMP]) + span.attrStepX[FRAG_ATTRIB_COL0][1] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][1] * adjy) + FIXED_HALF;
                  bLeft = (GLint)(ChanToFixed(vLower->color[BCOMP]) + span.attrStepX[FRAG_ATTRIB_COL0][2] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][2] * adjy) + FIXED_HALF;
                  fdrOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][0]);
                  fdgOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][1] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][1]);
                  fdbOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][2]);
#  endif
#  ifdef INTERP_ALPHA
#    if CHAN_TYPE == GL_FLOAT
                  aLeft = vLower->color[ACOMP] + (span.attrStepX[FRAG_ATTRIB_COL0][3] * adjx + span.attrStepY[FRAG_ATTRIB_COL0][3] * adjy) * (1.0F / FIXED_SCALE);
                  fdaOuter = span.attrStepY[FRAG_ATTRIB_COL0][3] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][3];
#    else
                  aLeft = (GLint)(ChanToFixed(vLower->color[ACOMP]) + span.attrStepX[FRAG_ATTRIB_COL0][3] * adjx + span.attrStepX[FRAG_ATTRIB_COL0][3] * adjy) + FIXED_HALF;
                  fdaOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL0][3] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL0][3]);
#    endif
#  endif
               }
               else {
                  ASSERT(ctx->Light.ShadeModel == GL_FLAT);
#  if CHAN_TYPE == GL_FLOAT
                  rLeft = v2->color[RCOMP];
                  gLeft = v2->color[GCOMP];
                  bLeft = v2->color[BCOMP];
                  fdrOuter = fdgOuter = fdbOuter = 0.0F;
#  else
                  rLeft = ChanToFixed(v2->color[RCOMP]);
                  gLeft = ChanToFixed(v2->color[GCOMP]);
                  bLeft = ChanToFixed(v2->color[BCOMP]);
                  fdrOuter = fdgOuter = fdbOuter = 0;
#  endif
#  ifdef INTERP_ALPHA
#    if CHAN_TYPE == GL_FLOAT
                  aLeft = v2->color[ACOMP];
                  fdaOuter = 0.0F;
#    else
                  aLeft = ChanToFixed(v2->color[ACOMP]);
                  fdaOuter = 0;
#    endif
#  endif
               }
#endif /* INTERP_RGB */


#ifdef INTERP_SPEC
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
#  if CHAN_TYPE == GL_FLOAT
                  srLeft = vLower->specular[RCOMP] + (span.attrStepX[FRAG_ATTRIB_COL1][0] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][0] * adjy) * (1.0F / FIXED_SCALE);
                  sgLeft = vLower->specular[GCOMP] + (span.attrStepX[FRAG_ATTRIB_COL1][1] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][1] * adjy) * (1.0F / FIXED_SCALE);
                  sbLeft = vLower->specular[BCOMP] + (span.attrStepX[FRAG_ATTRIB_COL1][2] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][2] * adjy) * (1.0F / FIXED_SCALE);
                  dsrOuter = span.attrStepY[FRAG_ATTRIB_COL1][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][0];
                  dsgOuter = span.attrStepY[FRAG_ATTRIB_COL1][1] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][1];
                  dsbOuter = span.attrStepY[FRAG_ATTRIB_COL1][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][2];
#  else
                  srLeft = (GLfixed) (ChanToFixed(vLower->specular[RCOMP]) + span.attrStepX[FRAG_ATTRIB_COL1][0] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][0] * adjy) + FIXED_HALF;
                  sgLeft = (GLfixed) (ChanToFixed(vLower->specular[GCOMP]) + span.attrStepX[FRAG_ATTRIB_COL1][1] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][1] * adjy) + FIXED_HALF;
                  sbLeft = (GLfixed) (ChanToFixed(vLower->specular[BCOMP]) + span.attrStepX[FRAG_ATTRIB_COL1][2] * adjx + span.attrStepY[FRAG_ATTRIB_COL1][2] * adjy) + FIXED_HALF;
                  dsrOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL1][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][0]);
                  dsgOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL1][1] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][1]);
                  dsbOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_COL1][2] + dxOuter * span.attrStepX[FRAG_ATTRIB_COL1][2]);
#  endif
               }
               else {
                  ASSERT(ctx->Light.ShadeModel == GL_FLAT);
#if  CHAN_TYPE == GL_FLOAT
                  srLeft = v2->specular[RCOMP];
                  sgLeft = v2->specular[GCOMP];
                  sbLeft = v2->specular[BCOMP];
                  dsrOuter = dsgOuter = dsbOuter = 0.0F;
#  else
                  srLeft = ChanToFixed(v2->specular[RCOMP]);
                  sgLeft = ChanToFixed(v2->specular[GCOMP]);
                  sbLeft = ChanToFixed(v2->specular[BCOMP]);
                  dsrOuter = dsgOuter = dsbOuter = 0;
#  endif
               }
#endif

#ifdef INTERP_INDEX
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
                  iLeft = (GLfixed)(vLower->index * FIXED_SCALE
                                 + didx * adjx + didy * adjy) + FIXED_HALF;
                  diOuter = SignedFloatToFixed(didy + dxOuter * didx);
               }
               else {
                  ASSERT(ctx->Light.ShadeModel == GL_FLAT);
                  iLeft = FloatToFixed(v2->index);
                  diOuter = 0;
               }
#endif
#ifdef INTERP_INT_TEX
               {
                  GLfloat s0, t0;
                  s0 = vLower->attrib[FRAG_ATTRIB_TEX0][0] * S_SCALE;
                  sLeft = (GLfixed)(s0 * FIXED_SCALE + span.attrStepX[FRAG_ATTRIB_TEX0][0] * adjx
                                 + span.attrStepY[FRAG_ATTRIB_TEX0][0] * adjy) + FIXED_HALF;
                  dsOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_TEX0][0] + dxOuter * span.attrStepX[FRAG_ATTRIB_TEX0][0]);

                  t0 = vLower->attrib[FRAG_ATTRIB_TEX0][1] * T_SCALE;
                  tLeft = (GLfixed)(t0 * FIXED_SCALE + span.attrStepX[FRAG_ATTRIB_TEX0][1] * adjx
                                 + span.attrStepY[FRAG_ATTRIB_TEX0][1] * adjy) + FIXED_HALF;
                  dtOuter = SignedFloatToFixed(span.attrStepY[FRAG_ATTRIB_TEX0][1] + dxOuter * span.attrStepX[FRAG_ATTRIB_TEX0][1]);
               }
#endif
#ifdef INTERP_ATTRIBS
               ATTRIB_LOOP_BEGIN
                  const GLfloat invW = vLower->win[3];
                  const GLfloat s0 = vLower->attrib[attr][0] * invW;
                  const GLfloat t0 = vLower->attrib[attr][1] * invW;
                  const GLfloat u0 = vLower->attrib[attr][2] * invW;
                  const GLfloat v0 = vLower->attrib[attr][3] * invW;
                  sLeft[attr] = s0 + (span.attrStepX[attr][0] * adjx + span.attrStepY[attr][0] * adjy) * (1.0F/FIXED_SCALE);
                  tLeft[attr] = t0 + (span.attrStepX[attr][1] * adjx + span.attrStepY[attr][1] * adjy) * (1.0F/FIXED_SCALE);
                  uLeft[attr] = u0 + (span.attrStepX[attr][2] * adjx + span.attrStepY[attr][2] * adjy) * (1.0F/FIXED_SCALE);
                  vLeft[attr] = v0 + (span.attrStepX[attr][3] * adjx + span.attrStepY[attr][3] * adjy) * (1.0F/FIXED_SCALE);
                  dsOuter[attr] = span.attrStepY[attr][0] + dxOuter * span.attrStepX[attr][0];
                  dtOuter[attr] = span.attrStepY[attr][1] + dxOuter * span.attrStepX[attr][1];
                  duOuter[attr] = span.attrStepY[attr][2] + dxOuter * span.attrStepX[attr][2];
                  dvOuter[attr] = span.attrStepY[attr][3] + dxOuter * span.attrStepX[attr][3];
               ATTRIB_LOOP_END
#endif
            } /*if setupLeft*/


            if (setupRight && eRight->lines>0) {
#if TRIANGLE_WALK_DOUBLE
               fxRightEdge = eRight->fsx;
               fdxRightEdge = eRight->dxdy;
#else
               fxRightEdge = eRight->fsx - FIXED_EPSILON;
               fdxRightEdge = eRight->fdxdy;
#endif
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
#ifdef INTERP_W
            dwInner = dwOuter + span.attrStepX[FRAG_ATTRIB_WPOS][3];
#endif
#ifdef INTERP_FOG
            dfogInner = dfogOuter + span.attrStepX[FRAG_ATTRIB_FOGC][0];
#endif
#ifdef INTERP_RGB
            fdrInner = fdrOuter + span.redStep;
            fdgInner = fdgOuter + span.greenStep;
            fdbInner = fdbOuter + span.blueStep;
#endif
#ifdef INTERP_ALPHA
            fdaInner = fdaOuter + span.alphaStep;
#endif
#ifdef INTERP_SPEC
            dsrInner = dsrOuter + span.specRedStep;
            dsgInner = dsgOuter + span.specGreenStep;
            dsbInner = dsbOuter + span.specBlueStep;
#endif
#ifdef INTERP_INDEX
            diInner = diOuter + span.indexStep;
#endif
#ifdef INTERP_INT_TEX
            dsInner = dsOuter + span.intTexStep[0];
            dtInner = dtOuter + span.intTexStep[1];
#endif
#ifdef INTERP_ATTRIBS
            ATTRIB_LOOP_BEGIN
               dsInner[attr] = dsOuter[attr] + span.attrStepX[attr][0];
               dtInner[attr] = dtOuter[attr] + span.attrStepX[attr][1];
               duInner[attr] = duOuter[attr] + span.attrStepX[attr][2];
               dvInner[attr] = dvOuter[attr] + span.attrStepX[attr][3];
            ATTRIB_LOOP_END
#endif

            while (lines > 0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
               const GLint right = InterpToInt(fxRightEdge);
               span.x = InterpToInt(fxLeftEdge);
               if (right <= span.x)
                  span.end = 0;
               else
                  span.end = right - span.x;

#ifdef INTERP_Z
               span.z = zLeft;
#endif
#ifdef INTERP_W
               span.attrStart[FRAG_ATTRIB_WPOS][3] = wLeft;
#endif
#ifdef INTERP_FOG
               span.attrStart[FRAG_ATTRIB_FOGC][0] = fogLeft;
#endif
#ifdef INTERP_RGB
               span.red = rLeft;
               span.green = gLeft;
               span.blue = bLeft;
#endif
#ifdef INTERP_ALPHA
               span.alpha = aLeft;
#endif
#ifdef INTERP_SPEC
               span.specRed = srLeft;
               span.specGreen = sgLeft;
               span.specBlue = sbLeft;
#endif
#ifdef INTERP_INDEX
               span.index = iLeft;
#endif
#ifdef INTERP_INT_TEX
               span.intTex[0] = sLeft;
               span.intTex[1] = tLeft;
#endif

#ifdef INTERP_ATTRIBS
               ATTRIB_LOOP_BEGIN
                  span.attrStart[attr][0] = sLeft[attr];
                  span.attrStart[attr][1] = tLeft[attr];
                  span.attrStart[attr][2] = uLeft[attr];
                  span.attrStart[attr][3] = vLeft[attr];
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
#ifdef INTERP_SPEC
                  CLAMP_INTERPOLANT(specRed, specRedStep, len);
                  CLAMP_INTERPOLANT(specGreen, specGreenStep, len);
                  CLAMP_INTERPOLANT(specBlue, specBlueStep, len);
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
                  fError -= INTERP_ONE;

#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE *) ((GLubyte *) pRow + dPRowOuter);
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *) ((GLubyte *) zRow + dZRowOuter);
#  endif
                  zLeft += fdzOuter;
#endif
#ifdef INTERP_W
                  wLeft += dwOuter;
#endif
#ifdef INTERP_FOG
                  fogLeft += dfogOuter;
#endif
#ifdef INTERP_RGB
                  rLeft += fdrOuter;
                  gLeft += fdgOuter;
                  bLeft += fdbOuter;
#endif
#ifdef INTERP_ALPHA
                  aLeft += fdaOuter;
#endif
#ifdef INTERP_SPEC
                  srLeft += dsrOuter;
                  sgLeft += dsgOuter;
                  sbLeft += dsbOuter;
#endif
#ifdef INTERP_INDEX
                  iLeft += diOuter;
#endif
#ifdef INTERP_INT_TEX
                  sLeft += dsOuter;
                  tLeft += dtOuter;
#endif
#ifdef INTERP_ATTRIBS
                  ATTRIB_LOOP_BEGIN
                     sLeft[attr] += dsOuter[attr];
                     tLeft[attr] += dtOuter[attr];
                     uLeft[attr] += duOuter[attr];
                     vLeft[attr] += dvOuter[attr];
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
#ifdef INTERP_W
                  wLeft += dwInner;
#endif
#ifdef INTERP_FOG
                  fogLeft += dfogInner;
#endif
#ifdef INTERP_RGB
                  rLeft += fdrInner;
                  gLeft += fdgInner;
                  bLeft += fdbInner;
#endif
#ifdef INTERP_ALPHA
                  aLeft += fdaInner;
#endif
#ifdef INTERP_SPEC
                  srLeft += dsrInner;
                  sgLeft += dsgInner;
                  sbLeft += dsbInner;
#endif
#ifdef INTERP_INDEX
                  iLeft += diInner;
#endif
#ifdef INTERP_INT_TEX
                  sLeft += dsInner;
                  tLeft += dtInner;
#endif
#ifdef INTERP_ATTRIBS
                  ATTRIB_LOOP_BEGIN
                     sLeft[attr] += dsInner[attr];
                     tLeft[attr] += dtInner[attr];
                     uLeft[attr] += duInner[attr];
                     vLeft[attr] += dvInner[attr];
                  ATTRIB_LOOP_END
#endif
               }
            } /*while lines>0*/

         } /* for subTriangle */

      }
#ifdef CLEANUP_CODE
      CLEANUP_CODE
#endif
   }
}

#undef SETUP_CODE
#undef CLEANUP_CODE
#undef RENDER_SPAN

#undef PIXEL_TYPE
#undef BYTES_PER_ROW
#undef PIXEL_ADDRESS
#undef DEPTH_TYPE

#undef INTERP_Z
#undef INTERP_W
#undef INTERP_FOG
#undef INTERP_RGB
#undef INTERP_ALPHA
#undef INTERP_SPEC
#undef INTERP_INDEX
#undef INTERP_INT_TEX
#undef INTERP_ATTRIBS
#undef TEX_UNIT_LOOP
#undef VARYING_LOOP

#undef S_SCALE
#undef T_SCALE

#undef FixedToDepth
#undef ColorTemp
#undef GLinterp
#undef InterpToInt
#undef INTERP_ONE

#undef NAME
