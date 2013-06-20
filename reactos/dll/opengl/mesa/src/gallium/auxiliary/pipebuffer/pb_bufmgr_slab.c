/**************************************************************************
 *
 * Copyright 2006-2008 Tungsten Graphics, Inc., Cedar Park, TX., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, FREE of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/

/**
 * @file
 * S-lab pool implementation.
 * 
 * @sa http://en.wikipedia.org/wiki/Slab_allocation
 * 
 * @author Thomas Hellstrom <thomas-at-tungstengraphics-dot-com>
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "os/os_thread.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"
#include "util/u_time.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


struct pb_slab;


/**
 * Buffer in a slab.
 * 
 * Sub-allocation of a contiguous buffer.
 */
struct pb_slab_buffer
{
   struct pb_buffer base;
   
   struct pb_slab *slab;
   
   struct list_head head;
   
   unsigned mapCount;
   
   /** Offset relative to the start of the slab buffer. */
   pb_size start;
   
   /** Use when validating, to signal that all mappings are finished */
   /* TODO: Actually validation does not reach this stage yet */
   pipe_condvar event;
};


/**
 * Slab -- a contiguous piece of memory. 
 */
struct pb_slab
{
   struct list_head head;
   struct list_head freeBuffers;
   pb_size numBuffers;
   pb_size numFree;
   
   struct pb_slab_buffer *buffers;
   struct pb_slab_manager *mgr;
   
   /** Buffer from the provider */
   struct pb_buffer *bo;
   
   void *virtual;   
};


/**
 * It adds/removes slabs as needed in order to meet the allocation/destruction 
 * of individual buffers.
 */
struct pb_slab_manager 
{
   struct pb_manager base;
   
   /** From where we get our buffers */
   struct pb_manager *provider;
   
   /** Size of the buffers we hand on downstream */
   pb_size bufSize;
   
   /** Size of the buffers we request upstream */
   pb_size slabSize;
   
   /** 
    * Alignment, usage to be used to allocate the slab buffers.
    * 
    * We can only provide buffers which are consistent (in alignment, usage) 
    * with this description.   
    */
   struct pb_desc desc;

   /** 
    * Partial slabs
    * 
    * Full slabs are not stored in any list. Empty slabs are destroyed 
    * immediatly.
    */
   struct list_head slabs;
   
   pipe_mutex mutex;
};


/**
 * Wrapper around several slabs, therefore capable of handling buffers of 
 * multiple sizes. 
 * 
 * This buffer manager just dispatches buffer allocations to the appropriate slab
 * manager, according to the requested buffer size, or by passes the slab 
 * managers altogether for even greater sizes.
 * 
 * The data of this structure remains constant after
 * initialization and thus needs no mutex protection.
 */
struct pb_slab_range_manager 
{
   struct pb_manager base;

   struct pb_manager *provider;
   
   pb_size minBufSize;
   pb_size maxBufSize;
   
   /** @sa pb_slab_manager::desc */ 
   struct pb_desc desc;
   
   unsigned numBuckets;
   pb_size *bucketSizes;
   
   /** Array of pb_slab_manager, one for each bucket size */
   struct pb_manager **buckets;
};


static INLINE struct pb_slab_buffer *
pb_slab_buffer(struct pb_buffer *buf)
{
   assert(buf);
   return (struct pb_slab_buffer *)buf;
}


static INLINE struct pb_slab_manager *
pb_slab_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_slab_manager *)mgr;
}


static INLINE struct pb_slab_range_manager *
pb_slab_range_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_slab_range_manager *)mgr;
}


/**
 * Delete a buffer from the slab delayed list and put
 * it on the slab FREE list.
 */
static void
pb_slab_buffer_destroy(struct pb_buffer *_buf)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   struct pb_slab *slab = buf->slab;
   struct pb_slab_manager *mgr = slab->mgr;
   struct list_head *list = &buf->head;

   pipe_mutex_lock(mgr->mutex);
   
   assert(!pipe_is_referenced(&buf->base.reference));
   
   buf->mapCount = 0;

   LIST_DEL(list);
   LIST_ADDTAIL(list, &slab->freeBuffers);
   slab->numFree++;

   if (slab->head.next == &slab->head)
      LIST_ADDTAIL(&slab->head, &mgr->slabs);

   /* If the slab becomes totally empty, free it */
   if (slab->numFree == slab->numBuffers) {
      list = &slab->head;
      LIST_DELINIT(list);
      pb_reference(&slab->bo, NULL);
      FREE(slab->buffers);
      FREE(slab);
   }

   pipe_mutex_unlock(mgr->mutex);
}


static void *
pb_slab_buffer_map(struct pb_buffer *_buf, 
                   unsigned flags,
                   void *flush_ctx)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);

   /* XXX: it will be necessary to remap here to propagate flush_ctx */

   ++buf->mapCount;
   return (void *) ((uint8_t *) buf->slab->virtual + buf->start);
}


static void
pb_slab_buffer_unmap(struct pb_buffer *_buf)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);

   --buf->mapCount;
   if (buf->mapCount == 0) 
       pipe_condvar_broadcast(buf->event);
}


static enum pipe_error 
pb_slab_buffer_validate(struct pb_buffer *_buf, 
                         struct pb_validate *vl,
                         unsigned flags)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   return pb_validate(buf->slab->bo, vl, flags);
}


static void
pb_slab_buffer_fence(struct pb_buffer *_buf, 
                      struct pipe_fence_handle *fence)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   pb_fence(buf->slab->bo, fence);
}


static void
pb_slab_buffer_get_base_buffer(struct pb_buffer *_buf,
                               struct pb_buffer **base_buf,
                               pb_size *offset)
{
   struct pb_slab_buffer *buf = pb_slab_buffer(_buf);
   pb_get_base_buffer(buf->slab->bo, base_buf, offset);
   *offset += buf->start;
}


static const struct pb_vtbl 
pb_slab_buffer_vtbl = {
      pb_slab_buffer_destroy,
      pb_slab_buffer_map,
      pb_slab_buffer_unmap,
      pb_slab_buffer_validate,
      pb_slab_buffer_fence,
      pb_slab_buffer_get_base_buffer
};


/**
 * Create a new slab.
 * 
 * Called when we ran out of free slabs.
 */
static enum pipe_error
pb_slab_create(struct pb_slab_manager *mgr)
{
   struct pb_slab *slab;
   struct pb_slab_buffer *buf;
   unsigned numBuffers;
   unsigned i;
   enum pipe_error ret;

   slab = CALLOC_STRUCT(pb_slab);
   if (!slab)
      return PIPE_ERROR_OUT_OF_MEMORY;

   slab->bo = mgr->provider->create_buffer(mgr->provider, mgr->slabSize, &mgr->desc);
   if(!slab->bo) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto out_err0;
   }

   /* Note down the slab virtual address. All mappings are accessed directly 
    * through this address so it is required that the buffer is pinned. */
   slab->virtual = pb_map(slab->bo, 
                          PB_USAGE_CPU_READ |
                          PB_USAGE_CPU_WRITE, NULL);
   if(!slab->virtual) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto out_err1;
   }
   pb_unmap(slab->bo);

   numBuffers = slab->bo->size / mgr->bufSize;

   slab->buffers = CALLOC(numBuffers, sizeof(*slab->buffers));
   if (!slab->buffers) {
      ret = PIPE_ERROR_OUT_OF_MEMORY;
      goto out_err1;
   }

   LIST_INITHEAD(&slab->head);
   LIST_INITHEAD(&slab->freeBuffers);
   slab->numBuffers = numBuffers;
   slab->numFree = 0;
   slab->mgr = mgr;

   buf = slab->buffers;
   for (i=0; i < numBuffers; ++i) {
      pipe_reference_init(&buf->base.reference, 0);
      buf->base.size = mgr->bufSize;
      buf->base.alignment = 0;
      buf->base.usage = 0;
      buf->base.vtbl = &pb_slab_buffer_vtbl;
      buf->slab = slab;
      buf->start = i* mgr->bufSize;
      buf->mapCount = 0;
      pipe_condvar_init(buf->event);
      LIST_ADDTAIL(&buf->head, &slab->freeBuffers);
      slab->numFree++;
      buf++;
   }

   /* Add this slab to the list of partial slabs */
   LIST_ADDTAIL(&slab->head, &mgr->slabs);

   return PIPE_OK;

out_err1: 
   pb_reference(&slab->bo, NULL);
out_err0: 
   FREE(slab);
   return ret;
}


static struct pb_buffer *
pb_slab_manager_create_buffer(struct pb_manager *_mgr,
                              pb_size size,
                              const struct pb_desc *desc)
{
   struct pb_slab_manager *mgr = pb_slab_manager(_mgr);
   static struct pb_slab_buffer *buf;
   struct pb_slab *slab;
   struct list_head *list;

   /* check size */
   assert(size <= mgr->bufSize);
   if(size > mgr->bufSize)
      return NULL;
   
   /* check if we can provide the requested alignment */
   assert(pb_check_alignment(desc->alignment, mgr->desc.alignment));
   if(!pb_check_alignment(desc->alignment, mgr->desc.alignment))
      return NULL;
   assert(pb_check_alignment(desc->alignment, mgr->bufSize));
   if(!pb_check_alignment(desc->alignment, mgr->bufSize))
      return NULL;

   assert(pb_check_usage(desc->usage, mgr->desc.usage));
   if(!pb_check_usage(desc->usage, mgr->desc.usage))
      return NULL;

   pipe_mutex_lock(mgr->mutex);
   
   /* Create a new slab, if we run out of partial slabs */
   if (mgr->slabs.next == &mgr->slabs) {
      (void) pb_slab_create(mgr);
      if (mgr->slabs.next == &mgr->slabs) {
	 pipe_mutex_unlock(mgr->mutex);
	 return NULL;
      }
   }
   
   /* Allocate the buffer from a partial (or just created) slab */
   list = mgr->slabs.next;
   slab = LIST_ENTRY(struct pb_slab, list, head);
   
   /* If totally full remove from the partial slab list */
   if (--slab->numFree == 0)
      LIST_DELINIT(list);

   list = slab->freeBuffers.next;
   LIST_DELINIT(list);

   pipe_mutex_unlock(mgr->mutex);
   buf = LIST_ENTRY(struct pb_slab_buffer, list, head);
   
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = desc->alignment;
   buf->base.usage = desc->usage;
   
   return &buf->base;
}


static void
pb_slab_manager_flush(struct pb_manager *_mgr)
{
   struct pb_slab_manager *mgr = pb_slab_manager(_mgr);

   assert(mgr->provider->flush);
   if(mgr->provider->flush)
      mgr->provider->flush(mgr->provider);
}


static void
pb_slab_manager_destroy(struct pb_manager *_mgr)
{
   struct pb_slab_manager *mgr = pb_slab_manager(_mgr);

   /* TODO: cleanup all allocated buffers */
   FREE(mgr);
}


struct pb_manager *
pb_slab_manager_create(struct pb_manager *provider,
                       pb_size bufSize,
                       pb_size slabSize,
                       const struct pb_desc *desc)
{
   struct pb_slab_manager *mgr;

   mgr = CALLOC_STRUCT(pb_slab_manager);
   if (!mgr)
      return NULL;

   mgr->base.destroy = pb_slab_manager_destroy;
   mgr->base.create_buffer = pb_slab_manager_create_buffer;
   mgr->base.flush = pb_slab_manager_flush;

   mgr->provider = provider;
   mgr->bufSize = bufSize;
   mgr->slabSize = slabSize;
   mgr->desc = *desc;

   LIST_INITHEAD(&mgr->slabs);
   
   pipe_mutex_init(mgr->mutex);

   return &mgr->base;
}


static struct pb_buffer *
pb_slab_range_manager_create_buffer(struct pb_manager *_mgr,
                                    pb_size size,
                                    const struct pb_desc *desc)
{
   struct pb_slab_range_manager *mgr = pb_slab_range_manager(_mgr);
   pb_size bufSize;
   pb_size reqSize = size;
   unsigned i;

   if(desc->alignment > reqSize)
	   reqSize = desc->alignment;

   bufSize = mgr->minBufSize;
   for (i = 0; i < mgr->numBuckets; ++i) {
      if(bufSize >= reqSize)
	 return mgr->buckets[i]->create_buffer(mgr->buckets[i], size, desc);
      bufSize *= 2;
   }

   /* Fall back to allocate a buffer object directly from the provider. */
   return mgr->provider->create_buffer(mgr->provider, size, desc);
}


static void
pb_slab_range_manager_flush(struct pb_manager *_mgr)
{
   struct pb_slab_range_manager *mgr = pb_slab_range_manager(_mgr);

   /* Individual slabs don't hold any temporary buffers so no need to call them */
   
   assert(mgr->provider->flush);
   if(mgr->provider->flush)
      mgr->provider->flush(mgr->provider);
}


static void
pb_slab_range_manager_destroy(struct pb_manager *_mgr)
{
   struct pb_slab_range_manager *mgr = pb_slab_range_manager(_mgr);
   unsigned i;
   
   for (i = 0; i < mgr->numBuckets; ++i)
      mgr->buckets[i]->destroy(mgr->buckets[i]);
   FREE(mgr->buckets);
   FREE(mgr->bucketSizes);
   FREE(mgr);
}


struct pb_manager *
pb_slab_range_manager_create(struct pb_manager *provider,
                             pb_size minBufSize,
                             pb_size maxBufSize,
                             pb_size slabSize,
                             const struct pb_desc *desc)
{
   struct pb_slab_range_manager *mgr;
   pb_size bufSize;
   unsigned i;

   if(!provider)
      return NULL;
   
   mgr = CALLOC_STRUCT(pb_slab_range_manager);
   if (!mgr)
      goto out_err0;

   mgr->base.destroy = pb_slab_range_manager_destroy;
   mgr->base.create_buffer = pb_slab_range_manager_create_buffer;
   mgr->base.flush = pb_slab_range_manager_flush;

   mgr->provider = provider;
   mgr->minBufSize = minBufSize;
   mgr->maxBufSize = maxBufSize;

   mgr->numBuckets = 1;
   bufSize = minBufSize;
   while(bufSize < maxBufSize) {
      bufSize *= 2;
      ++mgr->numBuckets;
   }
   
   mgr->buckets = CALLOC(mgr->numBuckets, sizeof(*mgr->buckets));
   if (!mgr->buckets)
      goto out_err1;

   bufSize = minBufSize;
   for (i = 0; i < mgr->numBuckets; ++i) {
      mgr->buckets[i] = pb_slab_manager_create(provider, bufSize, slabSize, desc);
      if(!mgr->buckets[i])
	 goto out_err2;
      bufSize *= 2;
   }

   return &mgr->base;

out_err2: 
   for (i = 0; i < mgr->numBuckets; ++i)
      if(mgr->buckets[i])
	    mgr->buckets[i]->destroy(mgr->buckets[i]);
   FREE(mgr->buckets);
out_err1: 
   FREE(mgr);
out_err0:
   return NULL;
}
