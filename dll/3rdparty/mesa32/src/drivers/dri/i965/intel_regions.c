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

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 *   - some logic for moving the buffers to the best memory pools for
 *     given operations.
 *
 * Most of this is to make it easier to implement the fixed-layout
 * mipmap tree required by intel hardware in the face of GL's
 * programming interface where each image can be specifed in random
 * order and it isn't clear what layout the tree should have until the
 * last moment.
 */

#include "intel_context.h"
#include "intel_regions.h"
#include "intel_blit.h"
#include "bufmgr.h"
#include "imports.h"

/* XXX: Thread safety?
 */
GLubyte *intel_region_map(struct intel_context *intel, struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!region->map_refcount++) {
      region->map = bmMapBuffer(intel, region->buffer, 0);
      if (!region->map)
	 region->map_refcount--;
   }

   return region->map;
}

void intel_region_unmap(struct intel_context *intel, 
			struct intel_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!--region->map_refcount) {
      bmUnmapBufferAUB(intel, region->buffer, 0, 0);
      region->map = NULL;
   }
}

struct intel_region *intel_region_alloc( struct intel_context *intel, 
					 GLuint cpp,
					 GLuint pitch, 
					 GLuint height )
{
   struct intel_region *region = calloc(sizeof(*region), 1);

   DBG("%s %dx%dx%d == 0x%x bytes\n", __FUNCTION__,
       cpp, pitch, height, cpp*pitch*height);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height; 	/* needed? */
   region->refcount = 1;

   bmGenBuffers(intel, "tex", 1, &region->buffer, 6);
   bmBufferData(intel, region->buffer, pitch * cpp * height, NULL, 0);

   return region;
}

void intel_region_reference( struct intel_region **dst,
			     struct intel_region *src)
{
   src->refcount++;
   assert(*dst == NULL);
   *dst = src;
}

void intel_region_release( struct intel_context *intel,
			   struct intel_region **region )
{
   if (!*region)
      return;

   DBG("%s %d\n", __FUNCTION__, (*region)->refcount-1);
   
   if (--(*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);
      bmDeleteBuffers(intel, 1, &(*region)->buffer);
      free(*region);
   }
   *region = NULL;
}


struct intel_region *intel_region_create_static( struct intel_context *intel, 
						 GLuint mem_type,
						 GLuint offset,
						 void *virtual,
						 GLuint cpp,
						 GLuint pitch, 
						 GLuint height,
						 GLuint size,
						 GLboolean tiled )
{
   struct intel_region *region = calloc(sizeof(*region), 1);
   GLint pool;

   DBG("%s\n", __FUNCTION__);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height; 	/* needed? */
   region->refcount = 1;
   region->tiled = tiled;

   /* Recipe for creating a static buffer - create a static pool with
    * the right offset and size, generate a buffer and use a special
    * call to bind it to all of the memory in that pool.
    */
   pool = bmInitPool(intel, offset, virtual, size, 
		     (BM_MEM_AGP |
		      BM_NO_UPLOAD | 
		      BM_NO_EVICT | 
		      BM_NO_MOVE));
   if (pool < 0) {
      _mesa_printf("bmInitPool failed for static region\n");
      exit(1);
   }

   region->buffer = bmGenBufferStatic(intel, pool);

   return region;
}




void _mesa_copy_rect( GLubyte *dst,
		      GLuint cpp,
		      GLuint dst_pitch,
		      GLuint dst_x, 
		      GLuint dst_y,
		      GLuint width,
		      GLuint height,
		      const GLubyte *src,
		      GLuint src_pitch,
		      GLuint src_x,
		      GLuint src_y )
{
   GLuint i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && 
       width == src_pitch)
      do_memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
	 do_memcpy(dst, src, width);
	 dst += dst_pitch;
	 src += src_pitch;
      }
   }
}


/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
GLboolean intel_region_data(struct intel_context *intel, 
			    struct intel_region *dst,
			    GLuint dst_offset,
			    GLuint dstx, GLuint dsty,
			    const void *src, GLuint src_pitch,
			    GLuint srcx, GLuint srcy,
			    GLuint width, GLuint height)
{
   DBG("%s\n", __FUNCTION__);

   if (width == dst->pitch && 
       width == src_pitch &&
       dst_offset == 0 &&
       height == dst->height &&
       srcx == 0 &&
       srcy == 0) 
   {
      return (bmBufferDataAUB(intel,
			      dst->buffer,
			      dst->cpp * width * dst->height,
			      src, 0, 0, 0) == 0);
   }
   else {
      GLubyte *map = intel_region_map(intel, dst);

      if (map) {
	 assert (dst_offset + dstx + width + 
		 (dsty + height - 1) * dst->pitch * dst->cpp <= 
		 dst->pitch * dst->cpp * dst->height);
	 
	 _mesa_copy_rect(map + dst_offset,
			 dst->cpp,
			 dst->pitch,
			 dstx, dsty,
			 width, height,
			 src,
			 src_pitch,
			 srcx, srcy);      
	 
	 intel_region_unmap(intel, dst);
	 return GL_TRUE;
      }
      else 
	 return GL_FALSE;
   }
}
			  
/* Copy rectangular sub-regions. Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void intel_region_copy( struct intel_context *intel,
			struct intel_region *dst,
			GLuint dst_offset,
			GLuint dstx, GLuint dsty,
			struct intel_region *src,
			GLuint src_offset,
			GLuint srcx, GLuint srcy,
			GLuint width, GLuint height )
{
   DBG("%s\n", __FUNCTION__);

   assert(src->cpp == dst->cpp);

   intelEmitCopyBlit(intel,
		     dst->cpp,
		     src->pitch, src->buffer, src_offset, src->tiled,
		     dst->pitch, dst->buffer, dst_offset, dst->tiled,
		     srcx, srcy,
		     dstx, dsty,
		     width, height,
		     GL_COPY );
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
void intel_region_fill( struct intel_context *intel,
			struct intel_region *dst,
			GLuint dst_offset,
			GLuint dstx, GLuint dsty,
			GLuint width, GLuint height,
			GLuint color )
{
   DBG("%s\n", __FUNCTION__);
   
   intelEmitFillBlit(intel,
		     dst->cpp,
		     dst->pitch, dst->buffer, dst_offset, dst->tiled,
		     dstx, dsty,
		     width, height,
		     color );
}

