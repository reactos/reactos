/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for RtlAllocateHeap
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <ndk/rtlfuncs.h>

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
        if (!((ULONG_PTR)Buffers[i] & 0x2))
        {
            Aligned = FALSE;
        }
    }

    for (i = 0; i < 0x100; ++i)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, Buffers[i]);
    }

    ok(Aligned  == FALSE, "No unaligned address returned\n");

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
        if (!((ULONG_PTR)Buffers[i] & 0x2))
        {
            Aligned = FALSE;
        }
    }

    for (i = 0; i < 0x100; ++i)
    {
        RtlFreeHeap(hHeap, 0, Buffers[i]);
    }

    RtlDestroyHeap(hHeap);

    ok(Aligned  == FALSE, "No unaligned address returned\n");
}
