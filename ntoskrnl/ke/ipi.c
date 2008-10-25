/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/ipi.c
 * PURPOSE:         Inter-Processor Packet Interface
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

KSPIN_LOCK KiIpiLock;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiIpiSendRequest(IN KAFFINITY TargetSet,
                 IN ULONG IpiRequest)
{
#ifdef CONFIG_SMP
    LONG i;
    PKPRCB Prcb;
    KAFFINITY Current;

    for (i = 0, Current = 1; i < KeNumberProcessors; i++, Current <<= 1)
    {
        if (TargetSet & Current)
        {
            /* Get the PRCB for this CPU */
            Prcb = KiProcessorBlock[i];

            InterlockedBitTestAndSet((PLONG)&Prcb->IpiFrozen, IpiRequest);
            HalRequestIpi(i);
        }
    }
#endif
}

VOID
NTAPI
KiIpiSendPacket(IN KAFFINITY TargetSet,
                IN PKIPI_BROADCAST_WORKER WorkerRoutine,
                IN ULONG_PTR Argument,
                IN ULONG Count,
                IN BOOLEAN Synchronize)
{
#ifdef CONFIG_SMP
    KAFFINITY Processor;
    LONG i;
    PKPRCB Prcb, CurrentPrcb;
    KIRQL oldIrql;

    ASSERT(KeGetCurrentIrql() == SYNCH_LEVEL);

    CurrentPrcb = KeGetCurrentPrcb();
    (void)InterlockedExchangeUL(&CurrentPrcb->TargetSet, TargetSet);
    (void)InterlockedExchangeUL(&CurrentPrcb->WorkerRoutine, (ULONG_PTR)WorkerRoutine);
    (void)InterlockedExchangePointer(&CurrentPrcb->CurrentPacket[0], Argument);
    (void)InterlockedExchangeUL(&CurrentPrcb->CurrentPacket[1], Count);
    (void)InterlockedExchangeUL(&CurrentPrcb->CurrentPacket[2], Synchronize ? 1 : 0);

    for (i = 0, Processor = 1; i < KeNumberProcessors; i++, Processor <<= 1)
    {
        if (TargetSet & Processor)
        {
            Prcb = KiProcessorBlock[i];
            while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone, (LONG)CurrentPrcb, 0));
            InterlockedBitTestAndSet((PLONG)&Prcb->IpiFrozen, IPI_SYNCH_REQUEST);
            if (Processor != CurrentPrcb->SetMember)
            {
                HalRequestIpi(i);
            }
        }
    }
    if (TargetSet & CurrentPrcb->SetMember)
    {
        KeRaiseIrql(IPI_LEVEL, &oldIrql);
        KiIpiServiceRoutine(NULL, NULL);
        KeLowerIrql(oldIrql);
    }
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame,
                    IN PVOID ExceptionFrame)
{
#ifdef CONFIG_SMP
    PKPRCB Prcb;
    ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

    Prcb = KeGetCurrentPrcb();

    if (InterlockedBitTestAndReset((PLONG)&Prcb->IpiFrozen, IPI_APC))
    {
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }

    if (InterlockedBitTestAndReset((PLONG)&Prcb->IpiFrozen, IPI_DPC))
    {
        Prcb->DpcInterruptRequested = TRUE;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }

    if (InterlockedBitTestAndReset((PLONG)&Prcb->IpiFrozen, IPI_SYNCH_REQUEST))
    {
        (void)InterlockedDecrementUL(&Prcb->SignalDone->CurrentPacket[1]);
        if (InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[2], 0, 0))
        {
            while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[1], 0, 0));
        }
        ((VOID (NTAPI*)(PVOID))(Prcb->SignalDone->WorkerRoutine))(Prcb->SignalDone->CurrentPacket[0]);
        InterlockedBitTestAndReset((PLONG)&Prcb->SignalDone->TargetSet, KeGetCurrentProcessorNumber());
        if (InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[2], 0, 0))
        {
            while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone->TargetSet, 0, 0));
        }
        (void)InterlockedExchangePointer(&Prcb->SignalDone, NULL);
    }
#endif
   return TRUE;
}

/*
 * @implemented
 */
ULONG_PTR
NTAPI
KeIpiGenericCall(IN PKIPI_BROADCAST_WORKER Function,
                 IN ULONG_PTR Argument)
{
#ifdef CONFIG_SMP
#error Not yet implemented!
#else
    ULONG_PTR Status;
    KIRQL OldIrql, OldIrql2;

    /* Raise to DPC level if required */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL) KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Acquire the IPI lock */
    KefAcquireSpinLockAtDpcLevel(&KiIpiLock);

    /* Raise to IPI level */
    KeRaiseIrql(IPI_LEVEL, &OldIrql2);

    /* Call the function */
    Status = Function(Argument);

    /* Release the lock */
    KefReleaseSpinLockFromDpcLevel(&KiIpiLock);

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
    return Status;
#endif
}

/* EOF */
