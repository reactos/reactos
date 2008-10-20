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
#ifdef DO_RGBA
   line->span.array->rgba[i][RCOMP] = solve_plane_chan(fx, fy, line->rPlane);
   line->span.array->rgba[i][GCOMP] = solve_plane_chan(fx, fy, line->gPlane);
   line->span.array->rgba[i][BCOMP] = solve_plane_chan(fx, fy, line->bPlane);
   line->span.array->rgba[i][ACOMP] = solve_plane_chan(fx, fy, line->aPlane);
#endif
#ifdef DO_INDEX
   line->span.array->index[i] = (GLint) solve_plane(fx, fy, line->iPlane);
#endif
#if defined(DO_ATTRIBS)
   ATTRIB_LOOP_BEGIN
      GLfloat (*attribArray)[4] = line->span.array->attribs[attr];
      if (attr >= FRAG_ATTRIB_TEX0 && attr < FRAG_ATTRIB_VAR0
          && !ctx->FragmentProgram._Active) {
         /* texcoord w/ divide by Q */
         const GLuint unit = attr - FRAG_ATTRIB_TEX0;
         const GLfloat invQ = solve_plane_recip(fx, fy, line->attrPlane[attr][3]);
         GLuint c;
         for (c = 0; c < 3; c++) {
            attribArray[i][c] = solve_plane(fx, fy, line->attrPlane[attr][c]) * invQ;
         }
         line->span.array->lambda[unit][i]
            = compute_lambda(line->attrPlane[attr][0],
                             line->attrPlane[attr][1], invQ,
                             line->texWidth[attr], line->texHeight[attr]);
      }
      else {
         /* non-texture attrib */
         const GLfloat invW = solve_plane_recip(fx, fy, line->wPlane);
         GLuint c;
         for (c = 0; c < 4; c++) {
            attribArray[i][c] = solve_plane(fx, fy, line->attrPlane[attr][c]) * invW;
         }
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
   line.x0 = v0->attrib[FRAG_ATTRIB_WPOS][0];
   line.y0 = v0->attrib[FRAG_ATTRIB_WPOS][1];
   line.x1 = v1->attrib[FRAG_ATTRIB_WPOS][0];
   line.y1 = v1->attrib[FRAG_ATTRIB_WPOS][1];
   line.dx = line.x1 - line.x0;
   line.dy = line.y1 - line.y0;
   line.len = SQRTF(line.dx * line.dx + line.dy * line.dy);
   line.halfWidth = 0.5F * CLAMP(ctx->Line.Width,
                                 ctx->Const.MinLineWidthAA,
                                 ctx->Const.MaxLineWidthAA);

   if (line.len == 0.0 || IS_INF_OR_NAN(line.len))
      return;

   INIT_SPAN(line.span, GL_LINE);
   line.span.arrayMask = SPAN_XY | SPAN_COVERAGE;
   line.span.facing = swrast->PointLineFacing;
   line.xAdj = line.dx / line.len * line.halfWidth;
   line.yAdj = line.dy / line.len * line.halfWidth;

#ifdef DO_Z
   line.span.arrayMask |= SPAN_Z;
   compute_plane(line.x0, line.y0, line.x1, line.y1,
                 v0->attrib[FRAG_ATTRIB_WPOS][2], v1->attrib[FRAG_ATTRIB_WPOS][2], line.zPlane);
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
#ifdef DO_INDEX
   line.span.arrayMask |= SPAN_INDEX;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      compute_plane(line.x0, line.y0, line.x1, line.y1,
                    v0->attrib[FRAG_ATTRIB_CI][0],
                    v1->attrib[FRAG_ATTRIB_CI][0], line.iPlane);
   }
   else {
      constant_plane(v1->attrib[FRAG_ATTRIB_CI][0], line.iPlane);
   }
#endif
#if defined(DO_ATTRIBS)
   {
      const GLfloat invW0 = v0->attrib[FRAG_ATTRIB_WPOS][3];
      const GLfloat invW1 = v1->attrib[FRAG_ATTRIB_WPOS][3];
      line.span.arrayMask |= SPAN_LAMBDA;
      compute_plane(line.x0, line.y0, line.x1, line.y1, invW0, invW1, line.wPlane);
      ATTRIB_LOOP_BEGIN
         GLuint c;
         if (swrast->_InterpMode[attr] == GL_FLAT) {
            for (c = 0; c < 4; c++) {
               constant_plane(v1->attrib[attr][c], line.attrPlane[attr][c]);
            }
         }
         else {
            for (c = 0; c < 4; c++) {
               const GLfloat a0 = v0->attrib[attr][c] * invW0;
               const GLfloat a1 = v1->attrib[attr][c] * invW1;
               compute_plane(line.x0, line.y0, line.x1, line.y1, a0, a1,
                             line.attrPlane[attr][c]);
            }
         }
         line.span.arrayAttribs |= (1 << attr);
         if (attr >= FRAG_ATTRIB_TEX0 && attr < FRAG_ATTRIB_VAR0) {
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
#undef DO_RGBA
#undef DO_INDEX
#undef DO_ATTRIBS
#undef NAME
