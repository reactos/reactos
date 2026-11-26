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

typedef struct APC_DELIVERY_TEST
{
    KAPC Apc;
    KEVENT CompletionEvent;
    volatile ULONG KernelCalls;
    volatile ULONG NormalCalls;
    BOOLEAN Normal;
} APC_DELIVERY_TEST, *PAPC_DELIVERY_TEST;

typedef enum APC_TEST_REGION
{
    ApcNoRegion,
    ApcCriticalRegion,
    ApcGuardedRegion
} APC_TEST_REGION;

static
VOID
NTAPI
KernelApcRoutine(
    _In_ PKAPC Apc,
    _Inout_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ PVOID *NormalContext,
    _Inout_ PVOID *SystemArgument1,
    _Inout_ PVOID *SystemArgument2)
{
    PAPC_DELIVERY_TEST Test = CONTAINING_RECORD(Apc, APC_DELIVERY_TEST, Apc);

    ok_irql(APC_LEVEL);
    Test->KernelCalls += 1;
    if (!Test->Normal)
    {
        KeSetEvent(&Test->CompletionEvent, IO_NO_INCREMENT, FALSE);
    }
}

static
VOID
NTAPI
NormalApcRoutine(
    _In_ PVOID NormalContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    PAPC_DELIVERY_TEST Test = NormalContext;

    ok_irql(PASSIVE_LEVEL);
    Test->NormalCalls += 1;
    KeSetEvent(&Test->CompletionEvent, IO_NO_INCREMENT, FALSE);
}

static
VOID
TestApcDelivery(
    _In_ BOOLEAN Normal,
    _In_ KIRQL Irql,
    _In_ APC_TEST_REGION Region)
{
    PAPC_DELIVERY_TEST Test;
    KIRQL OldIrql;
    BOOLEAN Inserted, Deferred;
    ULONG KernelOnReturn, NormalOnReturn;
    ULONG KernelAfterInner = 0, NormalAfterInner = 0;
    ULONG KernelFinal, NormalFinal;
    ULONGLONG Start;

    Test = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Test), 'pAmK');
    if (!Test)
    {
        skip(FALSE, "Could not allocate APC test storage\n");
        return;
    }
    RtlZeroMemory(Test, sizeof(*Test));
    Test->Normal = Normal;
    KeInitializeEvent(&Test->CompletionEvent, NotificationEvent, FALSE);
    KeInitializeApc(&Test->Apc,
                    KeGetCurrentThread(),
                    OriginalApcEnvironment,
                    KernelApcRoutine,
                    NULL,
                    Normal ? NormalApcRoutine : NULL,
                    KernelMode,
                    Test);

    /*
     * Enter the region twice to check that APCs disabled by the
     * region are pending until the outermost region is left
     */
    if (Region == ApcCriticalRegion)
    {
        KeEnterCriticalRegion();
        KeEnterCriticalRegion();
    }
    else if (Region == ApcGuardedRegion)
    {
        pKeEnterGuardedRegion();
        pKeEnterGuardedRegion();
    }

    KeRaiseIrql(Irql, &OldIrql);
    Inserted = KeInsertQueueApc(&Test->Apc, NULL, NULL, 0);
    KernelOnReturn = Test->KernelCalls;
    NormalOnReturn = Test->NormalCalls;
    KeLowerIrql(OldIrql);

    /* Check APC delivery inside the outer region */
    if (Region == ApcCriticalRegion)
    {
        KeLeaveCriticalRegion();
        KernelAfterInner = Test->KernelCalls;
        NormalAfterInner = Test->NormalCalls;
        KeLeaveCriticalRegion();
    }
    else if (Region == ApcGuardedRegion)
    {
        pKeLeaveGuardedRegion();
        KernelAfterInner = Test->KernelCalls;
        NormalAfterInner = Test->NormalCalls;
        pKeLeaveGuardedRegion();
    }

    /*
     * Wait for the APC interrupt without entering a dispatcher wait,
     * which could deliver the pending APC itself
     */
    if (Irql != PASSIVE_LEVEL && Region == ApcNoRegion)
    {
        Start = KeQueryInterruptTime();
        while (Inserted &&
               (Test->KernelCalls == 0 || (Normal && Test->NormalCalls == 0)) &&
               KeQueryInterruptTime() - Start < 100 * 10000)
        {
            YieldProcessor();
        }
    }
    KernelFinal = Test->KernelCalls;
    NormalFinal = Test->NormalCalls;

    Deferred = Irql != PASSIVE_LEVEL ||
               Region == ApcGuardedRegion ||
               (Normal && Region == ApcCriticalRegion);
    ok_bool_true(Inserted, "KeInsertQueueApc");
    ok_eq_ulong(KernelOnReturn, Deferred ? 0 : 1);
    ok_eq_ulong(NormalOnReturn, Normal && !Deferred ? 1 : 0);
    if (Region != ApcNoRegion)
    {
        ok_eq_ulong(KernelAfterInner, Region == ApcGuardedRegion || Normal ? 0 : 1);
        ok_eq_ulong(NormalAfterInner, 0);
    }
    ok_eq_ulong(KernelFinal, 1);
    ok_eq_ulong(NormalFinal, Normal ? 1 : 0);

    if (Inserted)
    {
        /* Force a check for pending kernel APCs before cleanup */
        pKeEnterGuardedRegion();
        pKeLeaveGuardedRegion();

        /* Wait for APC completion before freeing the test context */
        KeWaitForSingleObject(&Test->CompletionEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    ExFreePoolWithTag(Test, 'pAmK');
}

static
VOID
TestKernelApcDelivery(VOID)
{
    if (skip(pKeEnterGuardedRegion && pKeLeaveGuardedRegion,
             "Guarded regions unavailable\n"))
    {
        return;
    }

    /* Special kernel APC delivery */
    TestApcDelivery(FALSE, PASSIVE_LEVEL, ApcNoRegion);
    TestApcDelivery(FALSE, APC_LEVEL, ApcNoRegion);
    TestApcDelivery(FALSE, DISPATCH_LEVEL, ApcNoRegion);
    TestApcDelivery(FALSE, PASSIVE_LEVEL, ApcCriticalRegion);
    TestApcDelivery(FALSE, PASSIVE_LEVEL, ApcGuardedRegion);

    /* Normal kernel APC delivery */
    TestApcDelivery(TRUE, PASSIVE_LEVEL, ApcNoRegion);
    TestApcDelivery(TRUE, APC_LEVEL, ApcNoRegion);
    TestApcDelivery(TRUE, DISPATCH_LEVEL, ApcNoRegion);
    TestApcDelivery(TRUE, PASSIVE_LEVEL, ApcCriticalRegion);
    TestApcDelivery(TRUE, PASSIVE_LEVEL, ApcGuardedRegion);
}

START_TEST(KeApc)
{
    KIRQL Irql;
    PKTHREAD Thread;

    pKeAreAllApcsDisabled = KmtGetSystemRoutineAddress(L"KeAreAllApcsDisabled");
    pKeEnterGuardedRegion = KmtGetSystemRoutineAddress(L"KeEnterGuardedRegion");
    pKeLeaveGuardedRegion = KmtGetSystemRoutineAddress(L"KeLeaveGuardedRegion");

    TestKernelApcDelivery();

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
