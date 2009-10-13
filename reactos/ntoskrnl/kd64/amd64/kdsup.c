/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/amd64/kdsup.c
 * PURPOSE:         KD support routines for AMD64
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
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
    Version->MajorVersion = 0;
    Version->MinorVersion = 0;
    Version->ProtocolVersion = DBGKD_64BIT_PROTOCOL_VERSION2;
    Version->KdSecondaryVersion = KD_SECONDARY_VERSION_AMD64_CONTEXT;
    Version->Flags = DBGKD_VERS_FLAG_PTR64 | DBGKD_VERS_FLAG_DATA;
    Version->MachineType = IMAGE_FILE_MACHINE_AMD64;
    Version->MaxPacketType = PACKET_TYPE_MAX;
    Version->MaxStateChange = 0;
    Version->MaxManipulate = 0;
    Version->Simulation = DBGKD_SIMULATION_NONE;
    Version->Unused[0] = 0;
    Version->KernBase = 0xfffff80000800000ULL;
    Version->PsLoadedModuleList = (ULONG_PTR)&KeLoaderBlock->LoadOrderListHead;
    Version->DebuggerDataList = 0;
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
    if (WaitStateChange->ControlReport.SegCs == KGDT_64_R0_CODE)
    {
        WaitStateChange->ControlReport.ReportFlags = REPORT_STANDARD_CS;
    }
}

NTSTATUS
NTAPI
KdpSysReadMsr(IN ULONG Msr,
              OUT PLARGE_INTEGER MsrValue)
{
    MsrValue->QuadPart = __readmsr(Msr);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysWriteMsr(IN ULONG Msr,
               IN PLARGE_INTEGER MsrValue)
{
    __writemsr(Msr, MsrValue->QuadPart);
    return STATUS_SUCCESS;
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

    if ((ULONG)BaseAddress <= 2)
    {
        PKPRCB Prcb = KiProcessorBlock[Processor];
        PKIPCR Pcr = CONTAINING_RECORD(Prcb, KIPCR, Prcb);

        switch ((ULONG_PTR)BaseAddress)
        {
            case 0:
                /* Copy a pointer to the Pcr */
                ControlStart = &Pcr;
                RealLength = sizeof(PVOID);
                break;

            case 1:
                /* Copy a pointer to the Prcb */
                ControlStart = &Prcb;
                RealLength = sizeof(PVOID);
                break;

            case 2:
                /* Copy SpecialRegisters */
                ControlStart = &Prcb->ProcessorState.SpecialRegisters;
                RealLength = sizeof(KSPECIAL_REGISTERS);
                break;

            default:
                RealLength = 0;
                ControlStart = NULL;
                ASSERT(FALSE);
        }

        if (RealLength < Length) Length = RealLength;

        /* Copy the memory */
        RtlCopyMemory(Buffer, ControlStart, Length);
        *ActualLength = Length;

        /* Finish up */
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
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_UNSUCCESSFUL;
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
