/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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

#include "main/glheader.h"
#include "main/macros.h"
#include "main/imports.h"
#include "main/colormac.h"

#include "s_context.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_zoom.h"


/**
 * Compute the bounds of the region resulting from zooming a pixel span.
 * The resulting region will be entirely inside the window/scissor bounds
 * so no additional clipping is needed.
 * \param imageX, imageY  position of the mage being drawn (gl WindowPos)
 * \param spanX, spanY  position of span being drawing
 * \param width  number of pixels in span
 * \param x0, x1  returned X bounds of zoomed region [x0, x1)
 * \param y0, y1  returned Y bounds of zoomed region [y0, y1)
 * \return GL_TRUE if any zoomed pixels visible, GL_FALSE if totally clipped
 */
static GLboolean
compute_zoomed_bounds(GLcontext *ctx, GLint imageX, GLint imageY,
                      GLint spanX, GLint spanY, GLint width,
                      GLint *x0, GLint *x1, GLint *y0, GLint *y1)
{
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLint c0, c1, r0, r1;

   ASSERT(spanX >= imageX);
   ASSERT(spanY >= imageY);

   /*
    * Compute destination columns: [c0, c1)
    */
   c0 = imageX + (GLint) ((spanX - imageX) * ctx->Pixel.ZoomX);
   c1 = imageX + (GLint) ((spanX + width - imageX) * ctx->Pixel.ZoomX);
   if (c1 < c0) {
      /* swap */
      GLint tmp = c1;
      c1 = c0;
      c0 = tmp;
   }
   c0 = CLAMP(c0, fb->_Xmin, fb->_Xmax);
   c1 = CLAMP(c1, fb->_Xmin, fb->_Xmax);
   if (c0 == c1) {
      return GL_FALSE; /* no width */
   }

   /*
    * Compute destination rows: [r0, r1)
    */
   r0 = imageY + (GLint) ((spanY - imageY) * ctx->Pixel.ZoomY);
   r1 = imageY + (GLint) ((spanY + 1 - imageY) * ctx->Pixel.ZoomY);
   if (r1 < r0) {
      /* swap */
      GLint tmp = r1;
      r1 = r0;
      r0 = tmp;
   }
   r0 = CLAMP(r0, fb->_Ymin, fb->_Ymax);
   r1 = CLAMP(r1, fb->_Ymin, fb->_Ymax);
   if (r0 == r1) {
      return GL_FALSE; /* no height */
   }

   *x0 = c0;
   *x1 = c1;
   *y0 = r0;
   *y1 = r1;

   return GL_TRUE;
}


/**
 * Convert a zoomed x image coordinate back to an unzoomed x coord.
 * 'zx' is screen position of a pixel in the zoomed image, who's left edge
 * is at 'imageX'.
 * return corresponding x coord in the original, unzoomed image.
 * This can use this for unzooming X or Y values.
 */
static INLINE GLint
unzoom_x(GLfloat zoomX, GLint imageX, GLint zx)
{
   /*
   zx = imageX + (x - imageX) * zoomX;
   zx - imageX = (x - imageX) * zoomX;
   (zx - imageX) / zoomX = x - imageX;
   */
   GLint x;
   if (zoomX < 0.0)
      zx++;
   x = imageX + (GLint) ((zx - imageX) / zoomX);
   return x;
}



/**
 * Helper function called from _swrast_write_zoomed_rgba/rgb/
 * index/depth_span().
 */
static void
zoom_span( GLcontext *ctx, GLint imgX, GLint imgY, const SWspan *span,
           const GLvoid *src, GLenum format )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   SWspan zoomed;
   GLint x0, x1, y0, y1;
   GLint zoomedWidth;

   if (!compute_zoomed_bounds(ctx, imgX, imgY, span->x, span->y, span->end,
                              &x0, &x1, &y0, &y1)) {
      return;  /* totally clipped */
   }

   if (!swrast->ZoomedArrays) {
      /* allocate on demand */
      swrast->ZoomedArrays = (SWspanarrays *) CALLOC(sizeof(SWspanarrays));
      if (!swrast->ZoomedArrays)
         return;
   }

   zoomedWidth = x1 - x0;
   ASSERT(zoomedWidth > 0);
   ASSERT(zoomedWidth <= MAX_WIDTH);

   /* no pixel arrays! must be horizontal spans. */
   ASSERT((span->arrayMask & SPAN_XY) == 0);
   ASSERT(span->primitive == GL_BITMAP);

   INIT_SPAN(zoomed, GL_BITMAP);
   zoomed.x = x0;
   zoomed.end = zoomedWidth;
   zoomed.array = swrast->ZoomedArrays;
   zoomed.array->ChanType = span->array->ChanType;
   if (zoomed.array->ChanType == GL_UNSIGNED_BYTE)
      zoomed.array->rgba = (GLchan (*)[4]) zoomed.array->rgba8;
   else if (zoomed.array->ChanType == GL_UNSIGNED_SHORT)
      zoomed.array->rgba = (GLchan (*)[4]) zoomed.array->rgba16;
   else
      zoomed.array->rgba = (GLchan (*)[4]) zoomed.array->attribs[FRAG_ATTRIB_COL0];

   COPY_4V(zoomed.attrStart[FRAG_ATTRIB_WPOS], span->attrStart[FRAG_ATTRIB_WPOS]);
   COPY_4V(zoomed.attrStepX[FRAG_ATTRIB_WPOS], span->attrStepX[FRAG_ATTRIB_WPOS]);
   COPY_4V(zoomed.attrStepY[FRAG_ATTRIB_WPOS], span->attrStepY[FRAG_ATTRIB_WPOS]);

   zoomed.attrStart[FRAG_ATTRIB_FOGC][0] = span->attrStart[FRAG_ATTRIB_FOGC][0];
   zoomed.attrStepX[FRAG_ATTRIB_FOGC][0] = span->attrStepX[FRAG_ATTRIB_FOGC][0];
   zoomed.attrStepY[FRAG_ATTRIB_FOGC][0] = span->attrStepY[FRAG_ATTRIB_FOGC][0];

   if (format == GL_RGBA || format == GL_RGB) {
      /* copy Z info */
      zoomed.z = span->z;
      zoomed.zStep = span->zStep;
      /* we'll generate an array of colorss */
      zoomed.interpMask = span->interpMask & ~SPAN_RGBA;
      zoomed.arrayMask |= SPAN_RGBA;
      zoomed.arrayAttribs |= FRAG_BIT_COL0;  /* we'll produce these values */
      ASSERT(span->arrayMask & SPAN_RGBA);
   }
   else if (format == GL_COLOR_INDEX) {
      /* copy Z info */
      zoomed.z = span->z;
      zoomed.zStep = span->zStep;
      /* we'll generate an array of color indexes */
      zoomed.interpMask = span->interpMask & ~SPAN_INDEX;
      zoomed.arrayMask |= SPAN_INDEX;
      ASSERT(span->arrayMask & SPAN_INDEX);
   }
   else if (format == GL_DEPTH_COMPONENT) {
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
      ASSERT(span->arrayMask & SPAN_Z);
   }
   else {
      _mesa_problem(ctx, "Bad format in zoom_span");
      return;
   }

   /* zoom the span horizontally */
   if (format == GL_RGBA) {
      if (zoomed.array->ChanType == GL_UNSIGNED_BYTE) {
         const GLubyte (*rgba)[4] = (const GLubyte (*)[4]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < (GLint) span->end);
            COPY_4UBV(zoomed.array->rgba8[i], rgba[j]);
         }
      }
      else if (zoomed.array->ChanType == GL_UNSIGNED_SHORT) {
         const GLushort (*rgba)[4] = (const GLushort (*)[4]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < (GLint) span->end);
            COPY_4V(zoomed.array->rgba16[i], rgba[j]);
         }
      }
      else {
         const GLfloat (*rgba)[4] = (const GLfloat (*)[4]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < span->end);
            COPY_4V(zoomed.array->attribs[FRAG_ATTRIB_COL0][i], rgba[j]);
         }
      }
   }
   else if (format == GL_RGB) {
      if (zoomed.array->ChanType == GL_UNSIGNED_BYTE) {
         const GLubyte (*rgb)[3] = (const GLubyte (*)[3]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < (GLint) span->end);
            zoomed.array->rgba8[i][0] = rgb[j][0];
            zoomed.array->rgba8[i][1] = rgb[j][1];
            zoomed.array->rgba8[i][2] = rgb[j][2];
            zoomed.array->rgba8[i][3] = 0xff;
         }
      }
      else if (zoomed.array->ChanType == GL_UNSIGNED_SHORT) {
         const GLushort (*rgb)[3] = (const GLushort (*)[3]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < (GLint) span->end);
            zoomed.array->rgba16[i][0] = rgb[j][0];
            zoomed.array->rgba16[i][1] = rgb[j][1];
            zoomed.array->rgba16[i][2] = rgb[j][2];
            zoomed.array->rgba16[i][3] = 0xffff;
         }
      }
      else {
         const GLfloat (*rgb)[3] = (const GLfloat (*)[3]) src;
         GLint i;
         for (i = 0; i < zoomedWidth; i++) {
            GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
            ASSERT(j >= 0);
            ASSERT(j < span->end);
            zoomed.array->attribs[FRAG_ATTRIB_COL0][i][0] = rgb[j][0];
            zoomed.array->attribs[FRAG_ATTRIB_COL0][i][1] = rgb[j][1];
            zoomed.array->attribs[FRAG_ATTRIB_COL0][i][2] = rgb[j][2];
            zoomed.array->attribs[FRAG_ATTRIB_COL0][i][3] = 1.0F;
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      const GLuint *indexes = (const GLuint *) src;
      GLint i;
      for (i = 0; i < zoomedWidth; i++) {
         GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
         ASSERT(j >= 0);
         ASSERT(j < (GLint) span->end);
         zoomed.array->index[i] = indexes[j];
      }
   }
   else if (format == GL_DEPTH_COMPONENT) {
      const GLuint *zValues = (const GLuint *) src;
      GLint i;
      for (i = 0; i < zoomedWidth; i++) {
         GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - span->x;
         ASSERT(j >= 0);
         ASSERT(j < (GLint) span->end);
         zoomed.array->z[i] = zValues[j];
      }
      /* Now, fall into either the RGB or COLOR_INDEX path below */
      format = ctx->Visual.rgbMode ? GL_RGBA : GL_COLOR_INDEX;
   }

   /* write the span in rows [r0, r1) */
   if (format == GL_RGBA || format == GL_RGB) {
      /* Writing the span may modify the colors, so make a backup now if we're
       * going to call _swrast_write_zoomed_span() more than once.
       * Also, clipping may change the span end value, so store it as well.
       */
      const GLint end = zoomed.end; /* save */
      GLuint rgbaSave[MAX_WIDTH][4];
      const GLint pixelSize =
         (zoomed.array->ChanType == GL_UNSIGNED_BYTE) ? 4 * sizeof(GLubyte) :
         ((zoomed.array->ChanType == GL_UNSIGNED_SHORT) ? 4 * sizeof(GLushort)
          : 4 * sizeof(GLfloat));
      if (y1 - y0 > 1) {
         MEMCPY(rgbaSave, zoomed.array->rgba, zoomed.end * pixelSize);
      }
      for (zoomed.y = y0; zoomed.y < y1; zoomed.y++) {
         _swrast_write_rgba_span(ctx, &zoomed);
         zoomed.end = end;  /* restore */
         if (y1 - y0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.array->rgba, rgbaSave, zoomed.end * pixelSize);
         }
      }
   }
   else if (format == GL_COLOR_INDEX) {
      /* use specular color array for temp storage */
      GLuint *indexSave = (GLuint *) zoomed.array->attribs[FRAG_ATTRIB_FOGC];
      const GLint end = zoomed.end; /* save */
      if (y1 - y0 > 1) {
         MEMCPY(indexSave, zoomed.array->index, zoomed.end * sizeof(GLuint));
      }
      for (zoomed.y = y0; zoomed.y < y1; zoomed.y++) {
         _swrast_write_index_span(ctx, &zoomed);
         zoomed.end = end;  /* restore */
         if (y1 - y0 > 1) {
            /* restore the colors */
            MEMCPY(zoomed.array->index, indexSave, zoomed.end * sizeof(GLuint));
         }
      }
   }
}


void
_swrast_write_zoomed_rgba_span(GLcontext *ctx, GLint imgX, GLint imgY,
                               const SWspan *span, const GLvoid *rgba)
{
   zoom_span(ctx, imgX, imgY, span, rgba, GL_RGBA);
}


void
_swrast_write_zoomed_rgb_span(GLcontext *ctx, GLint imgX, GLint imgY,
                              const SWspan *span, const GLvoid *rgb)
{
   zoom_span(ctx, imgX, imgY, span, rgb, GL_RGB);
}


void
_swrast_write_zoomed_index_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                const SWspan *span)
{
   zoom_span(ctx, imgX, imgY, span,
             (const GLvoid *) span->array->index, GL_COLOR_INDEX);
}


void
_swrast_write_zoomed_depth_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                const SWspan *span)
{
   zoom_span(ctx, imgX, imgY, span,
             (const GLvoid *) span->array->z, GL_DEPTH_COMPONENT);
}


/**
 * Zoom/write stencil values.
 * No per-fragment operations are applied.
 */
void
_swrast_write_zoomed_stencil_span(GLcontext *ctx, GLint imgX, GLint imgY,
                                  GLint width, GLint spanX, GLint spanY,
                                  const GLstencil stencil[])
{
   GLstencil zoomedVals[MAX_WIDTH];
   GLint x0, x1, y0, y1, y;
   GLint i, zoomedWidth;

   if (!compute_zoomed_bounds(ctx, imgX, imgY, spanX, spanY, width,
                              &x0, &x1, &y0, &y1)) {
      return;  /* totally clipped */
   }

   zoomedWidth = x1 - x0;
   ASSERT(zoomedWidth > 0);
   ASSERT(zoomedWidth <= MAX_WIDTH);

   /* zoom the span horizontally */
   for (i = 0; i < zoomedWidth; i++) {
      GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - spanX;
      ASSERT(j >= 0);
      ASSERT(j < width);
      zoomedVals[i] = stencil[j];
   }

   /* write the zoomed spans */
   for (y = y0; y < y1; y++) {
      _swrast_write_stencil_span(ctx, zoomedWidth, x0, y, zoomedVals);
   }
}


/**
 * Zoom/write z values (16 or 32-bit).
 * No per-fragment operations are applied.
 */
void
_swrast_write_zoomed_z_span(GLcontext *ctx, GLint imgX, GLint imgY,
                            GLint width, GLint spanX, GLint spanY,
                            const GLvoid *z)
{
   struct gl_renderbuffer *rb = ctx->DrawBuffer->_DepthBuffer;
   GLushort zoomedVals16[MAX_WIDTH];
   GLuint zoomedVals32[MAX_WIDTH];
   GLint x0, x1, y0, y1, y;
   GLint i, zoomedWidth;

   if (!compute_zoomed_bounds(ctx, imgX, imgY, spanX, spanY, width,
                              &x0, &x1, &y0, &y1)) {
      return;  /* totally clipped */
   }

   zoomedWidth = x1 - x0;
   ASSERT(zoomedWidth > 0);
   ASSERT(zoomedWidth <= MAX_WIDTH);

   /* zoom the span horizontally */
   if (rb->DataType == GL_UNSIGNED_SHORT) {
      for (i = 0; i < zoomedWidth; i++) {
         GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - spanX;
         ASSERT(j >= 0);
         ASSERT(j < width);
         zoomedVals16[i] = ((GLushort *) z)[j];
      }
      z = zoomedVals16;
   }
   else {
      ASSERT(rb->DataType == GL_UNSIGNED_INT);
      for (i = 0; i < zoomedWidth; i++) {
         GLint j = unzoom_x(ctx->Pixel.ZoomX, imgX, x0 + i) - spanX;
         ASSERT(j >= 0);
         ASSERT(j < width);
         zoomedVals32[i] = ((GLuint *) z)[j];
      }
      z = zoomedVals32;
   }

   /* write the zoomed spans */
   for (y = y0; y < y1; y++) {
      rb->PutRow(ctx, rb, zoomedWidth, x0, y, z, NULL);
   }
}
