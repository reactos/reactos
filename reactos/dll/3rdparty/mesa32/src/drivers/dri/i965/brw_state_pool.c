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
#include "imports.h"

#include "intel_ioctl.h"
#include "bufmgr.h"

GLboolean brw_pool_alloc( struct brw_mem_pool *pool,
			  GLuint size,
			  GLuint align,
			  GLuint *offset_return)
{
   GLuint align_mask = (1<<align)-1;
   GLuint fixup = ((pool->offset + align_mask) & ~align_mask) - pool->offset;

   size = (size + 3) & ~3;

   if (pool->offset + fixup + size >= pool->size) {
      _mesa_printf("%s failed\n", __FUNCTION__);
      assert(0);
      exit(0);
   }

   pool->offset += fixup;
   *offset_return = pool->offset;
   pool->offset += size; 
  
   return GL_TRUE;
}

static
void brw_invalidate_pool( struct intel_context *intel,
			  struct brw_mem_pool *pool )
{
   if (INTEL_DEBUG & DEBUG_STATE)
      _mesa_printf("\n\n\n %s \n\n\n", __FUNCTION__);
   
   bmBufferData(intel,
		pool->buffer,
		pool->size,
		NULL,
		0); 

   pool->offset = 0;

   brw_clear_all_caches(pool->brw);
}

static void brw_invalidate_pool_cb( struct intel_context *intel, void *ptr )
{
   struct brw_mem_pool *pool = (struct brw_mem_pool *) ptr;

   pool->offset = 0;
   brw_clear_all_caches(pool->brw);
}



static void brw_init_pool( struct brw_context *brw,
			   GLuint pool_id,
			   GLuint size )
{
   struct brw_mem_pool *pool = &brw->pool[pool_id];

   pool->size = size;   
   pool->brw = brw;
   
   bmGenBuffers(&brw->intel, "pool", 1, &pool->buffer, 12);

   /* Also want to say not to wait on fences when data is presented
    */
   bmBufferSetInvalidateCB(&brw->intel, pool->buffer, 
			   brw_invalidate_pool_cb, 
			   pool,
			   GL_TRUE);   

   bmBufferData(&brw->intel,
		pool->buffer,
		pool->size,
		NULL,
		0); 

}

static void brw_destroy_pool( struct brw_context *brw,
			      GLuint pool_id )
{
   struct brw_mem_pool *pool = &brw->pool[pool_id];
   
   bmDeleteBuffers(&brw->intel, 1, &pool->buffer);
}


void brw_pool_check_wrap( struct brw_context *brw,
			  struct brw_mem_pool *pool )
{
   if (pool->offset > (pool->size * 3) / 4) {
      if (brw->intel.aub_file)
	 brw->intel.aub_wrap = 1;
      else
	 brw->state.dirty.brw |= BRW_NEW_CONTEXT;
   }

}

void brw_init_pools( struct brw_context *brw )
{
   brw_init_pool(brw, BRW_GS_POOL, 0x80000);
   brw_init_pool(brw, BRW_SS_POOL, 0x80000);
}

void brw_destroy_pools( struct brw_context *brw )
{
   brw_destroy_pool(brw, BRW_GS_POOL);
   brw_destroy_pool(brw, BRW_SS_POOL);
}


void brw_invalidate_pools( struct brw_context *brw )
{
   brw_invalidate_pool(&brw->intel, &brw->pool[BRW_GS_POOL]);
   brw_invalidate_pool(&brw->intel, &brw->pool[BRW_SS_POOL]);
}
