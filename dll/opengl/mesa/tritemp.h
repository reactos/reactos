/* $Id: tritemp.h,v 1.17 1998/01/16 03:46:07 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  2.6
 * Copyright (C) 1995-1997  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/*
 * $Log: tritemp.h,v $
 * Revision 1.17  1998/01/16 03:46:07  brianp
 * fixed a few Windows compilation warnings (Theodore Jump)
 *
 * Revision 1.16  1997/09/18 01:08:10  brianp
 * fixed S_SCALE / T_SCALE mix-up
 *
 * Revision 1.15  1997/08/22 01:53:03  brianp
 * another attempt at fixing under/overflow errors
 *
 * Revision 1.14  1997/08/13 02:10:13  brianp
 * added code to prevent over/underflow (Guido Jansen, Magnus Lundin)
 *
 * Revision 1.13  1997/06/20 02:52:49  brianp
 * changed color components from GLfixed to GLubyte
 *
 * Revision 1.12  1997/03/14 00:25:02  brianp
 * fixed unitialized memory read, contributed by Tom Schmidt
 *
 * Revision 1.11  1997/02/09 18:51:10  brianp
 * fixed typo in texture R interpolation code
 *
 * Revision 1.10  1996/12/20 23:12:23  brianp
 * another attempt at preventing color interpolation over/underflow
 *
 * Revision 1.9  1996/12/18 20:38:25  brianp
 * commented out unused zp declaration
 *
 * Revision 1.8  1996/12/12 22:37:49  brianp
 * projective textures didn't work right
 *
 * Revision 1.7  1996/11/02 06:17:37  brianp
 * fixed some float/int roundoff and over/underflow errors (hopefully)
 *
 * Revision 1.6  1996/10/01 04:13:09  brianp
 * fixed Z interpolation for >16-bit depth buffer
 * added color underflow error check
 *
 * Revision 1.5  1996/09/27 01:32:59  brianp
 * removed unused variables
 *
 * Revision 1.4  1996/09/18 01:03:43  brianp
 * tightened threshold for culling by area
 *
 * Revision 1.3  1996/09/15 14:19:16  brianp
 * now use GLframebuffer and GLvisual
 *
 * Revision 1.2  1996/09/14 06:41:38  brianp
 * perspective correct texture code wasn't sub-pixel accurate (Doug Rabson)
 *
 * Revision 1.1  1996/09/13 01:38:16  brianp
 * Initial revision
 *
 */


/*
 * Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom triangle rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated across the triangle:
 *    INTERP_Z      - if defined, interpolate Z values
 *    INTERP_RGB    - if defined, interpolate RGB values
 *    INTERP_ALPHA  - if defined, interpolate Alpha values
 *    INTERP_INDEX  - if defined, interpolate color index values
 *    INTERP_ST     - if defined, interpolate integer ST texcoords
 *                         (fast, simple 2-D texture mapping)
 *    INTERP_STW    - if defined, interpolate float ST texcoords and W
 *                         (2-D texture maps with perspective correction)
 *    INTERP_UV     - if defined, interpolate float UV texcoords too
 *                         (for 3-D, 4-D? texture maps)
 *
 * When one can directly address pixels in the color buffer the following
 * macros can be defined and used to compute pixel addresses during
 * rasterization (see pRow):
 *    PIXEL_TYPE          - the datatype of a pixel (GLubyte, GLushort, GLuint)
 *    BYTES_PER_ROW       - number of bytes per row in the color buffer
 *    PIXEL_ADDRESS(X,Y)  - returns the address of pixel at (X,Y) where
 *                          Y==0 at bottom of screen and increases upward.
 *
 * Optionally, one may provide one-time setup code per triangle:
 *    SETUP_CODE    - code which is to be executed once per triangle
 * 
 * The following macro MUST be defined:
 *    INNER_LOOP(LEFT,RIGHT,Y) - code to write a span of pixels.
 *        Something like:
 *
 *                    for (x=LEFT; x<RIGHT;x++) {
 *                       put_pixel(x,Y);
 *                       // increment fixed point interpolants
 *                    }
 *
 * This code was designed for the origin to be in the lower-left corner.
 *
 * Inspired by triangle rasterizer code written by Allen Akin.  Thanks Allen!
 */


/*void triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv )*/
{
   typedef struct {
        GLint v0, v1;     /* Y(v0) < Y(v1) */
	GLfloat dx;	/* X(v1) - X(v0) */
	GLfloat dy;	/* Y(v1) - Y(v0) */
	GLfixed fdxdy;	/* dx/dy in fixed-point */
	GLfixed fsx;	/* first sample point x coord */
	GLfixed fsy;
	GLfloat adjy;	/* adjust from v[0]->fy to fsy, scaled */
	GLint lines;	/* number of lines to be sampled on this edge */
	GLfixed fx0;	/* fixed pt X of lower endpoint */
   } EdgeT;

   struct vertex_buffer *VB = ctx->VB;
   EdgeT eMaj, eTop, eBot;
   GLfloat oneOverArea;
   int vMin, vMid, vMax;       /* vertex indexes:  Y(vMin)<=Y(vMid)<=Y(vMax) */

   /* find the order of the 3 vertices along the Y axis */
   {
      GLfloat y0 = VB->Win[v0][1];
      GLfloat y1 = VB->Win[v1][1];
      GLfloat y2 = VB->Win[v2][1];

      if (y0<=y1) {
	 if (y1<=y2) {
	    vMin = v0;   vMid = v1;   vMax = v2;   /* y0<=y1<=y2 */
	 }
	 else if (y2<=y0) {
	    vMin = v2;   vMid = v0;   vMax = v1;   /* y2<=y0<=y1 */
	 }
	 else {
	    vMin = v0;   vMid = v2;   vMax = v1;   /* y0<=y2<=y1 */
	 }
      }
      else {
	 if (y0<=y2) {
	    vMin = v1;   vMid = v0;   vMax = v2;   /* y1<=y0<=y2 */
	 }
	 else if (y2<=y1) {
	    vMin = v2;   vMid = v1;   vMax = v0;   /* y2<=y1<=y0 */
	 }
	 else {
	    vMin = v1;   vMid = v2;   vMax = v0;   /* y1<=y2<=y0 */
	 }
      }
   }

   /* vertex/edge relationship */
   eMaj.v0 = vMin;   eMaj.v1 = vMax;   /*TODO: .v1's not needed */
   eTop.v0 = vMid;   eTop.v1 = vMax;
   eBot.v0 = vMin;   eBot.v1 = vMid;

   /* compute deltas for each edge:  vertex[v1] - vertex[v0] */
   eMaj.dx = VB->Win[vMax][0] - VB->Win[vMin][0];
   eMaj.dy = VB->Win[vMax][1] - VB->Win[vMin][1];
   eTop.dx = VB->Win[vMax][0] - VB->Win[vMid][0];
   eTop.dy = VB->Win[vMax][1] - VB->Win[vMid][1];
   eBot.dx = VB->Win[vMid][0] - VB->Win[vMin][0];
   eBot.dy = VB->Win[vMid][1] - VB->Win[vMin][1];

   /* compute oneOverArea */
   {
      GLfloat area = eMaj.dx * eBot.dy - eBot.dx * eMaj.dy;
      if (area>-0.05f && area<0.05f) {
         return;  /* very small; CULLED */
      }
      oneOverArea = 1.0F / area;
   }

   /* Edge setup.  For a triangle strip these could be reused... */
   {
      /* fixed point Y coordinates */
      GLfixed vMin_fx = FloatToFixed(VB->Win[vMin][0] + 0.5F);
      GLfixed vMin_fy = FloatToFixed(VB->Win[vMin][1] - 0.5F);
      GLfixed vMid_fx = FloatToFixed(VB->Win[vMid][0] + 0.5F);
      GLfixed vMid_fy = FloatToFixed(VB->Win[vMid][1] - 0.5F);
      GLfixed vMax_fy = FloatToFixed(VB->Win[vMax][1] - 0.5F);

      eMaj.fsy = FixedCeil(vMin_fy);
      eMaj.lines = FixedToInt(vMax_fy + FIXED_ONE - FIXED_EPSILON - eMaj.fsy);
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
      eTop.lines = FixedToInt(vMax_fy + FIXED_ONE - FIXED_EPSILON - eTop.fsy);
      if (eTop.lines > 0) {
         GLfloat dxdy = eTop.dx / eTop.dy;
         eTop.fdxdy = SignedFloatToFixed(dxdy);
         eTop.adjy = (GLfloat) (eTop.fsy - vMid_fy); /* SCALED! */
         eTop.fx0 = vMid_fx;
         eTop.fsx = eTop.fx0 + (GLfixed) (eTop.adjy * dxdy);
      }

      eBot.fsy = FixedCeil(vMin_fy);
      eBot.lines = FixedToInt(vMid_fy + FIXED_ONE - FIXED_EPSILON - eBot.fsy);
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
      GLint ltor;		/* true if scanning left-to-right */
#if INTERP_Z
      GLfloat dzdx, dzdy;      GLfixed fdzdx;
#endif
#if INTERP_RGB
      GLfloat drdx, drdy;      GLfixed fdrdx;
      GLfloat dgdx, dgdy;      GLfixed fdgdx;
      GLfloat dbdx, dbdy;      GLfixed fdbdx;
#endif
#if INTERP_ALPHA
      GLfloat dadx, dady;      GLfixed fdadx;
#endif
#if INTERP_INDEX
      GLfloat didx, didy;      GLfixed fdidx;
#endif
#if INTERP_ST
      GLfloat dsdx, dsdy;      GLfixed fdsdx;
      GLfloat dtdx, dtdy;      GLfixed fdtdx;
#endif
#if INTERP_STW
      GLfloat dsdx, dsdy;
      GLfloat dtdx, dtdy;
      GLfloat dwdx, dwdy;
#endif
#if INTERP_UV
      GLfloat dudx, dudy;
      GLfloat dvdx, dvdy;
#endif

      /*
       * Execute user-supplied setup code
       */
#ifdef SETUP_CODE
      SETUP_CODE
#endif

      ltor = (oneOverArea < 0.0F);

      /* compute d?/dx and d?/dy derivatives */
#if INTERP_Z
      {
         GLfloat eMaj_dz, eBot_dz;
         eMaj_dz = VB->Win[vMax][2] - VB->Win[vMin][2];
         eBot_dz = VB->Win[vMid][2] - VB->Win[vMin][2];
         dzdx = oneOverArea * (eMaj_dz * eBot.dy - eMaj.dy * eBot_dz);
         if (dzdx>DEPTH_SCALE || dzdx<-DEPTH_SCALE) {
            /* probably a sliver triangle */
            dzdx = 0.0;
            dzdy = 0.0;
         }
         else {
            dzdy = oneOverArea * (eMaj.dx * eBot_dz - eMaj_dz * eBot.dx);
         }
         fdzdx = (GLint) dzdx;
      }
#endif
#if INTERP_RGB
      {
         GLfloat eMaj_dr, eBot_dr;
         eMaj_dr = (GLint) VB->Color[vMax][0] - (GLint) VB->Color[vMin][0];
         eBot_dr = (GLint) VB->Color[vMid][0] - (GLint) VB->Color[vMin][0];
         drdx = oneOverArea * (eMaj_dr * eBot.dy - eMaj.dy * eBot_dr);
         fdrdx = SignedFloatToFixed(drdx);
         drdy = oneOverArea * (eMaj.dx * eBot_dr - eMaj_dr * eBot.dx);
      }
      {
         GLfloat eMaj_dg, eBot_dg;
         eMaj_dg = (GLint) VB->Color[vMax][1] - (GLint) VB->Color[vMin][1];
	 eBot_dg = (GLint) VB->Color[vMid][1] - (GLint) VB->Color[vMin][1];
         dgdx = oneOverArea * (eMaj_dg * eBot.dy - eMaj.dy * eBot_dg);
         fdgdx = SignedFloatToFixed(dgdx);
         dgdy = oneOverArea * (eMaj.dx * eBot_dg - eMaj_dg * eBot.dx);
      }
      {
         GLfloat eMaj_db, eBot_db;
         eMaj_db = (GLint) VB->Color[vMax][2] - (GLint) VB->Color[vMin][2];
         eBot_db = (GLint) VB->Color[vMid][2] - (GLint) VB->Color[vMin][2];
         dbdx = oneOverArea * (eMaj_db * eBot.dy - eMaj.dy * eBot_db);
         fdbdx = SignedFloatToFixed(dbdx);
	 dbdy = oneOverArea * (eMaj.dx * eBot_db - eMaj_db * eBot.dx);
      }
#endif
#if INTERP_ALPHA
      {
         GLfloat eMaj_da, eBot_da;
         eMaj_da = (GLint) VB->Color[vMax][3] - (GLint) VB->Color[vMin][3];
         eBot_da = (GLint) VB->Color[vMid][3] - (GLint) VB->Color[vMin][3];
         dadx = oneOverArea * (eMaj_da * eBot.dy - eMaj.dy * eBot_da);
         fdadx = SignedFloatToFixed(dadx);
         dady = oneOverArea * (eMaj.dx * eBot_da - eMaj_da * eBot.dx);
      }
#endif
#if INTERP_INDEX
      {
         GLfloat eMaj_di, eBot_di;
         eMaj_di = (GLint) VB->Index[vMax] - (GLint) VB->Index[vMin];
         eBot_di = (GLint) VB->Index[vMid] - (GLint) VB->Index[vMin];
         didx = oneOverArea * (eMaj_di * eBot.dy - eMaj.dy * eBot_di);
         fdidx = SignedFloatToFixed(didx);
         didy = oneOverArea * (eMaj.dx * eBot_di - eMaj_di * eBot.dx);
      }
#endif
#if INTERP_ST
      {
         GLfloat eMaj_ds, eBot_ds;
         eMaj_ds = (VB->TexCoord[vMax][0] - VB->TexCoord[vMin][0]) * S_SCALE;
         eBot_ds = (VB->TexCoord[vMid][0] - VB->TexCoord[vMin][0]) * S_SCALE;
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         fdsdx = SignedFloatToFixed(dsdx);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);
      }
      {
         GLfloat eMaj_dt, eBot_dt;
         eMaj_dt = (VB->TexCoord[vMax][1] - VB->TexCoord[vMin][1]) * T_SCALE;
         eBot_dt = (VB->TexCoord[vMid][1] - VB->TexCoord[vMin][1]) * T_SCALE;
         dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         fdtdx = SignedFloatToFixed(dtdx);
         dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
      }
#endif
#if INTERP_STW
      {
         GLfloat wMax = 1.0F / VB->Clip[vMax][3];
         GLfloat wMin = 1.0F / VB->Clip[vMin][3];
         GLfloat wMid = 1.0F / VB->Clip[vMid][3];
         GLfloat eMaj_dw, eBot_dw;
         GLfloat eMaj_ds, eBot_ds;
         GLfloat eMaj_dt, eBot_dt;
#if INTERP_UV
         GLfloat eMaj_du, eBot_du;
         GLfloat eMaj_dv, eBot_dv;
#endif
         eMaj_dw = wMax - wMin;
         eBot_dw = wMid - wMin;
         dwdx = oneOverArea * (eMaj_dw * eBot.dy - eMaj.dy * eBot_dw);
         dwdy = oneOverArea * (eMaj.dx * eBot_dw - eMaj_dw * eBot.dx);

         eMaj_ds = VB->TexCoord[vMax][0]*wMax - VB->TexCoord[vMin][0]*wMin;
         eBot_ds = VB->TexCoord[vMid][0]*wMid - VB->TexCoord[vMin][0]*wMin;
         dsdx = oneOverArea * (eMaj_ds * eBot.dy - eMaj.dy * eBot_ds);
         dsdy = oneOverArea * (eMaj.dx * eBot_ds - eMaj_ds * eBot.dx);

         eMaj_dt = VB->TexCoord[vMax][1]*wMax - VB->TexCoord[vMin][1]*wMin;
         eBot_dt = VB->TexCoord[vMid][1]*wMid - VB->TexCoord[vMin][1]*wMin;
         dtdx = oneOverArea * (eMaj_dt * eBot.dy - eMaj.dy * eBot_dt);
         dtdy = oneOverArea * (eMaj.dx * eBot_dt - eMaj_dt * eBot.dx);
#if INTERP_UV
         eMaj_du = VB->TexCoord[vMax][2]*wMax - VB->TexCoord[vMin][2]*wMin;
         eBot_du = VB->TexCoord[vMid][2]*wMid - VB->TexCoord[vMin][2]*wMin;
         dudx = oneOverArea * (eMaj_du * eBot.dy - eMaj.dy * eBot_du);
         dudy = oneOverArea * (eMaj.dx * eBot_du - eMaj_du * eBot.dx);

         /* Note: don't divide V component by W */
         eMaj_dv = VB->TexCoord[vMax][3] - VB->TexCoord[vMin][3];
         eBot_dv = VB->TexCoord[vMid][3] - VB->TexCoord[vMin][3];
         dvdx = oneOverArea * (eMaj_dv * eBot.dy - eMaj.dy * eBot_dv);
         dvdy = oneOverArea * (eMaj.dx * eBot_dv - eMaj_dv * eBot.dx);
#endif
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
         GLfixed fx, fxLeftEdge, fxRightEdge, fdxLeftEdge, fdxRightEdge;
         GLfixed fdxOuter;
         int idxOuter;
         float dxOuter;
         GLfixed fError, fdError;
         float adjx, adjy;
         GLfixed fy;
         int iy;
#ifdef PIXEL_ADDRESS
         PIXEL_TYPE *pRow;
         int dPRowOuter, dPRowInner;  /* offset in bytes */
#endif
#if INTERP_Z
         GLdepth *zRow;
         int dZRowOuter, dZRowInner;  /* offset in bytes */
         GLfixed fz, fdzOuter, fdzInner;
#endif
#if INTERP_RGB
         GLfixed fr, fdrOuter, fdrInner;
         GLfixed fg, fdgOuter, fdgInner;
         GLfixed fb, fdbOuter, fdbInner;
#endif
#if INTERP_ALPHA
         GLfixed fa, fdaOuter, fdaInner;
#endif
#if INTERP_INDEX
         GLfixed fi, fdiOuter, fdiInner;
#endif
#if INTERP_ST
         GLfixed fs, fdsOuter, fdsInner;
         GLfixed ft, fdtOuter, fdtInner;
#endif
#if INTERP_STW
         GLfloat sLeft, dsOuter, dsInner;
         GLfloat tLeft, dtOuter, dtInner;
         GLfloat wLeft, dwOuter, dwInner;
#endif
#if INTERP_UV
         GLfloat uLeft, duOuter, duInner;
         GLfloat vLeft, dvOuter, dvInner;
#endif

         for (subTriangle=0; subTriangle<=1; subTriangle++) {
            EdgeT *eLeft, *eRight;
            int setupLeft, setupRight;
            int lines;

            if (subTriangle==0) {
               /* bottom half */
               if (ltor) {
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
               if (ltor) {
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
               if (lines==0) return;
            }

            if (setupLeft && eLeft->lines>0) {
               GLint vLower;
               GLfixed fsx = eLeft->fsx;
               fx = FixedCeil(fsx);
               fError = fx - fsx - FIXED_ONE;
               fxLeftEdge = fsx - FIXED_EPSILON;
               fdxLeftEdge = eLeft->fdxdy;
               fdxOuter = FixedFloor(fdxLeftEdge - FIXED_EPSILON);
               fdError = fdxOuter - fdxLeftEdge + FIXED_ONE;
               idxOuter = FixedToInt(fdxOuter);
               dxOuter = (float) idxOuter;

               fy = eLeft->fsy;
               iy = FixedToInt(fy);

               adjx = (float)(fx - eLeft->fx0);  /* SCALED! */
               adjy = eLeft->adjy;		 /* SCALED! */

               vLower = eLeft->v0;

#ifdef PIXEL_ADDRESS
               {
                  pRow = PIXEL_ADDRESS( FixedToInt(fxLeftEdge), iy );
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

#if INTERP_Z
               {
                  GLfloat z0;
                  z0 = VB->Win[vLower][2] + ctx->PolygonZoffset;

                  /* interpolate depth values exactly */
                  fz = (GLint) (z0 + dzdx*FixedToFloat(adjx) + dzdy*FixedToFloat(adjy));
                  fdzOuter = (GLint) (dzdy + dxOuter * dzdx);
                  zRow = Z_ADDRESS( ctx, FixedToInt(fxLeftEdge), iy );
                  dZRowOuter = (ctx->Buffer->Width + idxOuter) * sizeof(GLdepth);
               }
#endif
#if INTERP_RGB
               fr = (GLfixed)(IntToFixed(VB->Color[vLower][0]) + drdx * adjx + drdy * adjy)
                    + FIXED_HALF;
               fdrOuter = SignedFloatToFixed(drdy + dxOuter * drdx);

               fg = (GLfixed)(IntToFixed(VB->Color[vLower][1]) + dgdx * adjx + dgdy * adjy)
                    + FIXED_HALF;
               fdgOuter = SignedFloatToFixed(dgdy + dxOuter * dgdx);

               fb = (GLfixed)(IntToFixed(VB->Color[vLower][2]) + dbdx * adjx + dbdy * adjy)
                    + FIXED_HALF;
               fdbOuter = SignedFloatToFixed(dbdy + dxOuter * dbdx);
#endif
#if INTERP_ALPHA
               fa = (GLfixed)(IntToFixed(VB->Color[vLower][3]) + dadx * adjx + dady * adjy)
                    + FIXED_HALF;
               fdaOuter = SignedFloatToFixed(dady + dxOuter * dadx);
#endif
#if INTERP_INDEX
               fi = (GLfixed)(VB->Index[vLower] * FIXED_SCALE + didx * adjx
                              + didy * adjy) + FIXED_HALF;
               fdiOuter = SignedFloatToFixed(didy + dxOuter * didx);
#endif
#if INTERP_ST
               {
                  GLfloat s0, t0;
                  s0 = VB->TexCoord[vLower][0] * S_SCALE;
                  fs = (GLfixed)(s0 * FIXED_SCALE + dsdx * adjx + dsdy * adjy) + FIXED_HALF;
                  fdsOuter = SignedFloatToFixed(dsdy + dxOuter * dsdx);
                  t0 = VB->TexCoord[vLower][1] * T_SCALE;
                  ft = (GLfixed)(t0 * FIXED_SCALE + dtdx * adjx + dtdy * adjy) + FIXED_HALF;
                  fdtOuter = SignedFloatToFixed(dtdy + dxOuter * dtdx);
               }
#endif
#if INTERP_STW
               {
                  GLfloat w0 = 1.0F / VB->Clip[vLower][3];
                  GLfloat s0, t0, u0, v0;
                  wLeft = w0 + (dwdx * adjx + dwdy * adjy) * (1.0F/FIXED_SCALE);
		  dwOuter = dwdy + dxOuter * dwdx;
                  s0 = VB->TexCoord[vLower][0] * w0;
                  sLeft = s0 + (dsdx * adjx + dsdy * adjy) * (1.0F/FIXED_SCALE);
                  dsOuter = dsdy + dxOuter * dsdx;
                  t0 = VB->TexCoord[vLower][1] * w0;
                  tLeft = t0 + (dtdx * adjx + dtdy * adjy) * (1.0F/FIXED_SCALE);
                  dtOuter = dtdy + dxOuter * dtdx;
#if INTERP_UV
                  u0 = VB->TexCoord[vLower][2] * w0;
                  uLeft = u0 + (dudx * adjx + dudy * adjy) * (1.0F/FIXED_SCALE);
                  duOuter = dudy + dxOuter * dudx;
                  /* Note: don't divide V component by W */
                  v0 = VB->TexCoord[vLower][3];
                  vLeft = v0 + (dvdx * adjx + dvdy * adjy) * (1.0F/FIXED_SCALE);
                  dvOuter = dvdy + dxOuter * dvdx;
#endif
               }
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
#if INTERP_Z
            dZRowInner = dZRowOuter + sizeof(GLdepth);
            fdzInner = fdzOuter + fdzdx;
#endif
#if INTERP_RGB
            fdrInner = fdrOuter + fdrdx;
            fdgInner = fdgOuter + fdgdx;
            fdbInner = fdbOuter + fdbdx;
#endif
#if INTERP_ALPHA
            fdaInner = fdaOuter + fdadx;
#endif
#if INTERP_INDEX
            fdiInner = fdiOuter + fdidx;
#endif
#if INTERP_ST
            fdsInner = fdsOuter + fdsdx;
            fdtInner = fdtOuter + fdtdx;
#endif
#if INTERP_STW
	    dwInner = dwOuter + dwdx;
	    dsInner = dsOuter + dsdx;
	    dtInner = dtOuter + dtdx;
#if INTERP_UV
	    duInner = duOuter + dudx;
	    dvInner = dvOuter + dvdx;
#endif
#endif

            while (lines>0) {
               /* initialize the span interpolants to the leftmost value */
               /* ff = fixed-pt fragment */
#if INTERP_Z
               GLfixed ffz = fz;
               /*GLdepth *zp = zRow;*/
#endif
#if INTERP_RGB
               GLfixed ffr = fr,  ffg = fg,  ffb = fb;
#endif
#if INTERP_ALPHA
               GLfixed ffa = fa;
#endif
#if INTERP_INDEX
               GLfixed ffi = fi;
#endif
#if INTERP_ST
               GLfixed ffs = fs,  fft = ft;
#endif
#if INTERP_STW
               GLfloat ss = sLeft,  tt = tLeft,  ww = wLeft;
#endif
#if INTERP_UV
               GLfloat uu = uLeft,  vv = vLeft;
#endif
               GLint left = FixedToInt(fxLeftEdge);
               GLint right = FixedToInt(fxRightEdge);

#if INTERP_RGB
               {
                  /* need this to accomodate round-off errors */
                  GLfixed ffrend = ffr+(right-left-1)*fdrdx;
                  GLfixed ffgend = ffg+(right-left-1)*fdgdx;
                  GLfixed ffbend = ffb+(right-left-1)*fdbdx;
                  if (ffrend<0) ffr -= ffrend;
                  if (ffgend<0) ffg -= ffgend;
                  if (ffbend<0) ffb -= ffbend;
                  if (ffr<0) ffr = 0;
                  if (ffg<0) ffg = 0;
                  if (ffb<0) ffb = 0;
               }
#endif
#if INTERP_ALPHA
               {
                  GLfixed ffaend = ffa+(right-left-1)*fdadx;
                  if (ffaend<0) ffa -= ffaend;
                  if (ffa<0) ffa = 0;
               }
#endif
#if INTERP_INDEX
               if (ffi<0) ffi = 0;
#endif

               INNER_LOOP( left, right, iy );

               /*
                * Advance to the next scan line.  Compute the
                * new edge coordinates, and adjust the
                * pixel-center x coordinate so that it stays
                * on or inside the major edge.
                */
               iy++;
               lines--;

               fxLeftEdge += fdxLeftEdge;
               fxRightEdge += fdxRightEdge;


               fError += fdError;
               if (fError >= 0) {
                  fError -= FIXED_ONE;
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE*) ((GLubyte*)pRow + dPRowOuter);
#endif
#if INTERP_Z
                  zRow = (GLdepth*) ((GLubyte*)zRow + dZRowOuter);
                  fz += fdzOuter;
#endif
#if INTERP_RGB
                  fr += fdrOuter;   fg += fdgOuter;   fb += fdbOuter;
#endif
#if INTERP_ALPHA
                  fa += fdaOuter;
#endif
#if INTERP_INDEX
                  fi += fdiOuter;
#endif
#if INTERP_ST
                  fs += fdsOuter;   ft += fdtOuter;
#endif
#if INTERP_STW
		  sLeft += dsOuter;
		  tLeft += dtOuter;
		  wLeft += dwOuter;
#endif
#if INTERP_UV
		  uLeft += duOuter;
		  vLeft += dvOuter;
#endif
               }
               else {
#ifdef PIXEL_ADDRESS
                  pRow = (PIXEL_TYPE*) ((GLubyte*)pRow + dPRowInner);
#endif
#if INTERP_Z
                  zRow = (GLdepth*) ((GLubyte*)zRow + dZRowInner);
                  fz += fdzInner;
#endif
#if INTERP_RGB
                  fr += fdrInner;   fg += fdgInner;   fb += fdbInner;
#endif
#if INTERP_ALPHA
                  fa += fdaInner;
#endif
#if INTERP_INDEX
                  fi += fdiInner;
#endif
#if INTERP_ST
                  fs += fdsInner;   ft += fdtInner;
#endif
#if INTERP_STW
		  sLeft += dsInner;
		  tLeft += dtInner;
		  wLeft += dwInner;
#endif
#if INTERP_UV
		  uLeft += duInner;
		  vLeft += dvInner;
#endif
               }
            } /*while lines>0*/

         } /* for subTriangle */

      }
   }
}

#undef SETUP_CODE
#undef INNER_LOOP

#undef PIXEL_TYPE
#undef BYTES_PER_ROW
#undef PIXEL_ADDRESS

#undef INTERP_Z
#undef INTERP_RGB
#undef INTERP_ALPHA
#undef INTERP_INDEX
#undef INTERP_ST
#undef INTERP_STW
#undef INTERP_UV

#undef S_SCALE
#undef T_SCALE
