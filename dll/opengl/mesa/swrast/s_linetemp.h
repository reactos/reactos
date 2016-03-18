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
 * Line Rasterizer Template
 *
 * This file is #include'd to generate custom line rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated along the line:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_ATTRIBS  - if defined, interpolate attribs (texcoords, varying, etc)
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
NAME( struct gl_context *ctx, const SWvertex *vert0, const SWvertex *vert1 )
{
   const SWcontext *swrast = SWRAST_CONTEXT(ctx);
   SWspan span;
   GLuint interpFlags = 0;
   GLint x0 = (GLint) vert0->attrib[FRAG_ATTRIB_WPOS][0];
   GLint x1 = (GLint) vert1->attrib[FRAG_ATTRIB_WPOS][0];
   GLint y0 = (GLint) vert0->attrib[FRAG_ATTRIB_WPOS][1];
   GLint y1 = (GLint) vert1->attrib[FRAG_ATTRIB_WPOS][1];
   GLint dx, dy;
   GLint numPixels;
   GLint xstep, ystep;
#if defined(DEPTH_TYPE)
   const GLint depthBits = ctx->DrawBuffer->Visual.depthBits;
   const GLint fixedToDepthShift = depthBits <= 16 ? FIXED_SHIFT : 0;
   struct gl_renderbuffer *zrb = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
#define FixedToDepth(F)  ((F) >> fixedToDepthShift)
   GLint zPtrXstep, zPtrYstep;
   DEPTH_TYPE *zPtr;
#elif defined(INTERP_Z)
   const GLint depthBits = ctx->DrawBuffer->Visual.depthBits;
#endif
#ifdef PIXEL_ADDRESS
   PIXEL_TYPE *pixelPtr;
   GLint pixelXstep, pixelYstep;
#endif

#ifdef SETUP_CODE
   SETUP_CODE
#endif

   (void) swrast;

   /* Cull primitives with malformed coordinates.
    */
   {
      GLfloat tmp = vert0->attrib[FRAG_ATTRIB_WPOS][0] + vert0->attrib[FRAG_ATTRIB_WPOS][1]
                  + vert1->attrib[FRAG_ATTRIB_WPOS][0] + vert1->attrib[FRAG_ATTRIB_WPOS][1];
      if (IS_INF_OR_NAN(tmp))
	 return;
   }

   /*
   printf("%s():\n", __FUNCTION__);
   printf(" (%f, %f, %f) -> (%f, %f, %f)\n",
          vert0->attrib[FRAG_ATTRIB_WPOS][0],
          vert0->attrib[FRAG_ATTRIB_WPOS][1],
          vert0->attrib[FRAG_ATTRIB_WPOS][2],
          vert1->attrib[FRAG_ATTRIB_WPOS][0],
          vert1->attrib[FRAG_ATTRIB_WPOS][1],
          vert1->attrib[FRAG_ATTRIB_WPOS][2]);
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
   zPtr = (DEPTH_TYPE *) _swrast_pixel_address(zrb, x0, y0);
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
#if defined(INTERP_Z) || defined(DEPTH_TYPE)
   interpFlags |= SPAN_Z;
   {
      if (depthBits <= 16) {
         span.z = FloatToFixed(vert0->attrib[FRAG_ATTRIB_WPOS][2]) + FIXED_HALF;
         span.zStep = FloatToFixed(  vert1->attrib[FRAG_ATTRIB_WPOS][2]
                                   - vert0->attrib[FRAG_ATTRIB_WPOS][2]) / numPixels;
      }
      else {
         /* don't use fixed point */
         span.z = (GLuint) vert0->attrib[FRAG_ATTRIB_WPOS][2];
         span.zStep = (GLint) ((  vert1->attrib[FRAG_ATTRIB_WPOS][2]
                                - vert0->attrib[FRAG_ATTRIB_WPOS][2]) / numPixels);
      }
   }
#endif
#if defined(INTERP_ATTRIBS)
   {
      const GLfloat invLen = 1.0F / numPixels;
      const GLfloat invw0 = vert0->attrib[FRAG_ATTRIB_WPOS][3];
      const GLfloat invw1 = vert1->attrib[FRAG_ATTRIB_WPOS][3];

      span.attrStart[FRAG_ATTRIB_WPOS][3] = invw0;
      span.attrStepX[FRAG_ATTRIB_WPOS][3] = (invw1 - invw0) * invLen;
      span.attrStepY[FRAG_ATTRIB_WPOS][3] = 0.0;

      ATTRIB_LOOP_BEGIN
         if (swrast->_InterpMode[attr] == GL_FLAT) {
            COPY_4V(span.attrStart[attr], vert1->attrib[attr]);
            ASSIGN_4V(span.attrStepX[attr], 0.0, 0.0, 0.0, 0.0);
         }
         else {
            GLuint c;
            for (c = 0; c < 4; c++) {
               float da;
               span.attrStart[attr][c] = invw0 * vert0->attrib[attr][c];
               da = (invw1 * vert1->attrib[attr][c]) - span.attrStart[attr][c];
               span.attrStepX[attr][c] = da * invLen;
            }
         }
         ASSIGN_4V(span.attrStepY[attr], 0.0, 0.0, 0.0, 0.0);
      ATTRIB_LOOP_END
   }
#endif

   INIT_SPAN(span, GL_LINE);
   span.end = numPixels;
   span.interpMask = interpFlags;
   span.arrayMask = SPAN_XY;


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
         GLuint Z = FixedToDepth(span.z);
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
         if (error < 0) {
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
         GLuint Z = FixedToDepth(span.z);
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
#undef INTERP_ATTRIBS
#undef PIXEL_ADDRESS
#undef PIXEL_TYPE
#undef DEPTH_TYPE
#undef BYTES_PER_ROW
#undef SETUP_CODE
#undef PLOT
#undef CLIP_HACK
#undef FixedToDepth
#undef RENDER_SPAN
