/*
 * Copyright 2000 Compaq Computer Inc. and VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file mgapixel.c
 * Implement framebuffer pixel operations for MGA.
 *
 * \todo
 * Someday the accelerated \c glReadPixels and \c glDrawPixels paths need to
 * be resurrected.  They are currently ifdef'ed out because they don't seem
 * to work and they only get activated some very rare circumstances.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgapixel.c,v 1.9 2002/11/05 17:46:08 tsi Exp $ */

#include "mtypes.h"
#include "macros.h"
#include "mgadd.h"
#include "mgacontext.h"
#include "mgaioctl.h"
#include "mgapixel.h"
#include "mgastate.h"

#include "swrast/swrast.h"
#include "imports.h"

#if 0
#define IS_AGP_MEM( mmesa, p )						  \
   ((unsigned long)mmesa->mgaScreen->buffers.map <= ((unsigned long)p) && \
    (unsigned long)mmesa->mgaScreen->buffers.map +			  \
    (unsigned long)mmesa->mgaScreen->buffers.size > ((unsigned long)p))
#define AGP_OFFSET( mmesa, p )						  \
     (((unsigned long)p) - (unsigned long)mmesa->mgaScreen->buffers.map)


#if defined(MESA_packed_depth_stencil)
static GLboolean
check_depth_stencil_24_8( const GLcontext *ctx, GLenum type,
			  const struct gl_pixelstore_attrib *packing,
			  const void *pixels, GLint sz,
			  GLint pitch )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   return ( type == GL_UNSIGNED_INT_24_8_MESA &&
	    ctx->Visual->DepthBits == 24 &&
	    ctx->Visual->StencilBits == 8 &&
	    mmesa->mgaScreen->cpp == 4 &&
	    mmesa->hw_stencil &&
	    !ctx->Pixel.IndexShift &&
	    !ctx->Pixel.IndexOffset &&
	    !ctx->Pixel.MapStencilFlag &&
	    ctx->Pixel.DepthBias == 0.0 &&
	    ctx->Pixel.DepthScale == 1.0 &&
	    !packing->SwapBytes &&
	    pitch % 32 == 0 &&
	    pitch < 4096 );
}
#endif


static GLboolean
check_depth( const GLcontext *ctx, GLenum type,
	     const struct gl_pixelstore_attrib *packing,
	     const void *pixels, GLint sz, GLint pitch )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   if ( IS_AGP_MEM( mmesa, pixels ) &&
	!( ( type == GL_UNSIGNED_INT && mmesa->mgaScreen->cpp == 4 ) ||
	   ( type == GL_UNSIGNED_SHORT && mmesa->mgaScreen->cpp == 2 ) ) )
      return GL_FALSE;

   return ( ctx->Pixel.DepthBias == 0.0 &&
	    ctx->Pixel.DepthScale == 1.0 &&
	    !packing->SwapBytes &&
	    pitch % 32 == 0 &&
	    pitch < 4096 );
}


static GLboolean
check_color( const GLcontext *ctx, GLenum type, GLenum format,
	     const struct gl_pixelstore_attrib *packing,
	     const void *pixels, GLint sz, GLint pitch )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint cpp = mmesa->mgaScreen->cpp;

   /* Can't do conversions on agp reads/draws.
    */
   if ( IS_AGP_MEM( mmesa, pixels ) &&
	!( pitch % 32 == 0 && pitch < 4096 &&
	   ( ( type == GL_UNSIGNED_BYTE &&
	       cpp == 4 && format == GL_BGRA ) ||
	     ( type == GL_UNSIGNED_INT_8_8_8_8 &&
	       cpp == 4 && format == GL_BGRA ) ||
	     ( type == GL_UNSIGNED_SHORT_5_6_5_REV &&
	       cpp == 2 && format == GL_RGB ) ) ) )
      return GL_FALSE;

   return (!ctx->_ImageTransferState &&
	   !packing->SwapBytes &&
	   !packing->LsbFirst);
}

static GLboolean
check_color_per_fragment_ops( const GLcontext *ctx )
{
   return (!(       ctx->Color.AlphaEnabled ||
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
	   ctx->Current.RasterPosValid &&
	   ctx->Pixel.ZoomX == 1.0F &&
	   (ctx->Pixel.ZoomY == 1.0F || ctx->Pixel.ZoomY == -1.0F));
}

static GLboolean
check_depth_per_fragment_ops( const GLcontext *ctx )
{
   return ( ctx->Current.RasterPosValid &&
	    ctx->Color.ColorMask[RCOMP] == 0 &&
	    ctx->Color.ColorMask[BCOMP] == 0 &&
	    ctx->Color.ColorMask[GCOMP] == 0 &&
	    ctx->Color.ColorMask[ACOMP] == 0 &&
	    ctx->Pixel.ZoomX == 1.0F &&
	    ( ctx->Pixel.ZoomY == 1.0F || ctx->Pixel.ZoomY == -1.0F ) );
}

/* In addition to the requirements for depth:
 */
#if defined(MESA_packed_depth_stencil)
static GLboolean
check_stencil_per_fragment_ops( const GLcontext *ctx )
{
   return ( !ctx->Pixel.IndexShift &&
	    !ctx->Pixel.IndexOffset );
}
#endif


static GLboolean
clip_pixelrect( const GLcontext *ctx,
		const GLframebuffer *buffer,
		GLint *x, GLint *y,
		GLsizei *width, GLsizei *height,
		GLint *skipPixels, GLint *skipRows,
		GLint *size )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);

   *width = MIN2(*width, MAX_WIDTH); /* redundant? */

   /* left clipping */
   if (*x < buffer->_Xmin) {
      *skipPixels += (buffer->_Xmin - *x);
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
      *skipRows += (buffer->_Ymin - *y);
      *height -= (buffer->_Ymin - *y);
      *y = buffer->_Ymin;
   }

   /* top clipping */
   if (*y + *height > buffer->_Ymax)
      *height -= (*y + *height - buffer->_Ymax - 1);

   if (*height <= 0)
      return GL_FALSE;

   *size = ((*y + *height - 1) * mmesa->mgaScreen->frontPitch +
	    (*x + *width - 1) * mmesa->mgaScreen->cpp);

   return GL_TRUE;
}

static GLboolean
mgaTryReadPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *pack,
		  GLvoid *pixels )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLint size, skipPixels, skipRows;
   GLint pitch = pack->RowLength ? pack->RowLength : width;
   GLboolean ok;

   GLuint planemask;
   GLuint source;
#if 0
   drmMGABlit blit;
   GLuint dest;
   GLint source_pitch, dest_pitch;
   GLint delta_sx, delta_sy;
   GLint delta_dx, delta_dy;
   GLint blit_height, ydir;
#endif

   if (!clip_pixelrect(ctx, ctx->ReadBuffer,
		       &x, &y, &width, &height,
		       &skipPixels, &skipRows, &size)) {
      return GL_TRUE;
   }

   /* Only accelerate reading to agp buffers.
    */
   if ( !IS_AGP_MEM(mmesa, (char *)pixels) ||
	!IS_AGP_MEM(mmesa, (char *)pixels + size) )
      return GL_FALSE;

   switch (format) {
#if defined(MESA_packed_depth_stencil)
   case GL_DEPTH_STENCIL_MESA:
      ok = check_depth_stencil_24_8(ctx, type, pack, pixels, size, pitch);
      planemask = ~0;
      source = mmesa->mgaScreen->depthOffset;
      break;
#endif

   case GL_DEPTH_COMPONENT:
      ok = check_depth(ctx, type, pack, pixels, size, pitch);

      /* Can't accelerate at this depth -- planemask does the wrong
       * thing; it doesn't clear the low order bits in the
       * destination, instead it leaves them untouched.
       *
       * Could get the acclerator to solid fill the destination with
       * zeros first...  Or get the cpu to do it...
       */
      if (ctx->Visual.depthBits == 24)
	 return GL_FALSE;

      planemask = ~0;
      source = mmesa->mgaScreen->depthOffset;
      break;

   case GL_RGB:
   case GL_BGRA:
      ok = check_color(ctx, type, format, pack, pixels, size, pitch);
      planemask = ~0;
      source = (mmesa->draw_buffer == MGA_FRONT ?
		mmesa->mgaScreen->frontOffset :
		mmesa->mgaScreen->backOffset);
      break;

   default:
      return GL_FALSE;
   }

   if (!ok) {
      return GL_FALSE;
   }


   LOCK_HARDWARE( mmesa );

#if 0
   {
      __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
      int nbox, retcode, i;

      UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );

      if (mmesa->dirty_cliprects & MGA_FRONT)
	 mgaUpdateRects( mmesa, MGA_FRONT );

      nbox = dPriv->numClipRects;

      y = dPriv->h - y - height;
      x += mmesa->drawX;
      y += mmesa->drawY;

      dest = ((mmesa->mgaScreen->agp.handle + AGP_OFFSET(mmesa, pixels)) |
	      DO_dstmap_sys | DO_dstacc_agp);
      source_pitch = mmesa->mgaScreen->frontPitch / mmesa->mgaScreen->cpp;
      dest_pitch = pitch;
      delta_sx = 0;
      delta_sy = 0;
      delta_dx = -x;
      delta_dy = -y;
      blit_height = 2*y + height;
      ydir = -1;

      if (0) fprintf(stderr, "XX doing readpixel blit src_pitch %d dst_pitch %d\n",
		     source_pitch, dest_pitch);



      for (i = 0 ; i < nbox ; )
      {
	 int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
	 drm_clip_rect_t *box = dPriv->pClipRects;
	 drm_clip_rect_t *b = mmesa->sarea->boxes;
	 int n = 0;

	 for ( ; i < nr ; i++) {
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

	    b->x1 = bx;
	    b->y1 = by;
	    b->x2 = bx + bw;
	    b->y2 = by + bh;
	    b++;
	    n++;
	 }

	 mmesa->sarea->nbox = n;

	 if (n && (retcode = drmCommandWrite( mmesa->driFd, DRM_MGA_BLIT,
                                              &blit, sizeof(drmMGABlit)))) {
	    fprintf(stderr, "blit ioctl failed, retcode = %d\n", retcode);
	    UNLOCK_HARDWARE( mmesa );
	    exit(1);
	 }
      }

      UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );
   }
#endif

   UNLOCK_HARDWARE( mmesa );

   return GL_TRUE;
}

static void
mgaDDReadPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *pack,
		 GLvoid *pixels )
{
   if (!mgaTryReadPixels( ctx, x, y, width, height, format, type, pack, pixels))
      _swrast_ReadPixels( ctx, x, y, width, height, format, type, pack, pixels);
}




static void do_draw_pix( GLcontext *ctx,
			 GLint x, GLint y, GLsizei width, GLsizei height,
			 GLint pitch,
			 const void *pixels,
			 GLuint dest, GLuint planemask)
{
#if 0
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   drmMGABlit blit;
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   drm_clip_rect_t pbox = dPriv->pClipRects;
   int nbox = dPriv->numClipRects;
   int retcode, i;

   y = dPriv->h - y - height;
   x += mmesa->drawX;
   y += mmesa->drawY;

   blit.dest = dest;
   blit.planemask = planemask;
   blit.source = ((mmesa->mgaScreen->agp.handle + AGP_OFFSET(mmesa, pixels))
		  | SO_srcmap_sys | SO_srcacc_agp);
   blit.dest_pitch = mmesa->mgaScreen->frontPitch / mmesa->mgaScreen->cpp;
   blit.source_pitch = pitch;
   blit.delta_sx = -x;
   blit.delta_sy = -y;
   blit.delta_dx = 0;
   blit.delta_dy = 0;
   if (ctx->Pixel.ZoomY == -1) {
      blit.height = height;
      blit.ydir = 1;
   } else {
      blit.height = height;
      blit.ydir = -1;
   }

   if (0) fprintf(stderr,
		  "doing drawpixel blit src_pitch %d dst_pitch %d\n",
		  blit.source_pitch, blit.dest_pitch);

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
      drm_clip_rect_t *box = mmesa->pClipRects;
      drm_clip_rect_t *b = mmesa->sarea->boxes;
      int n = 0;

      for ( ; i < nr ; i++) {
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

	 b->x1 = bx;
	 b->y1 = by;
	 b->x2 = bx + bw;
	 b->y2 = by + bh;
	 b++;
	 n++;
      }

      mmesa->sarea->nbox = n;

      if (n && (retcode = drmCommandWrite( mmesa->driFd, DRM_MGA_BLIT,
                                              &blit, sizeof(drmMGABlit)))) {
	 fprintf(stderr, "blit ioctl failed, retcode = %d\n", retcode);
	 UNLOCK_HARDWARE( mmesa );
	 exit(1);
      }
   }
#endif
}




static GLboolean
mgaTryDrawPixels( GLcontext *ctx,
		  GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type,
		  const struct gl_pixelstore_attrib *unpack,
		  const GLvoid *pixels )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLint size, skipPixels, skipRows;
   GLint pitch = unpack->RowLength ? unpack->RowLength : width;
   GLuint dest, planemask;
   GLuint cpp = mmesa->mgaScreen->cpp;

   if (!clip_pixelrect(ctx, ctx->DrawBuffer,
		       &x, &y, &width, &height,
		       &skipPixels, &skipRows, &size)) {
      return GL_TRUE;
   }


   switch (format) {
#if defined(MESA_packed_depth_stencil)
   case GL_DEPTH_STENCIL_MESA:
      dest = mmesa->mgaScreen->depthOffset;
      planemask = ~0;
      if (!check_depth_stencil_24_8(ctx, type, unpack, pixels, size, pitch) ||
	  !check_depth_per_fragment_ops(ctx) ||
	  !check_stencil_per_fragment_ops(ctx))
	 return GL_FALSE;
      break;
#endif

   case GL_DEPTH_COMPONENT:
      dest = mmesa->mgaScreen->depthOffset;

      if (ctx->Visual.depthBits == 24)
	 planemask = ~0xff;
      else
	 planemask = ~0;

      if (!check_depth(ctx, type, unpack, pixels, size, pitch) ||
	  !check_depth_per_fragment_ops(ctx))
	 return GL_FALSE;
      break;

   case GL_RGB:
   case GL_BGRA:
      dest = (mmesa->draw_buffer == MGA_FRONT ?
	      mmesa->mgaScreen->frontOffset :
	      mmesa->mgaScreen->backOffset);

      planemask = mgaPackColor(cpp,
			       ctx->Color.ColorMask[RCOMP],
			       ctx->Color.ColorMask[GCOMP],
			       ctx->Color.ColorMask[BCOMP],
			       ctx->Color.ColorMask[ACOMP]);

      if (cpp == 2)
	 planemask |= planemask << 16;

      if (!check_color(ctx, type, format, unpack, pixels, size, pitch)) {
	 return GL_FALSE;
      }
      if (!check_color_per_fragment_ops(ctx)) {
	 return GL_FALSE;
      }
      break;

   default:
      return GL_FALSE;
   }

   LOCK_HARDWARE_QUIESCENT( mmesa );

   if (mmesa->dirty_cliprects & MGA_FRONT)
      mgaUpdateRects( mmesa, MGA_FRONT );

   if ( IS_AGP_MEM(mmesa, (char *)pixels) &&
	IS_AGP_MEM(mmesa, (char *)pixels + size) )
   {
      do_draw_pix( ctx, x, y, width, height, pitch, pixels,
		   dest, planemask );
      UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );
   }
   else
   {
      /* Pixels is in regular memory -- get dma buffers and perform
       * upload through them.
       */
/*        drmBufPtr buf = mgaGetBufferLocked(mmesa); */
      GLuint bufferpitch = (width*cpp+31)&~31;

      char *address = 0; /*  mmesa->mgaScreen->agp.map; */

      do {
/*  	 GLuint rows = MIN2( height, MGA_DMA_BUF_SZ / bufferpitch ); */
	 GLuint rows = height;


	 if (0) fprintf(stderr, "trying to upload %d rows (pitch %d)\n",
			rows, bufferpitch);

	 /* The texture conversion code is so slow that there is only
	  * negligble speedup when the buffers/images don't exactly
	  * match:
	  */
#if 0
	 if (cpp == 2) {
	    if (!_mesa_convert_texsubimage2d( MESA_FORMAT_RGB565,
					      0, 0, width, rows,
					      bufferpitch, format, type,
					      unpack, pixels, address )) {
/*  	       mgaReleaseBufLocked( mmesa, buf ); */
	       UNLOCK_HARDWARE(mmesa);
	       return GL_FALSE;
	    }
	 } else {
	    if (!_mesa_convert_texsubimage2d( MESA_FORMAT_ARGB8888,
					      0, 0, width, rows,
					      bufferpitch, format, type,
					      unpack, pixels, address )) {
/*  	       mgaReleaseBufLocked( mmesa, buf ); */
	       UNLOCK_HARDWARE(mmesa);
	       return GL_FALSE;
	    }
	 }
#else
	 MEMCPY( address, pixels, rows*bufferpitch );
#endif

	 do_draw_pix( ctx, x, y, width, rows,
		      bufferpitch/cpp, address, dest, planemask );

	 /* Fix me -- use multiple buffers to avoid flush.
	  */
	 UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );

	 pixels = (void *)((char *) pixels + rows * pitch);
	 height -= rows;
	 y += rows;
      } while (height);

/*        mgaReleaseBufLocked( mmesa, buf ); */
   }

   UNLOCK_HARDWARE( mmesa );
   mmesa->dirty |= MGA_UPLOAD_CLIPRECTS;

   return GL_TRUE;
}

static void
mgaDDDrawPixels( GLcontext *ctx,
		 GLint x, GLint y, GLsizei width, GLsizei height,
		 GLenum format, GLenum type,
		 const struct gl_pixelstore_attrib *unpack,
		 const GLvoid *pixels )
{
   if (!mgaTryDrawPixels( ctx, x, y, width, height, format, type,
			  unpack, pixels ))
      _swrast_DrawPixels( ctx, x, y, width, height, format, type,
			  unpack, pixels );
}
#endif


/* Stub functions - not a real allocator, always returns pointer to
 * the same block of agp space which isn't used for anything else at
 * present.
 */
void mgaDDInitPixelFuncs( GLcontext *ctx )
{
#if 0
   /* evidently, these functions don't always work */
   if (getenv("MGA_BLIT_PIXELS")) {
      ctx->Driver.ReadPixels = mgaDDReadPixels; /* requires agp dest */
      ctx->Driver.DrawPixels = mgaDDDrawPixels; /* works with agp/normal mem */
   }
#endif
}
