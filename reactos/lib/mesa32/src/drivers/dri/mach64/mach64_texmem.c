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
#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   if (mmesa)
      mmesa->c_textureSwaps++;
#endif
   if ( !t ) return;

#if 0
   if ( t->tObj && t->memBlock && mmesa ) {
      /* not a placeholder, so release from global LRU if necessary */
      int heap = t->heap;
      drmTextureRegion *list = mmesa->sarea->tex_list[heap];
      int log2sz = mmesa->mach64Screen->logTexGranularity[heap];
      int start = t->memBlock->ofs >> log2sz;
      int end = (t->memBlock->ofs + t->memBlock->size - 1) >> log2sz;
      int i;

      mmesa->lastTexAge[heap] = ++mmesa->sarea->tex_age[heap];

      /* Update the global LRU */
      for ( i = start ; i <= end ; i++ ) {
	 /* do we own this block? */
	 if (list[i].in_use == mmesa->hHWContext) {
	    list[i].in_use = 0;
	    list[i].age = mmesa->lastTexAge[heap];

	    /* remove_from_list(i) */
	    list[(GLuint)list[i].next].prev = list[i].prev;
	    list[(GLuint)list[i].prev].next = list[i].next;
	 }
      }
   }
#endif

   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   if ( t->tObj ) {
      t->tObj->DriverData = NULL;
   }

   if ( t->bound && mmesa )
      mmesa->CurrentTexObj[t->bound-1] = NULL;

   remove_from_list( t );
   FREE( t );
}

/* Keep track of swapped out texture objects.
 */
void mach64SwapOutTexObj( mach64ContextPtr mmesa,
			  mach64TexObjPtr t )
{
#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   if (mmesa)
     mmesa->c_textureSwaps++;
#endif

#if 0
   if ( t->tObj && t->memBlock && mmesa ) {
      /* not a placeholder, so release from global LRU if necessary */
      int heap = t->heap;
      drmTextureRegion *list = mmesa->sarea->tex_list[heap];
      int log2sz = mmesa->mach64Screen->logTexGranularity[heap];
      int start = t->memBlock->ofs >> log2sz;
      int end = (t->memBlock->ofs + t->memBlock->size - 1) >> log2sz;
      int i;

      mmesa->lastTexAge[heap] = ++mmesa->sarea->tex_age[heap];

      /* Update the global LRU */
      for ( i = start ; i <= end ; i++ ) {
	 /* do we own this block? */
	 if (list[i].in_use == mmesa->hHWContext) {
	    list[i].in_use = 0;
	    list[i].age = mmesa->lastTexAge[heap];

	    /* remove_from_list(i) */
	    list[(GLuint)list[i].next].prev = list[i].prev;
	    list[(GLuint)list[i].prev].next = list[i].next;
	 }
      }
   }
#endif

   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   t->dirty = ~0;
   move_to_tail( &mmesa->SwappedOut, t );
}

/* Print out debugging information about texture LRU.
 */
void mach64PrintLocalLRU( mach64ContextPtr mmesa, int heap )
{
   mach64TexObjPtr t;
   int sz = 1 << (mmesa->mach64Screen->logTexGranularity[heap]);

   fprintf( stderr, "\nLocal LRU, heap %d:\n", heap );

   foreach( t, &mmesa->TexObjList[heap] ) {
      if ( !t->tObj ) {
	 fprintf( stderr, "Placeholder %d at 0x%x sz 0x%x\n",
		  t->memBlock->ofs / sz,
		  t->memBlock->ofs,
		  t->memBlock->size );
      } else {
	 fprintf( stderr, "Texture (bound %d) at 0x%x sz 0x%x\n",
		  t->bound,
		  t->memBlock->ofs,
		  t->memBlock->size );
      }
   }

   fprintf( stderr, "\n" );
}

void mach64PrintGlobalLRU( mach64ContextPtr mmesa, int heap )
{
   drm_tex_region_t *list = mmesa->sarea->tex_list[heap];
   int i, j;

   fprintf( stderr, "\nGlobal LRU, heap %d list %p:\n", heap, list );

   for ( i = 0, j = MACH64_NR_TEX_REGIONS ; i < MACH64_NR_TEX_REGIONS ; i++ ) {
      fprintf( stderr, "list[%d] age %d in_use %d next %d prev %d\n",
	       j, list[j].age, list[j].in_use, list[j].next, list[j].prev );
      j = list[j].next;
      if ( j == MACH64_NR_TEX_REGIONS ) break;
   }

   if ( j != MACH64_NR_TEX_REGIONS ) {
      fprintf( stderr, "Loop detected in global LRU\n" );
      for ( i = 0 ; i < MACH64_NR_TEX_REGIONS ; i++ ) {
	 fprintf( stderr, "list[%d] age %d in_use %d next %d prev %d\n",
		  i, list[i].age, list[i].in_use, list[i].next, list[i].prev );
      }
   }

   fprintf( stderr, "\n" );
}

/* Reset the global texture LRU.
 */
/* NOTE: This function is only called while holding the hardware lock */
static void mach64ResetGlobalLRU( mach64ContextPtr mmesa, int heap )
{
   drm_tex_region_t *list = mmesa->sarea->tex_list[heap];
   int sz = 1 << mmesa->mach64Screen->logTexGranularity[heap];
   int i;

   /* (Re)initialize the global circular LRU list.  The last element in
    * the array (MACH64_NR_TEX_REGIONS) is the sentinal.  Keeping it at
    * the end of the array allows it to be addressed rationally when
    * looking up objects at a particular location in texture memory.
    */
   for ( i = 0 ; (i+1) * sz <= mmesa->mach64Screen->texSize[heap] ; i++ ) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
      list[i].in_use = 0;
   }

   i--;
   list[0].prev = MACH64_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = MACH64_NR_TEX_REGIONS;
   list[MACH64_NR_TEX_REGIONS].prev = i;
   list[MACH64_NR_TEX_REGIONS].next = 0;
   mmesa->sarea->tex_age[heap] = 0;
}

/* Update the local and global texture LRUs.
 */
/* NOTE: This function is only called while holding the hardware lock */
void mach64UpdateTexLRU( mach64ContextPtr mmesa,
			 mach64TexObjPtr t )
{
   int heap = t->heap;
   drm_tex_region_t *list = mmesa->sarea->tex_list[heap];
   int log2sz = mmesa->mach64Screen->logTexGranularity[heap];
   int start = t->memBlock->ofs >> log2sz;
   int end = (t->memBlock->ofs + t->memBlock->size - 1) >> log2sz;
   int i;

   mmesa->lastTexAge[heap] = ++mmesa->sarea->tex_age[heap];

   if ( !t->memBlock ) {
      fprintf( stderr, "no memblock\n\n" );
      return;
   }

   /* Update our local LRU */
   move_to_head( &mmesa->TexObjList[heap], t );

   /* Update the global LRU */
   for ( i = start ; i <= end ; i++ ) {
      list[i].in_use = mmesa->hHWContext;
      list[i].age = mmesa->lastTexAge[heap];

#if 0
      /* if this is the last region, it's not in the list */
      if ( !(i*(1<<log2sz) > mmesa->mach64Screen->texSize[heap] ) ) {
#endif
	 /* remove_from_list(i) */
	 list[(GLuint)list[i].next].prev = list[i].prev;
	 list[(GLuint)list[i].prev].next = list[i].next;
#if 0
      }
#endif

      /* insert_at_head(list, i) */
      list[i].prev = MACH64_NR_TEX_REGIONS;
      list[i].next = list[MACH64_NR_TEX_REGIONS].next;
      list[(GLuint)list[MACH64_NR_TEX_REGIONS].next].prev = i;
      list[MACH64_NR_TEX_REGIONS].next = i;
   }

   if ( MACH64_DEBUG & DEBUG_VERBOSE_LRU ) {
      mach64PrintGlobalLRU( mmesa, t->heap );
      mach64PrintLocalLRU( mmesa, t->heap );
   }
}

/* Update our notion of what textures have been changed since we last
 * held the lock.  This pertains to both our local textures and the
 * textures belonging to other clients.  Keep track of other client's
 * textures by pushing a placeholder texture onto the LRU list -- these
 * are denoted by (tObj == NULL).
 */
/* NOTE: This function is only called while holding the hardware lock */
static void mach64TexturesGone( mach64ContextPtr mmesa, int heap,
				int offset, int size, int in_use )
{
   mach64TexObjPtr t, tmp;

   foreach_s ( t, tmp, &mmesa->TexObjList[heap] ) {
      if ( t->memBlock->ofs >= offset + size ||
	   t->memBlock->ofs + t->memBlock->size <= offset )
	 continue;

      /* It overlaps - kick it out.  Need to hold onto the currently
       * bound objects, however.
       */
      if ( t->bound ) {
	 mach64SwapOutTexObj( mmesa, t );
      } else {
	 mach64DestroyTexObj( mmesa, t );
      }
   }

   if ( in_use > 0 && in_use != mmesa->hHWContext ) {
      t = (mach64TexObjPtr) CALLOC( sizeof(*t) );
      if (!t) return;

      t->memBlock = mmAllocMem( mmesa->texHeap[heap], size, 0, offset );
      if ( !t->memBlock ) {
	 fprintf( stderr, "Couldn't alloc placeholder sz %x ofs %x\n",
		  (int)size, (int)offset );
	 mmDumpMemInfo( mmesa->texHeap[heap] );
	 return;
      }
      insert_at_head( &mmesa->TexObjList[heap], t );
   }
}

/* Update our client's shared texture state.  If another client has
 * modified a region in which we have textures, then we need to figure
 * out which of our textures has been removed, and update our global
 * LRU.
 */
void mach64AgeTextures( mach64ContextPtr mmesa, int heap )
{
   drm_mach64_sarea_t *sarea = mmesa->sarea;

   if ( sarea->tex_age[heap] != mmesa->lastTexAge[heap] ) {
      int sz = 1 << mmesa->mach64Screen->logTexGranularity[heap];
      int nr = 0;
      int idx;

      /* Have to go right round from the back to ensure stuff ends up
       * LRU in our local list...  Fix with a cursor pointer.
       */
      for ( idx = sarea->tex_list[heap][MACH64_NR_TEX_REGIONS].prev ;
	    idx != MACH64_NR_TEX_REGIONS && nr < MACH64_NR_TEX_REGIONS ;
	    idx = sarea->tex_list[heap][idx].prev, nr++ )
      {
	 /* If switching texturing schemes, then the SAREA might not
	  * have been properly cleared, so we need to reset the
	  * global texture LRU.
	  */
	 if ( idx * sz > mmesa->mach64Screen->texSize[heap] ) {
	    nr = MACH64_NR_TEX_REGIONS;
	    break;
	 }

	 if ( sarea->tex_list[heap][idx].age > mmesa->lastTexAge[heap] ) {
	    mach64TexturesGone( mmesa, heap, idx * sz, sz,
				sarea->tex_list[heap][idx].in_use );
	 }
      }

      /* If switching texturing schemes, then the SAREA might not
       * have been properly cleared, so we need to reset the
       * global texture LRU.
       */
      if ( nr == MACH64_NR_TEX_REGIONS ) {
	 mach64TexturesGone( mmesa, heap, 0,
			     mmesa->mach64Screen->texSize[heap], 0 );
	 mach64ResetGlobalLRU( mmesa, heap );
      }

      if ( 0 ) {
	 mach64PrintGlobalLRU( mmesa, heap );
	 mach64PrintLocalLRU( mmesa, heap );
      }

      mmesa->dirty |= (MACH64_UPLOAD_CONTEXT |
		       MACH64_UPLOAD_TEX0IMAGE |
		       MACH64_UPLOAD_TEX1IMAGE);
      mmesa->lastTexAge[heap] = sarea->tex_age[heap];
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

   image = t->tObj->Image[0][level];
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
	       (GLuint)t->offset, (GLint)width, dwords );
      mmDumpMemInfo( mmesa->texHeap[t->heap] );
   }

   assert(image->Data);

   {
      CARD32 *dst = (CARD32 *)((char *)mach64Screen->agpTextures.map + t->memBlock->ofs);
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

   image = t->tObj->Image[0][level];
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
   offset = t->offset;

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   mmesa->c_textureBytes += (dwords << 2);
#endif

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "mach64UploadSubImage: %d,%d of %d,%d at %d,%d\n",
	       width, height, image->Width, image->Height, x, y );
      fprintf( stderr, "            blit ofs: 0x%07x pitch: 0x%x dwords: %d\n",
	       (GLuint)offset, (GLint)width, dwords );
      mmDumpMemInfo( mmesa->texHeap[t->heap] );
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
       drmBufPtr buffer;
       CARD32 *dst;

       height = MIN2(remaining, rows);

       /* Grab the dma buffer for the texture blit */
       buffer = mach64GetBufferLocked( mmesa );

       dst = (CARD32 *)((char *)buffer->address + MACH64_HOSTDATA_BLIT_OFFSET);

       assert(image->Data);

       {
          const GLubyte *src = (const GLubyte *) image->Data +
             (y * image->Width + x) * image->TexFormat->TexelBytes;
          const GLuint bytes = width * height * image->TexFormat->TexelBytes;
          memcpy(dst, src, bytes);
       }

       mach64FireBlitLocked( mmesa, buffer, offset, pitch, format,
			     x, y, width, height );

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
   GLint heap;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p )\n",
	       __FUNCTION__, mmesa->glCtx, t );
   }

   assert(t);
   assert(t->tObj);

   /* Choose the heap appropriately */
   heap = MACH64_CARD_HEAP;

   if ( !mmesa->mach64Screen->IsPCI &&
	t->size > mmesa->mach64Screen->texSize[heap] ) {
      heap = MACH64_AGP_HEAP;
   }

   /* Do we need to eject LRU texture objects? */
   if ( !t->memBlock ) {
      t->heap = heap;

      /* Allocate a memory block on a 64-byte boundary */
      t->memBlock = mmAllocMem( mmesa->texHeap[heap], t->size, 6, 0 );

      /* Try AGP before kicking anything out of local mem */
      if ( !mmesa->mach64Screen->IsPCI && !t->memBlock && heap == MACH64_CARD_HEAP ) {
	 t->memBlock = mmAllocMem( mmesa->texHeap[MACH64_AGP_HEAP],
				   t->size, 6, 0 );

	 if ( t->memBlock )
	    heap = t->heap = MACH64_AGP_HEAP;
      }

      /* Kick out textures until the requested texture fits */
      while ( !t->memBlock ) {
	 if ( mmesa->TexObjList[heap].prev->bound ) {
	    fprintf( stderr,
		     "mach64UploadTexImages: ran into bound texture\n" );
	    return;
	 }
	 if ( mmesa->TexObjList[heap].prev == &mmesa->TexObjList[heap] ) {
	    if ( mmesa->mach64Screen->IsPCI ) {
	       fprintf( stderr, "%s: upload texture failure on "
			"local texture heaps, sz=%d\n", __FUNCTION__,
			t->size );
	       return;
	    } else if ( heap == MACH64_CARD_HEAP ) {
	       heap = t->heap = MACH64_AGP_HEAP;
	       continue;
	    } else {
	      int i;
	       fprintf( stderr, "%s: upload texture failure on "
			"%sAGP texture heaps, sz=%d\n", __FUNCTION__,
			mmesa->firstTexHeap == MACH64_CARD_HEAP ? "both local and " : "",
			t->size );
	       for ( i = mmesa->firstTexHeap ; i < mmesa->lastTexHeap ; i++ ) {
		  mach64PrintLocalLRU( mmesa, i );
	          mmDumpMemInfo( mmesa->texHeap[i] );
	       }
	       exit(-1);
	       return;
	    }
	 }

	 mach64SwapOutTexObj( mmesa, mmesa->TexObjList[heap].prev );

	 t->memBlock = mmAllocMem( mmesa->texHeap[heap], t->size, 6, 0 );
      }

      /* Set the base offset of the texture image */
      t->offset = mmesa->mach64Screen->texOffset[heap] + t->memBlock->ofs;

      /* Force loading the new state into the hardware */
      mmesa->dirty |= (MACH64_UPLOAD_SCALE_3D_CNTL |
		       MACH64_UPLOAD_TEXTURE);
   }

   /* Let the world know we've used this memory recently */
   mach64UpdateTexLRU( mmesa, t );

   /* Upload any images that are new */
   if ( t->dirty ) {
      if (t->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t, t->tObj->BaseLevel, 0, 0,
				  t->tObj->Image[0][t->tObj->BaseLevel]->Width,
				  t->tObj->Image[0][t->tObj->BaseLevel]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t, t->tObj->BaseLevel, 0, 0,
				    t->tObj->Image[0][t->tObj->BaseLevel]->Width,
				    t->tObj->Image[0][t->tObj->BaseLevel]->Height );
      }

      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
   }

   mmesa->dirty |= MACH64_UPLOAD_TEXTURE;

   t->dirty = 0;
}

/* The mach64 needs to have both primary and secondary textures in either
 * local or AGP memory, so we need a "buddy system" to make sure that allocation
 * succeeds or fails for both textures.
 * FIXME: This needs to be optimized better.
 */
void mach64UploadMultiTexImages( mach64ContextPtr mmesa, 
				 mach64TexObjPtr t0,
				 mach64TexObjPtr t1 )
{
   GLint heap;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p %p )\n",
	       __FUNCTION__, mmesa->glCtx, t0, t1 );
   }

   assert(t0 && t1);
   assert(t0->tObj && t1->tObj);

   /* Choose the heap appropriately */
   heap = MACH64_CARD_HEAP;

   if ( !mmesa->mach64Screen->IsPCI &&
	((t0->size + t1->size) > mmesa->mach64Screen->texSize[heap]) ) {
      heap = MACH64_AGP_HEAP;
   }

   /* Do we need to eject LRU texture objects? */
   if ( !t0->memBlock || !t1->memBlock || t0->heap != t1->heap ) {
      /* FIXME: starting from scratch for now to keep it simple */
      if ( t0->memBlock ) {
	 mach64SwapOutTexObj( mmesa, t0 );
      }
      if ( t1->memBlock ) {
	 mach64SwapOutTexObj( mmesa, t1 );
      }
      t0->heap = t1->heap = heap;
      /* Allocate a memory block on a 64-byte boundary */
      t0->memBlock = mmAllocMem( mmesa->texHeap[heap], t0->size, 6, 0 );
      if ( t0->memBlock ) {
	 t1->memBlock = mmAllocMem( mmesa->texHeap[heap], t1->size, 6, 0 );
	 if ( !t1->memBlock ) {
	    mmFreeMem( t0->memBlock );
	    t0->memBlock = NULL;
	 }
      }
      /* Try AGP before kicking anything out of local mem */
      if ( (!t0->memBlock || !t1->memBlock) && heap == MACH64_CARD_HEAP ) {
	 t0->memBlock = mmAllocMem( mmesa->texHeap[MACH64_AGP_HEAP], t0->size, 6, 0 );
	 if ( t0->memBlock ) {
	    t1->memBlock = mmAllocMem( mmesa->texHeap[MACH64_AGP_HEAP], t1->size, 6, 0 );
	    if ( !t1->memBlock ) {
	       mmFreeMem( t0->memBlock );
	       t0->memBlock = NULL;
	    }
	 }

	 if ( t0->memBlock && t1->memBlock )
	    heap = t0->heap = t1->heap = MACH64_AGP_HEAP;
      }

      /* Kick out textures until the requested texture fits */
      while ( !t0->memBlock || !t1->memBlock ) {
	 if ( mmesa->TexObjList[heap].prev->bound ) {
	    fprintf( stderr,
		     "%s: ran into bound texture\n", __FUNCTION__ );
	    return;
	 }
	 if ( mmesa->TexObjList[heap].prev == &mmesa->TexObjList[heap] ) {
	    if ( mmesa->mach64Screen->IsPCI ) {
	       fprintf( stderr, "%s: upload texture failure on local "
			"texture heaps, tex0 sz=%d  tex1 sz=%d\n", __FUNCTION__, 
			t0->size, t1->size );
	       return;
	    } else if ( heap == MACH64_CARD_HEAP ) {
	       /* If only one allocation succeeded, start over again in AGP */
	       if (t0->memBlock) {
		  mmFreeMem( t0->memBlock );
	          t0->memBlock = NULL;
	       }
	       if (t1->memBlock) {
		  mmFreeMem( t1->memBlock );
	          t1->memBlock = NULL;
	       }
	       heap = t0->heap = t1->heap = MACH64_AGP_HEAP;
	       continue;
	    } else {
	      int i;
	       fprintf( stderr, "%s: upload texture failure on %s"
			"AGP texture heaps, tex0 sz=%d  tex1 sz=%d\n", __FUNCTION__,
			mmesa->firstTexHeap == MACH64_CARD_HEAP ? "both local and " : "",
			t0->size, t1->size );
	       for ( i = mmesa->firstTexHeap ; i < mmesa->lastTexHeap ; i++ ) {
		  mach64PrintLocalLRU( mmesa, i );
	          mmDumpMemInfo( mmesa->texHeap[i] );
	       }
	       exit(-1);
	       return;
	    }
	 }

	 mach64SwapOutTexObj( mmesa, mmesa->TexObjList[heap].prev );
	 
	 if (!t0->memBlock)
	    t0->memBlock = mmAllocMem( mmesa->texHeap[heap], t0->size, 6, 0 );
	 if (!t1->memBlock)
	    t1->memBlock = mmAllocMem( mmesa->texHeap[heap], t1->size, 6, 0 );
      }

      /* Set the base offset of the texture image */
      t0->offset = mmesa->mach64Screen->texOffset[heap] + t0->memBlock->ofs;
      t1->offset = mmesa->mach64Screen->texOffset[heap] + t1->memBlock->ofs;

      /* Force loading the new state into the hardware */
      mmesa->dirty |= (MACH64_UPLOAD_SCALE_3D_CNTL |
		       MACH64_UPLOAD_TEXTURE);
   }

   /* Let the world know we've used this memory recently */
   mach64UpdateTexLRU( mmesa, t0 );
   mach64UpdateTexLRU( mmesa, t1 );

   /* Upload any images that are new */
   if ( t0->dirty ) {
      if (t0->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t0, t0->tObj->BaseLevel, 0, 0,
				    t0->tObj->Image[0][t0->tObj->BaseLevel]->Width,
				    t0->tObj->Image[0][t0->tObj->BaseLevel]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t0, t0->tObj->BaseLevel, 0, 0,
				    t0->tObj->Image[0][t0->tObj->BaseLevel]->Width,
				    t0->tObj->Image[0][t0->tObj->BaseLevel]->Height );
      }
      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
   }
   if ( t1->dirty ) {
      if (t1->heap == MACH64_AGP_HEAP) {
	 /* Need to make sure any vertex buffers in the queue complete */
	 mach64WaitForIdleLocked( mmesa );
	 mach64UploadAGPSubImage( mmesa, t1, t1->tObj->BaseLevel, 0, 0,
			       t1->tObj->Image[0][t1->tObj->BaseLevel]->Width,
			       t1->tObj->Image[0][t1->tObj->BaseLevel]->Height );
      } else {
	 mach64UploadLocalSubImage( mmesa, t1, t1->tObj->BaseLevel, 0, 0,
			       t1->tObj->Image[0][t1->tObj->BaseLevel]->Width,
			       t1->tObj->Image[0][t1->tObj->BaseLevel]->Height );
      }
      
      mmesa->setup.tex_cntl |= MACH64_TEX_CACHE_FLUSH;
   }

   mmesa->dirty |= MACH64_UPLOAD_TEXTURE;

   t0->dirty = 0;
   t1->dirty = 0;
}
