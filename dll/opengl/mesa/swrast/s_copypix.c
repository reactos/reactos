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

#include <precomp.h>

/**
 * Determine if there's overlap in an image copy.
 * This test also compensates for the fact that copies are done from
 * bottom to top and overlaps can sometimes be handled correctly
 * without making a temporary image copy.
 * \return GL_TRUE if the regions overlap, GL_FALSE otherwise.
 */
static GLboolean
regions_overlap(GLint srcx, GLint srcy,
                GLint dstx, GLint dsty,
                GLint width, GLint height,
                GLfloat zoomX, GLfloat zoomY)
{
   if (zoomX == 1.0 && zoomY == 1.0) {
      /* no zoom */
      if (srcx >= dstx + width || (srcx + width <= dstx)) {
         return GL_FALSE;
      }
      else if (srcy < dsty) { /* this is OK */
         return GL_FALSE;
      }
      else if (srcy > dsty + height) {
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
   else {
      /* add one pixel of slop when zooming, just to be safe */
      if (srcx > (dstx + ((zoomX > 0.0F) ? (width * zoomX + 1.0F) : 0.0F))) {
         /* src is completely right of dest */
         return GL_FALSE;
      }
      else if (srcx + width + 1.0F < dstx + ((zoomX > 0.0F) ? 0.0F : (width * zoomX))) {
         /* src is completely left of dest */
         return GL_FALSE;
      }
      else if ((srcy < dsty) && (srcy + height < dsty + (height * zoomY))) {
         /* src is completely below dest */
         return GL_FALSE;
      }
      else if ((srcy > dsty) && (srcy + height > dsty + (height * zoomY))) {
         /* src is completely above dest */
         return GL_FALSE;
      }
      else {
         return GL_TRUE;
      }
   }
}


/**
 * RGBA copypixels
 */
static void
copy_rgba_pixels(struct gl_context *ctx, GLint srcx, GLint srcy,
                 GLint width, GLint height, GLint destx, GLint desty)
{
   GLfloat *tmpImage, *p;
   GLint sy, dy, stepy, row;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   GLuint transferOps = ctx->_ImageTransferState;
   SWspan span;

   if (!ctx->ReadBuffer->_ColorReadBuffer) {
      /* no readbuffer - OK */
      return;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be done bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   INIT_SPAN(span, GL_BITMAP);
   _swrast_span_default_attribs(ctx, &span);
   span.arrayMask = SPAN_RGBA;
   span.arrayAttribs = FRAG_BIT_COL; /* we'll fill in COL0 attrib values */

   if (overlapping) {
      tmpImage = (GLfloat *) malloc(width * height * sizeof(GLfloat) * 4);
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      /* read the source image as RGBA/float */
      p = tmpImage;
      for (row = 0; row < height; row++) {
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy + row, p );
         p += width * 4;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warnings */
      p = NULL;
   }

   ASSERT(width < MAX_WIDTH);

   for (row = 0; row < height; row++, sy += stepy, dy += stepy) {
      GLvoid *rgba = span.array->attribs[FRAG_ATTRIB_COL];

      /* Get row/span of source pixels */
      if (overlapping) {
         /* get from buffered image */
         memcpy(rgba, p, width * sizeof(GLfloat) * 4);
         p += width * 4;
      }
      else {
         /* get from framebuffer */
         _swrast_read_rgba_span( ctx, ctx->ReadBuffer->_ColorReadBuffer,
                                 width, srcx, sy, rgba );
      }

      if (transferOps) {
         _mesa_apply_rgba_transfer_ops(ctx, transferOps, width,
                                       (GLfloat (*)[4]) rgba);
      }

      /* Write color span */
      span.x = destx;
      span.y = dy;
      span.end = width;
      span.array->ChanType = GL_FLOAT;
      if (zoom) {
         _swrast_write_zoomed_rgba_span(ctx, destx, desty, &span, rgba);
      }
      else {
         _swrast_write_rgba_span(ctx, &span);
      }
   }

   span.array->ChanType = CHAN_TYPE; /* restore */

   if (overlapping)
      free(tmpImage);
}


/**
 * Convert floating point Z values to integer Z values with pixel transfer's
 * Z scale and bias.
 */
static void
scale_and_bias_z(struct gl_context *ctx, GLuint width,
                 const GLfloat depth[], GLuint z[])
{
   const GLuint depthMax = ctx->DrawBuffer->_DepthMax;
   GLuint i;

   if (depthMax <= 0xffffff &&
       ctx->Pixel.DepthScale == 1.0 &&
       ctx->Pixel.DepthBias == 0.0) {
      /* no scale or bias and no clamping and no worry of overflow */
      const GLfloat depthMaxF = ctx->DrawBuffer->_DepthMaxF;
      for (i = 0; i < width; i++) {
         z[i] = (GLuint) (depth[i] * depthMaxF);
      }
   }
   else {
      /* need to be careful with overflow */
      const GLdouble depthMaxF = ctx->DrawBuffer->_DepthMaxF;
      for (i = 0; i < width; i++) {
         GLdouble d = depth[i] * ctx->Pixel.DepthScale + ctx->Pixel.DepthBias;
         d = CLAMP(d, 0.0, 1.0) * depthMaxF;
         if (d >= depthMaxF)
            z[i] = depthMax;
         else
            z[i] = (GLuint) d;
      }
   }
}



/*
 * TODO: Optimize!!!!
 */
static void
copy_depth_pixels( struct gl_context *ctx, GLint srcx, GLint srcy,
                   GLint width, GLint height,
                   GLint destx, GLint desty )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *readRb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   GLfloat *p, *tmpImage;
   GLint sy, dy, stepy;
   GLint j;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;
   SWspan span;

   if (!readRb) {
      /* no readbuffer - OK */
      return;
   }

   INIT_SPAN(span, GL_BITMAP);
   _swrast_span_default_attribs(ctx, &span);
   span.arrayMask = SPAN_Z;

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLfloat *) malloc(width * height * sizeof(GLfloat));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, ssy, p);
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLfloat depth[MAX_WIDTH];
      /* get depth values */
      if (overlapping) {
         memcpy(depth, p, width * sizeof(GLfloat));
         p += width;
      }
      else {
         _swrast_read_depth_span_float(ctx, readRb, width, srcx, sy, depth);
      }

      /* apply scale and bias */
      scale_and_bias_z(ctx, width, depth, span.array->z);

      /* write depth values */
      span.x = destx;
      span.y = dy;
      span.end = width;
      if (zoom)
         _swrast_write_zoomed_depth_span(ctx, destx, desty, &span);
      else
         _swrast_write_rgba_span(ctx, &span);
   }

   if (overlapping)
      free(tmpImage);
}



static void
copy_stencil_pixels( struct gl_context *ctx, GLint srcx, GLint srcy,
                     GLint width, GLint height,
                     GLint destx, GLint desty )
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLint sy, dy, stepy;
   GLint j;
   GLubyte *p, *tmpImage;
   const GLboolean zoom = ctx->Pixel.ZoomX != 1.0F || ctx->Pixel.ZoomY != 1.0F;
   GLint overlapping;

   if (!rb) {
      /* no readbuffer - OK */
      return;
   }

   if (ctx->DrawBuffer == ctx->ReadBuffer) {
      overlapping = regions_overlap(srcx, srcy, destx, desty, width, height,
                                    ctx->Pixel.ZoomX, ctx->Pixel.ZoomY);
   }
   else {
      overlapping = GL_FALSE;
   }

   /* Determine if copy should be bottom-to-top or top-to-bottom */
   if (!overlapping && srcy < desty) {
      /* top-down  max-to-min */
      sy = srcy + height - 1;
      dy = desty + height - 1;
      stepy = -1;
   }
   else {
      /* bottom-up  min-to-max */
      sy = srcy;
      dy = desty;
      stepy = 1;
   }

   if (overlapping) {
      GLint ssy = sy;
      tmpImage = (GLubyte *) malloc(width * height * sizeof(GLubyte));
      if (!tmpImage) {
         _mesa_error( ctx, GL_OUT_OF_MEMORY, "glCopyPixels" );
         return;
      }
      p = tmpImage;
      for (j = 0; j < height; j++, ssy += stepy) {
         _swrast_read_stencil_span( ctx, rb, width, srcx, ssy, p );
         p += width;
      }
      p = tmpImage;
   }
   else {
      tmpImage = NULL;  /* silence compiler warning */
      p = NULL;
   }

   for (j = 0; j < height; j++, sy += stepy, dy += stepy) {
      GLubyte stencil[MAX_WIDTH];

      /* Get stencil values */
      if (overlapping) {
         memcpy(stencil, p, width * sizeof(GLubyte));
         p += width;
      }
      else {
         _swrast_read_stencil_span( ctx, rb, width, srcx, sy, stencil );
      }

      _mesa_apply_stencil_transfer_ops(ctx, width, stencil);

      /* Write stencil values */
      if (zoom) {
         _swrast_write_zoomed_stencil_span(ctx, destx, desty, width,
                                           destx, dy, stencil);
      }
      else {
         _swrast_write_stencil_span( ctx, width, destx, dy, stencil );
      }
   }

   if (overlapping)
      free(tmpImage);
}


/**
 * Try to do a fast 1:1 blit with memcpy.
 * \return GL_TRUE if successful, GL_FALSE otherwise.
 */
GLboolean
swrast_fast_copy_pixels(struct gl_context *ctx,
			GLint srcX, GLint srcY, GLsizei width, GLsizei height,
			GLint dstX, GLint dstY, GLenum type)
{
   struct gl_framebuffer *srcFb = ctx->ReadBuffer;
   struct gl_framebuffer *dstFb = ctx->DrawBuffer;
   struct gl_renderbuffer *srcRb, *dstRb;
   GLint row;
   GLuint pixelBytes, widthInBytes;
   GLubyte *srcMap, *dstMap;
   GLint srcRowStride, dstRowStride;

   if (type == GL_COLOR) {
      srcRb = srcFb->_ColorReadBuffer;
      dstRb = dstFb->_ColorDrawBuffer;
   }
   else if (type == GL_STENCIL) {
      srcRb = srcFb->Attachment[BUFFER_STENCIL].Renderbuffer;
      dstRb = dstFb->Attachment[BUFFER_STENCIL].Renderbuffer;
   }
   else if (type == GL_DEPTH) {
      srcRb = srcFb->Attachment[BUFFER_DEPTH].Renderbuffer;
      dstRb = dstFb->Attachment[BUFFER_DEPTH].Renderbuffer;
   }

   /* src and dst renderbuffers must be same format */
   if (!srcRb || !dstRb || srcRb->Format != dstRb->Format) {
      return GL_FALSE;
   }

   /* clipping not supported */
   if (srcX < 0 || srcX + width > (GLint) srcFb->Width ||
       srcY < 0 || srcY + height > (GLint) srcFb->Height ||
       dstX < dstFb->_Xmin || dstX + width > dstFb->_Xmax ||
       dstY < dstFb->_Ymin || dstY + height > dstFb->_Ymax) {
      return GL_FALSE;
   }

   pixelBytes = _mesa_get_format_bytes(srcRb->Format);
   widthInBytes = width * pixelBytes;

   if (srcRb == dstRb) {
      /* map whole buffer for read/write */
      /* XXX we could be clever and just map the union region of the
       * source and dest rects.
       */
      GLubyte *map;
      GLint rowStride;

      ctx->Driver.MapRenderbuffer(ctx, srcRb, 0, 0,
                                  srcRb->Width, srcRb->Height,
                                  GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                                  &map, &rowStride);
      if (!map) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
         return GL_TRUE; /* don't retry with slow path */
      }

      srcMap = map + srcY * rowStride + srcX * pixelBytes;
      dstMap = map + dstY * rowStride + dstX * pixelBytes;

      /* this handles overlapping copies */
      if (srcY < dstY) {
         /* copy in reverse (top->down) order */
         srcMap += rowStride * (height - 1);
         dstMap += rowStride * (height - 1);
         srcRowStride = -rowStride;
         dstRowStride = -rowStride;
      }
      else {
         /* copy in normal (bottom->up) order */
         srcRowStride = rowStride;
         dstRowStride = rowStride;
      }
   }
   else {
      /* different src/dst buffers */
      ctx->Driver.MapRenderbuffer(ctx, srcRb, srcX, srcY,
                                  width, height,
                                  GL_MAP_READ_BIT, &srcMap, &srcRowStride);
      if (!srcMap) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
         return GL_TRUE; /* don't retry with slow path */
      }
      ctx->Driver.MapRenderbuffer(ctx, dstRb, dstX, dstY,
                                  width, height,
                                  GL_MAP_WRITE_BIT, &dstMap, &dstRowStride);
      if (!dstMap) {
         ctx->Driver.UnmapRenderbuffer(ctx, srcRb);
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyPixels");
         return GL_TRUE; /* don't retry with slow path */
      }
   }

   for (row = 0; row < height; row++) {
      /* memmove() in case of overlap */
      memmove(dstMap, srcMap, widthInBytes);
      dstMap += dstRowStride;
      srcMap += srcRowStride;
   }

   ctx->Driver.UnmapRenderbuffer(ctx, srcRb);
   if (dstRb != srcRb) {
      ctx->Driver.UnmapRenderbuffer(ctx, dstRb);
   }

   return GL_TRUE;
}


/**
 * Find/map the renderbuffer that we'll be reading from.
 * The swrast_render_start() function only maps the drawing buffers,
 * not the read buffer.
 */
static struct gl_renderbuffer *
map_readbuffer(struct gl_context *ctx, GLenum type)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct gl_renderbuffer *rb;
   struct swrast_renderbuffer *srb;

   switch (type) {
   case GL_COLOR:
      rb = fb->Attachment[fb->_ColorReadBufferIndex].Renderbuffer;
      break;
   case GL_DEPTH:
      rb = fb->Attachment[BUFFER_DEPTH].Renderbuffer;
      break;
   case GL_STENCIL:
      rb = fb->Attachment[BUFFER_STENCIL].Renderbuffer;
      break;
   default:
      return NULL;
   }

   srb = swrast_renderbuffer(rb);

   if (!srb || srb->Map) {
      /* no buffer, or buffer is mapped already, we're done */
      return NULL;
   }

   ctx->Driver.MapRenderbuffer(ctx, rb,
                               0, 0, rb->Width, rb->Height,
                               GL_MAP_READ_BIT,
                               &srb->Map, &srb->RowStride);

   return rb;
}


/**
 * Do software-based glCopyPixels.
 * By time we get here, all parameters will have been error-checked.
 */
void
_swrast_CopyPixels( struct gl_context *ctx,
		    GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		    GLint destx, GLint desty, GLenum type )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   struct gl_renderbuffer *rb;

   if (swrast->NewState)
      _swrast_validate_derived( ctx );

   if (!(SWRAST_CONTEXT(ctx)->_RasterMask != 0x0 ||
	 ctx->Pixel.ZoomX != 1.0F ||
	 ctx->Pixel.ZoomY != 1.0F ||
	 ctx->_ImageTransferState) &&
       swrast_fast_copy_pixels(ctx, srcx, srcy, width, height, destx, desty,
			       type)) {
      /* all done */
      return;
   }

   swrast_render_start(ctx);
   rb = map_readbuffer(ctx, type);

   switch (type) {
   case GL_COLOR:
      copy_rgba_pixels( ctx, srcx, srcy, width, height, destx, desty );
      break;
   case GL_DEPTH:
      copy_depth_pixels( ctx, srcx, srcy, width, height, destx, desty );
      break;
   case GL_STENCIL:
      copy_stencil_pixels( ctx, srcx, srcy, width, height, destx, desty );
      break;
   default:
      _mesa_problem(ctx, "unexpected type in _swrast_CopyPixels");
   }

   swrast_render_finish(ctx);

   if (rb) {
      struct swrast_renderbuffer *srb = swrast_renderbuffer(rb);
      ctx->Driver.UnmapRenderbuffer(ctx, rb);
      srb->Map = NULL;
   }
}
