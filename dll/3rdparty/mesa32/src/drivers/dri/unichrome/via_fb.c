/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>

#include "via_context.h"
#include "via_ioctl.h"
#include "via_fb.h"
#include "xf86drm.h"
#include "imports.h"
#include "simple_list.h"
#include <sys/ioctl.h>

GLboolean
via_alloc_draw_buffer(struct via_context *vmesa, struct via_renderbuffer *buf)
{
   drm_via_mem_t mem;
   mem.context = vmesa->hHWContext;
   mem.size = buf->size;
   mem.type = VIA_MEM_VIDEO;
   mem.offset = 0;
   mem.index = 0;

   if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_ALLOCMEM, &mem)) 
      return GL_FALSE;
    
    
   buf->offset = mem.offset;
   buf->map = (char *)vmesa->driScreen->pFB + mem.offset;
   buf->index = mem.index;
   return GL_TRUE;
}

void
via_free_draw_buffer(struct via_context *vmesa, struct via_renderbuffer *buf)
{
   drm_via_mem_t mem;

   if (!vmesa) return;

   mem.context = vmesa->hHWContext;
   mem.index = buf->index;
   mem.type = VIA_MEM_VIDEO;
   mem.offset = buf->offset;
   mem.size = buf->size;

   ioctl(vmesa->driFd, DRM_IOCTL_VIA_FREEMEM, &mem);
   buf->map = NULL;
}


GLboolean
via_alloc_dma_buffer(struct via_context *vmesa)
{
   drm_via_dma_init_t init;

   vmesa->dma = (GLubyte *) malloc(VIA_DMA_BUFSIZ);
    
   /*
    * Check whether AGP DMA has been initialized.
    */
   memset(&init, 0, sizeof(init));
   init.func = VIA_DMA_INITIALIZED;

   vmesa->useAgp = 
     ( 0 == drmCommandWrite(vmesa->driFd, DRM_VIA_DMA_INIT, 
			     &init, sizeof(init)));
   if (VIA_DEBUG & DEBUG_DMA) {
      if (vmesa->useAgp) 
         fprintf(stderr, "unichrome_dri.so: Using AGP.\n");
      else
         fprintf(stderr, "unichrome_dri.so: Using PCI.\n");
   }
      
   return ((vmesa->dma) ? GL_TRUE : GL_FALSE);
}

void
via_free_dma_buffer(struct via_context *vmesa)
{
    if (!vmesa) return;
    free(vmesa->dma);
    vmesa->dma = 0;
} 


/* These functions now allocate and free the via_tex_buffer struct as well:
 */
struct via_tex_buffer *
via_alloc_texture(struct via_context *vmesa,
		  GLuint size,
		  GLuint memType)
{
   struct via_tex_buffer *t = CALLOC_STRUCT(via_tex_buffer);
   
   if (!t)
      goto cleanup;

   t->size = size;
   t->memType = memType;
   insert_at_tail(&vmesa->tex_image_list[memType], t);

   if (t->memType == VIA_MEM_AGP || 
       t->memType == VIA_MEM_VIDEO) {
      drm_via_mem_t fb;

      fb.context = vmesa->hHWContext;
      fb.size = t->size;
      fb.type = t->memType;
      fb.offset = 0;
      fb.index = 0;

      if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_ALLOCMEM, &fb) != 0 || 
	  fb.index == 0) 
	 goto cleanup;

      if (0)
	 fprintf(stderr, "offset %lx index %lx\n", fb.offset, fb.index);

      t->offset = fb.offset;
      t->index = fb.index;
      
      if (t->memType == VIA_MEM_AGP) {
	 t->bufAddr = (GLubyte *)((unsigned long)vmesa->viaScreen->agpLinearStart +
				  fb.offset); 	
	 t->texBase = vmesa->agpBase + fb.offset;
      }
      else {
	 t->bufAddr = (GLubyte *)((unsigned long)vmesa->driScreen->pFB + fb.offset);
	 t->texBase = fb.offset;
      }

      vmesa->total_alloc[t->memType] += t->size;
      return t;
   }
   else if (t->memType == VIA_MEM_SYSTEM) {
      
      t->bufAddr = _mesa_malloc(t->size);      
      if (!t->bufAddr)
	 goto cleanup;

      vmesa->total_alloc[t->memType] += t->size;
      return t;
   }

 cleanup:
   if (t) {
      remove_from_list(t);
      FREE(t);
   }

   return NULL;
}


static void
via_do_free_texture(struct via_context *vmesa, struct via_tex_buffer *t)
{
   drm_via_mem_t fb;

   remove_from_list( t );

   vmesa->total_alloc[t->memType] -= t->size;

   fb.context = vmesa->hHWContext;
   fb.index = t->index;
   fb.offset = t->offset;
   fb.type = t->memType;
   fb.size = t->size;

   if (ioctl(vmesa->driFd, DRM_IOCTL_VIA_FREEMEM, &fb)) {
      fprintf(stderr, "via_free_texture fail\n");
   }

   FREE(t);
}


/* Release textures which were potentially still being referenced by
 * hardware at the time when they were originally freed.
 */
void 
via_release_pending_textures( struct via_context *vmesa )
{
   struct via_tex_buffer *s, *tmp;
   
   foreach_s( s, tmp, &vmesa->freed_tex_buffers ) {
      if (!VIA_GEQ_WRAP(s->lastUsed, vmesa->lastBreadcrumbRead)) {
	 if (VIA_DEBUG & DEBUG_TEXTURE)
	    fprintf(stderr, "%s: release tex sz %d lastUsed %x\n",
		    __FUNCTION__, s->size, s->lastUsed); 
	 via_do_free_texture(vmesa, s);
      }
   }
}
      


void
via_free_texture(struct via_context *vmesa, struct via_tex_buffer *t)
{
   if (!t) {
      return;
   }
   else if (t->memType == VIA_MEM_SYSTEM) {
      remove_from_list(t);
      vmesa->total_alloc[t->memType] -= t->size;
      _mesa_free(t->bufAddr);
      _mesa_free(t);
   }
   else if (t->index && viaCheckBreadcrumb(vmesa, t->lastUsed)) {
      via_do_free_texture( vmesa, t );
   }
   else {
      /* Close current breadcrumb so that we can free this eventually:
       */
      if (t->lastUsed == vmesa->lastBreadcrumbWrite) 
	 viaEmitBreadcrumb(vmesa);

      move_to_tail( &vmesa->freed_tex_buffers, t );
   }
}
