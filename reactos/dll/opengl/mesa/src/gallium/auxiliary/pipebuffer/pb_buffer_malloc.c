/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * \file
 * Implementation of malloc-based buffers to store data that can't be processed
 * by the hardware. 
 * 
 * \author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "util/u_debug.h"
#include "util/u_memory.h"
#include "pb_buffer.h"
#include "pb_bufmgr.h"


struct malloc_buffer 
{
   struct pb_buffer base;
   void *data;
};


extern const struct pb_vtbl malloc_buffer_vtbl;

static INLINE struct malloc_buffer *
malloc_buffer(struct pb_buffer *buf)
{
   assert(buf);
   if (!buf)
      return NULL;
   assert(buf->vtbl == &malloc_buffer_vtbl);
   return (struct malloc_buffer *)buf;
}


static void
malloc_buffer_destroy(struct pb_buffer *buf)
{
   align_free(malloc_buffer(buf)->data);
   FREE(buf);
}


static void *
malloc_buffer_map(struct pb_buffer *buf, 
                  unsigned flags,
		  void *flush_ctx)
{
   return malloc_buffer(buf)->data;
}


static void
malloc_buffer_unmap(struct pb_buffer *buf)
{
   /* No-op */
}


static enum pipe_error 
malloc_buffer_validate(struct pb_buffer *buf, 
                       struct pb_validate *vl,
                       unsigned flags)
{
   assert(0);
   return PIPE_ERROR;
}


static void
malloc_buffer_fence(struct pb_buffer *buf, 
                    struct pipe_fence_handle *fence)
{
   assert(0);
}


static void
malloc_buffer_get_base_buffer(struct pb_buffer *buf,
                              struct pb_buffer **base_buf,
                              pb_size *offset)
{
   *base_buf = buf;
   *offset = 0;
}


const struct pb_vtbl 
malloc_buffer_vtbl = {
      malloc_buffer_destroy,
      malloc_buffer_map,
      malloc_buffer_unmap,
      malloc_buffer_validate,
      malloc_buffer_fence,
      malloc_buffer_get_base_buffer
};


struct pb_buffer *
pb_malloc_buffer_create(pb_size size,
                   	const struct pb_desc *desc) 
{
   struct malloc_buffer *buf;
   
   /* TODO: do a single allocation */
   
   buf = CALLOC_STRUCT(malloc_buffer);
   if(!buf)
      return NULL;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.usage = desc->usage;
   buf->base.size = size;
   buf->base.alignment = desc->alignment;
   buf->base.vtbl = &malloc_buffer_vtbl;

   buf->data = align_malloc(size, desc->alignment < sizeof(void*) ? sizeof(void*) : desc->alignment);
   if(!buf->data) {
      FREE(buf);
      return NULL;
   }

   return &buf->base;
}


static struct pb_buffer *
pb_malloc_bufmgr_create_buffer(struct pb_manager *mgr, 
                               pb_size size,
                               const struct pb_desc *desc) 
{
   return pb_malloc_buffer_create(size, desc);
}


static void
pb_malloc_bufmgr_flush(struct pb_manager *mgr) 
{
   /* No-op */
}


static void
pb_malloc_bufmgr_destroy(struct pb_manager *mgr) 
{
   /* No-op */
}


static boolean
pb_malloc_bufmgr_is_buffer_busy( struct pb_manager *mgr,
                                 struct pb_buffer *buf )
{
   return FALSE;
}


static struct pb_manager 
pb_malloc_bufmgr = {
   pb_malloc_bufmgr_destroy,
   pb_malloc_bufmgr_create_buffer,
   pb_malloc_bufmgr_flush,
   pb_malloc_bufmgr_is_buffer_busy
};


struct pb_manager *
pb_malloc_bufmgr_create(void) 
{
  return &pb_malloc_bufmgr;
}
