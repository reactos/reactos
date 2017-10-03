/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Interrupt test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define CheckSpinLock(Lock, Locked) do                  \
{                                                       \
    if (KmtIsMultiProcessorBuild)                       \
        ok_eq_ulongptr(*(Lock), (Locked) != 0);         \
    else                                                \
        ok_eq_ulongptr(*(Lock), 0);                     \
} while (0)

typedef struct
{
    BOOLEAN ReturnValue;
    KIRQL ExpectedIrql;
    PKINTERRUPT Interrupt;
} TEST_CONTEXT, *PTEST_CONTEXT;

static KSYNCHRONIZE_ROUTINE SynchronizeRoutine;

static
BOOLEAN
NTAPI
SynchronizeRoutine(
    IN PVOID Context)
{
    PTEST_CONTEXT TestContext = Context;

    ok_irql(TestContext->ExpectedIrql);

    CheckSpinLock(TestContext->Interrupt->ActualLock, TRUE);

    return TestContext->ReturnValue;
}

static
VOID
TestSynchronizeExecution(VOID)
{
    KINTERRUPT Interrupt;
    TEST_CONTEXT TestContext;
    KIRQL SynchIrql;
    KIRQL OriginalIrql;
    KIRQL Irql;
    KSPIN_LOCK ActualLock;
    BOOLEAN Ret;

    RtlFillMemory(&Interrupt, sizeof Interrupt, 0x55);
    Interrupt.ActualLock = &ActualLock;
    KeInitializeSpinLock(Interrupt.ActualLock);
    CheckSpinLock(Interrupt.ActualLock, FALSE);

    TestContext.Interrupt = &Interrupt;
    TestContext.ReturnValue = TRUE;

    for (TestContext.ReturnValue = 0; TestContext.ReturnValue <= 2; ++TestContext.ReturnValue)
    {
        for (OriginalIrql = PASSIVE_LEVEL; OriginalIrql <= HIGH_LEVEL; ++OriginalIrql)
        {
            /* TODO: don't hardcode this :| */
            if (OriginalIrql == 3 || (OriginalIrql >= 11 && OriginalIrql <= 26) || OriginalIrql == 30)
                continue;
            KeRaiseIrql(OriginalIrql, &Irql);
            for (SynchIrql = max(DISPATCH_LEVEL, OriginalIrql); SynchIrql <= HIGH_LEVEL; ++SynchIrql)
            {
                if (SynchIrql == 3 || (SynchIrql >= 11 && SynchIrql <= 26) || SynchIrql == 30)
                    continue;
                Interrupt.SynchronizeIrql = SynchIrql;
                ok_irql(OriginalIrql);
                CheckSpinLock(Interrupt.ActualLock, FALSE);
                TestContext.ExpectedIrql = SynchIrql;
                Ret = KeSynchronizeExecution(&Interrupt, SynchronizeRoutine, &TestContext);
                ok_eq_int(Ret, TestContext.ReturnValue);
                ok_irql(OriginalIrql);
                CheckSpinLock(Interrupt.ActualLock, FALSE);
                /* TODO: Check that all other fields of the interrupt are untouched */
            }
            KeLowerIrql(Irql);
        }
    }
}

START_TEST(IoInterrupt)
{
    TestSynchronizeExecution();
}
