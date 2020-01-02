/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlAllocateHeap
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include "precomp.h"

PVOID Buffers[0x100];

START_TEST(RtlAllocateHeap)
{
    USHORT i;
    HANDLE hHeap;
    BOOLEAN Aligned = TRUE;
    RTL_HEAP_PARAMETERS Parameters = {0};

    for (i = 0; i < 0x100; ++i)
    {
        Buffers[i] = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_CREATE_ALIGN_16, (i % 16 ) + 1);
        ASSERT(Buffers[i] != NULL);
        if (!((ULONG_PTR)Buffers[i] & 0xF))
        {
            Aligned = FALSE;
        }
    }

    for (i = 0; i < 0x100; ++i)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Buffers[i]);
    }

    ok(Aligned == FALSE, "No unaligned address returned\n");

    Aligned = TRUE;
    Parameters.Length = sizeof(Parameters);
    hHeap = RtlCreateHeap(HEAP_CREATE_ALIGN_16, NULL, 0, 0, NULL, &Parameters);
    if (hHeap == NULL)
    {
        return;
    }

    for (i = 0; i < 0x100; ++i)
    {
        Buffers[i] = RtlAllocateHeap(hHeap, 0, (i % 16 ) + 1);
        ASSERT(Buffers[i] != NULL);
        if (!((ULONG_PTR)Buffers[i] & 0xF))
        {
            Aligned = FALSE;
        }
    }

    for (i = 0; i < 0x100; ++i)
    {
        RtlFreeHeap(hHeap, 0, Buffers[i]);
    }

    RtlDestroyHeap(hHeap);

    ok(Aligned == TRUE, "Unaligned address returned\n");

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
