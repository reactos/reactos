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