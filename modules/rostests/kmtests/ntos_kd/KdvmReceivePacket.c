/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite for KdvmReceivePacket integer overflow check
 * COPYRIGHT:   Copyright 2026 OrbisAI Security
 */

#include <kmt_test.h>
#include <kdvm.h>

START_TEST(KdvmReceivePacket)
{
    static const struct
    {
        ULONG HeaderSize;
        ULONG DataSize;
        BOOLEAN ShouldOverflow;
    } TestCases[] =
    {
        { 0xFFFFFFFF, 1,          TRUE  },  /* overflow: HeaderSize > MAXULONG - DataSize */
        { 0x7FFFFFFF, 0x7FFFFFFF, TRUE  },  /* overflow: sum exceeds MAXULONG - sizeof(result) */
        { 1024,       2048,       FALSE },  /* valid: normal sizes */
        { 0,          0,          FALSE },  /* valid: zero sizes */
        { 0x80000000, 0x7FFFFFFF, TRUE  },  /* overflow: sum = MAXULONG, exceeds MAXULONG - sizeof(result) */
    };
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(TestCases); i++)
    {
        ULONG HeaderSize = TestCases[i].HeaderSize;
        ULONG DataSize = TestCases[i].DataSize;
        /* Mirror the exact overflow check from KdReceivePacket in drivers/base/kdvm/kdvm.c */
        BOOLEAN Overflows = (HeaderSize > MAXULONG - DataSize ||
                             HeaderSize + DataSize > MAXULONG - (ULONG)sizeof(KDVM_RECV_PKT_RESULT));
        ok(Overflows == TestCases[i].ShouldOverflow,
           "Test %lu (HeaderSize=0x%lx, DataSize=0x%lx): expected %soverflow, got %soverflow\n",
           i, HeaderSize, DataSize,
           TestCases[i].ShouldOverflow ? "" : "no ",
           Overflows ? "" : "no ");
    }
}
