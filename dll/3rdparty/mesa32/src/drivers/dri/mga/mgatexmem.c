/*
 * Copyright 2000-2001 VA Linux Systems, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgatexmem.c,v 1.7 2002/10/30 12:51:36 alanh Exp $ */

#include "glheader.h"

#include "mm.h"
#include "mgacontext.h"
#include "mgatex.h"
#include "mgaregs.h"
#include "mgaioctl.h"
#include "mga_xmesa.h"

#include "imports.h"
#include "simple_list.h"

/**
 * Destroy any device-dependent state associated with the texture.  This may
 * include NULLing out hardware state that points to the texture.
 */
void
mgaDestroyTexObj( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
    unsigned   i;


    /* See if it was the driver's current object.
     */

    if ( mmesa != NULL )
    { 
	if ( t->age > mmesa->dirtyAge )
	    mmesa->dirtyAge = t->age;

	for ( i = 0 ; i < mmesa->glCtx->Const.MaxTextureUnits ; i++ )
	{
	    if ( t == mmesa->CurrentTexObj[ i ] ) {
		mmesa->CurrentTexObj[ i ] = NULL;
	    }
	}
    }
}


/**
 * Upload a texture image from system memory to either on-card or AGP
 * memory.  Uploads to on-card memory are performed using an ILOAD operation.
 * This is used for both initial loading of the entire image, and texSubImage
 * updates.
 *
 * Performed with the hardware lock held.
 * 
 * Even though this function is named "upload subimage," the entire image
 * is uploaded.
 * 
 * \param mmesa  Driver context.
 * \param t      Texture to be uploaded.
 * \param hwlevel  Mipmap level of the texture to be uploaded.
 * 
 * \bug As mentioned above, this fuction actually copies the entier mipmap
 *      level.  There should be a version of this function that performs
 *      sub-rectangle uploads.  This will perform quite a bit better if only
 *      a small portion of a larger texture has been updated.  Care would
 *      need to be take with such an implementation once glCopyTexImage has
 *      been hardware accelerated.
 */
static void mgaUploadSubImage( mgaContextPtr mmesa,
			       mgaTextureObjectPtr t, GLint hwlevel )
{
   struct gl_texture_image * texImage;
   unsigned     offset;
   unsigned     texelBytes;
   unsigned     length;
   const int level = hwlevel + t->base.firstLevel;


   if ( (hwlevel < 0) 
	|| (hwlevel >= (MGA_IS_G200(mmesa) 
		      ? G200_TEX_MAXLEVELS : G400_TEX_MAXLEVELS)) ) {
      fprintf( stderr, "[%s:%d] level = %d\n", __FILE__, __LINE__, level );
      return;
   }

   texImage = t->base.tObj->Image[0][level];
   if ( texImage == NULL ) {
      fprintf( stderr, "[%s:%d] Image[%d] = NULL\n", __FILE__, __LINE__,
	       level );
      return;
   }


   if (texImage->Data == NULL) {
      fprintf(stderr, "null texture image data tObj %p level %d\n",
	      (void *) t->base.tObj, level);
      return;
   }


   /* find the proper destination offset for this level */
   if ( MGA_IS_G200(mmesa) ) {
      offset = (t->base.memBlock->ofs + t->offsets[hwlevel]);
   }
   else {
      unsigned  i;

      offset = t->base.memBlock->ofs;
      for ( i = 0 ; i < hwlevel ; i++ ) {
	 offset += (t->offsets[1] >> (i * 2));
      }
   }


   /* Copy the texture from system memory to a memory space that can be
    * directly used by the hardware for texturing.
    */

   texelBytes = texImage->TexFormat->TexelBytes;
   length = texImage->Width * texImage->Height * texelBytes;
   if ( t->base.heap->heapId == MGA_CARD_HEAP ) {
      unsigned  tex_offset = 0;
      unsigned  to_copy;


      /* We may not be able to upload the entire texture in one batch due to
       * register limits or dma buffer limits.  Split the copy up into maximum
       * sized chunks.
       */

      offset += mmesa->mgaScreen->textureOffset[ t->base.heap->heapId ];
      while ( length != 0 ) {
	 mgaGetILoadBufferLocked( mmesa );

	 /* The kernel ILOAD ioctl requires that the lenght be an even multiple
	  * of MGA_ILOAD_ALIGN.
	  */
	 length = ((length) + MGA_ILOAD_MASK) & ~MGA_ILOAD_MASK;

	 to_copy = MIN2( length, MGA_BUFFER_SIZE );
	 (void) memcpy( mmesa->iload_buffer->address,
			(GLubyte *) texImage->Data + tex_offset, to_copy );

	 if ( MGA_DEBUG & DEBUG_VERBOSE_TEXTURE )
	     fprintf(stderr, "[%s:%d] address/size = 0x%08lx/%d\n",
		     __FILE__, __LINE__,
		     (long) (offset + tex_offset),
		     to_copy );

	 mgaFireILoadLocked( mmesa, offset + tex_offset, to_copy );
	 tex_offset += to_copy;
	 length -= to_copy;
      }
   } else {
      /* FIXME: the sync for direct copy reduces speed.. */
      /* This works, is slower for uploads to card space and needs
       * additional synchronization with the dma stream.
       */
       
      UPDATE_LOCK(mmesa, DRM_LOCK_FLUSH | DRM_LOCK_QUIESCENT);

      memcpy( mmesa->mgaScreen->texVirtual[t->base.heap->heapId] + offset,
	      texImage->Data, length );

      if ( MGA_DEBUG & DEBUG_VERBOSE_TEXTURE )
	 fprintf(stderr, "[%s:%d] address/size = 0x%08lx/%d\n",
		 __FILE__, __LINE__,
		 (long) (mmesa->mgaScreen->texVirtual[t->base.heap->heapId] 
			 + offset),
		 length);
   }
}


/**
 * Upload the texture images associated with texture \a t.  This might
 * require the allocation of texture memory.
 * 
 * \param mmesa Context pointer
 * \param t Texture to be uploaded
 */

int mgaUploadTexImages( mgaContextPtr mmesa, mgaTextureObjectPtr t )
{
   int i;
   int ofs;


   if ( (t == NULL) || (t->base.totalSize == 0) )
      return 0;

   LOCK_HARDWARE( mmesa );

   if (t->base.memBlock == NULL ) {
      int heap;

      heap = driAllocateTexture( mmesa->texture_heaps, mmesa->nr_heaps,
				 (driTextureObject *) t );
      if ( heap == -1 ) {
	 UNLOCK_HARDWARE( mmesa );
	 return -1;
      }

      ofs = mmesa->mgaScreen->textureOffset[ heap ]
	   + t->base.memBlock->ofs;

      if ( MGA_IS_G200(mmesa) ) {
	 t->setup.texorg  = ofs;
	 t->setup.texorg1 = ofs + t->offsets[1];
	 t->setup.texorg2 = ofs + t->offsets[2];
	 t->setup.texorg3 = ofs + t->offsets[3];
	 t->setup.texorg4 = ofs + t->offsets[4];
      }
      else {
	 t->setup.texorg  = ofs | TO_texorgoffsetsel;
	 t->setup.texorg1 = t->offsets[1];
	 t->setup.texorg2 = 0;
	 t->setup.texorg3 = 0;
	 t->setup.texorg4 = 0;
      }

      mmesa->dirty |= MGA_UPLOAD_CONTEXT;
   }

   /* Let the world know we've used this memory recently.
    */
   driUpdateTextureLRU( (driTextureObject *) t );

   if (MGA_DEBUG&DEBUG_VERBOSE_TEXTURE)
      fprintf(stderr, "[%s:%d] dispatch age: %d age freed memory: %d\n",
	      __FILE__, __LINE__,
	      GET_DISPATCH_AGE(mmesa), mmesa->dirtyAge);

   if (mmesa->dirtyAge >= GET_DISPATCH_AGE(mmesa))
      mgaWaitAgeLocked( mmesa, mmesa->dirtyAge );

   if (t->base.dirty_images[0]) {
      const int numLevels = t->base.lastLevel - t->base.firstLevel + 1;

      if (MGA_DEBUG&DEBUG_VERBOSE_TEXTURE)
	 fprintf(stderr, "[%s:%d] dirty_images[0] = 0x%04x\n",
		 __FILE__, __LINE__, t->base.dirty_images[0] );

      for (i = 0 ; i < numLevels ; i++) {
	 if ( (t->base.dirty_images[0] & (1U << i)) != 0 ) {
	    mgaUploadSubImage( mmesa, t, i );
	 }
      }
      t->base.dirty_images[0] = 0;
   }


   UNLOCK_HARDWARE( mmesa );

   return 0;
}
