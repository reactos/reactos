//+------------------------------------------------------------------------
//
//  File:       memutil.cxx
//
//  Contents:   Memory utilities for Win16
//
//  History:    20-Jan-97   Stevepro    Ported old GTR_* routines.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

DeclareTag(tagWin16Mem, "Memory Allocation", "Trace All Memory Allocations")

//
// General Tagging Info
//
typedef struct {
    WORD wMemType;
    WORD wBegTag; // for padding & for general tagging.

#if DBG>=1
    DWORD dwSize;
#endif

} MEM_TAG, FAR * LPMEM_TAG;

// this number should be adjusted.
#define MEM_MAX_FMALLOC 0x1000
#define MEMTRACK_ALLOC 1
#define MEMTRACK_FREE  2
const UINT MEM_FMALLOC_MEMORY = 0x6969;
const UINT MEM_GLOBAL_MEMORY = 0xA5A5;
const long BLOCK_64K = 0x10000;
//#define MEM_POOL_128       0xFABB

#define DRV_BEG_TAG        0x1234
#define MemTrack(a, b, c)

#if DBG==1
long lGlobalAllocCount;
long lfMallocCount;
long lMemInUse=0; // total memory in use.
#endif

#ifndef MEMAPI_H__
extern "C" void hmemset(void huge *pMem, int value, long memSize);

//+------------------------------------------------------------------------
//
//  Function:   hmemset
//
//  Synopsis:   Set a huge block of memory to a given value.
//
//  Arguments:  [Mem] - Pointer to memory.
//              [value] - character to set
//              [memSize] - number of characters
//
//-------------------------------------------------------------------------
void hmemset(void huge *pMem, int value, long memSize)
{
    char huge *hpdst = (char huge *)pMem;

    while (memSize > 0)
    {
        long memCount;
        if (OFFSETOF(hpdst) + memSize < BLOCK_64K)
        {
            memCount =  memSize;
        }
        else
        {
            memCount = BLOCK_64K - OFFSETOF(hpdst);
        }
        memset(hpdst, value, memCount);
        hpdst += memCount;
        memSize -= memCount;
    }
}

//+------------------------------------------------------------------------
//
//  Function:   MemAlloc
//
//  Synopsis:   Allocate block of memory.
//
//              The contents of the block are undefined.  If the requested size
//              is zero, this function returns a valid pointer.  The returned
//              pointer is guaranteed to be suitably aligned for storage of any
//              object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void FAR *
MemAlloc(ULONG cb)
{
    LPMEM_TAG lpMemTag;
    WORD wMemType;
    BOOL fAllocFailed = FALSE;

    AssertSz(cb > 0, "cb is <= 0\n");
    Assert(_fheapchk() == _HEAPOK);

    TraceTag((tagWin16Mem, "Allocation %ld bytes", cb));
    
    //
    // First thing is decide what kind of memory to allock
    //

    cb += sizeof(MEM_TAG);

malloc_retry:
    if (cb > MEM_MAX_FMALLOC)
    {
        //
        // Since its greater than the threshold, GlobalAlloc it...
        //
        lpMemTag = (LPMEM_TAG)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, cb);
        wMemType = MEM_GLOBAL_MEMORY;
#if DBG==1
        lGlobalAllocCount++;
#endif
    }
    else
    {
        //
        // Below the threshold, lets _fmalloc
        //
        lpMemTag = (LPMEM_TAG)_fmalloc( (size_t) cb );
        wMemType = MEM_FMALLOC_MEMORY;
#if DBG==1
        lfMallocCount++;
#endif
    }

    // If our memory allocation failed, let's see
    // if something can be done about it.
    if (!lpMemTag)
    {
        if (!fAllocFailed)
        {
            fAllocFailed = TRUE;
            Assert(0 && "Memory Allocation Failed");
            goto malloc_retry;
        }
    }

    AssertSz(lpMemTag, "Memory allocation failed\n");

    //
    // Set up the rest of the info
    //
    if (lpMemTag)
    {

        lpMemTag->wMemType = wMemType;

#if DBG>=1
        lpMemTag->wBegTag = DRV_BEG_TAG;
        lpMemTag->dwSize = cb;

        MemTrack(MEMTRACK_ALLOC, NULL, cb);
#endif
        lpMemTag++;
    }

#if DBG==1
    if ( lpMemTag )
        lMemInUse += cb;
    TraceTag((tagWin16Mem,"Memory in use = %ld", lMemInUse));
#endif
    return lpMemTag;
}

//+------------------------------------------------------------------------
//
//  Function:   MemFree
//
//  Synopsis:   Free a block of memory allocated with MemAlloc,
//              MemAllocFree or MemRealloc.
//
//  Arguments:  [pv] - Pointer to block to free.  A value of zero is
//              is ignored.
//
//-------------------------------------------------------------------------

void
MemFree(void FAR *pv)
{
    LPMEM_TAG lpMemTag;

    Assert(_fheapchk() == _HEAPOK);
    if (pv)
    {
        AssertSz((OFFSETOF(pv) >= sizeof(MEM_TAG)),
                "Invalid Memory passed in\n");

        lpMemTag = (LPMEM_TAG)pv;
        lpMemTag--;

        AssertSz(lpMemTag->wBegTag == DRV_BEG_TAG, "Invalid Begin Tag !!\n");
		
		Assert(lpMemTag->wMemType == MEM_GLOBAL_MEMORY || lpMemTag->wMemType == MEM_FMALLOC_MEMORY);

#if DBG==1
    if ( lpMemTag )
        lMemInUse -= lpMemTag->dwSize;
    TraceTag((tagWin16Mem,"Memory in use = %ld", lMemInUse));
    Assert(lMemInUse >= 0);
#endif

#if DBG>=1
        MemTrack(MEMTRACK_FREE, NULL, lpMemTag->dwSize);

        // fill in the memory with '16'
        hmemset(pv, 0x16, lpMemTag->dwSize - sizeof(MEM_TAG));
#endif
	    TraceTag((tagWin16Mem, "FREE %ld bytes", lpMemTag->dwSize - sizeof(MEM_TAG)));

        switch(lpMemTag->wMemType)
        {
        case MEM_GLOBAL_MEMORY:
            lpMemTag->wMemType = 0x6161;
            Verify(GlobalFreePtr((void FAR*)lpMemTag) == 0);
#if DBG==1
            lGlobalAllocCount--;
#endif
            break;
        case MEM_FMALLOC_MEMORY:
            lpMemTag->wMemType = 0x1616;
            _ffree((void FAR*)lpMemTag);
#if DBG==1
            lfMallocCount--;
#endif
            break;
        default:
            AssertSz(FALSE, "Free: Unknown mem type\n");
            break;
        }
    }
}
#endif

//+------------------------------------------------------------------------
//  Function:   MemRealloc
//
//  Synopsis:   Change the size of an existing block of memory, allocate a
//              block of memory, or free a block of memory depending on the
//              arguments.
//
//              If cb is zero, this function always frees the block of memory
//              and *ppv is set to zero.
//
//              If cb is not zero and *ppv is zero, then this function allocates
//              cb bytes.
//
//              If cb is not zero and *ppv is non-zero, then this function
//              changes the size of the block, possibly by moving it.
//
//              On error, *ppv is left unchanged.  The block contents remains
//              unchanged up to the smaller of the new and old sizes.  The
//              contents of the block beyond the old size is undefined.
//              The returned pointer is guaranteed to be suitably aligned for
//              storage of any object type.
//
//              The signature of this function is different than thy typical
//              realloc-like function to avoid the following common error:
//                  pv = realloc(pv, cb);
//              If realloc fails, then null is returned and the pointer to the
//              original block of memory is leaked.
//
//  Arguments:  [cb] - Requested size in bytes. A value of zero always frees
//                  the block.
//              [ppv] - On input, pointer to existing block pointer or null.
//                  On output, pointer to new block pointer.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
//#include "stdio.h"
HRESULT
MemRealloc(void **ppv, ULONG cb)
{
    LPMEM_TAG lpMemTag;
    BOOL fAllocFailed = FALSE;

    TraceTag((tagWin16Mem, "REALLOC: %ld bytes", cb));

    if (cb == 0)
    {
        MemFree(*ppv);
        *ppv = 0;
    }
    else if (*ppv == NULL)
    {
        *ppv = MemAlloc(cb);
        if (*ppv == NULL)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {

        AssertSz((OFFSETOF(ppv) >= sizeof(MEM_TAG)),
                  "Invalid Memory passed in\n" );

        lpMemTag = (LPMEM_TAG)*ppv;
        lpMemTag--;

        AssertSz((lpMemTag->wBegTag == DRV_BEG_TAG), "Invalid Begin Tag !!\n");

        TraceTag((tagWin16Mem, "REALLOC: was %ld bytes need %ld bytes", lpMemTag->dwSize, cb));

        cb += sizeof(MEM_TAG);

#if DBG==1
        if ( lpMemTag )
            lMemInUse += (cb - lpMemTag->dwSize);
        TraceTag((tagWin16Mem,"Memory in use = %ld", lMemInUse));
        Assert(lMemInUse >= 0);
#endif

        switch(lpMemTag->wMemType)
        {
        case MEM_GLOBAL_MEMORY:

            lpMemTag = (LPMEM_TAG)GlobalReAllocPtr( (void FAR *) lpMemTag,
                                            cb,
                                            GMEM_MOVEABLE | GMEM_SHARE);
            break;

        case MEM_FMALLOC_MEMORY:

            // If the new block will cross a segment boundry, _frealloc will ignore the request!
            // So we will reallocate the memory ourselves (convert these to use GlobalAlloc if we
            // exceed 64K in size).  Note that normally, we avoid GlobalAlloc because it has
            // 32-bytes of overhead and uses a precious selector.  There are only 8192 selectors
            // for the whole system in Windows 3.x!
            //
            if ((cb < MEM_MAX_FMALLOC) && (OFFSETOF(lpMemTag) + cb < BLOCK_64K))
            {
                // No segment crossing so we can use _frealloc
                lpMemTag = (LPMEM_TAG)_frealloc(lpMemTag, cb);
            }
            else
            {
                // Crosses segment so reallocate (using global alloc for large buffers)
                LPMEM_TAG lpNewMem = (LPMEM_TAG)MemAlloc(cb);
                if (lpNewMem)
                {
                    // Copy over to new buffer
                    memcpy(lpNewMem, lpMemTag+1, _fmsize(lpMemTag) - sizeof(MEM_TAG));

                    // Free the old memory
                    MemFree(lpMemTag+1);

                    lpMemTag = --lpNewMem;
                }
                else
                {
                    // We're in trouble, so try _frealloc and pray for a miracle
                    lpMemTag = (LPMEM_TAG)_frealloc(lpMemTag, cb);
                }
            }
            break;

        default:
            AssertSz(FALSE, "Realloc: Unknown mem type\n");
            break;
        }

        // If our memory allocation failed, let's see
        // if something can be done about it.
        if (!lpMemTag)
        {   
        	Assert(0 && "Memory Allocation Failed");
            return E_OUTOFMEMORY;
        }

#if DBG>=1
        lpMemTag->dwSize = cb;
#endif
        *ppv = ++lpMemTag;
    }

    return S_OK;
}

#ifndef MEMAPI_H__
//+------------------------------------------------------------------------
//  Function:   MemAllocClear
//
//  Synopsis:   Allocate a zero filled block of memory.
//
//              If the requested size is zero, this function returns a valid
//              pointer. The returned pointer is guaranteed to be suitably
//              aligned for storage of any object type.
//
//  Arguments:  [cb] - Number of bytes to allocate.
//
//  Returns:    Pointer to the allocated block, or NULL on error.
//
//-------------------------------------------------------------------------
void FAR *
MemAllocClear(ULONG cb)
{
    void FAR *ptrmem;

    ptrmem = MemAlloc(cb);
    if (ptrmem)
    {
        hmemset(ptrmem, 0, cb);
    }

    return ptrmem;
}

//+------------------------------------------------------------------------
//
//  Function:   MemGetSize
//
//  Synopsis:   Get size of block allocated with MemAlloc/MemRealloc.
//
//              Note that MemAlloc/MemRealloc can allocate more than
//              the requested number of bytes. Therefore the size returned
//              from this function is possibly greater than the size
//              passed to MemAlloc/Realloc.
//
//  Arguments:  [pv] - Return size of this block.
//
//  Returns:    The size of the block, or zero of pv == NULL.
//
//-------------------------------------------------------------------------

ULONG
MemGetSize(void *pv)
{
    LPMEM_TAG lpMemTag;

    if (pv)
    {
        AssertSz((OFFSETOF(pv) >= sizeof(MEM_TAG)),
                "Invalid Memory passed in\n");

        lpMemTag = (LPMEM_TAG)pv;
        lpMemTag--;

        AssertSz(lpMemTag->wBegTag == DRV_BEG_TAG, "Invalid Begin Tag !!\n");

        switch(lpMemTag->wMemType)
        {
        case MEM_GLOBAL_MEMORY:
            return(GlobalSize(GlobalPtrHandle(lpMemTag)) - sizeof(MEM_TAG));
            break;
        case MEM_FMALLOC_MEMORY:
            return(_fmsize(lpMemTag) - sizeof(MEM_TAG));
            break;
        default:
            AssertSz(FALSE, "MemGetSize: Unknown mem type\n");
            break;
        }
    }
    return 0;
}
#endif

//+------------------------------------------------------------------------
//
//  Function:   GetBaseMem
//
//  Synopsis:   Get the amount of base memory by reading the CMOS storage.
//
//  Arguments:  None
//
//  Returns:    Number of kilobytes below 1Meg
//
//-------------------------------------------------------------------------
unsigned short GetBaseMem()
{
    unsigned short base;
    _asm
    {
        mov dx, 71h             // port to read from
        mov ax, 15h             // lo-byte value
        out 70h, al             // write to port
        jmp a                   // allow bus to settle down
a:
        in  al, dx              // read from port
        mov byte ptr [base], al // write to lo-byte
        mov ax, 16h             // hi-byte value
        out 70h, al             // writing
        jmp b                   // settling
b:
        in  al, dx              // reading
        mov byte ptr [base+1], al // write to hi-byte
    }
    return base;                // return K's of base memory
}

//+------------------------------------------------------------------------
//
//  Function:   GetExpMem
//
//  Synopsis:   Get amount of expansion memory (above 1Meg boundry) stored
//              in CMOS
//
//  Arguments:  None
//
//  Returns:    Number of kilobytes above 1Meg
//
//-------------------------------------------------------------------------
unsigned short GetExpMem()
{
    unsigned short extend;
    _asm
    {
        mov dx, 71h             // port to read from
        mov ax, 17h             // lo-byte value
        out 70h, al             // write to port
        jmp a                   // allow bus to settle down
a:
        in  al, dx              // read from port
        mov byte ptr [extend], al // write to lo-byte
        mov ax, 18h             // hi-byte value
        out 70h, al             // writing
        jmp b                   // settling
b:
        in al, dx               // reading
        mov byte ptr [extend+1], al // write to hi-byte
   }
    return extend;              // return K's of expansion memory
}

//+------------------------------------------------------------------------
//
//  Function:   DetectPhysicalMem
//
//  Synopsis:   This routine uses two helper funtions to query the CMOS
//              for the amount of physical memory present.
//
//  Arguments:  None
//
//  Returns:    Amount of physical memory (in K) rounded to nearest meg.
//
//-------------------------------------------------------------------------
DWORD WINAPI DetectPhysicalMem()
{
    DWORD wTemp ;

    // Get the amount of memory.
    wTemp = (DWORD)GetBaseMem() + (DWORD)GetExpMem();

    // Now round to the nearest meg.
    wTemp = ((wTemp + 512) / 1024) * 1024;

    return wTemp ;
}


void InitHeap()
{
#if DBG==1
    lGlobalAllocCount = 0;
    lfMallocCount = 0;
    lMemInUse = 0;
#endif
}

void DeinitHeap()
{
}
