/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
     


#include "brw_state.h"
#include "brw_aub.h"
#include "intel_batchbuffer.h"
#include "imports.h"



/* A facility similar to the data caching code above, which aims to
 * prevent identical commands being issued repeatedly.
 */
GLboolean brw_cached_batch_struct( struct brw_context *brw,
				   const void *data,
				   GLuint sz )
{
   struct brw_cached_batch_item *item = brw->cached_batch_items;
   struct header *newheader = (struct header *)data;

   if (brw->emit_state_always) {
      intel_batchbuffer_data(brw->intel.batch, data, sz, 0);
      return GL_TRUE;
   }

   while (item) {
      if (item->header->opcode == newheader->opcode) {
	 if (item->sz == sz && memcmp(item->header, newheader, sz) == 0)
	    return GL_FALSE;
	 if (item->sz != sz) {
	    _mesa_free(item->header);
	    item->header = _mesa_malloc(sz);
	    item->sz = sz;
	 }
	 goto emit;
      }
      item = item->next;
   }

   assert(!item);
   item = CALLOC_STRUCT(brw_cached_batch_item);
   item->header = _mesa_malloc(sz);
   item->sz = sz;
   item->next = brw->cached_batch_items;
   brw->cached_batch_items = item;

 emit:
   memcpy(item->header, newheader, sz);
   intel_batchbuffer_data(brw->intel.batch, data, sz, 0);
   return GL_TRUE;
}

static void clear_batch_cache( struct brw_context *brw )
{
   struct brw_cached_batch_item *item = brw->cached_batch_items;

   while (item) {
      struct brw_cached_batch_item *next = item->next;
      free((void *)item->header);
      free(item);
      item = next;
   }

   brw->cached_batch_items = NULL;


   brw_clear_all_caches(brw);

   bmReleaseBuffers(&brw->intel);
   
   brw_invalidate_pools(brw);
}

void brw_clear_batch_cache_flush( struct brw_context *brw )
{
   clear_batch_cache(brw);

   brw->wrap = 0;
   
/*    brw_do_flush(brw, BRW_FLUSH_STATE_CACHE|BRW_FLUSH_READ_CACHE); */
   
   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;
}



void brw_destroy_batch_cache( struct brw_context *brw )
{
   clear_batch_cache(brw);
}
