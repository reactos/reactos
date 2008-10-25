/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_pixel.c,v 1.2 2002/12/16 16:18:54 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "enums.h"
#include "mtypes.h"
#include "macros.h"
#include "swrast/swrast.h"

#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_pixel.h"
#include "r200_swtcl.h"

#include "drirenderbuffer.h"


static GLboolean
check_color( const GLcontext *ctx, GLenum type, GLenum format,
	     const struct gl_pixelstore_attrib *packing,
	     const void *pixels, GLint sz, GLint pitch )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint cpp = rmesa->r200Screen->cpp;

   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (	(pitch & 63) ||
	ctx->_ImageTransferState ||
	packing->SwapBytes ||
	packing->LsbFirst) {
      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: failed 1\n", __FUNCTION__);
      return GL_FALSE;
   }

   if ( type == GL_UNSIGNED_INT_8_8_8_8_REV && 
	cpp == 4 && 
	format == GL_BGRA ) {
      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: passed 2\n", __FUNCTION__);
      return GL_TRUE;
   }

   if (R200_DEBUG & DEBUG_PIXEL)
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



static GLboolean
clip_pixelrect( const GLcontext *ctx,
		const GLframebuffer *buffer,
		GLint *x, GLint *y,
		GLsizei *width, GLsizei *height,
		GLint *size )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

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

   *size = ((*y + *height - 1) * rmesa->r200Screen->frontPitch +
	    (*x + *width - 1) * rmesa->r200Screen->cpp);

   return GL_TRUE;
}

static GLboolean
r200TryReadPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *pack,
		  GLvoid *pixels )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint pitch = pack->RowLength ? pack->RowLength : width;
   GLint blit_format;
   GLuint cpp = rmesa->r200Screen->cpp;
   GLint size = width * height * cpp;

   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* Only accelerate reading to GART buffers.
    */
   if ( !r200IsGartMemory(rmesa, pixels, 
			 pitch * height * rmesa->r200Screen->cpp ) ) {
      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: dest not GART\n", __FUNCTION__);
      return GL_FALSE;
   }

   /* Need GL_PACK_INVERT_MESA to cope with upsidedown results from
    * blitter:
    */
   if (!pack->Invert) {
      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s: MESA_PACK_INVERT not set\n", __FUNCTION__);
      return GL_FALSE;
   }

   if (!check_color(ctx, type, format, pack, pixels, size, pitch))
      return GL_FALSE;

   switch ( rmesa->r200Screen->cpp ) {
   case 4:
      blit_format = R200_CP_COLOR_FORMAT_ARGB8888;
      break;
   default:
      return GL_FALSE;
   }


   /* Although the blits go on the command buffer, need to do this and
    * fire with lock held to guarentee cliprects and drawOffset are
    * correct.
    *
    * This is an unusual situation however, as the code which flushes
    * a full command buffer expects to be called unlocked.  As a
    * workaround, immediately flush the buffer on aquiring the lock.
    */
   LOCK_HARDWARE( rmesa );

   if (rmesa->store.cmd_used)
      r200FlushCmdBufLocked( rmesa, __FUNCTION__ );

   if (!clip_pixelrect(ctx, ctx->ReadBuffer, &x, &y, &width, &height,
		       &size)) {
      UNLOCK_HARDWARE( rmesa );
      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "%s totally clipped -- nothing to do\n",
		 __FUNCTION__);
      return GL_TRUE;
   }

   {
      __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
      driRenderbuffer *drb = (driRenderbuffer *) ctx->ReadBuffer->_ColorReadBuffer;
      int nbox = dPriv->numClipRects;
      int src_offset = drb->offset
		     + rmesa->r200Screen->fbLocation;
      int src_pitch = drb->pitch * drb->cpp;
      int dst_offset = r200GartOffsetFromVirtual( rmesa, pixels );
      int dst_pitch = pitch * rmesa->r200Screen->cpp;
      drm_clip_rect_t *box = dPriv->pClipRects;
      int i;

      r200EmitWait( rmesa, RADEON_WAIT_3D ); 

      y = dPriv->h - y - height;
      x += dPriv->x;
      y += dPriv->y;


      if (R200_DEBUG & DEBUG_PIXEL)
	 fprintf(stderr, "readpixel blit src_pitch %d dst_pitch %d\n",
		 src_pitch, dst_pitch);

      for (i = 0 ; i < nbox ; i++)
      {
	 GLint bx = box[i].x1;
	 GLint by = box[i].y1;
	 GLint bw = box[i].x2 - bx;
	 GLint bh = box[i].y2 - by;
	 
	 if (bx < x) bw -= x - bx, bx = x;
	 if (by < y) bh -= y - by, by = y;
	 if (bx + bw > x + width) bw = x + width - bx;
	 if (by + bh > y + height) bh = y + height - by;
	 if (bw <= 0) continue;
	 if (bh <= 0) continue;

	 r200EmitBlit( rmesa,
		       blit_format,
		       src_pitch, src_offset,
		       dst_pitch, dst_offset,
		       bx, by,
		       bx - x, by - y,
		       bw, bh );
      }

      r200FlushCmdBufLocked( rmesa, __FUNCTION__ );
   }
   UNLOCK_HARDWARE( rmesa );

   r200Finish( ctx ); /* required by GL */

   return GL_TRUE;
}

static void
r200ReadPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *pack,
		 GLvoid *pixels )
{
   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!r200TryReadPixels( ctx, x, y, width, height, format, type, pack, 
			   pixels))
      _swrast_ReadPixels( ctx, x, y, width, height, format, type, pack, 
			  pixels);
}




static void do_draw_pix( GLcontext *ctx,
			 GLint x, GLint y, GLsizei width, GLsizei height,
			 GLint pitch,
			 const void *pixels,
			 GLuint planemask)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   drm_clip_rect_t *box = dPriv->pClipRects;
   struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorDrawBuffers[0][0];
   driRenderbuffer *drb = (driRenderbuffer *) rb;
   int nbox = dPriv->numClipRects;
   int i;
   int blit_format;
   int size;
   int src_offset = r200GartOffsetFromVirtual( rmesa, pixels );
   int src_pitch = pitch * rmesa->r200Screen->cpp;

   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   switch ( rmesa->r200Screen->cpp ) {
   case 2:
      blit_format = R200_CP_COLOR_FORMAT_RGB565;
      break;
   case 4:
      blit_format = R200_CP_COLOR_FORMAT_ARGB8888;
      break;
   default:
      return;
   }


   LOCK_HARDWARE( rmesa );

   if (rmesa->store.cmd_used)
      r200FlushCmdBufLocked( rmesa, __FUNCTION__ );

   y -= height;			/* cope with pixel zoom */
   
   if (!clip_pixelrect(ctx, ctx->DrawBuffer,
		       &x, &y, &width, &height,
		       &size)) {
      UNLOCK_HARDWARE( rmesa );
      return;
   }

   y = dPriv->h - y - height; 	/* convert from gl to hardware coords */
   x += dPriv->x;
   y += dPriv->y;


   r200EmitWait( rmesa, RADEON_WAIT_3D );

   for (i = 0 ; i < nbox ; i++ )
   {
      GLint bx = box[i].x1;
      GLint by = box[i].y1;
      GLint bw = box[i].x2 - bx;
      GLint bh = box[i].y2 - by;

      if (bx < x) bw -= x - bx, bx = x;
      if (by < y) bh -= y - by, by = y;
      if (bx + bw > x + width) bw = x + width - bx;
      if (by + bh > y + height) bh = y + height - by;
      if (bw <= 0) continue;
      if (bh <= 0) continue;

      r200EmitBlit( rmesa,
		    blit_format,
		    src_pitch, src_offset,
		    drb->pitch * drb->cpp,
		    drb->offset + rmesa->r200Screen->fbLocation,
		    bx - x, by - y,
		    bx, by,
		    bw, bh );
   }

   r200FlushCmdBufLocked( rmesa, __FUNCTION__ );
   r200WaitForIdleLocked( rmesa ); /* required by GL */
   UNLOCK_HARDWARE( rmesa );
}




static GLboolean
r200TryDrawPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLvoid *pixels )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLint pitch = unpack->RowLength ? unpack->RowLength : width;
   GLuint planemask;
   GLuint cpp = rmesa->r200Screen->cpp;
   GLint size = width * pitch * cpp;

   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* check that we're drawing to exactly one color buffer */
   if (ctx->DrawBuffer->_NumColorDrawBuffers[0] != 1)
     return GL_FALSE;

   switch (format) {
   case GL_RGB:
   case GL_RGBA:
   case GL_BGRA:
      planemask = r200PackColor(cpp,
				ctx->Color.ColorMask[RCOMP],
				ctx->Color.ColorMask[GCOMP],
				ctx->Color.ColorMask[BCOMP],
				ctx->Color.ColorMask[ACOMP]);

      if (cpp == 2)
	 planemask |= planemask << 16;

      if (planemask != ~0)
	 return GL_FALSE;	/* fix me -- should be possible */

      /* Can't do conversions on GART reads/draws. 
       */
      if ( !r200IsGartMemory( rmesa, pixels, size ) ) {
	 if (R200_DEBUG & DEBUG_PIXEL)
	    fprintf(stderr, "%s: not GART memory\n", __FUNCTION__);
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

   if ( r200IsGartMemory(rmesa, pixels, size) )
   {
      do_draw_pix( ctx, x, y, width, height, pitch, pixels, planemask );
      return GL_TRUE;
   }
   else if (0)
   {
      /* Pixels is in regular memory -- get dma buffers and perform
       * upload through them.
       */
   }
   else
      return GL_FALSE;
}

static void
r200DrawPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *unpack,
		 const GLvoid *pixels )
{
   if (R200_DEBUG & DEBUG_PIXEL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   if (!r200TryDrawPixels( ctx, x, y, width, height, format, type,
			  unpack, pixels ))
      _swrast_DrawPixels( ctx, x, y, width, height, format, type,
			  unpack, pixels );
}


static void
r200Bitmap( GLcontext *ctx, GLint px, GLint py,
		  GLsizei width, GLsizei height,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLubyte *bitmap )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   if (rmesa->Fallback)
      _swrast_Bitmap( ctx, px, py, width, height, unpack, bitmap );
   else
      r200PointsBitmap( ctx, px, py, width, height, unpack, bitmap );
}



void r200InitPixelFuncs( GLcontext *ctx )
{
   if (!getenv("R200_NO_BLITS")) {
      ctx->Driver.ReadPixels = r200ReadPixels;  
      ctx->Driver.DrawPixels = r200DrawPixels; 
      if (getenv("R200_HW_BITMAP")) 
	 ctx->Driver.Bitmap = r200Bitmap;
   }
}
