/*
 * GLX Hardware Device Driver for Intel i810
 * Copyright (C) 1999 Keith Whitwell
 * Texmem interface changes (C) 2003 Dave Airlie
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * KEITH WHITWELL, OR ANY OTHER CONTRIBUTORS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "colormac.h"
#include "mm.h"
#include "texformat.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810tex.h"
#include "i810state.h"
#include "i810ioctl.h"


void i810DestroyTexObj(i810ContextPtr imesa, i810TextureObjectPtr t)
{
   /* See if it was the driver's current object.
    */
   if ( imesa != NULL ) { 
     if (imesa->CurrentTexObj[0] == t) {
       imesa->CurrentTexObj[0] = 0;
       imesa->dirty &= ~I810_UPLOAD_TEX0;
     }
     
     if (imesa->CurrentTexObj[1] == t) {
       imesa->CurrentTexObj[1] = 0;
       imesa->dirty &= ~I810_UPLOAD_TEX1;
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
static void i810UploadTexLevel( i810ContextPtr imesa,
				i810TextureObjectPtr t, int hwlevel )
{
   const struct gl_texture_image *image = t->image[hwlevel].image;
   int j;

   if (!image || !image->Data)
      return;

   if (image->Width * image->TexFormat->TexelBytes == t->Pitch) {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[hwlevel].offset);
	 GLubyte *src = (GLubyte *)image->Data;
	 
	 memcpy( dst, src, t->Pitch * image->Height );
   }
   else switch (image->TexFormat->TexelBytes) {
   case 1:
      {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[hwlevel].offset);
	 GLubyte *src = (GLubyte *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += t->Pitch) {
	    __memcpy(dst, src, image->Width );
	    src += image->Width;
	 }
      }
      break;

   case 2:
      {
	 GLushort *dst = (GLushort *)(t->BufAddr + t->image[hwlevel].offset);
	 GLushort *src = (GLushort *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += (t->Pitch/2)) {
	    __memcpy(dst, src, image->Width * 2 );
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
int i810UploadTexImagesLocked( i810ContextPtr imesa, i810TextureObjectPtr t )
{
   int i;
   int ofs;
   int numLevels;

   /* Do we need to eject LRU texture objects?
    */
   if (!t->base.memBlock) {
      int heap;
       
      heap = driAllocateTexture( imesa->texture_heaps, imesa->nr_heaps,
				 (driTextureObject *) t);
      
      if ( heap == -1 ) {
	return -1;
      }
      
      ofs = t->base.memBlock->ofs;
      t->BufAddr = imesa->i810Screen->tex.map + ofs;
      t->Setup[I810_TEXREG_MI3] = imesa->i810Screen->textureOffset + ofs;
      
      if (t == imesa->CurrentTexObj[0])
	I810_STATECHANGE(imesa, I810_UPLOAD_TEX0);
      
      if (t == imesa->CurrentTexObj[1])
	 I810_STATECHANGE(imesa, I810_UPLOAD_TEX1);
      
       /*      i810UpdateTexLRU( imesa, t );*/
     }
   driUpdateTextureLRU( (driTextureObject *) t );
   
   if (imesa->texture_heaps[0]->timestamp >= GET_DISPATCH_AGE(imesa))
      i810WaitAgeLocked( imesa, imesa->texture_heaps[0]->timestamp );

   numLevels = t->base.lastLevel - t->base.firstLevel + 1;
   for (i = 0 ; i < numLevels ; i++)
      if (t->base.dirty_images[0] & (1<<i))
	 i810UploadTexLevel( imesa, t, i );

   t->base.dirty_images[0] = 0;

   return 0;
}  
