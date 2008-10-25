/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
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
 * \file mgaioctl.c
 * MGA IOCTL related wrapper functions.
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author Gareth Hughes <gareth@valinux.com>
 */

#include <errno.h>
#include "mtypes.h"
#include "macros.h"
#include "dd.h"
#include "swrast/swrast.h"

#include "mm.h"
#include "drm.h"
#include "mga_drm.h"
#include "mgacontext.h"
#include "mgadd.h"
#include "mgastate.h"
#include "mgatex.h"
#include "mgavb.h"
#include "mgaioctl.h"
#include "mgatris.h"

#include "vblank.h"


static int
mgaSetFence( mgaContextPtr mmesa, uint32_t * fence )
{
    int ret = ENOSYS;

    if ( mmesa->driScreen->drmMinor >= 2 ) {
	ret = drmCommandWriteRead( mmesa->driScreen->fd, DRM_MGA_SET_FENCE,
				   fence, sizeof( uint32_t ));
	if (ret) {
	    fprintf(stderr, "drmMgaSetFence: %d\n", ret);
	    exit(1);
	}
    }

    return ret;
}


static int
mgaWaitFence( mgaContextPtr mmesa, uint32_t fence, uint32_t * curr_fence )
{
    int ret = ENOSYS;

    if ( mmesa->driScreen->drmMinor >= 2 ) {
	uint32_t temp = fence;
	
	ret = drmCommandWriteRead( mmesa->driScreen->fd,
				   DRM_MGA_WAIT_FENCE,
				   & temp, sizeof( uint32_t ));
	if (ret) {
	   fprintf(stderr, "drmMgaSetFence: %d\n", ret);
	    exit(1);
	}

	if ( curr_fence ) {
	    *curr_fence = temp;
	}
    }

    return ret;
}


static void mga_iload_dma_ioctl(mgaContextPtr mmesa,
				unsigned long dest,
				int length)
{
   drmBufPtr buf = mmesa->iload_buffer;
   drm_mga_iload_t iload;
   int ret, i;

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "DRM_IOCTL_MGA_ILOAD idx %d dst %x length %d\n",
	      buf->idx, (int) dest, length);

   if ( (length & MGA_ILOAD_MASK) != 0 ) {
      UNLOCK_HARDWARE( mmesa );
      fprintf( stderr, "%s: Invalid ILOAD datasize (%d), must be "
	       "multiple of %u.\n", __FUNCTION__, length, MGA_ILOAD_ALIGN );
      exit( 1 );
   }

   iload.idx = buf->idx;
   iload.dstorg = dest;
   iload.length = length;

   i = 0;
   do {
      ret = drmCommandWrite( mmesa->driFd, DRM_MGA_ILOAD, 
                             &iload, sizeof(iload) );
   } while ( ret == -EBUSY && i++ < DRM_MGA_IDLE_RETRY );

   if ( ret < 0 ) {
      printf("send iload retcode = %d\n", ret);
      exit(1);
   }

   mmesa->iload_buffer = 0;

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "finished iload dma put\n");

}

drmBufPtr mga_get_buffer_ioctl( mgaContextPtr mmesa )
{
   int idx = 0;
   int size = 0;
   drmDMAReq dma;
   int retcode;
   drmBufPtr buf;

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr,  "Getting dma buffer\n");

   dma.context = mmesa->hHWContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = MGA_BUFFER_SIZE;
   dma.request_list = &idx;
   dma.request_sizes = &size;
   dma.granted_count = 0;


   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "drmDMA (get) ctx %d count %d size 0x%x\n",
	   dma.context, dma.request_count,
	   dma.request_size);

   while (1) {
      retcode = drmDMA(mmesa->driFd, &dma);

      if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
	 fprintf(stderr, "retcode %d sz %d idx %d count %d\n",
		 retcode,
		 dma.request_sizes[0],
		 dma.request_list[0],
		 dma.granted_count);

      if (retcode == 0 &&
	  dma.request_sizes[0] &&
	  dma.granted_count)
	 break;

      if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
	 fprintf(stderr, "\n\nflush");

      UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT );
   }

   buf = &(mmesa->mgaScreen->bufs->list[idx]);
   buf->used = 0;

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr,
	   "drmDMA (get) returns size[0] 0x%x idx[0] %d\n"
	   "dma_buffer now: buf idx: %d size: %d used: %d addr %p\n",
	   dma.request_sizes[0], dma.request_list[0],
	   buf->idx, buf->total,
	   buf->used, buf->address);

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "finished getbuffer\n");

   return buf;
}




static void
mgaClear( GLcontext *ctx, GLbitfield mask )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = mmesa->driDrawable;
   GLuint flags = 0;
   GLuint clear_color = mmesa->ClearColor;
   GLuint clear_depth = 0;
   GLuint color_mask = 0;
   GLuint depth_mask = 0;
   int ret;
   int i;
   static int nrclears;
   drm_mga_clear_t clear;
   GLint cx, cy, cw, ch;

   FLUSH_BATCH( mmesa );

   if ( mask & BUFFER_BIT_FRONT_LEFT ) {
      flags |= MGA_FRONT;
      color_mask = mmesa->setup.plnwt;
      mask &= ~BUFFER_BIT_FRONT_LEFT;
   }

   if ( mask & BUFFER_BIT_BACK_LEFT ) {
      flags |= MGA_BACK;
      color_mask = mmesa->setup.plnwt;
      mask &= ~BUFFER_BIT_BACK_LEFT;
   }

   if ( (mask & BUFFER_BIT_DEPTH) && ctx->Depth.Mask ) {
      flags |= MGA_DEPTH;
      clear_depth = (mmesa->ClearDepth & mmesa->depth_clear_mask);
      depth_mask |= mmesa->depth_clear_mask;
      mask &= ~BUFFER_BIT_DEPTH;
   }

   if ( (mask & BUFFER_BIT_STENCIL) && mmesa->hw_stencil ) {
      flags |= MGA_DEPTH;
      clear_depth |= (ctx->Stencil.Clear & mmesa->stencil_clear_mask);
      depth_mask |= mmesa->stencil_clear_mask;
      mask &= ~BUFFER_BIT_STENCIL;
   }

   if ( flags ) {
      LOCK_HARDWARE( mmesa );

      /* compute region after locking: */
      cx = ctx->DrawBuffer->_Xmin;
      cy = ctx->DrawBuffer->_Ymin;
      cw = ctx->DrawBuffer->_Xmax - cx;
      ch = ctx->DrawBuffer->_Ymax - cy;

      if ( mmesa->dirty_cliprects )
	 mgaUpdateRects( mmesa, (MGA_FRONT | MGA_BACK) );

      /* flip top to bottom */
      cy = dPriv->h-cy-ch;
      cx += mmesa->drawX;
      cy += mmesa->drawY;

      if ( MGA_DEBUG & DEBUG_VERBOSE_IOCTL )
	 fprintf( stderr, "Clear, bufs %x nbox %d\n",
		  (int)flags, (int)mmesa->numClipRects );

      for (i = 0 ; i < mmesa->numClipRects ; )
      {
	 int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, mmesa->numClipRects);
	 drm_clip_rect_t *box = mmesa->pClipRects;
	 drm_clip_rect_t *b = mmesa->sarea->boxes;
	 int n = 0;

	 if (cw != dPriv->w || ch != dPriv->h) {
            /* clear subregion */
	    for ( ; i < nr ; i++) {
	       GLint x = box[i].x1;
	       GLint y = box[i].y1;
	       GLint w = box[i].x2 - x;
	       GLint h = box[i].y2 - y;

	       if (x < cx) w -= cx - x, x = cx;
	       if (y < cy) h -= cy - y, y = cy;
	       if (x + w > cx + cw) w = cx + cw - x;
	       if (y + h > cy + ch) h = cy + ch - y;
	       if (w <= 0) continue;
	       if (h <= 0) continue;

	       b->x1 = x;
	       b->y1 = y;
	       b->x2 = x + w;
	       b->y2 = y + h;
	       b++;
	       n++;
	    }
	 } else {
            /* clear whole window */
	    for ( ; i < nr ; i++) {
	       *b++ = box[i];
	       n++;
	    }
	 }


	 if ( MGA_DEBUG & DEBUG_VERBOSE_IOCTL )
	    fprintf( stderr,
		     "DRM_IOCTL_MGA_CLEAR flag 0x%x color %x depth %x nbox %d\n",
		     flags, clear_color, clear_depth, mmesa->sarea->nbox );

	 mmesa->sarea->nbox = n;

         clear.flags = flags;
         clear.clear_color = clear_color;
         clear.clear_depth = clear_depth;
         clear.color_mask = color_mask;
         clear.depth_mask = depth_mask;
         ret = drmCommandWrite( mmesa->driFd, DRM_MGA_CLEAR,
                                 &clear, sizeof(clear));
	 if ( ret ) {
	    fprintf( stderr, "send clear retcode = %d\n", ret );
	    exit( 1 );
	 }
	 if ( MGA_DEBUG & DEBUG_VERBOSE_IOCTL )
	    fprintf( stderr, "finished clear %d\n", ++nrclears );
      }

      UNLOCK_HARDWARE( mmesa );
      mmesa->dirty |= MGA_UPLOAD_CLIPRECTS|MGA_UPLOAD_CONTEXT;
   }

   if (mask) 
      _swrast_Clear( ctx, mask );
}


/**
 * Wait for the previous frame of rendering has completed.
 * 
 * \param mmesa  Hardware context pointer.
 *
 * \bug
 * The loop in this function should have some sort of a timeout mechanism.
 *
 * \warning
 * This routine used to assume that the hardware lock was held on entry.  It
 * now assumes that the lock is \b not held on entry.
 */

static void mgaWaitForFrameCompletion( mgaContextPtr mmesa )
{
    if ( mgaWaitFence( mmesa, mmesa->last_frame_fence, NULL ) == ENOSYS ) {
	unsigned wait = 0;
	GLuint last_frame;
	GLuint last_wrap;


	LOCK_HARDWARE( mmesa );
	last_frame = mmesa->sarea->last_frame.head;
	last_wrap = mmesa->sarea->last_frame.wrap;

	/* The DMA routines in the kernel track a couple values in the SAREA
	 * that we use here.  The number of times that the primary DMA buffer
	 * has "wrapped" around is tracked in last_wrap.  In addition, the
	 * wrap count and the buffer position at the end of the last frame are
	 * stored in last_frame.wrap and last_frame.head.
	 * 
	 * By comparing the wrap counts and the current DMA pointer value
	 * (read directly from the hardware) to last_frame.head, we can
	 * determine when the graphics processor has processed all of the
	 * commands for the last frame.
	 * 
	 * In this case "last frame" means the frame of the *previous* swap-
	 * buffers call.  This is done to prevent queuing a second buffer swap
	 * before the previous swap is executed.
	 */
	while ( 1 ) {
	    if ( last_wrap < mmesa->sarea->last_wrap ||
		 ( last_wrap == mmesa->sarea->last_wrap &&
		   last_frame <= (MGA_READ( MGAREG_PRIMADDRESS ) -
				  mmesa->primary_offset) ) ) {
		break;
	    }
	    if ( 0 ) {
		wait++;
		fprintf( stderr, "   last: head=0x%06x wrap=%d\n",
			 last_frame, last_wrap );
		fprintf( stderr, "   head: head=0x%06lx wrap=%d\n",
			 (long)(MGA_READ( MGAREG_PRIMADDRESS ) - mmesa->primary_offset),
			 mmesa->sarea->last_wrap );
	    }
	    UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH );

	    UNLOCK_HARDWARE( mmesa );
	    DO_USLEEP( 1 );
	    LOCK_HARDWARE( mmesa );
	}
	if ( wait )
	  fprintf( stderr, "\n" );

	UNLOCK_HARDWARE( mmesa );
    }
}


/*
 * Copy the back buffer to the front buffer.
 */
void mgaCopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   mgaContextPtr mmesa;
   drm_clip_rect_t *pbox;
   GLint nbox;
   GLint ret;
   GLint i;
   GLboolean   missed_target;


   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   mmesa = (mgaContextPtr) dPriv->driContextPriv->driverPrivate;

   FLUSH_BATCH( mmesa );

   mgaWaitForFrameCompletion( mmesa );
   driWaitForVBlank( dPriv, & mmesa->vbl_seq, mmesa->vblank_flags,
		     & missed_target );
   if ( missed_target ) {
      mmesa->swap_missed_count++;
      (void) (*dri_interface->getUST)( & mmesa->swap_missed_ust );
   }
   LOCK_HARDWARE( mmesa );

   /* Use the frontbuffer cliprects
    */
   if (mmesa->dirty_cliprects & MGA_FRONT)
      mgaUpdateRects( mmesa, MGA_FRONT );


   pbox = dPriv->pClipRects;
   nbox = dPriv->numClipRects;

   for (i = 0 ; i < nbox ; )
   {
      int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, dPriv->numClipRects);
      drm_clip_rect_t *b = mmesa->sarea->boxes;

      mmesa->sarea->nbox = nr - i;

      for ( ; i < nr ; i++)
	 *b++ = pbox[i];

      if (0)
	 fprintf(stderr, "DRM_IOCTL_MGA_SWAP\n");

      ret = drmCommandNone( mmesa->driFd, DRM_MGA_SWAP );
      if ( ret ) {
	 printf("send swap retcode = %d\n", ret);
	 exit(1);
      }
   }

   (void) mgaSetFence( mmesa, & mmesa->last_frame_fence );
   UNLOCK_HARDWARE( mmesa );

   mmesa->dirty |= MGA_UPLOAD_CLIPRECTS;
   mmesa->swap_count++;
   (void) (*dri_interface->getUST)( & mmesa->swap_ust );
}


/**
 * Implement the hardware-specific portion of \c glFinish.
 *
 * Flushes all pending commands to the hardware and wait for them to finish.
 * 
 * \param ctx  Context where the \c glFinish command was issued.
 *
 * \sa glFinish, mgaFlush, mgaFlushDMA
 */
static void mgaFinish( GLcontext *ctx  )
{
    mgaContextPtr mmesa = MGA_CONTEXT(ctx);
    uint32_t  fence;


    LOCK_HARDWARE( mmesa );
    if ( mmesa->vertex_dma_buffer != NULL ) {
	mgaFlushVerticesLocked( mmesa );
    }

    if ( mgaSetFence( mmesa, & fence ) == 0 ) {
	UNLOCK_HARDWARE( mmesa );
	(void) mgaWaitFence( mmesa, fence, NULL );
    }
    else {
	if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL) {
	    fprintf(stderr, "mgaRegetLockQuiescent\n");
	}

	UPDATE_LOCK( mmesa, DRM_LOCK_QUIESCENT | DRM_LOCK_FLUSH );
	UNLOCK_HARDWARE( mmesa );
    }
}


/**
 * Flush all commands upto at least a certain point to the hardware.
 *
 * \note
 * The term "wait" in the name of this function is misleading.  It doesn't
 * actually wait for anything.  It just makes sure that the commands have
 * been flushed to the hardware.
 *
 * \warning
 * As the name implies, this function assumes that the hardware lock is
 * held on entry.
 */
void mgaWaitAgeLocked( mgaContextPtr mmesa, int age  )
{
   if (GET_DISPATCH_AGE(mmesa) < age) {
      UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH );
   }
}


static GLboolean intersect_rect( drm_clip_rect_t *out,
				 const drm_clip_rect_t *a,
				 const drm_clip_rect_t *b )
{
   *out = *a;
   if (b->x1 > out->x1) out->x1 = b->x1;
   if (b->y1 > out->y1) out->y1 = b->y1;
   if (b->x2 < out->x2) out->x2 = b->x2;
   if (b->y2 < out->y2) out->y2 = b->y2;

   return ((out->x1 < out->x2) && (out->y1 < out->y2));
}




static void age_mmesa( mgaContextPtr mmesa, int age )
{
   if (mmesa->CurrentTexObj[0]) mmesa->CurrentTexObj[0]->age = age;
   if (mmesa->CurrentTexObj[1]) mmesa->CurrentTexObj[1]->age = age;
}


void mgaFlushVerticesLocked( mgaContextPtr mmesa )
{
   drm_clip_rect_t *pbox = mmesa->pClipRects;
   int nbox = mmesa->numClipRects;
   drmBufPtr buffer = mmesa->vertex_dma_buffer;
   drm_mga_vertex_t vertex;
   int i;

   mmesa->vertex_dma_buffer = 0;

   if (!buffer)
      return;

   if (mmesa->dirty_cliprects & mmesa->draw_buffer)
      mgaUpdateRects( mmesa, mmesa->draw_buffer );

   if (mmesa->dirty & ~MGA_UPLOAD_CLIPRECTS)
      mgaEmitHwStateLocked( mmesa );

   /* FIXME: Workaround bug in kernel module.
    */
   mmesa->sarea->dirty |= MGA_UPLOAD_CONTEXT;

   if (!nbox)
      buffer->used = 0;

   if (nbox >= MGA_NR_SAREA_CLIPRECTS)
      mmesa->dirty |= MGA_UPLOAD_CLIPRECTS;

#if 0
   if (!buffer->used || !(mmesa->dirty & MGA_UPLOAD_CLIPRECTS))
   {
      if (nbox == 1)
	 mmesa->sarea->nbox = 0;
      else
	 mmesa->sarea->nbox = nbox;

      if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
	 fprintf(stderr, "Firing vertex -- case a nbox %d\n", nbox);

      vertex.idx = buffer->idx;
      vertex.used = buffer->used;
      vertex.discard = 1;
      drmCommandWrite( mmesa->driFd, DRM_MGA_VERTEX, 
                       &vertex, sizeof(drmMGAVertex) );

      age_mmesa(mmesa, mmesa->sarea->last_enqueue);
   }
   else
#endif
   {
      for (i = 0 ; i < nbox ; )
      {
	 int nr = MIN2(i + MGA_NR_SAREA_CLIPRECTS, nbox);
	 drm_clip_rect_t *b = mmesa->sarea->boxes;
	 int discard = 0;

	 if (mmesa->scissor) {
	    mmesa->sarea->nbox = 0;

	    for ( ; i < nr ; i++) {
	       *b = pbox[i];
	       if (intersect_rect(b, b, &mmesa->scissor_rect)) {
		  mmesa->sarea->nbox++;
		  b++;
	       }
	    }

	    /* Culled?
	     */
	    if (!mmesa->sarea->nbox) {
	       if (nr < nbox) continue;
	       buffer->used = 0;
	    }
	 } else {
	    mmesa->sarea->nbox = nr - i;
	    for ( ; i < nr ; i++)
	       *b++ = pbox[i];
	 }

	 /* Finished with the buffer?
	  */
	 if (nr == nbox)
	    discard = 1;

	 mmesa->sarea->dirty |= MGA_UPLOAD_CLIPRECTS;

         vertex.idx = buffer->idx;
         vertex.used = buffer->used;
         vertex.discard = discard;
         drmCommandWrite( mmesa->driFd, DRM_MGA_VERTEX,
                          &vertex, sizeof(vertex) );

	 age_mmesa(mmesa, mmesa->sarea->last_enqueue);
      }
   }

   mmesa->dirty &= ~MGA_UPLOAD_CLIPRECTS;
}

void mgaFlushVertices( mgaContextPtr mmesa )
{
   LOCK_HARDWARE( mmesa );
   mgaFlushVerticesLocked( mmesa );
   UNLOCK_HARDWARE( mmesa );
}


void mgaFireILoadLocked( mgaContextPtr mmesa,
			 GLuint offset, GLuint length )
{
   if (!mmesa->iload_buffer) {
      fprintf(stderr, "mgaFireILoad: no buffer\n");
      return;
   }

   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "mgaFireILoad idx %d ofs 0x%x length %d\n",
	      mmesa->iload_buffer->idx, (int)offset, (int)length );

   mga_iload_dma_ioctl( mmesa, offset, length );
}

void mgaGetILoadBufferLocked( mgaContextPtr mmesa )
{
   if (MGA_DEBUG&DEBUG_VERBOSE_IOCTL)
      fprintf(stderr, "mgaGetIloadBuffer (buffer now %p)\n",
              (void *) mmesa->iload_buffer);

   mmesa->iload_buffer = mga_get_buffer_ioctl( mmesa );
}


/**
 * Implement the hardware-specific portion of \c glFlush.
 *
 * \param ctx  Context to be flushed.
 *
 * \sa glFlush, mgaFinish, mgaFlushDMA
 */
static void mgaFlush( GLcontext *ctx )
{
    mgaContextPtr mmesa = MGA_CONTEXT( ctx );


    LOCK_HARDWARE( mmesa );
    if ( mmesa->vertex_dma_buffer != NULL ) {
	mgaFlushVerticesLocked( mmesa );
    }

    UPDATE_LOCK( mmesa, DRM_LOCK_FLUSH );
    UNLOCK_HARDWARE( mmesa );
}


int mgaFlushDMA( int fd, drmLockFlags flags )
{
   drm_lock_t lock;
   int ret, i = 0;

   memset( &lock, 0, sizeof(lock) );

   lock.flags = flags & (DRM_LOCK_QUIESCENT | DRM_LOCK_FLUSH 
			 | DRM_LOCK_FLUSH_ALL);

   do {
      ret = drmCommandWrite( fd, DRM_MGA_FLUSH, &lock, sizeof(lock) );
   } while ( ret && errno == EBUSY && i++ < DRM_MGA_IDLE_RETRY );

   if ( ret == 0 )
      return 0;
   if ( errno != EBUSY )
      return -errno;

   if ( lock.flags & DRM_LOCK_QUIESCENT ) {
      /* Only keep trying if we need quiescence.
       */
      lock.flags &= ~(DRM_LOCK_FLUSH | DRM_LOCK_FLUSH_ALL);

      do {
         ret = drmCommandWrite( fd, DRM_MGA_FLUSH, &lock, sizeof(lock) );
      } while ( ret && errno == EBUSY && i++ < DRM_MGA_IDLE_RETRY );
   }

   if ( ret == 0 ) {
      return 0;
   } else {
      return -errno;
   }
}

void mgaInitIoctlFuncs( struct dd_function_table *functions )
{
   functions->Clear = mgaClear;
   functions->Flush = mgaFlush;
   functions->Finish = mgaFinish;
}
