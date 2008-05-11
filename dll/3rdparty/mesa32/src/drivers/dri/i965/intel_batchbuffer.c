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

#include "imports.h"
#include "intel_batchbuffer.h"
#include "intel_ioctl.h"
#include "bufmgr.h"


static void intel_batchbuffer_reset( struct intel_batchbuffer *batch )
{
   assert(batch->map == NULL);

   batch->offset = (unsigned long)batch->ptr;
   batch->offset = (batch->offset + 63) & ~63;
   batch->ptr = (unsigned char *) batch->offset;

   if (BATCH_SZ - batch->offset < BATCH_REFILL) {
      bmBufferData(batch->intel, 
		   batch->buffer,
		   BATCH_SZ, 
		   NULL, 
		   0); 
      batch->offset = 0;
      batch->ptr = NULL;
   }
		
   batch->flags = 0;
}

static void intel_batchbuffer_reset_cb( struct intel_context *intel,
					void *ptr )
{
   struct intel_batchbuffer *batch = (struct intel_batchbuffer *)ptr;
   assert(batch->map == NULL);
   batch->flags = 0;
   batch->offset = 0;
   batch->ptr = NULL;
}

GLubyte *intel_batchbuffer_map( struct intel_batchbuffer *batch )
{
   if (!batch->map) {
      batch->map = bmMapBuffer(batch->intel, batch->buffer, 
			       BM_MEM_AGP|BM_MEM_LOCAL|BM_CLIENT|BM_WRITE);
      batch->ptr += (unsigned long)batch->map;
   }

   return batch->map;
}

void intel_batchbuffer_unmap( struct intel_batchbuffer *batch )
{
   if (batch->map) {
      batch->ptr -= (unsigned long)batch->map;
      batch->map = NULL;
      bmUnmapBuffer(batch->intel, batch->buffer);
   }
}



/*======================================================================
 * Public functions
 */
struct intel_batchbuffer *intel_batchbuffer_alloc( struct intel_context *intel )
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;

   bmGenBuffers(intel, "batch", 1, &batch->buffer, 12);

   bmBufferSetInvalidateCB(intel, batch->buffer,
			   intel_batchbuffer_reset_cb,
			   batch,
			   GL_TRUE);

   bmBufferData(batch->intel,
		batch->buffer,
		BATCH_SZ,
		NULL,
		0);


   return batch;
}

void intel_batchbuffer_free( struct intel_batchbuffer *batch )
{
   if (batch->map) 
      bmUnmapBuffer(batch->intel, batch->buffer);
   
   bmDeleteBuffers(batch->intel, 1, &batch->buffer);
   free(batch);
}


#define MI_BATCH_BUFFER_END 	(0xA<<23)


GLboolean intel_batchbuffer_flush( struct intel_batchbuffer *batch )
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - (batch->map + batch->offset);
   GLuint offset;
   GLint retval = GL_TRUE;

   assert(intel->locked);

   if (used == 0) {
      bmReleaseBuffers( batch->intel );
      return GL_TRUE;
   }

   /* Add the MI_BATCH_BUFFER_END.  Always add an MI_FLUSH - this is a
    * performance drain that we would like to avoid.
    */
   if (used & 4) {
      ((int *)batch->ptr)[0] = MI_BATCH_BUFFER_END;
      batch->ptr += 4;
      used += 4;
   }
   else {
      ((int *)batch->ptr)[0] = 0;
      ((int *)batch->ptr)[1] = MI_BATCH_BUFFER_END;

      batch->ptr += 8;
      used += 8;
   }

   intel_batchbuffer_unmap(batch);

   /* Get the batch buffer offset: Must call bmBufferOffset() before
    * bmValidateBuffers(), otherwise the buffer won't be on the inuse
    * list.
    */
   offset = bmBufferOffset(batch->intel, batch->buffer);

   if (bmValidateBuffers( batch->intel ) != 0) {
      assert(intel->locked);
      bmReleaseBuffers( batch->intel );
      retval = GL_FALSE;
      goto out;
   }


   if (intel->aub_file) {
      /* Send buffered commands to aubfile as a single packet. 
       */
      intel_batchbuffer_map(batch);
      ((int *)batch->ptr)[-1] = intel->vtbl.flush_cmd();
      intel->vtbl.aub_commands(intel,
			       offset, /* Fulsim wierdness - don't adjust */
			       batch->map + batch->offset,
			       used);
      ((int *)batch->ptr)[-1] = MI_BATCH_BUFFER_END;
      intel_batchbuffer_unmap(batch);
   }


   /* Fire the batch buffer, which was uploaded above:
    */
   intel_batch_ioctl(batch->intel, 
		     offset + batch->offset,
		     used);

   if (intel->aub_file && 
       intel->ctx.DrawBuffer->_ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT)
      intel->vtbl.aub_dump_bmp( intel, 0 );

   /* Reset the buffer:
    */
 out:
   intel_batchbuffer_reset( batch );
   intel_batchbuffer_map( batch );

   if (!retval)
      DBG("%s failed\n", __FUNCTION__);

   return retval;
}







void intel_batchbuffer_align( struct intel_batchbuffer *batch,
			      GLuint align,
			      GLuint sz )
{
   unsigned long ptr = (unsigned long) batch->ptr;
   unsigned long aptr = (ptr + align) & ~((unsigned long)align-1);
   GLuint fixup = aptr - ptr;

   if (intel_batchbuffer_space(batch) < fixup + sz)
      intel_batchbuffer_flush(batch);
   else {
      memset(batch->ptr, 0, fixup);      
      batch->ptr += fixup;
   }
}




void intel_batchbuffer_data(struct intel_batchbuffer *batch,
			    const void *data,
			    GLuint bytes,
			    GLuint flags)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes, flags);
   __memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}

