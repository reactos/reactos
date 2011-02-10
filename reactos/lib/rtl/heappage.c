/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heappage.c
 * PURPOSE:         RTL Page Heap implementation
 * PROGRAMMERS:     Copyright 2011 Aleksey Bragin
 */

/* Useful references:
    http://msdn.microsoft.com/en-us/library/ms220938(VS.80).aspx
*/

/* INCLUDES *****************************************************************/

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
RtlpPageHeapCreate(ULONG Flags,
                   PVOID Addr,
                   SIZE_T TotalSize,
                   SIZE_T CommitSize,
                   PVOID Lock,
                   PRTL_HEAP_PARAMETERS Parameters)
{
    return NULL;
}

PVOID NTAPI
RtlpPageHeapDestroy(HANDLE HeapPtr)
{
    return FALSE;
}

PVOID NTAPI
RtlpPageHeapAllocate(IN PVOID HeapPtr,
                     IN ULONG Flags,
                     IN SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapFree(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return FALSE;
}

PVOID NTAPI
RtlpPageHeapReAllocate(HANDLE HeapPtr,
                       ULONG Flags,
                       PVOID Ptr,
                       SIZE_T Size)
{
    return NULL;
}

BOOLEAN NTAPI
RtlpPageHeapGetUserInfo(PVOID HeapHandle,
                        ULONG Flags,
                        PVOID BaseAddress,
                        PVOID *UserValue,
                        PULONG UserFlags)
{
    return FALSE;
}

BOOLEAN NTAPI
RtlpPageHeapSetUserValue(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         PVOID UserValue)
{
    return FALSE;
}

BOOLEAN
NTAPI
RtlpPageHeapSetUserFlags(PVOID HeapHandle,
                         ULONG Flags,
                         PVOID BaseAddress,
                         ULONG UserFlagsReset,
                         ULONG UserFlagsSet)
{
    return FALSE;
}

SIZE_T NTAPI
RtlpPageHeapSize(HANDLE HeapPtr,
                 ULONG Flags,
                 PVOID Ptr)
{
    return 0;
}

/* EOF */
