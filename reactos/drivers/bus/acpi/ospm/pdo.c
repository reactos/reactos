/* $Id: pdo.c,v 1.1 2001/08/23 17:32:04 chorns Exp $
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      08-08-2001  CSH  Created
 */
#include <acpisys.h>
#include <bm.h>
#include <bn.h>

#define NDEBUG
#include <debug.h>

/*** PRIVATE *****************************************************************/

NTSTATUS
PdoQueryId(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  ACPI_STATUS AcpiStatus;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

//  Irp->IoStatus.Information = 0;

  switch (IrpSp->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
      break;

    case BusQueryHardwareIDs:
    case BusQueryCompatibleIDs:
    case BusQueryInstanceID:
    case BusQueryDeviceSerialNumber:
    default:
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


NTSTATUS
PdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  ACPI_STATUS AcpiStatus;
  NTSTATUS Status;
  ULONG AcpiState;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == DevicePowerState) {
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.Power.State.SystemState) {
    default:
      Status = STATUS_UNSUCCESSFUL;
    }
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
STDCALL
PdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    break;

  case IRP_MN_CANCEL_STOP_DEVICE:
    break;

  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    break;

  case IRP_MN_EJECT:
    break;

  case IRP_MN_QUERY_BUS_INFORMATION:
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    break;

  case IRP_MN_QUERY_DEVICE_RELATIONS:
    /* FIXME: Possibly handle for RemovalRelations */
    break;

  case IRP_MN_QUERY_DEVICE_TEXT:
    break;

  case IRP_MN_QUERY_ID:
    break;

  case IRP_MN_QUERY_PNP_DEVICE_STATE:
    break;

  case IRP_MN_QUERY_REMOVE_DEVICE:
    break;

  case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    break;

  case IRP_MN_QUERY_RESOURCES:
    break;

  case IRP_MN_QUERY_STOP_DEVICE:
    break;

  case IRP_MN_REMOVE_DEVICE:
    break;

  case IRP_MN_SET_LOCK:
    break;

  case IRP_MN_START_DEVICE:
    break;

  case IRP_MN_STOP_DEVICE:
    break;

  case IRP_MN_SURPRISE_REMOVAL:
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

NTSTATUS
STDCALL
PdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_SET_POWER:
    Status = PdoSetPower(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

/* EOF */
