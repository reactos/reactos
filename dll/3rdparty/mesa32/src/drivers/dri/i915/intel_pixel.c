/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"



static GLboolean
check_color( const GLcontext *ctx, GLenum type, GLenum format,
	     const struct gl_pixelstore_attrib *packing,
	     const void *pixels, GLint sz, GLint pitch )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   GLuint cpp = intel->intelScreen->cpp;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (	(pitch & 63) ||
	ctx->_ImageTransferState ||
	packing->SwapBytes ||
	packing->LsbFirst) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: failed 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   if ( type == GL_UNSIGNED_INT_8_8_8_8_REV && 
	cpp == 4 && 
	format == GL_BGRA ) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: passed 2\n", __FUNCTION__);
      return GL_TRUE;
   }

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s: failed\n", __FUNCTION__);

   return GL_FALSE;
}

static GLboolean
check_color_per_fragment_ops( const GLcontext *ctx )
{
   int result;
   result = (!(     ctx->Color.AlphaEnabled || 
		    ctx->Depth.Test ||
		    ctx->Fog.Enabled ||
		    ctx->Scissor.Enabled ||
		    ctx->Stencil.Enabled ||
		    !ctx->Color.ColorMask[0] ||
		    !ctx->Color.ColorMask[1] ||
		    !ctx->Color.ColorMask[2] ||
		    !ctx->Color.ColorMask[3] ||
		    ctx->Color.ColorLogicOpEnabled ||
		    ctx->Texture._EnabledUnits
           ) &&
	   ctx->Current.RasterPosValid);
   
   return result;
}


/**
 * Clip the given rectangle against the buffer's bounds (including scissor).
 * \param size returns the 
 * \return GL_TRUE if any pixels remain, GL_FALSE if totally clipped.
 *
 * XXX Replace this with _mesa_clip_drawpixels() and _mesa_clip_readpixels()
 * from Mesa 6.4.  We shouldn't apply scissor for ReadPixels.
 */
static GLboolean
clip_pixelrect( const GLcontext *ctx,
		const GLframebuffer *buffer,
		GLint *x, GLint *y,
		GLsizei *width, GLsizei *height)
{
   /* left clipping */
   if (*x < buffer->_Xmin) {
      *width -= (buffer->_Xmin - *x);
      *x = buffer->_Xmin;
   }

   /* right clipping */
   if (*x + *width > buffer->_Xmax)
      *width -= (*x + *width - buffer->_Xmax - 1);

   if (*width <= 0)
      return GL_FALSE;

   /* bottom clipping */
   if (*y < buffer->_Ymin) {
      *height -= (buffer->_Ymin - *y);
      *y = buffer->_Ymin;
   }

   /* top clipping */
   if (*y + *height > buffer->_Ymax)
      *height -= (*y + *height - buffer->_Ymax - 1);

   if (*height <= 0)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Compute intersection of a clipping rectangle and pixel rectangle,
 * returning results in x/y/w/hOut vars.
 * \return GL_TRUE if there's intersection, GL_FALSE if disjoint.
 */
static INLINE GLboolean
intersect_region(const drm_clip_rect_t *box,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLint *xOut, GLint *yOut, GLint *wOut, GLint *hOut)
{
   GLint bx = box->x1;
   GLint by = box->y1;
   GLint bw = box->x2 - bx;
   GLint bh = box->y2 - by;

   if (bx < x) bw -= x - bx, bx = x;
   if (by < y) bh -= y - by, by = y;
   if (bx + bw > x + width) bw = x + width - bx;
   if (by + bh > y + height) bh = y + height - by;

   *xOut = bx;
   *yOut = by;
   *wOut = bw;
   *hOut = bh;

   if (bw <= 0) return GL_FALSE;
   if (bh <= 0) return GL_FALSE;

   return GL_TRUE;
}



static GLboolean
intelTryReadPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *pack,
		  GLvoid *pixels )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   GLint size = 0; /* not really used */
   GLint pitch = pack->RowLength ? pack->RowLength : width;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* Only accelerate reading to agp buffers.
    */
   if ( !intelIsAgpMemory(intel, pixels, 
			pitch * height * intel->intelScreen->cpp ) ) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: dest not agp\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* Need GL_PACK_INVERT_MESA to cope with upsidedown results from
    * blitter:
    */
   if (!pack->Invert) {
      if (INTEL_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: MESA_PACK_INVERT not set\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (!check_color(ctx, type, format, pack, pixels, size, pitch))
      return GL_FALSE;

   switch ( intel->intelScreen->cpp ) {
   case 4:
      break;
   default:
      return GL_FALSE;
   }


   /* Although the blits go on the command buffer, need to do this and
    * fire with lock held to guarentee cliprects and drawing offset are
    * correct.
    *
    * This is an unusual situation however, as the code which flushes
    * a full command buffer expects to be called unlocked.  As a
    * workaround, immediately flush the buffer on aquiring the lock.
    */
   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );
   {
      __DRIdrawablePrivate *dPriv = intel->driDrawable;
      int nbox = dPriv->numClipRects;
      int src_offset = intel->readRegion->offset;
      int src_pitch = intel->intelScreen->front.pitch;
      int dst_offset = intelAgpOffsetFromVirtual( intel, pixels);
      drm_clip_rect_t *box = dPriv->pClipRects;
      int i;

      assert(dst_offset != ~0);  /* should have been caught above */

      if (!clip_pixelrect(ctx, ctx->ReadBuffer, &x, &y, &width, &height)) {
	 UNLOCK_HARDWARE( intel );
	 if (INTEL_DEBUG & DEBUG_PIXEL)
	    fprintf(stderr, "%s totally clipped -- nothing to do\n",
		    __FUNCTION__);
	 return GL_TRUE;
      }

      /* convert to screen coords (y=0=top) */
      y = dPriv->h - y - height;
      x += dPriv->x;
      y += dPriv->y;

      if (INTEL_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "readpixel blit src_pitch %d dst_pitch %d\n",
		 src_pitch, pitch);

      /* We don't really have to do window clipping for readpixels.
       * The OpenGL spec says that pixels read from outside the
       * visible window region (pixel ownership) have undefined value.
       */
      for (i = 0 ; i < nbox ; i++)
      {
         GLint bx, by, bw, bh;
         if (intersect_region(box+i, x, y, width, height,
                              &bx, &by, &bw, &bh)) {
            intelEmitCopyBlitLocked( intel,
                                     intel->intelScreen->cpp,
                                     src_pitch, src_offset,
                                     pitch, dst_offset,
                                     bx, by,
                                     bx - x, by - y,
                                     bw, bh );
         }
      }
   }
   UNLOCK_HARDWARE( intel );
   intelFinish( &intel->ctx );

   return GL_TRUE;
}

static void
intelReadPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *pack,
		 GLvoid *pixels )
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!intelTryReadPixels( ctx, x, y, width, height, format, type, pack, 
                            pixels))
      _swrast_ReadPixels( ctx, x, y, width, height, format, type, pack, 
			  pixels);
}




static void do_draw_pix( GLcontext *ctx,
			 GLint x, GLint y, GLsizei width, GLsizei height,
			 GLint pitch,
			 const void *pixels,
			 GLuint dest )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   drm_clip_rect_t *box = dPriv->pClipRects;
   int nbox = dPriv->numClipRects;
   int i;
   int src_offset = intelAgpOffsetFromVirtual( intel, pixels);
   int src_pitch = pitch;

   assert(src_offset != ~0);  /* should be caught earlier */

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );
   if (ctx->DrawBuffer)
   {
      y -= height;			/* cope with pixel zoom */
   
      if (!clip_pixelrect(ctx, ctx->DrawBuffer,
			  &x, &y, &width, &height)) {
	 UNLOCK_HARDWARE( intel );
	 return;
      }

      y = dPriv->h - y - height; 	/* convert from gl to hardware coords */
      x += dPriv->x;
      y += dPriv->y;

      for (i = 0 ; i < nbox ; i++ )
      {
	 GLint bx, by, bw, bh;
	 if (intersect_region(box + i, x, y, width, height,
			      &bx, &by, &bw, &bh)) {
            intelEmitCopyBlitLocked( intel,
                                     intel->intelScreen->cpp,
                                     src_pitch, src_offset,
                                     intel->intelScreen->front.pitch,
                                     intel->drawRegion->offset,
                                     bx - x, by - y,
                                     bx, by,
                                     bw, bh );
         }
      }
   }
   UNLOCK_HARDWARE( intel );
   intelFinish( &intel->ctx );
}



static GLboolean
intelTryDrawPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLvoid *pixels )
{
   intelContextPtr intel = INTEL_CONTEXT(ctx);
   GLint pitch = unpack->RowLength ? unpack->RowLength : width;
   GLuint dest;
   GLuint cpp = intel->intelScreen->cpp;
   GLint size = width * pitch * cpp;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   switch (format) {
   case GL_RGB:
   case GL_RGBA:
   case GL_BGRA:
      dest = intel->drawRegion->offset;

      /* Planemask doesn't have full support in blits.
       */
      if (!ctx->Color.ColorMask[RCOMP] ||
	  !ctx->Color.ColorMask[GCOMP] ||
	  !ctx->Color.ColorMask[BCOMP] ||
	  !ctx->Color.ColorMask[ACOMP]) {
	 if (INTEL_DEBUG & DEBUG_PIXEL)
	    fprintf(stderr, "%s: planemask\n", __FUNCTION__);
	 return GL_FALSE;	
      }

      /* Can't do conversions on agp reads/draws. 
       */
      if ( !intelIsAgpMemory( intel, pixels, size ) ) {
	 if (INTEL_DEBUG & DEBUG_PIXEL)
	    fprintf(stderr, "%s: not agp memory\n", __FUNCTION__);
	 return GL_FALSE;
      }

      if (!check_color(ctx, type, format, unpack, pixels, size, pitch)) {
	 return GL_FALSE;
      }
      if (!check_color_per_fragment_ops(ctx)) {
	 return GL_FALSE;
      }

      if (ctx->Pixel.ZoomX != 1.0F ||
	  ctx->Pixel.ZoomY != -1.0F)
	 return GL_FALSE;
      break;

   default:
      return GL_FALSE;
   }

   if ( intelIsAgpMemory(intel, pixels, size) )
   {
      do_draw_pix( ctx, x, y, width, height, pitch, pixels, dest );
      return GL_TRUE;
   }
   else if (0)
   {
      /* Pixels is in regular memory -- get dma buffers and perform
       * upload through them.  No point doing this for regular uploads
       * but once we remove some of the restrictions above (colormask,
       * pixelformat conversion, zoom?, etc), this could be a win.
       */
   }
   else
      return GL_FALSE;

   return GL_FALSE;
}

static void
intelDrawPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *unpack,
		 const GLvoid *pixels )
{
   if (INTEL_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!intelTryDrawPixels( ctx, x, y, width, height, format, type,
                            unpack, pixels ))
      _swrast_DrawPixels( ctx, x, y, width, height, format, type,
			  unpack, pixels );
}




/**
 * Implement glCopyPixels for the front color buffer (or back buffer Pixmap)
 * for the color buffer.  Don't support zooming, pixel transfer, etc.
 * We do support copying from one window to another, ala glXMakeCurrentRead.
 */
static void
intelCopyPixels( GLcontext *ctx,
		 GLint srcx, GLint srcy, GLsizei width, GLsizei height,
		 GLint destx, GLint desty, GLenum type )
{
#if 0
   const XMesaContext xmesa = XMESA_CONTEXT(ctx);
   const SWcontext *swrast = SWRAST_CONTEXT( ctx );
   XMesaDisplay *dpy = xmesa->xm_visual->display;
   const XMesaDrawable drawBuffer = xmesa->xm_draw_buffer->buffer;
   const XMesaDrawable readBuffer = xmesa->xm_read_buffer->buffer;
   const XMesaGC gc = xmesa->xm_draw_buffer->gc;

   ASSERT(dpy);
   ASSERT(gc);

   if (drawBuffer &&  /* buffer != 0 means it's a Window or Pixmap */
       readBuffer &&
       type == GL_COLOR &&
       (swrast->_RasterMask & ~CLIP_BIT) == 0 && /* no blend, z-test, etc */
       ctx->_ImageTransferState == 0 &&  /* no color tables, scale/bias, etc */
       ctx->Pixel.ZoomX == 1.0 &&        /* no zooming */
       ctx->Pixel.ZoomY == 1.0) {
      /* Note: we don't do any special clipping work here.  We could,
       * but X will do it for us.
       */
      srcy = FLIP(xmesa->xm_read_buffer, srcy) - height + 1;
      desty = FLIP(xmesa->xm_draw_buffer, desty) - height + 1;
      XCopyArea(dpy, readBuffer, drawBuffer, gc,
                srcx, srcy, width, height, destx, desty);
   }
#else
   _swrast_CopyPixels(ctx, srcx, srcy, width, height, destx, desty, type );
#endif
}




void intelInitPixelFuncs( struct dd_function_table *functions )
{
   functions->CopyPixels = intelCopyPixels;
   if (!getenv("INTEL_NO_BLITS")) {
      functions->ReadPixels = intelReadPixels;  
      functions->DrawPixels = intelDrawPixels; 
   }
}
