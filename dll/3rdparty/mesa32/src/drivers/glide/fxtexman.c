/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 *    Daniel Borca
 *    Hiroshi Morii
 */

/* fxtexman.c - 3Dfx VooDoo texture memory functions */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)

#include "hash.h"
#include "fxdrv.h"

int texSwaps = 0;
static FxU32 texBoundMask;

#define FX_2MB_SPLIT 0x200000

static struct gl_texture_object *fxTMFindOldestObject(fxMesaContext fxMesa,
						      int tmu);


#ifdef TEXSANITY
static void
fubar()
{
}

  /* Sanity Check */
static void
sanity(fxMesaContext fxMesa)
{
   MemRange *tmp, *prev, *pos;

   prev = 0;
   tmp = fxMesa->tmFree[0];
   while (tmp) {
      if (!tmp->startAddr && !tmp->endAddr) {
	 fprintf(stderr, "Textures fubar\n");
	 fubar();
      }
      if (tmp->startAddr >= tmp->endAddr) {
	 fprintf(stderr, "Node fubar\n");
	 fubar();
      }
      if (prev && (prev->startAddr >= tmp->startAddr ||
		   prev->endAddr > tmp->startAddr)) {
	 fprintf(stderr, "Sorting fubar\n");
	 fubar();
      }
      prev = tmp;
      tmp = tmp->next;
   }
   prev = 0;
   tmp = fxMesa->tmFree[1];
   while (tmp) {
      if (!tmp->startAddr && !tmp->endAddr) {
	 fprintf(stderr, "Textures fubar\n");
	 fubar();
      }
      if (tmp->startAddr >= tmp->endAddr) {
	 fprintf(stderr, "Node fubar\n");
	 fubar();
      }
      if (prev && (prev->startAddr >= tmp->startAddr ||
		   prev->endAddr > tmp->startAddr)) {
	 fprintf(stderr, "Sorting fubar\n");
	 fubar();
      }
      prev = tmp;
      tmp = tmp->next;
   }
}
#endif

static MemRange *
fxTMNewRangeNode(fxMesaContext fxMesa, FxU32 start, FxU32 end)
{
   MemRange *result = 0;

   if (fxMesa->tmPool) {
      result = fxMesa->tmPool;
      fxMesa->tmPool = fxMesa->tmPool->next;
   }
   else {
      if (!(result = MALLOC(sizeof(MemRange)))) {
	 fprintf(stderr, "fxTMNewRangeNode: ERROR: out of memory!\n");
	 fxCloseHardware();
	 exit(-1);
      }
   }
   result->startAddr = start;
   result->endAddr = end;
   return result;
}

#if 1
#define fxTMDeleteRangeNode(fxMesa, range) \
   do {                                    \
       range->next = fxMesa->tmPool;       \
       fxMesa->tmPool = range;             \
   } while (0);
#else
static void
fxTMDeleteRangeNode(fxMesaContext fxMesa, MemRange * range)
{
   range->next = fxMesa->tmPool;
   fxMesa->tmPool = range;
}
#endif

static void
fxTMUInit(fxMesaContext fxMesa, int tmu)
{
   MemRange *tmn, *last;
   FxU32 start, end, blockstart, blockend, chunk;

   start = grTexMinAddress(tmu);
   end = grTexMaxAddress(tmu);

   chunk = (fxMesa->type >= GR_SSTTYPE_Banshee) ? (end - start) : FX_2MB_SPLIT;

   if (fxMesa->verbose) {
      fprintf(stderr, "Voodoo TMU%d configuration:\n", tmu);
   }

   fxMesa->freeTexMem[tmu] = end - start;
   fxMesa->tmFree[tmu] = NULL;

   last = 0;
   blockstart = start;
   while (blockstart < end) {
      if (blockstart + chunk > end)
	 blockend = end;
      else
	 blockend = blockstart + chunk;

      if (fxMesa->verbose)
	 fprintf(stderr, "Voodoo   %08u-%08u\n",
		 (unsigned int) blockstart, (unsigned int) blockend);

      tmn = fxTMNewRangeNode(fxMesa, blockstart, blockend);
      tmn->next = NULL;


      if (last)
	 last->next = tmn;
      else
	 fxMesa->tmFree[tmu] = tmn;
      last = tmn;

      blockstart += chunk;
   }
}

static int
fxTMFindStartAddr(fxMesaContext fxMesa, GLint tmu, int size)
{
   MemRange *prev, *tmp;
   int result;
   struct gl_texture_object *obj;

   if (fxMesa->HaveTexUma) {
      tmu = FX_TMU0;
   }

   while (1) {
      prev = 0;
      tmp = fxMesa->tmFree[tmu];
      while (tmp) {
	 if (tmp->endAddr - tmp->startAddr >= size) {	/* Fits here */
	    result = tmp->startAddr;
	    tmp->startAddr += size;
	    if (tmp->startAddr == tmp->endAddr) {	/* Empty */
	       if (prev) {
		  prev->next = tmp->next;
	       }
	       else {
		  fxMesa->tmFree[tmu] = tmp->next;
	       }
	       fxTMDeleteRangeNode(fxMesa, tmp);
	    }
	    fxMesa->freeTexMem[tmu] -= size;
	    return result;
	 }
	 prev = tmp;
	 tmp = tmp->next;
      }
      /* No free space. Discard oldest */
      if (TDFX_DEBUG & VERBOSE_TEXTURE) {
	 fprintf(stderr, "fxTMFindStartAddr: No free space. Discard oldest\n");
      }
      obj = fxTMFindOldestObject(fxMesa, tmu);
      if (!obj) {
	 fprintf(stderr, "fxTMFindStartAddr: ERROR: No space for texture\n");
	 return -1;
      }
      fxTMMoveOutTM(fxMesa, obj);
      texSwaps++;
   }
}

int fxTMCheckStartAddr (fxMesaContext fxMesa, GLint tmu, tfxTexInfo *ti)
{
 MemRange *tmp;
 int size;

 if (fxMesa->HaveTexUma) {
    return FXTRUE;
 }

 size = grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &(ti->info));

 tmp = fxMesa->tmFree[tmu];
 while (tmp) {
       if (tmp->endAddr - tmp->startAddr >= size) { /* Fits here */
          return FXTRUE;
       }
       tmp = tmp->next;
 }

 return FXFALSE;
}

static void
fxTMRemoveRange(fxMesaContext fxMesa, GLint tmu, MemRange * range)
{
   MemRange *tmp, *prev;

   if (fxMesa->HaveTexUma) {
      tmu = FX_TMU0;
   }

   if (range->startAddr == range->endAddr) {
      fxTMDeleteRangeNode(fxMesa, range);
      return;
   }
   fxMesa->freeTexMem[tmu] += range->endAddr - range->startAddr;
   prev = 0;
   tmp = fxMesa->tmFree[tmu];
   while (tmp) {
      if (range->startAddr > tmp->startAddr) {
	 prev = tmp;
	 tmp = tmp->next;
      }
      else
	 break;
   }
   /* When we create the regions, we make a split at the 2MB boundary.
      Now we have to make sure we don't join those 2MB boundary regions
      back together again. */
   range->next = tmp;
   if (tmp) {
      if (range->endAddr == tmp->startAddr
	  && tmp->startAddr & texBoundMask) {
	 /* Combine */
	 tmp->startAddr = range->startAddr;
	 fxTMDeleteRangeNode(fxMesa, range);
	 range = tmp;
      }
   }
   if (prev) {
      if (prev->endAddr == range->startAddr
	  && range->startAddr & texBoundMask) {
	 /* Combine */
	 prev->endAddr = range->endAddr;
	 prev->next = range->next;
	 fxTMDeleteRangeNode(fxMesa, range);
      }
      else
	 prev->next = range;
   }
   else {
      fxMesa->tmFree[tmu] = range;
   }
}

static struct gl_texture_object *
fxTMFindOldestObject(fxMesaContext fxMesa, int tmu)
{
   GLuint age, old, lasttime, bindnumber;
   GLfloat lowestPriority;
   struct gl_texture_object *obj, *lowestPriorityObj;
   struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;
   GLuint id;

   if (!_mesa_HashFirstEntry(textures))
      return 0;

   obj = NULL;
   old = 0;

   lowestPriorityObj = NULL;
   lowestPriority = 1.0F;

   bindnumber = fxMesa->texBindNumber;

   for (id = _mesa_HashFirstEntry(textures);
        id;
        id = _mesa_HashNextEntry(textures, id)) {
      struct gl_texture_object *tmp
         = (struct gl_texture_object *) _mesa_HashLookup(textures, id);
      tfxTexInfo *info = fxTMGetTexInfo(tmp);

      if (info && info->isInTM &&
	  ((info->whichTMU == tmu) ||
	   (info->whichTMU == FX_TMU_BOTH) ||
	   (info->whichTMU == FX_TMU_SPLIT) ||
	   fxMesa->HaveTexUma
	  )
	 ) {
	 lasttime = info->lastTimeUsed;

	 if (lasttime > bindnumber)
	    age = bindnumber + (UINT_MAX - lasttime + 1);	/* TO DO: check wrap around */
	 else
	    age = bindnumber - lasttime;

	 if (age >= old) {
	    old = age;
	    obj = tmp;
	 }

         /* examine priority */
         if (tmp->Priority < lowestPriority) {
            lowestPriority = tmp->Priority;
            lowestPriorityObj = tmp;
         }
      }
   }

   if (lowestPriorityObj != NULL) {
       if (TDFX_DEBUG & VERBOSE_TEXTURE) {
          fprintf(stderr, "fxTMFindOldestObject: %d pri=%f\n", lowestPriorityObj->Name, lowestPriority);
       }
       return lowestPriorityObj;
   }
   else {
       if (TDFX_DEBUG & VERBOSE_TEXTURE) {
          if (obj != NULL) {
             fprintf(stderr, "fxTMFindOldestObject: %d age=%d\n", obj->Name, old);
          }
       }
       return obj;
   }
}

static MemRange *
fxTMAddObj(fxMesaContext fxMesa,
	   struct gl_texture_object *tObj, GLint tmu, int texmemsize)
{
   FxI32 startAddr;
   MemRange *range;

   startAddr = fxTMFindStartAddr(fxMesa, tmu, texmemsize);
   if (startAddr < 0)
      return 0;
   range = fxTMNewRangeNode(fxMesa, startAddr, startAddr + texmemsize);
   return range;
}

/* External Functions */

void
fxTMMoveInTM_NoLock(fxMesaContext fxMesa, struct gl_texture_object *tObj,
		    GLint where)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   int i, l;
   int texmemsize;

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxTMMoveInTM_NoLock(%d)\n", tObj->Name);
   }

   fxMesa->stats.reqTexUpload++;

   if (!ti->validated) {
      fprintf(stderr, "fxTMMoveInTM_NoLock: INTERNAL ERROR: not validated\n");
      fxCloseHardware();
      exit(-1);
   }

   if (ti->isInTM) {
      if (ti->whichTMU == where)
	 return;
      if (where == FX_TMU_SPLIT || ti->whichTMU == FX_TMU_SPLIT)
	 fxTMMoveOutTM_NoLock(fxMesa, tObj);
      else {
	 if (ti->whichTMU == FX_TMU_BOTH)
	    return;
	 where = FX_TMU_BOTH;
      }
   }

   if (TDFX_DEBUG & (VERBOSE_DRIVER | VERBOSE_TEXTURE)) {
      fprintf(stderr, "fxTMMoveInTM_NoLock: downloading %p (%d) in texture memory in %d\n",
	              (void *)tObj, tObj->Name, where);
   }

   ti->whichTMU = (FxU32) where;

   switch (where) {
   case FX_TMU0:
   case FX_TMU1:
      texmemsize = (int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
      ti->tm[where] = fxTMAddObj(fxMesa, tObj, where, texmemsize);
      fxMesa->stats.memTexUpload += texmemsize;

      for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
	   i <= FX_smallLodValue(ti->info); i++, l++) {
	 struct gl_texture_image *texImage = tObj->Image[0][l];
	 grTexDownloadMipMapLevel(where,
					    ti->tm[where]->startAddr,
					    FX_valueToLod(i),
					    FX_largeLodLog2(ti->info),
					    FX_aspectRatioLog2(ti->info),
					    ti->info.format,
					    GR_MIPMAPLEVELMASK_BOTH,
					    texImage->Data);
      }
      break;
   case FX_TMU_SPLIT:
      texmemsize = (int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD, &(ti->info));
      ti->tm[FX_TMU0] = fxTMAddObj(fxMesa, tObj, FX_TMU0, texmemsize);
      fxMesa->stats.memTexUpload += texmemsize;

      texmemsize = (int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN, &(ti->info));
      ti->tm[FX_TMU1] = fxTMAddObj(fxMesa, tObj, FX_TMU1, texmemsize);
      fxMesa->stats.memTexUpload += texmemsize;

      for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
	   i <= FX_smallLodValue(ti->info); i++, l++) {
	 struct gl_texture_image *texImage = tObj->Image[0][l];

	 grTexDownloadMipMapLevel(GR_TMU0,
					    ti->tm[FX_TMU0]->startAddr,
					    FX_valueToLod(i),
					    FX_largeLodLog2(ti->info),
					    FX_aspectRatioLog2(ti->info),
					    ti->info.format,
					    GR_MIPMAPLEVELMASK_ODD,
					    texImage->Data);

	 grTexDownloadMipMapLevel(GR_TMU1,
					    ti->tm[FX_TMU1]->startAddr,
					    FX_valueToLod(i),
					    FX_largeLodLog2(ti->info),
					    FX_aspectRatioLog2(ti->info),
					    ti->info.format,
					    GR_MIPMAPLEVELMASK_EVEN,
					    texImage->Data);
      }
      break;
   case FX_TMU_BOTH:
      texmemsize = (int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &(ti->info));
      ti->tm[FX_TMU0] = fxTMAddObj(fxMesa, tObj, FX_TMU0, texmemsize);
      fxMesa->stats.memTexUpload += texmemsize;

      /*texmemsize = (int)grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH, &(ti->info));*/
      ti->tm[FX_TMU1] = fxTMAddObj(fxMesa, tObj, FX_TMU1, texmemsize);
      fxMesa->stats.memTexUpload += texmemsize;

      for (i = FX_largeLodValue(ti->info), l = ti->minLevel;
	   i <= FX_smallLodValue(ti->info); i++, l++) {
	 struct gl_texture_image *texImage = tObj->Image[0][l];
	 grTexDownloadMipMapLevel(GR_TMU0,
					    ti->tm[FX_TMU0]->startAddr,
					    FX_valueToLod(i),
					    FX_largeLodLog2(ti->info),
					    FX_aspectRatioLog2(ti->info),
					    ti->info.format,
					    GR_MIPMAPLEVELMASK_BOTH,
					    texImage->Data);

	 grTexDownloadMipMapLevel(GR_TMU1,
					    ti->tm[FX_TMU1]->startAddr,
					    FX_valueToLod(i),
					    FX_largeLodLog2(ti->info),
					    FX_aspectRatioLog2(ti->info),
					    ti->info.format,
					    GR_MIPMAPLEVELMASK_BOTH,
					    texImage->Data);
      }
      break;
   default:
      fprintf(stderr, "fxTMMoveInTM_NoLock: INTERNAL ERROR: wrong tmu (%d)\n", where);
      fxCloseHardware();
      exit(-1);
   }

   fxMesa->stats.texUpload++;

   ti->isInTM = GL_TRUE;
}


void
fxTMMoveInTM(fxMesaContext fxMesa, struct gl_texture_object *tObj,
	     GLint where)
{
   BEGIN_BOARD_LOCK();
   fxTMMoveInTM_NoLock(fxMesa, tObj, where);
   END_BOARD_LOCK();
}


void
fxTMReloadMipMapLevel(fxMesaContext fxMesa, struct gl_texture_object *tObj,
		      GLint level)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   GrLOD_t lodlevel;
   GLint tmu;
   struct gl_texture_image *texImage = tObj->Image[0][level];
   tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
      fprintf(stderr, "fxTMReloadMipMapLevel(%p (%d), %d)\n", (void *)tObj, tObj->Name, level);
   }

   assert(mml);
   assert(mml->width > 0);
   assert(mml->height > 0);
   assert(mml->glideFormat > 0);
   assert(ti->isInTM);

   if (!ti->validated) {
      fprintf(stderr, "fxTMReloadMipMapLevel: INTERNAL ERROR: not validated\n");
      fxCloseHardware();
      exit(-1);
   }

   tmu = (int) ti->whichTMU;
   fxMesa->stats.reqTexUpload++;
   fxMesa->stats.texUpload++;

   lodlevel =  ti->info.largeLodLog2 - (level - ti->minLevel);

   switch (tmu) {
   case FX_TMU0:
   case FX_TMU1:
      grTexDownloadMipMapLevel(tmu,
				  ti->tm[tmu]->startAddr,
				  lodlevel,
				  FX_largeLodLog2(ti->info),
				  FX_aspectRatioLog2(ti->info),
				  ti->info.format,
				  GR_MIPMAPLEVELMASK_BOTH, texImage->Data);
      break;
   case FX_TMU_SPLIT:
      grTexDownloadMipMapLevel(GR_TMU0,
				  ti->tm[GR_TMU0]->startAddr,
				  lodlevel,
				  FX_largeLodLog2(ti->info),
				  FX_aspectRatioLog2(ti->info),
				  ti->info.format,
				  GR_MIPMAPLEVELMASK_ODD, texImage->Data);

      grTexDownloadMipMapLevel(GR_TMU1,
				  ti->tm[GR_TMU1]->startAddr,
				  lodlevel,
				  FX_largeLodLog2(ti->info),
				  FX_aspectRatioLog2(ti->info),
				  ti->info.format,
				  GR_MIPMAPLEVELMASK_EVEN, texImage->Data);
      break;
   case FX_TMU_BOTH:
      grTexDownloadMipMapLevel(GR_TMU0,
				  ti->tm[GR_TMU0]->startAddr,
				  lodlevel,
				  FX_largeLodLog2(ti->info),
				  FX_aspectRatioLog2(ti->info),
				  ti->info.format,
				  GR_MIPMAPLEVELMASK_BOTH, texImage->Data);

      grTexDownloadMipMapLevel(GR_TMU1,
				  ti->tm[GR_TMU1]->startAddr,
				  lodlevel,
				  FX_largeLodLog2(ti->info),
				  FX_aspectRatioLog2(ti->info),
				  ti->info.format,
				  GR_MIPMAPLEVELMASK_BOTH, texImage->Data);
      break;

   default:
      fprintf(stderr, "fxTMReloadMipMapLevel: INTERNAL ERROR: wrong tmu (%d)\n", tmu);
      fxCloseHardware();
      exit(-1);
   }
}

void
fxTMReloadSubMipMapLevel(fxMesaContext fxMesa,
			 struct gl_texture_object *tObj,
			 GLint level, GLint yoffset, GLint height)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   GrLOD_t lodlevel;
   unsigned short *data;
   GLint tmu;
   struct gl_texture_image *texImage = tObj->Image[0][level];
   tfxMipMapLevel *mml = FX_MIPMAP_DATA(texImage);

   assert(mml);

   if (!ti->validated) {
      fprintf(stderr, "fxTMReloadSubMipMapLevel: INTERNAL ERROR: not validated\n");
      fxCloseHardware();
      exit(-1);
   }

   tmu = (int) ti->whichTMU;
   fxTMMoveInTM(fxMesa, tObj, tmu);

   fxTexGetInfo(mml->width, mml->height,
		&lodlevel, NULL, NULL, NULL, NULL, NULL);

   if ((ti->info.format == GR_TEXFMT_INTENSITY_8) ||
       (ti->info.format == GR_TEXFMT_P_8) ||
       (ti->info.format == GR_TEXFMT_ALPHA_8))
	 data = (GLushort *) texImage->Data + ((yoffset * mml->width) >> 1);
   else
      data = (GLushort *) texImage->Data + yoffset * mml->width;

   switch (tmu) {
   case FX_TMU0:
   case FX_TMU1:
      grTexDownloadMipMapLevelPartial(tmu,
					 ti->tm[tmu]->startAddr,
					 FX_valueToLod(FX_lodToValue(lodlevel)
						       + level),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_BOTH, data,
					 yoffset, yoffset + height - 1);
      break;
   case FX_TMU_SPLIT:
      grTexDownloadMipMapLevelPartial(GR_TMU0,
					 ti->tm[FX_TMU0]->startAddr,
					 FX_valueToLod(FX_lodToValue(lodlevel)
						       + level),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_ODD, data,
					 yoffset, yoffset + height - 1);

      grTexDownloadMipMapLevelPartial(GR_TMU1,
					 ti->tm[FX_TMU1]->startAddr,
					 FX_valueToLod(FX_lodToValue(lodlevel)
						       + level),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_EVEN, data,
					 yoffset, yoffset + height - 1);
      break;
   case FX_TMU_BOTH:
      grTexDownloadMipMapLevelPartial(GR_TMU0,
					 ti->tm[FX_TMU0]->startAddr,
					 FX_valueToLod(FX_lodToValue(lodlevel)
						       + level),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_BOTH, data,
					 yoffset, yoffset + height - 1);

      grTexDownloadMipMapLevelPartial(GR_TMU1,
					 ti->tm[FX_TMU1]->startAddr,
					 FX_valueToLod(FX_lodToValue(lodlevel)
						       + level),
					 FX_largeLodLog2(ti->info),
					 FX_aspectRatioLog2(ti->info),
					 ti->info.format,
					 GR_MIPMAPLEVELMASK_BOTH, data,
					 yoffset, yoffset + height - 1);
      break;
   default:
      fprintf(stderr, "fxTMReloadSubMipMapLevel: INTERNAL ERROR: wrong tmu (%d)\n", tmu);
      fxCloseHardware();
      exit(-1);
   }
}

void
fxTMMoveOutTM(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);

   if (TDFX_DEBUG & VERBOSE_DRIVER) {
      fprintf(stderr, "fxTMMoveOutTM(%p (%d))\n", (void *)tObj, tObj->Name);
   }

   if (!ti->isInTM)
      return;

   switch (ti->whichTMU) {
   case FX_TMU0:
   case FX_TMU1:
      fxTMRemoveRange(fxMesa, (int) ti->whichTMU, ti->tm[ti->whichTMU]);
      break;
   case FX_TMU_SPLIT:
   case FX_TMU_BOTH:
      fxTMRemoveRange(fxMesa, FX_TMU0, ti->tm[FX_TMU0]);
      fxTMRemoveRange(fxMesa, FX_TMU1, ti->tm[FX_TMU1]);
      break;
   default:
      fprintf(stderr, "fxTMMoveOutTM: INTERNAL ERROR: bad TMU (%ld)\n", ti->whichTMU);
      fxCloseHardware();
      exit(-1);
   }

   ti->isInTM = GL_FALSE;
   ti->whichTMU = FX_TMU_NONE;
}

void
fxTMFreeTexture(fxMesaContext fxMesa, struct gl_texture_object *tObj)
{
   tfxTexInfo *ti = fxTMGetTexInfo(tObj);
   int i;

   if (TDFX_DEBUG & VERBOSE_TEXTURE) {
      fprintf(stderr, "fxTMFreeTexture(%p (%d))\n", (void *)tObj, tObj->Name);
   }

   fxTMMoveOutTM(fxMesa, tObj);

   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      struct gl_texture_image *texImage = tObj->Image[0][i];
      if (texImage) {
         if (texImage->DriverData) {
            FREE(texImage->DriverData);
            texImage->DriverData = NULL;
         }
      }
   }
   switch (ti->whichTMU) {
   case FX_TMU0:
   case FX_TMU1:
      fxTMDeleteRangeNode(fxMesa, ti->tm[ti->whichTMU]);
      break;
   case FX_TMU_SPLIT:
   case FX_TMU_BOTH:
      fxTMDeleteRangeNode(fxMesa, ti->tm[FX_TMU0]);
      fxTMDeleteRangeNode(fxMesa, ti->tm[FX_TMU1]);
      break;
   }
}

void
fxTMInit(fxMesaContext fxMesa)
{
   fxMesa->texBindNumber = 0;
   fxMesa->tmPool = 0;

   if (fxMesa->HaveTexUma) {
      grEnable(GR_TEXTURE_UMA_EXT);
   }

   fxTMUInit(fxMesa, FX_TMU0);

   if (!fxMesa->HaveTexUma && fxMesa->haveTwoTMUs)
      fxTMUInit(fxMesa, FX_TMU1);

   texBoundMask = (fxMesa->type >= GR_SSTTYPE_Banshee) ? -1 : (FX_2MB_SPLIT - 1);
}

void
fxTMClose(fxMesaContext fxMesa)
{
   MemRange *tmp, *next;

   tmp = fxMesa->tmPool;
   while (tmp) {
      next = tmp->next;
      FREE(tmp);
      tmp = next;
   }
   tmp = fxMesa->tmFree[FX_TMU0];
   while (tmp) {
      next = tmp->next;
      FREE(tmp);
      tmp = next;
   }
   if (fxMesa->haveTwoTMUs) {
      tmp = fxMesa->tmFree[FX_TMU1];
      while (tmp) {
	 next = tmp->next;
	 FREE(tmp);
	 tmp = next;
      }
   }
}

void
fxTMRestoreTextures_NoLock(fxMesaContext ctx)
{
   struct _mesa_HashTable *textures = ctx->glCtx->Shared->TexObjects;
   GLuint id;

   for (id = _mesa_HashFirstEntry(textures);
        id;
        id = _mesa_HashNextEntry(textures, id)) {
      struct gl_texture_object *tObj
         = (struct gl_texture_object *) _mesa_HashLookup(textures, id);
      tfxTexInfo *ti = fxTMGetTexInfo(tObj);
      if (ti && ti->isInTM) {
         int i;
	 for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
	    if (ctx->glCtx->Texture.Unit[i]._Current == tObj) {
	       /* Force the texture onto the board, as it could be in use */
	       int where = ti->whichTMU;
	       fxTMMoveOutTM_NoLock(ctx, tObj);
	       fxTMMoveInTM_NoLock(ctx, tObj, where);
	       break;
	    }
         }
	 if (i == MAX_TEXTURE_UNITS)	/* Mark the texture as off the board */
	    fxTMMoveOutTM_NoLock(ctx, tObj);
      }
   }
}

#else


/*
 * Need this to provide at least one external definition.
 */

extern int gl_fx_dummy_function_texman(void);
int
gl_fx_dummy_function_texman(void)
{
   return 0;
}

#endif /* FX */
