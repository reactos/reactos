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

/*
 * Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom triangle rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated across the triangle:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_FOG      - if defined, interpolate fog values
 *    INTERP_RGB      - if defined, interpolate RGB values
 *    INTERP_ALPHA    - if defined, interpolate Alpha values (req's INTERP_RGB)
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_INT_TEX  - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping)
 *    INTERP_TEX      - if defined, interpolate set 0 float STRQ texcoords
 *                         NOTE:  OpenGL STRQ = Mesa STUV (R was taken for red)
 *    INTERP_MULTITEX - if defined, interpolate N units of STRQ texcoords
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
 * Either loop over all texture units, or just use unit zero.
 */
#ifdef INTERP_MULTITEX
#define TEX_UNIT_LOOP(CODE)					\
   {								\
      GLuint u;							\
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
         if (ctx->Texture._EnabledCoordUnits & (1 << u)) {	\
            CODE						\
         }							\
      }								\
   }
#define INTERP_TEX
#elif defined(INTERP_TEX)
#define TEX_UNIT_LOOP(CODE)					\
   {								\
      const GLuint u = 0;					\
      CODE							\
   }
#endif


static void NAME(GLcontext *ctx, const SWvertex *v0,
                                 const SWvertex *v1,
                                 const SWvertex *v2 )
{
   typedef struct {
        const SWvertex *v0, *v1;   /* Y(v0) < Y(v1) */
        GLfloat dx;	/* X(v1) - X(v0) */
        GLfloat dy;	/* Y(v1) - Y(v0) */
        GLfixed fdxdy;	/* dx/dy in fixed-point */
        GLfixed fsx;	/* first sample point x coord */
        GLfixed fsy;
        GLfloat adjy;	/* adjust from v[0]->fy to fsy, scaled */
        GLint lines;	/* number of lines to be sampled on this edge */
        GLfixed fx0;	/* fixed pt X of lower endpoint */
   } EdgeT;

#ifdef INTERP_Z
   const GLint depthBits = ctx->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
   const GLfloat maxDepth = ctx->DepthMaxF;
#define FixedToDepth(F)  ((F) >> fixedToDepthShift)
#endif
   EdgeT eMaj, eTop, eBot;
   GLfloat oneOverArea;
   const SWvertex *vMin, *vMid, *vMax;  /* Y(vMin)<=Y(vMid)<=Y(vMax) */
   GLfloat bf = SWRAST_CONTEXT(ctx)->_BackfaceSign;
   const GLint snapMask = ~((FIXED_ONE / (1 << SUB_PIXEL_BITS)) - 1); /* for x/y coord snapping */
   GLfixed vMin_fx, vMin_fy, vMid_fx, vMid_fy, vMax_fx, vMax_fy;

   struct sw_span span;

   INIT_SPAN(span, GL_POLYGON, 0, 0, 0);

#ifdef INTERP_Z
   (void) fixedToDepthShift;
#endif

   /*
   printf("%s()\n", __FUNCTION__);
   printf("  %g, %g, %g\n", v0->win[0], v0->win[1], v0->win[2]);
   printf("  %g, %g, %g\n", v1->win[0], v1->win[1], v1->win[2]);
   printf("  %g, %g, %g\n", v2->win[0], v2->win[1], v2->win[2]);
   */

   /* Compute fixed point x,y coords w/ half-pixel offsets and snapping.
    * And find the order of the 3 vertices along the Y axis.
    */
   {
      const GLfixed fy0 = FloatToFixed(v0->win[1] - 0.5F) & snapMask;
      const GLfixed fy1 = FloatToFixed(v1->win[1] - 0.5F) & snapMask;
      const GLfixed fy2 = FloatToFixed(v2->win[1] - 0.5F) & snapMask;

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
      vMin_fx = FloatToFixed(vMin->win[0] + 0.5F) & snapMask;
      vMid_fx = FloatToFixed(vMid->win[0] + 0.5F) & snapMask;
      vMax_fx = FloatToFixed(vMax->win[0] + 0.5F) & snapMask;
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

      /* Do backface culling */
      if (area * bf < 0.0)
         return;

      if (IS_INF_OR_NAN(area) || area == 0.0F)
         return;

      oneOverArea = 1.0F / area;
   }

#ifndef DO_OCCLUSION_TEST
   ctx->OcclusionResult = GL_TRUE;
#endif
   span.facing = ctx->_Facing; /* for 2-sided stencil test */

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(FixedCeil(vMax_fy - eMaj.fsy));
      if (eMaj.lines > 0) {
         GLfloat dxdy = eMaj.dx / eMaj.dy;
         eMaj.fdxdy = SignedFloatToFixed(dxdy);
         eMaj.adjy = (GLfloat) (eMaj.fsy - vMin_fy);  /* SCALED! */
         eMaj.fx0 = vMin_fx;
         eMaj.fsx = eMaj.fx0 + (GLfixed) (eMaj.adjy * dxdy);
      }
      else {
         return;  /*CULLED*/
      }

      eTop.fsy = FixedCeil(vMid_fy);
      eTop.lines = FixedToInt(FixedCeil(vMax_fy - eTop.fsy));
      if (eTop.lines > 0) {
         GLfloat dxdy = eTop.dx / eTop.dy;
         eTop.fdxdy = SignedFloatToFixed(dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * dxdy);
      }

      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(FixedCeil(vMid_fy - eBot.fsy));
      if (eBot.lines > 0) {
         GLfloat dxdy = eBot.dx / eBot.dy;
         eBot.fdxdy = SignedFloatToFixed(dxdy);
         eBot.adjy = (GLfloat) (eBot.fsy - vMin_fy);  /* SCALED! */
         eBot.fx0 = vMin_fx;
         eBot.fsx = eBot.fx0 + (GLfixed) (eBot.adjy * dxdy);
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
         span.dzdx = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (span.dzdx > maxDepth || span.dzdx < -maxDepth) {
            /* probably a sliver triangle */
            span.dzdx = 0.0;
            span.dzdy = 0.0;
         }
         else {
            span.dzdy = oneOverArea * (eMaj.dx * eBot_dz - eMaj_dz * eBot.dx);
         }
         if (depthBits <= 16)
            span.zStep = SignedFloatToFixed(span.dzdx);
         else
            span.zStep = (GLint) span.dzdx;
      }
#endif
#ifdef INTERP_W
      {
         const GLfloat eMaj_dw = vMax->win[3] - vMin->win[3];
         const GLfloat eBot_dw = vMid->win[3] - vMin->win[3];
         span.dwdx = oneOverArea * (eMaj_dw * eBot.dy - eMaj.dy * eBot_dw);
         span.dwdy = oneOverArea * (eMaj.dx * eBot_dw - eMaj_dw * eBot.dx);
      }
#endif
#ifdef INTERP_FOG
      span.interpMask |= SPAN_FOG;
      {
         const GLfloat eMaj_dfog = vMax->fog - vMin->fog;
         const GLfloat eBot_dfog = vMid->fog - vMin->fog;
         span.dfogdx = oneOverArea * (eMaj_dfog * eBot.dy - eMaj.dy * eBot_dfog);
         span.dfogdy = oneOverArea * (eMaj.dx * eBot_dfog - eMaj_dfog * eBot.dx);
         span.fogStep = span.dfogdx;
      }
#endif
#ifdef INTERP_RGB
      span.interpMask |= SPAN_RGBA;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         GLfloat eMaj_dr = (GLfloat) ((ColorTemp) vMax->color[RCOMP] - vMin->color[RCOMP]);
         GLfloat eBot_dr = (GLfloat) ((ColorTemp) vMid->color[RCOMP] - vMin->color[RCOMP]);
         GLfloat eMaj_dg = (GLfloat) ((ColorTemp) vMax->color[GCOMP] - vMin->color[GCOMP]);
         GLfloat eBot_dg = (GLfloat) ((ColorTemp) vMid->color[GCOMP] - vMin->color[GCOMP]);
         GLfloat eMaj_db = (GLfloat) ((ColorTemp) vMax->color[BCOMP] - vMin->color[BCOMP]);
         GLfloat eBot_db = (GLfloat) ((ColorTemp) vMid->color[BCOMP] - vMin->color[BCOMP]);
#  ifdef INTERP_ALPHA
         GLfloat eMaj_da = (GLfloat) ((ColorTemp) vMax->color[ACOMP] - vMin->color[ACOMP]);
         GLfloat eBot_da = (GLfloat) ((ColorTemp) vMid->color[ACOMP] - vMin->color[ACOMP]);
#  endif
         span.drdx = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         span.drdy = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
         span.dgdx = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         span.dgdy = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
         span.dbdx = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         span.dbdy = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
#  if CHAN_TYPE == GL_FLOAT
         span.redStep   = span.drdx;
         span.greenStep = span.dgdx;
         span.blueStep  = span.dbdx;
#  else
         span.redStep   = SignedFloatToFixed(span.drdx);
         span.greenStep = SignedFloatToFixed(span.dgdx);
         span.blueStep  = SignedFloatToFixed(span.dbdx);
#  endif /* GL_FLOAT */
#  ifdef INTERP_ALPHA
         span.dadx = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         span.dady = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
#    if CHAN_TYPE == GL_FLOAT
         span.alphaStep = span.dadx;
#    else
         span.alphaStep = SignedFloatToFixed(span.dadx);
#    endif /* GL_FLOAT */
#  endif /* INTERP_ALPHA */
      }
      else {
         ASSERT (ctx->Light.ShadeModel == GL_FLAT);
         span.interpMask |= SPAN_FLAT;
         span.drdx = span.drdy = 0.0F;
         span.dgdx = span.dgdy = 0.0F;
         span.dbdx = span.dbdy = 0.0F;
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
         span.dadx = span.dady = 0.0F;
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
         GLfloat eMaj_dsr = (GLfloat) ((ColorTemp) vMax->specular[RCOMP] - vMin->specular[RCOMP]);
         GLfloat eBot_dsr = (GLfloat) ((ColorTemp) vMid->specular[RCOMP] - vMin->specular[RCOMP]);
         GLfloat eMaj_dsg = (GLfloat) ((ColorTemp) vMax->specular[GCOMP] - vMin->specular[GCOMP]);
         GLfloat eBot_dsg = (GLfloat) ((ColorTemp) vMid->specular[GCOMP] - vMin->specular[GCOMP]);
         GLfloat eMaj_dsb = (GLfloat) ((ColorTemp) vMax->specular[BCOMP] - vMin->specular[BCOMP]);
         GLfloat eBot_dsb = (GLfloat) ((ColorTemp) vMid->specular[BCOMP] - vMin->specular[BCOMP]);
         span.dsrdx = oneOverArea * (eMaj_dsr * eBot.dy - eMaj.dy * eBot_dsr);
         span.dsrdy = oneOverArea * (eMaj.dx * eBot_dsr - eMaj_dsr * eBot.dx);
         span.dsgdx = oneOverArea * (eMaj_dsg * eBot.dy - eMaj.dy * eBot_dsg);
         span.dsgdy = oneOverArea * (eMaj.dx * eBot_dsg - eMaj_dsg * eBot.dx);
         span.dsbdx = oneOverArea * (eMaj_dsb * eBot.dy - eMaj.dy * eBot_dsb);
         span.dsbdy = oneOverArea * (eMaj.dx * eBot_dsb - eMaj_dsb * eBot.dx);
#  if CHAN_TYPE == GL_FLOAT
         span.specRedStep   = span.dsrdx;
         span.specGreenStep = span.dsgdx;
         span.specBlueStep  = span.dsbdx;
#  else
         span.specRedStep   = SignedFloatToFixed(span.dsrdx);
         span.specGreenStep = SignedFloatToFixed(span.dsgdx);
         span.specBlueStep  = SignedFloatToFixed(span.dsbdx);
#  endif
      }
      else {
         span.dsrdx = span.dsrdy = 0.0F;
         span.dsgdx = span.dsgdy = 0.0F;
         span.dsbdx = span.dsbdy = 0.0F;
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
         GLfloat eMaj_ds = (vMax->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         GLfloat eBot_ds = (vMid->texcoord[0][0] - vMin->texcoord[0][0]) * S_SCALE;
         GLfloat eMaj_dt = (vMax->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         GLfloat eBot_dt = (vMid->texcoord[0][1] - vMin->texcoord[0][1]) * T_SCALE;
         span.texStepX[0][0] = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         span.texStepY[0][0] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
         span.texStepX[0][1] = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         span.texStepY[0][1] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
         span.intTexStep[0] = SignedFloatToFixed(span.texStepX[0][0]);
         span.intTexStep[1] = SignedFloatToFixed(span.texStepX[0][1]);
      }
#endif
#ifdef INTERP_TEX
      span.interpMask |= SPAN_TEXTURE;
      {
         /* win[3] is 1/W */
         const GLfloat wMax = vMax->win[3], wMin = vMin->win[3], wMid = vMid->win[3];
         TEX_UNIT_LOOP(
            GLfloat eMaj_ds = vMax->texcoord[u][0] * wMax - vMin->texcoord[u][0] * wMin;
            GLfloat eBot_ds = vMid->texcoord[u][0] * wMid - vMin->texcoord[u][0] * wMin;
            GLfloat eMaj_dt = vMax->texcoord[u][1] * wMax - vMin->texcoord[u][1] * wMin;
            GLfloat eBot_dt = vMid->texcoord[u][1] * wMid - vMin->texcoord[u][1] * wMin;
            GLfloat eMaj_du = vMax->texcoord[u][2] * wMax - vMin->texcoord[u][2] * wMin;
            GLfloat eBot_du = vMid->texcoord[u][2] * wMid - vMin->texcoord[u][2] * wMin;
            GLfloat eMaj_dv = vMax->texcoord[u][3] * wMax - vMin->texcoord[u][3] * wMin;
            GLfloat eBot_dv = vMid->texcoord[u][3] * wMid - vMin->texcoord[u][3] * wMin;
            span.texStepX[u][0] = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
            span.texStepY[u][0] = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
            span.texStepX[u][1] = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
            span.texStepY[u][1] = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
            span.texStepX[u][2] = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
            span.texStepY[u][2] = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);
            span.texStepX[u][3] = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
            span.texStepY[u][3] = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
         )
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
         int subTriangle;
         GLfixed fx;
         GLfixed fxLeftEdge = 0, fxRightEdge = 0;
         GLfixed fdxLeftEdge = 0, fdxRightEdge = 0;
         GLfixed fdxOuter;
         int idxOuter;
         float dxOuter;
         GLfixed fError = 0, fdError = 0;
         float adjx, adjy;
         GLfixed fy;
#ifdef PIXEL_ADDRESS
         PIXEL_TYPE *pRow = NULL;
         int dPRowOuter = 0, dPRowInner;  /* offset in bytes */
#endif
#ifdef INTERP_Z
#  ifdef DEPTH_TYPE
         DEPTH_TYPE *zRow = NULL;
         int dZRowOuter = 0, dZRowInner;  /* offset in bytes */
#  endif
         GLfixed zLeft = 0, fdzOuter = 0, fdzInner;
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
#ifdef INTERP_TEX
         GLfloat sLeft[MAX_TEXTURE_COORD_UNITS];
         GLfloat tLeft[MAX_TEXTURE_COORD_UNITS];
         GLfloat uLeft[MAX_TEXTURE_COORD_UNITS];
         GLfloat vLeft[MAX_TEXTURE_COORD_UNITS];
         GLfloat dsOuter[MAX_TEXTURE_COORD_UNITS], dsInner[MAX_TEXTURE_COORD_UNITS];
         GLfloat dtOuter[MAX_TEXTURE_COORD_UNITS], dtInner[MAX_TEXTURE_COORD_UNITS];
         GLfloat duOuter[MAX_TEXTURE_COORD_UNITS], duInner[MAX_TEXTURE_COORD_UNITS];
         GLfloat dvOuter[MAX_TEXTURE_COORD_UNITS], dvInner[MAX_TEXTURE_COORD_UNITS];
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
               const SWvertex *vLower;
               GLfixed fsx = eLeft->fsx;
               fx = FixedCeil(fsx);
               fError = fx - fsx - FIXED_ONE;
               fxLeftEdge = fsx - FIXED_EPSILON;
               fdxLeftEdge = eLeft->fdxdy;
               fdxOuter = FixedFloor(fdxLeftEdge - FIXED_EPSILON);
               fdError = fdxOuter - fdxLeftEdge + FIXED_ONE;
               idxOuter = FixedToInt(fdxOuter);
               dxOuter = (float) idxOuter;
               (void) dxOuter;

               fy = eLeft->fsy;
               span.y = FixedToInt(fy);

               adjx = (float)(fx - eLeft->fx0);  /* SCALED! */
               adjy = eLeft->adjy;		 /* SCALED! */
#ifndef __IBMCPP__
               (void) adjx;  /* silence compiler warnings */
               (void) adjy;  /* silence compiler warnings */
#endif
               vLower = eLeft->v0;
#ifndef __IBMCPP__
               (void) vLower;  /* silence compiler warnings */
#endif

#ifdef PIXEL_ADDRESS
               {
                  pRow = (PIXEL_TYPE *) PIXEL_ADDRESS(FixedToInt(fxLeftEdge), span.y);
                  dPRowOuter = -((int)BYTES_PER_ROW) + idxOuter * sizeof(PIXEL_TYPE);
                  /* negative because Y=0 at bottom and increases upward */
               }
#endif
               /*
                * Now we need the set of parameter (z, color, etc.) values at
                * the point (fx, fy).  This gives us properly-sampled parameter
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
                     GLfloat tmp = (z0 * FIXED_SCALE + span.dzdx * adjx + span.dzdy * adjy) + FIXED_HALF;
                     if (tmp < MAX_GLUINT / 2)
                        zLeft = (GLfixed) tmp;
                     else
                        zLeft = MAX_GLUINT / 2;
                     fdzOuter = SignedFloatToFixed(span.dzdy + dxOuter * span.dzdx);
                  }
                  else {
                     /* interpolate depth values exactly */
                     zLeft = (GLint) (z0 + span.dzdx * FixedToFloat(adjx) + span.dzdy * FixedToFloat(adjy));
                     fdzOuter = (GLint) (span.dzdy + dxOuter * span.dzdx);
                  }
#  ifdef DEPTH_TYPE
                  zRow = (DEPTH_TYPE *)
                    _swrast_zbuffer_address(ctx, FixedToInt(fxLeftEdge), span.y);
                  dZRowOuter = (ctx->DrawBuffer->Width + idxOuter) * sizeof(DEPTH_TYPE);
#  endif
               }
#endif
#ifdef INTERP_W
               wLeft = vLower->win[3] + (span.dwdx * adjx + span.dwdy * adjy) * (1.0F/FIXED_SCALE);
               dwOuter = span.dwdy + dxOuter * span.dwdx;
#endif
#ifdef INTERP_FOG
               fogLeft = vLower->fog + (span.dfogdx * adjx + span.dfogdy * adjy) * (1.0F/FIXED_SCALE);
               dfogOuter = span.dfogdy + dxOuter * span.dfogdx;
#endif
#ifdef INTERP_RGB
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
#  if CHAN_TYPE == GL_FLOAT
                  rLeft = vLower->color[RCOMP] + (span.drdx * adjx + span.drdy * adjy) * (1.0F / FIXED_SCALE);
                  gLeft = vLower->color[GCOMP] + (span.dgdx * adjx + span.dgdy * adjy) * (1.0F / FIXED_SCALE);
                  bLeft = vLower->color[BCOMP] + (span.dbdx * adjx + span.dbdy * adjy) * (1.0F / FIXED_SCALE);
                  fdrOuter = span.drdy + dxOuter * span.drdx;
                  fdgOuter = span.dgdy + dxOuter * span.dgdx;
                  fdbOuter = span.dbdy + dxOuter * span.dbdx;
#  else
                  rLeft = (GLint)(ChanToFixed(vLower->color[RCOMP]) + span.drdx * adjx + span.drdy * adjy) + FIXED_HALF;
                  gLeft = (GLint)(ChanToFixed(vLower->color[GCOMP]) + span.dgdx * adjx + span.dgdy * adjy) + FIXED_HALF;
                  bLeft = (GLint)(ChanToFixed(vLower->color[BCOMP]) + span.dbdx * adjx + span.dbdy * adjy) + FIXED_HALF;
                  fdrOuter = SignedFloatToFixed(span.drdy + dxOuter * span.drdx);
                  fdgOuter = SignedFloatToFixed(span.dgdy + dxOuter * span.dgdx);
                  fdbOuter = SignedFloatToFixed(span.dbdy + dxOuter * span.dbdx);
#  endif
#  ifdef INTERP_ALPHA
#    if CHAN_TYPE == GL_FLOAT
                  aLeft = vLower->color[ACOMP] + (span.dadx * adjx + span.dady * adjy) * (1.0F / FIXED_SCALE);
                  fdaOuter = span.dady + dxOuter * span.dadx;
#    else
                  aLeft = (GLint)(ChanToFixed(vLower->color[ACOMP]) + span.dadx * adjx + span.dady * adjy) + FIXED_HALF;
                  fdaOuter = SignedFloatToFixed(span.dady + dxOuter * span.dadx);
#    endif
#  endif
               }
               else {
                  ASSERT (ctx->Light.ShadeModel == GL_FLAT);
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
#endif

#ifdef INTERP_SPEC
               if (ctx->Light.ShadeModel == GL_SMOOTH) {
#  if CHAN_TYPE == GL_FLOAT
                  srLeft = vLower->specular[RCOMP] + (span.dsrdx * adjx + span.dsrdy * adjy) * (1.0F / FIXED_SCALE);
                  sgLeft = vLower->specular[GCOMP] + (span.dsgdx * adjx + span.dsgdy * adjy) * (1.0F / FIXED_SCALE);
                  sbLeft = vLower->specular[BCOMP] + (span.dsbdx * adjx + span.dsbdy * adjy) * (1.0F / FIXED_SCALE);
                  dsrOuter = span.dsrdy + dxOuter * span.dsrdx;
                  dsgOuter = span.dsgdy + dxOuter * span.dsgdx;
                  dsbOuter = span.dsbdy + dxOuter * span.dsbdx;
#  else
                  srLeft = (GLfixed) (ChanToFixed(vLower->specular[RCOMP]) + span.dsrdx * adjx + span.dsrdy * adjy) + FIXED_HALF;
                  sgLeft = (GLfixed) (ChanToFixed(vLower->specular[GCOMP]) + span.dsgdx * adjx + span.dsgdy * adjy) + FIXED_HALF;
                  sbLeft = (GLfixed) (ChanToFixed(vLower->specular[BCOMP]) + span.dsbdx * adjx + span.dsbdy * adjy) + FIXED_HALF;
                  dsrOuter = SignedFloatToFixed(span.dsrdy + dxOuter * span.dsrdx);
                  dsgOuter = SignedFloatToFixed(span.dsgdy + dxOuter * span.dsgdx);
                  dsbOuter = SignedFloatToFixed(span.dsbdy + dxOuter * span.dsbdx);
#  endif
               }
               else {
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
                  iLeft = FloatToFixed(v2->index);
                  diOuter = 0;
               }
#endif
#ifdef INTERP_INT_TEX
               {
                  GLfloat s0, t0;
                  s0 = vLower->texcoord[0][0] * S_SCALE;
                  sLeft = (GLfixed)(s0 * FIXED_SCALE + span.texStepX[0][0] * adjx
                                 + span.texStepY[0][0] * adjy) + FIXED_HALF;
                  dsOuter = SignedFloatToFixed(span.texStepY[0][0] + dxOuter * span.texStepX[0][0]);

                  t0 = vLower->texcoord[0][1] * T_SCALE;
                  tLeft = (GLfixed)(t0 * FIXED_SCALE + span.texStepX[0][1] * adjx
                                 + span.texStepY[0][1] * adjy) + FIXED_HALF;
                  dtOuter = SignedFloatToFixed(span.texStepY[0][1] + dxOuter * span.texStepX[0][1]);
               }
#endif
#ifdef INTERP_TEX
               TEX_UNIT_LOOP(
                  const GLfloat invW = vLower->win[3];
                  const GLfloat s0 = vLower->texcoord[u][0] * invW;
                  const GLfloat t0 = vLower->texcoord[u][1] * invW;
                  const GLfloat u0 = vLower->texcoord[u][2] * invW;
                  const GLfloat v0 = vLower->texcoord[u][3] * invW;
                  sLeft[u] = s0 + (span.texStepX[u][0] * adjx + span.texStepY[u][0] * adjy) * (1.0F/FIXED_SCALE);
                  tLeft[u] = t0 + (span.texStepX[u][1] * adjx + span.texStepY[u][1] * adjy) * (1.0F/FIXED_SCALE);
                  uLeft[u] = u0 + (span.texStepX[u][2] * adjx + span.texStepY[u][2] * adjy) * (1.0F/FIXED_SCALE);
                  vLeft[u] = v0 + (span.texStepX[u][3] * adjx + span.texStepY[u][3] * adjy) * (1.0F/FIXED_SCALE);
                  dsOuter[u] = span.texStepY[u][0] + dxOuter * span.texStepX[u][0];
                  dtOuter[u] = span.texStepY[u][1] + dxOuter * span.texStepX[u][1];
                  duOuter[u] = span.texStepY[u][2] + dxOuter * span.texStepX[u][2];
                  dvOuter[u] = span.texStepY[u][3] + dxOuter * span.texStepX[u][3];
               )
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
#ifdef INTERP_W
            dwInner = dwOuter + span.dwdx;
#endif
#ifdef INTERP_FOG
            dfogInner = dfogOuter + span.dfogdx;
#endif
#if defined(INTERP_RGB)
            fdrInner = fdrOuter + span.redStep;
            fdgInner = fdgOuter + span.greenStep;
            fdbInner = fdbOuter + span.blueStep;
#endif
#if defined(INTERP_ALPHA)
            fdaInner = fdaOuter + span.alphaStep;
#endif
#if defined(INTERP_SPEC)
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
#ifdef INTERP_TEX
            TEX_UNIT_LOOP(
               dsInner[u] = dsOuter[u] + span.texStepX[u][0];
               dtInner[u] = dtOuter[u] + span.texStepX[u][1];
               duInner[u] = duOuter[u] + span.texStepX[u][2];
               dvInner[u] = dvOuter[u] + span.texStepX[u][3];
            )
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
#ifdef INTERP_W
               span.w = wLeft;
#endif
#ifdef INTERP_FOG
               span.fog = fogLeft;
#endif
#if defined(INTERP_RGB)
               span.red = rLeft;
               span.green = gLeft;
               span.blue = bLeft;
#endif
#if defined(INTERP_ALPHA)
               span.alpha = aLeft;
#endif
#if defined(INTERP_SPEC)
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

#ifdef INTERP_TEX
               TEX_UNIT_LOOP(
                  span.tex[u][0] = sLeft[u];
                  span.tex[u][1] = tLeft[u];
                  span.tex[u][2] = uLeft[u];
                  span.tex[u][3] = vLeft[u];
               )
#endif

#ifdef INTERP_RGB
               {
                  /* need this to accomodate round-off errors */
                  const GLint len = right - span.x - 1;
                  GLfixed ffrend = span.red + len * span.redStep;
                  GLfixed ffgend = span.green + len * span.greenStep;
                  GLfixed ffbend = span.blue + len * span.blueStep;
                  if (ffrend < 0) {
                     span.red -= ffrend;
                     if (span.red < 0)
                        span.red = 0;
                  }
                  if (ffgend < 0) {
                     span.green -= ffgend;
                     if (span.green < 0)
                        span.green = 0;
                  }
                  if (ffbend < 0) {
                     span.blue -= ffbend;
                     if (span.blue < 0)
                        span.blue = 0;
                  }
               }
#endif
#ifdef INTERP_ALPHA
               {
                  const GLint len = right - span.x - 1;
                  GLfixed ffaend = span.alpha + len * span.alphaStep;
                  if (ffaend < 0) {
                     span.alpha -= ffaend;
                     if (span.alpha < 0)
                        span.alpha = 0;
                  }
               }
#endif
#ifdef INTERP_SPEC
               {
                  /* need this to accomodate round-off errors */
                  const GLint len = right - span.x - 1;
                  GLfixed ffsrend = span.specRed + len * span.specRedStep;
                  GLfixed ffsgend = span.specGreen + len * span.specGreenStep;
                  GLfixed ffsbend = span.specBlue + len * span.specBlueStep;
                  if (ffsrend < 0) {
                     span.specRed -= ffsrend;
                     if (span.specRed < 0)
                        span.specRed = 0;
                  }
                  if (ffsgend < 0) {
                     span.specGreen -= ffsgend;
                     if (span.specGreen < 0)
                        span.specGreen = 0;
                  }
                  if (ffsbend < 0) {
                     span.specBlue -= ffsbend;
                     if (span.specBlue < 0)
                        span.specBlue = 0;
                  }
               }
#endif
#ifdef INTERP_INDEX
               if (span.index < 0)  span.index = 0;
#endif

               /* This is where we actually generate fragments */
               if (span.end > 0) {
                  RENDER_SPAN( span );
               }

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               (span.y)++;
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
#ifdef INTERP_W
                  wLeft += dwOuter;
#endif
#ifdef INTERP_FOG
                  fogLeft += dfogOuter;
#endif
#if defined(INTERP_RGB)
                  rLeft += fdrOuter;
                  gLeft += fdgOuter;
                  bLeft += fdbOuter;
#endif
#if defined(INTERP_ALPHA)
                  aLeft += fdaOuter;
#endif
#if defined(INTERP_SPEC)
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
#ifdef INTERP_TEX
                  TEX_UNIT_LOOP(
                     sLeft[u] += dsOuter[u];
                     tLeft[u] += dtOuter[u];
                     uLeft[u] += duOuter[u];
                     vLeft[u] += dvOuter[u];
                  )
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
#if defined(INTERP_RGB)
                  rLeft += fdrInner;
                  gLeft += fdgInner;
                  bLeft += fdbInner;
#endif
#if defined(INTERP_ALPHA)
                  aLeft += fdaInner;
#endif
#if defined(INTERP_SPEC)
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
#ifdef INTERP_TEX
                  TEX_UNIT_LOOP(
                     sLeft[u] += dsInner[u];
                     tLeft[u] += dtInner[u];
                     uLeft[u] += duInner[u];
                     vLeft[u] += dvInner[u];
                  )
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

#undef INTERP_Z
#undef INTERP_W
#undef INTERP_FOG
#undef INTERP_RGB
#undef INTERP_ALPHA
#undef INTERP_SPEC
#undef INTERP_INDEX
#undef INTERP_INT_TEX
#undef INTERP_TEX
#undef INTERP_MULTITEX
#undef TEX_UNIT_LOOP

#undef S_SCALE
#undef T_SCALE

#undef FixedToDepth

#undef DO_OCCLUSION_TEST
#undef NAME
