/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Asynchronous Procedure Call test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <ntddk.h>
#include <kmt_test.h>

#define CheckApcs(KernelApcsDisabled, SpecialApcsDisabled, Irql) do \
{                                                                   \
    ok_eq_bool(KeAreApcsDisabled(), KernelApcsDisabled);            \
    ok_eq_bool(KeAreAllApcsDisabled(), SpecialApcsDisabled);        \
    ok_irql(Irql);                                                  \
} while (0)

START_TEST(KeApc)
{
    KIRQL Irql;

    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* critical region */
    KeEnterCriticalRegion();
      CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
        KeEnterCriticalRegion();
          CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* guarded region */
    KeEnterGuardedRegion();
      CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
      KeEnterGuardedRegion();
        CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
        KeEnterGuardedRegion();
          CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
      KeLeaveGuardedRegion();
      CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* mix them */
    KeEnterGuardedRegion();
      CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    KeEnterCriticalRegion();
      CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
      KeEnterGuardedRegion();
        CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
      KeLeaveGuardedRegion();
      CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* raised irql - APC_LEVEL should disable APCs */
    KeRaiseIrql(APC_LEVEL, &Irql);
      CheckApcs(FALSE, TRUE, APC_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* KeAre*ApcsDisabled are documented to work up to DISPATCH_LEVEL... */
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
      CheckApcs(FALSE, TRUE, DISPATCH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* ... but also work on higher levels! */
    KeRaiseIrql(HIGH_LEVEL, &Irql);
      CheckApcs(FALSE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    /* now comes the crazy stuff */
    KeRaiseIrql(HIGH_LEVEL, &Irql);
      CheckApcs(FALSE, TRUE, HIGH_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(TRUE, TRUE, HIGH_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(FALSE, TRUE, HIGH_LEVEL);

      KeEnterGuardedRegion();
        CheckApcs(TRUE, TRUE, HIGH_LEVEL);
      KeLeaveGuardedRegion();
      CheckApcs(FALSE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    KeRaiseIrql(HIGH_LEVEL, &Irql);
    CheckApcs(FALSE, TRUE, HIGH_LEVEL);
    KeEnterCriticalRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeEnterGuardedRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    KeEnterGuardedRegion();
    CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeEnterCriticalRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    KeEnterCriticalRegion();
    CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeEnterGuardedRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(TRUE, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);

    KeEnterCriticalRegion();
    CheckApcs(TRUE, FALSE, PASSIVE_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    CheckApcs(TRUE, TRUE, HIGH_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(FALSE, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(FALSE, FALSE, PASSIVE_LEVEL);
}
