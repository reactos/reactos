/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/kd64/i386/kdx86.c
 * PURPOSE:         KD support routines for x86
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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
    if (WaitStateChange->ControlReport.SegCs == KGDT_R0_CODE)
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
    /* Just forward to HAL */
    *ActualLength = HalGetBusDataByOffset(BusDataType,
                                          BusNumber,
                                          SlotNumber,
                                          Buffer,
                                          Offset,
                                          Length);

    /* Return status */
    return (*ActualLength != 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
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
    /* Just forward to HAL */
    *ActualLength = HalSetBusDataByOffset(BusDataType,
                                          BusNumber,
                                          SlotNumber,
                                          Buffer,
                                          Offset,
                                          Length);

    /* Return status */
    return (*ActualLength != 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
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
    ULONG RealLength;

    /* Make sure that this is a valid request */
    if ((BaseAddress < sizeof(KPROCESSOR_STATE)) &&
        (Processor < KeNumberProcessors))
    {
        /* Get the actual length */
        RealLength = sizeof(KPROCESSOR_STATE) - (ULONG_PTR)BaseAddress;
        if (RealLength < Length) Length = RealLength;

        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)BaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[Processor]->
                                           ProcessorState);

        /* Read the control state safely */
        return KdpCopyMemoryChunks((ULONG_PTR)Buffer,
                                   ControlStart,
                                   Length,
                                   0,
                                   MMDBG_COPY_UNSAFE | MMDBG_COPY_WRITE,
                                   ActualLength);
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
KdpSysWriteControlSpace(
    _In_ ULONG Processor,
    _In_ ULONG64 BaseAddress,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG ActualLength)
{
    PVOID ControlStart;

    /* Make sure that this is a valid request */
    if (((BaseAddress + Length) <= sizeof(KPROCESSOR_STATE)) &&
        (Processor < KeNumberProcessors))
    {
        /* Set the proper address */
        ControlStart = (PVOID)((ULONG_PTR)BaseAddress +
                               (ULONG_PTR)&KiProcessorBlock[Processor]->
                                           ProcessorState);

        /* Write the control state safely */
        return KdpCopyMemoryChunks((ULONG_PTR)Buffer,
                                   ControlStart,
                                   Length,
                                   0,
                                   MMDBG_COPY_UNSAFE,
                                   ActualLength);
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
KdpSysReadIoSpace(
    _In_ INTERFACE_TYPE InterfaceType,
    _In_ ULONG BusNumber,
    _In_ ULONG AddressSpace,
    _In_ ULONG64 IoAddress,
    _Out_writes_bytes_(DataSize) PVOID DataValue,
    _In_ ULONG DataSize,
    _Out_ PULONG ActualDataSize)
{
    NTSTATUS Status;

    /* Verify parameters */
    if ((InterfaceType != Isa) || (BusNumber != 0) || (AddressSpace != 1))
    {
        /* Fail, we don't support this */
        *ActualDataSize = 0;
        return STATUS_UNSUCCESSFUL;
    }

    /* Check the size */
    switch (DataSize)
    {
        case sizeof(UCHAR):
        {
            /* Read 1 byte */
            *(PUCHAR)DataValue =
                READ_PORT_UCHAR((PUCHAR)(ULONG_PTR)IoAddress);
            *ActualDataSize = sizeof(UCHAR);
            Status = STATUS_SUCCESS;
            break;
        }

        case sizeof(USHORT):
        {
            /* Make sure the address is aligned */
            if ((IoAddress & (sizeof(USHORT) - 1)) != 0)
            {
                /* It isn't, bail out */
                *ActualDataSize = 0;
                Status = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }

            /* Read 2 bytes */
            *(PUSHORT)DataValue =
                READ_PORT_USHORT((PUSHORT)(ULONG_PTR)IoAddress);
            *ActualDataSize = sizeof(USHORT);
            Status = STATUS_SUCCESS;
            break;
        }

        case sizeof(ULONG):
        {
            /* Make sure the address is aligned */
            if ((IoAddress & (sizeof(ULONG) - 1)) != 0)
            {
                /* It isn't, bail out */
                *ActualDataSize = 0;
                Status = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }

            /* Read 4 bytes */
            *(PULONG)DataValue =
                READ_PORT_ULONG((PULONG)(ULONG_PTR)IoAddress);
            *ActualDataSize = sizeof(ULONG);
            Status = STATUS_SUCCESS;
            break;
        }

        default:
            /* Invalid size, fail */
            *ActualDataSize = 0;
            Status = STATUS_INVALID_PARAMETER;
    }

    /* Return status */
    return Status;
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
    NTSTATUS Status;

    /* Verify parameters */
    if ((InterfaceType != Isa) || (BusNumber != 0) || (AddressSpace != 1))
    {
        /* Fail, we don't support this */
        *ActualDataSize = 0;
        return STATUS_UNSUCCESSFUL;
    }

    /* Check the size */
    switch (DataSize)
    {
        case sizeof(UCHAR):
        {
            /* Write 1 byte */
            WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR)IoAddress,
                             *(PUCHAR)DataValue);
            *ActualDataSize = sizeof(UCHAR);
            Status = STATUS_SUCCESS;
            break;
        }

        case sizeof(USHORT):
        {
            /* Make sure the address is aligned */
            if ((IoAddress & (sizeof(USHORT) - 1)) != 0)
            {
                /* It isn't, bail out */
                *ActualDataSize = 0;
                Status = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }

            /* Write 2 bytes */
            WRITE_PORT_USHORT((PUSHORT)(ULONG_PTR)IoAddress,
                             *(PUSHORT)DataValue);
            *ActualDataSize = sizeof(USHORT);
            Status = STATUS_SUCCESS;
            break;
        }

        case sizeof(ULONG):
        {
            /* Make sure the address is aligned */
            if ((IoAddress & (sizeof(ULONG) - 1)) != 0)
            {
                /* It isn't, bail out */
                *ActualDataSize = 0;
                Status = STATUS_DATATYPE_MISALIGNMENT;
                break;
            }

            /* Write 4 bytes */
            WRITE_PORT_ULONG((PULONG)(ULONG_PTR)IoAddress,
                             *(PULONG)DataValue);
            *ActualDataSize = sizeof(ULONG);
            Status = STATUS_SUCCESS;
            break;
        }

        default:
            /* Invalid size, fail */
            *ActualDataSize = 0;
            Status = STATUS_INVALID_PARAMETER;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
KdpSysCheckLowMemory(IN ULONG Flags)
{
    /* Stubbed as we don't support PAE */
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
