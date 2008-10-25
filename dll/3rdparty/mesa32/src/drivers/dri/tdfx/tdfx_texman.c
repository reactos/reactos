/* -*- mode: c; c-basic-offset: 3 -*-
 *
 * Copyright 2000 VA Linux Systems Inc., Fremont, California.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/tdfx/tdfx_texman.c,v 1.5 2002/02/22 21:45:04 dawes Exp $ */

/*
 * Original rewrite:
 *	Gareth Hughes <gareth@valinux.com>, 29 Sep - 1 Oct 2000
 *
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Brian Paul <brianp@valinux.com>
 *
 */

#include "tdfx_context.h"
#include "tdfx_tex.h"
#include "tdfx_texman.h"
#include "texobj.h"
#include "hash.h"


#define BAD_ADDRESS ((FxU32) -1)


#if 0 /* DEBUG use */
/*
 * Verify the consistancy of the texture memory manager.
 * This involves:
 *    Traversing all texture objects and computing total memory used.
 *    Traverse the free block list and computing total memory free.
 *    Compare the total free and total used amounts to the total memory size.
 *    Make various assertions about the results.
 */
static void
VerifyFreeList(tdfxContextPtr fxMesa, FxU32 tmu)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxMemRange *block;
    int prevStart = -1, prevEnd = -1;
    int totalFree = 0;
    int numObj = 0, numRes = 0;
    int totalUsed = 0;

    for (block = shared->tmFree[tmu]; block; block = block->next) {
       assert( block->endAddr > 0 );
       assert( block->startAddr <= shared->totalTexMem[tmu] );
       assert( block->endAddr <= shared->totalTexMem[tmu] );
       assert( (int) block->startAddr > prevStart );
       assert( (int) block->startAddr >= prevEnd );
       prevStart = (int) block->startAddr;
       prevEnd = (int) block->endAddr;
       totalFree += (block->endAddr - block->startAddr);
    }
    assert(totalFree == shared->freeTexMem[tmu]);

    {
       struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;
       GLuint id;
       for (id = _mesa_HashFirstEntry(textures);
            id;
            id = _mesa_HashNextEntry(textures, id)) {
          struct gl_texture_object *tObj
             = _mesa_lookup_texture(fxMesa->glCtx, id);
          tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
          if (ti) {
             if (ti->isInTM) {
                numRes++;
                assert(ti->tm[0]);
                if (ti->tm[tmu])
                   totalUsed += (ti->tm[tmu]->endAddr - ti->tm[tmu]->startAddr);
             }
             else {
                assert(!ti->tm[0]);
             }
          }
       }
    }

    printf("totalFree: %d  totalUsed: %d  totalMem: %d #objs=%d  #res=%d\n",
           shared->freeTexMem[tmu], totalUsed, shared->totalTexMem[tmu],
           numObj, numRes);

    assert(totalUsed + totalFree == shared->totalTexMem[tmu]);
}


static void
dump_texmem(tdfxContextPtr fxMesa)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct _mesa_HashTable *textures = mesaShared->TexObjects;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxMemRange *r;
    FxU32 prev;
    GLuint id;

    printf("DUMP Objects:\n");
    for (id = _mesa_HashFirstEntry(textures);
         id;
         id = _mesa_HashNextEntry(textures, id)) {
        struct gl_texture_object *obj
           = _mesa_lookup_texture(fxMesa->glCtx, id);
        tdfxTexInfo *info = TDFX_TEXTURE_DATA(obj);

        if (info && info->isInTM) {
        printf("Obj %8p: %4d  info = %p\n", obj, obj->Name, info);

           printf("  isInTM=%d  whichTMU=%d  lastTimeUsed=%d\n",
                  info->isInTM, info->whichTMU, info->lastTimeUsed);
           printf("    tm[0] = %p", info->tm[0]);
           assert(info->tm[0]);
           if (info->tm[0]) {
              printf("  tm startAddr = %d  endAddr = %d",
                     info->tm[0]->startAddr,
                     info->tm[0]->endAddr);
           }
           printf("\n");
           printf("    tm[1] = %p", info->tm[1]);
           if (info->tm[1]) {
              printf("  tm startAddr = %d  endAddr = %d",
                     info->tm[1]->startAddr,
                     info->tm[1]->endAddr);
           }
           printf("\n");
        }
    }

    VerifyFreeList(fxMesa, 0);
    VerifyFreeList(fxMesa, 1);

    printf("Free memory unit 0:  %d bytes\n", shared->freeTexMem[0]);
    prev = 0;
    for (r = shared->tmFree[0]; r; r = r->next) {
       printf("%8p:  start %8d  end %8d  size %8d  gap %8d\n", r, r->startAddr, r->endAddr, r->endAddr - r->startAddr, r->startAddr - prev);
       prev = r->endAddr;
    }

    printf("Free memory unit 1:  %d bytes\n", shared->freeTexMem[1]);
    prev = 0;
    for (r = shared->tmFree[1]; r; r = r->next) {
       printf("%8p:  start %8d  end %8d  size %8d  gap %8d\n", r, r->startAddr, r->endAddr, r->endAddr - r->startAddr, r->startAddr - prev);
       prev = r->endAddr;
    }

}
#endif



#ifdef TEXSANITY
static void
fubar(void)
{
}

/*
 * Sanity Check
 */
static void
sanity(tdfxContextPtr fxMesa)
{
    tdfxMemRange *tmp, *prev, *pos;

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





/*
 * Allocate and initialize a new MemRange struct.
 * Try to allocate it from the pool of free MemRange nodes rather than malloc.
 */
static tdfxMemRange *
NewRangeNode(tdfxContextPtr fxMesa, FxU32 start, FxU32 end)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxMemRange *result;

    _glthread_LOCK_MUTEX(mesaShared->Mutex);
    if (shared && shared->tmPool) {
        result = shared->tmPool;
        shared->tmPool = shared->tmPool->next;
    }
    else {
        result = MALLOC(sizeof(tdfxMemRange));

    }
    _glthread_UNLOCK_MUTEX(mesaShared->Mutex);

    if (!result) {
        /*fprintf(stderr, "fxDriver: out of memory!\n");*/
        return NULL;
    }

    result->startAddr = start;
    result->endAddr = end;
    result->next = NULL;

    return result;
}


/*
 * Initialize texture memory.
 * We take care of one or both TMU's here.
 */
void
tdfxTMInit(tdfxContextPtr fxMesa)
{
    if (!fxMesa->glCtx->Shared->DriverData) {
        const char *extensions;
        struct tdfxSharedState *shared = CALLOC_STRUCT(tdfxSharedState);
        if (!shared)
           return;

        LOCK_HARDWARE(fxMesa);
        extensions = fxMesa->Glide.grGetString(GR_EXTENSION);
        UNLOCK_HARDWARE(fxMesa);
        if (strstr(extensions, "TEXUMA")) {
            FxU32 start, end;
            shared->umaTexMemory = GL_TRUE;
            LOCK_HARDWARE(fxMesa);
            fxMesa->Glide.grEnable(GR_TEXTURE_UMA_EXT);
            start = fxMesa->Glide.grTexMinAddress(0);
            end = fxMesa->Glide.grTexMaxAddress(0);
            UNLOCK_HARDWARE(fxMesa);
            shared->totalTexMem[0] = end - start;
            shared->totalTexMem[1] = 0;
            shared->freeTexMem[0] = end - start;
            shared->freeTexMem[1] = 0;
            shared->tmFree[0] = NewRangeNode(fxMesa, start, end);
            shared->tmFree[1] = NULL;
            /*printf("UMA tex memory: %d\n", (int) (end - start));*/
        }
        else {
            const int numTMUs = fxMesa->haveTwoTMUs ? 2 : 1;
            int tmu;
            shared->umaTexMemory = GL_FALSE;
            LOCK_HARDWARE(fxMesa);
            for (tmu = 0; tmu < numTMUs; tmu++) {
                FxU32 start = fxMesa->Glide.grTexMinAddress(tmu);
                FxU32 end = fxMesa->Glide.grTexMaxAddress(tmu);
                shared->totalTexMem[tmu] = end - start;
                shared->freeTexMem[tmu] = end - start;
                shared->tmFree[tmu] = NewRangeNode(fxMesa, start, end);
                /*printf("Split tex memory: %d\n", (int) (end - start));*/
            }
            UNLOCK_HARDWARE(fxMesa);
        }

        shared->tmPool = NULL;
        fxMesa->glCtx->Shared->DriverData = shared;
        /*printf("Texture memory init UMA: %d\n", shared->umaTexMemory);*/
    }
}


/*
 * Clean-up texture memory before destroying context.
 */
void
tdfxTMClose(tdfxContextPtr fxMesa)
{
    if (fxMesa->glCtx->Shared->RefCount == 1 && fxMesa->driDrawable) {
        /* refcount will soon go to zero, free our 3dfx stuff */
        struct tdfxSharedState *shared = (struct tdfxSharedState *) fxMesa->glCtx->Shared->DriverData;

        const int numTMUs = fxMesa->haveTwoTMUs ? 2 : 1;
        int tmu;
        tdfxMemRange *tmp, *next;

        /* Deallocate the pool of free tdfxMemRange nodes */
        tmp = shared->tmPool;
        while (tmp) {
            next = tmp->next;
            FREE(tmp);
            tmp = next;
        }

        /* Delete the texture memory block tdfxMemRange nodes */
        for (tmu = 0; tmu < numTMUs; tmu++) {
            tmp = shared->tmFree[tmu];
            while (tmp) {
                next = tmp->next;
                FREE(tmp);
                tmp = next;
            }
        }

        FREE(shared);
        fxMesa->glCtx->Shared->DriverData = NULL;
    }
}



/*
 * Delete a tdfxMemRange struct.
 * We keep a linked list of free/available tdfxMemRange structs to
 * avoid extra malloc/free calls.
 */
#if 0
static void
DeleteRangeNode_NoLock(struct TdfxSharedState *shared, tdfxMemRange *range)
{
    /* insert at head of list */
    range->next = shared->tmPool;
    shared->tmPool = range;
}
#endif

#define DELETE_RANGE_NODE(shared, range) \
    (range)->next = (shared)->tmPool;    \
    (shared)->tmPool = (range)



/*
 * When we've run out of texture memory we have to throw out an
 * existing texture to make room for the new one.  This function
 * determins the texture to throw out.
 */
static struct gl_texture_object *
FindOldestObject(tdfxContextPtr fxMesa, FxU32 tmu)
{
    const GLuint bindnumber = fxMesa->texBindNumber;
    struct gl_texture_object *oldestObj, *lowestPriorityObj;
    GLfloat lowestPriority;
    GLuint oldestAge;
    GLuint id;
    struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;

    oldestObj = NULL;
    oldestAge = 0;

    lowestPriority = 1.0F;
    lowestPriorityObj = NULL;

    for (id = _mesa_HashFirstEntry(textures);
         id;
         id = _mesa_HashNextEntry(textures, id)) {
        struct gl_texture_object *obj
           = _mesa_lookup_texture(fxMesa->glCtx, id);
        tdfxTexInfo *info = TDFX_TEXTURE_DATA(obj);

        if (info && info->isInTM &&
            ((info->whichTMU == tmu) || (info->whichTMU == TDFX_TMU_BOTH) ||
             (info->whichTMU == TDFX_TMU_SPLIT))) {
            GLuint age, lasttime;

            assert(info->tm[0]);
            lasttime = info->lastTimeUsed;

            if (lasttime > bindnumber)
                age = bindnumber + (UINT_MAX - lasttime + 1); /* TO DO: check wrap around */
            else
                age = bindnumber - lasttime;

            if (age >= oldestAge) {
                oldestAge = age;
                oldestObj = obj;
            }

            /* examine priority */
            if (obj->Priority < lowestPriority) {
                lowestPriority = obj->Priority;
                lowestPriorityObj = obj;
            }
        }
    }

    if (lowestPriority < 1.0) {
        ASSERT(lowestPriorityObj);
        /*
        printf("discard %d pri=%f\n", lowestPriorityObj->Name, lowestPriority);
        */
        return lowestPriorityObj;
    }
    else {
        /*
        printf("discard %d age=%d\n", oldestObj->Name, oldestAge);
        */
        return oldestObj;
    }
}


#if 0
static void
FlushTexMemory(tdfxContextPtr fxMesa)
{
    struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;
    GLuint id;

    for (id = _mesa_HashFirstEntry(textures);
         id;
         id = _mesa_HashNextEntry(textures, id)) {
       struct gl_texture_object *obj
          = _mesa_lookup_texture(fxMesa->glCtx, id);
       if (obj->RefCount < 2) {
          /* don't flush currently bound textures */
          tdfxTMMoveOutTM_NoLock(fxMesa, obj);
       }
    }
}
#endif


/*
 * Find the address (offset?) at which we can store a new texture.
 * <tmu> is the texture unit.
 * <size> is the texture size in bytes.
 */
static FxU32
FindStartAddr(tdfxContextPtr fxMesa, FxU32 tmu, FxU32 size)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxMemRange *prev, *block;
    FxU32 result;
#if 0
    int discardedCount = 0;
#define MAX_DISCARDS 10
#endif

    if (shared->umaTexMemory) {
        assert(tmu == TDFX_TMU0);
    }

    _glthread_LOCK_MUTEX(mesaShared->Mutex);
    while (1) {
        prev = NULL;
        block = shared->tmFree[tmu];
        while (block) {
            if (block->endAddr - block->startAddr >= size) {
                /* The texture will fit here */
                result = block->startAddr;
                block->startAddr += size;
                if (block->startAddr == block->endAddr) {
                    /* Remove this node since it's empty */
                    if (prev) {
                        prev->next = block->next;
                    }
                    else {
                        shared->tmFree[tmu] = block->next;
                    }
                    DELETE_RANGE_NODE(shared, block);
                }
                shared->freeTexMem[tmu] -= size;
                _glthread_UNLOCK_MUTEX(mesaShared->Mutex);
                return result;
            }
            prev = block;
            block = block->next;
        }
        /* We failed to find a block large enough to accomodate <size> bytes.
         * Find the oldest texObject and free it.
         */
#if 0
        discardedCount++;
        if (discardedCount > MAX_DISCARDS + 1) {
            _mesa_problem(NULL, "%s: extreme texmem fragmentation", __FUNCTION__);
            _glthread_UNLOCK_MUTEX(mesaShared->Mutex);
            return BAD_ADDRESS;
        }
        else if (discardedCount > MAX_DISCARDS) {
            /* texture memory is probably really fragmented, flush it */
            FlushTexMemory(fxMesa);
        }
        else
#endif
        {
            struct gl_texture_object *obj = FindOldestObject(fxMesa, tmu);
            if (obj) {
                tdfxTMMoveOutTM_NoLock(fxMesa, obj);
                fxMesa->stats.texSwaps++;
            }
            else {
                _mesa_problem(NULL, "%s: extreme texmem fragmentation", __FUNCTION__);
                _glthread_UNLOCK_MUTEX(mesaShared->Mutex);
                return BAD_ADDRESS;
            }
        }
    }

    /* never get here, but play it safe */
    _glthread_UNLOCK_MUTEX(mesaShared->Mutex);
    return BAD_ADDRESS;
}


/*
 * Remove the given tdfxMemRange node from hardware texture memory.
 */
static void
RemoveRange_NoLock(tdfxContextPtr fxMesa, FxU32 tmu, tdfxMemRange *range)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxMemRange *block, *prev;

    if (shared->umaTexMemory) {
       assert(tmu == TDFX_TMU0);
    }

    if (!range)
        return;

    if (range->startAddr == range->endAddr) {
        DELETE_RANGE_NODE(shared, range);
        return;
    }
    shared->freeTexMem[tmu] += range->endAddr - range->startAddr;

    /* find position in linked list to insert this tdfxMemRange node */
    prev = NULL;
    block = shared->tmFree[tmu];
    while (block) {
        assert(range->startAddr != block->startAddr);
        if (range->startAddr > block->startAddr) {
            prev = block;
            block = block->next;
        }
        else {
            break;
        }
    }

    /* Insert the free block, combine with adjacent blocks when possible */
    range->next = block;
    if (block) {
        if (range->endAddr == block->startAddr) {
            /* Combine */
            block->startAddr = range->startAddr;
            DELETE_RANGE_NODE(shared, range);
            range = block;
        }
    }
    if (prev) {
        if (prev->endAddr == range->startAddr) {
            /* Combine */
            prev->endAddr = range->endAddr;
            prev->next = range->next;
            DELETE_RANGE_NODE(shared, range);
        }
        else {
            prev->next = range;
        }
    }
    else {
        shared->tmFree[tmu] = range;
    }
}


#if 0 /* NOT USED */
static void
RemoveRange(tdfxContextPtr fxMesa, FxU32 tmu, tdfxMemRange *range)
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    _glthread_LOCK_MUTEX(mesaShared->Mutex);
    RemoveRange_NoLock(fxMesa, tmu, range);
    _glthread_UNLOCK_MUTEX(mesaShared->Mutex);
}
#endif


/*
 * Allocate space for a texture image.
 * <tmu> is the texture unit
 * <texmemsize> is the number of bytes to allocate
 */
static tdfxMemRange *
AllocTexMem(tdfxContextPtr fxMesa, FxU32 tmu, FxU32 texmemsize)
{
    FxU32 startAddr;
    startAddr = FindStartAddr(fxMesa, tmu, texmemsize);
    if (startAddr == BAD_ADDRESS) {
        _mesa_problem(fxMesa->glCtx, "%s returned NULL!  tmu=%d texmemsize=%d",
               __FUNCTION__, (int) tmu, (int) texmemsize);
        return NULL;
    }
    else {
        tdfxMemRange *range;
        range = NewRangeNode(fxMesa, startAddr, startAddr + texmemsize);
        return range;
    }
}


/*
 * Download (copy) the given texture data (all mipmap levels) into the
 * Voodoo's texture memory.
 * The texture memory must have already been allocated.
 */
void
tdfxTMDownloadTexture(tdfxContextPtr fxMesa, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti;
    GLint l;
    FxU32 targetTMU;

    assert(tObj);
    ti = TDFX_TEXTURE_DATA(tObj);
    assert(ti);
    targetTMU = ti->whichTMU;

    switch (targetTMU) {
    case TDFX_TMU0:
    case TDFX_TMU1:
        if (ti->tm[targetTMU]) {
            for (l = ti->minLevel; l <= ti->maxLevel
                    && tObj->Image[0][l]->Data; l++) {
                GrLOD_t glideLod = ti->info.largeLodLog2 - l + tObj->BaseLevel;
                fxMesa->Glide.grTexDownloadMipMapLevel(targetTMU,
                                                  ti->tm[targetTMU]->startAddr,
                                                  glideLod,
                                                  ti->info.largeLodLog2,
                                                  ti->info.aspectRatioLog2,
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  tObj->Image[0][l]->Data);
            }
        }
        break;
    case TDFX_TMU_SPLIT:
        if (ti->tm[TDFX_TMU0] && ti->tm[TDFX_TMU1]) {
            for (l = ti->minLevel; l <= ti->maxLevel
                    && tObj->Image[0][l]->Data; l++) {
                GrLOD_t glideLod = ti->info.largeLodLog2 - l + tObj->BaseLevel;
                fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU0,
                                                  ti->tm[TDFX_TMU0]->startAddr,
                                                  glideLod,
                                                  ti->info.largeLodLog2,
                                                  ti->info.aspectRatioLog2,
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_ODD,
                                                  tObj->Image[0][l]->Data);

                fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU1,
                                                  ti->tm[TDFX_TMU1]->startAddr,
                                                  glideLod,
                                                  ti->info.largeLodLog2,
                                                  ti->info.aspectRatioLog2,
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_EVEN,
                                                  tObj->Image[0][l]->Data);
            }
        }
        break;
    case TDFX_TMU_BOTH:
        if (ti->tm[TDFX_TMU0] && ti->tm[TDFX_TMU1]) {
            for (l = ti->minLevel; l <= ti->maxLevel
                    && tObj->Image[0][l]->Data; l++) {
                GrLOD_t glideLod = ti->info.largeLodLog2 - l + tObj->BaseLevel;
                fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU0,
                                                  ti->tm[TDFX_TMU0]->startAddr,
                                                  glideLod,
                                                  ti->info.largeLodLog2,
                                                  ti->info.aspectRatioLog2,
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  tObj->Image[0][l]->Data);

                fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU1,
                                                  ti->tm[TDFX_TMU1]->startAddr,
                                                  glideLod,
                                                  ti->info.largeLodLog2,
                                                  ti->info.aspectRatioLog2,
                                                  ti->info.format,
                                                  GR_MIPMAPLEVELMASK_BOTH,
                                                  tObj->Image[0][l]->Data);
            }
        }
        break;
    default:
        _mesa_problem(NULL, "%s: bad tmu (%d)", __FUNCTION__, (int)targetTMU);
        return;
    }
}


void
tdfxTMReloadMipMapLevel(GLcontext *ctx, struct gl_texture_object *tObj,
                        GLint level)
{
    tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    GrLOD_t glideLod;
    FxU32 tmu;

    tmu = ti->whichTMU;
    glideLod =  ti->info.largeLodLog2 - level + tObj->BaseLevel;
    ASSERT(ti->isInTM);

    LOCK_HARDWARE(fxMesa);

    switch (tmu) {
    case TDFX_TMU0:
    case TDFX_TMU1:
        fxMesa->Glide.grTexDownloadMipMapLevel(tmu,
                                    ti->tm[tmu]->startAddr,
                                    glideLod,
                                    ti->info.largeLodLog2,
                                    ti->info.aspectRatioLog2,
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    tObj->Image[0][level]->Data);
        break;
    case TDFX_TMU_SPLIT:
        fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU0,
                                    ti->tm[GR_TMU0]->startAddr,
                                    glideLod,
                                    ti->info.largeLodLog2,
                                    ti->info.aspectRatioLog2,
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_ODD,
                                    tObj->Image[0][level]->Data);

        fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU1,
                                    ti->tm[GR_TMU1]->startAddr,
                                    glideLod,
                                    ti->info.largeLodLog2,
                                    ti->info.aspectRatioLog2,
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_EVEN,
                                    tObj->Image[0][level]->Data);
        break;
    case TDFX_TMU_BOTH:
        fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU0,
                                    ti->tm[GR_TMU0]->startAddr,
                                    glideLod,
                                    ti->info.largeLodLog2,
                                    ti->info.aspectRatioLog2,
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    tObj->Image[0][level]->Data);

        fxMesa->Glide.grTexDownloadMipMapLevel(GR_TMU1,
                                    ti->tm[GR_TMU1]->startAddr,
                                    glideLod,
                                    ti->info.largeLodLog2,
                                    ti->info.aspectRatioLog2,
                                    ti->info.format,
                                    GR_MIPMAPLEVELMASK_BOTH,
                                    tObj->Image[0][level]->Data);
        break;

    default:
        _mesa_problem(ctx, "%s: bad tmu (%d)", __FUNCTION__, (int)tmu);
        break;
    }
    UNLOCK_HARDWARE(fxMesa);
}


/*
 * Allocate space for the given texture in texture memory then
 * download (copy) it into that space.
 */
void
tdfxTMMoveInTM_NoLock( tdfxContextPtr fxMesa, struct gl_texture_object *tObj,
                       FxU32 targetTMU )
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    FxU32 texmemsize;

    fxMesa->stats.reqTexUpload++;

    if (ti->isInTM) {
        if (ti->whichTMU == targetTMU)
            return;
        if (targetTMU == TDFX_TMU_SPLIT || ti->whichTMU == TDFX_TMU_SPLIT) {
            tdfxTMMoveOutTM_NoLock(fxMesa, tObj);
        }
        else {
            if (ti->whichTMU == TDFX_TMU_BOTH)
                return;
            targetTMU = TDFX_TMU_BOTH;
        }
    }

    ti->whichTMU = targetTMU;

    switch (targetTMU) {
    case TDFX_TMU0:
    case TDFX_TMU1:
        texmemsize = fxMesa->Glide.grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));
        ti->tm[targetTMU] = AllocTexMem(fxMesa, targetTMU, texmemsize);
        break;
    case TDFX_TMU_SPLIT:
        texmemsize = fxMesa->Glide.grTexTextureMemRequired(GR_MIPMAPLEVELMASK_ODD,
                                                       &(ti->info));
        ti->tm[TDFX_TMU0] = AllocTexMem(fxMesa, TDFX_TMU0, texmemsize);
        if (ti->tm[TDFX_TMU0])
           fxMesa->stats.memTexUpload += texmemsize;

        texmemsize = fxMesa->Glide.grTexTextureMemRequired(GR_MIPMAPLEVELMASK_EVEN,
                                                       &(ti->info));
        ti->tm[TDFX_TMU1] = AllocTexMem(fxMesa, TDFX_TMU1, texmemsize);
        break;
    case TDFX_TMU_BOTH:
        texmemsize = fxMesa->Glide.grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));
        ti->tm[TDFX_TMU0] = AllocTexMem(fxMesa, TDFX_TMU0, texmemsize);
        if (ti->tm[TDFX_TMU0])
           fxMesa->stats.memTexUpload += texmemsize;

        /*texmemsize = fxMesa->Glide.grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,
                                                       &(ti->info));*/
        ti->tm[TDFX_TMU1] = AllocTexMem(fxMesa, TDFX_TMU1, texmemsize);
        break;
    default:
        _mesa_problem(NULL, "%s: bad tmu (%d)", __FUNCTION__, (int)targetTMU);
        return;
    }

    ti->reloadImages = GL_TRUE;
    ti->isInTM = GL_TRUE;

    fxMesa->stats.texUpload++;
}


/*
 * Move the given texture out of hardware texture memory.
 * This deallocates the texture's memory space.
 */
void
tdfxTMMoveOutTM_NoLock( tdfxContextPtr fxMesa, struct gl_texture_object *tObj )
{
    struct gl_shared_state *mesaShared = fxMesa->glCtx->Shared;
    struct tdfxSharedState *shared = (struct tdfxSharedState *) mesaShared->DriverData;
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);

    if (MESA_VERBOSE & VERBOSE_DRIVER) {
        fprintf(stderr, "fxmesa: %s(%p (%d))\n", __FUNCTION__, (void *)tObj, tObj->Name);
    }

    /*
    VerifyFreeList(fxMesa, 0);
    VerifyFreeList(fxMesa, 1);
    */

    if (!ti || !ti->isInTM)
        return;

    switch (ti->whichTMU) {
    case TDFX_TMU0:
    case TDFX_TMU1:
        RemoveRange_NoLock(fxMesa, ti->whichTMU, ti->tm[ti->whichTMU]);
        break;
    case TDFX_TMU_SPLIT:
    case TDFX_TMU_BOTH:
        assert(!shared->umaTexMemory);
        RemoveRange_NoLock(fxMesa, TDFX_TMU0, ti->tm[TDFX_TMU0]);
        RemoveRange_NoLock(fxMesa, TDFX_TMU1, ti->tm[TDFX_TMU1]);
        break;
    default:
        _mesa_problem(NULL, "%s: bad tmu (%d)", __FUNCTION__, (int)ti->whichTMU);
        return;
    }

    ti->isInTM = GL_FALSE;
    ti->tm[0] = NULL;
    ti->tm[1] = NULL;
    ti->whichTMU = TDFX_TMU_NONE;

    /*
    VerifyFreeList(fxMesa, 0);
    VerifyFreeList(fxMesa, 1);
    */
}


/*
 * Called via glDeleteTexture to delete a texture object.
 */
void
tdfxTMFreeTexture(tdfxContextPtr fxMesa, struct gl_texture_object *tObj)
{
    tdfxTexInfo *ti = TDFX_TEXTURE_DATA(tObj);
    if (ti) {
        tdfxTMMoveOutTM(fxMesa, tObj);
        FREE(ti);
        tObj->DriverData = NULL;
    }
    /*
    VerifyFreeList(fxMesa, 0);
    VerifyFreeList(fxMesa, 1);
    */
}



/*
 * After a context switch this function will be called to restore
 * texture memory for the new context.
 */
void tdfxTMRestoreTextures_NoLock( tdfxContextPtr fxMesa )
{
   GLcontext *ctx = fxMesa->glCtx;
   struct _mesa_HashTable *textures = fxMesa->glCtx->Shared->TexObjects;
   GLuint id;

   for (id = _mesa_HashFirstEntry(textures);
        id;
        id = _mesa_HashNextEntry(textures, id)) {
      struct gl_texture_object *tObj
         = _mesa_lookup_texture(fxMesa->glCtx, id);
      tdfxTexInfo *ti = TDFX_TEXTURE_DATA( tObj );
      if ( ti && ti->isInTM ) {
         int i;
	 for ( i = 0 ; i < MAX_TEXTURE_UNITS ; i++ ) {
	    if ( ctx->Texture.Unit[i]._Current == tObj ) {
	       tdfxTMDownloadTexture( fxMesa, tObj );
	       break;
	    }
	 }
	 if ( i == MAX_TEXTURE_UNITS ) {
	    tdfxTMMoveOutTM_NoLock( fxMesa, tObj );
	 }
      }
   }
   /*
   VerifyFreeList(fxMesa, 0);
   VerifyFreeList(fxMesa, 1);
   */
}
