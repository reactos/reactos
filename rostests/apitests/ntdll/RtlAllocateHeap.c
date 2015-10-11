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
    BOOLEAN Aligned = TRUE;

    for (i = 0; i < 0x100; ++i)
    {
        SetLastError(0xdeadbeef);
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
}
