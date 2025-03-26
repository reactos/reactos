/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/amd64/kdx64.c
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
KdpSetContextState(IN PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
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
    if (WaitStateChange->ControlReport.SegCs == KGDT64_R0_CODE)
    {
        WaitStateChange->ControlReport.ReportFlags |= REPORT_STANDARD_CS;
    }
}

NTSTATUS
NTAPI
KdpSysReadMsr(
    _In_ ULONG Msr,
    _Out_ PULONGLONG MsrValue)
{
    /* Use SEH to protect from invalid MSRs */
    _SEH2_TRY
    {
        *MsrValue = __readmsr(Msr);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return STATUS_NO_SUCH_DEVICE);
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysWriteMsr(
    _In_ ULONG Msr,
    _In_ PULONGLONG MsrValue)
{
    /* Use SEH to protect from invalid MSRs */
    _SEH2_TRY
    {
        __writemsr(Msr, *MsrValue);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        _SEH2_YIELD(return STATUS_NO_SUCH_DEVICE);
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysReadBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ ULONG Offset,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysWriteBusData(
    _In_ BUS_DATA_TYPE BusDataType,
    _In_ ULONG BusNumber,
    _In_ ULONG SlotNumber,
    _In_ ULONG Offset,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpSysReadControlSpace(
    _In_ ULONG Processor,
    _In_ ULONG64 BaseAddress,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength)
{
    PVOID ControlStart;
    PKPRCB Prcb = KiProcessorBlock[Processor];
    PKIPCR Pcr = CONTAINING_RECORD(Prcb, KIPCR, Prcb);

    switch (BaseAddress)
    {
        case AMD64_DEBUG_CONTROL_SPACE_KPCR:
            /* Copy a pointer to the Pcr */
            ControlStart = &Pcr;
            *ActualLength = sizeof(PVOID);
            break;

        case AMD64_DEBUG_CONTROL_SPACE_KPRCB:
            /* Copy a pointer to the Prcb */
            ControlStart = &Prcb;
            *ActualLength = sizeof(PVOID);
            break;

        case AMD64_DEBUG_CONTROL_SPACE_KSPECIAL:
            /* Copy SpecialRegisters */
            ControlStart = &Prcb->ProcessorState.SpecialRegisters;
            *ActualLength = sizeof(KSPECIAL_REGISTERS);
            break;

        case AMD64_DEBUG_CONTROL_SPACE_KTHREAD:
            /* Copy a pointer to the current Thread */
            ControlStart = &Prcb->CurrentThread;
            *ActualLength = sizeof(PVOID);
            break;

        default:
            *ActualLength = 0;
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
    }

    /* Copy the memory */
    RtlCopyMemory(Buffer, ControlStart, min(Length, *ActualLength));

    /* Finish up */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysWriteControlSpace(
    _In_ ULONG Processor,
    _In_ ULONG64 BaseAddress,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength)
{
    PVOID ControlStart;
    PKPRCB Prcb = KiProcessorBlock[Processor];

    switch (BaseAddress)
    {
        case AMD64_DEBUG_CONTROL_SPACE_KSPECIAL:
            /* Copy SpecialRegisters */
            ControlStart = &Prcb->ProcessorState.SpecialRegisters;
            *ActualLength = sizeof(KSPECIAL_REGISTERS);
            break;

        default:
            *ActualLength = 0;
            ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
    }

    /* Copy the memory */
    RtlCopyMemory(ControlStart, Buffer, min(Length, *ActualLength));

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysReadIoSpace(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG AddressSpace,
    _In_ ULONG64 IoAddress,
    _Out_writes_bytes_(DataSize) PVOID DataValue,
    _In_ ULONG DataSize,
    _Out_ PULONG ActualDataSize)
{
    /* Verify parameters */
    if (InterfaceType != Isa || BusNumber != 0 || AddressSpace != 1)
    {
        /* No data was read */
        *ActualDataSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for correct alignment */
    if ((IoAddress & (DataSize - 1)))
    {
        /* Invalid alignment */
        *ActualDataSize = 0;
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    switch (DataSize)
    {
        case sizeof(UCHAR):
            /* Read one UCHAR */
            *(PUCHAR)DataValue = READ_PORT_UCHAR((PUCHAR)IoAddress);
            break;

        case sizeof(USHORT):
            /* Read one USHORT */
            *(PUSHORT)DataValue = READ_PORT_USHORT((PUSHORT)IoAddress);
            break;

        case sizeof(ULONG):
            /* Read one ULONG */
            *(PULONG)DataValue = READ_PORT_ULONG((PULONG)IoAddress);
            break;

        default:
            /* Invalid data size */
            *ActualDataSize = 0;
            return STATUS_INVALID_PARAMETER;
    }

    /* Return the size of the data */
    *ActualDataSize = DataSize;

    /* Success! */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysWriteIoSpace(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG AddressSpace,
    _In_ ULONG64 IoAddress,
    _In_reads_bytes_(DataSize) PVOID DataValue,
    _In_ ULONG DataSize,
    _Out_ PULONG ActualDataSize)
{
    /* Verify parameters */
    if (InterfaceType != Isa || BusNumber != 0 || AddressSpace != 1)
    {
        /* No data was written */
        *ActualDataSize = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* Check for correct alignment */
    if ((IoAddress & (DataSize - 1)))
    {
        /* Invalid alignment */
        *ActualDataSize = 0;
        return STATUS_DATATYPE_MISALIGNMENT;
    }

    switch (DataSize)
    {
        case sizeof(UCHAR):
            /* Write one UCHAR */
            WRITE_PORT_UCHAR((PUCHAR)IoAddress, *(PUCHAR)DataValue);
            break;

        case sizeof(USHORT):
            /* Write one USHORT */
            WRITE_PORT_USHORT((PUSHORT)IoAddress, *(PUSHORT)DataValue);
            break;

        case sizeof(ULONG):
            /* Write one ULONG */
            WRITE_PORT_ULONG((PULONG)IoAddress, *(PULONG)DataValue);
            break;

        default:
            /* Invalid data size */
            *ActualDataSize = 0;
            return STATUS_INVALID_PARAMETER;
    }

    /* Return the size of the data */
    *ActualDataSize = DataSize;

    /* Success! */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdpSysCheckLowMemory(IN ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpAllowDisable(VOID)
{
    ULONG i;

    /* Loop every processor */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        PKPROCESSOR_STATE ProcessorState = &KiProcessorBlock[i]->ProcessorState;

        /* If any processor breakpoints are active,
         * we can't allow running without a debugger */
        if (ProcessorState->SpecialRegisters.KernelDr7 & 0xFF)
            return STATUS_ACCESS_DENIED;
    }

    /* No processor breakpoints, allow disabling the debugger */
    return STATUS_SUCCESS;
}

/* EOF */
