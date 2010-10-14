/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/rtl/heapdbg.c
 * PURPOSE:         Heap manager debug heap
 * PROGRAMMERS:     Copyright 2010 Aleksey Bragin
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

BOOLEAN RtlpPageHeapEnabled = FALSE;
ULONG RtlpPageHeapGlobalFlags;
ULONG RtlpPageHeapSizeRangeStart, RtlpPageHeapSizeRangeEnd;
ULONG RtlpPageHeapDllRangeStart, RtlpPageHeapDllRangeEnd;
WCHAR RtlpPageHeapTargetDlls[512];

/* FUNCTIONS ******************************************************************/

HANDLE NTAPI
RtlDebugCreateHeap(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    return NULL;
}

HANDLE NTAPI
RtlDebugDestroyHeap(HANDLE HeapPtr)
{
    return NULL;
}

PVOID NTAPI
RtlDebugAllocateHeap(PVOID HeapPtr,
                     ULONG Flags,
                     SIZE_T Size)
{
    return NULL;
}

PVOID NTAPI
RtlDebugReAllocateHeap(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlDebugFreeHeap(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return FALSE;
}

BOOLEAN NTAPI
RtlDebugGetUserInfoHeap(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    return FALSE;
}

BOOLEAN NTAPI
RtlDebugSetUserValueHeap(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    return FALSE;
}

BOOLEAN
NTAPI
RtlDebugSetUserFlagsHeap(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    return FALSE;
}

SIZE_T NTAPI
RtlDebugSizeHeap(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return 0;
}


// Page heap -> move to another file

HANDLE NTAPI
RtlpPageHeapCreate(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    return NULL;
}

/* EOF */