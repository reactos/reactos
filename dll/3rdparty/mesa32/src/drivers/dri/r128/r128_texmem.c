/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_texmem.c,v 1.1 2002/02/22 21:44:58 dawes Exp $ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
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
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Kevin E. Martin <martin@valinux.com>
 *   Brian Paul <brianp@valinux.com>
 */

#include "r128_context.h"
#include "r128_state.h"
#include "r128_ioctl.h"
#include "r128_tris.h"
#include "r128_tex.h"

#include "context.h"
#include "macros.h"
#include "simple_list.h"
#include "texformat.h"
#include "imports.h"

#define TEX_0	1
#define TEX_1	2


/* Destroy hardware state associated with texture `t'.
 */
void r128DestroyTexObj( r128ContextPtr rmesa, r128TexObjPtr t )
{
    unsigned   i;


    /* See if it was the driver's current object.
     */

    if ( rmesa != NULL )
    { 
	for ( i = 0 ; i < rmesa->glCtx->Const.MaxTextureUnits ; i++ )
	{
	    if ( t == rmesa->CurrentTexObj[ i ] ) {
		assert( t->base.bound & (1 << i) );
		rmesa->CurrentTexObj[ i ] = NULL;
	    }
	}
    }
}


/**
 * Upload the texture image associated with texture \a t at the specified
 * level at the address relative to \a start.
 */
static void uploadSubImage( r128ContextPtr rmesa, r128TexObjPtr t,
			    GLint level,
			    GLint x, GLint y, GLint width, GLint height )
{
   struct gl_texture_image *image;
   int texelsPerDword = 0;
   int imageWidth, imageHeight;
   int remaining, rows;
   int format, dwords;
   u_int32_t pitch, offset;
   int i;

   /* Ensure we have a valid texture to upload */
   if ( ( level < 0 ) || ( level > R128_MAX_TEXTURE_LEVELS ) )
      return;

   image = t->base.tObj->Image[0][level];
   if ( !image )
      return;

   switch ( image->TexFormat->TexelBytes ) {
   case 1: texelsPerDword = 4; break;
   case 2: texelsPerDword = 2; break;
   case 4: texelsPerDword = 1; break;
   }

#if 1
   /* FIXME: The subimage index calcs are wrong... */
   x = 0;
   y = 0;
   width = image->Width;
   height = image->Height;
#endif

   imageWidth  = image->Width;
   imageHeight = image->Height;

   format = t->textureFormat >> 16;

   /* The texel upload routines have a minimum width, so force the size
    * if needed.
    */
   if ( imageWidth < texelsPerDword ) {
      int factor;

      factor = texelsPerDword / imageWidth;
      imageWidth = texelsPerDword;
      imageHeight /= factor;
      if ( imageHeight == 0 ) {
	 /* In this case, the texel converter will actually walk a
	  * texel or two off the end of the image, but normal malloc
	  * alignment should prevent it from ever causing a fault.
	  */
	 imageHeight = 1;
      }
   }

   /* We can't upload to a pitch less than 8 texels so we will need to
    * linearly upload all modified rows for textures smaller than this.
    * This makes the x/y/width/height different for the blitter and the
    * texture walker.
    */
   if ( imageWidth >= 8 ) {
      /* The texture walker and the blitter look identical */
      pitch = imageWidth >> 3;
   } else {
      int factor;
      int y2;
      int start, end;

      start = (y * imageWidth) & ~7;
      end = (y + height) * imageWidth;

      if ( end - start < 8 ) {
	 /* Handle the case where the total number of texels
	  * uploaded is < 8.
	  */
	 x = 0;
	 y = start / 8;
	 width = end - start;
	 height = 1;
      } else {
	 /* Upload some number of full 8 texel blit rows */
	 factor = 8 / imageWidth;

	 y2 = y + height - 1;
	 y /= factor;
	 y2 /= factor;

	 x = 0;
	 width = 8;
	 height = y2 - y + 1;
      }

      /* Fixed pitch of 8 */
      pitch = 1;
   }

   dwords = width * height / texelsPerDword;
   offset = t->bufAddr + t->image[level - t->base.firstLevel].offset;

#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   rmesa->c_textureBytes += (dwords << 2);
#endif

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "r128UploadSubImage: %d,%d of %d,%d at %d,%d\n",
	       width, height, image->Width, image->Height, x, y );
      fprintf( stderr, "          blit ofs: 0x%07x pitch: 0x%x dwords: %d "
	       "level: %d format: %x\n",
	       (GLuint)offset, (GLuint)pitch, dwords, level, format );
   }

   /* Subdivide the texture if required */
   if ( dwords <= R128_BUFFER_MAX_DWORDS / 2 ) {
      rows = height;
   } else {
      rows = (R128_BUFFER_MAX_DWORDS * texelsPerDword) / (2 * width);
   }

   for ( i = 0, remaining = height ;
	 remaining > 0 ;
	 remaining -= rows, y += rows, i++ )
   {
      u_int32_t *dst;
      drmBufPtr buffer;

      assert(image->Data);

      height = MIN2(remaining, rows);

      /* Grab the indirect buffer for the texture blit */
      LOCK_HARDWARE( rmesa );
      buffer = r128GetBufferLocked( rmesa );

      dst = (u_int32_t *)((char *)buffer->address + R128_HOSTDATA_BLIT_OFFSET);

      /* Copy the next chunck of the texture image into the blit buffer */
      {
         const GLubyte *src = (const GLubyte *) image->Data +
            (y * image->Width + x) * image->TexFormat->TexelBytes;
         const GLuint bytes = width * height * image->TexFormat->TexelBytes;
         memcpy(dst, src, bytes);
      }

      r128FireBlitLocked( rmesa, buffer,
			  offset, pitch, format,
			  x, y, width, height );
      UNLOCK_HARDWARE( rmesa );
   }

   rmesa->new_state |= R128_NEW_CONTEXT;
   rmesa->dirty |= R128_UPLOAD_CONTEXT | R128_UPLOAD_MASKS;
}


/* Upload the texture images associated with texture `t'.  This might
 * require removing our own and/or other client's texture objects to
 * make room for these images.
 */
void r128UploadTexImages( r128ContextPtr rmesa, r128TexObjPtr t )
{
   const GLint numLevels = t->base.lastLevel - t->base.firstLevel + 1;
   GLint i;

   if ( R128_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p )\n",
	       __FUNCTION__, (void *) rmesa->glCtx, (void *) t );
   }

   assert(t);

   LOCK_HARDWARE( rmesa );

   if ( !t->base.memBlock ) {
      int heap;


      heap = driAllocateTexture( rmesa->texture_heaps, rmesa->nr_heaps,
				 (driTextureObject *) t );
      if ( heap == -1 ) {
	 UNLOCK_HARDWARE( rmesa );
	 return;
      }

      /* Set the base offset of the texture image */
      t->bufAddr = rmesa->r128Screen->texOffset[heap] 
	   + t->base.memBlock->ofs;

      /* Set texture offsets for each mipmap level */
      if ( t->setup.tex_cntl & R128_MIP_MAP_DISABLE ) {
	 for ( i = 0 ; i < R128_MAX_TEXTURE_LEVELS ; i++ ) {
	    t->setup.tex_offset[i] = t->bufAddr;
	 }
      } else {
         for ( i = 0; i < numLevels; i++ ) {
            const int j = numLevels - i - 1;
            t->setup.tex_offset[j] = t->bufAddr + t->image[i].offset;
         }
      }
   }

   /* Let the world know we've used this memory recently.
    */
   driUpdateTextureLRU( (driTextureObject *) t );
   UNLOCK_HARDWARE( rmesa );

   /* Upload any images that are new */
   if ( t->base.dirty_images[0] ) {
      for ( i = 0 ; i < numLevels; i++ ) {
         const GLint j = t->base.firstLevel + i;  /* the texObj's level */
	 if ( t->base.dirty_images[0] & (1 << j) ) {
	    uploadSubImage( rmesa, t, j, 0, 0,
			    t->image[i].width, t->image[i].height );
	 }
      }

      rmesa->setup.tex_cntl_c |= R128_TEX_CACHE_FLUSH;
      rmesa->dirty |= R128_UPLOAD_CONTEXT;
      t->base.dirty_images[0] = 0;
   }
}
