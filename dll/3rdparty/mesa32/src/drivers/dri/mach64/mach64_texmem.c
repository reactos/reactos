/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
 *                                                Cedar Park, Texas.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Gareth Hughes <gareth@valinux.com>
 *   Leif Delgass <ldelgass@retinalburn.net>
 *   Jose Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "mach64_context.h"
#include "mach64_state.h"
#include "mach64_ioctl.h"
#include "mach64_vb.h"
#include "mach64_tris.h"
#include "mach64_tex.h"

#include "context.h"
#include "macros.h"
#include "simple_list.h"
#include "texformat.h"
#include "imports.h"


/* Destroy hardware state associated with texture `t'.
 */
void mach64DestroyTexObj( mach64ContextPtr mmesa, mach64TexObjPtr t )
{
   unsigned   i;

   /* See if it was the driver's current object.
    */
   if ( mmesa != NULL )
   {
      for ( i = 0 ; i < mmesa->glCtx->Const.MaxTextureUnits ; i++ )
      {
         if ( t == mmesa->CurrentTexObj[ i ] ) {
            assert( t->base.bound & (1 << i) );
            mmesa->CurrentTexObj[ i ] = NULL;
         }
      }
   }
}

/* Upload the texture image associated with texture `t' at level `level'
 * at the address relative to `start'.
 */
static void mach64UploadAGPSubImage( mach64ContextPtr mmesa,
				     mach64TexObjPtr t, int level,
				     int x, int y, int width, int height )
{
   mach64ScreenRec *mach64Screen = mmesa->mach64Screen;
   struct gl_texture_image *image;
   int texelsPerDword = 0;
   int dwords;

   /* Ensure we have a valid texture to upload */
   if ( ( level < 0 ) || ( level > mmesa->glCtx->Const.MaxTextureLevels ) )
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

   dwords = width * height / texelsPerDword;

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   mmesa->c_agpTextureBytes += (dwords << 2);
#endif

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "mach64UploadSubImage: %d,%d of %d,%d at %d,%d\n",
	       width, height, image->Width, image->Height, x, y );
      fprintf( stderr, "            blit ofs: 0x%07x pitch: 0x%x dwords: %d\n",
	       (GLuint)t->bufAddr, (GLint)width, dwords );
   }

   assert(image->Data);

   {
      CARD32 *dst = (CARD32 *)((char *)mach64Screen->agpTextures.map + t->base.memBlock->ofs);
      const GLubyte *src = (const GLubyte *) image->Data +
	 (y * image->Width + x) * image->TexFormat->TexelBytes;
      const GLuint bytes = width * height * image->TexFormat->TexelBytes;
      memcpy(dst, src, bytes);
   }

}

/* Upload the texture image associated with texture `t' at level `level'
 * at the address relative to `start'.
 */
static void mach64UploadLocalSubImage( mach64ContextPtr mmesa,
				  mach64TexObjPtr t, int level,
				  int x, int y, int width, int height )
{
   struct gl_texture_image *image;
   int texelsPerDword = 0;
   int imageWidth, imageHeight;
   int remaining, rows;
   int format, dwords;
   const int maxdwords = (MACH64_BUFFER_MAX_DWORDS - (MACH64_HOSTDATA_BLIT_OFFSET / 4));
   CARD32 pitch, offset;
   int i;

   /* Ensure we have a valid texture to upload */
   if ( ( level < 0 ) || ( level > mmesa->glCtx->Const.MaxTextureLevels ) )
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

   format = t->textureFormat;

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

   /* We can't upload to a pitch less than 64 texels so we will need to
    * linearly upload all modified rows for textures smaller than this.
    * This makes the x/y/width/height different for the blitter and the
    * texture walker.
    */
   if ( imageWidth >= 64 ) {
      /* The texture walker and the blitter look identical */
      pitch = imageWidth >> 3;
   } else {
      int factor;
      int y2;
      int start, end;

      start = (y * imageWidth) & ~63;
      end = (y + height) * imageWidth;

      if ( end - start < 64 ) {
	 /* Handle the case where the total number of texels
	  * uploaded is < 64.
	  */
	 x = 0;
	 y = start / 64;
	 width = end - start;
	 height = 1;
      } else {
	 /* Upload some number of full 64 texel blit rows */
	 factor = 64 / imageWidth;

	 y2 = y + height - 1;
	 y /= factor;
	 y2 /= factor;

	 x = 0;
	 width = 64;
	 height = y2 - y + 1;
      }

      /* Fixed pitch of 64 */
      pitch = 8;
   }

   dwords = width * height / texelsPerDword;
   offset = t->bufAddr;

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   mmesa->c_textureBytes += (dwords << 2);
#endif

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "mach64UploadSubImage: %d,%d of %d,%d at %d,%d\n",
	       width, height, image->Width, image->Height, x, y );
      fprintf( stderr, "            blit ofs: 0x%07x pitch: 0x%x dwords: %d\n",
	       (GLuint)offset, (GLint)width, dwords );
   }

   /* Subdivide the texture if required (account for the registers added by the drm) */
   if ( dwords <= maxdwords ) {
      rows = height;
   } else {
      rows = (maxdwords * texelsPerDword) / (2 * width);
   }

   for ( i = 0, remaining = height ;
	 remaining > 0 ;
	 remaining -= rows, y += rows, i++ )
   {
       height = MIN2(remaining, rows);

       assert(image->Data);

       {
          const GLubyte *src = (const GLubyte *) image->Data +
             (y * image->Width + x) * image->TexFormat->TexelBytes;

          mach64FireBlitLocked( mmesa, (void *)src, offset, pitch, format,
				x, y, width, height );
       }

   }

   mmesa->new_state |= MACH64_NEW_CONTEXT;
   mmesa->dirty |= MACH64_UPLOAD_CONTEXT | MACH64_UPLOAD_MISC;
}


/* Upload the texture images associated with texture `t'.  This might
 * require removing our own and/or other client's texture objects to
 * make room for these images.
 */
void mach64UploadTexImages( mach64ContextPtr mmesa, mach64TexObjPtr t )
{
   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p )\n",
	       __FUNCTION__, mmesa->glCtx, t );
   }

   assert(t);
   assert(t->base.tObj);

   if ( !t->base.memBlock ) {
      int heap;

      /* NULL heaps are skipped */
      heap = driAllocateTexture( mmesa->texture_heaps, MACH64_NR_TEX_HEAPS,
				 (driTextureObject *) t );

      if ( heap == -1 ) {
	 fprintf( stderr, "%s: upload texture failure, sz=%d\n", __FUNCTION__,
		  t->base.totalSize );
	 exit(-1);
	 return;
      }

      t->heap = heap;

      /* Set the base offset of the texture image */
      t->bufAddr = mmesa->mach64Screen->texOffset[heap] + t->base.memBlock->ofs;

      /* Force loading the new state into the hardware */
      mmesa->dirty |= (MACH64_UPLOAD_SCALE_3D_CNTL |
		       MACH64_UPLOAD_TEXTURE);
   }

   /* Let the world know we've used this memory recently */
   driUpdateTextureLRU( (driTextureObject *) t );

   /* Upload any images that are new */
   if ( t->base.dirty_images[0] ) {
      const GLint j = t->base.tObj->BaseLevel;
      if (t->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t, j, 0, 0,
				  t->base.tObj->Image[0][j]->Width,
				  t->base.tObj->Image[0][j]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t, j, 0, 0,
				    t->base.tObj->Image[0][j]->Width,
				    t->base.tObj->Image[0][j]->Height );
      }

      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
      t->base.dirty_images[0] = 0;
   }

   mmesa->dirty |= MACH64_UPLOAD_TEXTURE;
}


/* Allocate memory from the same texture heap `heap' for both textures
 * `u0' and `u1'.
 */
static int mach64AllocateMultiTex( mach64ContextPtr mmesa,
				   mach64TexObjPtr u0,
				   mach64TexObjPtr u1,
				   int heap, GLboolean alloc_u0 )
{
   /* Both objects should be bound */
   assert( u0->base.bound && u1->base.bound );

   if ( alloc_u0 ) {
      /* Evict u0 from its current heap */
      if ( u0->base.memBlock ) {
	 assert( u0->heap != heap );
	 driSwapOutTextureObject( (driTextureObject *) u0 );
      }

      /* Try to allocate u0 in the chosen heap */
      u0->heap = driAllocateTexture( &mmesa->texture_heaps[heap], 1,
				     (driTextureObject *) u0 );

      if ( u0->heap == -1 ) {
	 return -1;
      }
   }

   /* Evict u1 from its current heap */
   if ( u1->base.memBlock ) {
      assert( u1->heap != heap );
      driSwapOutTextureObject( (driTextureObject *) u1 );
   }

   /* Try to allocate u1 in the same heap as u0 */
   u1->heap = driAllocateTexture( &mmesa->texture_heaps[heap], 1,
				  (driTextureObject *) u1 );

   if ( u1->heap == -1 ) {
      return -1;
   }

   /* Bound objects are not evicted */
   assert( u0->base.memBlock && u1->base.memBlock );
   assert( u0->heap == u1->heap );

   return heap;
}

/* The mach64 needs to have both primary and secondary textures in either
 * local or AGP memory, so we need a "buddy system" to make sure that allocation
 * succeeds or fails for both textures.
 */
void mach64UploadMultiTexImages( mach64ContextPtr mmesa, 
				 mach64TexObjPtr t0,
				 mach64TexObjPtr t1 )
{
   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p %p )\n",
	       __FUNCTION__, mmesa->glCtx, t0, t1 );
   }

   assert(t0 && t1);
   assert(t0->base.tObj && t1->base.tObj);

   if ( !t0->base.memBlock || !t1->base.memBlock || t0->heap != t1->heap ) {
      mach64TexObjPtr u0 = NULL;
      mach64TexObjPtr u1 = NULL;
      unsigned totalSize = t0->base.totalSize + t1->base.totalSize;

      int heap, ret;

      /* Check if one of the textures is already swapped in a heap and the
       * other texture fits in that heap.
       */
      if ( t0->base.memBlock && totalSize <= t0->base.heap->size ) {
	 u0 = t0;
	 u1 = t1;
      } else if ( t1->base.memBlock && totalSize <= t1->base.heap->size ) {
	 u0 = t1;
	 u1 = t0;
      }

      if ( u0 ) {
	 heap = u0->heap;

	 ret = mach64AllocateMultiTex( mmesa, u0, u1, heap, GL_FALSE );
      } else {
	 /* Both textures are swapped out or collocation is impossible */
	 u0 = t0;
	 u1 = t1;

	 /* Choose the heap appropriately */
	 heap = MACH64_CARD_HEAP;

	 if ( totalSize > mmesa->texture_heaps[heap]->size ) {
	    heap = MACH64_AGP_HEAP;
	 }

	 ret = mach64AllocateMultiTex( mmesa, u0, u1, heap, GL_TRUE );
      }

      if ( ret == -1 && heap == MACH64_CARD_HEAP ) {
	 /* Try AGP if local memory failed */
	 heap = MACH64_AGP_HEAP;

	 ret = mach64AllocateMultiTex( mmesa, u0, u1, heap, GL_TRUE );
      }

      if ( ret == -1 ) {
	 /* FIXME:
	  * Swap out all textures from the AGP heap and re-run allocation, this
	  * should succeed in all cases.
	  */
	 fprintf( stderr, "%s: upload multi-texture failure, sz0=%d sz1=%d\n",
		  __FUNCTION__, t0->base.totalSize, t1->base.totalSize );
	 exit(-1);
      }

      /* Set the base offset of the texture image */
      t0->bufAddr = mmesa->mach64Screen->texOffset[heap] + t0->base.memBlock->ofs;
      t1->bufAddr = mmesa->mach64Screen->texOffset[heap] + t1->base.memBlock->ofs;

      /* Force loading the new state into the hardware */
      mmesa->dirty |= (MACH64_UPLOAD_SCALE_3D_CNTL |
		       MACH64_UPLOAD_TEXTURE);
   }

   /* Let the world know we've used this memory recently */
   driUpdateTextureLRU( (driTextureObject *) t0 );
   driUpdateTextureLRU( (driTextureObject *) t1 );

   /* Upload any images that are new */
   if ( t0->base.dirty_images[0] ) {
      const GLint j0 = t0->base.tObj->BaseLevel;
      if (t0->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t0, j0, 0, 0,
				    t0->base.tObj->Image[0][j0]->Width,
				    t0->base.tObj->Image[0][j0]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t0, j0, 0, 0,
				    t0->base.tObj->Image[0][j0]->Width,
				    t0->base.tObj->Image[0][j0]->Height );
      }
      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
      t0->base.dirty_images[0] = 0;
   }
   if ( t1->base.dirty_images[0] ) {
      const GLint j1 = t1->base.tObj->BaseLevel;
      if (t1->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t1, j1, 0, 0,
			       t1->base.tObj->Image[0][j1]->Width,
			       t1->base.tObj->Image[0][j1]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t1, j1, 0, 0,
			       t1->base.tObj->Image[0][j1]->Width,
			       t1->base.tObj->Image[0][j1]->Height );
      }
      
      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
      t1->base.dirty_images[0] = 0;
   }

   mmesa->dirty |= MACH64_UPLOAD_TEXTURE;
}
