/* $Id: pnproot.c,v 1.3 2001/08/14 21:14:05 hbirr Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/pnproot.c
 * PURPOSE:        PnP manager root device
 * PROGRAMMER:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *  16/04/2001 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE {
  LIST_ENTRY ListEntry;
  PDEVICE_OBJECT Pdo;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

PDEVICE_OBJECT PnpRootDeviceObject;
LIST_ENTRY PnpRootDeviceListHead;
ULONG PnpRootDeviceListCount;
KSPIN_LOCK PnpRootDeviceListLock;

/* FUNCTIONS *****************************************************************/

NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject)
{
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;

  DPRINT("Called\n");

  Device = (PPNPROOT_DEVICE)ExAllocatePool(PagedPool, sizeof(PNPROOT_DEVICE));
  if (!Device)
    return STATUS_INSUFFICIENT_RESOURCES;

  Status = IoCreateDevice(PnpRootDeviceObject->DriverObject, 0,
    NULL, FILE_DEVICE_CONTROLLER, 0, FALSE, &Device->Pdo);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
      ExFreePool(Device);
      return Status;
    }

  Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

  ObReferenceObject(Device->Pdo);

  ExInterlockedInsertTailList(&PnpRootDeviceListHead,
    &Device->ListEntry,
    &PnpRootDeviceListLock);

  *PhysicalDeviceObject = Device->Pdo;

  return STATUS_SUCCESS;
}


NTSTATUS
PnpRootQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (PnpRootDeviceListCount - 1);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = PnpRootDeviceListCount;

  i = 0;
  CurrentEntry = PnpRootDeviceListHead.Flink;
  while (CurrentEntry != &PnpRootDeviceListHead)
    {
    Device = CONTAINING_RECORD(
      CurrentEntry, PNPROOT_DEVICE, ListEntry);

    if (!Device->Pdo) {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(DeviceObject->DriverObject, 0,
        NULL, FILE_DEVICE_CONTROLLER, 0, FALSE, &Device->Pdo);
      if (!NT_SUCCESS(Status)) {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        ExFreePool(Relations);
        return Status;
      }

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[i] = Device->Pdo;

    i++;

    CurrentEntry = CurrentEntry->Flink;
    }

  Irp->IoStatus.Information = (ULONG)Relations;

  return Status;
}

NTSTATUS
PnpRootQueryDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  NTSTATUS Status;

  DPRINT("Called\n");

  switch (IrpSp->Parameters.QueryDeviceRelations.Type)
    {
  case BusRelations:
    Status = PnpRootQueryBusRelations(DeviceObject, Irp, IrpSp);
    break;

  default:
    Status = STATUS_NOT_IMPLEMENTED;
    }

  return Status;
}

NTSTATUS
STDCALL
PnpRootPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction)
    {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = PnpRootQueryDeviceRelations(DeviceObject, Irp, IrpSp);
    break;
  
  case IRP_MN_START_DEVICE:
    PnpRootDeviceListCount = 0;
    Status = STATUS_SUCCESS;
    break;

  case IRP_MN_STOP_DEVICE:
    /* Root device cannot be stopped */
    Status = STATUS_UNSUCCESSFUL;
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
    }

  if (Status != STATUS_PENDING)
    {
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

NTSTATUS
STDCALL
PnpRootPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction)
  {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING)
    {
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

NTSTATUS
PnpRootAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PDEVICE_OBJECT Ldo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, 0, NULL, FILE_DEVICE_BUS_EXTENDER,
    FILE_DEVICE_SECURE_OPEN, TRUE, &PnpRootDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
      KeBugCheck(0);
      return Status;
    }

  Ldo = IoAttachDeviceToDeviceStack(PnpRootDeviceObject, PhysicalDeviceObject);

  if (!PnpRootDeviceObject)
    {
      DbgPrint("PnpRootDeviceObject 0x%X\n", PnpRootDeviceObject);
      KeBugCheck(0);
    }

  if (!PhysicalDeviceObject)
    {
      DbgPrint("PhysicalDeviceObject 0x%X\n", PhysicalDeviceObject);
      KeBugCheck(0);
    }

  InitializeListHead(&PnpRootDeviceListHead);
  PnpRootDeviceListCount = 0;
  KeInitializeSpinLock(&PnpRootDeviceListLock);

  PnpRootDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Done\n");

  return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DPRINT("Called\n");

  DriverObject->MajorFunction[IRP_MJ_PNP] = PnpRootPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = PnpRootPowerControl;
  DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
