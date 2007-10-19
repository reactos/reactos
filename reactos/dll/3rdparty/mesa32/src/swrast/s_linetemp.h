/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * Line Rasterizer Template
 *
 * This file is #include'd to generate custom line rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated along the line:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_FOG      - if defined, interpolate FOG values
 *    INTERP_RGBA     - if defined, interpolate RGBA values
 *    INTERP_SPEC     - if defined, interpolate specular RGB values
 *    INTERP_INDEX    - if defined, interpolate color index values
 *    INTERP_TEX      - if defined, interpolate unit 0 texcoords
 *    INTERP_MULTITEX - if defined, interpolate multi-texcoords
 *
 * When one can directly address pixels in the color buffer the following
 * macros can be defined and used to directly compute pixel addresses during
 * rasterization (see pixelPtr):
 *    PIXEL_TYPE          - the datatype of a pixel (GLubyte, GLushort, GLuint)
 *    BYTES_PER_ROW       - number of bytes per row in the color buffer
 *    PIXEL_ADDRESS(X,Y)  - returns the address of pixel at (X,Y) where
 *                          Y==0 at bottom of screen and increases upward.
 *
 * Similarly, for direct depth buffer access, this type is used for depth
 * buffer addressing:
 *    DEPTH_TYPE          - either GLushort or GLuint
 *
 * Optionally, one may provide one-time setup code
 *    SETUP_CODE    - code which is to be executed once per line
 *
 * To actually "plot" each pixel the PLOT macro must be defined...
 *    PLOT(X,Y) - code to plot a pixel.  Example:
 *                if (Z < *zPtr) {
 *                   *zPtr = Z;
 *                   color = pack_rgb( FixedToInt(r0), FixedToInt(g0),
 *                                     FixedToInt(b0) );
 *                   put_pixel( X, Y, color );
 *                }
 *
 * This code was designed for the origin to be in the lower-left corner.
 *
 */


static void
NAME( GLcontext *ctx, const SWvertex *vert0, const SWvertex *vert1 )
{
   struct sw_span span;
   GLuint interpFlags = 0;
   GLint x0 = (GLint) vert0->win[0];
   GLint x1 = (GLint) vert1->win[0];
   GLint y0 = (GLint) vert0->win[1];
   GLint y1 = (GLint) vert1->win[1];
   GLint dx, dy;
   GLint numPixels;
   GLint xstep, ystep;
#if defined(DEPTH_TYPE)
   const GLint depthBits = ctx->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
   struct gl_renderbuffer *zrb = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
#define FixedToDepth(F)  ((F) >> fixedToDepthShift)
   GLint zPtrXstep, zPtrYstep;
   DEPTH_TYPE *zPtr;
#elif defined(INTERP_Z)
   const GLint depthBits = ctx->Visual.depthBits;
#endif
#ifdef PIXEL_ADDRESS
   PIXEL_TYPE *pixelPtr;
   GLint pixelXstep, pixelYstep;
#endif

#ifdef SETUP_CODE
   SETUP_CODE
#endif

   /* Cull primitives with malformed coordinates.
    */
   {
      GLfloat tmp = vert0->win[0] + vert0->win[1]
                  + vert1->win[0] + vert1->win[1];
      if (IS_INF_OR_NAN(tmp))
	 return;
   }

   /*
   printf("%s():\n", __FUNCTION__);
   printf(" (%f, %f, %f) -> (%f, %f, %f)\n",
          vert0->win[0], vert0->win[1], vert0->win[2],
          vert1->win[0], vert1->win[1], vert1->win[2]);
   printf(" (%d, %d, %d) -> (%d, %d, %d)\n",
          vert0->color[0], vert0->color[1], vert0->color[2],
          vert1->color[0], vert1->color[1], vert1->color[2]);
   printf(" (%d, %d, %d) -> (%d, %d, %d)\n",
          vert0->specular[0], vert0->specular[1], vert0->specular[2],
          vert1->specular[0], vert1->specular[1], vert1->specular[2]);
   */

/*
 * Despite being clipped to the view volume, the line's window coordinates
 * may just lie outside the window bounds.  That is, if the legal window
 * coordinates are [0,W-1][0,H-1], it's possible for x==W and/or y==H.
 * This quick and dirty code nudges the endpoints inside the window if
 * necessary.
 */
#ifdef CLIP_HACK
   {
      GLint w = ctx->DrawBuffer->Width;
      GLint h = ctx->DrawBuffer->Height;
      if ((x0==w) | (x1==w)) {
         if ((x0==w) & (x1==w))
           return;
         x0 -= x0==w;
         x1 -= x1==w;
      }
      if ((y0==h) | (y1==h)) {
         if ((y0==h) & (y1==h))
           return;
         y0 -= y0==h;
         y1 -= y1==h;
      }
   }
#endif

   dx = x1 - x0;
   dy = y1 - y0;
   if (dx == 0 && dy == 0)
      return;

#ifdef DEPTH_TYPE
   zPtr = (DEPTH_TYPE *) zrb->GetPointer(ctx, zrb, x0, y0);
#endif
#ifdef PIXEL_ADDRESS
   pixelPtr = (PIXEL_TYPE *) PIXEL_ADDRESS(x0,y0);
#endif

   if (dx<0) {
      dx = -dx;   /* make positive */
      xstep = -1;
#ifdef DEPTH_TYPE
      zPtrXstep = -((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelXstep = -((GLint)sizeof(PIXEL_TYPE));
#endif
   }
   else {
      xstep = 1;
#ifdef DEPTH_TYPE
      zPtrXstep = ((GLint)sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelXstep = ((GLint)sizeof(PIXEL_TYPE));
#endif
   }

   if (dy<0) {
      dy = -dy;   /* make positive */
      ystep = -1;
#ifdef DEPTH_TYPE
      zPtrYstep = -((GLint) (ctx->DrawBuffer->Width * sizeof(DEPTH_TYPE)));
#endif
#ifdef PIXEL_ADDRESS
      pixelYstep = BYTES_PER_ROW;
#endif
   }
   else {
      ystep = 1;
#ifdef DEPTH_TYPE
      zPtrYstep = (GLint) (ctx->DrawBuffer->Width * sizeof(DEPTH_TYPE));
#endif
#ifdef PIXEL_ADDRESS
      pixelYstep = -(BYTES_PER_ROW);
#endif
   }

   ASSERT(dx >= 0);
   ASSERT(dy >= 0);

   numPixels = MAX2(dx, dy);

   /*
    * Span setup: compute start and step values for all interpolated values.
    */
#ifdef INTERP_RGBA
   interpFlags |= SPAN_RGBA;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      span.red   = ChanToFixed(vert0->color[0]);
      span.green = ChanToFixed(vert0->color[1]);
      span.blue  = ChanToFixed(vert0->color[2]);
      span.alpha = ChanToFixed(vert0->color[3]);
      span.redStep   = (ChanToFixed(vert1->color[0]) - span.red  ) / numPixels;
      span.greenStep = (ChanToFixed(vert1->color[1]) - span.green) / numPixels;
      span.blueStep  = (ChanToFixed(vert1->color[2]) - span.blue ) / numPixels;
      span.alphaStep = (ChanToFixed(vert1->color[3]) - span.alpha) / numPixels;
   }
   else {
      span.red   = ChanToFixed(vert1->color[0]);
      span.green = ChanToFixed(vert1->color[1]);
      span.blue  = ChanToFixed(vert1->color[2]);
      span.alpha = ChanToFixed(vert1->color[3]);
      span.redStep   = 0;
      span.greenStep = 0;
      span.blueStep  = 0;
      span.alphaStep = 0;
   }
#endif
#ifdef INTERP_SPEC
   interpFlags |= SPAN_SPEC;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      span.specRed       = ChanToFixed(vert0->specular[0]);
      span.specGreen     = ChanToFixed(vert0->specular[1]);
      span.specBlue      = ChanToFixed(vert0->specular[2]);
      span.specRedStep   = (ChanToFixed(vert1->specular[0]) - span.specRed) / numPixels;
      span.specGreenStep = (ChanToFixed(vert1->specular[1]) - span.specBlue) / numPixels;
      span.specBlueStep  = (ChanToFixed(vert1->specular[2]) - span.specGreen) / numPixels;
   }
   else {
      span.specRed       = ChanToFixed(vert1->specular[0]);
      span.specGreen     = ChanToFixed(vert1->specular[1]);
      span.specBlue      = ChanToFixed(vert1->specular[2]);
      span.specRedStep   = 0;
      span.specGreenStep = 0;
      span.specBlueStep  = 0;
   }
#endif
#ifdef INTERP_INDEX
   interpFlags |= SPAN_INDEX;
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      span.index = FloatToFixed(vert0->index);
      span.indexStep = FloatToFixed(vert1->index - vert0->index) / numPixels;
   }
   else {
      span.index = FloatToFixed(vert1->index);
      span.indexStep = 0;
   }
#endif
#if defined(INTERP_Z) || defined(DEPTH_TYPE)
   interpFlags |= SPAN_Z;
   {
      if (depthBits <= 16) {
         span.z = FloatToFixed(vert0->win[2]) + FIXED_HALF;
         span.zStep = FloatToFixed(vert1->win[2] - vert0->win[2]) / numPixels;
      }
      else {
         /* don't use fixed point */
         span.z = (GLint) vert0->win[2];
         span.zStep = (GLint) ((vert1->win[2] - vert0->win[2]) / numPixels);
      }
   }
#endif
#ifdef INTERP_FOG
   interpFlags |= SPAN_FOG;
   span.fog = vert0->fog;
   span.fogStep = (vert1->fog - vert0->fog) / numPixels;
#endif
#ifdef INTERP_TEX
   interpFlags |= SPAN_TEXTURE;
   {
      const GLfloat invw0 = vert0->win[3];
      const GLfloat invw1 = vert1->win[3];
      const GLfloat invLen = 1.0F / numPixels;
      GLfloat ds, dt, dr, dq;
      span.tex[0][0] = invw0 * vert0->texcoord[0][0];
      span.tex[0][1] = invw0 * vert0->texcoord[0][1];
      span.tex[0][2] = invw0 * vert0->texcoord[0][2];
      span.tex[0][3] = invw0 * vert0->texcoord[0][3];
      ds = (invw1 * vert1->texcoord[0][0]) - span.tex[0][0];
      dt = (invw1 * vert1->texcoord[0][1]) - span.tex[0][1];
      dr = (invw1 * vert1->texcoord[0][2]) - span.tex[0][2];
      dq = (invw1 * vert1->texcoord[0][3]) - span.tex[0][3];
      span.texStepX[0][0] = ds * invLen;
      span.texStepX[0][1] = dt * invLen;
      span.texStepX[0][2] = dr * invLen;
      span.texStepX[0][3] = dq * invLen;
      span.texStepY[0][0] = 0.0F;
      span.texStepY[0][1] = 0.0F;
      span.texStepY[0][2] = 0.0F;
      span.texStepY[0][3] = 0.0F;
   }
#endif
#ifdef INTERP_MULTITEX
   interpFlags |= SPAN_TEXTURE;
   {
      const GLfloat invLen = 1.0F / numPixels;
      GLuint u;
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (ctx->Texture.Unit[u]._ReallyEnabled) {
            const GLfloat invw0 = vert0->win[3];
            const GLfloat invw1 = vert1->win[3];
            GLfloat ds, dt, dr, dq;
            span.tex[u][0] = invw0 * vert0->texcoord[u][0];
            span.tex[u][1] = invw0 * vert0->texcoord[u][1];
            span.tex[u][2] = invw0 * vert0->texcoord[u][2];
            span.tex[u][3] = invw0 * vert0->texcoord[u][3];
            ds = (invw1 * vert1->texcoord[u][0]) - span.tex[u][0];
            dt = (invw1 * vert1->texcoord[u][1]) - span.tex[u][1];
            dr = (invw1 * vert1->texcoord[u][2]) - span.tex[u][2];
            dq = (invw1 * vert1->texcoord[u][3]) - span.tex[u][3];
            span.texStepX[u][0] = ds * invLen;
            span.texStepX[u][1] = dt * invLen;
            span.texStepX[u][2] = dr * invLen;
            span.texStepX[u][3] = dq * invLen;
            span.texStepY[u][0] = 0.0F;
            span.texStepY[u][1] = 0.0F;
            span.texStepY[u][2] = 0.0F;
            span.texStepY[u][3] = 0.0F;
	 }
      }
   }
#endif

   INIT_SPAN(span, GL_LINE, numPixels, interpFlags, SPAN_XY);

   /* Need these for fragment prog texcoord interpolation */
   span.w = 1.0F;
   span.dwdx = 0.0F;
   span.dwdy = 0.0F;

   /*
    * Draw
    */

   if (dx > dy) {
      /*** X-major line ***/
      GLint i;
      GLint errorInc = dy+dy;
      GLint error = errorInc-dx;
      GLint errorDec = error-dx;

      for (i = 0; i < dx; i++) {
#ifdef DEPTH_TYPE
         GLdepth Z = FixedToDepth(span.z);
#endif
#ifdef PLOT
         PLOT( x0, y0 );
#else
         span.array->x[i] = x0;
         span.array->y[i] = y0;
#endif
         x0 += xstep;
#ifdef DEPTH_TYPE
         zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrXstep);
         span.z += span.zStep;
#endif
#ifdef PIXEL_ADDRESS
         pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelXstep);
#endif
         if (error<0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            y0 += ystep;
#ifdef DEPTH_TYPE
            zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrYstep);
#endif
#ifdef PIXEL_ADDRESS
            pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelYstep);
#endif
         }
      }
   }
   else {
      /*** Y-major line ***/
      GLint i;
      GLint errorInc = dx+dx;
      GLint error = errorInc-dy;
      GLint errorDec = error-dy;

      for (i=0;i<dy;i++) {
#ifdef DEPTH_TYPE
         GLdepth Z = FixedToDepth(span.z);
#endif
#ifdef PLOT
         PLOT( x0, y0 );
#else
         span.array->x[i] = x0;
         span.array->y[i] = y0;
#endif
         y0 += ystep;
#ifdef DEPTH_TYPE
         zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrYstep);
         span.z += span.zStep;
#endif
#ifdef PIXEL_ADDRESS
         pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelYstep);
#endif
         if (error<0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            x0 += xstep;
#ifdef DEPTH_TYPE
            zPtr = (DEPTH_TYPE *) ((GLubyte*) zPtr + zPtrXstep);
#endif
#ifdef PIXEL_ADDRESS
            pixelPtr = (PIXEL_TYPE*) ((GLubyte*) pixelPtr + pixelXstep);
#endif
         }
      }
   }

#ifdef RENDER_SPAN
   RENDER_SPAN( span );
#endif

   (void)span;

}


#undef NAME
#undef INTERP_Z
#undef INTERP_FOG
#undef INTERP_RGBA
#undef INTERP_SPEC
#undef INTERP_TEX
#undef INTERP_MULTITEX
#undef INTERP_INDEX
#undef PIXEL_ADDRESS
#undef PIXEL_TYPE
#undef DEPTH_TYPE
#undef BYTES_PER_ROW
#undef SETUP_CODE
#undef PLOT
#undef CLIP_HACK
#undef FixedToDepth
#undef RENDER_SPAN
