/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite Example kernel-mode test part
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

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
    ok_eq_bool(TRUE, TRUE);
    ok_eq_bool(TRUE, FALSE);
    ok_eq_bool(FALSE, TRUE);
    ok_bool_true(FALSE, "foo");
    ok_bool_false(TRUE, "bar");
    ok_eq_print(1, 2, "%i");
    ok_eq_str("Hello", "world");
    ok_eq_wstr(L"ABC", L"DEF");

    if (!skip(KeGetCurrentIrql() == HIGH_LEVEL, "This should only work on HIGH_LEVEL\n"))
    {
        /* do tests depending on HIGH_LEVEL here */
        ok(1, "This is fine\n");
    }

    KeLowerIrql(Irql);
}
