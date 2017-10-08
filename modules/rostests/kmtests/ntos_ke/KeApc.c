/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Asynchronous Procedure Call test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

static
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI
*pKeAreAllApcsDisabled)(VOID);

static
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
(NTAPI
*pKeEnterGuardedRegion)(VOID);

static
_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
(NTAPI
*pKeLeaveGuardedRegion)(VOID);

#define CheckApcs(KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, Irql) do    \
{                                                                                       \
    ok_eq_bool(KeAreApcsDisabled(), KernelApcsDisabled || SpecialApcsDisabled);         \
    ok_eq_int(Thread->KernelApcDisable, KernelApcsDisabled);                            \
    if (pKeAreAllApcsDisabled)                                                          \
        ok_eq_bool(pKeAreAllApcsDisabled(), AllApcsDisabled);                           \
    ok_eq_int(Thread->SpecialApcDisable, SpecialApcsDisabled);                          \
    ok_irql(Irql);                                                                      \
} while (0)

START_TEST(KeApc)
{
    KIRQL Irql;
    PKTHREAD Thread;

    pKeAreAllApcsDisabled = KmtGetSystemRoutineAddress(L"KeAreAllApcsDisabled");
    pKeEnterGuardedRegion = KmtGetSystemRoutineAddress(L"KeEnterGuardedRegion");
    pKeLeaveGuardedRegion = KmtGetSystemRoutineAddress(L"KeLeaveGuardedRegion");

    if (skip(pKeAreAllApcsDisabled != NULL, "KeAreAllApcsDisabled unavailable\n"))
    {
        /* We can live without this function here */
    }

    Thread = KeGetCurrentThread();

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
    if (!skip(pKeEnterGuardedRegion &&
              pKeLeaveGuardedRegion, "Guarded regions not available\n"))
    {
        pKeEnterGuardedRegion();
          CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
          pKeEnterGuardedRegion();
            CheckApcs(0, -2, TRUE, PASSIVE_LEVEL);
            pKeEnterGuardedRegion();
              CheckApcs(0, -3, TRUE, PASSIVE_LEVEL);
            pKeLeaveGuardedRegion();
            CheckApcs(0, -2, TRUE, PASSIVE_LEVEL);
          pKeLeaveGuardedRegion();
          CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        pKeLeaveGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        /* mix them */
        pKeEnterGuardedRegion();
          CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
          KeEnterCriticalRegion();
            CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
          KeLeaveCriticalRegion();
          CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        pKeLeaveGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeEnterCriticalRegion();
          CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
          pKeEnterGuardedRegion();
            CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
          pKeLeaveGuardedRegion();
          CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
    }

    /* leave without entering */
    if (!KmtIsCheckedBuild)
    {
        KeLeaveCriticalRegion();
        CheckApcs(1, 0, FALSE, PASSIVE_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        if (!skip(pKeEnterGuardedRegion &&
                  pKeLeaveGuardedRegion, "Guarded regions not available\n"))
        {
            pKeLeaveGuardedRegion();
            CheckApcs(0, 1, TRUE, PASSIVE_LEVEL);
            pKeEnterGuardedRegion();
            CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

            KeLeaveCriticalRegion();
            CheckApcs(1, 0, FALSE, PASSIVE_LEVEL);
            pKeLeaveGuardedRegion();
            CheckApcs(1, 1, TRUE, PASSIVE_LEVEL);
            KeEnterCriticalRegion();
            CheckApcs(0, 1, TRUE, PASSIVE_LEVEL);
            pKeEnterGuardedRegion();
            CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);
        }
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
      if (!KmtIsCheckedBuild &&
          !skip(pKeEnterGuardedRegion &&
                pKeLeaveGuardedRegion, "Guarded regions not available\n"))
      {
          pKeEnterGuardedRegion();
            CheckApcs(0, -1, TRUE, HIGH_LEVEL);
          pKeLeaveGuardedRegion();
      }
      CheckApcs(0, 0, TRUE, HIGH_LEVEL);
    KeLowerIrql(Irql);
    CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

    if (!KmtIsCheckedBuild &&
        !skip(pKeEnterGuardedRegion &&
              pKeLeaveGuardedRegion, "Guarded regions not available\n"))
    {
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(0, 0, TRUE, HIGH_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        pKeEnterGuardedRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(-1, -1, TRUE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        pKeLeaveGuardedRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        pKeEnterGuardedRegion();
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(0, -1, TRUE, HIGH_LEVEL);
        KeEnterCriticalRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        pKeLeaveGuardedRegion();
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, 0, FALSE, PASSIVE_LEVEL);

        KeEnterCriticalRegion();
        CheckApcs(-1, 0, FALSE, PASSIVE_LEVEL);
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        CheckApcs(-1, 0, TRUE, HIGH_LEVEL);
        pKeEnterGuardedRegion();
        CheckApcs(-1, -1, TRUE, HIGH_LEVEL);
        KeLeaveCriticalRegion();
        CheckApcs(0, -1, TRUE, HIGH_LEVEL);
        KeLowerIrql(Irql);
        CheckApcs(0, -1, TRUE, PASSIVE_LEVEL);
        pKeLeaveGuardedRegion();
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
