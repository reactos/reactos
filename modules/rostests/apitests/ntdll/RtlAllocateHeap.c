/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlAllocateHeap
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

#include <pseh/pseh2.h>

START_TEST(RtlAllocateHeap)
{
    PVOID Buffers[0x100];
    USHORT i;
    HANDLE hHeap;
    BOOLEAN Not8BytesAligned = FALSE;
    BOOLEAN Not16BytesAligned = FALSE;
    RTL_HEAP_PARAMETERS Parameters = {0};

    for (i = 0; i < ARRAYSIZE(Buffers); ++i)
    {
        /* RtlAllocateHeap() totally ignores HEAP_CREATE_ALIGN_16 (it's only for RtlCreateHeap() */
        Buffers[i] = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_CREATE_ALIGN_16, (i % 16 ) + 1);
        ASSERT(Buffers[i] != NULL);

        if (Buffers[i] != ALIGN_DOWN_POINTER_BY(Buffers[i], 8))
            Not8BytesAligned = TRUE;

        if (Buffers[i] != ALIGN_DOWN_POINTER_BY(Buffers[i], 16))
            Not16BytesAligned = TRUE;
    }

    for (i = 0; i < ARRAYSIZE(Buffers); ++i)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Buffers[i]);
    }

    ok(Not8BytesAligned == FALSE, "Found address that was not 8 byte aligned\n");
#ifdef _WIN64
    ok(Not16BytesAligned == FALSE, "Found address that was not 16 byte aligned\n");
#endif

    Not16BytesAligned = FALSE;
    Parameters.Length = sizeof(Parameters);
    hHeap = RtlCreateHeap(HEAP_CREATE_ALIGN_16, NULL, 0, 0, NULL, &Parameters);
    ok(hHeap != NULL, "Failed to create heap!\n");
    if (hHeap == NULL)
    {
        skip("Failed to create heap!\n");
        return;
    }

#ifdef _M_IX86
    PULONG pheap = (PULONG)hHeap;
    trace("Heap: %p, Heap->AlignRound: %lx\n", hHeap, pheap[12]);
    trace("Heap: %p, Heap->AlignMask: %lx\n", hHeap, pheap[13]);
#endif

    for (i = 0; i < ARRAYSIZE(Buffers); ++i)
    {
        Buffers[i] = RtlAllocateHeap(hHeap, 0, (i % 16 ) + 1);
        ASSERT(Buffers[i] != NULL);

        if (Buffers[i] != ALIGN_DOWN_POINTER_BY(Buffers[i], 16))
        {
            ok(FALSE, "Buffer %d: %p is unaligned!\n", i, Buffers[i]);
            Not16BytesAligned = TRUE;
        }
    }

    for (i = 0; i < ARRAYSIZE(Buffers); ++i)
    {
        RtlFreeHeap(hHeap, 0, Buffers[i]);
    }

    RtlDestroyHeap(hHeap);

    ok(Not16BytesAligned == FALSE, "Found address that was not 16 byte aligned\n");

    _SEH2_TRY
    {
        hHeap = RtlCreateHeap(HEAP_CREATE_ALIGN_16, NULL, 0, 0, NULL, (PRTL_HEAP_PARAMETERS)(ULONG_PTR)0xdeadbeefdeadbeefULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        hHeap = INVALID_HANDLE_VALUE;
    }
    _SEH2_END;

    ok(hHeap == NULL, "Unexpected heap value: %p\n", hHeap);
}
