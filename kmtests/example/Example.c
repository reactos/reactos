/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example Test kernel-mode part
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>

VOID Test_Example(VOID)
{
    KIRQL Irql;

    ok(1, "This test should succeed.\n");
    ok(0, "This test should fail.\n");
    trace("Message from kernel, low-irql. %s. %ls.\n", "Format strings work", L"Even with Unicode");
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    trace("Message from kernel, high-irql. %s. %ls.\n", "Format strings work", L"Even with Unicode");
    KeLowerIrql(Irql);
}
