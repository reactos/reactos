/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Runtime library memory functions test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define KMT_EMULATE_KERNEL
#include <kmt_test.h>

START_TEST(RtlMemory)
{
    UCHAR Buffer[512];
    KIRQL Irql;
    int i;

    KeRaiseIrql(HIGH_LEVEL, &Irql);

    RtlFillMemory(Buffer, sizeof Buffer / 2, 0x55);
    RtlFillMemory(Buffer + sizeof Buffer / 2, sizeof Buffer / 2, 0xAA);
    for (i = 0; i < sizeof Buffer / 2; ++i)
        ok_eq_uint(Buffer[i], 0x55);
    for (i = sizeof Buffer / 2; i < sizeof Buffer; ++i)
        ok_eq_uint(Buffer[i], 0xAA);

    KeLowerIrql(Irql);
}
