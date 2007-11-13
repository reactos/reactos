/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
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
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 */

#include <xf86drm.h>
#include <stdlib.h>
#include <errno.h>
#include "imports.h"
#include "glthread.h"
#include "dri_bufpool.h"
#include "dri_bufmgr.h"
#include "intel_screen.h"

typedef struct
{
   drmMMListHead head;
   struct _BPool *parent;
   struct _DriFenceObject *fence;
   unsigned long start;
   int unfenced;
   int mapped;
} BBuf;

typedef struct _BPool
{
   _glthread_Mutex mutex;
   unsigned long bufSize;
   unsigned poolSize;
   unsigned numFree;
   unsigned numTot;
   unsigned numDelayed;
   unsigned checkDelayed;
   drmMMListHead free;
   drmMMListHead delayed;
   drmMMListHead head;
   drmBO kernelBO;
   void *virtual;
   BBuf *bufs;
} BPool;


static BPool *
createBPool(int fd, unsigned long bufSize, unsigned numBufs, unsigned flags,
            unsigned checkDelayed)
{
   BPool *p = (BPool *) malloc(sizeof(*p));
   BBuf *buf;
   int i;

   if (!p)
      return NULL;

   p->bufs = (BBuf *) malloc(numBufs * sizeof(*p->bufs));
   if (!p->bufs) {
      free(p);
      return NULL;
   }

   DRMINITLISTHEAD(&p->free);
   DRMINITLISTHEAD(&p->head);
   DRMINITLISTHEAD(&p->delayed);

   p->numTot = numBufs;
   p->numFree = numBufs;
   p->bufSize = bufSize;
   p->numDelayed = 0;
   p->checkDelayed = checkDelayed;

   _glthread_INIT_MUTEX(p->mutex);

   if (drmBOCreate(fd, 0, numBufs * bufSize, 0, NULL, drm_bo_type_dc,
                   flags, DRM_BO_HINT_DONT_FENCE, &p->kernelBO)) {
      free(p->bufs);
      free(p);
      return NULL;
   }
   if (drmBOMap(fd, &p->kernelBO, DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0,
                &p->virtual)) {
      drmBODestroy(fd, &p->kernelBO);
      free(p->bufs);
      free(p);
      return NULL;
   }

   /*
    * We unmap the buffer so that we can validate it later. Note that this is
    * just a synchronizing operation. The buffer will have a virtual mapping
    * until it is destroyed.
    */

   drmBOUnmap(fd, &p->kernelBO);

   buf = p->bufs;
   for (i = 0; i < numBufs; ++i) {
      buf->parent = p;
      buf->fence = NULL;
      buf->start = i * bufSize;
      buf->mapped = 0;
      buf->unfenced = 0;
      DRMLISTADDTAIL(&buf->head, &p->free);
      buf++;
   }

   return p;
}


static void
pool_checkFree(BPool * p, int wait)
{
   drmMMListHead *list, *prev;
   BBuf *buf;
   int signaled = 0;
   int i;

   list = p->delayed.next;

   if (p->numDelayed > 3) {
      for (i = 0; i < p->numDelayed; i += 3) {
         list = list->next;
      }
   }

   prev = list->prev;
   for (; list != &p->delayed; list = prev, prev = list->prev) {

      buf = DRMLISTENTRY(BBuf, list, head);

      if (!signaled) {
         if (wait) {
            driFenceFinish(buf->fence, DRM_FENCE_TYPE_EXE, 1);
            signaled = 1;
         }
         else {
            signaled = driFenceSignaled(buf->fence, DRM_FENCE_TYPE_EXE);
         }
      }

      if (!signaled)
         break;

      driFenceUnReference(buf->fence);
      buf->fence = NULL;
      DRMLISTDEL(list);
      p->numDelayed--;
      DRMLISTADD(list, &p->free);
      p->numFree++;
   }
}

static void *
pool_create(struct _DriBufferPool *pool,
            unsigned long size, unsigned flags, unsigned hint,
            unsigned alignment)
{
   BPool *p = (BPool *) pool->data;

   drmMMListHead *item;

   if (alignment && (alignment != 4096))
      return NULL;

   _glthread_LOCK_MUTEX(p->mutex);

   if (p->numFree == 0)
      pool_checkFree(p, GL_TRUE);

   if (p->numFree == 0) {
      fprintf(stderr, "Out of fixed size buffer objects\n");
      BM_CKFATAL(-ENOMEM);
   }

   item = p->free.next;

   if (item == &p->free) {
      fprintf(stderr, "Fixed size buffer pool corruption\n");
   }

   DRMLISTDEL(item);
   --p->numFree;

   _glthread_UNLOCK_MUTEX(p->mutex);
   return (void *) DRMLISTENTRY(BBuf, item, head);
}


static int
pool_destroy(struct _DriBufferPool *pool, void *private)
{
   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;

   _glthread_LOCK_MUTEX(p->mutex);

   if (buf->fence) {
      DRMLISTADDTAIL(&buf->head, &p->delayed);
      p->numDelayed++;
   }
   else {
      buf->unfenced = 0;
      DRMLISTADD(&buf->head, &p->free);
      p->numFree++;
   }

   if ((p->numDelayed % p->checkDelayed) == 0)
      pool_checkFree(p, 0);

   _glthread_UNLOCK_MUTEX(p->mutex);
   return 0;
}


static int
pool_map(struct _DriBufferPool *pool, void *private, unsigned flags,
         int hint, void **virtual)
{

   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;

   _glthread_LOCK_MUTEX(p->mutex);

   /*
    * Currently Mesa doesn't have any condition variables to resolve this
    * cleanly in a multithreading environment.
    * We bail out instead.
    */

   if (buf->mapped) {
      fprintf(stderr, "Trying to map already mapped buffer object\n");
      BM_CKFATAL(-EINVAL);
   }

#if 0
   if (buf->unfenced && !(hint & DRM_BO_HINT_ALLOW_UNFENCED_MAP)) {
      fprintf(stderr, "Trying to map an unfenced buffer object 0x%08x"
              " 0x%08x %d\n", hint, flags, buf->start);
      BM_CKFATAL(-EINVAL);
   }

#endif

   if (buf->fence) {
      _glthread_UNLOCK_MUTEX(p->mutex);
      return -EBUSY;
   }

   buf->mapped = GL_TRUE;
   *virtual = (unsigned char *) p->virtual + buf->start;
   _glthread_UNLOCK_MUTEX(p->mutex);
   return 0;
}

static int
pool_waitIdle(struct _DriBufferPool *pool, void *private, int lazy)
{
   BBuf *buf = (BBuf *) private;
   driFenceFinish(buf->fence, 0, lazy);
   return 0;
}

static int
pool_unmap(struct _DriBufferPool *pool, void *private)
{
   BBuf *buf = (BBuf *) private;

   buf->mapped = 0;
   return 0;
}

static unsigned long
pool_offset(struct _DriBufferPool *pool, void *private)
{
   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;

   return p->kernelBO.offset + buf->start;
}

static unsigned
pool_flags(struct _DriBufferPool *pool, void *private)
{
   BPool *p = (BPool *) pool->data;

   return p->kernelBO.flags;
}

static unsigned long
pool_size(struct _DriBufferPool *pool, void *private)
{
   BPool *p = (BPool *) pool->data;

   return p->bufSize;
}


static int
pool_fence(struct _DriBufferPool *pool, void *private,
           struct _DriFenceObject *fence)
{
   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;

   _glthread_LOCK_MUTEX(p->mutex);
   if (buf->fence) {
      driFenceUnReference(buf->fence);
   }
   buf->fence = fence;
   buf->unfenced = 0;
   driFenceReference(buf->fence);
   _glthread_UNLOCK_MUTEX(p->mutex);

   return 0;
}

static drmBO *
pool_kernel(struct _DriBufferPool *pool, void *private)
{
   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;

   return &p->kernelBO;
}

static int
pool_validate(struct _DriBufferPool *pool, void *private)
{
   BBuf *buf = (BBuf *) private;
   BPool *p = buf->parent;
   _glthread_LOCK_MUTEX(p->mutex);
   buf->unfenced = GL_TRUE;
   _glthread_UNLOCK_MUTEX(p->mutex);
   return 0;
}

static void
pool_takedown(struct _DriBufferPool *pool)
{
   BPool *p = (BPool *) pool->data;

   /*
    * Wait on outstanding fences. 
    */

   _glthread_LOCK_MUTEX(p->mutex);
   while ((p->numFree < p->numTot) && p->numDelayed) {
      _glthread_UNLOCK_MUTEX(p->mutex);
      sched_yield();
      pool_checkFree(p, GL_TRUE);
      _glthread_LOCK_MUTEX(p->mutex);
   }

   drmBODestroy(pool->fd, &p->kernelBO);
   free(p->bufs);
   _glthread_UNLOCK_MUTEX(p->mutex);
   free(p);
   free(pool);
}


struct _DriBufferPool *
driBatchPoolInit(int fd, unsigned flags,
                 unsigned long bufSize,
                 unsigned numBufs, unsigned checkDelayed)
{
   struct _DriBufferPool *pool;

   pool = (struct _DriBufferPool *) malloc(sizeof(*pool));
   if (!pool)
      return NULL;

   pool->data = createBPool(fd, bufSize, numBufs, flags, checkDelayed);
   if (!pool->data)
      return NULL;

   pool->fd = fd;
   pool->map = &pool_map;
   pool->unmap = &pool_unmap;
   pool->destroy = &pool_destroy;
   pool->offset = &pool_offset;
   pool->flags = &pool_flags;
   pool->size = &pool_size;
   pool->create = &pool_create;
   pool->fence = &pool_fence;
   pool->kernel = &pool_kernel;
   pool->validate = &pool_validate;
   pool->waitIdle = &pool_waitIdle;
   pool->setstatic = NULL;
   pool->takeDown = &pool_takedown;
   return pool;
}
