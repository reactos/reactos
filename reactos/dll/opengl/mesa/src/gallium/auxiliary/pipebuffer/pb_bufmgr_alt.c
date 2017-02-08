/**************************************************************************
 *
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * Allocate buffers from two alternative buffer providers.
 * 
 * \author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


#include "pipe/p_compiler.h"
#include "util/u_debug.h"
#include "util/u_memory.h"

#include "pb_buffer.h"
#include "pb_bufmgr.h"


struct pb_alt_manager
{
   struct pb_manager base;

   struct pb_manager *provider1;
   struct pb_manager *provider2;
};


static INLINE struct pb_alt_manager *
pb_alt_manager(struct pb_manager *mgr)
{
   assert(mgr);
   return (struct pb_alt_manager *)mgr;
}


static struct pb_buffer *
pb_alt_manager_create_buffer(struct pb_manager *_mgr, 
                             pb_size size,
                             const struct pb_desc *desc)
{
   struct pb_alt_manager *mgr = pb_alt_manager(_mgr);
   struct pb_buffer *buf;
   
   buf = mgr->provider1->create_buffer(mgr->provider1, size, desc);
   if(buf)
      return buf;
   
   buf = mgr->provider2->create_buffer(mgr->provider2, size, desc);
   return buf;
}


static void
pb_alt_manager_flush(struct pb_manager *_mgr)
{
   struct pb_alt_manager *mgr = pb_alt_manager(_mgr);
   
   assert(mgr->provider1->flush);
   if(mgr->provider1->flush)
      mgr->provider1->flush(mgr->provider1);
   
   assert(mgr->provider2->flush);
   if(mgr->provider2->flush)
      mgr->provider2->flush(mgr->provider2);
}


static void
pb_alt_manager_destroy(struct pb_manager *mgr)
{
   FREE(mgr);
}


struct pb_manager *
pb_alt_manager_create(struct pb_manager *provider1, 
                      struct pb_manager *provider2)
{
   struct pb_alt_manager *mgr;

   if(!provider1 || !provider2)
      return NULL;
   
   mgr = CALLOC_STRUCT(pb_alt_manager);
   if (!mgr)
      return NULL;

   mgr->base.destroy = pb_alt_manager_destroy;
   mgr->base.create_buffer = pb_alt_manager_create_buffer;
   mgr->base.flush = pb_alt_manager_flush;
   mgr->provider1 = provider1;
   mgr->provider2 = provider2;
      
   return &mgr->base;
}
