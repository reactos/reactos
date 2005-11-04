/**************************************************************************

Copyright 2001 2d3d Inc., Delray Beach, FL

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_texmem.c,v 1.3 2002/12/10 01:26:53 dawes Exp $ */

/*
 * Author:
 *   Jeff Hartmann <jhartmann@2d3d.com>
 *
 * Heavily based on the I810 driver, which was written by:
 *   Keith Whitwell <keithw@tungstengraphics.com>
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texformat.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_tex.h"
#include "i830_state.h"
#include "i830_ioctl.h"


void i830DestroyTexObj(i830ContextPtr imesa, i830TextureObjectPtr t)
{
   unsigned   i;


   /* See if it was the driver's current object.
    */
   if ( imesa != NULL ) { 
      for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ ) {
	 if ( t == imesa->CurrentTexObj[ i ] ) {
	    imesa->CurrentTexObj[ i ] = NULL;
	    imesa->dirty &= ~I830_UPLOAD_TEX_N( i );
	 }
      }
   }
}

#if defined(i386) || defined(__i386__)
/* From linux kernel i386 header files, copes with odd sizes better
 * than COPY_DWORDS would:
 */
static __inline__ void * __memcpy(void * to, const void * from, size_t n)
{
int d0, d1, d2;
__asm__ __volatile__(
	"rep ; movsl\n\t"
	"testb $2,%b4\n\t"
	"je 1f\n\t"
	"movsw\n"
	"1:\ttestb $1,%b4\n\t"
	"je 2f\n\t"
	"movsb\n"
	"2:"
	: "=&c" (d0), "=&D" (d1), "=&S" (d2)
	:"0" (n/4), "q" (n),"1" ((long) to),"2" ((long) from)
	: "memory");
return (to);
}
#else
/* Allow compilation on other architectures */
#define __memcpy memcpy
#endif

/* Upload an image from mesa's internal copy.
 */
static void i830UploadTexLevel( i830ContextPtr imesa,
				i830TextureObjectPtr t, int hwlevel )
{
   const struct gl_texture_image *image = t->image[0][hwlevel].image;
   int j;

   if (!image || !image->Data)
      return;

   if (image->IsCompressed) {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[0][hwlevel].offset);
	 GLubyte *src = (GLubyte *)image->Data;

	 if ((t->Setup[I830_TEXREG_TM0S1] & TM0S1_MT_FORMAT_MASK)==MT_COMPRESS_FXT1)
	   {
	     for (j = 0 ; j < image->Height/4 ; j++, dst += (t->Pitch)) {
	       __memcpy(dst, src, (image->Width*2) );
	       src += image->Width*2;
	     }
	   }
	 else if ((t->Setup[I830_TEXREG_TM0S1] & TM0S1_MT_FORMAT_MASK)==MT_COMPRESS_DXT1)
	   {
	     for (j = 0 ; j < image->Height/4 ; j++, dst += (t->Pitch)) {
	       __memcpy(dst, src, (image->Width*2) );
	       src += image->Width*2;
	     }
	   }
	 else if (((t->Setup[I830_TEXREG_TM0S1] & TM0S1_MT_FORMAT_MASK)==MT_COMPRESS_DXT2_3) || ((t->Setup[I830_TEXREG_TM0S1] & TM0S1_MT_FORMAT_MASK)==MT_COMPRESS_DXT4_5))
	   {
	     for (j = 0 ; j < image->Height/4 ; j++, dst += (t->Pitch)) {
	       __memcpy(dst, src, (image->Width*4) );
	       src += image->Width*4;
	     }
	   }
   }
   else if (image->Width * image->TexFormat->TexelBytes == t->Pitch) {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[0][hwlevel].offset);
	 GLubyte *src = (GLubyte *)image->Data;
	 
	 memcpy( dst, src, t->Pitch * image->Height );
   }
   else switch (image->TexFormat->TexelBytes) {
   case 1:
      {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[0][hwlevel].offset);
	 GLubyte *src = (GLubyte *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += t->Pitch) {
	    __memcpy(dst, src, image->Width );
	    src += image->Width;
	 }
      }
      break;

   case 2:
      {
	 GLushort *dst = (GLushort *)(t->BufAddr + t->image[0][hwlevel].offset);
	 GLushort *src = (GLushort *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += (t->Pitch/2)) {
	    __memcpy(dst, src, image->Width * 2 );
	    src += image->Width;
	 }
      }
      break;

   case 4:
      {
	 GLuint *dst = (GLuint *)(t->BufAddr + t->image[0][hwlevel].offset);
	 GLuint *src = (GLuint *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += (t->Pitch/4)) {
	    __memcpy(dst, src, image->Width * 4 );
	    src += image->Width;
	 }
      }
      break;

   default:
      fprintf(stderr, "%s: Not supported texel size %d\n",
	      __FUNCTION__, image->TexFormat->TexelBytes);
   }
}


/* This is called with the lock held.  May have to eject our own and/or
 * other client's texture objects to make room for the upload.
 */

int i830UploadTexImagesLocked( i830ContextPtr imesa, i830TextureObjectPtr t )
{
   int ofs;
   int i;

   if ( t->base.memBlock == NULL ) {
      int heap;

      heap = driAllocateTexture( imesa->texture_heaps, imesa->nr_heaps,
				 (driTextureObject *) t );
      if ( heap == -1 ) {
	 return -1;
      }

      /* Set the base offset of the texture image */
      ofs = t->base.memBlock->ofs;
      t->BufAddr = imesa->i830Screen->tex.map + ofs;
      t->Setup[I830_TEXREG_TM0S0] = (TM0S0_USE_FENCE |
				     (imesa->i830Screen->textureOffset + ofs));

      for ( i = 0 ; i < imesa->glCtx->Const.MaxTextureUnits ; i++ ) {
	 if (t == imesa->CurrentTexObj[i]) {
	     imesa->dirty |= I830_UPLOAD_TEX_N( i );
	 }
      }
   }


   /* Let the world know we've used this memory recently.
    */
   driUpdateTextureLRU( (driTextureObject *) t );

   if (imesa->texture_heaps[0]->timestamp >= GET_DISPATCH_AGE(imesa)) 
      i830WaitAgeLocked( imesa, imesa->texture_heaps[0]->timestamp ); 

   /* Upload any images that are new */
   if (t->base.dirty_images[0]) {
      const int numLevels = t->base.lastLevel - t->base.firstLevel + 1;

      for (i = 0 ; i < numLevels ; i++) { 
         if ( (t->base.dirty_images[0] & (1 << (i+t->base.firstLevel))) != 0 ) {
	    i830UploadTexLevel( imesa, t, i );
         }
      }
      t->base.dirty_images[0] = 0;
      imesa->sarea->perf_boxes |= I830_BOX_TEXTURE_LOAD;
   }

   return 0;
}
