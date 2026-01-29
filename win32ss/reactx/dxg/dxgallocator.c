/*
 * PROJECT:     ReactX Graphics Legacy Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     DirectDraw video memory allocator
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <dxg_int.h>

FLATPTR
WINAPI
DdrawMemAlloc(
    _In_ LPVMEMHEAP pvmh,
    _In_ DWORD Width,
    _In_ DWORD Height,
    _Out_opt_ LPDWORD AllocSize,
    _In_opt_ LPSURFACEALIGNMENT Alignment,
    _Out_opt_ LPDWORD ResolvedPitch)
{
    FLATPTR MemPtr;
    DWORD RequiredBytes;
    DWORD CurrentPos;

    if (!pvmh)
    {
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    /* Retrieve memory boundaries from temporary storage */
    FLATPTR MemBase = (FLATPTR)pvmh->freeList;
    FLATPTR MemLimit = (FLATPTR)pvmh->allocList;

    if (!MemBase || !MemLimit || MemLimit <= MemBase)
    {
        DbgPrint("No valid video memory range\n");
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    CurrentPos = pvmh->dwCommitedSize;
    CurrentPos = ALIGN_UP(CurrentPos, sizeof(ULONG));

    if (pvmh->dwFlags & VMEMHEAP_LINEAR)
    {
        /* Handle linear memory allocation */
        RequiredBytes = Width;
    }
    else
    {
        /* Handle rectangular memory allocation */
        Width = pvmh->stride ? pvmh->stride : Width;
        RequiredBytes = Width * Height;
    }
    MemPtr = MemBase + CurrentPos;

    if (MemPtr + RequiredBytes > MemLimit)
    {
        DbgPrint("Out of memory\n");
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    pvmh->dwCommitedSize = CurrentPos + RequiredBytes;

    if (ResolvedPitch)
        *ResolvedPitch = (LONG)Width;
    if (AllocSize)
        *AllocSize = RequiredBytes;

    return MemPtr;
}

/*
 * Allocates memory from a DirectDraw video memory heap
 */
FLATPTR
WINAPI
DxDdHeapDdrawMemAlloc(
    _In_ LPVIDMEM DdrawVidMem,
    _In_ DWORD Width,
    _In_ DWORD Height,
    _In_opt_ LPSURFACEALIGNMENT Alignment,
    _Out_opt_ LPDWORD ResolvedPitch,
    _Out_ PDWORD AllocSize)
{
    FLATPTR Result;
    DWORD ActualSize = 0;

    if (!DdrawVidMem || !DdrawVidMem->lpHeap)
    {
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    if ((DdrawVidMem->dwFlags & VIDMEM_ISNONLOCAL) &&
        !DdrawVidMem->lpHeap->pvPhysRsrv)
    {
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    /* Determine memory region boundaries */
    FLATPTR MemBegin = DdrawVidMem->fpStart;
    FLATPTR MemEnd;

    if (DdrawVidMem->dwFlags & VIDMEM_ISLINEAR)
    {
        MemEnd = DdrawVidMem->fpEnd;
    }
    else if (DdrawVidMem->dwFlags & VIDMEM_ISRECTANGULAR)
    {
        MemEnd = MemBegin + (DdrawVidMem->dwWidth * DdrawVidMem->dwHeight);
    }
    else
    {
        MemEnd = DdrawVidMem->fpEnd;
    }

    if (!MemBegin)
    {
        if (AllocSize)
            *AllocSize = 0;
        return (FLATPTR)NULL;
    }

    /* Fallback to heap size if end address is invalid */
    if (!MemEnd || MemEnd <= MemBegin)
    {
        if (DdrawVidMem->lpHeap && DdrawVidMem->lpHeap->dwTotalSize > 0)
        {
            MemEnd = MemBegin + DdrawVidMem->lpHeap->dwTotalSize;
        }
        else
        {
            if (AllocSize)
                *AllocSize = 0;
            return (FLATPTR)NULL;
        }
    }

    /*
     * In DxgKrnl we are given APIs to allocate ranges for DMA.
     * In Vista this is used for trying to deal with ddraw on a dedicated surface
     * outside of DWMs... control.
     *
     * In legacy DirectX these ranges are passed from the driver as valid areas to do
     * allocation from, depending on this dxg driver to do the tracking.
     * We don't care much to replicate ALL of this yet.
     */
    FLATPTR SavedBegin = (FLATPTR)DdrawVidMem->lpHeap->freeList;
    FLATPTR SavedEnd = (FLATPTR)DdrawVidMem->lpHeap->allocList;
    DdrawVidMem->lpHeap->freeList = (LPVOID)MemBegin;
    DdrawVidMem->lpHeap->allocList = (LPVOID)MemEnd;

    Result = DdrawMemAlloc(DdrawVidMem->lpHeap, Width, Height, &ActualSize,
                           Alignment, ResolvedPitch);

    if (!Result)
    {
        if (AllocSize)
            *AllocSize = 0;
        return Result;
    }

    DdrawVidMem->lpHeap->freeList = (LPVOID)SavedBegin;
    DdrawVidMem->lpHeap->allocList = (LPVOID)SavedEnd;

    if (AllocSize)
        *AllocSize = ActualSize;

    return Result;
}
 
FLATPTR
NTAPI
DxDdHeapVidMemAllocAligned(
    LPVIDMEM DdrawVidMem,
    DWORD Width,
    DWORD Height,
    LPSURFACEALIGNMENT Alignment,
    LPDWORD ResolvedPitch)
{
    DWORD SizeOut = 0;

    if (!DdrawVidMem || !DdrawVidMem->lpHeap ||
        (DdrawVidMem->dwFlags & VIDMEM_HEAPDISABLED))
    {
        return (FLATPTR)NULL;
    }

    if (DdrawVidMem->dwFlags & VIDMEM_ISNONLOCAL)
    {
        if (!DdrawVidMem->lpHeap->pvPhysRsrv)
            return (FLATPTR)NULL;
        DbgPrint("AGP memory not supported\n");
        return (FLATPTR)NULL;
    }

    return DxDdHeapDdrawMemAlloc(DdrawVidMem, Width, Height,
                                 Alignment, ResolvedPitch, &SizeOut);
}
