/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/i386/kdsup.c
 * PURPOSE:         KD support routines for x86
 * PROGRAMMERS:     Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef UNIMPLEMENTED
#define UNIMPLEMENTED KdpDprintf("%s is unimplemented\n", __FUNCTION__)

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdpSysGetVersion(IN PDBGKD_GET_VERSION64 Version)
{
    /* Copy the version block */
    RtlCopyMemory(Version, &KdVersionBlock, sizeof(DBGKD_GET_VERSION64));
}

VOID
NTAPI
KdpGetStateChange(IN PDBGKD_MANIPULATE_STATE64 State,
                  IN PCONTEXT Context)
{
    PKPRCB Prcb;
    ULONG i;

    /* Check for success */
    if (NT_SUCCESS(State->u.Continue2.ContinueStatus))
    {
        /* Check if we're tracing */
        if (State->u.Continue2.ControlSet.TraceFlag)
        {
            /* Enable TF */
            Context->EFlags |= EFLAGS_TF;
        }
        else
        {
            /* Remove it */
            Context->EFlags &= ~EFLAGS_TF;
        }

        /* Loop all processors */
        for (i = 0; i < KeNumberProcessors; i++)
        {
            /* Get the PRCB and update DR7 and DR6 */
            Prcb = KiProcessorBlock[i];
            Prcb->ProcessorState.SpecialRegisters.KernelDr7 =
                State->u.Continue2.ControlSet.Dr7;
            Prcb->ProcessorState.SpecialRegisters.KernelDr6 = 0;
        }

        /* Check if we have new symbol information */
        if (State->u.Continue2.ControlSet.CurrentSymbolStart != 1)
        {
            /* Update it */
            KdpCurrentSymbolStart =
                State->u.Continue2.ControlSet.CurrentSymbolStart;
            KdpCurrentSymbolEnd= State->u.Continue2.ControlSet.CurrentSymbolEnd;
        }
    }
}

VOID
NTAPI
KdpSetContextState(IN PDBGKD_WAIT_STATE_CHANGE64 WaitStateChange,
                   IN PCONTEXT Context)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Copy i386 specific debug registers */
    WaitStateChange->ControlReport.Dr6 = Prcb->ProcessorState.SpecialRegisters.
                                         KernelDr6;
    WaitStateChange->ControlReport.Dr7 = Prcb->ProcessorState.SpecialRegisters.
                                         KernelDr7;

    /* Copy i386 specific segments */
    WaitStateChange->ControlReport.SegCs = (USHORT)Context->SegCs;
    WaitStateChange->ControlReport.SegDs = (USHORT)Context->SegDs;
    WaitStateChange->ControlReport.SegEs = (USHORT)Context->SegEs;
    WaitStateChange->ControlReport.SegFs = (USHORT)Context->SegFs;

    /* Copy EFlags */
    WaitStateChange->ControlReport.EFlags = Context->EFlags;

    /* Set Report Flags */
    WaitStateChange->ControlReport.ReportFlags = REPORT_INCLUDES_SEGS;
    if (WaitStateChange->ControlReport.SegCs == KGDT_R0_CODE)
    {
        WaitStateChange->ControlReport.ReportFlags = REPORT_STANDARD_CS;
    }
}

NTSTATUS
NTAPI
KdpSysReadMsr(IN ULONG Msr,
              OUT PLARGE_INTEGER MsrValue)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysWriteMsr(IN ULONG Msr,
               IN PLARGE_INTEGER MsrValue)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysReadBusData(IN ULONG BusDataType,
                  IN ULONG BusNumber,
                  IN ULONG SlotNumber,
                  IN PVOID Buffer,
                  IN ULONG Offset,
                  IN ULONG Length,
                  OUT PULONG ActualLength)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysWriteBusData(IN ULONG BusDataType,
                   IN ULONG BusNumber,
                   IN ULONG SlotNumber,
                   IN PVOID Buffer,
                   IN ULONG Offset,
                   IN ULONG Length,
                   OUT PULONG ActualLength)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysReadControlSpace(IN ULONG Processor,
                       IN ULONG64 BaseAddress,
                       IN PVOID Buffer,
                       IN ULONG Length,
                       OUT PULONG ActualLength)
{
    PVOID ControlStart;
    ULONG RealLength;

    /* Make sure that this is a valid request */
    if (((ULONG)BaseAddress < sizeof(KPROCESSOR_STATE)) &&
        (Processor < KeNumberProcessors))
    {
        /* Get the actual length */
        RealLength = sizeof(KPROCESSOR_STATE) - (ULONG_PTR)BaseAddress;
        if (RealLength < Length) Length = RealLength;

        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)BaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[Processor]->
                                           ProcessorState);

        /* Copy the memory */
        RtlCopyMemory(Buffer, ControlStart, Length);

        /* Finish up */
        *ActualLength = Length;
        return STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        *ActualLength = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NTAPI
KdpSysWriteControlSpace(IN ULONG Processor,
                        IN ULONG64 BaseAddress,
                        IN PVOID Buffer,
                        IN ULONG Length,
                        OUT PULONG ActualLength)
{
    PVOID ControlStart;

    /* Make sure that this is a valid request */
    if ((((ULONG)BaseAddress + Length) <= sizeof(KPROCESSOR_STATE)) &&
        (Processor < KeNumberProcessors))
    {
        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)BaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[Processor]->
                                           ProcessorState);

        /* Copy the memory */
        RtlCopyMemory(ControlStart, Buffer, Length);

        /* Finish up */
        *ActualLength = Length;
        return STATUS_SUCCESS;
    }
    else
    {
        /* Invalid request */
        *ActualLength = 0;
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
NTAPI
KdpSysReadIoSpace(IN ULONG InterfaceType,
                  IN ULONG BusNumber,
                  IN ULONG AddressSpace,
                  IN ULONG64 IoAddress,
                  IN PULONG DataValue,
                  IN ULONG DataSize,
                  OUT PULONG ActualDataSize)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysWriteIoSpace(IN ULONG InterfaceType,
                   IN ULONG BusNumber,
                   IN ULONG AddressSpace,
                   IN ULONG64 IoAddress,
                   IN PULONG DataValue,
                   IN ULONG DataSize,
                   OUT PULONG ActualDataSize)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysCheckLowMemory(IN ULONG Flags)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
}
