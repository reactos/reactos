/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include <stdlib.h>
#include <stdio.h>

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"

#include "mm.h"
#include "s3v_context.h"
#include "s3v_lock.h"
#include "s3v_tex.h"

void s3vSwapOutTexObj(s3vContextPtr vmesa, s3vTextureObjectPtr t);
void s3vUpdateTexLRU( s3vContextPtr vmesa, s3vTextureObjectPtr t );


void s3vDestroyTexObj(s3vContextPtr vmesa, s3vTextureObjectPtr t)
{
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vDestroyTexObj: #%i ***\n", ++times));
#endif

   if (!t) return;

/* FIXME: useful? */
#if _TEXFLUSH
	if (vmesa)
		DMAFLUSH();
#endif

   /* This is sad - need to sync *in case* we upload a texture
    * to this newly free memory...
    */
   if (t->MemBlock) {
      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (vmesa && t->age > vmesa->dirtyAge)
	     vmesa->dirtyAge = t->age;
   }

   if (t->globj)
      t->globj->DriverData = NULL;

   if (vmesa) {
      if (vmesa->CurrentTexObj[0] == t) {
         	vmesa->CurrentTexObj[0] = 0;
        	vmesa->dirty &= ~S3V_UPLOAD_TEX0;
      }

#if 0
      if (vmesa->CurrentTexObj[1] == t) {
         vmesa->CurrentTexObj[1] = 0;
         vmesa->dirty &= ~S3V_UPLOAD_TEX1;
      }
#endif
   }

   remove_from_list(t);
   FREE(t);
}


void s3vSwapOutTexObj(s3vContextPtr vmesa, s3vTextureObjectPtr t)
{
/*   int i; */
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vSwapOutTexObj: #%i ***\n", ++times));
#endif

   if (t->MemBlock) {

      mmFreeMem(t->MemBlock);
      t->MemBlock = 0;

      if (t->age > vmesa->dirtyAge)
         vmesa->dirtyAge = t->age;
   
      t->dirty_images = ~0; 
      move_to_tail(&(vmesa->SwappedOut), t);
   }
}


/* Upload an image from mesa's internal copy.
 */

static void s3vUploadTexLevel( s3vContextPtr vmesa, s3vTextureObjectPtr t,
				int level )
{
	__DRIscreenPrivate *sPriv = vmesa->driScreen;
	const struct gl_texture_image *image = t->image[level].image;
	int i,j;
	int l2d;
	/* int offset = 0; */
	int words;
	GLuint* dest;
#if TEX_DEBUG_ON
	static unsigned int times=0;
#endif
	if ( !image ) return;
	if (image->Data == 0) return;

	DEBUG_TEX(("*** s3vUploadTexLevel: #%i ***\n", ++times));
	DEBUG_TEX(("level = %i\n", level));

	l2d = 5; /* 32bits per texel == 1<<5 */
/*
	if (level == 0) 
		;
*/
	DEBUG_TEX(("t->image[%i].offset = 0x%x\n",
		level, t->image[level].offset));
		
	t->TextureBaseAddr[level] = (GLuint)(t->BufAddr + t->image[level].offset
		+ _TEXALIGN) & (GLuint)(~_TEXALIGN);
	dest = (GLuint*)(sPriv->pFB + t->TextureBaseAddr[level]); 

	DEBUG_TEX(("sPriv->pFB = 0x%x\n", sPriv->pFB));
	DEBUG_TEX(("dest = 0x%x\n", dest));
	DEBUG_TEX(("dest - sPriv->pFB = 0x%x\n", ((int)dest - (int)sPriv->pFB)));

	/* NOTE: we implicitly suppose t->texelBytes == 2 */

	words = (image->Width * image->Height) >> 1;

	DEBUG_TEX(("\n\n"));

	switch (t->image[level].internalFormat) {
	case GL_RGB:
	case 3:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_RGB:\n"));
/*
		if (level == 0)
     			;
*/
		/* The UGLY way, and SLOW : use DMA FIXME ! */

		for (i = 0; i < words; i++) {
		unsigned int data;
		/* data = PACK_COLOR_565(src[0],src[1],src[2]); */
		data = S3VIRGEPACKCOLOR555(src[0],src[1],src[2],255)
			|(S3VIRGEPACKCOLOR555(src[3],src[4],src[5],255)<<16);

		*dest++ = data;
	 	/* src += 3; */
		src +=6;
      	}
	}
	break;

	case GL_RGBA:
	case 4:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_RGBA:\n"));
/*
		if (level == 0)
			;
*/
		for (i = 0; i < words; i++) {		
		unsigned int data;
		
		/* data = PACK_COLOR_8888(src[0],src[1],src[2],src[3]); */
		data = S3VIRGEPACKCOLOR4444(src[0], src[1],src[2], src[3])
		| (S3VIRGEPACKCOLOR4444(src[4], src[5], src[6], src[7]) << 16);
		
		*dest++ = data;
		/* src += 4; */
		src += 8;
		}
	}
	break;

	case GL_LUMINANCE:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_LUMINANCE:\n"));
/*
		if (level == 0)
			;
*/
		for (i = 0; i < words; i++) {
		unsigned int data;
		
		/* data = PACK_COLOR_888(src[0],src[0],src[0]); */
		data = S3VIRGEPACKCOLOR4444(src[0],src[0],src[0],src[0])
		| (S3VIRGEPACKCOLOR4444(src[1],src[1],src[1],src[1]) << 16);
		 
		*dest++ = data;
		/* src ++; */
		src +=2;
		}
	}
	break;

	case GL_INTENSITY:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_INTENSITY:\n"));
/*	
		if (level == 0)
			;
*/
		for (i = 0; i < words; i++) {
		unsigned int data;
		
		/* data = PACK_COLOR_8888(src[0],src[0],src[0],src[0]); */
		data = S3VIRGEPACKCOLOR4444(src[0],src[0],src[0],src[0])
	        | (S3VIRGEPACKCOLOR4444(src[1],src[1],src[1],src[1]) << 16);

		*dest++ = data; 
		/* src ++; */
		src += 2;
		}
	}
	break;

	case GL_LUMINANCE_ALPHA:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_LUMINANCE_ALPHA:\n"));
/*
		if (level == 0)
			;
*/
		for (i = 0; i < words; i++) {
		unsigned int data;
		
		/* data = PACK_COLOR_8888(src[0],src[0],src[0],src[1]); */
		data = S3VIRGEPACKCOLOR4444(src[0],src[0],src[0],src[1])
	        | (S3VIRGEPACKCOLOR4444(src[2],src[2],src[2],src[3]) << 16);
		
		*dest++ = data;
		/* src += 2; */
		src += 4;
		}
	}
	break;

	case GL_ALPHA:
	{
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_ALPHA:\n"));
/*
		if (level == 0)
			;
*/
		for (i = 0; i < words; i++) {
		unsigned int data;
		
		/* data = PACK_COLOR_8888(255,255,255,src[0]); */
		data = S3VIRGEPACKCOLOR4444(255,255,255,src[0])
		| (S3VIRGEPACKCOLOR4444(255,255,255,src[1]) << 16);
		
		*dest++ = data;
		/* src += 1; */
		src += 2;
		}
	}
	break;

	/* TODO: Translate color indices *now*:
	 */
	case GL_COLOR_INDEX:
	{
	
		GLubyte *dst = (GLubyte *)(t->BufAddr + t->image[level].offset);
		GLubyte *src = (GLubyte *)image->Data;

		DEBUG_TEX(("GL_COLOR_INDEX:\n"));

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
			_mesa_lookup_enum_by_nr(image->_BaseFormat));
	}

	DEBUG_TEX(("words = %i\n\n", words));
}

void s3vPrintLocalLRU( s3vContextPtr vmesa )
{
   s3vTextureObjectPtr t;
   int sz = 1 << (vmesa->s3vScreen->logTextureGranularity);

#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vPrintLocalLRU: #%i ***\n", ++times));
#endif

   foreach( t, &vmesa->TexObjList ) {
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

void s3vPrintGlobalLRU( s3vContextPtr vmesa )
{
   int i, j;
   S3VTexRegionPtr list = vmesa->sarea->texList;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vPrintGlobalLRU: #%i ***\n", ++times));
#endif

   for (i = 0, j = S3V_NR_TEX_REGIONS ; i < S3V_NR_TEX_REGIONS ; i++) {
      fprintf(stderr, "list[%d] age %d next %d prev %d\n",
	      j, list[j].age, list[j].next, list[j].prev);
      j = list[j].next;
      if (j == S3V_NR_TEX_REGIONS) break;
   }

   if (j != S3V_NR_TEX_REGIONS)
      fprintf(stderr, "Loop detected in global LRU\n");
}


void s3vResetGlobalLRU( s3vContextPtr vmesa )
{
   S3VTexRegionPtr list = vmesa->sarea->texList;
   int sz = 1 << vmesa->s3vScreen->logTextureGranularity;
   int i;

#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vResetGlobalLRU: #%i ***\n", ++times));
#endif

   /* (Re)initialize the global circular LRU list.  The last element
    * in the array (S3V_NR_TEX_REGIONS) is the sentinal.  Keeping it
    * at the end of the array allows it to be addressed rationally
    * when looking up objects at a particular location in texture
    * memory.
    */
   for (i = 0 ; (i+1) * sz <= vmesa->s3vScreen->textureSize ; i++) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = S3V_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = S3V_NR_TEX_REGIONS;
   list[S3V_NR_TEX_REGIONS].prev = i;
   list[S3V_NR_TEX_REGIONS].next = 0;
   vmesa->sarea->texAge = 0;
}


void s3vUpdateTexLRU( s3vContextPtr vmesa, s3vTextureObjectPtr t )
{
/*
   int i;
   int logsz = vmesa->s3vScreen->logTextureGranularity;
   int start = t->MemBlock->ofs >> logsz;
   int end = (t->MemBlock->ofs + t->MemBlock->size - 1) >> logsz;
   S3VTexRegionPtr list = vmesa->sarea->texList;
*/

#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vUpdateTexLRU: #%i ***\n", ++times));
#endif

   vmesa->texAge = ++vmesa->sarea->texAge;

   /* Update our local LRU
    */
   move_to_head( &(vmesa->TexObjList), t );

   /* Update the global LRU
    */
#if 0
   for (i = start ; i <= end ; i++) {

      list[i].in_use = 1;
      list[i].age = vmesa->texAge;

      /* remove_from_list(i)
       */
      list[(unsigned)list[i].next].prev = list[i].prev;
      list[(unsigned)list[i].prev].next = list[i].next;

      /* insert_at_head(list, i)
       */
      list[i].prev = S3V_NR_TEX_REGIONS;
      list[i].next = list[S3V_NR_TEX_REGIONS].next;
      list[(unsigned)list[S3V_NR_TEX_REGIONS].next].prev = i;
      list[S3V_NR_TEX_REGIONS].next = i;
   }
#endif
}


/* Called for every shared texture region which has increased in age
 * since we last held the lock.
 *
 * Figures out which of our textures have been ejected by other clients,
 * and pushes a placeholder texture onto the LRU list to represent
 * the other client's textures.
 */
void s3vTexturesGone( s3vContextPtr vmesa,
		       GLuint offset,
		       GLuint size,
		       GLuint in_use )
{
   s3vTextureObjectPtr t, tmp;
#if TEX_DEBUG_ON
   static unsigned int times=0;
   DEBUG_TEX(("*** s3vTexturesGone: #%i ***\n", ++times));
#endif

   foreach_s ( t, tmp, &vmesa->TexObjList ) {

      if (t->MemBlock->ofs >= offset + size ||
	  t->MemBlock->ofs + t->MemBlock->size <= offset)
         continue;

      /* It overlaps - kick it off.  Need to hold onto the currently bound
       * objects, however.
       */
	  s3vSwapOutTexObj( vmesa, t );
   }

   if (in_use) {
		   t = (s3vTextureObjectPtr) calloc(1,sizeof(*t));
		   if (!t) return;

		   t->MemBlock = mmAllocMem( vmesa->texHeap, size, 0, offset);
		   insert_at_head( &vmesa->TexObjList, t );
   }

   /* Reload any lost textures referenced by current vertex buffer.
	*/
#if 0
   if (vmesa->vertex_buffer) {
		   int i, j;

		   fprintf(stderr, "\n\nreload tex\n");

		   for (i = 0 ; i < vmesa->statenr ; i++) {
				   for (j = 0 ; j < 2 ; j++) {
						   s3vTextureObjectPtr t = vmesa->state_tex[j][i];
						   if (t) {
								   if (t->MemBlock == 0)
										   s3vUploadTexImages( vmesa, t );
						   }
				   }
		   }

		   /* Hard to do this with the lock held:
			*/
		   /*        S3V_FIREVERTICES( vmesa ); */
   }
#endif
}


/* This is called with the lock held.  May have to eject our own and/or
 * other client's texture objects to make room for the upload.
 */
void s3vUploadTexImages( s3vContextPtr vmesa, s3vTextureObjectPtr t )
{
	int i;
	int ofs;
	int numLevels;
#if TEX_DEBUG_ON
	static unsigned int times=0;
	static unsigned int try=0;

	DEBUG_TEX(("*** s3vUploadTexImages: #%i ***\n", ++times));
	DEBUG_TEX(("vmesa->texHeap = 0x%x; t->totalSize = %i\n",
		(unsigned int)vmesa->texHeap, t->totalSize));
#endif

	/* Do we need to eject LRU texture objects?
	 */
	if (!t->MemBlock) {

		while (1)
		{
			/* int try = 0; */
			DEBUG_TEX(("trying to alloc mem for tex (try %i)\n", ++try));

			t->MemBlock = mmAllocMem( vmesa->texHeap, t->totalSize, 12, 0 );

			if (t->MemBlock)
				break;

			if (vmesa->TexObjList.prev == vmesa->CurrentTexObj[0]) {
/*			    || vmesa->TexObjList.prev == vmesa->CurrentTexObj[1]) {
				fprintf(stderr, "Hit bound texture in upload\n");
				s3vPrintLocalLRU( vmesa ); */
				return;
			}

			if (vmesa->TexObjList.prev == &(vmesa->TexObjList)) {
/*				fprintf(stderr, "Failed to upload texture, sz %d\n",
					t->totalSize);
				mmDumpMemInfo( vmesa->texHeap ); */
				return;
			}

			DEBUG_TEX(("swapping out: %p\n", vmesa->TexObjList.prev));
			s3vSwapOutTexObj( vmesa, vmesa->TexObjList.prev );
		}

	ofs = t->MemBlock->ofs;

	t->BufAddr = vmesa->s3vScreen->texOffset + ofs;

	DEBUG_TEX(("ofs = 0x%x\n", ofs));
	DEBUG_TEX(("t->BufAddr = 0x%x\n", t->BufAddr));

/* FIXME: check if we need it */
#if 0
	if (t == vmesa->CurrentTexObj[0]) {
		vmesa->dirty |= S3V_UPLOAD_TEX0; 
		vmesa->restore_primitive = -1; 
	}
#endif

#if 0
	if (t == vmesa->CurrentTexObj[1])
		vmesa->dirty |= S3V_UPLOAD_TEX1;
#endif

	s3vUpdateTexLRU( vmesa, t );
	}

#if 0
	if (vmesa->dirtyAge >= GET_DISPATCH_AGE(vmesa))
		s3vWaitAgeLocked( vmesa, vmesa->dirtyAge );
#endif

#if _TEXLOCK
	S3V_SIMPLE_FLUSH_LOCK(vmesa);
#endif
	numLevels = t->lastLevel - t->firstLevel + 1;
	for (i = 0 ; i < numLevels ; i++)
		if (t->dirty_images & (1<<i))
			s3vUploadTexLevel( vmesa, t, i );

	t->dirty_images = 0;
#if _TEXLOCK
	S3V_SIMPLE_UNLOCK(vmesa);
#endif
}
