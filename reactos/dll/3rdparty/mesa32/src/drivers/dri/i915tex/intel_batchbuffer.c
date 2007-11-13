/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_batchbuffer.h"
#include "intel_ioctl.h"

/* Relocations in kernel space:
 *    - pass dma buffer seperately
 *    - memory manager knows how to patch
 *    - pass list of dependent buffers
 *    - pass relocation list
 *
 * Either:
 *    - get back an offset for buffer to fire
 *    - memory manager knows how to fire buffer
 *
 * Really want the buffer to be AGP and pinned.
 *
 */

/* Cliprect fence: The highest fence protecting a dma buffer
 * containing explicit cliprect information.  Like the old drawable
 * lock but irq-driven.  X server must wait for this fence to expire
 * before changing cliprects [and then doing sw rendering?].  For
 * other dma buffers, the scheduler will grab current cliprect info
 * and mix into buffer.  X server must hold the lock while changing
 * cliprects???  Make per-drawable.  Need cliprects in shared memory
 * -- beats storing them with every cmd buffer in the queue.
 *
 * ==> X server must wait for this fence to expire before touching the
 * framebuffer with new cliprects.
 *
 * ==> Cliprect-dependent buffers associated with a
 * cliprect-timestamp.  All of the buffers associated with a timestamp
 * must go to hardware before any buffer with a newer timestamp.
 *
 * ==> Dma should be queued per-drawable for correct X/GL
 * synchronization.  Or can fences be used for this?
 *
 * Applies to: Blit operations, metaops, X server operations -- X
 * server automatically waits on its own dma to complete before
 * modifying cliprects ???
 */

static void
intel_dump_batchbuffer(GLuint offset, GLuint * ptr, GLuint count)
{
   int i;
   fprintf(stderr, "\n\n\nSTART BATCH (%d dwords):\n", count / 4);
   for (i = 0; i < count / 4; i += 4)
      fprintf(stderr, "0x%x:\t0x%08x 0x%08x 0x%08x 0x%08x\n",
              offset + i * 4, ptr[i], ptr[i + 1], ptr[i + 2], ptr[i + 3]);
   fprintf(stderr, "END BATCH\n\n\n");
}

void
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{

   int i;

   /*
    * Get a new, free batchbuffer.
    */

   batch->size =  batch->intel->intelScreen->maxBatchSize;
   driBOData(batch->buffer, batch->size, NULL, 0);

   driBOResetList(&batch->list);

   /*
    * Unreference buffers previously on the relocation list.
    */

   for (i = 0; i < batch->nr_relocs; i++) {
      struct buffer_reloc *r = &batch->reloc[i];
      driBOUnReference(r->buf);
   }

   batch->list_count = 0;
   batch->nr_relocs = 0;
   batch->flags = 0;

   /*
    * We don't refcount the batchbuffer itself since we can't destroy it
    * while it's on the list.
    */


   driBOAddListItem(&batch->list, batch->buffer,
                    DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_EXE,
                    DRM_BO_MASK_MEM | DRM_BO_FLAG_EXE);


   batch->map = driBOMap(batch->buffer, DRM_BO_FLAG_WRITE, 0);
   batch->ptr = batch->map;
}

/*======================================================================
 * Public functions
 */
struct intel_batchbuffer *
intel_batchbuffer_alloc(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;

   driGenBuffers(intel->intelScreen->batchPool, "batchbuffer", 1,
                 &batch->buffer, 4096,
                 DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_EXE, 0);
   batch->last_fence = NULL;
   driBOCreateList(20, &batch->list);
   intel_batchbuffer_reset(batch);
   return batch;
}

void
intel_batchbuffer_free(struct intel_batchbuffer *batch)
{
   if (batch->last_fence) {
      driFenceFinish(batch->last_fence,
      DRM_FENCE_TYPE_EXE | DRM_I915_FENCE_TYPE_RW, GL_FALSE);
      driFenceUnReference(batch->last_fence);
      batch->last_fence = NULL;
   }
   if (batch->map) {
      driBOUnmap(batch->buffer);
      batch->map = NULL;
   }
   driBOUnReference(batch->buffer);
   batch->buffer = NULL;
   free(batch);
}

/* TODO: Push this whole function into bufmgr.
 */
static void
do_flush_locked(struct intel_batchbuffer *batch,
                GLuint used,
                GLboolean ignore_cliprects, GLboolean allow_unlock)
{
   GLuint *ptr;
   GLuint i;
   struct intel_context *intel = batch->intel;
   unsigned fenceFlags;
   struct _DriFenceObject *fo;

   driBOValidateList(batch->intel->driFd, &batch->list);

   /* Apply the relocations.  This nasty map indicates to me that the
    * whole task should be done internally by the memory manager, and
    * that dma buffers probably need to be pinned within agp space.
    */
   ptr = (GLuint *) driBOMap(batch->buffer, DRM_BO_FLAG_WRITE,
                             DRM_BO_HINT_ALLOW_UNFENCED_MAP);


   for (i = 0; i < batch->nr_relocs; i++) {
      struct buffer_reloc *r = &batch->reloc[i];

      ptr[r->offset / 4] = driBOOffset(r->buf) + r->delta;
   }

   if (INTEL_DEBUG & DEBUG_BATCH)
      intel_dump_batchbuffer(0, ptr, used);

   driBOUnmap(batch->buffer);
   batch->map = NULL;

   /* Throw away non-effective packets.  Won't work once we have
    * hardware contexts which would preserve statechanges beyond a
    * single buffer.
    */

   if (!(intel->numClipRects == 0 && !ignore_cliprects)) {
      intel_batch_ioctl(batch->intel,
                        driBOOffset(batch->buffer),
                        used, ignore_cliprects, allow_unlock);
   }


   /*
    * Kernel fencing. The flags tells the kernel that we've 
    * programmed an MI_FLUSH.
    */
   
   fenceFlags = DRM_I915_FENCE_FLAG_FLUSHED;
   fo = driFenceBuffers(batch->intel->driFd,
			"Batch fence", fenceFlags);

   /*
    * User space fencing.
    */

   driBOFence(batch->buffer, fo);

   if (driFenceType(fo) == DRM_FENCE_TYPE_EXE) {

     /*
      * Oops. We only validated a batch buffer. This means we
      * didn't do any proper rendering. Discard this fence object.
      */

      driFenceUnReference(fo);
   } else {
      driFenceUnReference(batch->last_fence);
      batch->last_fence = fo;
      for (i = 0; i < batch->nr_relocs; i++) {
	struct buffer_reloc *r = &batch->reloc[i];
	driBOFence(r->buf, fo);
      }
   }

   if (intel->numClipRects == 0 && !ignore_cliprects) {
      if (allow_unlock) {
         UNLOCK_HARDWARE(intel);
         sched_yield();
         LOCK_HARDWARE(intel);
      }
      intel->vtbl.lost_hardware(intel);
   }
}


struct _DriFenceObject *
intel_batchbuffer_flush(struct intel_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;
   GLboolean was_locked = intel->locked;

   if (used == 0)
      return batch->last_fence;

   /* Add the MI_BATCH_BUFFER_END.  Always add an MI_FLUSH - this is a
    * performance drain that we would like to avoid.
    */
   if (used & 4) {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = 0;
      ((int *) batch->ptr)[2] = MI_BATCH_BUFFER_END;
      used += 12;
   }
   else {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = MI_BATCH_BUFFER_END;
      used += 8;
   }

   driBOUnmap(batch->buffer);
   batch->ptr = NULL;
   batch->map = NULL;

   /* TODO: Just pass the relocation list and dma buffer up to the
    * kernel.
    */
   if (!was_locked)
      LOCK_HARDWARE(intel);

   do_flush_locked(batch, used, !(batch->flags & INTEL_BATCH_CLIPRECTS),
		   GL_FALSE);

   if (!was_locked)
      UNLOCK_HARDWARE(intel);

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(batch);
   return batch->last_fence;
}

void
intel_batchbuffer_finish(struct intel_batchbuffer *batch)
{
   struct _DriFenceObject *fence = intel_batchbuffer_flush(batch);
   driFenceReference(fence);
   driFenceFinish(fence, 3, GL_FALSE);
   driFenceUnReference(fence);
}


/*  This is the only way buffers get added to the validate list.
 */
GLboolean
intel_batchbuffer_emit_reloc(struct intel_batchbuffer *batch,
                             struct _DriBufferObject *buffer,
                             GLuint flags, GLuint mask, GLuint delta)
{
   assert(batch->nr_relocs < MAX_RELOCS);

   driBOAddListItem(&batch->list, buffer, flags, mask);

   {
      struct buffer_reloc *r = &batch->reloc[batch->nr_relocs++];
      driBOReference(buffer);
      r->buf = buffer;
      r->offset = batch->ptr - batch->map;
      r->delta = delta;
   }

   batch->ptr += 4;
   return GL_TRUE;
}



void
intel_batchbuffer_data(struct intel_batchbuffer *batch,
                       const void *data, GLuint bytes, GLuint flags)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes, flags);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}
