/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1996, Microsoft Corporation
 *
 *  WPARAM.C
 *
 *  Created:    VadimB
 *  Added cache VadimB
 *
-*/


#include "precomp.h"
#pragma hdrstop

MODNAME(wparam.c);

///////////////////////////////////////////////////////////////////////////
// Some defines


// Pre-allocated cache size for nodes
#define MAPCACHESIZE 0x1000 // 4K

// max "pointer movements" allowed per mapping
#define MAXNODEALIAS 0x10 // 16 aliases max
                          // (never ever seen more then 2 used)

// macro to generate the number of elements in array
#define ARRAYCOUNT(array) (sizeof(array)/sizeof((array)[0]))

// This define will enable code that allows for keeping 32-bit buffers
// allocated and integrated with nodes in cache
// #define MAPPARAM_EXTRA


///////////////////////////////////////////////////////////////////////////
typedef struct tagParamNode* LPPARAMNODE;

typedef struct tagParamNode {
  LPPARAMNODE pNext;

  DWORD dwPtr32;    // flat pointer
  DWORD dwPtr16;
  DWORD dwFlags;    // flags just in case
  DWORD dwRefCount; // reference count

#ifdef MAPPARAM_EXTRA
  DWORD cbExtra;     // buffer size
#endif

  DWORD nAliasCount;  // index for an alias array
  DWORD rgdwAlias[MAXNODEALIAS];

  // word sized member of the struct -- alignment alert
  HAND16 htask16;    // this is HAND16 really - keep simple and aligned

} PARAMNODE, *LPPARAMNODE;

typedef struct tagMapParam {

  LPPARAMNODE pHead;

  BLKCACHE  blkCache;

} MAPPARAM, *LPMAPPARAM;

typedef struct tagFindParam {
  LPPARAMNODE lpNode;
  LPPARAMNODE lpLast;
} FINDPARAM, *LPFINDPARAM;

MAPPARAM gParamMap;

/////////////////////////////////////////////////////////////////////////////
//
//  FindParamMap
//     Finds lParam in a list assuming it is 16-bit (fMode == PARAM_16) or
//  32-bit flat (fMode == PARAM_32) pointer
//
//  lpFindParam should be NULL or point to a valid FINDPARAM structure
//


DWORD FindParamMap(VOID* lpFindParam, DWORD lParam, UINT fMode)
{
    LPPARAMNODE lpn = gParamMap.pHead;
    LPPARAMNODE lplast = NULL;
    DWORD dwRet = 0;
    BOOL fFound = FALSE;

    switch(fMode) {

         case PARAM_16:
              while (NULL != lpn) {
                  if (lParam == lpn->dwPtr16) {
                      dwRet = lpn->dwPtr32;
                      break;
                  }
                  lplast = lpn;
                  lpn = lpn->pNext;
              }
              break;

         case PARAM_32:
              // We are looking for a 32-bit pointer
              // cases:
              // - exact match
              // - no match because ptr has moved (ouch!)

              while (NULL != lpn) {

                  INT i;

                  if (lParam == lpn->dwPtr32) {
                      fFound = TRUE;
                  }
                  else
                  if (lParam == (DWORD)GetPModeVDMPointer(lpn->dwPtr16, 0)) {
                      LOGDEBUG(LOG_ALWAYS,
                               ("WPARAM: Pointer has moved: 16:16 @%lx was 32 @%lx now @%lx\n",
                               lpn->dwPtr16, lpn->dwPtr32, lParam));
                      fFound = TRUE;
                  }
                  else {

                      // look through the list of aliases

                      for (i = 0; i < (INT)lpn->nAliasCount; ++i) {
                           if (lpn->rgdwAlias[i] == lParam) {
                               fFound = TRUE;
                               break;
                           }
                      }
                  }

                  if (fFound) {         // we found alias one way or the other...
                      dwRet = lpn->dwPtr16;
                      break;
                  }


                  lplast = lpn;
                  lpn = lpn->pNext;
              }
              break;
    }

    if (lpn)  {
        LPFINDPARAM lpfp = (LPFINDPARAM)lpFindParam;
        lpfp->lpNode = lpn;
        lpfp->lpLast = lplast;
    }

    return dwRet;
}

//
//     Find 32-bit param and return 16-bit equivalent
//
//


DWORD GetParam16(DWORD dwParam32)
{
    FINDPARAM fp;
    DWORD dwParam16;

    dwParam16 = FindParamMap(&fp, dwParam32, PARAM_32);
    if (dwParam16) {
        ++fp.lpNode->dwRefCount;
    }

    return dwParam16;
}

// set undead map entry
BOOL SetParamRefCount(DWORD dwParam, UINT fMode, DWORD dwRefCount)
{

   FINDPARAM fp;

   FindParamMap(&fp, dwParam, fMode);
   if (NULL != fp.lpNode) {
      fp.lpNode->dwRefCount = dwRefCount;
   }
   return(NULL != fp.lpNode);
}



//
// Typically this is called either from a thunk for an api or from
// 16->32 thunk for a message
//
// dwPtr32 most often is obtained by GETPSZPTR or GetPModeVdmPointer
//
//

PVOID AddParamMap(DWORD dwPtr32, DWORD dwPtr16)
{
    LPPARAMNODE lpn;
    FINDPARAM fp;

     // see if it's there already
    if (FindParamMap(&fp, dwPtr16, PARAM_16)) {

        lpn = fp.lpNode; // a bit faster ref

        ++lpn->dwRefCount; // increase ref count

        ParamMapUpdateNode(dwPtr32, PARAM_32, lpn); // just update the node
    }
    else {
        if (NULL != (lpn = CacheBlockAllocate(&gParamMap.blkCache, sizeof(*lpn)))) {
            lpn->dwPtr32 = dwPtr32;
            lpn->dwPtr16 = dwPtr16;
            lpn->pNext   = gParamMap.pHead;
            lpn->dwRefCount = 1;
#ifdef MAPPARAM_EXTRA
            lpn->cbExtra = 0;
#endif
            lpn->nAliasCount = 0;
            lpn->htask16 = CURRENTPTD()->htask16;
            gParamMap.pHead = lpn;
        }
    }

    return lpn ? (PVOID)lpn->dwPtr32 : NULL;
}

#ifdef MAPPARAM_EXTRA

PVOID AddParamMapEx(DWORD dwPtr16, DWORD cbExtra)
{
    LPPARAMNODE lpn;
    FINDPARAM fp;

    // see if it's there already
    if (FindParamMap(&fp, dwPtr16, PARAM_16)) {
        lpn = fp.lpNode;
        if (lpn->cbExtra == cbExtra) {
            ++lpn->dwRefCount;
        }
        else {
            WOW32ASSERTMSG(FALSE, ("\nWOW32: AddParamEx misused. Please contact VadimB or DOSWOW alias\n"));
            lpn = NULL;
        }
    }
    else {
        if (NULL != (lpn = CacheBlockAllocate(&gParamMap.blkCache, sizeof(*lpn) + cbExtra))) {
            lpn->dwPtr32 = (DWORD)(PVOID)(lpn+1);
            lpn->dwPtr16 = dwPtr16;
            lpn->pNext   = gParamMap.pHead;
            lpn->dwRefCount = 1;
            lpn->cbExtra = cbExtra;
            lpn->htask16 = CURRENTPTD()->htask16;
            gParamMap.pHead = lpn;
        }
    }

    return lpn ? (PVOID)lpn->dwPtr32 : NULL;
}

#endif

//
//  This should be called from the places we know pointers could get updated
//
//
PVOID ParamMapUpdateNode(DWORD dwPtr, UINT fMode, VOID* lpNode)
{
    LPPARAMNODE lpn;
    PVOID pv;

    if (NULL == lpNode) {
        FINDPARAM fp;
        if (FindParamMap(&fp, dwPtr, fMode)) {
            lpn = fp.lpNode; // node found!
        }
        else {
            LOGDEBUG(LOG_ALWAYS, ("WOW: ParamMapUpdateNode could not find node\n"));
            // return here as we've failed to find node same as we got in
            return (PVOID)dwPtr;
        }
    }
    else {
        lpn = (LPPARAMNODE)lpNode;
    }

    // if pointer is up-to-date then exit
    pv = GetPModeVDMPointer(lpn->dwPtr16, 0);
    if ((DWORD)pv == lpn->dwPtr32) {
        return pv; // up-to-date
    }
#ifdef MAPPARAM_EXTRA
    else
    if (0 < lpn->cbExtra) {
        return (PVOID)lpn->dwPtr32;
    }
#endif


    if (lpn->nAliasCount < ARRAYCOUNT(lpn->rgdwAlias)) {

        lpn->rgdwAlias[lpn->nAliasCount++] = lpn->dwPtr32;
    }
    else {
        WOW32ASSERTMSG(FALSE, ("WOW:AddParamMap is out of alias space\n"));
        // so we will throw the oldest alias out - this will mean if they refer
        // to it - they are doomed... That is why we assert here!
        lpn->rgdwAlias[0] = lpn->dwPtr32;
    }

    lpn->dwPtr32 = (DWORD)pv; // new pointer here

    return pv;
}


//
// lParam    - 16- or 32-bit pointer (see fMode)
// fMode     - PARAM_16 or PARAM_32 - specifies what lParam represents
// pfFreePtr - points to a boolean that receives TRUE if caller should
//             do a FREEVDMPTR on a 32-bit parameter
// Returns TRUE if parameter was found and FALSE otherwise
//


BOOL DeleteParamMap(DWORD lParam, UINT fMode, BOOL* pfFreePtr)
{
    FINDPARAM fp;
    LPPARAMNODE lpn = NULL;

    if (FindParamMap(&fp, lParam, fMode)) {
        lpn = fp.lpNode;

        if (!--lpn->dwRefCount) {

            if (NULL != fp.lpLast) {
                fp.lpLast->pNext = lpn->pNext;
            }
            else {
                gParamMap.pHead = lpn->pNext;
            }

            if (NULL != pfFreePtr) {
#ifdef MAPPARAM_EXTRA
                *pfFreePtr = !!lpn->cbExtra;
#else
                *pfFreePtr = FALSE;
#endif
            }
            CacheBlockFree(&gParamMap.blkCache, lpn);
        }
        else {
            LOGDEBUG(12, ("\nWOW: DeleteParamMap called refCount > 0 Node@%x\n", (DWORD)lpn));

            if (NULL != pfFreePtr) { // not done with mapping yet
                *pfFreePtr = FALSE;
            }
        }
    }
    else {
        LOGDEBUG(LOG_ALWAYS, ("\nWOW: DeleteParamMap called but param was not found\n"));
        if (NULL != pfFreePtr) {
            *pfFreePtr = TRUE; // we found none, assume free
        }
    }

    return NULL != lpn;
}

BOOL W32CheckThunkParamFlag(void)
{
    return !!(CURRENTPTD()->dwWOWCompatFlags & WOWCF_NOCBDIRTHUNK);
}

//
//  This function is called to cleanup all the leftover items in case
//  application is dead. Please note, that it should not be called in
//  any other case ever.
//
//

VOID FreeParamMap(HAND16 htask16)
{
    LPPARAMNODE lpn = gParamMap.pHead;
    LPPARAMNODE lplast = NULL, lpnext;

    while (NULL != lpn) {

        lpnext = lpn->pNext;

        if (lpn->htask16 == htask16) {

            if (NULL != lplast) {
                lplast->pNext = lpnext;
            }
            else {
                gParamMap.pHead = lpnext;
            }

            CacheBlockFree(&gParamMap.blkCache, lpn);
        }
        else {
            lplast = lpn;
        }

        lpn = lpnext;
    }
}

VOID InitParamMap(VOID)
{
    CacheBlockInit(&gParamMap.blkCache, MAPCACHESIZE);
}


////////////////////////////////////////////////////////////////////////////
//
// Cache manager
//
//

// This is a rather simplistic allocator which uses stack-like allocation
// as this is the pattern in which allocation/free is being used
// each block is preceded by a 2-dword header indicating it's size


/*

    Note:

    1. Free Blocks are included in the list in the order of descending
        address value, that is, the free block with the highest address
        goes first. This leads allocator not to re-use free blocks unless
        there is no more memory left
    2. When the block is allocated, it is chipped away from the first block
        that fits (no best-fit or other allocating strategy).
    3. When the block is being freed, it is inserted in appropriate place in
        the list of free blocks or appended to the existing block

    Usually allocations occur on first in - first out basis. These points
    above provide for minimal overhead in this scenario. In more complicated
    cases (when hooks are installed and some other crazy things happen) it
    could be necessary to free block that was allocated out-of order
    In this case this block would be included somewhere in the free list
    and possibly re-used.

    The list of free blocks never needs compacting as it could never become
    fragmented.

    My performance testing suggests that 95% of allocations occur in a stack-
    like fashion. The most often hit code path is optimized for this case.
    With random allocations (which is not the case with wow thunks)
    the ratio of left merges to right(more effective) merges on 'free' calls
    is 3:1. With wow thunks it is more like 1:10.

*/


BOOL IsCacheBlock(PBLKCACHE pc, LPVOID pv);


#define LINK_FREELIST(pc, pNew, pLast) \
if (NULL == pLast) { \
    pc->pCacheFree = pNew; \
} \
else { \
    pLast->pNext = pNew; \
}

#ifdef DEBUG
#define LINK_WORKLIST(pc, pNew, pLast) \
if (NULL == pLast) { \
    pc->pCacheHead = pNew; \
} \
else { \
    pLast->pNext = pNew; \
}
#else
#define LINK_WORKLIST(pc, pNew, pLast)
#endif

VOID CacheBlockInit(PBLKCACHE pc, DWORD dwCacheSize)
{
    PBLKHEADER pCache = (PBLKHEADER)malloc_w(dwCacheSize);

    RtlZeroMemory(pc, sizeof(*pc));

    if (NULL != pCache) {
        pc->pCache = (LPBYTE)pCache;
        pc->pCacheFree = pCache;
        pc->dwCacheSize= dwCacheSize;
        pCache->dwSize = dwCacheSize;
        pCache->pNext  = NULL;
    }
}

LPVOID CacheBlockAllocate(PBLKCACHE pc, DWORD dwSize)
{
    LPVOID lpv;

    // suballocate a block from the free list

    if (NULL != pc->pCacheFree) {

        PBLKHEADER pbh = pc->pCacheFree;
        PBLKHEADER pbhLast = NULL;
        DWORD dwSizeBlk;

        // dword - align dwSizeBlk, sizeof(DWORD) is power of 2 always
        dwSizeBlk = (dwSize + sizeof(BLKHEADER) + (sizeof(DWORD) - 1)) & ~(sizeof(DWORD)-1);

        // so we allocate from the highest address in hopes of filling holes
        // almost always this will be the largest block around

        while (NULL != pbh) {
            if (pbh->dwSize >= dwSizeBlk) { // does this block fit ?

                if (pbh->dwSize - dwSizeBlk > sizeof(BLKHEADER)) { // do we keep the leftovers ?

                    // most often hit - chip off from the end

                    pbh->dwSize -= dwSizeBlk;

                    // now on to the new chunk

                    pbh = (PBLKHEADER)((LPBYTE)pbh + pbh->dwSize);
                    pbh->dwSize = dwSizeBlk;
                }
                else {

                    // less likely case - entire block will be used
                    // so unlink from the free list

                    LINK_FREELIST(pc, pbh->pNext, pbhLast);
                }

                // include into busy blocks
#ifdef DEBUG
                pbh->pNext = pc->pCacheHead;
                pc->pCacheHead = pbh;
#endif
                return (LPVOID)(pbh+1);
           }

           pbhLast = pbh;
           pbh = pbh->pNext;
        }

    }

    // no free blocks
    if (NULL == (lpv = (LPPARAMNODE)malloc_w(dwSize))) {
        LOGDEBUG(2, ("Malloc failure in CacheBlockAllocate\n"));
    }

    return (lpv);
}


VOID CacheBlockFree(PBLKCACHE pc, LPVOID lpv)
{
    if (IsCacheBlock(pc, lpv)) {
        PBLKHEADER pbh = (PBLKHEADER)lpv - 1;

#ifdef DEBUG
        PBLKHEADER pbhf = pc->pCacheHead;
        PBLKHEADER pbhLast = NULL;

        // remove from the list of working nodes
        while (NULL != pbhf && pbhf != pbh) {
            pbhLast = pbhf;
            pbhf = pbhf->pNext;
        }

        if (NULL != pbhf) {

            // link in pbh->pNext into a worklist

            LINK_WORKLIST(pc, pbh->pNext, pbhLast);
        }
        else {
            LOGDEBUG(LOG_ALWAYS, ("Alert! CacheBlockFree - invalid ptr\n"));
            return;
        }

        pbhf = pc->pCacheFree;
        pbhLast = NULL;

#else
        PBLKHEADER pbhf = pc->pCacheFree;
        PBLKHEADER pbhLast = NULL;
#endif
        // list of free nodes

        // insert in order
        while (NULL != pbhf) {

            // most often case - append from the right

            if (((LPBYTE)pbhf + pbhf->dwSize) == (LPBYTE)pbh) {

                pbhf->dwSize += pbh->dwSize; // adjust the size

                // now see if we need compact
                if (NULL != pbhLast) {
                    if (((LPBYTE)pbhf + pbhf->dwSize) == (LPBYTE)pbhLast) {
                        // consolidate
                        pbhLast->dwSize += pbhf->dwSize;
                        pbhLast->pNext   = pbhf->pNext;
                    }
                }

                return;
            }
            else
            // check if we can append from the left
            if (((LPBYTE)pbh + pbh->dwSize) == (LPBYTE)pbhf) {

                pbh->dwSize += pbhf->dwSize;    // adjust the size
                pbh->pNext   = pbhf->pNext;     // next ptr too

                // now also check the next free ptr so we can compact
                // the next ptr has lesser address

                if (NULL != pbh->pNext) {
                    pbhf = pbh->pNext;

                    if (((LPBYTE)pbhf + pbhf->dwSize) == (LPBYTE)pbh) {

                        pbhf->dwSize += pbh->dwSize;
                        pbh = pbhf;
                    }
                }

                LINK_FREELIST(pc, pbh, pbhLast);

                return;
            }

            // check for address

            if (pbh > pbhf) {
                // we have to link-in a standalone block
                break;
            }

            pbhLast = pbhf;
            pbhf = pbhf->pNext; // on to the next block
        }

        // LOGDEBUG(LOG_ALWAYS, ("Param Map Cache: OUT-OF-ORDER free!!!\n"));

        pbh->pNext = pbhf;

        LINK_FREELIST(pc, pbh, pbhLast);

    }
    else {
        free_w(lpv);
    }
}

BOOL IsCacheBlock(PBLKCACHE pc, LPVOID pv)
{
    LONG lOffset = (LONG)pv - (LONG)pc->pCache;
    return (lOffset >= 0 && lOffset < (LONG)pc->dwCacheSize);
}







