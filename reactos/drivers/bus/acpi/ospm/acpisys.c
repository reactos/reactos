/* $Id: acpisys.c,v 1.4 2001/08/27 01:24:36 ekohl Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/acpisys.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      01-05-2001  CSH  Created
 */
#include <acpisys.h>
#include <bm.h>
#include <bn.h>

#define NDEBUG
#include <debug.h>

#ifdef  ALLOC_PRAGMA

// Make the initialization routines discardable, so that they 
// don't waste space

#pragma  alloc_text(init, DriverEntry)

#endif  /*  ALLOC_PRAGMA  */


NTSTATUS
STDCALL
ACPIDispatchDeviceControl(
  IN PDEVICE_OBJECT DeviceObject, 
  IN PIRP Irp) 
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called. IRP is at (0x%X)\n", Irp);

  Irp->IoStatus.Information = 0;

  IrpSp  = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;

    DPRINT("Completing IRP at 0x%X\n", Irp);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
STDCALL
ACPIPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PCOMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO) {
    Status = FdoPnpControl(DeviceObject, Irp);
  } else {
    Status = FdoPnpControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
ACPIPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PCOMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO) {
    Status = FdoPowerControl(DeviceObject, Irp);
  } else {
    Status = PdoPowerControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
ACPIAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, sizeof(FDO_DEVICE_EXTENSION),
    NULL, FILE_DEVICE_ACPI, FILE_DEVICE_SECURE_OPEN, TRUE, &Fdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;

  DeviceExtension->Pdo = PhysicalDeviceObject;

  DeviceExtension->Ldo =
    IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

  DeviceExtension->State = dsStopped;

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Done AddDevice\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("Advanced Configuration and Power Interface Bus Driver\n");

  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ACPIDispatchDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = ACPIPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = ACPIPowerControl;
  DriverObject->DriverExtension->AddDevice = ACPIAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
