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


#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>

#include "mtypes.h"
#include "context.h"
#include "swrast/swrast.h"

#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"
#include "drm.h"

u_int32_t intelGetLastFrame (intelContextPtr intel) 
{
   int ret;
   u_int32_t frame;
   drm_i915_getparam_t gp;
   
   gp.param = I915_PARAM_LAST_DISPATCH;
   gp.value = (int *)&frame;
   ret = drmCommandWriteRead( intel->driFd, DRM_I915_GETPARAM,
			      &gp, sizeof(gp) );
   return frame;
}

int intelEmitIrqLocked( intelContextPtr intel )
{
   drmI830IrqEmit ie;
   int ret, seq;
      
   assert(((*(int *)intel->driHwLock) & ~DRM_LOCK_CONT) == 
	  (DRM_LOCK_HELD|intel->hHWContext));

   ie.irq_seq = &seq;
	 
   ret = drmCommandWriteRead( intel->driFd, DRM_I830_IRQ_EMIT, 
			      &ie, sizeof(ie) );
   if ( ret ) {
      fprintf( stderr, "%s: drmI830IrqEmit: %d\n", __FUNCTION__, ret );
      exit(1);
   }
   
   if (0)
      fprintf(stderr, "%s -->  %d\n", __FUNCTION__, seq );

   return seq;
}

void intelWaitIrq( intelContextPtr intel, int seq )
{
   int ret;
      
   if (0)
      fprintf(stderr, "%s %d\n", __FUNCTION__, seq );

   intel->iw.irq_seq = seq;
	 
   do {
     ret = drmCommandWrite( intel->driFd, DRM_I830_IRQ_WAIT, &intel->iw, sizeof(intel->iw) );
   } while (ret == -EAGAIN || ret == -EINTR);

   if ( ret ) {
      fprintf( stderr, "%s: drmI830IrqWait: %d\n", __FUNCTION__, ret );
      if (0)
	 intel_dump_batchbuffer( intel->alloc.offset,
				 intel->alloc.ptr,
				 intel->alloc.size );
      exit(1);
   }
}



static void age_intel( intelContextPtr intel, int age )
{
   GLuint i;

   for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++)
      if (intel->CurrentTexObj[i]) 
	 intel->CurrentTexObj[i]->age = age;
}

void intel_dump_batchbuffer( long offset,
			     int *ptr,
			     int count )
{
   int i;
   fprintf(stderr, "\n\n\nSTART BATCH (%d dwords):\n", count);
   for (i = 0; i < count/4; i += 4) 
      fprintf(stderr, "\t0x%x: 0x%08x 0x%08x 0x%08x 0x%08x\n", 
	      (unsigned int)offset + i*4, ptr[i], ptr[i+1], ptr[i+2], ptr[i+3]);
   fprintf(stderr, "END BATCH\n\n\n");
}

void intelRefillBatchLocked( intelContextPtr intel, GLboolean allow_unlock )
{
   GLuint last_irq = intel->alloc.irq_emitted;
   GLuint half = intel->alloc.size / 2;
   GLuint buf = (intel->alloc.active_buf ^= 1);

   intel->alloc.irq_emitted = intelEmitIrqLocked( intel );

   if (last_irq) {
      if (allow_unlock) UNLOCK_HARDWARE( intel ); 
      intelWaitIrq( intel, last_irq );
      if (allow_unlock) LOCK_HARDWARE( intel ); 
   }

   if (0)
      fprintf(stderr, "%s: now using half %d\n", __FUNCTION__, buf);

   intel->batch.start_offset = intel->alloc.offset + buf * half;
   intel->batch.ptr = (unsigned char *)intel->alloc.ptr + buf * half;
   intel->batch.size = half - 8;
   intel->batch.space = half - 8;
   assert(intel->batch.space >= 0);
}

#define MI_BATCH_BUFFER_END 	(0xA<<23)


void intelFlushBatchLocked( intelContextPtr intel, 
			    GLboolean ignore_cliprects,
			    GLboolean refill,
			    GLboolean allow_unlock)
{
   drmI830BatchBuffer batch;

   assert(intel->locked);

   if (0)
      fprintf(stderr, "%s used %d of %d offset %x..%x refill %d (started in %s)\n",
	      __FUNCTION__, 
	      (intel->batch.size - intel->batch.space), 
	      intel->batch.size,
	      intel->batch.start_offset,
	      intel->batch.start_offset + 
	      (intel->batch.size - intel->batch.space), 
	      refill,
	      intel->batch.func);

   /* Throw away non-effective packets.  Won't work once we have
    * hardware contexts which would preserve statechanges beyond a
    * single buffer.
    */
   if (intel->numClipRects == 0 && !ignore_cliprects) {
      
      /* Without this yeild, an application with no cliprects can hog
       * the hardware.  Without unlocking, the effect is much worse -
       * effectively a lock-out of other contexts.
       */
      if (allow_unlock) {
	 UNLOCK_HARDWARE( intel );
	 sched_yield();
	 LOCK_HARDWARE( intel );
      }

      /* Note that any state thought to have been emitted actually
       * hasn't:
       */
      intel->batch.ptr -= (intel->batch.size - intel->batch.space);
      intel->batch.space = intel->batch.size;
      intel->vtbl.lost_hardware( intel ); 
   }

   if (intel->batch.space != intel->batch.size) {

      if (intel->sarea->ctxOwner != intel->hHWContext) {
	 intel->perf_boxes |= I830_BOX_LOST_CONTEXT;
	 intel->sarea->ctxOwner = intel->hHWContext;
      }

      batch.start = intel->batch.start_offset;
      batch.used = intel->batch.size - intel->batch.space;
      batch.cliprects = intel->pClipRects;
      batch.num_cliprects = ignore_cliprects ? 0 : intel->numClipRects;
      batch.DR1 = 0;
      batch.DR4 = ((((GLuint)intel->drawX) & 0xffff) | 
		   (((GLuint)intel->drawY) << 16));
      
      if (intel->alloc.offset) {
	 if ((batch.used & 0x4) == 0) {
	    ((int *)intel->batch.ptr)[0] = 0;
	    ((int *)intel->batch.ptr)[1] = MI_BATCH_BUFFER_END;
	    batch.used += 0x8;
	    intel->batch.ptr += 0x8;
	 }
	 else {
	    ((int *)intel->batch.ptr)[0] = MI_BATCH_BUFFER_END;
	    batch.used += 0x4;
	    intel->batch.ptr += 0x4;
	 }      
      }

      if (0)
 	 intel_dump_batchbuffer( batch.start,
				 (int *)(intel->batch.ptr - batch.used),
				 batch.used );

      intel->batch.start_offset += batch.used;
      intel->batch.size -= batch.used;

      if (intel->batch.size < 8) {
	 refill = GL_TRUE;
	 intel->batch.space = intel->batch.size = 0;
      }
      else {
	 intel->batch.size -= 8;
	 intel->batch.space = intel->batch.size;
      }


      assert(intel->batch.space >= 0);
      assert(batch.start >= intel->alloc.offset);
      assert(batch.start < intel->alloc.offset + intel->alloc.size);
      assert(batch.start + batch.used > intel->alloc.offset);
      assert(batch.start + batch.used <= 
	     intel->alloc.offset + intel->alloc.size);


      if (intel->alloc.offset) {
	 if (drmCommandWrite (intel->driFd, DRM_I830_BATCHBUFFER, &batch, 
			      sizeof(batch))) {
	    fprintf(stderr, "DRM_I830_BATCHBUFFER: %d\n",  -errno);
	    UNLOCK_HARDWARE(intel);
	    exit(1);
	 }
      } else {
	 drmI830CmdBuffer cmd;
	 cmd.buf = (char *)intel->alloc.ptr + batch.start;
	 cmd.sz = batch.used;
	 cmd.DR1 = batch.DR1;
	 cmd.DR4 = batch.DR4;
	 cmd.num_cliprects = batch.num_cliprects;
	 cmd.cliprects = batch.cliprects;
	 
	 if (drmCommandWrite (intel->driFd, DRM_I830_CMDBUFFER, &cmd, 
			      sizeof(cmd))) {
	    fprintf(stderr, "DRM_I830_CMDBUFFER: %d\n",  -errno);
	    UNLOCK_HARDWARE(intel);
	    exit(1);
	 }
      }	 

      
      age_intel(intel, intel->sarea->last_enqueue);

      /* FIXME: use hardware contexts to avoid 'losing' hardware after
       * each buffer flush.
       */
      if (intel->batch.contains_geometry) 
	 assert(intel->batch.last_emit_state == intel->batch.counter);

      intel->batch.counter++;
      intel->batch.contains_geometry = 0;
      intel->batch.func = 0;
      intel->vtbl.lost_hardware( intel );
   }

   if (refill)
      intelRefillBatchLocked( intel, allow_unlock );
}

void intelFlushBatch( intelContextPtr intel, GLboolean refill )
{
   if (intel->locked) {
      intelFlushBatchLocked( intel, GL_FALSE, refill, GL_FALSE );
   } 
   else {
      LOCK_HARDWARE(intel);
      intelFlushBatchLocked( intel, GL_FALSE, refill, GL_TRUE );
      UNLOCK_HARDWARE(intel);
   }
}


void intelWaitForIdle( intelContextPtr intel )
{   
   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   intel->vtbl.emit_flush( intel );
   intelFlushBatch( intel, GL_TRUE );

   /* Use an irq to wait for dma idle -- Need to track lost contexts
    * to shortcircuit consecutive calls to this function:
    */
   intelWaitIrq( intel, intel->alloc.irq_emitted );
   intel->alloc.irq_emitted = 0;
}


/**
 * Check if we need to rotate/warp the front color buffer to the
 * rotated screen.  We generally need to do this when we get a glFlush
 * or glFinish after drawing to the front color buffer.
 */
static void
intelCheckFrontRotate(GLcontext *ctx)
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   if (intel->ctx.DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT) {
      intelScreenPrivate *screen = intel->intelScreen;
      if (screen->current_rotation != 0) {
         __DRIdrawablePrivate *dPriv = intel->driDrawable;
         intelRotateWindow(intel, dPriv, BUFFER_BIT_FRONT_LEFT);
      }
   }
}


/**
 * NOT directly called via glFlush.
 */
void intelFlush( GLcontext *ctx )
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );

   if (intel->Fallback)
      _swrast_flush( ctx );

   INTEL_FIREVERTICES( intel );

   if (intel->batch.size != intel->batch.space)
      intelFlushBatch( intel, GL_FALSE );
}


/**
 * Called via glFlush.
 */
void intelglFlush( GLcontext *ctx )
{
   intelFlush(ctx);
   intelCheckFrontRotate(ctx);
}


void intelFinish( GLcontext *ctx  ) 
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   intelFlush( ctx );
   intelWaitForIdle( intel );
   intelCheckFrontRotate(ctx);
}


void intelClear(GLcontext *ctx, GLbitfield mask)
{
   intelContextPtr intel = INTEL_CONTEXT( ctx );
   const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* Take care of cliprects, which are handled differently for
    * clears, etc.
    */
   intelFlush( &intel->ctx );

   if (mask & BUFFER_BIT_FRONT_LEFT) {
      if (colorMask == ~0) {
	 blit_mask |= BUFFER_BIT_FRONT_LEFT;
      } 
      else {
	 tri_mask |= BUFFER_BIT_FRONT_LEFT;
      }
   }

   if (mask & BUFFER_BIT_BACK_LEFT) {
      if (colorMask == ~0) {
	 blit_mask |= BUFFER_BIT_BACK_LEFT;
      } 
      else {
	 tri_mask |= BUFFER_BIT_BACK_LEFT;
      }
   }

   if (mask & BUFFER_BIT_DEPTH) {
      blit_mask |= BUFFER_BIT_DEPTH;
   }

   if (mask & BUFFER_BIT_STENCIL) {
      if (!intel->hw_stencil) {
	 swrast_mask |= BUFFER_BIT_STENCIL;
      }
      else if ((ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
	 tri_mask |= BUFFER_BIT_STENCIL;
      } 
      else {
	 blit_mask |= BUFFER_BIT_STENCIL;
      }
   }

   swrast_mask |= (mask & BUFFER_BIT_ACCUM);

   if (blit_mask) 
      intelClearWithBlit( ctx, blit_mask, 0, 0, 0, 0, 0);

   if (tri_mask) 
      intel->vtbl.clear_with_tris( intel, tri_mask, 0, 0, 0, 0, 0);

   if (swrast_mask)
      _swrast_Clear( ctx, swrast_mask );
}


void
intelRotateWindow(intelContextPtr intel, __DRIdrawablePrivate *dPriv,
                  GLuint srcBuffer)
{
   if (intel->vtbl.rotate_window) {
      intel->vtbl.rotate_window(intel, dPriv, srcBuffer);
   }
}


void *intelAllocateAGP( intelContextPtr intel, GLsizei size )
{
   int region_offset;
   drmI830MemAlloc alloc;
   int ret;

   if (0)
      fprintf(stderr, "%s: %d bytes\n", __FUNCTION__, size);

   alloc.region = I830_MEM_REGION_AGP;
   alloc.alignment = 0;
   alloc.size = size;
   alloc.region_offset = &region_offset;

   LOCK_HARDWARE(intel);

   /* Make sure the global heap is initialized
    */
   if (intel->texture_heaps[0])
      driAgeTextures( intel->texture_heaps[0] );


   ret = drmCommandWriteRead( intel->driFd,
			      DRM_I830_ALLOC,
			      &alloc, sizeof(alloc));
   
   if (ret) {
      fprintf(stderr, "%s: DRM_I830_ALLOC ret %d\n", __FUNCTION__, ret);
      UNLOCK_HARDWARE(intel);
      return NULL;
   }
   
   if (0)
      fprintf(stderr, "%s: allocated %d bytes\n", __FUNCTION__, size);

   /* Need to propogate this information (agp memory in use) to our
    * local texture lru.  The kernel has already updated the global
    * lru.  An alternative would have been to allocate memory the
    * usual way and then notify the kernel to pin the allocation.
    */
   if (intel->texture_heaps[0])
      driAgeTextures( intel->texture_heaps[0] );

   UNLOCK_HARDWARE(intel);   

   return (void *)((char *)intel->intelScreen->tex.map + region_offset);
}

void intelFreeAGP( intelContextPtr intel, void *pointer )
{
   int region_offset;
   drmI830MemFree memfree;
   int ret;

   region_offset = (char *)pointer - (char *)intel->intelScreen->tex.map;

   if (region_offset < 0 || 
       region_offset > intel->intelScreen->tex.size) {
      fprintf(stderr, "offset %d outside range 0..%d\n", region_offset,
	      intel->intelScreen->tex.size);
      return;
   }

   memfree.region = I830_MEM_REGION_AGP;
   memfree.region_offset = region_offset;
   
   ret = drmCommandWrite( intel->driFd,
			  DRM_I830_FREE,
			  &memfree, sizeof(memfree));
   
   if (ret) 
      fprintf(stderr, "%s: DRM_I830_FREE ret %d\n", __FUNCTION__, ret);
}

/* This version of AllocateMemoryMESA allocates only agp memory, and
 * only does so after the point at which the driver has been
 * initialized.
 *
 * Theoretically a valid context isn't required.  However, in this
 * implementation, it is, as I'm using the hardware lock to protect
 * the kernel data structures, and the current context to get the
 * device fd.
 */
void *intelAllocateMemoryMESA(__DRInativeDisplay *dpy, int scrn,
			      GLsizei size, GLfloat readfreq,
			      GLfloat writefreq, GLfloat priority)
{
   GET_CURRENT_CONTEXT(ctx);

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s sz %d %f/%f/%f\n", __FUNCTION__, size, readfreq, 
	      writefreq, priority);

   if (getenv("INTEL_NO_ALLOC"))
      return NULL;
   
   if (!ctx || INTEL_CONTEXT(ctx) == 0) 
      return NULL;
   
   return intelAllocateAGP( INTEL_CONTEXT(ctx), size );
}


/* Called via glXFreeMemoryMESA() */
void intelFreeMemoryMESA(__DRInativeDisplay *dpy, int scrn, GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   if (INTEL_DEBUG & DEBUG_IOCTL) 
      fprintf(stderr, "%s %p\n", __FUNCTION__, pointer);

   if (!ctx || INTEL_CONTEXT(ctx) == 0) {
      fprintf(stderr, "%s: no context\n", __FUNCTION__);
      return;
   }

   intelFreeAGP( INTEL_CONTEXT(ctx), pointer );
}

/* Called via glXGetMemoryOffsetMESA() 
 *
 * Returns offset of pointer from the start of agp aperture.
 */
GLuint intelGetMemoryOffsetMESA(__DRInativeDisplay *dpy, int scrn, 
				const GLvoid *pointer)
{
   GET_CURRENT_CONTEXT(ctx);
   intelContextPtr intel;

   if (!ctx || !(intel = INTEL_CONTEXT(ctx)) ) {
      fprintf(stderr, "%s: no context\n", __FUNCTION__);
      return ~0;
   }

   if (!intelIsAgpMemory( intel, pointer, 0 ))
      return ~0;

   return intelAgpOffsetFromVirtual( intel, pointer );
}


GLboolean intelIsAgpMemory( intelContextPtr intel, const GLvoid *pointer,
			   GLint size )
{
   int offset = (char *)pointer - (char *)intel->intelScreen->tex.map;
   int valid = (size >= 0 &&
		offset >= 0 &&
		offset + size < intel->intelScreen->tex.size);

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "intelIsAgpMemory( %p ) : %d\n", pointer, valid );
   
   return valid;
}


GLuint intelAgpOffsetFromVirtual( intelContextPtr intel, const GLvoid *pointer )
{
   int offset = (char *)pointer - (char *)intel->intelScreen->tex.map;

   if (offset < 0 || offset > intel->intelScreen->tex.size)
      return ~0;
   else
      return intel->intelScreen->tex.offset + offset;
}





/* Flip the front & back buffes
 */
void intelPageFlip( const __DRIdrawablePrivate *dPriv )
{
#if 0
   intelContextPtr intel;
   int tmp, ret;

   if (INTEL_DEBUG & DEBUG_IOCTL)
      fprintf(stderr, "%s\n", __FUNCTION__);

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   intel = (intelContextPtr) dPriv->driContextPriv->driverPrivate;

   intelFlush( &intel->ctx );
   LOCK_HARDWARE( intel );

   if (dPriv->pClipRects) {
      *(drm_clip_rect_t *)intel->sarea->boxes = dPriv->pClipRects[0];
      intel->sarea->nbox = 1;
   }

   ret = drmCommandNone(intel->driFd, DRM_I830_FLIP); 
   if (ret) {
      fprintf(stderr, "%s: %d\n", __FUNCTION__, ret);
      UNLOCK_HARDWARE( intel );
      exit(1);
   }

   tmp = intel->sarea->last_enqueue;
   intelRefillBatchLocked( intel );
   UNLOCK_HARDWARE( intel );


   intelSetDrawBuffer( &intel->ctx, intel->ctx.Color.DriverDrawBuffer );
#endif
}
