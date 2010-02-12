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
 * Regarding GL_NV_point_sprite:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */


/*
 * Point rendering template code.
 *
 * Set FLAGS = bitwise-OR of the following tokens:
 *
 *   RGBA = do rgba instead of color index
 *   SMOOTH = do antialiasing
 *   ATTRIBS = general attributes (texcoords, etc)
 *   SPECULAR = do separate specular color
 *   LARGE = do points with diameter > 1 pixel
 *   ATTENUATE = compute point size attenuation
 *   SPRITE = GL_ARB_point_sprite / GL_NV_point_sprite
 *
 * Notes: LARGE and ATTENUATE are exclusive of each other.
 *        ATTRIBS requires RGBA
 */


/*
 * NOTES on antialiased point rasterization:
 *
 * Let d = distance of fragment center from vertex.
 * if d < rmin2 then
 *    fragment has 100% coverage
 * else if d > rmax2 then
 *    fragment has 0% coverage
 * else
 *    fragment has % coverage = (d - rmin2) / (rmax2 - rmin2)
 */


static void
NAME ( GLcontext *ctx, const SWvertex *vert )
{
#if FLAGS & (ATTENUATE | LARGE | SMOOTH | SPRITE)
   GLfloat size;
#endif
#if FLAGS & RGBA
#if (FLAGS & ATTENUATE) && (FLAGS & SMOOTH)
   GLfloat alphaAtten;
#endif
   const GLchan red   = vert->color[0];
   const GLchan green = vert->color[1];
   const GLchan blue  = vert->color[2];
   const GLchan alpha = vert->color[3];
#endif
#if FLAGS & SPECULAR
   const GLchan specRed   = vert->specular[0];
   const GLchan specGreen = vert->specular[1];
   const GLchan specBlue  = vert->specular[2];
#endif
#if FLAGS & INDEX
   const GLuint colorIndex = (GLuint) vert->index; /* XXX round? */
#endif
#if FLAGS & ATTRIBS
   GLfloat attrib[FRAG_ATTRIB_MAX][4]; /* texture & varying */
#endif
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   SWspan *span = &(swrast->PointSpan);

   /* Cull primitives with malformed coordinates.
    */
   {
      float tmp = vert->win[0] + vert->win[1];
      if (IS_INF_OR_NAN(tmp))
	 return;
   }

   /*
    * Span init
    */
   span->interpMask = SPAN_FOG;
   span->arrayMask = SPAN_XY | SPAN_Z;
   span->attrStart[FRAG_ATTRIB_FOGC][0] = vert->attrib[FRAG_ATTRIB_FOGC][0];
   span->attrStepX[FRAG_ATTRIB_FOGC][0] = 0.0;
   span->attrStepY[FRAG_ATTRIB_FOGC][0] = 0.0;
#if FLAGS & RGBA
   span->arrayMask |= SPAN_RGBA;
#endif
#if FLAGS & SPECULAR
   span->arrayMask |= SPAN_SPEC;
#endif
#if FLAGS & INDEX
   span->arrayMask |= SPAN_INDEX;
#endif
#if FLAGS & ATTRIBS
   span->arrayMask |= (SPAN_TEXTURE | SPAN_LAMBDA);
   if (ctx->FragmentProgram._Active) {
      /* Don't divide texture s,t,r by q (use TXP to do that) */
      ATTRIB_LOOP_BEGIN
         COPY_4V(attrib[attr], vert->attrib[attr]);
      ATTRIB_LOOP_END
   }
   else {
      /* Divide texture s,t,r by q here */
      ATTRIB_LOOP_BEGIN
         const GLfloat q = vert->attrib[attr][3];
         const GLfloat invQ = (q == 0.0F || q == 1.0F) ? 1.0F : (1.0F / q);
         attrib[attr][0] = vert->attrib[attr][0] * invQ;
         attrib[attr][1] = vert->attrib[attr][1] * invQ;
         attrib[attr][2] = vert->attrib[attr][2] * invQ;
         attrib[attr][3] = q;
      ATTRIB_LOOP_END
   }
   /* need these for fragment programs */
   span->attrStart[FRAG_ATTRIB_WPOS][3] = 1.0F;
   span->attrStepX[FRAG_ATTRIB_WPOS][3] = 0.0F;
   span->attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0F;
#endif
#if FLAGS & SMOOTH
   span->arrayMask |= SPAN_COVERAGE;
#endif
#if FLAGS & SPRITE
   span->arrayMask |= (SPAN_TEXTURE | SPAN_LAMBDA);
#endif

   /* Compute point size if not known to be one */
#if FLAGS & ATTENUATE
   /* first, clamp attenuated size to the user-specifed range */
   size = CLAMP(vert->pointSize, ctx->Point.MinSize, ctx->Point.MaxSize);
#if (FLAGS & RGBA) && (FLAGS & SMOOTH)
   /* only if multisampling, compute the fade factor */
   if (ctx->Multisample.Enabled) {
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
#endif
#elif FLAGS & (LARGE | SMOOTH | SPRITE)
   /* constant, non-attenuated size */
   size = ctx->Point._Size; /* this is already clamped */
#endif


#if FLAGS & (ATTENUATE | LARGE | SMOOTH | SPRITE)
   /***
    *** Multi-pixel points
    ***/

   /* do final clamping now */
   if (ctx->Point.SmoothFlag) {
      size = CLAMP(size, ctx->Const.MinPointSizeAA, ctx->Const.MaxPointSizeAA);
   }
   else {
      size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);
   }

   {{
      GLint x, y;
      const GLfloat radius = 0.5F * size;
      const GLuint z = (GLuint) (vert->win[2] + 0.5F);
      GLuint count;
#if FLAGS & SMOOTH
      const GLfloat rmin = radius - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
      const GLfloat rmax = radius + 0.7071F;
      const GLfloat rmin2 = MAX2(0.0F, rmin * rmin);
      const GLfloat rmax2 = rmax * rmax;
      const GLfloat cscale = 1.0F / (rmax2 - rmin2);
      const GLint xmin = (GLint) (vert->win[0] - radius);
      const GLint xmax = (GLint) (vert->win[0] + radius);
      const GLint ymin = (GLint) (vert->win[1] - radius);
      const GLint ymax = (GLint) (vert->win[1] + radius);
#else
      /* non-smooth */
      GLint xmin, xmax, ymin, ymax;
      GLint iSize = (GLint) (size + 0.5F);
      GLint iRadius;
      iSize = MAX2(1, iSize);
      iRadius = iSize / 2;
      if (iSize & 1) {
         /* odd size */
         xmin = (GLint) (vert->win[0] - iRadius);
         xmax = (GLint) (vert->win[0] + iRadius);
         ymin = (GLint) (vert->win[1] - iRadius);
         ymax = (GLint) (vert->win[1] + iRadius);
      }
      else {
         /* even size */
         xmin = (GLint) vert->win[0] - iRadius;
         xmax = xmin + iSize - 1;
         ymin = (GLint) vert->win[1] - iRadius;
         ymax = ymin + iSize - 1;
      }
#endif /*SMOOTH*/

      /* check if we need to flush */
      if (span->end + (xmax-xmin+1) * (ymax-ymin+1) >= MAX_WIDTH ||
          (swrast->_RasterMask & (BLEND_BIT | LOGIC_OP_BIT | MASKING_BIT))) {
         if (span->end > 0) {
#if FLAGS & RGBA
            _swrast_write_rgba_span(ctx, span);
#else
            _swrast_write_index_span(ctx, span);
#endif
            span->end = 0;
         }
      }

      /*
       * OK, generate fragments
       */
      count = span->end;
      (void) radius;
      for (y = ymin; y <= ymax; y++) {
         /* check if we need to flush */
         if (count + (xmax-xmin+1) >= MAX_WIDTH) {
	     span->end = count;
#if FLAGS & RGBA
            _swrast_write_rgba_span(ctx, span);
#else
            _swrast_write_index_span(ctx, span);
#endif
            count = span->end = 0;
         }
         for (x = xmin; x <= xmax; x++) {
#if FLAGS & SPRITE
            GLuint u;
#endif

#if FLAGS & RGBA
            span->array->rgba[count][RCOMP] = red;
            span->array->rgba[count][GCOMP] = green;
            span->array->rgba[count][BCOMP] = blue;
            span->array->rgba[count][ACOMP] = alpha;
#endif
#if FLAGS & SPECULAR
            span->array->spec[count][RCOMP] = specRed;
            span->array->spec[count][GCOMP] = specGreen;
            span->array->spec[count][BCOMP] = specBlue;
#endif
#if FLAGS & INDEX
            span->array->index[count] = colorIndex;
#endif
#if FLAGS & ATTRIBS
            ATTRIB_LOOP_BEGIN
               COPY_4V(span->array->attribs[attr][count], attrib[attr]);
               if (attr < FRAG_ATTRIB_VAR0 && attr >= FRAG_ATTRIB_TEX0) {
                  const GLuint u = attr - FRAG_ATTRIB_TEX0;
                  span->array->lambda[u][count] = 0.0;
               }
            ATTRIB_LOOP_END
#endif

#if FLAGS & SMOOTH
            /* compute coverage */
            {
               const GLfloat dx = x - vert->win[0] + 0.5F;
               const GLfloat dy = y - vert->win[1] + 0.5F;
               const GLfloat dist2 = dx * dx + dy * dy;
               if (dist2 < rmax2) {
                  if (dist2 >= rmin2) {
                     /* compute partial coverage */
                     span->array->coverage[count] = 1.0F - (dist2 - rmin2) * cscale;
#if FLAGS & INDEX
                     /* coverage in [0,15] */
                     span->array->coverage[count] *= 15.0;
#endif
                  }
                  else {
                     /* full coverage */
                     span->array->coverage[count] = 1.0F;
                  }

                  span->array->x[count] = x;
                  span->array->y[count] = y;
                  span->array->z[count] = z;

#if (FLAGS & ATTENUATE) && (FLAGS & RGBA)
                  span->array->rgba[count][ACOMP] = (GLchan) (alpha * alphaAtten);
#elif FLAGS & RGBA
                  span->array->rgba[count][ACOMP] = alpha;
#endif /*ATTENUATE*/
                  count++;
               } /*if*/
            }

#else /*SMOOTH*/

            /* not smooth (square points) */
            span->array->x[count] = x;
            span->array->y[count] = y;
            span->array->z[count] = z;

#if FLAGS & SPRITE
            for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
               GLuint attr = FRAG_ATTRIB_TEX0 + u;
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  if (ctx->Point.CoordReplace[u]) {
                     GLfloat s = 0.5F + (x + 0.5F - vert->win[0]) / size;
                     GLfloat t, r;
                     if (ctx->Point.SpriteOrigin == GL_LOWER_LEFT)
                        t = 0.5F + (y + 0.5F - vert->win[1]) / size;
                     else /* GL_UPPER_LEFT */
                        t = 0.5F - (y + 0.5F - vert->win[1]) / size;
                     if (ctx->Point.SpriteRMode == GL_ZERO)
                        r = 0.0F;
                     else if (ctx->Point.SpriteRMode == GL_S)
                        r = vert->attrib[attr][0];
                     else /* GL_R */
                        r = vert->attrib[attr][2];
                     span->array->attribs[attr][count][0] = s;
                     span->array->attribs[attr][count][1] = t;
                     span->array->attribs[attr][count][2] = r;
                     span->array->attribs[attr][count][3] = 1.0F;
                     span->array->lambda[u][count] = 0.0; /* XXX fix? */
                  }
                  else {
                     COPY_4V(span->array->attribs[attr][count],
                             vert->attrib[attr]);
                  }
               }
            }
#endif /*SPRITE*/

            count++;  /* square point */

#endif /*SMOOTH*/

	 } /*for x*/
      } /*for y*/
      span->end = count;
   }}

#else /* LARGE | ATTENUATE | SMOOTH | SPRITE */

   /***
    *** Single-pixel points
    ***/
   {{
      GLuint count;

      /* check if we need to flush */
      if (span->end >= MAX_WIDTH ||
          (swrast->_RasterMask & (BLEND_BIT | LOGIC_OP_BIT | MASKING_BIT))) {
#if FLAGS & RGBA
         _swrast_write_rgba_span(ctx, span);
#else
         _swrast_write_index_span(ctx, span);
#endif
         span->end = 0;
      }

      count = span->end;

#if FLAGS & RGBA
      span->array->rgba[count][RCOMP] = red;
      span->array->rgba[count][GCOMP] = green;
      span->array->rgba[count][BCOMP] = blue;
      span->array->rgba[count][ACOMP] = alpha;
#endif
#if FLAGS & SPECULAR
      span->array->spec[count][RCOMP] = specRed;
      span->array->spec[count][GCOMP] = specGreen;
      span->array->spec[count][BCOMP] = specBlue;
#endif
#if FLAGS & INDEX
      span->array->index[count] = colorIndex;
#endif
#if FLAGS & ATTRIBS
      ATTRIB_LOOP_BEGIN
         COPY_4V(span->array->attribs[attr][count], attribs[attr]);
      ATTRIB_LOOP_END
#endif

      span->array->x[count] = (GLint) vert->win[0];
      span->array->y[count] = (GLint) vert->win[1];
      span->array->z[count] = (GLint) (vert->win[2] + 0.5F);
      span->end = count + 1;
   }}

#endif /* LARGE || ATTENUATE || SMOOTH */

   ASSERT(span->end <= MAX_WIDTH);
}


#undef FLAGS
#undef NAME
