/*
 * PROJECT:         ReactOS Runtime Library
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            lib/rtl/heapdbg.c
 * PURPOSE:         Heap manager debug heap
 * PROGRAMMERS:     Copyright 2010 Aleksey Bragin
 */

/* INCLUDES ******************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

BOOLEAN RtlpPageHeapEnabled = FALSE;
ULONG RtlpPageHeapGlobalFlags;
ULONG RtlpPageHeapSizeRangeStart, RtlpPageHeapSizeRangeEnd;
ULONG RtlpPageHeapDllRangeStart, RtlpPageHeapDllRangeEnd;
WCHAR RtlpPageHeapTargetDlls[512];

/* FUNCTIONS ******************************************************************/

/* EOF */