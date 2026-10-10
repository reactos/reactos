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

extern KSPIN_LOCK KiReverseStallIpiLock;

/* PRIVATE FUNCTIONS *********************************************************/

#ifndef _M_AMD64

#ifdef CONFIG_SMP
static
VOID
KiIpiPublishPacket(
    _In_ KAFFINITY TargetSet,
    _In_ PKIPI_WORKER WorkerRoutine,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKPRCB TargetPrcb;
    KAFFINITY RemainingSet;
    ULONG_PTR Signal;
    ULONG Processor;

    /* Fill in the packet before any target can see it */
    Prcb->TargetSet = TargetSet;
    Prcb->WorkerRoutine = WorkerRoutine;
    Prcb->CurrentPacket[0] = Parameter1;
    Prcb->CurrentPacket[1] = Parameter2;
    Prcb->CurrentPacket[2] = Parameter3;

    /* Low bit of the signal marks a single target packet */
    if ((TargetSet & (TargetSet - 1)) == 0)
    {
        Signal = (ULONG_PTR)Prcb | 1;
    }
    else
    {
        Prcb->PacketBarrier = TargetSet;
        Signal = (ULONG_PTR)Prcb;
    }

    RemainingSet = TargetSet;
    while (RemainingSet != 0)
    {
        NT_VERIFY(BitScanForwardAffinity(&Processor, RemainingSet) != FALSE);
        ASSERT(Processor < (ULONG)KeNumberProcessors);
        RemainingSet &= ~AFFINITY_MASK(Processor);

        TargetPrcb = KiProcessorBlock[Processor];

        /* Spin on a plain read and only retry the exchange once the slot is free */
        while (InterlockedCompareExchangePointer((PVOID*)&TargetPrcb->SignalDone,
                                                 (PVOID)Signal,
                                                 NULL) != NULL)
        {
            while (TargetPrcb->SignalDone != NULL)
            {
                YieldProcessor();
                KeMemoryBarrier();
            }
        }
    }
}
#endif /* CONFIG_SMP */

VOID
NTAPI
KiIpiGenericCallTarget(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID BroadcastFunction,
    _In_ PVOID Argument,
    _In_ PVOID Count)
{
#ifdef CONFIG_SMP
    PKIPI_BROADCAST_WORKER Worker = (PKIPI_BROADCAST_WORKER)BroadcastFunction;

    /* Check in and wait for every processor so the function runs everywhere at once */
    InterlockedDecrementUL((PULONG)Count);
    while (*(volatile ULONG *)Count != 0)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }

    Worker((ULONG_PTR)Argument);
    KiIpiSignalPacketDone(PacketContext);
#else
    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(BroadcastFunction);
    UNREFERENCED_PARAMETER(Argument);
    UNREFERENCED_PARAMETER(Count);
#endif
}

VOID
FASTCALL
KiIpiSend(
    _In_ KAFFINITY TargetProcessors,
    _In_ ULONG IpiRequest)
{
#ifdef CONFIG_SMP
    KAFFINITY RemainingSet;
    ULONG Processor;

    /* Post the request to every target before sending a single interrupt */
    RemainingSet = TargetProcessors;
    while (RemainingSet != 0)
    {
        NT_VERIFY(BitScanForwardAffinity(&Processor, RemainingSet) != FALSE);
        ASSERT(Processor < (ULONG)KeNumberProcessors);
        RemainingSet &= ~AFFINITY_MASK(Processor);

        InterlockedOr((PLONG)&KiProcessorBlock[Processor]->RequestSummary,
                      (LONG)IpiRequest);
    }

    HalRequestIpi(TargetProcessors);
#else
    UNREFERENCED_PARAMETER(TargetProcessors);
    UNREFERENCED_PARAMETER(IpiRequest);
#endif
}

VOID
NTAPI
KiIpiSendPacket(
    _In_ KAFFINITY TargetProcessors,
    _In_ PKIPI_WORKER WorkerFunction,
    _In_opt_ PKIPI_BROADCAST_WORKER BroadcastFunction,
    _In_ ULONG_PTR Context,
    _In_opt_ PULONG Count)
{
#ifdef CONFIG_SMP
    KiIpiPublishPacket(TargetProcessors,
                       WorkerFunction,
                       (PVOID)BroadcastFunction,
                       (PVOID)Context,
                       (PVOID)Count);

    HalRequestIpi(TargetProcessors);
#else
    UNREFERENCED_PARAMETER(TargetProcessors);
    UNREFERENCED_PARAMETER(WorkerFunction);
    UNREFERENCED_PARAMETER(BroadcastFunction);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Count);
#endif
}

VOID
FASTCALL
KiIpiSignalPacketDone(
    _In_opt_ PKIPI_CONTEXT PacketContext)
{
#ifdef CONFIG_SMP
    ULONG_PTR Signal = (ULONG_PTR)PacketContext;
    PKPRCB SenderPrcb = (PKPRCB)(Signal & ~(ULONG_PTR)1);
    KAFFINITY SetMember = KeGetCurrentPrcb()->SetMember;

    /* Worker was called locally, there is no sender to report to */
    if (SenderPrcb == NULL)
        return;

    if (Signal & 1)
    {
        SenderPrcb->TargetSet = 0;
    }
    else
    {
        /* Whoever clears the last bit releases the barrier */
        if ((KAFFINITY)InterlockedXor((PLONG)&SenderPrcb->TargetSet,
                                      (LONG)SetMember) == SetMember)
            SenderPrcb->PacketBarrier = 0;
    }
#else
    UNREFERENCED_PARAMETER(PacketContext);
#endif
}

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ volatile PULONG ReverseStall)
{
#ifdef CONFIG_SMP
    ULONG Original = *ReverseStall;

    /* Report completion, then hold until the sender advances the stall value */
    KiIpiSignalPacketDone(PacketContext);

    while (Original == *ReverseStall)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }
#else
    UNREFERENCED_PARAMETER(PacketContext);
    UNREFERENCED_PARAMETER(ReverseStall);
#endif
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KiIpiServiceRoutine(
    _In_opt_ PKTRAP_FRAME TrapFrame,
    _In_opt_ PKEXCEPTION_FRAME ExceptionFrame)
{
#ifdef CONFIG_SMP
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKPRCB SenderPrcb;
    PKIPI_WORKER WorkerRoutine;
    ULONG_PTR Signal;
    ULONG Summary;

    /* Grab and clear atomically so a request posted in between is not lost */
    Summary = (ULONG)InterlockedExchange((PLONG)&Prcb->RequestSummary, 0);
    Signal = (ULONG_PTR)InterlockedExchangePointer((PVOID*)&Prcb->SignalDone,
                                                   NULL);

    if (Summary & IPI_FREEZE)
        KiProcessorFreezeHandler(TrapFrame, ExceptionFrame);

    if (Signal != 0)
    {
        SenderPrcb = (PKPRCB)(Signal & ~(ULONG_PTR)1);
        WorkerRoutine = SenderPrcb->WorkerRoutine;
        Prcb->IpiFrame = TrapFrame;

        /* The worker is responsible for signaling the packet done */
        WorkerRoutine((PKIPI_CONTEXT)Signal,
                      SenderPrcb->CurrentPacket[0],
                      SenderPrcb->CurrentPacket[1],
                      SenderPrcb->CurrentPacket[2]);
    }

    if (Summary & IPI_APC)
        HalRequestSoftwareInterrupt(APC_LEVEL);

    if (Summary & IPI_DPC)
    {
        Prcb->DpcInterruptRequested = TRUE;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }
#else
    UNREFERENCED_PARAMETER(TrapFrame);
    UNREFERENCED_PARAMETER(ExceptionFrame);
#endif
    return TRUE;
}

/*
 * @implemented
 */
ULONG_PTR
NTAPI
KeIpiGenericCall(
    _In_ PKIPI_BROADCAST_WORKER Function,
    _In_ ULONG_PTR Argument)
{
    ULONG_PTR Status;
    KIRQL OldIrql, OldIrql2;
#ifdef CONFIG_SMP
    KAFFINITY Affinity;
    ULONG Count;
    PKPRCB Prcb = KeGetCurrentPrcb();
#endif

    /* Raise to DPC level if required */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL) KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

#ifdef CONFIG_SMP
    /* Get current processor count and affinity */
    Count = KeNumberProcessors;
    Affinity = KeActiveProcessors;

    /* Exclude ourselves */
    Affinity &= ~Prcb->SetMember;
#endif

    /* Acquire the IPI lock */
    KeAcquireSpinLockAtDpcLevel(&KiReverseStallIpiLock);

#ifdef CONFIG_SMP
    /* Make sure this is MP */
    if (Affinity)
    {
        /* Send an IPI */
        KiIpiSendPacket(Affinity,
                        KiIpiGenericCallTarget,
                        Function,
                        Argument,
                        &Count);

        /* Spin until the other processors are ready */
        while (Count != 1)
        {
            YieldProcessor();
            KeMemoryBarrierWithoutFence();
        }
    }
#endif

    /* Raise to IPI level */
    KeRaiseIrql(IPI_LEVEL, &OldIrql2);

#ifdef CONFIG_SMP
    /* Let the other processors know it is time */
    Count = 0;
#endif

    /* Call the function */
    Status = Function(Argument);

#ifdef CONFIG_SMP
    /* If this is MP, wait for the other processors to finish */
    if (Affinity)
    {
        /* Sanity check */
        ASSERT(Prcb == KeGetCurrentPrcb());

        /* Wait for every target to retire the packet */
        while (Prcb->TargetSet != 0)
        {
            YieldProcessor();
            KeMemoryBarrier();
        }
    }
#endif

    /* Drop back to DISPATCH_LEVEL before releasing the lock */
    KeLowerIrql(OldIrql2);

    /* Release the lock */
    KeReleaseSpinLockFromDpcLevel(&KiReverseStallIpiLock);

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
    return Status;
}

VOID
NTAPI
KiIpiSendRequest(
    _In_ KAFFINITY TargetSet,
    _In_ PKIPI_WORKER WorkerRoutine,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Parameter3)
{
#ifdef CONFIG_SMP
    PKPRCB Prcb = KeGetCurrentPrcb();
    KAFFINITY SetMember = Prcb->SetMember;
    KAFFINITY RemoteSet;
    KIRQL OldIrql;

    TargetSet &= KeActiveProcessors;
    RemoteSet = TargetSet & ~SetMember;

    if (RemoteSet == 0)
    {
        if (TargetSet & SetMember)
            WorkerRoutine(NULL, Parameter1, Parameter2, Parameter3);
        return;
    }

    /* Only one packet per sender can be outstanding */
    KeRaiseIrql(SYNCH_LEVEL, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&KiReverseStallIpiLock);

    KiIpiPublishPacket(RemoteSet,
                       WorkerRoutine,
                       Parameter1,
                       Parameter2,
                       Parameter3);

    HalRequestIpi(RemoteSet);

    if (TargetSet & SetMember)
        WorkerRoutine(NULL, Parameter1, Parameter2, Parameter3);

    /* Do not return until every target has finished */
    while (Prcb->TargetSet != 0)
    {
        YieldProcessor();
        KeMemoryBarrier();
    }

    KeReleaseSpinLockFromDpcLevel(&KiReverseStallIpiLock);
    KeLowerIrql(OldIrql);
#else
    UNREFERENCED_PARAMETER(TargetSet);
    WorkerRoutine(NULL, Parameter1, Parameter2, Parameter3);
#endif
}

#endif // !_M_AMD64
