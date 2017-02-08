/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * @file
 * A variation of malloc buffers which get transferred to real graphics memory
 * when there is an attempt to validate them. 
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "util/u_debug.h"
#include "util/u_memory.h"
#include "pb_buffer.h"
#include "pb_bufmgr.h"


struct pb_ondemand_manager;


struct pb_ondemand_buffer 
{
   struct pb_buffer base;
   
   struct pb_ondemand_manager *mgr;
   
   /** Regular malloc'ed memory */
   void *data;
   unsigned mapcount;
   
   /** Real buffer */
   struct pb_buffer *buffer;
   pb_size size;
   struct pb_desc desc;
};


struct pb_ondemand_manager
{
   struct pb_manager base;
   
   struct pb_manager *provider;
};


extern const struct pb_vtbl pb_ondemand_buffer_vtbl;

static INLINE struct pb_ondemand_buffer *
pb_ondemand_buffer(struct pb_buffer *buf)
{
   assert(buf);
   if (!buf)
      return NULL;
   assert(buf->vtbl == &pb_ondemand_buffer_vtbl);
   return (struct pb_ondemand_buffer *)buf;
}

static INLINE struct pb_ondemand_manager *
pb_ondemand_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_ondemand_manager *)mgr;
}


static void
pb_ondemand_buffer_destroy(struct pb_buffer *_buf)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);
   
   pb_reference(&buf->buffer, NULL);
   
   align_free(buf->data);
   
   FREE(buf);
}


static void *
pb_ondemand_buffer_map(struct pb_buffer *_buf, 
                       unsigned flags, void *flush_ctx)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);

   if(buf->buffer) {
      assert(!buf->data);
      return pb_map(buf->buffer, flags, flush_ctx);
   }
   else {
      assert(buf->data);
      ++buf->mapcount;
      return buf->data;
   }
}


static void
pb_ondemand_buffer_unmap(struct pb_buffer *_buf)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);

   if(buf->buffer) {
      assert(!buf->data);
      pb_unmap(buf->buffer);
   }
   else {
      assert(buf->data);
      assert(buf->mapcount);
      if(buf->mapcount)
         --buf->mapcount;
   }
}


static enum pipe_error 
pb_ondemand_buffer_instantiate(struct pb_ondemand_buffer *buf)
{
   if(!buf->buffer) {
      struct pb_manager *provider = buf->mgr->provider;
      uint8_t *map;
      
      assert(!buf->mapcount);
      
      buf->buffer = provider->create_buffer(provider, buf->size, &buf->desc);
      if(!buf->buffer)
         return PIPE_ERROR_OUT_OF_MEMORY;
      
      map = pb_map(buf->buffer, PB_USAGE_CPU_READ, NULL);
      if(!map) {
         pb_reference(&buf->buffer, NULL);
         return PIPE_ERROR;
      }
      
      memcpy(map, buf->data, buf->size);
      
      pb_unmap(buf->buffer);
      
      if(!buf->mapcount) {
         FREE(buf->data);
         buf->data = NULL;
      }
   }
   
   return PIPE_OK;
}

static enum pipe_error 
pb_ondemand_buffer_validate(struct pb_buffer *_buf, 
                            struct pb_validate *vl,
                            unsigned flags)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);
   enum pipe_error ret; 

   assert(!buf->mapcount);
   if(buf->mapcount)
      return PIPE_ERROR;

   ret = pb_ondemand_buffer_instantiate(buf);
   if(ret != PIPE_OK)
      return ret;
   
   return pb_validate(buf->buffer, vl, flags);
}


static void
pb_ondemand_buffer_fence(struct pb_buffer *_buf, 
                         struct pipe_fence_handle *fence)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);
   
   assert(buf->buffer);
   if(!buf->buffer)
      return;
   
   pb_fence(buf->buffer, fence);
}


static void
pb_ondemand_buffer_get_base_buffer(struct pb_buffer *_buf,
                                   struct pb_buffer **base_buf,
                                   pb_size *offset)
{
   struct pb_ondemand_buffer *buf = pb_ondemand_buffer(_buf);

   if(pb_ondemand_buffer_instantiate(buf) != PIPE_OK) {
      assert(0);
      *base_buf = &buf->base;
      *offset = 0;
      return;
   }

   pb_get_base_buffer(buf->buffer, base_buf, offset);
}


const struct pb_vtbl 
pb_ondemand_buffer_vtbl = {
      pb_ondemand_buffer_destroy,
      pb_ondemand_buffer_map,
      pb_ondemand_buffer_unmap,
      pb_ondemand_buffer_validate,
      pb_ondemand_buffer_fence,
      pb_ondemand_buffer_get_base_buffer
};


static struct pb_buffer *
pb_ondemand_manager_create_buffer(struct pb_manager *_mgr, 
                                  pb_size size,
                                  const struct pb_desc *desc) 
{
   struct pb_ondemand_manager *mgr = pb_ondemand_manager(_mgr);
   struct pb_ondemand_buffer *buf;
   
   buf = CALLOC_STRUCT(pb_ondemand_buffer);
   if(!buf)
      return NULL;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment = desc->alignment;
   buf->base.usage = desc->usage;
   buf->base.size = size;
   buf->base.vtbl = &pb_ondemand_buffer_vtbl;
   
   buf->mgr = mgr;
   
   buf->data = align_malloc(size, desc->alignment < sizeof(void*) ? sizeof(void*) : desc->alignment);
   if(!buf->data) {
      FREE(buf);
      return NULL;
   }
   
   buf->size = size;
   buf->desc = *desc;

   return &buf->base;
}


static void
pb_ondemand_manager_flush(struct pb_manager *_mgr) 
{
   struct pb_ondemand_manager *mgr = pb_ondemand_manager(_mgr);
   
   mgr->provider->flush(mgr->provider);
}


static void
pb_ondemand_manager_destroy(struct pb_manager *_mgr) 
{
   struct pb_ondemand_manager *mgr = pb_ondemand_manager(_mgr);

   FREE(mgr);
}


struct pb_manager *
pb_ondemand_manager_create(struct pb_manager *provider) 
{
   struct pb_ondemand_manager *mgr;

   if(!provider)
      return NULL;
   
   mgr = CALLOC_STRUCT(pb_ondemand_manager);
   if(!mgr)
      return NULL;
   
   mgr->base.destroy = pb_ondemand_manager_destroy;
   mgr->base.create_buffer = pb_ondemand_manager_create_buffer;
   mgr->base.flush = pb_ondemand_manager_flush;
   
   mgr->provider = provider;

   return &mgr->base;
}
