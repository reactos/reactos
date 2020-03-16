/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            pdo.c
 * PURPOSE:         PDO-specific code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <isapnp.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IsaPdoQueryDeviceRelations(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS DeviceRelations;

    if (IrpSp->Parameters.QueryDeviceRelations.Type == RemovalRelations &&
        PdoExt->Common.Self == PdoExt->FdoExt->DataPortPdo)
    {
        return IsaPnpFillDeviceRelations(PdoExt->FdoExt, Irp, FALSE);
    }

    if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
        return Irp->IoStatus.Status;

    DeviceRelations = ExAllocatePool(PagedPool, sizeof(*DeviceRelations));
    if (!DeviceRelations)
        return STATUS_NO_MEMORY;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = PdoExt->Common.Self;
    ObReferenceObject(PdoExt->Common.Self);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryCapabilities(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DeviceCapabilities;
    PISAPNP_LOGICAL_DEVICE LogDev = PdoExt->IsaPnpDevice;
    ULONG i;

    DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;
    if (DeviceCapabilities->Version != 1)
        return STATUS_REVISION_MISMATCH;

    if (LogDev)
    {
        DeviceCapabilities->UniqueID = LogDev->SerialNumber != 0xffffffff;
        DeviceCapabilities->Address = LogDev->CSN;
    }
    else
    {
        DeviceCapabilities->UniqueID = TRUE;
        DeviceCapabilities->SilentInstall = TRUE;
        DeviceCapabilities->RawDeviceOK = TRUE;
        for (i = 0; i < POWER_SYSTEM_MAXIMUM; i++)
            DeviceCapabilities->DeviceState[i] = PowerDeviceD3;
        DeviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryId(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    UNICODE_STRING EmptyString = RTL_CONSTANT_STRING(L"");
    PUNICODE_STRING Source;
    PWCHAR Buffer;

    switch (IrpSp->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            Source = &PdoExt->DeviceID;
            break;

        case BusQueryHardwareIDs:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
            Source = &PdoExt->HardwareIDs;
            break;

        case BusQueryCompatibleIDs:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            Source = &EmptyString;
            break;

        case BusQueryInstanceID:
            DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            Source = &PdoExt->InstanceID;
            break;

        default:
          DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n",
                  IrpSp->Parameters.QueryId.IdType);
          return Irp->IoStatus.Status;
    }

    Buffer = ExAllocatePool(PagedPool, Source->MaximumLength);
    if (!Buffer)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Buffer, Source->Buffer, Source->MaximumLength);
    Irp->IoStatus.Information = (ULONG_PTR)Buffer;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoPnp(
    IN PISAPNP_PDO_EXTENSION PdoExt,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status = Irp->IoStatus.Status;

    switch (IrpSp->MinorFunction)
    {
       case IRP_MN_START_DEVICE:
           if (PdoExt->IsaPnpDevice)
               Status = IsaHwActivateDevice(PdoExt->IsaPnpDevice);
           else
               Status = STATUS_SUCCESS;

         if (NT_SUCCESS(Status))
             PdoExt->Common.State = dsStarted;
         break;

       case IRP_MN_STOP_DEVICE:
           if (PdoExt->IsaPnpDevice)
               Status = IsaHwDeactivateDevice(PdoExt->IsaPnpDevice);
           else
               Status = STATUS_SUCCESS;

         if (NT_SUCCESS(Status))
             PdoExt->Common.State = dsStopped;
         break;

       case IRP_MN_QUERY_DEVICE_RELATIONS:
         Status = IsaPdoQueryDeviceRelations(PdoExt, Irp, IrpSp);
         break;

       case IRP_MN_QUERY_CAPABILITIES:
         Status = IsaPdoQueryCapabilities(PdoExt, Irp, IrpSp);
         break;

       case IRP_MN_QUERY_RESOURCES:
         DPRINT1("IRP_MN_QUERY_RESOURCES is UNIMPLEMENTED!\n");
         break;

       case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
         DPRINT1("IRP_MN_QUERY_RESOURCE_REQUIREMENTS is UNIMPLEMENTED!\n");
         break;

       case IRP_MN_QUERY_ID:
         Status = IsaPdoQueryId(PdoExt, Irp, IrpSp);
         break;

       default:
         DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
         break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
