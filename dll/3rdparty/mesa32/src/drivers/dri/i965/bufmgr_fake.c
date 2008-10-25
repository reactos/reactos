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

/* Originally a fake version of the buffer manager so that we can
 * prototype the changes in a driver fairly quickly, has been fleshed
 * out to a fully functional interim solution.
 *
 * Basically wraps the old style memory management in the new
 * programming interface, but is more expressive and avoids many of
 * the bugs in the old texture manager.
 */
#include "bufmgr.h"

#include "intel_context.h"
#include "intel_ioctl.h"
#include "intel_batchbuffer.h"

#include "simple_list.h"
#include "mm.h"
#include "imports.h"

#define BM_POOL_MAX 8

/* Internal flags:
 */
#define BM_NO_BACKING_STORE   0x2000
#define BM_NO_FENCE_SUBDATA   0x4000


static int check_fenced( struct intel_context *intel );

static int nr_attach = 0;

/* Wrapper around mm.c's mem_block, which understands that you must
 * wait for fences to expire before memory can be freed.  This is
 * specific to our use of memcpy for uploads - an upload that was
 * processed through the command queue wouldn't need to care about
 * fences.
 */
struct block {
   struct block *next, *prev;
   struct pool *pool;		/* BM_MEM_AGP */
   struct mem_block *mem;	/* BM_MEM_AGP */

   unsigned referenced:1;
   unsigned on_hardware:1;
   unsigned fenced:1;	
   

   unsigned fence;		/* BM_MEM_AGP, Split to read_fence, write_fence */

   struct buffer *buf;
   void *virtual;
};


struct buffer {
   unsigned id;			/* debug only */
   const char *name;
   unsigned size;

   unsigned mapped:1;		
   unsigned dirty:1;		
   unsigned aub_dirty:1;	
   unsigned alignment:13;
   unsigned flags:16;

   struct block *block;
   void *backing_store;
   void (*invalidate_cb)( struct intel_context *, void * );
   void *invalidate_ptr;
};

struct pool {
   unsigned size;
   unsigned low_offset;
   struct buffer *static_buffer;
   unsigned flags;
   struct mem_block *heap;
   void *virtual;
   struct block lru;		/* only allocated, non-fence-pending blocks here */
};

struct bufmgr {
   _glthread_Mutex mutex;	/**< for thread safety */
   struct pool pool[BM_POOL_MAX];
   unsigned nr_pools;

   unsigned buf_nr;		/* for generating ids */

   struct block referenced;	/* after bmBufferOffset */
   struct block on_hardware;	/* after bmValidateBuffers */
   struct block fenced;		/* after bmFenceBuffers (mi_flush, emit irq, write dword) */
                                /* then to pool->lru or free() */

   unsigned ctxId;
   unsigned last_fence;
   unsigned free_on_hardware;

   unsigned fail:1;
   unsigned need_fence:1;
};

#define MAXFENCE 0x7fffffff

static GLboolean FENCE_LTE( unsigned a, unsigned b )
{
   if (a == b)
      return GL_TRUE;

   if (a < b && b - a < (1<<24))
      return GL_TRUE;

   if (a > b && MAXFENCE - a + b < (1<<24))
      return GL_TRUE;

   return GL_FALSE;
}

int bmTestFence( struct intel_context *intel, unsigned fence )
{
   /* Slight problem with wrap-around:
    */
   return fence == 0 || FENCE_LTE(fence, intel->sarea->last_dispatch);
}

#define LOCK(bm) \
  int dolock = nr_attach > 1; \
  if (dolock) _glthread_LOCK_MUTEX(bm->mutex)

#define UNLOCK(bm) \
  if (dolock) _glthread_UNLOCK_MUTEX(bm->mutex)



static GLboolean alloc_from_pool( struct intel_context *intel,				
				  unsigned pool_nr,
				  struct buffer *buf )
{
   struct bufmgr *bm = intel->bm;
   struct pool *pool = &bm->pool[pool_nr];
   struct block *block = (struct block *)calloc(sizeof *block, 1);
   GLuint sz, align = (1<<buf->alignment);

   if (!block)
      return GL_FALSE;

   sz = (buf->size + align-1) & ~(align-1);

   block->mem = mmAllocMem(pool->heap, 
			   sz, 
			   buf->alignment, 0);
   if (!block->mem) {
      free(block);
      return GL_FALSE;
   }

   make_empty_list(block);

   /* Insert at head or at tail???   
    */
   insert_at_tail(&pool->lru, block);

   block->pool = pool;
   block->virtual = pool->virtual + block->mem->ofs;
   block->buf = buf;

   buf->block = block;

   return GL_TRUE;
}








/* Release the card storage associated with buf:
 */
static void free_block( struct intel_context *intel, struct block *block )
{
   DBG("free block %p\n", block);

   if (!block) 
      return;

   check_fenced(intel);

   if (block->referenced) {
      _mesa_printf("tried to free block on referenced list\n");
      assert(0);
   }
   else if (block->on_hardware) {
      block->buf = NULL;
      intel->bm->free_on_hardware += block->mem->size;
   }
   else if (block->fenced) {
      block->buf = NULL;
   }
   else {
      DBG("    - free immediately\n");
      remove_from_list(block);

      mmFreeMem(block->mem);
      free(block);
   }
}


static void alloc_backing_store( struct intel_context *intel, struct buffer *buf )
{
   assert(!buf->backing_store);
   assert(!(buf->flags & (BM_NO_EVICT|BM_NO_BACKING_STORE)));

   buf->backing_store = ALIGN_MALLOC(buf->size, 64);
}

static void free_backing_store( struct intel_context *intel, struct buffer *buf )
{
   assert(!(buf->flags & (BM_NO_EVICT|BM_NO_BACKING_STORE)));
	  
   if (buf->backing_store) {
      ALIGN_FREE(buf->backing_store);
      buf->backing_store = NULL;
   }
}






static void set_dirty( struct intel_context *intel,
			      struct buffer *buf )
{
   if (buf->flags & BM_NO_BACKING_STORE)
      buf->invalidate_cb(intel, buf->invalidate_ptr);

   assert(!(buf->flags & BM_NO_EVICT));

   DBG("set_dirty - buf %d\n", buf->id);
   buf->dirty = 1;
}


static int evict_lru( struct intel_context *intel, GLuint max_fence, GLuint *pool )
{
   struct bufmgr *bm = intel->bm;
   struct block *block, *tmp;
   int i;

   DBG("%s\n", __FUNCTION__);

   for (i = 0; i < bm->nr_pools; i++) {
      if (!(bm->pool[i].flags & BM_NO_EVICT)) {
	 foreach_s(block, tmp, &bm->pool[i].lru) {

	    if (block->buf &&
		(block->buf->flags & BM_NO_FENCE_SUBDATA))
	       continue;

	    if (block->fence && max_fence &&
		!FENCE_LTE(block->fence, max_fence))
	       return 0;

	    set_dirty(intel, block->buf);
	    block->buf->block = NULL;

	    free_block(intel, block);
	    *pool = i;
	    return 1;
	 }
      }
   }


   return 0;
}


#define foreach_s_rev(ptr, t, list)   \
        for(ptr=(list)->prev,t=(ptr)->prev; list != ptr; ptr=t, t=(t)->prev)

static int evict_mru( struct intel_context *intel, GLuint *pool )
{
   struct bufmgr *bm = intel->bm;
   struct block *block, *tmp;
   int i;

   DBG("%s\n", __FUNCTION__);

   for (i = 0; i < bm->nr_pools; i++) {
      if (!(bm->pool[i].flags & BM_NO_EVICT)) {
	 foreach_s_rev(block, tmp, &bm->pool[i].lru) {

	    if (block->buf &&
		(block->buf->flags & BM_NO_FENCE_SUBDATA))
	       continue;

	    set_dirty(intel, block->buf);
	    block->buf->block = NULL;

	    free_block(intel, block);
	    *pool = i;
	    return 1;
	 }
      }
   }


   return 0;
}


static int check_fenced( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;
   struct block *block, *tmp;
   int ret = 0;

   foreach_s(block, tmp, &bm->fenced ) {
      assert(block->fenced);

      if (bmTestFence(intel, block->fence)) {

	 block->fenced = 0;

	 if (!block->buf) {
	    DBG("delayed free: offset %x sz %x\n", block->mem->ofs, block->mem->size);
	    remove_from_list(block);
	    mmFreeMem(block->mem);
	    free(block);
	 }
	 else {
	    DBG("return to lru: offset %x sz %x\n", block->mem->ofs, block->mem->size);
	    move_to_tail(&block->pool->lru, block);
	 }

	 ret = 1;
      }
      else {
	 /* Blocks are ordered by fence, so if one fails, all from
	  * here will fail also:
	  */
	 break;
      }
   }

   /* Also check the referenced list: 
    */
   foreach_s(block, tmp, &bm->referenced ) {
      if (block->fenced &&
	  bmTestFence(intel, block->fence)) {
	 block->fenced = 0;
      }
   }

   
   DBG("%s: %d\n", __FUNCTION__, ret);
   return ret;
}



static void fence_blocks( struct intel_context *intel,
			  unsigned fence )
{
   struct bufmgr *bm = intel->bm;
   struct block *block, *tmp;

   foreach_s (block, tmp, &bm->on_hardware) {
      DBG("Fence block %p (sz 0x%x buf %p) with fence %d\n", block, 
	  block->mem->size, block->buf, fence);
      block->fence = fence;

      block->on_hardware = 0;
      block->fenced = 1;

      /* Move to tail of pending list here
       */
      move_to_tail(&bm->fenced, block);
   }

   /* Also check the referenced list:
    */  
   foreach_s (block, tmp, &bm->referenced) {
      if (block->on_hardware) {
	 DBG("Fence block %p (sz 0x%x buf %p) with fence %d\n", block, 
	     block->mem->size, block->buf, fence);
	 
	 block->fence = fence;
	 block->on_hardware = 0;
	 block->fenced = 1;
      }
   }


   bm->last_fence = fence;
   assert(is_empty_list(&bm->on_hardware));
}




static GLboolean alloc_block( struct intel_context *intel,
			      struct buffer *buf )
{
   struct bufmgr *bm = intel->bm;
   int i;

   assert(intel->locked);

   DBG("%s 0x%x bytes (%s)\n", __FUNCTION__, buf->size, buf->name);

   for (i = 0; i < bm->nr_pools; i++) {
      if (!(bm->pool[i].flags & BM_NO_ALLOC) &&
	  alloc_from_pool(intel, i, buf)) {

	 DBG("%s --> 0x%x (sz %x)\n", __FUNCTION__, 
	     buf->block->mem->ofs, buf->block->mem->size);
	 
	 return GL_TRUE;
      }
   }

   DBG("%s --> fail\n", __FUNCTION__);
   return GL_FALSE;   
}


static GLboolean evict_and_alloc_block( struct intel_context *intel,
					struct buffer *buf )
{
   GLuint pool;
   struct bufmgr *bm = intel->bm;

   assert(buf->block == NULL);

   /* Put a cap on the amount of free memory we'll allow to accumulate
    * before emitting a fence.
    */
   if (bm->free_on_hardware > 1 * 1024 * 1024) {
      DBG("fence for free space: %x\n", bm->free_on_hardware);
      bmSetFence(intel);
   }

   /* Search for already free memory:
    */
   if (alloc_block(intel, buf))
      return GL_TRUE;

   /* Look for memory that may have become free: 
    */
   if (check_fenced(intel) &&
       alloc_block(intel, buf))
      return GL_TRUE;

   /* Look for memory blocks not used for >1 frame:
    */
   while (evict_lru(intel, intel->second_last_swap_fence, &pool))
      if (alloc_from_pool(intel, pool, buf))
	 return GL_TRUE;

   /* If we're not thrashing, allow lru eviction to dig deeper into
    * recently used textures.  We'll probably be thrashing soon:
    */
   if (!intel->thrashing) {
      while (evict_lru(intel, 0, &pool))
	 if (alloc_from_pool(intel, pool, buf))
	    return GL_TRUE;
   }

   /* Keep thrashing counter alive?
    */
   if (intel->thrashing)
      intel->thrashing = 20;

   /* Wait on any already pending fences - here we are waiting for any
    * freed memory that has been submitted to hardware and fenced to
    * become available:
    */
   while (!is_empty_list(&bm->fenced)) {
      GLuint fence = bm->fenced.next->fence;
      bmFinishFence(intel, fence);

      if (alloc_block(intel, buf))
	 return GL_TRUE;
   }


   /* 
    */
   if (!is_empty_list(&bm->on_hardware)) {
      bmSetFence(intel);

      while (!is_empty_list(&bm->fenced)) {
	 GLuint fence = bm->fenced.next->fence;
	 bmFinishFence(intel, fence);
      }

      if (!intel->thrashing) {	 
	 DBG("thrashing\n");
      }
      intel->thrashing = 20; 

      if (alloc_block(intel, buf))
	 return GL_TRUE;
   }

   while (evict_mru(intel, &pool))
      if (alloc_from_pool(intel, pool, buf))
	 return GL_TRUE;

   DBG("%s 0x%x bytes failed\n", __FUNCTION__, buf->size);

   assert(is_empty_list(&bm->on_hardware));
   assert(is_empty_list(&bm->fenced));

   return GL_FALSE;
}










/***********************************************************************
 * Public functions
 */


/* The initialization functions are skewed in the fake implementation.
 * This call would be to attach to an existing manager, rather than to
 * create a local one.
 */
struct bufmgr *bm_fake_intel_Attach( struct intel_context *intel )
{
   _glthread_DECLARE_STATIC_MUTEX(initMutex);   
   static struct bufmgr bm;
   
   /* This function needs a mutex of its own...
    */
   _glthread_LOCK_MUTEX(initMutex);

   if (nr_attach == 0) {
      _glthread_INIT_MUTEX(bm.mutex);

      make_empty_list(&bm.referenced);
      make_empty_list(&bm.fenced);
      make_empty_list(&bm.on_hardware);
      
      /* The context id of any of the share group.  This won't be used
       * in communication with the kernel, so it doesn't matter if
       * this context is eventually deleted.
       */
      bm.ctxId = intel->hHWContext;
   }

   nr_attach++;

   _glthread_UNLOCK_MUTEX(initMutex);

   return &bm;
}



/* The virtual pointer would go away in a true implementation.
 */
int bmInitPool( struct intel_context *intel, 
		unsigned long low_offset,
		void *low_virtual,
		unsigned long size,
		unsigned flags)
{
   struct bufmgr *bm = intel->bm;
   int retval = 0;

   LOCK(bm);
   {
      GLuint i;

      for (i = 0; i < bm->nr_pools; i++) {
	 if (bm->pool[i].low_offset == low_offset &&
	     bm->pool[i].size == size) {
	    retval = i;
	    goto out;
	 }
      }


      if (bm->nr_pools >= BM_POOL_MAX)
	 retval = -1;
      else {
	 i = bm->nr_pools++;
   
	 DBG("bmInitPool %d low_offset %x sz %x\n",
	     i, low_offset, size);
   
	 bm->pool[i].low_offset = low_offset;
	 bm->pool[i].size = size;
	 bm->pool[i].heap = mmInit( low_offset, size );
	 bm->pool[i].virtual = low_virtual - low_offset;
	 bm->pool[i].flags = flags;
   
	 make_empty_list(&bm->pool[i].lru);
	 
	 retval = i;
      }
   }
 out:
   UNLOCK(bm);
   return retval;
}

static struct buffer *do_GenBuffer(struct intel_context *intel, const char *name, int align)
{
   struct bufmgr *bm = intel->bm;
   struct buffer *buf = calloc(sizeof(*buf), 1);

   buf->id = ++bm->buf_nr;
   buf->name = name;
   buf->alignment = align;	
   buf->flags = BM_MEM_AGP|BM_MEM_VRAM|BM_MEM_LOCAL;

   return buf;
}


void *bmFindVirtual( struct intel_context *intel,
		     unsigned int offset,
		     size_t sz )
{
   struct bufmgr *bm = intel->bm;
   int i;

   for (i = 0; i < bm->nr_pools; i++)
      if (offset >= bm->pool[i].low_offset &&
	  offset + sz <= bm->pool[i].low_offset + bm->pool[i].size)
	 return bm->pool[i].virtual + offset;

   return NULL;
}
 

void bmGenBuffers(struct intel_context *intel, 
		  const char *name, unsigned n, 
		  struct buffer **buffers,
		  int align )
{
   struct bufmgr *bm = intel->bm;
   LOCK(bm);
   {
      int i;

      for (i = 0; i < n; i++)
	 buffers[i] = do_GenBuffer(intel, name, align);
   }
   UNLOCK(bm);
}


void bmDeleteBuffers(struct intel_context *intel, unsigned n, struct buffer **buffers)
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      unsigned i;
   
      for (i = 0; i < n; i++) {
	 struct buffer *buf = buffers[i];

	 if (buf && buf->block)
	    free_block(intel, buf->block);

	 if (buf) 
	    free(buf);	 
      }
   }
   UNLOCK(bm);
}




/* Hook to inform faked buffer manager about fixed-position
 * front,depth,back buffers.  These may move to a fully memory-managed
 * scheme, or they may continue to be managed as is.  It will probably
 * be useful to pass a fixed offset here one day.
 */
struct buffer *bmGenBufferStatic(struct intel_context *intel,
				 unsigned pool )
{
   struct bufmgr *bm = intel->bm;
   struct buffer *buf;
   LOCK(bm);
   {
      assert(bm->pool[pool].flags & BM_NO_EVICT);
      assert(bm->pool[pool].flags & BM_NO_MOVE);

      if (bm->pool[pool].static_buffer)
	 buf = bm->pool[pool].static_buffer;
      else {
	 buf = do_GenBuffer(intel, "static", 12);
   
	 bm->pool[pool].static_buffer = buf;
	 assert(!buf->block);

	 buf->size = bm->pool[pool].size;
	 buf->flags = bm->pool[pool].flags;
	 buf->alignment = 12;
	 
	 if (!alloc_from_pool(intel, pool, buf))
	    assert(0);
      }
   }
   UNLOCK(bm);
   return buf;
}


static void wait_quiescent(struct intel_context *intel,
			   struct block *block)
{
   if (block->on_hardware) {
      assert(intel->bm->need_fence);
      bmSetFence(intel);
      assert(!block->on_hardware);
   }


   if (block->fenced) {
      bmFinishFence(intel, block->fence);
   }

   assert(!block->on_hardware);
   assert(!block->fenced);
}



/* If buffer size changes, free and reallocate.  Otherwise update in
 * place.
 */
int bmBufferData(struct intel_context *intel, 
		 struct buffer *buf, 
		 unsigned size, 
		 const void *data, 
		 unsigned flags )
{
   struct bufmgr *bm = intel->bm;
   int retval = 0;

   LOCK(bm);
   {
      DBG("bmBufferData %d sz 0x%x data: %p\n", buf->id, size, data);

      assert(!buf->mapped);

      if (buf->block) {
	 struct block *block = buf->block;

	 /* Optimistic check to see if we can reuse the block -- not
	  * required for correctness:
	  */
	 if (block->fenced)
	    check_fenced(intel);

	 if (block->on_hardware ||
	     block->fenced ||
	     (buf->size && buf->size != size) || 
	     (data == NULL)) {

	    assert(!block->referenced);

	    free_block(intel, block);
	    buf->block = NULL;
	    buf->dirty = 1;
	 }
      }

      buf->size = size;
      if (buf->block) {
	 assert (buf->block->mem->size >= size);
      }

      if (buf->flags & (BM_NO_BACKING_STORE|BM_NO_EVICT)) {

	 assert(intel->locked || data == NULL);

	 if (data != NULL) {
	    if (!buf->block && !evict_and_alloc_block(intel, buf)) {
	       bm->fail = 1;
	       retval = -1;
	       goto out;
	    }

	    wait_quiescent(intel, buf->block);

	    DBG("bmBufferData %d offset 0x%x sz 0x%x\n", 
		buf->id, buf->block->mem->ofs, size);

	    assert(buf->block->virtual == buf->block->pool->virtual + buf->block->mem->ofs);

	    do_memcpy(buf->block->virtual, data, size);
	 }
	 buf->dirty = 0;
      }
      else {
	       DBG("%s - set buf %d dirty\n", __FUNCTION__, buf->id);
	 set_dirty(intel, buf);
	 free_backing_store(intel, buf);
   
	 if (data != NULL) {      
	    alloc_backing_store(intel, buf);
	    do_memcpy(buf->backing_store, data, size);
	 }
      }
   }
 out:
   UNLOCK(bm);
   return retval;
}


/* Update the buffer in place, in whatever space it is currently resident:
 */
int bmBufferSubData(struct intel_context *intel, 
		     struct buffer *buf, 
		     unsigned offset, 
		     unsigned size, 
		     const void *data )
{
   struct bufmgr *bm = intel->bm;
   int retval = 0;

   if (size == 0) 
      return 0;

   LOCK(bm); 
   {
      DBG("bmBufferSubdata %d offset 0x%x sz 0x%x\n", buf->id, offset, size);
      
      assert(offset+size <= buf->size);

      if (buf->flags & (BM_NO_EVICT|BM_NO_BACKING_STORE)) {

	 assert(intel->locked);

	 if (!buf->block && !evict_and_alloc_block(intel, buf)) {
	    bm->fail = 1;
	    retval = -1;
	    goto out;
	 }
	 
	 if (!(buf->flags & BM_NO_FENCE_SUBDATA))
	    wait_quiescent(intel, buf->block);

	 buf->dirty = 0;

	 do_memcpy(buf->block->virtual + offset, data, size);
      }
      else {
	 DBG("%s - set buf %d dirty\n", __FUNCTION__, buf->id);
	 set_dirty(intel, buf);

	 if (buf->backing_store == NULL)
	    alloc_backing_store(intel, buf);

	 do_memcpy(buf->backing_store + offset, data, size); 
      }
   }
 out:
   UNLOCK(bm);
   return retval;
}



int bmBufferDataAUB(struct intel_context *intel, 
		     struct buffer *buf, 
		     unsigned size, 
		     const void *data, 
		     unsigned flags,
		     unsigned aubtype,
		     unsigned aubsubtype )
{
   int retval = bmBufferData(intel, buf, size, data, flags);
   

   /* This only works because in this version of the buffer manager we
    * allocate all buffers statically in agp space and so can emit the
    * uploads to the aub file with the correct offsets as they happen.
    */
   if (retval == 0 && data && intel->aub_file) {

      if (buf->block && !buf->dirty) {
	 intel->vtbl.aub_gtt_data(intel,
				      buf->block->mem->ofs,
				      buf->block->virtual,
				      size,
				      aubtype,
				      aubsubtype);
	 buf->aub_dirty = 0;
      }
   }
   
   return retval;
}
		       

int bmBufferSubDataAUB(struct intel_context *intel, 
			struct buffer *buf, 
			unsigned offset, 
			unsigned size, 
			const void *data,
			unsigned aubtype,
			unsigned aubsubtype )
{
   int retval = bmBufferSubData(intel, buf, offset, size, data);
   

   /* This only works because in this version of the buffer manager we
    * allocate all buffers statically in agp space and so can emit the
    * uploads to the aub file with the correct offsets as they happen.
    */
   if (intel->aub_file) {
      if (retval == 0 && buf->block && !buf->dirty)
	 intel->vtbl.aub_gtt_data(intel,
				      buf->block->mem->ofs + offset,
				      ((const char *)buf->block->virtual) + offset,
				      size,
				      aubtype,
				      aubsubtype);
   }

   return retval;
}

void bmUnmapBufferAUB( struct intel_context *intel, 
		       struct buffer *buf,
		       unsigned aubtype,
		       unsigned aubsubtype )
{
   bmUnmapBuffer(intel, buf);

   if (intel->aub_file) {
      /* Hack - exclude the framebuffer mappings.  If you removed
       * this, you'd get very big aubfiles, but you *would* be able to
       * see fallback rendering.
       */
      if (buf->block  && !buf->dirty && buf->block->pool == &intel->bm->pool[0]) {
	 buf->aub_dirty = 1;
      }
   }
}

unsigned bmBufferOffset(struct intel_context *intel, 
			struct buffer *buf)
{
   struct bufmgr *bm = intel->bm;
   unsigned retval = 0;

   LOCK(bm);
   {
      assert(intel->locked);

      if (!buf->block &&
	  !evict_and_alloc_block(intel, buf)) {
	 bm->fail = 1;
	 retval = ~0;
      }
      else {
	 assert(buf->block);
	 assert(buf->block->buf == buf);

	 DBG("Add buf %d (block %p, dirty %d) to referenced list\n", buf->id, buf->block,
	     buf->dirty);

	 move_to_tail(&bm->referenced, buf->block);
	 buf->block->referenced = 1;

	 retval = buf->block->mem->ofs;
      }
   }
   UNLOCK(bm);

   return retval;
}



/* Extract data from the buffer:
 */
void bmBufferGetSubData(struct intel_context *intel, 
			struct buffer *buf, 
			unsigned offset, 
			unsigned size, 
			void *data )
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      DBG("bmBufferSubdata %d offset 0x%x sz 0x%x\n", buf->id, offset, size);

      if (buf->flags & (BM_NO_EVICT|BM_NO_BACKING_STORE)) {
	 if (buf->block && size) {
	    wait_quiescent(intel, buf->block);
	    do_memcpy(data, buf->block->virtual + offset, size); 
	 }
      }
      else {
	 if (buf->backing_store && size) {
	    do_memcpy(data, buf->backing_store + offset, size); 
	 }
      }
   }
   UNLOCK(bm);
}


/* Return a pointer to whatever space the buffer is currently resident in:
 */
void *bmMapBuffer( struct intel_context *intel,
		   struct buffer *buf, 
		   unsigned flags )
{
   struct bufmgr *bm = intel->bm;
   void *retval = NULL;

   LOCK(bm);
   {
      DBG("bmMapBuffer %d\n", buf->id);

      if (buf->mapped) {
	 _mesa_printf("%s: already mapped\n", __FUNCTION__);
	 retval = NULL;
      }
      else if (buf->flags & (BM_NO_BACKING_STORE|BM_NO_EVICT)) {

	 assert(intel->locked);

	 if (!buf->block && !evict_and_alloc_block(intel, buf)) {
	    DBG("%s: alloc failed\n", __FUNCTION__);
	    bm->fail = 1;
	    retval = NULL;
	 }
	 else {
	    assert(buf->block);
	    buf->dirty = 0;

	    if (!(buf->flags & BM_NO_FENCE_SUBDATA)) 
	       wait_quiescent(intel, buf->block);

	    buf->mapped = 1;
	    retval = buf->block->virtual;
	 }
      }
      else {
	 DBG("%s - set buf %d dirty\n", __FUNCTION__, buf->id);
	 set_dirty(intel, buf);

	 if (buf->backing_store == 0)
	    alloc_backing_store(intel, buf);

	 buf->mapped = 1;
	 retval = buf->backing_store;
      }
   }
   UNLOCK(bm);
   return retval;
}

void bmUnmapBuffer( struct intel_context *intel, struct buffer *buf )
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      DBG("bmUnmapBuffer %d\n", buf->id);
      buf->mapped = 0;
   }
   UNLOCK(bm);
}




/* This is the big hack that turns on BM_NO_BACKING_STORE.  Basically
 * says that an external party will maintain the backing store, eg
 * Mesa's local copy of texture data.
 */
void bmBufferSetInvalidateCB(struct intel_context *intel,
			     struct buffer *buf,
			     void (*invalidate_cb)( struct intel_context *, void *ptr ),
			     void *ptr,
			     GLboolean dont_fence_subdata)
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      if (buf->backing_store)
	 free_backing_store(intel, buf);

      buf->flags |= BM_NO_BACKING_STORE;
      
      if (dont_fence_subdata)
	 buf->flags |= BM_NO_FENCE_SUBDATA;

      DBG("bmBufferSetInvalidateCB set buf %d dirty\n", buf->id);
      buf->dirty = 1;
      buf->invalidate_cb = invalidate_cb;
      buf->invalidate_ptr = ptr;

      /* Note that it is invalid right from the start.  Also note
       * invalidate_cb is called with the bufmgr locked, so cannot
       * itself make bufmgr calls.
       */
      invalidate_cb( intel, ptr );
   }
   UNLOCK(bm);
}







/* This is only protected against thread interactions by the DRI lock
 * and the policy of ensuring that all dma is flushed prior to
 * releasing that lock.  Otherwise you might have two threads building
 * up a list of buffers to validate at once.
 */
int bmValidateBuffers( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;
   int retval = 0;

   LOCK(bm);
   {
      DBG("%s fail %d\n", __FUNCTION__, bm->fail);
      assert(intel->locked);

      if (!bm->fail) {
	 struct block *block, *tmp;

	 foreach_s(block, tmp, &bm->referenced) {
	    struct buffer *buf = block->buf;

	    DBG("Validate buf %d / block %p / dirty %d\n", buf->id, block, buf->dirty);

	    /* Upload the buffer contents if necessary:
	     */
	    if (buf->dirty) {
	       DBG("Upload dirty buf %d (%s) sz %d offset 0x%x\n", buf->id, 
		   buf->name, buf->size, block->mem->ofs);

	       assert(!(buf->flags & (BM_NO_BACKING_STORE|BM_NO_EVICT)));

	       wait_quiescent(intel, buf->block);

	       do_memcpy(buf->block->virtual,
			 buf->backing_store, 
			 buf->size);

	       if (intel->aub_file) {
		  intel->vtbl.aub_gtt_data(intel,
					       buf->block->mem->ofs,
					       buf->backing_store,
					       buf->size,
					       0,
					       0);
	       }

	       buf->dirty = 0;
	       buf->aub_dirty = 0;
	    }
	    else if (buf->aub_dirty) {
	       intel->vtbl.aub_gtt_data(intel,
					    buf->block->mem->ofs,
					    buf->block->virtual,
					    buf->size,
					    0,
					    0);
	       buf->aub_dirty = 0;
	    }

	    block->referenced = 0;
	    block->on_hardware = 1;
	    move_to_tail(&bm->on_hardware, block);
	 }

	 bm->need_fence = 1;
      }

      retval = bm->fail ? -1 : 0;
   }
   UNLOCK(bm);


   if (retval != 0)
      DBG("%s failed\n", __FUNCTION__);

   return retval;
}




void bmReleaseBuffers( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      struct block *block, *tmp;

      foreach_s (block, tmp, &bm->referenced) {

	 DBG("remove block %p from referenced list\n", block);

	 if (block->on_hardware) {
	    /* Return to the on-hardware list.
	     */
	    move_to_tail(&bm->on_hardware, block);	    
	 }
	 else if (block->fenced) {
	    struct block *s;

	    /* Hmm - have to scan the fenced list to insert the
	     * buffers in order.  This is O(nm), but rare and the
	     * numbers are low.
	     */
	    foreach (s, &bm->fenced) {
	       if (FENCE_LTE(block->fence, s->fence))
		  break;
	    }
	    
	    move_to_tail(s, block);
	 }
	 else {			
	    /* Return to the lru list:
	     */
	    move_to_tail(&block->pool->lru, block);
	 }

	 block->referenced = 0;
      }
   }
   UNLOCK(bm);
}


/* This functionality is used by the buffer manager, not really sure
 * if we need to be exposing it in this way, probably libdrm will
 * offer equivalent calls.
 *
 * For now they can stay, but will likely change/move before final:
 */
unsigned bmSetFence( struct intel_context *intel )
{
   assert(intel->locked);

   /* Emit MI_FLUSH here:
    */
   if (intel->bm->need_fence) {

      /* Emit a flush without using a batchbuffer.  Can't rely on the
       * batchbuffer at this level really.  Would really prefer that
       * the IRQ ioctly emitted the flush at the same time.
       */
      GLuint dword[2];
      dword[0] = intel->vtbl.flush_cmd();
      dword[1] = 0;
      intel_cmd_ioctl(intel, (char *)&dword, sizeof(dword));
      
      intel->bm->last_fence = intelEmitIrqLocked( intel );
      
      fence_blocks(intel, intel->bm->last_fence);

      intel->vtbl.note_fence(intel, intel->bm->last_fence);
      intel->bm->need_fence = 0;

      if (intel->thrashing) {
	 intel->thrashing--;
	 if (!intel->thrashing)
	    DBG("not thrashing\n");
      }
      
      intel->bm->free_on_hardware = 0;
   }
   
   return intel->bm->last_fence;
}

unsigned bmSetFenceLock( struct intel_context *intel )
{
  unsigned last;
  LOCK(intel->bm);
  last = bmSetFence(intel);
  UNLOCK(intel->bm);
  return last;
}
unsigned bmLockAndFence( struct intel_context *intel )
{
   if (intel->bm->need_fence) {
      LOCK_HARDWARE(intel);
      LOCK(intel->bm);
      bmSetFence(intel);
      UNLOCK(intel->bm);
      UNLOCK_HARDWARE(intel);
   }

   return intel->bm->last_fence;
}


void bmFinishFence( struct intel_context *intel, unsigned fence )
{
   if (!bmTestFence(intel, fence)) {
      DBG("...wait on fence %d\n", fence);
      intelWaitIrq( intel, fence );
   }
   assert(bmTestFence(intel, fence));
   check_fenced(intel);
}

void bmFinishFenceLock( struct intel_context *intel, unsigned fence )
{
   LOCK(intel->bm);
   bmFinishFence(intel, fence);
   UNLOCK(intel->bm);
}


/* Specifically ignore texture memory sharing.
 *  -- just evict everything
 *  -- and wait for idle
 */
void bm_fake_NotifyContendedLockTake( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      struct block *block, *tmp;
      GLuint i;

      assert(is_empty_list(&bm->referenced));

      bm->need_fence = 1;
      bm->fail = 0;
      bmFinishFence(intel, bmSetFence(intel));

      assert(is_empty_list(&bm->fenced));
      assert(is_empty_list(&bm->on_hardware));

      for (i = 0; i < bm->nr_pools; i++) {
	 if (!(bm->pool[i].flags & BM_NO_EVICT)) {
	    foreach_s(block, tmp, &bm->pool[i].lru) {
	       assert(bmTestFence(intel, block->fence));
	       set_dirty(intel, block->buf);
	    }
	 }
      }
   }
   UNLOCK(bm);
}



void bmEvictAll( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;

   LOCK(bm);
   {
      struct block *block, *tmp;
      GLuint i;

      DBG("%s\n", __FUNCTION__);

      assert(is_empty_list(&bm->referenced));

      bm->need_fence = 1;
      bm->fail = 0;
      bmFinishFence(intel, bmSetFence(intel));

      assert(is_empty_list(&bm->fenced));
      assert(is_empty_list(&bm->on_hardware));

      for (i = 0; i < bm->nr_pools; i++) {
	 if (!(bm->pool[i].flags & BM_NO_EVICT)) {
	    foreach_s(block, tmp, &bm->pool[i].lru) {
	       assert(bmTestFence(intel, block->fence));
	       set_dirty(intel, block->buf);
	       block->buf->block = NULL;

	       free_block(intel, block);
	    }
	 }
      }
   }
   UNLOCK(bm);
}


GLboolean bmError( struct intel_context *intel )
{
   struct bufmgr *bm = intel->bm;
   GLboolean retval;

   LOCK(bm);
   {
      retval = bm->fail;
   }
   UNLOCK(bm);

   return retval;
}


GLuint bmCtxId( struct intel_context *intel )
{
   return intel->bm->ctxId;
}
