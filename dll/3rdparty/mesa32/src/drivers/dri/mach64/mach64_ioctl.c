/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	Jos�Fonseca <j_r_fonseca@yahoo.co.uk>
 */
#include <errno.h>

#include "mach64_context.h"
#include "mach64_state.h"
#include "mach64_ioctl.h"
#include "mach64_tex.h"

#include "imports.h"
#include "macros.h"

#include "swrast/swrast.h"

#include "vblank.h"

#define MACH64_TIMEOUT        10 /* the DRM already has a timeout, so keep this small */


/* =============================================================
 * Hardware vertex buffer handling
 */

/* Get a new VB from the pool of vertex buffers in AGP space.
 */
drmBufPtr mach64GetBufferLocked( mach64ContextPtr mmesa )
{
   int fd = mmesa->mach64Screen->driScreen->fd;
   int index = 0;
   int size = 0;
   drmDMAReq dma;
   drmBufPtr buf = NULL;
   int to = 0;
   int ret;

   dma.context = mmesa->hHWContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = MACH64_BUFFER_SIZE;
   dma.request_list = &index;
   dma.request_sizes = &size;
   dma.granted_count = 0;

   while ( !buf && ( to++ < MACH64_TIMEOUT ) ) {
      ret = drmDMA( fd, &dma );

      if ( ret == 0 ) {
	 buf = &mmesa->mach64Screen->buffers->list[index];
	 buf->used = 0;
#if ENABLE_PERF_BOXES
	 /* Bump the performance counter */
	 mmesa->c_vertexBuffers++;
#endif
	 return buf;
      }
   }

   if ( !buf ) {
      drmCommandNone( fd, DRM_MACH64_RESET );
      UNLOCK_HARDWARE( mmesa );
      fprintf( stderr, "Error: Could not get new VB... exiting\n" );
      exit( -1 );
   }

   return buf;
}

void mach64FlushVerticesLocked( mach64ContextPtr mmesa )
{
   drm_clip_rect_t *pbox = mmesa->pClipRects;
   int nbox = mmesa->numClipRects;
   void *buffer = mmesa->vert_buf;
   int count = mmesa->vert_used;
   int prim = mmesa->hw_primitive;
   int fd = mmesa->driScreen->fd;
   drm_mach64_vertex_t vertex;
   int i;

   mmesa->num_verts = 0;
   mmesa->vert_used = 0;

   if ( !count )
      return;

   if ( mmesa->dirty & ~MACH64_UPLOAD_CLIPRECTS )
      mach64EmitHwStateLocked( mmesa );

   if ( !nbox )
      count = 0;

   if ( nbox > MACH64_NR_SAREA_CLIPRECTS )
      mmesa->dirty |= MACH64_UPLOAD_CLIPRECTS;

   if ( !count || !(mmesa->dirty & MACH64_UPLOAD_CLIPRECTS) ) {
      int to = 0;
      int ret;

      /* FIXME: Is this really necessary */
      if ( nbox == 1 )
	 mmesa->sarea->nbox = 0;
      else
	 mmesa->sarea->nbox = nbox;

      vertex.prim = prim;
      vertex.buf = buffer;
      vertex.used = count;
      vertex.discard = 1;
      do {
	 ret = drmCommandWrite( fd, DRM_MACH64_VERTEX,
				&vertex, sizeof(drm_mach64_vertex_t) );
      } while ( ( ret == -EAGAIN ) && ( to++ < MACH64_TIMEOUT ) );
      if ( ret ) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "Error flushing vertex buffer: return = %d\n", ret );
	 exit( -1 );
      }

   } else {

      for ( i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + MACH64_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = mmesa->sarea->boxes;
	 int discard = 0;
	 int to = 0;
	 int ret;

	 mmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++ ) {
	    *b++ = pbox[i];
	 }

	 /* Finished with the buffer?
	  */
	 if ( nr == nbox ) {
	    discard = 1;
	 }

	 mmesa->sarea->dirty |= MACH64_UPLOAD_CLIPRECTS;
	 
	 vertex.prim = prim;
	 vertex.buf = buffer;
	 vertex.used = count;
	 vertex.discard = discard;
	 do {
	    ret = drmCommandWrite( fd, DRM_MACH64_VERTEX,
				   &vertex, sizeof(drm_mach64_vertex_t) );
	 } while ( ( ret == -EAGAIN ) && ( to++ < MACH64_TIMEOUT ) );
	 if ( ret ) {
	    UNLOCK_HARDWARE( mmesa );
	    fprintf( stderr, "Error flushing vertex buffer: return = %d\n", ret );
	    exit( -1 );
	 }
      }
   }

   mmesa->dirty &= ~MACH64_UPLOAD_CLIPRECTS;
}

/* ================================================================
 * Texture uploads
 */

void mach64FireBlitLocked( mach64ContextPtr mmesa, void *buffer,
			   GLint offset, GLint pitch, GLint format,
			   GLint x, GLint y, GLint width, GLint height )
{
   drm_mach64_blit_t blit;
   int to = 0;
   int ret;

   blit.buf = buffer;
   blit.offset = offset;
   blit.pitch = pitch;
   blit.format = format;
   blit.x = x;
   blit.y = y;
   blit.width = width;
   blit.height = height;

   do {
      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_BLIT, 
			     &blit, sizeof(drm_mach64_blit_t) );
   } while ( ( ret == -EAGAIN ) && ( to++ < MACH64_TIMEOUT ) );

   if ( ret ) {
      UNLOCK_HARDWARE( mmesa );
      fprintf( stderr, "DRM_MACH64_BLIT: return = %d\n", ret );
      exit( -1 );
   }
}


/* ================================================================
 * SwapBuffers with client-side throttling
 */
static void delay( void ) {
/* Prevent an optimizing compiler from removing a spin loop */
}

/* Throttle the frame rate -- only allow MACH64_MAX_QUEUED_FRAMES
 * pending swap buffers requests at a time.
 *
 * GH: We probably don't want a timeout here, as we can wait as
 * long as we want for a frame to complete.  If it never does, then
 * the card has locked.
 */
static int mach64WaitForFrameCompletion( mach64ContextPtr mmesa )
{
   int fd = mmesa->driFd;
   int i;
   int wait = 0;
   int frames;

   while ( 1 ) {
      drm_mach64_getparam_t gp;
      int ret;

      if ( mmesa->sarea->frames_queued < MACH64_MAX_QUEUED_FRAMES ) {
	 break;
      }

      if (MACH64_DEBUG & DEBUG_NOWAIT) {
	 return 1;
      }

      gp.param = MACH64_PARAM_FRAMES_QUEUED;
      gp.value = &frames; /* also copied into sarea->frames_queued by DRM */

      ret = drmCommandWriteRead( fd, DRM_MACH64_GETPARAM, &gp, sizeof(gp) );

      if ( ret ) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_GETPARAM: return = %d\n", ret );
	 exit( -1 );
      }

      /* Spin in place a bit so we aren't hammering the register */
      wait++;

      for ( i = 0 ; i < 1024 ; i++ ) {
	 delay();
      }

   }

   return wait;
}

/* Copy the back color buffer to the front color buffer.
 */
void mach64CopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   mach64ContextPtr mmesa;
   GLint nbox, i, ret;
   drm_clip_rect_t *pbox;
   GLboolean missed_target;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   mmesa = (mach64ContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "\n********************************\n" );
      fprintf( stderr, "\n%s( %p )\n\n",
	       __FUNCTION__, mmesa->glCtx );
      fflush( stderr );
   }

   /* Flush any outstanding vertex buffers */
   FLUSH_BATCH( mmesa );

   LOCK_HARDWARE( mmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   if ( !mach64WaitForFrameCompletion( mmesa ) ) {
      mmesa->hardwareWentIdle = 1;
   } else {
      mmesa->hardwareWentIdle = 0;
   }

#if ENABLE_PERF_BOXES
   if ( mmesa->boxes ) {
      mach64PerformanceBoxesLocked( mmesa );
   }
#endif

   UNLOCK_HARDWARE( mmesa );
   driWaitForVBlank( dPriv, &mmesa->vbl_seq, mmesa->vblank_flags, &missed_target );
   LOCK_HARDWARE( mmesa );

   /* use front buffer cliprects */
   nbox = dPriv->numClipRects;
   pbox = dPriv->pClipRects;

   for ( i = 0 ; i < nbox ; ) {
      GLint nr = MIN2( i + MACH64_NR_SAREA_CLIPRECTS , nbox );
      drm_clip_rect_t *b = mmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {
	 *b++ = pbox[i];
	 n++;
      }
      mmesa->sarea->nbox = n;

      ret = drmCommandNone( mmesa->driFd, DRM_MACH64_SWAP );

      if ( ret ) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_SWAP: return = %d\n", ret );
	 exit( -1 );
      }
   }

   if ( MACH64_DEBUG & DEBUG_ALWAYS_SYNC ) {
      mach64WaitForIdleLocked( mmesa );
   }

   UNLOCK_HARDWARE( mmesa );

   mmesa->dirty |= (MACH64_UPLOAD_CONTEXT |
		    MACH64_UPLOAD_MISC |
		    MACH64_UPLOAD_CLIPRECTS);

#if ENABLE_PERF_BOXES
   /* Log the performance counters if necessary */
   mach64PerformanceCounters( mmesa );
#endif
}

#if ENABLE_PERF_BOXES
/* ================================================================
 * Performance monitoring
 */

void mach64PerformanceCounters( mach64ContextPtr mmesa )
{

   if (MACH64_DEBUG & DEBUG_VERBOSE_COUNT) {
      /* report performance counters */
      fprintf( stderr, "mach64CopyBuffer: vertexBuffers:%i drawWaits:%i clears:%i\n",
	       mmesa->c_vertexBuffers, mmesa->c_drawWaits, mmesa->c_clears );
   }

   mmesa->c_vertexBuffers = 0;
   mmesa->c_drawWaits = 0;
   mmesa->c_clears = 0;

   if ( mmesa->c_textureSwaps || mmesa->c_textureBytes || mmesa->c_agpTextureBytes ) {
      if (MACH64_DEBUG & DEBUG_VERBOSE_COUNT) {
	 fprintf( stderr, "    textureSwaps:%i  textureBytes:%i agpTextureBytes:%i\n",
		  mmesa->c_textureSwaps, mmesa->c_textureBytes, mmesa->c_agpTextureBytes );
      }
      mmesa->c_textureSwaps = 0;
      mmesa->c_textureBytes = 0;
      mmesa->c_agpTextureBytes = 0;
   }

   mmesa->c_texsrc_agp = 0;
   mmesa->c_texsrc_card = 0;

   if (MACH64_DEBUG & DEBUG_VERBOSE_COUNT)
      fprintf( stderr, "---------------------------------------------------------\n" );
}


void mach64PerformanceBoxesLocked( mach64ContextPtr mmesa )
{
   GLint ret;
   drm_mach64_clear_t clear;
   GLint x, y, w, h;
   GLuint color;
   GLint nbox;
   GLint x1, y1, x2, y2;
   drm_clip_rect_t *b = mmesa->sarea->boxes;

   /* save cliprects */
   nbox = mmesa->sarea->nbox;
   x1 = b[0].x1;
   y1 = b[0].y1;
   x2 = b[0].x2;
   y2 = b[0].y2;
 
   /* setup a single cliprect and call the clear ioctl for each box */
   mmesa->sarea->nbox = 1;

   w = h = 8;
   x = mmesa->drawX;
   y = mmesa->drawY;
   b[0].x1 = x;
   b[0].x2 = x + w;
   b[0].y1 = y;
   b[0].y2 = y + h;

   clear.flags = MACH64_BACK;
   clear.clear_depth = 0;

   /* Red box if DDFinish was called to wait for rendering to complete */
   if ( mmesa->c_drawWaits ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 255, 0, 0, 0 );
      
      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
			     &clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }

   }

   x += w;
   b[0].x1 = x;
   b[0].x2 = x + w;

   /* draw a green box if we had to wait for previous frame(s) to complete */
   if ( !mmesa->hardwareWentIdle ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 0, 255, 0, 0 );
      
      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
			     &clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }

   }

   x += w;
   w = 20;
   b[0].x1 = x;

   /* show approx. ratio of AGP/card textures used - Blue = AGP, Purple = Card */
   if ( mmesa->c_texsrc_agp || mmesa->c_texsrc_card ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 0, 0, 255, 0 );
      w = ((GLfloat)mmesa->c_texsrc_agp / (GLfloat)(mmesa->c_texsrc_agp + mmesa->c_texsrc_card))*20;
      if (w > 1) {

	 b[0].x2 = x + w;

	 clear.x = x;
	 clear.y = y;
	 clear.w = w;
	 clear.h = h;
	 clear.clear_color = color;

	 ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

	 if (ret < 0) {
	    UNLOCK_HARDWARE( mmesa );
	    fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	    exit( -1 );
	 }
      }

      x += w;
      w = 20 - w;

      if (w > 1) {
	 b[0].x1 = x;
	 b[0].x2 = x + w;

	 color = mach64PackColor( mmesa->mach64Screen->cpp, 255, 0, 255, 0 );

	 clear.x = x;
	 clear.y = y;
	 clear.w = w;
	 clear.h = h;
	 clear.clear_color = color;

	 ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

	 if (ret < 0) {
	    UNLOCK_HARDWARE( mmesa );
	    fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	    exit( -1 );
	 }
      }
   }  

   x += w;
   w = 8;
   b[0].x1 = x;
   b[0].x2 = x + w;

   /* Yellow box if we swapped textures */
   if ( mmesa->c_textureSwaps ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 255, 255, 0, 0 );

      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }
      
   }

   h = 4;
   x += 8;
   b[0].x1 = x;
   b[0].y2 = y + h;

   /* Purple bar for card memory texture blits/uploads */
   if ( mmesa->c_textureBytes ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 255, 0, 255, 0 );
      w = mmesa->c_textureBytes / 16384;
      if ( w <= 0 ) 
	 w = 1; 
      if (w > (mmesa->driDrawable->w - 44))
	 w = mmesa->driDrawable->w - 44;

      b[0].x2 = x + w;

      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }
   }

   /* Blue bar for AGP memory texture blits/uploads */
   if ( mmesa->c_agpTextureBytes ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 0, 0, 255, 0 );
      w = mmesa->c_agpTextureBytes / 16384;
      if ( w <= 0 ) 
	 w = 1; 
      if (w > (mmesa->driDrawable->w - 44))
	 w = mmesa->driDrawable->w - 44;

      y += 4;
      b[0].x2 = x + w;
      b[0].y1 = y;
      b[0].y2 = y + h;

      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }
   }

   /* Pink bar for number of vertex buffers used */
   if ( mmesa->c_vertexBuffers ) {
      color = mach64PackColor( mmesa->mach64Screen->cpp, 196, 128, 128, 0 );

      w = mmesa->c_vertexBuffers;
      if (w > (mmesa->driDrawable->w))
	 w = mmesa->driDrawable->w;

      h = 8;
      x = mmesa->drawX;
      y = mmesa->drawY + 8;
      b[0].x1 = x;
      b[0].x2 = x + w;
      b[0].y1 = y;
      b[0].y2 = y + h;

      clear.x = x;
      clear.y = y;
      clear.w = w;
      clear.h = h;
      clear.clear_color = color;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
				&clear, sizeof(drm_mach64_clear_t) );

      if (ret < 0) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }
   }

   /* restore cliprects */
   mmesa->sarea->nbox = nbox;
   b[0].x1 = x1;
   b[0].y1 = y1;
   b[0].x2 = x2;
   b[0].y2 = y2;

}

#endif

/* ================================================================
 * Buffer clear
 */

static void mach64DDClear( GLcontext *ctx, GLbitfield mask )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   drm_mach64_clear_t clear;
   GLuint flags = 0;
   GLint i;
   GLint ret;
   GLint cx, cy, cw, ch;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "mach64DDClear\n");
   }

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   mmesa->c_clears++;
#endif

   FLUSH_BATCH( mmesa );

   /* The only state changes we care about here are the RGBA colormask
    * and scissor/clipping.  We'll just update that state, if needed.
    */
   if ( mmesa->new_state & (MACH64_NEW_MASKS | MACH64_NEW_CLIP) ) {
      const GLuint save_state = mmesa->new_state;
      mmesa->new_state &= (MACH64_NEW_MASKS | MACH64_NEW_CLIP);
      mach64DDUpdateHWState( ctx );
      mmesa->new_state = save_state & ~(MACH64_NEW_MASKS | MACH64_NEW_CLIP);
   }

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= MACH64_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= MACH64_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( ( mask & BUFFER_BIT_DEPTH ) && ctx->Depth.Mask ) {
      flags |= MACH64_DEPTH;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if ( mask )
      _swrast_Clear( ctx, mask );

   if ( !flags )
      return;

   LOCK_HARDWARE( mmesa );

   /* compute region after locking: */
   cx = ctx->DrawBuffer->_Xmin;
   cy = ctx->DrawBuffer->_Ymin;
   cw = ctx->DrawBuffer->_Xmax - cx;
   ch = ctx->DrawBuffer->_Ymax - cy;

   /* Flip top to bottom */
   cx += mmesa->drawX;
   cy  = mmesa->drawY + dPriv->h - cy - ch;

   /* HACK?
    */
   if ( mmesa->dirty & ~MACH64_UPLOAD_CLIPRECTS ) {
      mach64EmitHwStateLocked( mmesa );
   }

   for ( i = 0 ; i < mmesa->numClipRects ; ) {
      int nr = MIN2( i + MACH64_NR_SAREA_CLIPRECTS, mmesa->numClipRects );
      drm_clip_rect_t *box = mmesa->pClipRects;
      drm_clip_rect_t *b = mmesa->sarea->boxes;
      GLint n = 0;

      if (cw != dPriv->w || ch != dPriv->h) {
         /* clear subregion */
	 for ( ; i < nr ; i++ ) {
	    GLint x = box[i].x1;
	    GLint y = box[i].y1;
	    GLint w = box[i].x2 - x;
	    GLint h = box[i].y2 - y;

	    if ( x < cx ) w -= cx - x, x = cx;
	    if ( y < cy ) h -= cy - y, y = cy;
	    if ( x + w > cx + cw ) w = cx + cw - x;
	    if ( y + h > cy + ch ) h = cy + ch - y;
	    if ( w <= 0 ) continue;
	    if ( h <= 0 ) continue;

	    b->x1 = x;
	    b->y1 = y;
	    b->x2 = x + w;
	    b->y2 = y + h;
	    b++;
	    n++;
	 }
      } else {
         /* clear whole window */
	 for ( ; i < nr ; i++ ) {
	    *b++ = box[i];
	    n++;
	 }
      }

      mmesa->sarea->nbox = n;

      if ( MACH64_DEBUG & DEBUG_VERBOSE_IOCTL ) {
	 fprintf( stderr,
		  "DRM_MACH64_CLEAR: flag 0x%x color %x depth %x nbox %d\n",
		  flags,
		  (GLuint)mmesa->ClearColor,
		  (GLuint)mmesa->ClearDepth,
		  mmesa->sarea->nbox );
      }

      clear.flags = flags;
      clear.x = cx;
      clear.y = cy;
      clear.w = cw;
      clear.h = ch;
      clear.clear_color = mmesa->ClearColor;
      clear.clear_depth = mmesa->ClearDepth;

      ret = drmCommandWrite( mmesa->driFd, DRM_MACH64_CLEAR,
			     &clear, sizeof(drm_mach64_clear_t) );

      if ( ret ) {
	 UNLOCK_HARDWARE( mmesa );
	 fprintf( stderr, "DRM_MACH64_CLEAR: return = %d\n", ret );
	 exit( -1 );
      }
   }

   UNLOCK_HARDWARE( mmesa );

   mmesa->dirty |= (MACH64_UPLOAD_CONTEXT |
		    MACH64_UPLOAD_MISC |
		    MACH64_UPLOAD_CLIPRECTS);

}


void mach64WaitForIdleLocked( mach64ContextPtr mmesa )
{
   int fd = mmesa->driFd;
   int to = 0;
   int ret;

   do {
      ret = drmCommandNone( fd, DRM_MACH64_IDLE );
   } while ( ( ret == -EBUSY ) && ( to++ < MACH64_TIMEOUT ) );

   if ( ret < 0 ) {
      drmCommandNone( fd, DRM_MACH64_RESET );
      UNLOCK_HARDWARE( mmesa );
      fprintf( stderr, "Error: Mach64 timed out... exiting\n" );
      exit( -1 );
   }
}

/* Flush the DMA queue to the hardware */
void mach64FlushDMALocked( mach64ContextPtr mmesa )
{
   int fd = mmesa->driFd;
   int ret;

   ret = drmCommandNone( fd, DRM_MACH64_FLUSH );

   if ( ret < 0 ) {
      drmCommandNone( fd, DRM_MACH64_RESET );
      UNLOCK_HARDWARE( mmesa );
      fprintf( stderr, "Error flushing DMA... exiting\n" );
      exit( -1 );
   }

   mmesa->dirty |= (MACH64_UPLOAD_CONTEXT |
		    MACH64_UPLOAD_MISC |
		    MACH64_UPLOAD_CLIPRECTS);

}

/* For client-side state emits - currently unused */
void mach64UploadHwStateLocked( mach64ContextPtr mmesa )
{
   drm_mach64_sarea_t *sarea = mmesa->sarea;
   
   drm_mach64_context_regs_t *regs = &sarea->context_state;
   unsigned int dirty = sarea->dirty;
   CARD32 offset = ((regs->tex_size_pitch & 0xf0) >> 2);

   DMALOCALS;

   DMAGETPTR( 19*2 );

   if ( dirty & MACH64_UPLOAD_MISC ) {
      DMAOUTREG( MACH64_DP_MIX, regs->dp_mix );
      DMAOUTREG( MACH64_DP_SRC, regs->dp_src );
      DMAOUTREG( MACH64_CLR_CMP_CNTL, regs->clr_cmp_cntl );
      DMAOUTREG( MACH64_GUI_TRAJ_CNTL, regs->gui_traj_cntl );
      DMAOUTREG( MACH64_SC_LEFT_RIGHT, regs->sc_left_right );
      DMAOUTREG( MACH64_SC_TOP_BOTTOM, regs->sc_top_bottom );
      sarea->dirty &= ~MACH64_UPLOAD_MISC;
   }

   if ( dirty & MACH64_UPLOAD_DST_OFF_PITCH ) {
      DMAOUTREG( MACH64_DST_OFF_PITCH, regs->dst_off_pitch );
      sarea->dirty &= ~MACH64_UPLOAD_DST_OFF_PITCH;
   }
   if ( dirty & MACH64_UPLOAD_Z_OFF_PITCH ) {
      DMAOUTREG( MACH64_Z_OFF_PITCH, regs->z_off_pitch );
      sarea->dirty &= ~MACH64_UPLOAD_Z_OFF_PITCH;
   }
   if ( dirty & MACH64_UPLOAD_Z_ALPHA_CNTL ) {
      DMAOUTREG( MACH64_Z_CNTL, regs->z_cntl );
      DMAOUTREG( MACH64_ALPHA_TST_CNTL, regs->alpha_tst_cntl );
      sarea->dirty &= ~MACH64_UPLOAD_Z_ALPHA_CNTL;
   }
   if ( dirty & MACH64_UPLOAD_SCALE_3D_CNTL ) {
      DMAOUTREG( MACH64_SCALE_3D_CNTL, regs->scale_3d_cntl );
      sarea->dirty &= ~MACH64_UPLOAD_SCALE_3D_CNTL;
   }
   if ( dirty & MACH64_UPLOAD_DP_FOG_CLR ) {
      DMAOUTREG( MACH64_DP_FOG_CLR, regs->dp_fog_clr );
      sarea->dirty &= ~MACH64_UPLOAD_DP_FOG_CLR;
   }
   if ( dirty & MACH64_UPLOAD_DP_WRITE_MASK ) {
      DMAOUTREG( MACH64_DP_WRITE_MASK, regs->dp_write_mask );
      sarea->dirty &= ~MACH64_UPLOAD_DP_WRITE_MASK;
   }
   if ( dirty & MACH64_UPLOAD_DP_PIX_WIDTH ) {
      DMAOUTREG( MACH64_DP_PIX_WIDTH, regs->dp_pix_width );
      sarea->dirty &= ~MACH64_UPLOAD_DP_PIX_WIDTH;
   }
   if ( dirty & MACH64_UPLOAD_SETUP_CNTL ) {
      DMAOUTREG( MACH64_SETUP_CNTL, regs->setup_cntl );
      sarea->dirty &= ~MACH64_UPLOAD_SETUP_CNTL;
   }

   if ( dirty & MACH64_UPLOAD_TEXTURE ) {
      DMAOUTREG( MACH64_TEX_SIZE_PITCH, regs->tex_size_pitch );
      DMAOUTREG( MACH64_TEX_CNTL, regs->tex_cntl );
      DMAOUTREG( MACH64_SECONDARY_TEX_OFF, regs->secondary_tex_off );
      DMAOUTREG( MACH64_TEX_0_OFF + offset, regs->tex_offset );
      sarea->dirty &= ~MACH64_UPLOAD_TEXTURE;
   }

#if 0
   if ( dirty & MACH64_UPLOAD_CLIPRECTS ) {
      DMAOUTREG( MACH64_SC_LEFT_RIGHT, regs->sc_left_right );
      DMAOUTREG( MACH64_SC_TOP_BOTTOM, regs->sc_top_bottom );
      sarea->dirty &= ~MACH64_UPLOAD_CLIPRECTS;
   }
#endif

   sarea->dirty = 0;

   DMAADVANCE();
}

void mach64InitIoctlFuncs( struct dd_function_table *functions )
{
    functions->Clear = mach64DDClear;
}
