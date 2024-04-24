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
    else if (KmtIsCheckedBuild)                         \
        ok_eq_bool(*(Lock) != 0, (Locked) != 0);        \
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

static
VOID
TestConnectInterrupt(VOID)
{
    PKINTERRUPT InterruptObject;
    NTSTATUS Status;

    /* If the IoConnectInterrupt() fails, the interrupt object should be set to NULL */
    InterruptObject = KmtInvalidPointer;

    /* Test for invalid interrupt */
    Status = IoConnectInterrupt(&InterruptObject,
                                (PKSERVICE_ROUTINE)TestConnectInterrupt,
                                NULL,
                                NULL,
                                0,
                                0,
                                0,
                                LevelSensitive,
                                TRUE,
                                (KAFFINITY)-1,
                                FALSE);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_pointer(InterruptObject, NULL);
}

START_TEST(IoInterrupt)
{
    TestSynchronizeExecution();
    TestConnectInterrupt();
}
