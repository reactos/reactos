/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_texmem.c,v 1.5 2002/11/05 17:46:07 tsi Exp $ */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "colormac.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"

#include "mm.h"
#include "glint_dri.h"
#include "gamma_context.h"
#include "gamma_lock.h"

void gammaDestroyTexObj(gammaContextPtr gmesa, gammaTextureObjectPtr t)
{
   if (!t) return;

   /* This is sad - need to sync *in case* we upload a texture
    * to this newly free memory...
    */
   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (gmesa && t->age > gmesa->dirtyAge)
	 gmesa->dirtyAge = t->age;
   }

   if (t->globj)
      t->globj->DriverData = 0;

   if (gmesa) {
      if (gmesa->CurrentTexObj[0] == t) {
         gmesa->CurrentTexObj[0] = 0;
         gmesa->dirty &= ~GAMMA_UPLOAD_TEX0;
      }

#if 0
      if (gmesa->CurrentTexObj[1] == t) {
         gmesa->CurrentTexObj[1] = 0;
         gmesa->dirty &= ~GAMMA_UPLOAD_TEX1;
      }
#endif
   }

   remove_from_list(t);
   free(t);
}


void gammaSwapOutTexObj(gammaContextPtr gmesa, gammaTextureObjectPtr t)
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (t->age > gmesa->dirtyAge)
	 gmesa->dirtyAge = t->age;
   }

   t->dirty_images = ~0;
   move_to_tail(&(gmesa->SwappedOut), t);
}



/* Upload an image from mesa's internal copy.
 */
static void gammaUploadTexLevel( gammaContextPtr gmesa, gammaTextureObjectPtr t, int level )
{
   const struct gl_texture_image *image = t->image[level].image;
   int i,j;
   int l2d;
#if 0
   int offset = 0;
#endif
   int words, depthLog2;

   /* fprintf(stderr, "%s\n", __FUNCTION__);  */

   l2d = 5; /* 32bits per texel == 1<<5 */

   if (level == 0) {
      t->TextureAddressMode &= ~(TAM_WidthMask | TAM_HeightMask);
      t->TextureAddressMode |= (image->WidthLog2 << 9) | 
			       (image->HeightLog2 << 13);
      t->TextureReadMode &= ~(TRM_WidthMask | TRM_HeightMask | 
			      TRM_DepthMask | TRM_Border |
			      TRM_Patch);
      t->TextureReadMode |= (image->WidthLog2 << 1) | 
			    (image->HeightLog2 << 5) | 
			    (l2d << 9);
      t->TextureFormat &= ~(TF_CompnentsMask | TF_OneCompFmt_Mask);
   }

   t->TextureBaseAddr[level] = /* ??? */
	(unsigned long)(t->image[level].offset + t->BufAddr) << 5;

   CALC_LOG2(depthLog2, 1<<l2d);
   words = (image->Width * image->Height) >> (5-depthLog2);

   CHECK_DMA_BUFFER(gmesa, 3);
   WRITE(gmesa->buf, LBWindowBase, t->TextureBaseAddr[level] >> 5);
   WRITE(gmesa->buf, TextureCacheControl, (TCC_Enable | TCC_Invalidate));
   WRITE(gmesa->buf, WaitForCompletion, 0);
   FLUSH_DMA_BUFFER(gmesa);

   switch (t->image[level].internalFormat) {
   case GL_RGB:
   case 3:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_3;
      
#if 0 /* This is the texture download code we SHOULD be using */
      /* In the routines below, but this causes an DMA overrun - WHY ? */
      while (offset < words) {
         int count = gmesa->bufSize;
	 int i;
	 count -= 3;
         if (count > words-offset) count = words-offset;

	 gmesa->buf->i = GlintTextureDownloadOffsetTag;
	 gmesa->buf++;
	 gmesa->buf->i = offset;
	 gmesa->buf++;
         gmesa->buf->i = (GlintTextureDataTag | ((count-1) << 16));
	 gmesa->buf++;

	 for (i = 0; i < count; i++) {
		gmesa->buf->i = PACK_COLOR_565(src[0],src[1],src[2]);
	 	gmesa->buf++;
		src += 3;
	 }

	 gmesa->bufCount = count+3; /* texture data + 3 values */
	 offset += count;

         FLUSH_DMA_BUFFER(gmesa);
      }   
#else
	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */

      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_565(src[0],src[1],src[2]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src += 3;
      }
      FLUSH_DMA_BUFFER(gmesa);
#endif
   }
   break;

   case GL_RGBA:
   case 4:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_4;

	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_8888(src[0],src[1],src[2],src[3]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src += 4;
      }
      FLUSH_DMA_BUFFER(gmesa);
   }
   break;

   case GL_LUMINANCE:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_1 | TF_OneCompFmt_Lum;

	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_888(src[0],src[0],src[0]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src ++;
      }
      FLUSH_DMA_BUFFER(gmesa);
   }
   break;

   case GL_INTENSITY:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_1 | TF_OneCompFmt_Intensity;

	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_8888(src[0],src[0],src[0],src[0]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src ++;
      }
      FLUSH_DMA_BUFFER(gmesa);
   }
   break;

   case GL_LUMINANCE_ALPHA:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_2;

	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_8888(src[0],src[0],src[0],src[1]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src += 2;
      }
      FLUSH_DMA_BUFFER(gmesa);
   }
   break;

   case GL_ALPHA:
   {
      GLubyte  *src = (GLubyte *)image->Data;

      if (level == 0)
         t->TextureFormat |= TF_Compnents_1 | TF_OneCompFmt_Alpha;

	/* The UGLY way, and SLOW !, but the above sometimes causes
	 * a DMA overrun error ??? FIXME ! */
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, TextureDownloadOffset, 0);
      for (i = 0; i < words; i++) {
 	unsigned int data;
	data = PACK_COLOR_8888(255,255,255,src[0]);
 	CHECK_DMA_BUFFER(gmesa, 1);
 	WRITE(gmesa->buf, TextureData, data);
 	src += 1;
      }
      FLUSH_DMA_BUFFER(gmesa);
   }
   break;

   /* TODO: Translate color indices *now*:
    */
   case GL_COLOR_INDEX:
      {
	 GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[level].offset);
	 GLubyte *src = (GLubyte *)image->Data;

	 for (j = 0 ; j < image->Height ; j++, dst += t->Pitch) {
	    for (i = 0 ; i < image->Width ; i++) {
	       dst[i] = src[0];
	       src += 1;
	    }
	 }
      }
   break;

   default:
      fprintf(stderr, "Not supported texture format %s\n",
              _mesa_lookup_enum_by_nr(image->Format));
   }

   CHECK_DMA_BUFFER(gmesa, 2);
   WRITE(gmesa->buf, WaitForCompletion, 0);
   WRITE(gmesa->buf, LBWindowBase, gmesa->LBWindowBase);
}

void gammaPrintLocalLRU( gammaContextPtr gmesa )
{
   gammaTextureObjectPtr t;
   int sz = 1 << (gmesa->gammaScreen->logTextureGranularity);

   foreach( t, &gmesa->TexObjList ) {
      if (!t->globj)
	 fprintf(stderr, "Placeholder %d at %x sz %x\n",
		 t->MemBlock->ofs / sz,
		 t->MemBlock->ofs,
		 t->MemBlock->size);
      else
	 fprintf(stderr, "Texture at %x sz %x\n",
		 t->MemBlock->ofs,
		 t->MemBlock->size);

   }
}

void gammaPrintGlobalLRU( gammaContextPtr gmesa )
{
   int i, j;
   GAMMATextureRegionPtr list = gmesa->sarea->texList;

   for (i = 0, j = GAMMA_NR_TEX_REGIONS ; i < GAMMA_NR_TEX_REGIONS ; i++) {
      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
	      j, list[j].age, list[j].next, list[j].prev);
      j = list[j].next;
      if (j == GAMMA_NR_TEX_REGIONS) break;
   }

   if (j != GAMMA_NR_TEX_REGIONS)
      fprintf(stderr, "Loop detected in global LRU\n");
}


void gammaResetGlobalLRU( gammaContextPtr gmesa )
{
   GAMMATextureRegionPtr list = gmesa->sarea->texList;
   int sz = 1 << gmesa->gammaScreen->logTextureGranularity;
   int i;

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (GAMMA_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.
    */
   for (i = 0 ; (i+1) * sz <= gmesa->gammaScreen->textureSize ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = GAMMA_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = GAMMA_NR_TEX_REGIONS;
   list[GAMMA_NR_TEX_REGIONS].prev = i;
   list[GAMMA_NR_TEX_REGIONS].next = 0;
   gmesa->sarea->texAge = 0;
}


void gammaUpdateTexLRU( gammaContextPtr gmesa, gammaTextureObjectPtr t )
{
   int i;
   int logsz = gmesa->gammaScreen->logTextureGranularity;
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   GAMMATextureRegionPtr list = gmesa->sarea->texList;

   gmesa->texAge = ++gmesa->sarea->texAge;

   /* Update our local LRU
    */
   move_to_head( &(gmesa->TexObjList), t );

   /* Update the global LRU
    */
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = gmesa->texAge;

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;

      /* insert_at_head(list, i)
       */
      list[i].prev = GAMMA_NR_TEX_REGIONS;
      list[i].next = list[GAMMA_NR_TEX_REGIONS].next;
      list[(unsigned)list[GAMMA_NR_TEX_REGIONS].next].prev = i;
      list[GAMMA_NR_TEX_REGIONS].next = i;
   }
}


/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent
 * the other client's textures.
 */
void gammaTexturesGone( gammaContextPtr gmesa,
		       GLuint offset,
		       GLuint size,
		       GLuint in_use )
{
   gammaTextureObjectPtr t, tmp;

   foreach_s ( t, tmp, &gmesa->TexObjList ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
	 continue;

      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
      gammaSwapOutTexObj( gmesa, t );
   }

   if (in_use) {
      t = (gammaTextureObjectPtr) calloc(1,sizeof(*t));
      if (!t) return;

      t->MemBlock = mmAllocMem( gmesa->texHeap, size, 0, offset);
      insert_at_head( &gmesa->TexObjList, t );
   }

   /* Reload any lost textures referenced by current vertex buffer.
    */
#if 0
   if (gmesa->vertex_buffer) {
      int i, j;

      fprintf(stderr, "\n\nreload tex\n");

      for (i = 0 ; i < gmesa->statenr ; i++) {
	 for (j = 0 ; j < 2 ; j++) {
	    gammaTextureObjectPtr t = gmesa->state_tex[j][i];
	    if (t) {
	       if (t->MemBlock == 0)
		  gammaUploadTexImages( gmesa, t );
	    }
	 }
      }

      /* Hard to do this with the lock held:
       */
/*        GAMMA_FIREVERTICES( gmesa ); */
   }
#endif
}





/* This is called with the lock held.  May have to eject our own and/or
 * other client's texture objects to make room for the upload.
 */
void gammaUploadTexImages( gammaContextPtr gmesa, gammaTextureObjectPtr t )
{
   int i;
   int ofs;
   int numLevels;

   /* /fprintf(stderr, "%s\n", __FUNCTION__); */
#if 0
   LOCK_HARDWARE( gmesa );
#endif

   /* Do we need to eject LRU texture objects?
    */
   if (!t->MemBlock) {
      while (1)
      {
	 t->MemBlock = mmAllocMem( gmesa->texHeap, t->totalSize, 12, 0 );
	 if (t->MemBlock)
	    break;

	 if (gmesa->TexObjList.prev == gmesa->CurrentTexObj[0] ||
	     gmesa->TexObjList.prev == gmesa->CurrentTexObj[1]) {
  	    fprintf(stderr, "Hit bound texture in upload\n");
	    gammaPrintLocalLRU( gmesa );
	    return;
	 }

	 if (gmesa->TexObjList.prev == &(gmesa->TexObjList)) {
 	    fprintf(stderr, "Failed to upload texture, sz %d\n", t->totalSize);
	    mmDumpMemInfo( gmesa->texHeap );
	    return;
	 }

	 gammaSwapOutTexObj( gmesa, gmesa->TexObjList.prev );
      }

      ofs = t->MemBlock->ofs;
      t->BufAddr = (char *)(unsigned long)(gmesa->LBWindowBase + ofs); /* ??? */

      if (t == gmesa->CurrentTexObj[0])
	 gmesa->dirty |= GAMMA_UPLOAD_TEX0;

#if 0
      if (t == gmesa->CurrentTexObj[1])
	 gmesa->dirty |= GAMMA_UPLOAD_TEX1;
#endif

      gammaUpdateTexLRU( gmesa, t );
   }

#if 0
   if (gmesa->dirtyAge >= GET_DISPATCH_AGE(gmesa))
      gammaWaitAgeLocked( gmesa, gmesa->dirtyAge );
#endif

   numLevels = t->lastLevel - t->firstLevel + 1;
   for (i = 0 ; i < numLevels ; i++)
      if (t->dirty_images & (1<<i))
	 gammaUploadTexLevel( gmesa, t, i );

   t->dirty_images = 0;

#if 0
   UNLOCK_HARDWARE( gmesa );
#endif
}
