/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP and power handling for the ACPI root FDO
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <wdmguid.h>
#include <debug.h>

/* Logs the interrupt band the HAL granted, the SCI is not part of it */
static
VOID
NTAPI
UacpiNtLogStartInterrupts(
    _In_opt_ PCM_RESOURCE_LIST Translated)
{
    PCM_FULL_RESOURCE_DESCRIPTOR Full;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial;
    ULONG FirstVector = 0;
    ULONG LastVector = 0;
    ULONG Count = 0;
    ULONG ListIndex;
    ULONG Index;

    if (!Translated)
        return;

    for (ListIndex = 0; ListIndex < Translated->Count; ListIndex++)
    {
        Full = &Translated->List[ListIndex];
        for (Index = 0; Index < Full->PartialResourceList.Count; Index++)
        {
            Partial = &Full->PartialResourceList.PartialDescriptors[Index];
            if (Partial->Type != CmResourceTypeInterrupt)
                continue;

            if (!Count)
                FirstVector = Partial->u.Interrupt.Vector;
            LastVector = Partial->u.Interrupt.Vector;
            Count++;
        }
    }

    if (Count)
        DPRINT("uACPI-NT: HAL interrupt band has %lu vector(s), 0x%lX to 0x%lX\n", Count, FirstVector, LastVector);
    else
        DPRINT("uACPI-NT: HAL reported no interrupt vectors\n");
}

static
NTSTATUS
NTAPI
UacpiNtFdoStartDevice(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    Status = UacpiNtForwardAndWait(Fdo->LowerDevice, Irp);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("uACPI-NT: lower START_DEVICE failed 0x%lx\n", Status);
        return UacpiNtCompleteIrp(Irp, Status, 0);
    }

    UacpiNtLogStartInterrupts(IoStack->Parameters.StartDevice.AllocatedResourcesTranslated);

    /* The IDT allocator may only hand out vectors from this grant */
    UacpiNtIrqLibSetHalVectors(IoStack->Parameters.StartDevice.AllocatedResources);

    Status = UacpiNtBringUpInterpreter(Fdo);
    if (!NT_SUCCESS(Status))
        return UacpiNtCompleteIrp(Irp, Status, 0);

    /* Notify routing and the interfaces find the FDO through this */
    GlobalAcpiFdo = Fdo;

    UacpiNtEnumerateNamespace(Fdo);
    UacpiNtIrqArbiterInitialize(Fdo);

    Fdo->Started = TRUE;
    return UacpiNtCompleteIrp(Irp, STATUS_SUCCESS, 0);
}

/* Serves the interrupt arbiter and translator, the IRP always goes down */
static
NTSTATUS
NTAPI
UacpiNtFdoQueryInterface(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    Status = UacpiNtQueryIrqArbiter(IoStack);
    if (Status != STATUS_NOT_SUPPORTED)
    {
        Irp->IoStatus.Status = Status;
    }
    else if (IsEqualGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                         &GUID_TRANSLATOR_INTERFACE_STANDARD) &&
             (ULONG_PTR)IoStack->Parameters.QueryInterface.InterfaceSpecificData == CmResourceTypeInterrupt)
    {
        Irp->IoStatus.Status = UacpiNtBuildIrqTranslator("ACPI root", IoStack);
    }

    return UacpiNtForwardAndForget(Fdo->LowerDevice, Irp);
}

static
NTSTATUS
NTAPI
UacpiNtFdoRemoveDevice(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PDEVICE_OBJECT LowerDevice = Fdo->LowerDevice;
    PDEVICE_OBJECT Self = Fdo->Shared.Self;
    NTSTATUS Status;

    UacpiNtTearDownInterpreter(Fdo);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Status = UacpiNtForwardAndForget(LowerDevice, Irp);

    IoDetachDevice(LowerDevice);
    IoDeleteDevice(Self);
    return Status;
}

NTSTATUS
NTAPI
UacpiNtFdoPnp(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    switch (IoStack->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return UacpiNtFdoStartDevice(Fdo, Irp);

        /* The raw PDO below has no children of its own */
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IoStack->Parameters.QueryDeviceRelations.Type != BusRelations)
                return UacpiNtForwardAndForget(Fdo->LowerDevice, Irp);

            Status = UacpiNtBuildBusRelations(Fdo, Irp);
            return UacpiNtCompleteIrp(Irp, Status, Irp->IoStatus.Information);

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Irp->IoStatus.Status = STATUS_SUCCESS;
            return UacpiNtForwardAndForget(Fdo->LowerDevice, Irp);

        case IRP_MN_QUERY_INTERFACE:
            return UacpiNtFdoQueryInterface(Fdo, Irp);

        case IRP_MN_REMOVE_DEVICE:
            return UacpiNtFdoRemoveDevice(Fdo, Irp);

        default:
            return UacpiNtForwardAndForget(Fdo->LowerDevice, Irp);
    }
}

NTSTATUS
NTAPI
UacpiNtFdoPower(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* Sleep and shutdown transitions run the _PTS and _WAK path */
    if (IoStack->MinorFunction == IRP_MN_SET_POWER &&
        IoStack->Parameters.Power.Type == SystemPowerState)
    {
        return UacpiNtSystemSetPower(Fdo, Irp);
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(Fdo->LowerDevice, Irp);
}
