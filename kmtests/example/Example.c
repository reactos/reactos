/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example Test kernel-mode part
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>

START_TEST(Example)
{
    KIRQL Irql;

    ok(1, "This test should succeed.\n");
    ok(0, "This test should fail.\n");
    trace("Message from kernel, low-irql. %s. %ls.\n", "Format strings work", L"Even with Unicode");
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    trace("Message from kernel, high-irql. %s. %ls.\n", "Format strings work", L"Even with Unicode");

    ok_irql(DISPATCH_LEVEL);
    ok_eq_int(5, 6);
    ok_eq_uint(6U, 7U);
    ok_eq_long(1L, 2L);
    ok_eq_ulong(3LU, 4LU);
    ok_eq_pointer((PVOID)8, (PVOID)9);
    ok_eq_hex(0x1234LU, 0x5678LU);
    ok_bool_true(FALSE, "foo");
    ok_bool_false(TRUE, "bar");
    ok_eq_print(1, 2, "%i");
    ok_eq_str("Hello", "world");
    ok_eq_wstr(L"ABC", L"DEF");
    KeLowerIrql(Irql);
}
