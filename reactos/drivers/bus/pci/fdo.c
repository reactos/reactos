/* $Id: fdo.c,v 1.2 2002/05/05 14:57:45 chorns Exp $
 *
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            fdo.c
 * PURPOSE:         PCI device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */
#include <pci.h>

#define NDEBUG
#include <debug.h>

/*** PRIVATE *****************************************************************/

NTSTATUS
FdoLocateChildDevice(
  PPCI_DEVICE *Device,
  PFDO_DEVICE_EXTENSION DeviceExtension,
  PPCI_COMMON_CONFIG PciConfig)
{
  PLIST_ENTRY CurrentEntry;
  PPCI_DEVICE CurrentDevice;

  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead) {
    CurrentDevice = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);

    /* If both vendor ID and device ID match, it is the same device */
    if ((PciConfig->VendorID == CurrentDevice->PciConfig.VendorID) &&
      (PciConfig->DeviceID == CurrentDevice->PciConfig.DeviceID)) {
      *Device = CurrentDevice;
      return STATUS_SUCCESS;
    }

    CurrentEntry = CurrentEntry->Flink;
  }

  *Device = NULL;
  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
FdoEnumerateDevices(
  PDEVICE_OBJECT DeviceObject)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PCI_COMMON_CONFIG PciConfig;
  PLIST_ENTRY CurrentEntry;
  PPCI_DEVICE Device;
  NTSTATUS Status;
  ULONG Slot;
  ULONG Size;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  /* Mark all devices to be removed. If we don't discover them again during
     enumeration, assume that they have been surprise removed */
  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead) {
    Device = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);
    Device->RemovePending = TRUE;
    CurrentEntry = CurrentEntry->Flink;
  }

  DeviceExtension->DeviceListCount = 0;

  /* Enumerate devices on the PCI bus */
  for (Slot = 0; Slot < 256; Slot++)
	{
	  Size = PciGetBusData(
      DeviceExtension->BusNumber,
			Slot,
			&PciConfig,
      0,
			sizeof(PCI_COMMON_CONFIG));
	  if (Size != 0)
    {
      DPRINT("Bus %1lu  Device %2lu  Func %1lu  VenID 0x%04hx  DevID 0x%04hx\n",
		  	DeviceExtension->BusNumber,
		  	Slot>>3,
		  	Slot & 0x07,
		  	PciConfig.VendorID,
		  	PciConfig.DeviceID);

      Status = FdoLocateChildDevice(&Device, DeviceExtension, &PciConfig);
      if (!NT_SUCCESS(Status)) {
        Device = (PPCI_DEVICE)ExAllocatePool(PagedPool, sizeof(PCI_DEVICE));
        if (!Device)
        {
          /* FIXME: Cleanup resources for already discovered devices */
          return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(Device, sizeof(PCI_DEVICE));

        RtlMoveMemory(&Device->PciConfig, &PciConfig, sizeof(PCI_COMMON_CONFIG));

        ExInterlockedInsertTailList(
          &DeviceExtension->DeviceListHead,
          &Device->ListEntry,
          &DeviceExtension->DeviceListLock);
      }

      /* Don't remove this device */
      Device->RemovePending = FALSE;

      DeviceExtension->DeviceListCount++;
    }
	}

  return STATUS_SUCCESS;
}


NTSTATUS
FdoQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION PdoDeviceExtension;
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  PPCI_DEVICE Device;
  NTSTATUS Status;
  BOOLEAN ErrorOccurred;
  NTSTATUS ErrorStatus;
  WCHAR Buffer[MAX_PATH];
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;

  Status = STATUS_SUCCESS;

  ErrorOccurred = FALSE;

  FdoEnumerateDevices(DeviceObject);

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (Irp->IoStatus.Information) {
    /* FIXME: Another bus driver has already created a DEVICE_RELATIONS 
              structure so we must merge this structure with our own */
  }

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (DeviceExtension->DeviceListCount - 1);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = DeviceExtension->DeviceListCount;

  i = 0;
  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead) {
    Device = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);

    PdoDeviceExtension = NULL;

    if (!Device->Pdo) {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(
        DeviceObject->DriverObject,
        sizeof(PDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_CONTROLLER,
        0,
        FALSE,
        &Device->Pdo);
      if (!NT_SUCCESS(Status)) {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        ErrorStatus = Status;
        ErrorOccurred = TRUE;
        break;
      }

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

      //Device->Pdo->Flags |= DO_POWER_PAGABLE;

      PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

      RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

      PdoDeviceExtension->Common.IsFDO = FALSE;

      PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

      PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

      /* FIXME: Get device properties (Hardware IDs, etc.) */

      swprintf(
        Buffer,
        L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
        Device->PciConfig.VendorID,
        Device->PciConfig.DeviceID,
        Device->PciConfig.u.type0.SubSystemID,
        Device->PciConfig.RevisionID);

      if (!PciCreateUnicodeString(
        &PdoDeviceExtension->DeviceID,
        Buffer,
        PagedPool)) {
        ErrorOccurred = TRUE;
        break;
      }

      DPRINT("DeviceID: %S\n", PdoDeviceExtension->DeviceID.Buffer);
    }

    if (!Device->RemovePending) {
      /* Reference the physical device object. The PnP manager
         will dereference it again when it is no longer needed */
      ObReferenceObject(Device->Pdo);

      Relations->Objects[i] = Device->Pdo;

      i++;
    }

    CurrentEntry = CurrentEntry->Flink;
  }

  if (ErrorOccurred) {
    /* FIXME: Cleanup all new PDOs created in this call. Please give me SEH!!! ;-) */
    /* FIXME: Should IoAttachDeviceToDeviceStack() be undone? */
    if (PdoDeviceExtension) {
      RtlFreeUnicodeString(&PdoDeviceExtension->DeviceID);
      ExFreePool(PdoDeviceExtension);
    }

    ExFreePool(Relations);
    return ErrorStatus;
  }

  Irp->IoStatus.Information = (ULONG_PTR)Relations;

  return Status;
}


NTSTATUS
FdoStartDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  assert(DeviceExtension->State == dsStopped);

  InitializeListHead(&DeviceExtension->DeviceListHead);
  KeInitializeSpinLock(&DeviceExtension->DeviceListLock);
  DeviceExtension->DeviceListCount = 0;

  PciBusConfigType = PciGetBusConfigType();

  DPRINT("Bus configuration is %d\n", PciBusConfigType);

  if (PciBusConfigType != pbtUnknown) {
    /* At least one PCI bus is found */
  }

  /* FIXME: Find a way to get this information */
  DeviceExtension->BusNumber = 0;

  DeviceExtension->State = dsStarted;

  //Irp->IoStatus.Information = 0;

	return STATUS_SUCCESS;
}


NTSTATUS
FdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == DevicePowerState) {
    /* FIXME: Set device power state for the device */
    Status = STATUS_UNSUCCESSFUL;
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
FdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the PCI device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the PCI driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  switch (IrpSp->MinorFunction) {
#if 0
  case IRP_MN_CANCEL_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_CANCEL_STOP_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
    Status = STATUS_NOT_IMPLEMENTED;
    break;
#endif
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = FdoQueryBusRelations(DeviceObject, Irp, IrpSp);
    break;
#if 0
  case IRP_MN_QUERY_PNP_DEVICE_STATE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_QUERY_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_QUERY_STOP_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;

  case IRP_MN_REMOVE_DEVICE:
    Status = STATUS_NOT_IMPLEMENTED;
    break;
#endif
  case IRP_MN_START_DEVICE:
    DPRINT("IRP_MN_START_DEVICE received\n");
    Status = FdoStartDevice(DeviceObject, Irp);
    break;
  case IRP_MN_STOP_DEVICE:
    /* Currently not supported */
    Status = STATUS_UNSUCCESSFUL;
    break;
#if 0
  case IRP_MN_SURPRISE_REMOVAL:
    Status = STATUS_NOT_IMPLEMENTED;
    break;
#endif
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);

    /*
     * Do NOT complete the IRP as it will be processed by the lower
     * device object, which will complete the IRP
     */
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->Ldo, Irp);
    return Status;
    break;
  }


  if (Status != STATUS_PENDING) {
    if (Status != STATUS_NOT_IMPLEMENTED)
      Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
FdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the PCI device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the PCI driver
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
    Status = FdoSetPower(DeviceObject, Irp, IrpSp);
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
