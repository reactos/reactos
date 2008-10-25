/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_ioctl.c,v 1.10 2002/12/16 16:18:53 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *
 */
#include <errno.h>

#define STANDALONE_MMIO
#include "r128_context.h"
#include "r128_state.h"
#include "r128_ioctl.h"
#include "imports.h"
#include "macros.h"

#include "swrast/swrast.h"

#include "vblank.h"
#include "mmio.h"
#include "drirenderbuffer.h"

#define R128_TIMEOUT        2048
#define R128_IDLE_RETRY       32


/* =============================================================
 * Hardware vertex buffer handling
 */

/* Get a new VB from the pool of vertex buffers in AGP space.
 */
drmBufPtr r128GetBufferLocked( r128ContextPtr rmesa )
{
   int fd = rmesa->r128Screen->driScreen->fd;
   int index = 0;
   int size = 0;
   drmDMAReq dma;
   drmBufPtr buf = NULL;
   int to = 0;
   int ret;

   dma.context = rmesa->hHWContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = R128_BUFFER_SIZE;
   dma.request_list = &index;
   dma.request_sizes = &size;
   dma.granted_count = 0;

   while ( !buf && ( to++ < R128_TIMEOUT ) ) {
      ret = drmDMA( fd, &dma );

      if ( ret == 0 ) {
	 buf = &rmesa->r128Screen->buffers->list[index];
	 buf->used = 0;
#if ENABLE_PERF_BOXES
	 /* Bump the performance counter */
	 rmesa->c_vertexBuffers++;
#endif
	 return buf;
      }
   }

   if ( !buf ) {
      drmCommandNone( fd, DRM_R128_CCE_RESET);
      UNLOCK_HARDWARE( rmesa );
      fprintf( stderr, "Error: Could not get new VB... exiting\n" );
      exit( -1 );
   }

   return buf;
}

void r128FlushVerticesLocked( r128ContextPtr rmesa )
{
   drm_clip_rect_t *pbox = rmesa->pClipRects;
   int nbox = rmesa->numClipRects;
   drmBufPtr buffer = rmesa->vert_buf;
   int count = rmesa->num_verts;
   int prim = rmesa->hw_primitive;
   int fd = rmesa->driScreen->fd;
   drm_r128_vertex_t vertex;
   int i;

   rmesa->num_verts = 0;
   rmesa->vert_buf = NULL;

   if ( !buffer )
      return;

   if ( rmesa->dirty & ~R128_UPLOAD_CLIPRECTS )
      r128EmitHwStateLocked( rmesa );

   if ( !nbox )
      count = 0;

   if ( nbox >= R128_NR_SAREA_CLIPRECTS )
      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;

   if ( !count || !(rmesa->dirty & R128_UPLOAD_CLIPRECTS) )
   {
      if ( nbox < 3 ) {
	 rmesa->sarea->nbox = 0;
      } else {
	 rmesa->sarea->nbox = nbox;
      }

      vertex.prim = prim;
      vertex.idx = buffer->idx;
      vertex.count = count;
      vertex.discard = 1;
      drmCommandWrite( fd, DRM_R128_VERTEX, &vertex, sizeof(vertex) );
   }
   else
   {
      for ( i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + R128_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = rmesa->sarea->boxes;
	 int discard = 0;

	 rmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++ ) {
	    *b++ = pbox[i];
	 }

	 /* Finished with the buffer?
	  */
	 if ( nr == nbox ) {
	    discard = 1;
	 }

	 rmesa->sarea->dirty |= R128_UPLOAD_CLIPRECTS;

         vertex.prim = prim;
         vertex.idx = buffer->idx;
         vertex.count = count;
         vertex.discard = discard;
         drmCommandWrite( fd, DRM_R128_VERTEX, &vertex, sizeof(vertex) );
      }
   }

   rmesa->dirty &= ~R128_UPLOAD_CLIPRECTS;
}





/* ================================================================
 * Texture uploads
 */

void r128FireBlitLocked( r128ContextPtr rmesa, drmBufPtr buffer,
			 GLint offset, GLint pitch, GLint format,
			 GLint x, GLint y, GLint width, GLint height )
{
   drm_r128_blit_t blit;
   GLint ret;

   blit.idx = buffer->idx;
   blit.offset = offset;
   blit.pitch = pitch;
   blit.format = format;
   blit.x = x;
   blit.y = y;
   blit.width = width;
   blit.height = height;

   ret = drmCommandWrite( rmesa->driFd, DRM_R128_BLIT, 
                          &blit, sizeof(blit) );

   if ( ret ) {
      UNLOCK_HARDWARE( rmesa );
      fprintf( stderr, "DRM_R128_BLIT: return = %d\n", ret );
      exit( 1 );
   }
}


/* ================================================================
 * SwapBuffers with client-side throttling
 */

static void delay( void ) {
/* Prevent an optimizing compiler from removing a spin loop */
}

#define R128_MAX_OUTSTANDING	2


/* Throttle the frame rate -- only allow one pending swap buffers
 * request at a time.
 * GH: We probably don't want a timeout here, as we can wait as
 * long as we want for a frame to complete.  If it never does, then
 * the card has locked.
 */
static int r128WaitForFrameCompletion( r128ContextPtr rmesa )
{
   unsigned char *R128MMIO = rmesa->r128Screen->mmio.map;
   int i;
   int wait = 0;

   while ( 1 ) {
      u_int32_t frame = read_MMIO_LE32( R128MMIO, R128_LAST_FRAME_REG );

      if ( rmesa->sarea->last_frame - frame <= R128_MAX_OUTSTANDING ) {
	 break;
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
void r128CopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   r128ContextPtr rmesa;
   GLint nbox, i, ret;
   GLboolean missed_target;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (r128ContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "\n********************************\n" );
      fprintf( stderr, "\n%s( %p )\n\n",
	       __FUNCTION__, (void *)rmesa->glCtx );
      fflush( stderr );
   }

   FLUSH_BATCH( rmesa );

   LOCK_HARDWARE( rmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   if ( !r128WaitForFrameCompletion( rmesa ) ) {
      rmesa->hardwareWentIdle = 1;
   } else {
      rmesa->hardwareWentIdle = 0;
   }

   UNLOCK_HARDWARE( rmesa );
   driWaitForVBlank( dPriv, &rmesa->vbl_seq, rmesa->vblank_flags, &missed_target );
   LOCK_HARDWARE( rmesa );

   nbox = dPriv->numClipRects;	/* must be in locked region */

   for ( i = 0 ; i < nbox ; ) {
      GLint nr = MIN2( i + R128_NR_SAREA_CLIPRECTS , nbox );
      drm_clip_rect_t *box = dPriv->pClipRects;
      drm_clip_rect_t *b = rmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {
	 *b++ = box[i];
	 n++;
      }
      rmesa->sarea->nbox = n;

      ret = drmCommandNone( rmesa->driFd, DRM_R128_SWAP );

      if ( ret ) {
	 UNLOCK_HARDWARE( rmesa );
	 fprintf( stderr, "DRM_R128_SWAP: return = %d\n", ret );
	 exit( 1 );
      }
   }

   if ( R128_DEBUG & DEBUG_ALWAYS_SYNC ) {
      i = 0;
      do {
         ret = drmCommandNone(rmesa->driFd, DRM_R128_CCE_IDLE);
      } while ( ret && errno == EBUSY && i++ < R128_IDLE_RETRY );
   }

   UNLOCK_HARDWARE( rmesa );

   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= (R128_UPLOAD_CONTEXT |
		    R128_UPLOAD_MASKS |
		    R128_UPLOAD_CLIPRECTS);

#if ENABLE_PERF_BOXES
   /* Log the performance counters if necessary */
   r128PerformanceCounters( rmesa );
#endif
}

void r128PageFlip( const __DRIdrawablePrivate *dPriv )
{
   r128ContextPtr rmesa;
   GLint ret;
   GLboolean missed_target;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (r128ContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "\n%s( %p ): page=%d\n\n",
	       __FUNCTION__, (void *)rmesa->glCtx, rmesa->sarea->pfCurrentPage );
   }

   FLUSH_BATCH( rmesa );

   LOCK_HARDWARE( rmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   if ( !r128WaitForFrameCompletion( rmesa ) ) {
      rmesa->hardwareWentIdle = 1;
   } else {
      rmesa->hardwareWentIdle = 0;
   }

   UNLOCK_HARDWARE( rmesa );
   driWaitForVBlank( dPriv, &rmesa->vbl_seq, rmesa->vblank_flags, &missed_target );
   LOCK_HARDWARE( rmesa );

   /* The kernel will have been initialized to perform page flipping
    * on a swapbuffers ioctl.
    */
   ret = drmCommandNone( rmesa->driFd, DRM_R128_FLIP );

   UNLOCK_HARDWARE( rmesa );

   if ( ret ) {
      fprintf( stderr, "DRM_R128_FLIP: return = %d\n", ret );
      exit( 1 );
   }

   /* Get ready for drawing next frame.  Update the renderbuffers'
    * flippedOffset/Pitch fields so we draw into the right place.
    */
   driFlipRenderbuffers(rmesa->glCtx->WinSysDrawBuffer,
                        rmesa->sarea->pfCurrentPage);

   rmesa->new_state |= R128_NEW_WINDOW;

   /* FIXME: Do we need this anymore? */
   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= (R128_UPLOAD_CONTEXT |
		    R128_UPLOAD_MASKS |
		    R128_UPLOAD_CLIPRECTS);

#if ENABLE_PERF_BOXES
   /* Log the performance counters if necessary */
   r128PerformanceCounters( rmesa );
#endif
}


/* ================================================================
 * Buffer clear
 */

static void r128Clear( GLcontext *ctx, GLbitfield mask )
{
   r128ContextPtr rmesa = R128_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->driDrawable;
   drm_r128_clear_t clear;
   GLuint flags = 0;
   GLint i;
   GLint ret;
   GLuint depthmask = 0;
   GLint cx, cy, cw, ch;

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s:\n", __FUNCTION__ );
   }

   FLUSH_BATCH( rmesa );

   /* The only state change we care about here is the RGBA colormask
    * We'll just update that state, if needed.  If we do more then
    * there's some strange side-effects that the conformance tests find.
    */
   if ( rmesa->new_state & R128_NEW_MASKS) {
      const GLuint save_state = rmesa->new_state;
      rmesa->new_state = R128_NEW_MASKS;
      r128DDUpdateHWState( ctx );
      rmesa->new_state = save_state & ~R128_NEW_MASKS;
   }

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= R128_FRONT;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= R128_BACK;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( ( mask & BUFFER_BIT_DEPTH ) && ctx->Depth.Mask ) {
      flags |= R128_DEPTH;
      /* if we're at 16 bits, extra plane mask won't hurt */
      depthmask |= 0x00ffffff;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if ( mask & BUFFER_BIT_STENCIL &&
	(ctx->Visual.stencilBits > 0 && ctx->Visual.depthBits == 24) ) {
      flags |= R128_DEPTH;
      depthmask |= ctx->Stencil.WriteMask[0] << 24;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( flags ) {

      LOCK_HARDWARE( rmesa );

      /* compute region after locking: */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - cx;
      ch = ctx->DrawBuffer->_Ymax - cy;

      /* Flip top to bottom */
      cx += dPriv->x;
      cy  = dPriv->y + dPriv->h - cy - ch;

      /* FIXME: Do we actually need this?
       */
      if ( rmesa->dirty & ~R128_UPLOAD_CLIPRECTS ) {
	 r128EmitHwStateLocked( rmesa );
      }

      for ( i = 0 ; i < rmesa->numClipRects ; ) {
	 GLint nr = MIN2( i + R128_NR_SAREA_CLIPRECTS , rmesa->numClipRects );
	 drm_clip_rect_t *box = rmesa->pClipRects;
	 drm_clip_rect_t *b = rmesa->sarea->boxes;
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

	 rmesa->sarea->nbox = n;

	 if ( R128_DEBUG & DEBUG_VERBOSE_IOCTL ) {
	    fprintf( stderr,
		     "DRM_R128_CLEAR: flag 0x%x color %x depth %x nbox %d\n",
		     flags,
		     (GLuint)rmesa->ClearColor,
		     (GLuint)rmesa->ClearDepth,
		     rmesa->sarea->nbox );
	 }

         clear.flags = flags;
         clear.clear_color = rmesa->ClearColor;
         clear.clear_depth = rmesa->ClearDepth;
         clear.color_mask = rmesa->setup.plane_3d_mask_c;
         clear.depth_mask = depthmask;

         ret = drmCommandWrite( rmesa->driFd, DRM_R128_CLEAR,
                                &clear, sizeof(clear) );

	 if ( ret ) {
	    UNLOCK_HARDWARE( rmesa );
	    fprintf( stderr, "DRM_R128_CLEAR: return = %d\n", ret );
	    exit( 1 );
	 }
      }

      UNLOCK_HARDWARE( rmesa );

      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
   }

   if ( mask )
      _swrast_Clear( ctx, mask );
}


/* ================================================================
 * Depth spans, pixels
 */

void r128WriteDepthSpanLocked( r128ContextPtr rmesa,
			       GLuint n, GLint x, GLint y,
			       const GLuint depth[],
			       const GLubyte mask[] )
{
   drm_clip_rect_t *pbox = rmesa->pClipRects;
   drm_r128_depth_t d;
   int nbox = rmesa->numClipRects;
   int fd = rmesa->driScreen->fd;
   int i;

   if ( !nbox || !n ) {
      return;
   }
   if ( nbox >= R128_NR_SAREA_CLIPRECTS ) {
      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
   }

   if ( !(rmesa->dirty & R128_UPLOAD_CLIPRECTS) )
   {
      if ( nbox < 3 ) {
	 rmesa->sarea->nbox = 0;
      } else {
	 rmesa->sarea->nbox = nbox;
      }

      d.func = R128_WRITE_SPAN;
      d.n = n;
      d.x = (int*)&x;
      d.y = (int*)&y;
      d.buffer = (unsigned int *)depth;
      d.mask = (unsigned char *)mask;

      drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));

   }
   else
   {
      for (i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + R128_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = rmesa->sarea->boxes;

	 rmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++) {
	    *b++ = pbox[i];
	 }

	 rmesa->sarea->dirty |= R128_UPLOAD_CLIPRECTS;

         d.func = R128_WRITE_SPAN;
         d.n = n;
         d.x = (int*)&x;
         d.y = (int*)&y;
         d.buffer = (unsigned int *)depth;
         d.mask = (unsigned char *)mask;

         drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
      }
   }

   rmesa->dirty &= ~R128_UPLOAD_CLIPRECTS;
}

void r128WriteDepthPixelsLocked( r128ContextPtr rmesa, GLuint n,
				 const GLint x[], const GLint y[],
				 const GLuint depth[],
				 const GLubyte mask[] )
{
   drm_clip_rect_t *pbox = rmesa->pClipRects;
   drm_r128_depth_t d;
   int nbox = rmesa->numClipRects;
   int fd = rmesa->driScreen->fd;
   int i;

   if ( !nbox || !n ) {
      return;
   }
   if ( nbox >= R128_NR_SAREA_CLIPRECTS ) {
      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
   }

   if ( !(rmesa->dirty & R128_UPLOAD_CLIPRECTS) )
   {
      if ( nbox < 3 ) {
	 rmesa->sarea->nbox = 0;
      } else {
	 rmesa->sarea->nbox = nbox;
      }

      d.func = R128_WRITE_PIXELS;
      d.n = n;
      d.x = (int*)&x;
      d.y = (int*)&y;
      d.buffer = (unsigned int *)depth;
      d.mask = (unsigned char *)mask;

      drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
   }
   else
   {
      for (i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + R128_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = rmesa->sarea->boxes;

	 rmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++) {
	    *b++ = pbox[i];
	 }

	 rmesa->sarea->dirty |= R128_UPLOAD_CLIPRECTS;

         d.func = R128_WRITE_PIXELS;
         d.n = n;
         d.x = (int*)&x;
         d.y = (int*)&y;
         d.buffer = (unsigned int *)depth;
         d.mask = (unsigned char *)mask;

         drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
      }
   }

   rmesa->dirty &= ~R128_UPLOAD_CLIPRECTS;
}

void r128ReadDepthSpanLocked( r128ContextPtr rmesa,
			      GLuint n, GLint x, GLint y )
{
   drm_clip_rect_t *pbox = rmesa->pClipRects;
   drm_r128_depth_t d;
   int nbox = rmesa->numClipRects;
   int fd = rmesa->driScreen->fd;
   int i;

   if ( !nbox || !n ) {
      return;
   }
   if ( nbox >= R128_NR_SAREA_CLIPRECTS ) {
      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
   }

   if ( !(rmesa->dirty & R128_UPLOAD_CLIPRECTS) )
   {
      if ( nbox < 3 ) {
	 rmesa->sarea->nbox = 0;
      } else {
	 rmesa->sarea->nbox = nbox;
      }

      d.func = R128_READ_SPAN;
      d.n = n;
      d.x = (int*)&x;
      d.y = (int*)&y;
      d.buffer = NULL;
      d.mask = NULL;

      drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
   }
   else
   {
      for (i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + R128_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = rmesa->sarea->boxes;

	 rmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++) {
	    *b++ = pbox[i];
	 }

	 rmesa->sarea->dirty |= R128_UPLOAD_CLIPRECTS;

         d.func = R128_READ_SPAN;
         d.n = n;
         d.x = (int*)&x;
         d.y = (int*)&y;
         d.buffer = NULL;
         d.mask = NULL;

         drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
      }
   }

   rmesa->dirty &= ~R128_UPLOAD_CLIPRECTS;
}

void r128ReadDepthPixelsLocked( r128ContextPtr rmesa, GLuint n,
				const GLint x[], const GLint y[] )
{
   drm_clip_rect_t *pbox = rmesa->pClipRects;
   drm_r128_depth_t d;
   int nbox = rmesa->numClipRects;
   int fd = rmesa->driScreen->fd;
   int i;

   if ( !nbox || !n ) {
      return;
   }
   if ( nbox >= R128_NR_SAREA_CLIPRECTS ) {
      rmesa->dirty |= R128_UPLOAD_CLIPRECTS;
   }

   if ( !(rmesa->dirty & R128_UPLOAD_CLIPRECTS) )
   {
      if ( nbox < 3 ) {
	 rmesa->sarea->nbox = 0;
      } else {
	 rmesa->sarea->nbox = nbox;
      }

      d.func = R128_READ_PIXELS;
      d.n = n;
      d.x = (int*)&x;
      d.y = (int*)&y;
      d.buffer = NULL;
      d.mask = NULL;

      drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
   }
   else
   {
      for (i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + R128_NR_SAREA_CLIPRECTS, nbox );
	 drm_clip_rect_t *b = rmesa->sarea->boxes;

	 rmesa->sarea->nbox = nr - i;
	 for ( ; i < nr ; i++) {
	    *b++ = pbox[i];
	 }

	 rmesa->sarea->dirty |= R128_UPLOAD_CLIPRECTS;

         d.func = R128_READ_PIXELS;
         d.n = n;
         d.x = (int*)&x;
         d.y = (int*)&y;
         d.buffer = NULL;
         d.mask = NULL;

         drmCommandWrite( fd, DRM_R128_DEPTH, &d, sizeof(d));
      }
   }

   rmesa->dirty &= ~R128_UPLOAD_CLIPRECTS;
}


void r128WaitForIdleLocked( r128ContextPtr rmesa )
{
    int fd = rmesa->r128Screen->driScreen->fd;
    int to = 0;
    int ret, i;

    do {
        i = 0;
        do {
            ret = drmCommandNone( fd, DRM_R128_CCE_IDLE);
        } while ( ret && errno == EBUSY && i++ < R128_IDLE_RETRY );
    } while ( ( ret == -EBUSY ) && ( to++ < R128_TIMEOUT ) );

    if ( ret < 0 ) {
        drmCommandNone( fd, DRM_R128_CCE_RESET);
	UNLOCK_HARDWARE( rmesa );
	fprintf( stderr, "Error: Rage 128 timed out... exiting\n" );
	exit( -1 );
    }
}

void r128InitIoctlFuncs( struct dd_function_table *functions )
{
    functions->Clear = r128Clear;
}
