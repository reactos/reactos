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
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

KSPIN_LOCK KiIpiLock;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
KiIpiSendRequest(IN KAFFINITY TargetSet,
                 IN ULONG IpiRequest)
{
#ifdef CONFIG_SMP
#error VerifyMe!
    LONG i;
    PKPCR Pcr;
    KAFFINITY Current;

    for (i = 0, Current = 1; i < KeNumberProcessors; i++, Current <<= 1)
    {
        if (TargetSet & Current)
        {
            Pcr = (PKPCR)(KPCR_BASE + i * PAGE_SIZE);
            Ke386TestAndSetBit(IpiRequest, (PULONG)&Pcr->Prcb->IpiFrozen);
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
#error VerifyMe!
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
            Prcb = ((PKPCR)(KPCR_BASE + i * PAGE_SIZE))->Prcb;
            while(0 != InterlockedCompareExchangeUL(&Prcb->SignalDone, (LONG)CurrentPrcb, 0));
            Ke386TestAndSetBit(IPI_SYNCH_REQUEST, (PULONG)&Prcb->IpiFrozen);
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
#error VerifyMe!
    PKPRCB Prcb;
    ASSERT(KeGetCurrentIrql() == IPI_LEVEL);

    Prcb = KeGetCurrentPrcb();

    if (Ke386TestAndClearBit(IPI_APC, (PULONG)&Prcb->IpiFrozen))
    {
        HalRequestSoftwareInterrupt(APC_LEVEL);
    }

    if (Ke386TestAndClearBit(IPI_DPC, (PULONG)&Prcb->IpiFrozen))
    {
        Prcb->DpcInterruptRequested = TRUE;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }

    if (Ke386TestAndClearBit(IPI_SYNCH_REQUEST, (PULONG)&Prcb->IpiFrozen))
    {
        (void)InterlockedDecrementUL(&Prcb->SignalDone->CurrentPacket[1]);
        if (InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[2], 0, 0))
        {
            while (0 != InterlockedCompareExchangeUL(&Prcb->SignalDone->CurrentPacket[1], 0, 0));
        }
        ((VOID (STDCALL*)(PVOID))(Prcb->SignalDone->WorkerRoutine))(Prcb->SignalDone->CurrentPacket[0]);
        Ke386TestAndClearBit(KeGetCurrentProcessorNumber(), (PULONG)&Prcb->SignalDone->TargetSet);
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
    KIRQL OldIrql;

    /* Raise to DPC level if required */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL) OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Acquire the IPI lock */
    KefAcquireSpinLockAtDpcLevel(&KiIpiLock);

    /* Raise to IPI level */
    KfRaiseIrql(IPI_LEVEL);

    /* Call the function */
    Status = Function(Argument);

    /* Release the lock */
    KefReleaseSpinLockFromDpcLevel(&KiIpiLock);

    /* Lower IRQL back */
    KfLowerIrql(OldIrql);
    return Status;
#endif
}

/* EOF */
