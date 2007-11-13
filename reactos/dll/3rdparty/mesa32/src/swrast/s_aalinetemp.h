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
 * Antialiased line template.
 */


/*
 * Function to render each fragment in the AA line.
 * \param ix  - integer fragment window X coordiante
 * \param iy  - integer fragment window Y coordiante
 */
static void
NAME(plot)(GLcontext *ctx, struct LineInfo *line, int ix, int iy)
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLfloat fx = (GLfloat) ix;
   const GLfloat fy = (GLfloat) iy;
#ifdef DO_INDEX
   const GLfloat coverage = compute_coveragei(line, ix, iy);
#else
   const GLfloat coverage = compute_coveragef(line, ix, iy);
#endif
   const GLuint i = line->span.end;

   (void) swrast;

   if (coverage == 0.0)
      return;

   line->span.end++;
   line->span.array->coverage[i] = coverage;
   line->span.array->x[i] = ix;
   line->span.array->y[i] = iy;

   /*
    * Compute Z, color, texture coords, fog for the fragment by
    * solving the plane equations at (ix,iy).
    */
#ifdef DO_Z
   line->span.array->z[i] = (GLuint) solve_plane(fx, fy, line->zPlane);
#endif
#ifdef DO_FOG
   line->span.array->attribs[FRAG_ATTRIB_FOGC][i][0] = solve_plane(fx, fy, line->fPlane);
#endif
#ifdef DO_RGBA
   line->span.array->rgba[i][RCOMP] = solve_plane_chan(fx, fy, line->rPlane);
   line->span.array->rgba[i][GCOMP] = solve_plane_chan(fx, fy, line->gPlane);
   line->span.array->rgba[i][BCOMP] = solve_plane_chan(fx, fy, line->bPlane);
   line->span.array->rgba[i][ACOMP] = solve_plane_chan(fx, fy, line->aPlane);
#endif
#ifdef DO_INDEX
   line->span.array->index[i] = (GLint) solve_plane(fx, fy, line->iPlane);
#endif
#ifdef DO_SPEC
   line->span.array->spec[i][RCOMP] = solve_plane_chan(fx, fy, line->srPlane);
   line->span.array->spec[i][GCOMP] = solve_plane_chan(fx, fy, line->sgPlane);
   line->span.array->spec[i][BCOMP] = solve_plane_chan(fx, fy, line->sbPlane);
#endif
#if defined(DO_ATTRIBS)
   ATTRIB_LOOP_BEGIN
      GLfloat (*attribArray)[4] = line->span.array->attribs[attr];
      GLfloat invQ;
      if (ctx->FragmentProgram._Active) {
         invQ = 1.0F;
      }
      else {
         invQ = solve_plane_recip(fx, fy, line->vPlane[attr]);
      }
      attribArray[i][0] = solve_plane(fx, fy, line->sPlane[attr]) * invQ;
      attribArray[i][1] = solve_plane(fx, fy, line->tPlane[attr]) * invQ;
      attribArray[i][2] = solve_plane(fx, fy, line->uPlane[attr]) * invQ;
      if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
         const GLuint unit = attr - FRAG_ATTRIB_TEX0;
         line->span.array->lambda[unit][i]
            = compute_lambda(line->sPlane[attr],
                             line->tPlane[attr], invQ,
                             line->texWidth[attr], line->texHeight[attr]);
      }
   ATTRIB_LOOP_END
#endif

   if (line->span.end == MAX_WIDTH) {
#if defined(DO_RGBA)
      _swrast_write_rgba_span(ctx, &(line->span));
#else
      _swrast_write_index_span(ctx, &(line->span));
#endif
      line->span.end = 0; /* reset counter */
   }
}



/*
 * Line setup
 */
static void
NAME(line)(GLcontext *ctx, const SWvertex *v0, const SWvertex *v1)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLfloat tStart, tEnd;   /* segment start, end along line length */
   GLboolean inSegment;
   GLint iLen, i;

   /* Init the LineInfo struct */
   struct LineInfo line;
   line.x0 = v0->win[0];
   line.y0 = v0->win[1];
   line.x1 = v1->win[0];
   line.y1 = v1->win[1];
   line.dx = line.x1 - line.x0;
   line.dy = line.y1 - line.y0;
   line.len = SQRTF(line.dx * line.dx + line.dy * line.dy);
   line.halfWidth = 0.5F * ctx->Line._Width;

   if (line.len == 0.0 || IS_INF_OR_NAN(line.len))
      return;

   INIT_SPAN(line.span, GL_LINE, 0, 0, SPAN_XY | SPAN_COVERAGE);

   line.xAdj = line.dx / line.len * line.halfWidth;
   line.yAdj = line.dy / line.len * line.halfWidth;

#ifdef DO_Z
   line.span.arrayMask |= SPAN_Z;
   compute_plane(line.x0, line.y0, line.x1, line.y1,
                 v0->win[2], v1->win[2], line.zPlane);
#endif
#ifdef DO_FOG
   line.span.arrayMask |= SPAN_FOG;
   compute_plane(line.x0, line.y0, line.x1, line.y1,
                 v0->attrib[FRAG_ATTRIB_FOGC][0],
                 v1->attrib[FRAG_ATTRIB_FOGC][0],
                 line.fPlane);
#endif
#ifdef DO_RGBA
   line.span.arrayMask |= SPAN_RGBA;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[RCOMP], v1->color[RCOMP], line.rPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[GCOMP], v1->color[GCOMP], line.gPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[BCOMP], v1->color[BCOMP], line.bPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->color[ACOMP], v1->color[ACOMP], line.aPlane);
   }
   else {
      constant_plane(v1->color[RCOMP], line.rPlane);
      constant_plane(v1->color[GCOMP], line.gPlane);
      constant_plane(v1->color[BCOMP], line.bPlane);
      constant_plane(v1->color[ACOMP], line.aPlane);
   }
#endif
#ifdef DO_SPEC
   line.span.arrayMask |= SPAN_SPEC;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[RCOMP], v1->specular[RCOMP], line.srPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[GCOMP], v1->specular[GCOMP], line.sgPlane);
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->specular[BCOMP], v1->specular[BCOMP], line.sbPlane);
   }
   else {
      constant_plane(v1->specular[RCOMP], line.srPlane);
      constant_plane(v1->specular[GCOMP], line.sgPlane);
      constant_plane(v1->specular[BCOMP], line.sbPlane);
   }
#endif
#ifdef DO_INDEX
   line.span.arrayMask |= SPAN_INDEX;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->index, v1->index, line.iPlane);
   }
   else {
      constant_plane(v1->index, line.iPlane);
   }
#endif
#if defined(DO_ATTRIBS)
   {
      const GLfloat invW0 = v0->win[3];
      const GLfloat invW1 = v1->win[3];
      line.span.arrayMask |= (SPAN_TEXTURE | SPAN_LAMBDA | SPAN_VARYING);
      ATTRIB_LOOP_BEGIN
         const GLfloat s0 = v0->attrib[attr][0] * invW0;
         const GLfloat s1 = v1->attrib[attr][0] * invW1;
         const GLfloat t0 = v0->attrib[attr][1] * invW0;
         const GLfloat t1 = v1->attrib[attr][1] * invW1;
         const GLfloat r0 = v0->attrib[attr][2] * invW0;
         const GLfloat r1 = v1->attrib[attr][2] * invW1;
         const GLfloat q0 = v0->attrib[attr][3] * invW0;
         const GLfloat q1 = v1->attrib[attr][3] * invW1;
         compute_plane(line.x0, line.y0, line.x1, line.y1, s0, s1, line.sPlane[attr]);
         compute_plane(line.x0, line.y0, line.x1, line.y1, t0, t1, line.tPlane[attr]);
         compute_plane(line.x0, line.y0, line.x1, line.y1, r0, r1, line.uPlane[attr]);
         compute_plane(line.x0, line.y0, line.x1, line.y1, q0, q1, line.vPlane[attr]);
         if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
            const GLuint u = attr - FRAG_ATTRIB_TEX0;
            const struct gl_texture_object *obj = ctx->Texture.Unit[u]._Current;
            const struct gl_texture_image *texImage = obj->Image[0][obj->BaseLevel];
            line.texWidth[attr]  = (GLfloat) texImage->Width;
            line.texHeight[attr] = (GLfloat) texImage->Height;
         }
      ATTRIB_LOOP_END
   }
#endif

   tStart = tEnd = 0.0;
   inSegment = GL_FALSE;
   iLen = (GLint) line.len;

   if (ctx->Line.StippleFlag) {
      for (i = 0; i < iLen; i++) {
         const GLuint bit = (swrast->StippleCounter / ctx->Line.StippleFactor) & 0xf;
         if ((1 << bit) & ctx->Line.StipplePattern) {
            /* stipple bit is on */
            const GLfloat t = (GLfloat) i / (GLfloat) line.len;
            if (!inSegment) {
               /* start new segment */
               inSegment = GL_TRUE;
               tStart = t;
            }
            else {
               /* still in the segment, extend it */
               tEnd = t;
            }
         }
         else {
            /* stipple bit is off */
            if (inSegment && (tEnd > tStart)) {
               /* draw the segment */
               segment(ctx, &line, NAME(plot), tStart, tEnd);
               inSegment = GL_FALSE;
            }
            else {
               /* still between segments, do nothing */
            }
         }
         swrast->StippleCounter++;
      }

      if (inSegment) {
         /* draw the final segment of the line */
         segment(ctx, &line, NAME(plot), tStart, 1.0F);
      }
   }
   else {
      /* non-stippled */
      segment(ctx, &line, NAME(plot), 0.0, 1.0);
   }

#if defined(DO_RGBA)
   _swrast_write_rgba_span(ctx, &(line.span));
#else
   _swrast_write_index_span(ctx, &(line.span));
#endif
}




#undef DO_Z
#undef DO_FOG
#undef DO_RGBA
#undef DO_INDEX
#undef DO_SPEC
#undef DO_ATTRIBS
#undef NAME
