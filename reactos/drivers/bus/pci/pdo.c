/* $Id: pdo.c,v 1.4 2004/03/14 17:10:43 navaraf Exp $
 *
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include <ddk/ntddk.h>

#include "pcidef.h"
#include "pci.h"

#define NDEBUG
#include <debug.h>

DEFINE_GUID(GUID_BUS_TYPE_PCI, 0xc8ebdfb0L, 0xb510, 0x11d0, 0x80, 0xe5, 0x00, 0xa0, 0xc9, 0x25, 0x42, 0xe3);

/*** PRIVATE *****************************************************************/

NTSTATUS
PdoQueryId(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  UNICODE_STRING String;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

//  Irp->IoStatus.Information = 0;

  Status = STATUS_SUCCESS;

  RtlInitUnicodeString(&String, NULL);

  switch (IrpSp->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
      Status = PciCreateUnicodeString(
        &String,
        DeviceExtension->DeviceID.Buffer,
        PagedPool);

      DPRINT("DeviceID: %S\n", String.Buffer);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryHardwareIDs:
    case BusQueryCompatibleIDs:
      Status = STATUS_NOT_IMPLEMENTED;
      break;

    case BusQueryInstanceID:
      Status = PciCreateUnicodeString(
        &String,
        L"0000",
        PagedPool);

      DPRINT("InstanceID: %S\n", String.Buffer);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryDeviceSerialNumber:
    default:
      Status = STATUS_NOT_IMPLEMENTED;
  }

  return Status;
}


NTSTATUS
PdoQueryBusInformation(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  PFDO_DEVICE_EXTENSION FdoDeviceExtension;
  PPNP_BUS_INFORMATION BusInformation;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceExtension->Fdo->DeviceExtension;
  BusInformation = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
  Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
  if (BusInformation != NULL)
  {
    BusInformation->BusTypeGuid = GUID_BUS_TYPE_PCI;
    BusInformation->LegacyBusType = PCIBus;
    BusInformation->BusNumber = DeviceExtension->BusNumber;

    return STATUS_SUCCESS;
  }

  return STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
PdoQueryCapabilities(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_CAPABILITIES DeviceCapabilities;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

  DeviceCapabilities->Address =
  DeviceCapabilities->UINumber = DeviceExtension->SlotNumber.u.AsULONG;

  return STATUS_SUCCESS;
}


NTSTATUS
PdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

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
#if 0
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    break;

  case IRP_MN_CANCEL_STOP_DEVICE:
    break;

  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    break;

  case IRP_MN_EJECT:
    break;
#endif

  case IRP_MN_QUERY_BUS_INFORMATION:
    Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
    break;

#if 0
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    /* FIXME: Possibly handle for RemovalRelations */
    break;

  case IRP_MN_QUERY_DEVICE_TEXT:
    break;
#endif

  case IRP_MN_QUERY_ID:
    Status = PdoQueryId(DeviceObject, Irp, IrpSp);
    break;

#if 0
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
#endif
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
