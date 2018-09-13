//*****************************************************************************
//
// MESSAGE HEAP 16 -
//
//     Heap allocation functions for 32-16 message thunks.
//
//     NOTE: these are NOT general purpose heap managment routines.
//
//
// 07-17-92  NanduriR   Created.
//
//*****************************************************************************

#include "precomp.h"
#pragma hdrstop

MODNAME(walloc16.c);

//*****************************************************************************
// General Notes:
//
// This heap maanger is for specific 'performance gains' and is thus not
// intended for general purpose use and therfore much of the overhead has
// been eliminated.
//
// This heap manager is intended mainly for thunks - where we are sure that
// an alloced block will definitely be freed. Thus it is meant for our use.
//
// The heap is conceptually an array  of constant-size blocks. The size of the
// block is predefined. The code is optimized for allocation requests of one
// blocksize or less. It is slower if the allocation request needs more than
// one block.
//
// The heap header is a static array. The header has two flags. One to note
// that a particular heapblock is in use and the other to note whether the
// block forms a part of a linked/chained set of contiguous blocks. The blocks
// are linked when the allocation request is for more than the predefined
// block size.
//
//*****************************************************************************


#define HEAP16_TOTALSIZE  0x2000
#define HEAP16_BLOCKSIZE  0x100    // We should set it to an optimum value
#define HEAP16_BLOCKCOUNT (HEAP16_TOTALSIZE/HEAP16_BLOCKSIZE)

#define HHDR16_FINUSE     0x01
#define HHDR16_FLINKED    0x02

#define ISBLOCKINUSE(block)    ((block) & HHDR16_FINUSE)
#define ISBLOCKLINKED(block) ((block) & HHDR16_FLINKED)


//*****************************************************************************
//
// Globals -
//
// vahdr - is the heap header. This header is 32bit memory and not part of the
//         16bit heap - this is so that the 16bit heap is put to maximum use.
//
// vpHeap16 - far pointer to the start of 16bit heap
//
// viFreeIndex - the index from which we start searching for a freeblock.
//               Normally this is set to the memoryblock that was most
//               recently freed, thus increasing the chances of finding
//               a freeblock instantly.
//*****************************************************************************

BYTE    vahdr[HEAP16_BLOCKCOUNT];
LPBYTE  vpHeap16 = (LPBYTE)NULL;
UINT    viFreeIndex = 0;            // First look for Free block here.



//*****************************************************************************
//
// malloc16 -
//
//      Allocs memory from 16bit block.
//      If heap is full, does normal GlobalAlloc
//
//      Returns farpointer to memoryblock;
//
//*****************************************************************************

VPVOID FASTCALL malloc16(UINT cb)
{
    INT i, j;
    INT cBlocksRequired;
    INT fContiguousFreeBlocks;
    INT vpT;

    if (vpHeap16 == (LPBYTE)NULL) {
        vpHeap16 = (LPBYTE)GlobalAllocLock16(GMEM_MOVEABLE | GMEM_SHARE, HEAP16_TOTALSIZE,
                                                                         NULL);
        if (vpHeap16 != NULL) {

            //
            // Initialize heap header.
            // LATER: is this necessary?. Heaphdr is a static array so
            //        is it already intialized to ZERO?
            //

            for(i = 0; i < HEAP16_BLOCKCOUNT ; i++) {
                vahdr[i] = 0;
            }

        }

    }

    if (vpHeap16 != (LPBYTE)NULL) {
        if (cb <= HEAP16_BLOCKSIZE && !ISBLOCKINUSE(vahdr[viFreeIndex])) {

            //
            // If 'single' block and the 'current' index is free.
            //

            vahdr[viFreeIndex] = HHDR16_FINUSE;
            i = viFreeIndex++;
            if (viFreeIndex == HEAP16_BLOCKCOUNT)
                viFreeIndex = 0;
            return (VPVOID)((LPBYTE)vpHeap16 + i * HEAP16_BLOCKSIZE);
        }
        else {

            //
            // if the 'current' index is not free or if 'multiple' blocks
            //

            cBlocksRequired = (cb / HEAP16_BLOCKSIZE) + 1;
            for (i = 0; i < HEAP16_BLOCKCOUNT ; i++ ) {
                 if ((viFreeIndex + i + cBlocksRequired) <=
                                                          HEAP16_BLOCKCOUNT) {
                     fContiguousFreeBlocks = TRUE;
                     for (j = 0; j < cBlocksRequired; j++) {
                         if (ISBLOCKINUSE(vahdr[viFreeIndex + i + j])) {
                             fContiguousFreeBlocks = FALSE;
                             i += j;
                             break;
                         }
                     }

                     if (fContiguousFreeBlocks) {
                         for (j = 0; j < (cBlocksRequired - 1); j++) {
                              vahdr[viFreeIndex + i + j] =
                                             (HHDR16_FINUSE |  HHDR16_FLINKED);
                         }
                         vahdr[viFreeIndex + i + j] = HHDR16_FINUSE;

                         i += viFreeIndex;
                         viFreeIndex = i + cBlocksRequired;
                         if (viFreeIndex == HEAP16_BLOCKCOUNT)
                             viFreeIndex = 0;
                         return (VPVOID)((LPBYTE)vpHeap16 + i * HEAP16_BLOCKSIZE);
                     }
                 }
                 else {

                     //
                     // Outside the heaphdr range. Reset viFreeIndex, so that
                     // we search from the start of heaphdr.
                     //

                     viFreeIndex = -(i+1);
                 }
            }
            viFreeIndex = 0;
        }

    }

    //
    // Here - if allocation from heap failed
    //

    vpT = (VPVOID)GlobalAllocLock16(GMEM_MOVEABLE, cb, NULL);
    if (vpT) {
        return vpT;
    }
    else {
        LOGDEBUG(0,("malloc16: failed\n"));
        return (VPVOID)NULL;
    }
}



//*****************************************************************************
//
// free16 -
//
//      frees 16bit memory block.
//      If the block is not part of the 16bit heap, does GlobalFree.
//
//      Returns TRUE;
//
//*****************************************************************************

BOOL FASTCALL free16(VPVOID vp)
{
    INT iStartIndex;
    BOOL fLinked;

    iStartIndex = ((LPBYTE)vp - (LPBYTE)vpHeap16) / HEAP16_BLOCKSIZE;

    //
    // Invalid iStartIndex implies that the block was GlobalAlloced
    //

    if (iStartIndex >= 0 && iStartIndex < HEAP16_BLOCKCOUNT) {

        //
        // If   'single'   block: get out fast
        // else 'multiple' block: loop for all the blocks
        //

        viFreeIndex = iStartIndex;
        if (!ISBLOCKLINKED(vahdr[iStartIndex])) {
            WOW32ASSERT(ISBLOCKINUSE(vahdr[iStartIndex]));
            vahdr[iStartIndex] = 0;
        }
        else {
            while (ISBLOCKINUSE(vahdr[iStartIndex])) {
                fLinked = ISBLOCKLINKED(vahdr[iStartIndex]);
                vahdr[iStartIndex] = 0;
                if (fLinked)
                    iStartIndex++;
                else
                    break;
            }
        }
    }
    else {
        WOW32ASSERT(LOWORD(vp)==0);  // GlobalAlloced pointers have offset = 0
        GlobalUnlockFree16(vp);
    }
    return (BOOL)TRUE;
}


//*****************************************************************************
//
// stackalloc16 -
//
//      Allocs memory from current task's 16bit stack.
//      Returns farpointer to memoryblock;
//
// NOTES!!!!!:
// - This is not intended to be a full blown memory manager. It is intended to 
//   replace the TDF_INITCALLBACKSTACKFLAG to avoid problems with ptd->vpCBStack
//   getting hosed by multiple stackalloc16 calls. (See bug #393267 et al)
// - All calls to stackalloc16() and stackfree16() must be properly nested.
//   If you can't assure that your usage is properly nested, you'd better use
//   GlobalAllocLock16() & GlobalUnlockFree16() instead.
//   Beware individual message thunks may have calls to stackalloc16() that
//   aren't readily apparent.
// - The request size passed to stackfree16() needs to be the same as the size
//   of the corresponding stackalloc16() call.
// - Run your changes involving stackalloc16() under debug WOW32.DLL.  The
//   built-in sanity checking will help you catch any gotcha's
// - ptd->vpCBStack *SHOULD NOT* be referenced or used outside of stackalloc16()
//   stackfree16(), and callback16().
// - If this mechanism gets out of whack, chances are the symptom will be
//   a 16-bit stack fault message.
//*****************************************************************************
VPVOID FASTCALL stackalloc16(UINT cb)
{
#ifdef DEBUG
    VPVOID   vp;
    DWORD   *psig, cb16;
#endif
    register PTD ptd;


    // get current task's 16bit stack
    ptd = CURRENTPTD();

#ifdef DEBUG
    // Save requested allocation size. Assume it will always be less than 64K
    cb16 = (DWORD)cb;      

    // Add a dword (for signature) to the requested size
    cb += sizeof(DWORD);   

    // Get the current callback sp
    if (ptd->cStackAlloc16 == 0) {
        vp = ptd->vpStack;
    } else {
        vp = ptd->vpCBStack;
    }
#endif

    // grow ss:sp and return this imaginary pointer.
    if (ptd->cStackAlloc16 == 0) {
        ptd->vpCBStack = ptd->vpStack - cb;
    }
    else {
        ptd->vpCBStack -= cb;
    }

#ifdef DEBUG
        // Slide our DWORD signature in after the allocated request
        vp -= sizeof(DWORD);
        GETVDMPTR(vp, sizeof(DWORD), psig);

        // The signature hiword is the offset (sp) we're returning.
        // The signature loword is the requested size.
        *psig = ((ptd->vpCBStack & 0x0000FFFF) << 16) | cb16;
        FLUSHVDMPTR(vp, sizeof(DWORD), psig);
        FREEVDMPTR(psig);
#endif

    ptd->cStackAlloc16++;
    WOW32ASSERT((ptd->cStackAlloc16 >= 1));

    return (VPVOID)ptd->vpCBStack;
}





//*****************************************************************************
//
// StackFree16 -
//
//  Decrements count of memory alloc'd by stackalloc16 
//
//  NOTES:  
//  - This is #define'd as stackfree16(vp,cb) StackFree16(cb) in free builds
//    and stackfree16(vp,cb) StackFree16(vp, cb) in DEBUG builds (wcall16.h)
//  - See stackalloc16() NOTES above
//
//*****************************************************************************
#ifdef DEBUG
VOID FASTCALL StackFree16(VPVOID vp, UINT cb)
#else
VOID FASTCALL StackFree16(UINT cb)
#endif
{
    register PTD ptd;
#ifdef DEBUG
    DWORD  *psig, sig;

    // reconstruct what our signature should be 
    sig = ((vp & 0x0000FFFFF) << 16) | cb;
#endif

    ptd = CURRENTPTD();

    ptd->vpCBStack += cb;

#ifdef DEBUG
    // vpCBStack should now be pointing at our signature.
    GETVDMPTR(ptd->vpCBStack, sizeof(DWORD), psig);

    // you are hitting this assertion for one of the following reasons:
    //  - calls to stackalloc16() & stackfree16() are not properly nested
    //  - the signature got overwritten
    //  - somebody changed ptd->vpCBStack incorrectly
    WOW32ASSERTMSG((*psig == sig), ("WOW::StackFree16 out of synch!!\n"));

    // adjust for the signature DWORD we added to the request
    ptd->vpCBStack += sizeof(DWORD);
#endif

    if(ptd->cStackAlloc16 > 0) {
        ptd->cStackAlloc16--;
    } else { 
        WOW32ASSERTMSG((FALSE), ("WOW::StackFree16:cStackAlloc16 <= 0!\n"));
        ptd->cStackAlloc16 = 0;  // if it was less than 0 somehow
    }
}

