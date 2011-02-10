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

/* EOF */
