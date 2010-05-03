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

     case IRP_MN_QUERY_RESOURCES:
       DPRINT1("IRP_MN_QUERY_RESOURCES is UNIMPLEMENTED!\n");
       break;

     case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
       DPRINT1("IRP_MN_QUERY_RESOURCE_REQUIREMENTS is UNIMPLEMENTED!\n");
       break;

     default:
       DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
       break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return Status;
}
