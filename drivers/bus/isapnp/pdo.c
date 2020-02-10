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
  IN PISAPNP_LOGICAL_DEVICE LogDev,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  PDEVICE_RELATIONS DeviceRelations;

  if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
      return Irp->IoStatus.Status;

  DeviceRelations = ExAllocatePool(PagedPool, sizeof(*DeviceRelations));
  if (!DeviceRelations)
      return STATUS_INSUFFICIENT_RESOURCES;

  DeviceRelations->Count = 1;
  DeviceRelations->Objects[0] = LogDev->Common.Self;
  ObReferenceObject(LogDev->Common.Self);

  Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryCapabilities(
  IN PISAPNP_LOGICAL_DEVICE LogDev,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  PDEVICE_CAPABILITIES DeviceCapabilities;

  DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;
  if (DeviceCapabilities->Version != 1)
    return STATUS_UNSUCCESSFUL;

  DeviceCapabilities->UniqueID = LogDev->SerialNumber != 0xffffffff;
  DeviceCapabilities->Address = LogDev->CSN;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoQueryId(
  IN PISAPNP_LOGICAL_DEVICE LogDev,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  WCHAR Temp[256];
  PWCHAR Buffer, End;
  ULONG Length;
  NTSTATUS Status;

  switch (IrpSp->Parameters.QueryId.IdType)
  {
    case BusQueryDeviceID:
    {
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
      Status = RtlStringCbPrintfExW(Temp, sizeof(Temp),
                                    &End,
                                    NULL, 0,
                                    L"ISAPNP\\%3S%04X",
                                    LogDev->VendorId,
                                    LogDev->ProdId);
      if (!NT_SUCCESS(Status))
        return Status;
      Length = End - Temp;
      Temp[Length++] = UNICODE_NULL;
      break;
    }

    case BusQueryHardwareIDs:
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
      Status = RtlStringCbPrintfExW(Temp, sizeof(Temp),
                                    &End,
                                    NULL, 0,
                                    L"ISAPNP\\%3S%04X",
                                    LogDev->VendorId,
                                    LogDev->ProdId);
      if (!NT_SUCCESS(Status))
        return Status;
      Length = End - Temp;
      Temp[Length++] = UNICODE_NULL;
      Status = RtlStringCbPrintfExW(Temp + Length, sizeof(Temp) - Length,
                                    &End,
                                    NULL, 0,
                                    L"*%3S%04X",
                                    LogDev->VendorId,
                                    LogDev->ProdId);
      if (!NT_SUCCESS(Status))
        return Status;
      Length = End - Temp;
      Temp[Length++] = UNICODE_NULL;
      Temp[Length++] = UNICODE_NULL;
      break;

    case BusQueryInstanceID:
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
      Status = RtlStringCbPrintfExW(Temp, sizeof(Temp),
                                    &End,
                                    NULL, 0,
                                    L"%u",
                                    LogDev->SerialNumber);
      if (!NT_SUCCESS(Status))
        return Status;
      Length = End - Temp;
      Temp[Length++] = UNICODE_NULL;
      break;

    default:
      DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n",
              IrpSp->Parameters.QueryId.IdType);
      return Irp->IoStatus.Status;
  }

  Buffer = ExAllocatePool(PagedPool, Length * sizeof(WCHAR));
  if (!Buffer)
      return STATUS_NO_MEMORY;

  RtlCopyMemory(Buffer, Temp, Length * sizeof(WCHAR));
  Irp->IoStatus.Information = (ULONG_PTR)Buffer;
  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaPdoPnp(
  IN PISAPNP_LOGICAL_DEVICE LogDev,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status = Irp->IoStatus.Status;

  switch (IrpSp->MinorFunction)
  {
     case IRP_MN_START_DEVICE:
       Status = IsaHwActivateDevice(LogDev);

       if (NT_SUCCESS(Status))
           LogDev->Common.State = dsStarted;
       break;

     case IRP_MN_STOP_DEVICE:
       Status = IsaHwDeactivateDevice(LogDev);

       if (NT_SUCCESS(Status))
           LogDev->Common.State = dsStopped;
       break;

     case IRP_MN_QUERY_DEVICE_RELATIONS:
       Status = IsaPdoQueryDeviceRelations(LogDev, Irp, IrpSp);
       break;

     case IRP_MN_QUERY_CAPABILITIES:
       Status = IsaPdoQueryCapabilities(LogDev, Irp, IrpSp);
       break;

     case IRP_MN_QUERY_RESOURCES:
       DPRINT1("IRP_MN_QUERY_RESOURCES is UNIMPLEMENTED!\n");
       break;

     case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
       DPRINT1("IRP_MN_QUERY_RESOURCE_REQUIREMENTS is UNIMPLEMENTED!\n");
       break;

     case IRP_MN_QUERY_ID:
       Status = IsaPdoQueryId(LogDev, Irp, IrpSp);
       break;

     default:
       DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
       break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}
