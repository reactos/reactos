/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel mode tests for Save/Restore FPU state API kernel support
 * COPYRIGHT:   Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#include <kmt_test.h>

START_TEST(KeFloatPointState)
{
    NTSTATUS Status;
    KFLOATING_SAVE FloatSave;
    KIRQL Irql;

    /* Save the state under normal conditions */
    Status = KeSaveFloatingPointState(&FloatSave);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the FPU state back */
    KeRestoreFloatingPointState(&FloatSave);

    /* Try to raise the IRQL to APC and do some operations again */
    KeRaiseIrql(APC_LEVEL, &Irql);

    /* Save the state under APC_LEVEL interrupt */
    Status = KeSaveFloatingPointState(&FloatSave);
    ok_irql(APC_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Restore the FPU state back */
    KeRestoreFloatingPointState(&FloatSave);

    /* Try to raise the IRQL to dispatch this time */
    KeLowerIrql(Irql);
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);

    /* Save the state under DISPATCH_LEVEL interrupt */
    Status = KeSaveFloatingPointState(&FloatSave);
    ok_irql(DISPATCH_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* We're done */
    KeRestoreFloatingPointState(&FloatSave);
    KeLowerIrql(Irql);
}
