/*
 * Mesa 3-D graphics library
 * Version:  7.0.3
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
 * Antialiased Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom AA triangle rasterizers.
 * NOTE: this code hasn't been optimized yet.  That'll come after it
 * works correctly.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be copmuted across the triangle:
 *    DO_Z         - if defined, compute Z values
 *    DO_RGBA      - if defined, compute RGBA values
 *    DO_INDEX     - if defined, compute color index values
 *    DO_ATTRIBS   - if defined, compute texcoords, varying, etc.
 */

/*void triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv )*/
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLfloat *p0 = v0->attrib[FRAG_ATTRIB_WPOS];
   const GLfloat *p1 = v1->attrib[FRAG_ATTRIB_WPOS];
   const GLfloat *p2 = v2->attrib[FRAG_ATTRIB_WPOS];
   const SWvertex *vMin, *vMid, *vMax;
   GLint iyMin, iyMax;
   GLfloat yMin, yMax;
   GLboolean ltor;
   GLfloat majDx, majDy;  /* major (i.e. long) edge dx and dy */
   
   SWspan span;
   
#ifdef DO_Z
   GLfloat zPlane[4];
#endif
#ifdef DO_RGBA
   GLfloat rPlane[4], gPlane[4], bPlane[4], aPlane[4];
#endif
#ifdef DO_INDEX
   GLfloat iPlane[4];
#endif
#if defined(DO_ATTRIBS)
   GLfloat attrPlane[FRAG_ATTRIB_MAX][4][4];
   GLfloat wPlane[4];  /* win[3] */
#endif
   GLfloat bf = SWRAST_CONTEXT(ctx)->_BackfaceCullSign;
   
   (void) swrast;

   INIT_SPAN(span, GL_POLYGON);
   span.arrayMask = SPAN_COVERAGE;

   /* determine bottom to top order of vertices */
   {
      GLfloat y0 = v0->attrib[FRAG_ATTRIB_WPOS][1];
      GLfloat y1 = v1->attrib[FRAG_ATTRIB_WPOS][1];
      GLfloat y2 = v2->attrib[FRAG_ATTRIB_WPOS][1];
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    vMin = v0;   vMid = v1;   vMax = v2;   /* y0<=y1<=y2 */
	 }
	 else if (y2 <= y0) {
	    vMin = v2;   vMid = v0;   vMax = v1;   /* y2<=y0<=y1 */
	 }
	 else {
	    vMin = v0;   vMid = v2;   vMax = v1;  bf = -bf; /* y0<=y2<=y1 */
	 }
      }
      else {
	 if (y0 <= y2) {
	    vMin = v1;   vMid = v0;   vMax = v2;  bf = -bf; /* y1<=y0<=y2 */
	 }
	 else if (y2 <= y1) {
	    vMin = v2;   vMid = v1;   vMax = v0;  bf = -bf; /* y2<=y1<=y0 */
	 }
	 else {
	    vMin = v1;   vMid = v2;   vMax = v0;   /* y1<=y2<=y0 */
	 }
      }
   }

   majDx = vMax->attrib[FRAG_ATTRIB_WPOS][0] - vMin->attrib[FRAG_ATTRIB_WPOS][0];
   majDy = vMax->attrib[FRAG_ATTRIB_WPOS][1] - vMin->attrib[FRAG_ATTRIB_WPOS][1];

   /* front/back-face determination and cullling */
   {
      const GLfloat botDx = vMid->attrib[FRAG_ATTRIB_WPOS][0] - vMin->attrib[FRAG_ATTRIB_WPOS][0];
      const GLfloat botDy = vMid->attrib[FRAG_ATTRIB_WPOS][1] - vMin->attrib[FRAG_ATTRIB_WPOS][1];
      const GLfloat area = majDx * botDy - botDx * majDy;
      /* Do backface culling */
      if (area * bf < 0 || area == 0 || IS_INF_OR_NAN(area))
	 return;
      ltor = (GLboolean) (area < 0.0F);

      span.facing = area * swrast->_BackfaceSign > 0.0F;
   }

   /* Plane equation setup:
    * We evaluate plane equations at window (x,y) coordinates in order
    * to compute color, Z, fog, texcoords, etc.  This isn't terribly
    * efficient but it's easy and reliable.
    */
#ifdef DO_Z
   compute_plane(p0, p1, p2, p0[2], p1[2], p2[2], zPlane);
   span.arrayMask |= SPAN_Z;
#endif
#ifdef DO_RGBA
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, v0->color[RCOMP], v1->color[RCOMP], v2->color[RCOMP], rPlane);
      compute_plane(p0, p1, p2, v0->color[GCOMP], v1->color[GCOMP], v2->color[GCOMP], gPlane);
      compute_plane(p0, p1, p2, v0->color[BCOMP], v1->color[BCOMP], v2->color[BCOMP], bPlane);
      compute_plane(p0, p1, p2, v0->color[ACOMP], v1->color[ACOMP], v2->color[ACOMP], aPlane);
   }
   else {
      constant_plane(v2->color[RCOMP], rPlane);
      constant_plane(v2->color[GCOMP], gPlane);
      constant_plane(v2->color[BCOMP], bPlane);
      constant_plane(v2->color[ACOMP], aPlane);
   }
   span.arrayMask |= SPAN_RGBA;
#endif
#ifdef DO_INDEX
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, (GLfloat) v0->attrib[FRAG_ATTRIB_CI][0],
                    v1->attrib[FRAG_ATTRIB_CI][0], v2->attrib[FRAG_ATTRIB_CI][0], iPlane);
   }
   else {
      constant_plane(v2->attrib[FRAG_ATTRIB_CI][0], iPlane);
   }
   span.arrayMask |= SPAN_INDEX;
#endif
#if defined(DO_ATTRIBS)
   {
      const GLfloat invW0 = v0->attrib[FRAG_ATTRIB_WPOS][3];
      const GLfloat invW1 = v1->attrib[FRAG_ATTRIB_WPOS][3];
      const GLfloat invW2 = v2->attrib[FRAG_ATTRIB_WPOS][3];
      compute_plane(p0, p1, p2, invW0, invW1, invW2, wPlane);
      span.attrStepX[FRAG_ATTRIB_WPOS][3] = plane_dx(wPlane);
      span.attrStepY[FRAG_ATTRIB_WPOS][3] = plane_dy(wPlane);
      ATTRIB_LOOP_BEGIN
         GLuint c;
         if (swrast->_InterpMode[attr] == GL_FLAT) {
            for (c = 0; c < 4; c++) {
               constant_plane(v2->attrib[attr][c] * invW2, attrPlane[attr][c]);
            }
         }
         else {
            for (c = 0; c < 4; c++) {
               const GLfloat a0 = v0->attrib[attr][c] * invW0;
               const GLfloat a1 = v1->attrib[attr][c] * invW1;
               const GLfloat a2 = v2->attrib[attr][c] * invW2;
               compute_plane(p0, p1, p2, a0, a1, a2, attrPlane[attr][c]);
            }
         }
         for (c = 0; c < 4; c++) {
            span.attrStepX[attr][c] = plane_dx(attrPlane[attr][c]);
            span.attrStepY[attr][c] = plane_dy(attrPlane[attr][c]);
         }
      ATTRIB_LOOP_END
   }
#endif

   /* Begin bottom-to-top scan over the triangle.
    * The long edge will either be on the left or right side of the
    * triangle.  We always scan from the long edge toward the shorter
    * edges, stopping when we find that coverage = 0.  If the long edge
    * is on the left we scan left-to-right.  Else, we scan right-to-left.
    */
   yMin = vMin->attrib[FRAG_ATTRIB_WPOS][1];
   yMax = vMax->attrib[FRAG_ATTRIB_WPOS][1];
   iyMin = (GLint) yMin;
   iyMax = (GLint) yMax + 1;

   if (ltor) {
      /* scan left to right */
      const GLfloat *pMin = vMin->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat *pMid = vMid->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat *pMax = vMax->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat dxdy = majDx / majDy;
      const GLfloat xAdj = dxdy < 0.0F ? -dxdy : 0.0F;
      GLfloat x = pMin[0] - (yMin - iyMin) * dxdy;
      GLint iy;
      for (iy = iyMin; iy < iyMax; iy++, x += dxdy) {
         GLint ix, startX = (GLint) (x - xAdj);
         GLuint count;
         GLfloat coverage = 0.0F;

         /* skip over fragments with zero coverage */
         while (startX < MAX_WIDTH) {
            coverage = compute_coveragef(pMin, pMid, pMax, startX, iy);
            if (coverage > 0.0F)
               break;
            startX++;
         }

         /* enter interior of triangle */
         ix = startX;

#if defined(DO_ATTRIBS)
         /* compute attributes at left-most fragment */
         span.attrStart[FRAG_ATTRIB_WPOS][3] = solve_plane(ix + 0.5, iy + 0.5, wPlane);
         ATTRIB_LOOP_BEGIN
            GLuint c;
            for (c = 0; c < 4; c++) {
               span.attrStart[attr][c] = solve_plane(ix + 0.5, iy + 0.5, attrPlane[attr][c]);
            }
         ATTRIB_LOOP_END
#endif

         count = 0;
         while (coverage > 0.0F) {
            /* (cx,cy) = center of fragment */
            const GLfloat cx = ix + 0.5F, cy = iy + 0.5F;
            SWspanarrays *array = span.array;
#ifdef DO_INDEX
            array->coverage[count] = (GLfloat) compute_coveragei(pMin, pMid, pMax, ix, iy);
#else
            array->coverage[count] = coverage;
#endif
#ifdef DO_Z
            array->z[count] = (GLuint) solve_plane(cx, cy, zPlane);
#endif
#ifdef DO_RGBA
            array->rgba[count][RCOMP] = solve_plane_chan(cx, cy, rPlane);
            array->rgba[count][GCOMP] = solve_plane_chan(cx, cy, gPlane);
            array->rgba[count][BCOMP] = solve_plane_chan(cx, cy, bPlane);
            array->rgba[count][ACOMP] = solve_plane_chan(cx, cy, aPlane);
#endif
#ifdef DO_INDEX
            array->index[count] = (GLint) solve_plane(cx, cy, iPlane);
#endif
            ix++;
            count++;
            coverage = compute_coveragef(pMin, pMid, pMax, ix, iy);
         }
         
         if (ix <= startX)
            continue;
         
         span.x = startX;
         span.y = iy;
         span.end = (GLuint) ix - (GLuint) startX;
#if defined(DO_RGBA)
         _swrast_write_rgba_span(ctx, &span);
#else
         _swrast_write_index_span(ctx, &span);
#endif
      }
   }
   else {
      /* scan right to left */
      const GLfloat *pMin = vMin->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat *pMid = vMid->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat *pMax = vMax->attrib[FRAG_ATTRIB_WPOS];
      const GLfloat dxdy = majDx / majDy;
      const GLfloat xAdj = dxdy > 0 ? dxdy : 0.0F;
      GLfloat x = pMin[0] - (yMin - iyMin) * dxdy;
      GLint iy;
      for (iy = iyMin; iy < iyMax; iy++, x += dxdy) {
         GLint ix, left, startX = (GLint) (x + xAdj);
         GLuint count, n;
         GLfloat coverage = 0.0F;
         
         /* make sure we're not past the window edge */
         if (startX >= ctx->DrawBuffer->_Xmax) {
            startX = ctx->DrawBuffer->_Xmax - 1;
         }

         /* skip fragments with zero coverage */
         while (startX > 0) {
            coverage = compute_coveragef(pMin, pMax, pMid, startX, iy);
            if (coverage > 0.0F)
               break;
            startX--;
         }
         
         /* enter interior of triangle */
         ix = startX;
         count = 0;
         while (coverage > 0.0F) {
            /* (cx,cy) = center of fragment */
            const GLfloat cx = ix + 0.5F, cy = iy + 0.5F;
            SWspanarrays *array = span.array;
            ASSERT(ix >= 0);
#ifdef DO_INDEX
            array->coverage[ix] = (GLfloat) compute_coveragei(pMin, pMax, pMid, ix, iy);
#else
            array->coverage[ix] = coverage;
#endif
#ifdef DO_Z
            array->z[ix] = (GLuint) solve_plane(cx, cy, zPlane);
#endif
#ifdef DO_RGBA
            array->rgba[ix][RCOMP] = solve_plane_chan(cx, cy, rPlane);
            array->rgba[ix][GCOMP] = solve_plane_chan(cx, cy, gPlane);
            array->rgba[ix][BCOMP] = solve_plane_chan(cx, cy, bPlane);
            array->rgba[ix][ACOMP] = solve_plane_chan(cx, cy, aPlane);
#endif
#ifdef DO_INDEX
            array->index[ix] = (GLint) solve_plane(cx, cy, iPlane);
#endif
            ix--;
            count++;
            coverage = compute_coveragef(pMin, pMax, pMid, ix, iy);
         }
         
#if defined(DO_ATTRIBS)
         /* compute attributes at left-most fragment */
         span.attrStart[FRAG_ATTRIB_WPOS][3] = solve_plane(ix + 1.5, iy + 0.5, wPlane);
         ATTRIB_LOOP_BEGIN
            GLuint c;
            for (c = 0; c < 4; c++) {
               span.attrStart[attr][c] = solve_plane(ix + 1.5, iy + 0.5, attrPlane[attr][c]);
            }
         ATTRIB_LOOP_END
#endif

         if (startX <= ix)
            continue;

         n = (GLuint) startX - (GLuint) ix;

         left = ix + 1;

         /* shift all values to the left */
         /* XXX this is temporary */
         {
            SWspanarrays *array = span.array;
            GLint j;
            for (j = 0; j < (GLint) n; j++) {
               array->coverage[j] = array->coverage[j + left];
#ifdef DO_RGBA
               COPY_CHAN4(array->rgba[j], array->rgba[j + left]);
#endif
#ifdef DO_INDEX
               array->index[j] = array->index[j + left];
#endif
#ifdef DO_Z
               array->z[j] = array->z[j + left];
#endif
            }
         }

         span.x = left;
         span.y = iy;
         span.end = n;
#if defined(DO_RGBA)
         _swrast_write_rgba_span(ctx, &span);
#else
         _swrast_write_index_span(ctx, &span);
#endif
      }
   }
}


#undef DO_Z
#undef DO_RGBA
#undef DO_INDEX
#undef DO_ATTRIBS
#undef DO_OCCLUSION_TEST
