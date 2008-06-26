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
 *    DO_SPEC      - if defined, compute specular RGB values
 *    DO_ATTRIBS   - if defined, compute texcoords, varying, etc.
 */

/*void triangle( GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv )*/
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLfloat *p0 = v0->win;
   const GLfloat *p1 = v1->win;
   const GLfloat *p2 = v2->win;
   const SWvertex *vMin, *vMid, *vMax;
   GLint iyMin, iyMax;
   GLfloat yMin, yMax;
   GLboolean ltor;
   GLfloat majDx, majDy;  /* major (i.e. long) edge dx and dy */
   
   SWspan span;
   
#ifdef DO_Z
   GLfloat zPlane[4];
#endif
#ifdef DO_FOG
   GLfloat fogPlane[4];
#else
   GLfloat *fog = NULL;
#endif
#ifdef DO_RGBA
   GLfloat rPlane[4], gPlane[4], bPlane[4], aPlane[4];
#endif
#ifdef DO_INDEX
   GLfloat iPlane[4];
#endif
#ifdef DO_SPEC
   GLfloat srPlane[4], sgPlane[4], sbPlane[4];
#endif
#if defined(DO_ATTRIBS)
   GLfloat sPlane[FRAG_ATTRIB_MAX][4];  /* texture S */
   GLfloat tPlane[FRAG_ATTRIB_MAX][4];  /* texture T */
   GLfloat uPlane[FRAG_ATTRIB_MAX][4];  /* texture R */
   GLfloat vPlane[FRAG_ATTRIB_MAX][4];  /* texture Q */
   GLfloat texWidth[FRAG_ATTRIB_MAX];
   GLfloat texHeight[FRAG_ATTRIB_MAX];
#endif
   GLfloat bf = SWRAST_CONTEXT(ctx)->_BackfaceSign;
   
   (void) swrast;

   INIT_SPAN(span, GL_POLYGON, 0, 0, SPAN_COVERAGE);

   /* determine bottom to top order of vertices */
   {
      GLfloat y0 = v0->win[1];
      GLfloat y1 = v1->win[1];
      GLfloat y2 = v2->win[1];
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

   majDx = vMax->win[0] - vMin->win[0];
   majDy = vMax->win[1] - vMin->win[1];

   {
      const GLfloat botDx = vMid->win[0] - vMin->win[0];
      const GLfloat botDy = vMid->win[1] - vMin->win[1];
      const GLfloat area = majDx * botDy - botDx * majDy;
      /* Do backface culling */
      if (area * bf < 0 || area == 0 || IS_INF_OR_NAN(area))
	 return;
      ltor = (GLboolean) (area < 0.0F);
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
#ifdef DO_FOG
   compute_plane(p0, p1, p2,
                 v0->attrib[FRAG_ATTRIB_FOGC][0],
                 v1->attrib[FRAG_ATTRIB_FOGC][0],
                 v2->attrib[FRAG_ATTRIB_FOGC][0],
                 fogPlane);
   span.arrayMask |= SPAN_FOG;
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
      compute_plane(p0, p1, p2, (GLfloat) v0->index,
                    v1->index, v2->index, iPlane);
   }
   else {
      constant_plane(v2->index, iPlane);
   }
   span.arrayMask |= SPAN_INDEX;
#endif
#ifdef DO_SPEC
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(p0, p1, p2, v0->specular[RCOMP], v1->specular[RCOMP], v2->specular[RCOMP], srPlane);
      compute_plane(p0, p1, p2, v0->specular[GCOMP], v1->specular[GCOMP], v2->specular[GCOMP], sgPlane);
      compute_plane(p0, p1, p2, v0->specular[BCOMP], v1->specular[BCOMP], v2->specular[BCOMP], sbPlane);
   }
   else {
      constant_plane(v2->specular[RCOMP], srPlane);
      constant_plane(v2->specular[GCOMP], sgPlane);
      constant_plane(v2->specular[BCOMP], sbPlane);
   }
   span.arrayMask |= SPAN_SPEC;
#endif
#if defined(DO_ATTRIBS)
   {
      const GLfloat invW0 = v0->win[3];
      const GLfloat invW1 = v1->win[3];
      const GLfloat invW2 = v2->win[3];
      ATTRIB_LOOP_BEGIN
         const GLfloat s0 = v0->attrib[attr][0] * invW0;
         const GLfloat s1 = v1->attrib[attr][0] * invW1;
         const GLfloat s2 = v2->attrib[attr][0] * invW2;
         const GLfloat t0 = v0->attrib[attr][1] * invW0;
         const GLfloat t1 = v1->attrib[attr][1] * invW1;
         const GLfloat t2 = v2->attrib[attr][1] * invW2;
         const GLfloat r0 = v0->attrib[attr][2] * invW0;
         const GLfloat r1 = v1->attrib[attr][2] * invW1;
         const GLfloat r2 = v2->attrib[attr][2] * invW2;
         const GLfloat q0 = v0->attrib[attr][3] * invW0;
         const GLfloat q1 = v1->attrib[attr][3] * invW1;
         const GLfloat q2 = v2->attrib[attr][3] * invW2;
         compute_plane(p0, p1, p2, s0, s1, s2, sPlane[attr]);
         compute_plane(p0, p1, p2, t0, t1, t2, tPlane[attr]);
         compute_plane(p0, p1, p2, r0, r1, r2, uPlane[attr]);
         compute_plane(p0, p1, p2, q0, q1, q2, vPlane[attr]);
         if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
            const GLuint u = attr - FRAG_ATTRIB_TEX0;
            const struct gl_texture_object *obj = ctx->Texture.Unit[u]._Current;
            const struct gl_texture_image *texImage = obj->Image[0][obj->BaseLevel];
            texWidth[attr]  = (GLfloat) texImage->Width;
            texHeight[attr] = (GLfloat) texImage->Height;
         }
         else {
            texWidth[attr] = texHeight[attr] = 1.0;
         }
      ATTRIB_LOOP_END
   }
   span.arrayMask |= (SPAN_TEXTURE | SPAN_LAMBDA | SPAN_VARYING);
#endif

   /* Begin bottom-to-top scan over the triangle.
    * The long edge will either be on the left or right side of the
    * triangle.  We always scan from the long edge toward the shorter
    * edges, stopping when we find that coverage = 0.  If the long edge
    * is on the left we scan left-to-right.  Else, we scan right-to-left.
    */
   yMin = vMin->win[1];
   yMax = vMax->win[1];
   iyMin = (GLint) yMin;
   iyMax = (GLint) yMax + 1;

   if (ltor) {
      /* scan left to right */
      const GLfloat *pMin = vMin->win;
      const GLfloat *pMid = vMid->win;
      const GLfloat *pMax = vMax->win;
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
#ifdef DO_FOG
	    array->attribs[FRAG_ATTRIB_FOGC][count][0] = solve_plane(cx, cy, fogPlane);
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
#ifdef DO_SPEC
            array->spec[count][RCOMP] = solve_plane_chan(cx, cy, srPlane);
            array->spec[count][GCOMP] = solve_plane_chan(cx, cy, sgPlane);
            array->spec[count][BCOMP] = solve_plane_chan(cx, cy, sbPlane);
#endif
#if defined(DO_ATTRIBS)
            ATTRIB_LOOP_BEGIN
               GLfloat invQ = solve_plane_recip(cx, cy, vPlane[attr]);
               array->attribs[attr][count][0] = solve_plane(cx, cy, sPlane[attr]) * invQ;
               array->attribs[attr][count][1] = solve_plane(cx, cy, tPlane[attr]) * invQ;
               array->attribs[attr][count][2] = solve_plane(cx, cy, uPlane[attr]) * invQ;
               if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
                  const GLuint unit = attr - FRAG_ATTRIB_TEX0;
                  array->lambda[unit][count] = compute_lambda(sPlane[attr], tPlane[attr],
                                                              vPlane[attr], cx, cy, invQ,
                                                              texWidth[attr], texHeight[attr]);
               }
            ATTRIB_LOOP_END
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
         ASSERT(span.interpMask == 0);
#if defined(DO_RGBA)
         _swrast_write_rgba_span(ctx, &span);
#else
         _swrast_write_index_span(ctx, &span);
#endif
      }
   }
   else {
      /* scan right to left */
      const GLfloat *pMin = vMin->win;
      const GLfloat *pMid = vMid->win;
      const GLfloat *pMax = vMax->win;
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
         while (startX >= 0) {
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
#ifdef DO_INDEX
            array->coverage[ix] = (GLfloat) compute_coveragei(pMin, pMax, pMid, ix, iy);
#else
            array->coverage[ix] = coverage;
#endif
#ifdef DO_Z
            array->z[ix] = (GLuint) solve_plane(cx, cy, zPlane);
#endif
#ifdef DO_FOG
            array->attribs[FRAG_ATTRIB_FOGC][ix][0] = solve_plane(cx, cy, fogPlane);
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
#ifdef DO_SPEC
            array->spec[ix][RCOMP] = solve_plane_chan(cx, cy, srPlane);
            array->spec[ix][GCOMP] = solve_plane_chan(cx, cy, sgPlane);
            array->spec[ix][BCOMP] = solve_plane_chan(cx, cy, sbPlane);
#endif
#if defined(DO_ATTRIBS)
            ATTRIB_LOOP_BEGIN
               GLfloat invQ = solve_plane_recip(cx, cy, vPlane[attr]);
               array->attribs[attr][ix][0] = solve_plane(cx, cy, sPlane[attr]) * invQ;
               array->attribs[attr][ix][1] = solve_plane(cx, cy, tPlane[attr]) * invQ;
               array->attribs[attr][ix][2] = solve_plane(cx, cy, uPlane[attr]) * invQ;
               if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
                  const GLuint unit = attr - FRAG_ATTRIB_TEX0;
                  array->lambda[unit][ix] = compute_lambda(sPlane[attr],
                                                           tPlane[attr],
                                                           vPlane[attr],
                                                           cx, cy, invQ,
                                                           texWidth[attr],
                                                           texHeight[attr]);
               }
            ATTRIB_LOOP_END
#endif
            ix--;
            count++;
            coverage = compute_coveragef(pMin, pMax, pMid, ix, iy);
         }
         
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
#ifdef DO_RGBA
               COPY_CHAN4(array->rgba[j], array->rgba[j + left]);
#endif
#ifdef DO_SPEC
               COPY_CHAN4(array->spec[j], array->spec[j + left]);
#endif
#ifdef DO_INDEX
               array->index[j] = array->index[j + left];
#endif
#ifdef DO_Z
               array->z[j] = array->z[j + left];
#endif
#ifdef DO_FOG
               array->attribs[FRAG_ATTRIB_FOGC][j][0]
                  = array->attribs[FRAG_ATTRIB_FOGC][j + left][0];
#endif
#if defined(DO_ATTRIBS)
               array->lambda[0][j] = array->lambda[0][j + left];
#endif
               array->coverage[j] = array->coverage[j + left];
            }
         }
#ifdef DO_ATTRIBS
         /* shift texcoords, varying */
         {
            SWspanarrays *array = span.array;
            ATTRIB_LOOP_BEGIN
               GLint j;
               for (j = 0; j < (GLint) n; j++) {
                  array->attribs[attr][j][0] = array->attribs[attr][j + left][0];
                  array->attribs[attr][j][1] = array->attribs[attr][j + left][1];
                  array->attribs[attr][j][2] = array->attribs[attr][j + left][2];
                  /*array->lambda[unit][j] = array->lambda[unit][j + left];*/
               }
            ATTRIB_LOOP_END
         }
#endif

         span.x = left;
         span.y = iy;
         span.end = n;
         ASSERT(span.interpMask == 0);
#if defined(DO_RGBA)
         _swrast_write_rgba_span(ctx, &span);
#else
         _swrast_write_index_span(ctx, &span);
#endif
      }
   }
}


#ifdef DO_Z
#undef DO_Z
#endif

#ifdef DO_FOG
#undef DO_FOG
#endif

#ifdef DO_RGBA
#undef DO_RGBA
#endif

#ifdef DO_INDEX
#undef DO_INDEX
#endif

#ifdef DO_SPEC
#undef DO_SPEC
#endif

#ifdef DO_ATTRIBS
#undef DO_ATTRIBS
#endif

#ifdef DO_OCCLUSION_TEST
#undef DO_OCCLUSION_TEST
#endif
