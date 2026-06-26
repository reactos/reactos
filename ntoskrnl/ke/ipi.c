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

VOID
NTAPI
KiIpiGenericCallTarget(IN PKIPI_CONTEXT PacketContext,
                       IN PVOID BroadcastFunction,
                       IN PVOID Argument,
                       IN PVOID Count)
{
    volatile ULONG *PacketCount = Count;

    ASSERT(PacketContext != NULL);
    ASSERT(BroadcastFunction != NULL);
    ASSERT(PacketCount != NULL);

    InterlockedDecrementUL((PULONG)PacketCount);
    while (*PacketCount != 0)
    {
        YieldProcessor();
        KeMemoryBarrierWithoutFence();
    }

    ((PKIPI_BROADCAST_WORKER)BroadcastFunction)((ULONG_PTR)Argument);
    KiIpiSignalPacketDone(PacketContext);
}

VOID
FASTCALL
KiIpiSend(IN KAFFINITY TargetProcessors,
          IN ULONG IpiRequest)
{
    KAFFINITY RemainingSet, RemoteProcessors = 0;
    BOOLEAN RequestSelf = FALSE;
    ULONG ProcessorIndex;
    PKPRCB CurrentPrcb, TargetPrcb;
    KIRQL OldIrql;

    TargetProcessors &= KeActiveProcessors;
    if (TargetProcessors == 0)
        return;

    CurrentPrcb = KeGetCurrentPrcb();
    RemainingSet = TargetProcessors;
    while (RemainingSet != 0)
    {
        KAFFINITY SetMember;

        NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, RemainingSet) != 0);
        SetMember = AFFINITY_MASK(ProcessorIndex);
        RemainingSet &= ~SetMember;

        TargetPrcb = KiProcessorBlock[ProcessorIndex];
        ASSERT(TargetPrcb != NULL);
        InterlockedBitTestAndSet((PLONG)&TargetPrcb->IpiFrozen, IpiRequest);

        if (TargetPrcb == CurrentPrcb)
            RequestSelf = TRUE;
        else
            RemoteProcessors |= SetMember;
    }

    if (RemoteProcessors != 0)
        HalRequestIpi(RemoteProcessors);

    if (RequestSelf)
    {
        KeRaiseIrql(IPI_LEVEL, &OldIrql);
        KiIpiServiceRoutine(NULL, NULL);
        KeLowerIrql(OldIrql);
    }
}

VOID
NTAPI
KiIpiSendPacket(IN KAFFINITY TargetProcessors,
                IN PKIPI_WORKER WorkerFunction,
                IN PKIPI_BROADCAST_WORKER BroadcastFunction,
                IN ULONG_PTR Context,
                IN PULONG Count)
{
    KAFFINITY RemainingSet, RemoteProcessors = 0;
    BOOLEAN RequestSelf = FALSE;
    ULONG ProcessorIndex;
    PKPRCB CurrentPrcb, TargetPrcb;
    KIRQL OldIrql;

    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    ASSERT(WorkerFunction != NULL);

    TargetProcessors &= KeActiveProcessors;
    if (TargetProcessors == 0)
        return;

    CurrentPrcb = KeGetCurrentPrcb();
    (VOID)InterlockedExchangeUL(&CurrentPrcb->TargetSet, TargetProcessors);
    (VOID)InterlockedExchangePointer((PVOID *)&CurrentPrcb->WorkerRoutine, WorkerFunction);
    (VOID)InterlockedExchangePointer((PVOID *)&CurrentPrcb->CurrentPacket[0], BroadcastFunction);
    (VOID)InterlockedExchangePointer((PVOID *)&CurrentPrcb->CurrentPacket[1], (PVOID)Context);
    (VOID)InterlockedExchangePointer((PVOID *)&CurrentPrcb->CurrentPacket[2], Count);

    RemainingSet = TargetProcessors;
    while (RemainingSet != 0)
    {
        KAFFINITY SetMember;

        NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, RemainingSet) != 0);
        SetMember = AFFINITY_MASK(ProcessorIndex);
        RemainingSet &= ~SetMember;

        TargetPrcb = KiProcessorBlock[ProcessorIndex];
        ASSERT(TargetPrcb != NULL);
        while (InterlockedCompareExchangePointer((PVOID *)&TargetPrcb->SignalDone,
                                                 CurrentPrcb,
                                                 NULL) != NULL)
        {
            YieldProcessor();
            KeMemoryBarrierWithoutFence();
        }

        InterlockedBitTestAndSet((PLONG)&TargetPrcb->IpiFrozen, IPI_SYNCH_REQUEST);
        if (TargetPrcb == CurrentPrcb)
            RequestSelf = TRUE;
        else
            RemoteProcessors |= SetMember;
    }

    if (RemoteProcessors != 0)
        HalRequestIpi(RemoteProcessors);

    if (RequestSelf)
    {
        KeRaiseIrql(IPI_LEVEL, &OldIrql);
        KiIpiServiceRoutine(NULL, NULL);
        KeLowerIrql(OldIrql);
    }
}

VOID
FASTCALL
KiIpiSignalPacketDone(IN PKIPI_CONTEXT PacketContext)
{
    PKPRCB SourcePrcb = (PKPRCB)PacketContext;
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();

    ASSERT(SourcePrcb != NULL);

    InterlockedBitTestAndReset((PLONG)&SourcePrcb->TargetSet, CurrentPrcb->Number);
    (VOID)InterlockedExchangePointer((PVOID *)&CurrentPrcb->SignalDone, NULL);
}

VOID
FASTCALL
KiIpiSignalPacketDoneAndStall(IN PKIPI_CONTEXT PacketContext,
                              IN volatile PULONG ReverseStall)
{
    ASSERT(ReverseStall != NULL);

    KiIpiSignalPacketDone(PacketContext);
    while (*ReverseStall != 0)
    {
        YieldProcessor();
        KeMemoryBarrierWithoutFence();
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
KiIpiServiceRoutine(IN PKTRAP_FRAME TrapFrame,
                    IN PKEXCEPTION_FRAME ExceptionFrame)
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
        PKPRCB SourcePrcb = (PKPRCB)Prcb->SignalDone;
        PKIPI_WORKER WorkerRoutine;

        ASSERT(SourcePrcb != NULL);
        WorkerRoutine = SourcePrcb->WorkerRoutine;
        ASSERT(WorkerRoutine != NULL);

        WorkerRoutine((PKIPI_CONTEXT)SourcePrcb,
                      SourcePrcb->CurrentPacket[0],
                      SourcePrcb->CurrentPacket[1],
                      SourcePrcb->CurrentPacket[2]);
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

        while (Prcb->TargetSet != 0)
        {
            YieldProcessor();
            KeMemoryBarrierWithoutFence();
        }
    }
#endif

    KeLowerIrql(OldIrql2);

    /* Release the lock */
    KeReleaseSpinLockFromDpcLevel(&KiReverseStallIpiLock);

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
    return Status;
}

#endif // !_M_AMD64
