/*
 * PROJECT:         ReactOS ISA PnP Bus driver
 * FILE:            fdo.c
 * PURPOSE:         FDO-specific code
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */
#include <isapnp.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
IsaFdoStartDevice(
  IN PISAPNP_FDO_EXTENSION FdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status;
  KIRQL OldIrql;

  KeAcquireSpinLock(&FdoExt->Lock, &OldIrql);

  Status = IsaHwDetectReadDataPort(FdoExt);
  if (!NT_SUCCESS(Status))
  {
      KeReleaseSpinLock(&FdoExt->Lock, OldIrql);
      return Status;
  }

  FdoExt->Common.State = dsStarted;

  KeReleaseSpinLock(&FdoExt->Lock, OldIrql);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaFdoQueryDeviceRelations(
  IN PISAPNP_FDO_EXTENSION FdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status;
  PLIST_ENTRY CurrentEntry;
  PISAPNP_LOGICAL_DEVICE IsaDevice;
  PDEVICE_RELATIONS DeviceRelations;
  KIRQL OldIrql;
  ULONG i = 0;

  if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
      return Irp->IoStatus.Status;

  KeAcquireSpinLock(&FdoExt->Lock, &OldIrql);

  Status = IsaHwFillDeviceList(FdoExt);
  if (!NT_SUCCESS(Status))
  {
      KeReleaseSpinLock(&FdoExt->Lock, OldIrql);
      return Status;
  }

  DeviceRelations = ExAllocatePool(NonPagedPool,
                            sizeof(DEVICE_RELATIONS) + sizeof(DEVICE_OBJECT) * (FdoExt->DeviceCount - 1));
  if (!DeviceRelations)
  {
      KeReleaseSpinLock(&FdoExt->Lock, OldIrql);
      return STATUS_INSUFFICIENT_RESOURCES;
  }

  CurrentEntry = FdoExt->DeviceListHead.Flink;
  while (CurrentEntry != &FdoExt->DeviceListHead)
  {
     IsaDevice = CONTAINING_RECORD(CurrentEntry, ISAPNP_LOGICAL_DEVICE, ListEntry);

     DeviceRelations->Objects[i++] = IsaDevice->Common.Self;

     ObReferenceObject(IsaDevice->Common.Self);

     CurrentEntry = CurrentEntry->Flink;
  }

  DeviceRelations->Count = FdoExt->DeviceCount;

  KeReleaseSpinLock(&FdoExt->Lock, OldIrql);

  Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IsaFdoPnp(
  IN PISAPNP_FDO_EXTENSION FdoExt,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status = Irp->IoStatus.Status;

  switch (IrpSp->MinorFunction)
  {
     case IRP_MN_START_DEVICE:
       Status = IsaForwardIrpSynchronous(FdoExt, Irp);

       if (NT_SUCCESS(Status))
         Status = IsaFdoStartDevice(FdoExt, Irp, IrpSp);

       Irp->IoStatus.Status = Status;

       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return Status;

     case IRP_MN_STOP_DEVICE:
       FdoExt->Common.State = dsStopped;

       Status = STATUS_SUCCESS;
       break;

     case IRP_MN_QUERY_DEVICE_RELATIONS:
       Status = IsaFdoQueryDeviceRelations(FdoExt, Irp, IrpSp);

       Irp->IoStatus.Status = Status;

       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return Status;

     case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
       DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
       break;

     default:
       DPRINT1("Unknown PnP code: %x\n", IrpSp->MinorFunction);
       break;
  }

  IoSkipCurrentIrpStackLocation(Irp);

  return IoCallDriver(FdoExt->Ldo, Irp);
}
