/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Asynchronous Procedure Call test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

#define CheckApcs(KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, Irql) do    \
{                                                                                       \
    ok_eq_bool(KeAreApcsDisabled(), KernelApcsDisabled || SpecialApcsDisabled);         \
    ok_eq_int(Thread->KernelApcDisable, KernelApcsDisabled);                            \
    ok_eq_bool(KeAreAllApcsDisabled(), AllApcsDisabled);                                \
    ok_eq_int(Thread->SpecialApcDisable, SpecialApcsDisabled);                          \
    ok_irql(Irql);                                                                      \
} while (0)

START_TEST(KeApc)
{
    KIRQL Irql;
    PKTHREAD Thread = KeGetCurrentThread();

    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* critical region */
    KeEnterCriticalRegion();
      CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(-2, 0, FALSE, PASSIVE_LEVEL);
        KeEnterCriticalRegion();
          CheckApcs(-3, 0, FALSE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(-2, 0, FALSE, PASSIVE_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* guarded region */
    KeEnterGuardedRegion();
      CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
      KeEnterGuardedRegion();
        CheckApcs(0, -2, TRUE, PASSIVE_LEVEL);
        KeEnterGuardedRegion();
          CheckApcs(0, -3, TRUE, PASSIVE_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(0, -2, TRUE, PASSIVE_LEVEL);
      KeLeaveGuardedRegion();
      CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* mix them */
    KeEnterGuardedRegion();
      CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
    KeLeaveGuardedRegion();
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    KeEnterCriticalRegion();
      CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
      KeEnterGuardedRegion();
        CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
      KeLeaveGuardedRegion();
      CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* leave without entering */
    if (!KmtIsCheckedBuild)
    {
        KeLeaveCriticalRegion();
        CheckApcs(1, 0, FALSE, PASSIVE_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeLeaveGuardedRegion();
        CheckApcs(0, 1, TRUE, PASSIVE_LEVEL);
        KeEnterGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeLeaveCriticalRegion();
        CheckApcs(1, 0, FALSE, PASSIVE_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(1, 1, TRUE, PASSIVE_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(0, 1, TRUE, PASSIVE_LEVEL);
        KeEnterGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    }

    /* manually disable APCs */
    Thread->KernelApcDisable = -1;
    CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
    Thread->SpecialApcDisable = -1;
    CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
    Thread->KernelApcDisable = 0;
    CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
    Thread->SpecialApcDisable = 0;
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* raised irql - APC_LEVEL should disable APCs */
    KeRaiseIrql(APC_LEVEL, &Irql);
      CheckApcs(0, 0, TRUE, APC_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* KeAre*ApcsDisabled are documented to work up to DISPATCH_LEVEL... */
    KeRaiseIrql(DISPATCH_LEVEL, &Irql);
      CheckApcs(0, 0, TRUE, DISPATCH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* ... but also work on higher levels! */
    KeRaiseIrql(HIGH_LEVEL, &Irql);
      CheckApcs(0, 0, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    /* now comes the crazy stuff */
    KeRaiseIrql(HIGH_LEVEL, &Irql);
      CheckApcs(0, 0, TRUE, HIGH_LEVEL);
      KeEnterCriticalRegion();
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
      KeLeaveCriticalRegion();
      CheckApcs(0, 0, TRUE, HIGH_LEVEL);

      /* Ke*GuardedRegion assert at > APC_LEVEL */
      if (!KmtIsCheckedBuild)
      {
          KeEnterGuardedRegion();
            CheckApcs(0, -1, TRUE, HIGH_LEVEL);
          KeLeaveGuardedRegion();
      }
      CheckApcs(0, 0, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    if (!KmtIsCheckedBuild)
    {
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(0, 0, TRUE, HIGH_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        KeEnterGuardedRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeEnterGuardedRegion();
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(0, -1, TRUE, HIGH_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeEnterCriticalRegion();
        CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        KeEnterGuardedRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, -1, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        KeLeaveGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    }

    KeEnterCriticalRegion();
    CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
    KeRaiseIrql(HIGH_LEVEL, &Irql);
    CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
    KeLeaveCriticalRegion();
    CheckApcs(0, 0, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
}
