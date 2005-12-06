
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

#include "glheader.h"
#include "macros.h"
#include "imports.h"
#include "colormac.h"

#include "s_context.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"


/*
 * Helper function called from _swrast_write_zoomed_rgba/rgb/index_span().
 */
static void
zoom_span( GLcontext *ctx, const struct sw_span *span,
           const GLvoid *src, GLint y0, GLenum format, GLint skipPixels )
{
   GLint r0, r1, row;
   GLint c0, c1, skipCol;
   GLint i, j;
   const GLuint maxWidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );
   struct sw_span zoomed;
   struct span_arrays zoomed_arrays;  /* this is big! */

   /* no pixel arrays! must be horizontal spans. */
   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(span->primitive == GL_BITMAP);

   INIT_SPAN(zoomed, GL_BITMAP, 0, 0, 0);
   zoomed.array = &zoomed_arrays;

   /* copy fog interp info */
   zoomed.fog = span->fog;
   zoomed.fogStep = span->fogStep;
   /* XXX copy texcoord info? */

   if (format == GL_RGBA || format == GL_RGB) {
      /* copy Z info */
      zoomed.z = span->z;
      zoomed.zStep = span->zStep;
      /* we'll generate an array of colorss */
      zoomed.interpMask = span->interpMask & ~SPAN_RGBA;
      zoomed.arrayMask |= SPAN_RGBA;
   }
   else if (format == GL_COLOR_INDEX) {
      /* copy Z info */
      zoomed.z = span->z;
      zoomed.zStep = span->zStep;
      /* we'll generate an array of color indexes */
      zoomed.interpMask = span->interpMask & ~SPAN_INDEX;
      zoomed.arrayMask |= SPAN_INDEX;
   }
   else {
      assert(format == GL_DEPTH_COMPONENT);
      /* Copy color info */
      zoomed.red = span->red;
      zoomed.green = span->green;
      zoomed.blue = span->blue;
      zoomed.alpha = span->alpha;
      zoomed.redStep = span->redStep;
      zoomed.greenStep = span->greenStep;
      zoomed.blueStep = span->blueStep;
      zoomed.alphaStep = span->alphaStep;
      /* we'll generate an array of depth values */
      zoomed.interpMask = span->interpMask & ~SPAN_Z;
      zoomed.arrayMask |= SPAN_Z;
   }

   /*
    * Compute which columns to draw: [c0, c1)
    */
   c0 = (GLint) (span->x + skipPixels * ctx->Pixel.ZoomX);
   c1 = (GLint) (span->x + (skipPixels + span->end) * ctx->Pixel.ZoomX);
   if (c0 == c1) {
      return;
   }
   else if (c1 < c0) {
      /* swap */
      GLint ctmp = c1;
      c1 = c0;
      c0 = ctmp;
   }
   if (c0 < 0) {
      zoomed.x = 0;
      zoomed.start = 0;
      zoomed.end = c1;
      skipCol = -c0;
   }
   else {
      zoomed.x = c0;
      zoomed.start = 0;
      zoomed.end = c1 - c0;
      skipCol = 0;
   }
   if (zoomed.end > maxWidth)
      zoomed.end = maxWidth;

   /*
    * Compute which rows to draw: [r0, r1)
    */
   row = span->y - y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0 == r1) {
      return;
   }
   else if (r1 < r0) {
      /* swap */
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   ASSERT(r0 < r1);
   ASSERT(c0 < c1);

   /*
    * Trivial clip rejection testing.
    */
   if (r1 < 0) /* below window */
      return;
   if (r0 >= (GLint) ctx->DrawBuffer->Height) /* above window */
      return;
   if (c1 < 0) /* left of window */
      return;
   if (c0 >= (GLint) ctx->DrawBuffer->Width) /* right of window */
      return;

   /* zoom the span horizontally */
   if (format == GL_RGBA) {
      const GLchan (*rgba)[4] = (const GLchan (*)[4]) src;
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            COPY_CHAN4(zoomed.array->rgba[j], rgba[i]);
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (ctx->Pixel.ZoomX < 0.0) {
               ASSERT(i <= 0);
               i = span->end + i - 1;
            }
            ASSERT(i >= 0);
            ASSERT(i < (GLint)  span->end);
            COPY_CHAN4(zoomed.array->rgba[j], rgba[i]);
         }
      }
   }
   else if (format == GL_RGB) {
      const GLchan (*rgb)[3] = (const GLchan (*)[3]) src;
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            zoomed.array->rgba[j][0] = rgb[i][0];
            zoomed.array->rgba[j][1] = rgb[i][1];
            zoomed.array->rgba[j][2] = rgb[i][2];
            zoomed.array->rgba[j][3] = CHAN_MAX;
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (ctx->Pixel.ZoomX < 0.0) {
               ASSERT(i <= 0);
               i = span->end + i - 1;
            }
            ASSERT(i >= 0);
            ASSERT(i < (GLint) span->end);
            zoomed.array->rgba[j][0] = rgb[i][0];
            zoomed.array->rgba[j][1] = rgb[i][1];
            zoomed.array->rgba[j][2] = rgb[i][2];
            zoomed.array->rgba[j][3] = CHAN_MAX;
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      const GLuint *indexes = (const GLuint *) src;
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            zoomed.array->index[j] = indexes[i];
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (ctx->Pixel.ZoomX < 0.0) {
               ASSERT(i <= 0);
               i = span->end + i - 1;
            }
            ASSERT(i >= 0);
            ASSERT(i < (GLint) span->end);
            zoomed.array->index[j] = indexes[i];
         }
      }
   }
   else {
      const GLdepth *zValues = (const GLuint *) src;
      assert(format == GL_DEPTH_COMPONENT);
      if (ctx->Pixel.ZoomX == -1.0F) {
         /* common case */
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = span->end - (j + skipCol) - 1;
            zoomed.array->z[j] = zValues[i];
         }
      }
      else {
         /* general solution */
         const GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
         for (j = (GLint) zoomed.start; j < (GLint) zoomed.end; j++) {
            i = (GLint) ((j + skipCol) * xscale);
            if (ctx->Pixel.ZoomX < 0.0) {
               ASSERT(i <= 0);
               i = span->end + i - 1;
            }
            ASSERT(i >= 0);
            ASSERT(i < (GLint) span->end);
            zoomed.array->z[j] = zValues[i];
         }
      }
      /* Now, fall into either the RGB or COLOR_INDEX path below */
      if (ctx->Visual.rgbMode)
         format = GL_RGBA;
      else
         format = GL_COLOR_INDEX;
   }


   /* write the span in rows [r0, r1) */
   if (format == GL_RGBA || format == GL_RGB) {
      /* Writing the span may modify the colors, so make a backup now if we're
       * going to call _swrast_write_zoomed_span() more than once.
       * Also, clipping may change the span end value, so store it as well.
       */
      GLchan rgbaSave[MAX_WIDTH][4];
      const GLint end = zoomed.end; /* save */
      if (r1 - r0 > 1) {
         MEMCPY(rgbaSave, zoomed.array->rgba, zoomed.end * 4 * sizeof(GLchan));
      }
      for (zoomed.y = r0; zoomed.y < r1; zoomed.y++) {
         _swrast_write_rgba_span(ctx, &zoomed);
         zoomed.end = end;  /* restore */
         if (r1 - r0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.array->rgba, rgbaSave, zoomed.end*4 * sizeof(GLchan));
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      GLuint indexSave[MAX_WIDTH];
      const GLint end = zoomed.end; /* save */
      if (r1 - r0 > 1) {
         MEMCPY(indexSave, zoomed.array->index, zoomed.end * sizeof(GLuint));
      }
      for (zoomed.y = r0; zoomed.y < r1; zoomed.y++) {
         _swrast_write_index_span(ctx, &zoomed);
         zoomed.end = end;  /* restore */
         if (r1 - r0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.array->index, indexSave, zoomed.end * sizeof(GLuint));
         }
      }
   }
}


void
_swrast_write_zoomed_rgba_span( GLcontext *ctx, const struct sw_span *span,
                              CONST GLchan rgba[][4], GLint y0,
                              GLint skipPixels )
{
   zoom_span(ctx, span, (const GLvoid *) rgba, y0, GL_RGBA, skipPixels);
}


void
_swrast_write_zoomed_rgb_span( GLcontext *ctx, const struct sw_span *span,
                             CONST GLchan rgb[][3], GLint y0,
                             GLint skipPixels )
{
   zoom_span(ctx, span, (const GLvoid *) rgb, y0, GL_RGB, skipPixels);
}


void
_swrast_write_zoomed_index_span( GLcontext *ctx, const struct sw_span *span,
                               GLint y0, GLint skipPixels )
{
   zoom_span(ctx, span, (const GLvoid *) span->array->index, y0,
             GL_COLOR_INDEX, skipPixels);
}


void
_swrast_write_zoomed_depth_span( GLcontext *ctx, const struct sw_span *span,
                                 GLint y0, GLint skipPixels )
{
   zoom_span(ctx, span, (const GLvoid *) span->array->z, y0,
             GL_DEPTH_COMPONENT, skipPixels);
}


/*
 * As above, but write stencil values.
 */
void
_swrast_write_zoomed_stencil_span( GLcontext *ctx,
                                 GLuint n, GLint x, GLint y,
                                 const GLstencil stencil[], GLint y0,
                                 GLint skipPixels )
{
   GLint m;
   GLint r0, r1, row, r;
   GLint i, j, skipcol;
   GLstencil zstencil[MAX_WIDTH];  /* zoomed stencil values */
   GLint maxwidth = MIN2( ctx->DrawBuffer->Width, MAX_WIDTH );

   (void) skipPixels;  /* XXX this shouldn't be ignored */

   /* compute width of output row */
   m = (GLint) FABSF( n * ctx->Pixel.ZoomX );
   if (m==0) {
      return;
   }
   if (ctx->Pixel.ZoomX<0.0) {
      /* adjust x coordinate for left/right mirroring */
      x = x - m;
   }

   /* compute which rows to draw */
   row = y - y0;
   r0 = y0 + (GLint) (row * ctx->Pixel.ZoomY);
   r1 = y0 + (GLint) ((row+1) * ctx->Pixel.ZoomY);
   if (r0==r1) {
      return;
   }
   else if (r1<r0) {
      GLint rtmp = r1;
      r1 = r0;
      r0 = rtmp;
   }

   /* return early if r0...r1 is above or below window */
   if (r0<0 && r1<0) {
      /* below window */
      return;
   }
   if (r0 >= (GLint) ctx->DrawBuffer->Height &&
       r1 >= (GLint) ctx->DrawBuffer->Height) {
      /* above window */
      return;
   }

   /* check if left edge is outside window */
   skipcol = 0;
   if (x<0) {
      skipcol = -x;
      m += x;
   }
   /* make sure span isn't too long or short */
   if (m>maxwidth) {
      m = maxwidth;
   }
   else if (m<=0) {
      return;
   }

   ASSERT( m <= MAX_WIDTH );

   /* zoom the span horizontally */
   if (ctx->Pixel.ZoomX==-1.0F) {
      /* n==m */
      for (j=0;j<m;j++) {
         i = n - (j+skipcol) - 1;
         zstencil[j] = stencil[i];
      }
   }
   else {
      GLfloat xscale = 1.0F / ctx->Pixel.ZoomX;
      for (j=0;j<m;j++) {
         i = (GLint) ((j+skipcol) * xscale);
         if (i<0)  i = n + i - 1;
         zstencil[j] = stencil[i];
      }
   }

   /* write the span */
   for (r=r0; r<r1; r++) {
      _swrast_write_stencil_span( ctx, m, x+skipcol, r, zstencil );
   }
}
