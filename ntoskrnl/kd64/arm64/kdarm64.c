/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     KD support routines for ARM64
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* Control space is a set of pointers to the per processor structures */
#define ARM64_CONTROL_SPACE_KPCR        0
#define ARM64_CONTROL_SPACE_KPRCB       1
#define ARM64_CONTROL_SPACE_KSPECIAL    2
#define ARM64_CONTROL_SPACE_KTHREAD     3

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdpGetStateChange(
    _In_ PDBGKD_MANIPULATE_STATE64 State,
    _In_ PCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Context);

    if (!NT_SUCCESS(State->u.Continue2.ContinueStatus))
        return;

    if (State->u.Continue2.ControlSet.CurrentSymbolStart != 1)
    {
        KdpCurrentSymbolStart = State->u.Continue2.ControlSet.CurrentSymbolStart;
        KdpCurrentSymbolEnd = State->u.Continue2.ControlSet.CurrentSymbolEnd;
    }
}

VOID
NTAPI
KdpSetContextState(
    _In_ PDBGKD_ANY_WAIT_STATE_CHANGE WaitStateChange,
    _In_ PCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Context);

    WaitStateChange->AnyControlReport.ARM64ControlReport.Bvr = 0;
    WaitStateChange->AnyControlReport.ARM64ControlReport.Wvr = 0;
}

NTSTATUS
NTAPI
KdpSysReadMsr(
    _In_ ULONG Msr,
    _Out_ PULONGLONG MsrValue)
{
    UNREFERENCED_PARAMETER(Msr);
    *MsrValue = 0;
    return STATUS_NO_SUCH_DEVICE;
}

NTSTATUS
NTAPI
KdpSysWriteMsr(
    _In_ ULONG Msr,
    _In_ PULONGLONG MsrValue)
{
    UNREFERENCED_PARAMETER(Msr);
    UNREFERENCED_PARAMETER(MsrValue);
    return STATUS_NO_SUCH_DEVICE;
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
    *ActualLength = HalGetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, Offset, Length);
    return *ActualLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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
    *ActualLength = HalSetBusDataByOffset(BusDataType, BusNumber, SlotNumber, Buffer, Offset, Length);
    return *ActualLength ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
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
    PKPRCB Prcb = KiProcessorBlock[Processor];
    PKIPCR Pcr = CONTAINING_RECORD(Prcb, KIPCR, Prcb);
    PVOID ControlStart;

    switch (BaseAddress)
    {
        case ARM64_CONTROL_SPACE_KPCR:
            ControlStart = &Pcr;
            *ActualLength = sizeof(PVOID);
            break;

        case ARM64_CONTROL_SPACE_KPRCB:
            ControlStart = &Prcb;
            *ActualLength = sizeof(PVOID);
            break;

        case ARM64_CONTROL_SPACE_KSPECIAL:
            ControlStart = &Prcb->ProcessorState.SpecialRegisters;
            *ActualLength = sizeof(KSPECIAL_REGISTERS);
            break;

        case ARM64_CONTROL_SPACE_KTHREAD:
            ControlStart = &Prcb->CurrentThread;
            *ActualLength = sizeof(PVOID);
            break;

        default:
            *ActualLength = 0;
            return STATUS_UNSUCCESSFUL;
    }

    RtlCopyMemory(Buffer, ControlStart, min(Length, *ActualLength));
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
    PKPRCB Prcb = KiProcessorBlock[Processor];

    if (BaseAddress != ARM64_CONTROL_SPACE_KSPECIAL)
    {
        *ActualLength = 0;
        return STATUS_UNSUCCESSFUL;
    }

    *ActualLength = sizeof(KSPECIAL_REGISTERS);
    RtlCopyMemory(&Prcb->ProcessorState.SpecialRegisters, Buffer, min(Length, *ActualLength));
    return STATUS_SUCCESS;
}

/* There is no I/O space on ARM64 */
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
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(AddressSpace);
    UNREFERENCED_PARAMETER(IoAddress);
    UNREFERENCED_PARAMETER(DataValue);
    UNREFERENCED_PARAMETER(DataSize);
    *ActualDataSize = 0;
    return STATUS_NOT_SUPPORTED;
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
    UNREFERENCED_PARAMETER(InterfaceType);
    UNREFERENCED_PARAMETER(BusNumber);
    UNREFERENCED_PARAMETER(AddressSpace);
    UNREFERENCED_PARAMETER(IoAddress);
    UNREFERENCED_PARAMETER(DataValue);
    UNREFERENCED_PARAMETER(DataSize);
    *ActualDataSize = 0;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
KdpSysCheckLowMemory(
    _In_ ULONG Flags)
{
    UNREFERENCED_PARAMETER(Flags);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
KdpAllowDisable(VOID)
{
    /* No hardware breakpoints are programmed by the debugger yet */
    return STATUS_SUCCESS;
}
