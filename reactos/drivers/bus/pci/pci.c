/* $Id: pci.c,v 1.8 2004/08/16 09:13:00 ekohl Exp $
 *
 * PROJECT:         ReactOS PCI Bus driver
 * FILE:            pci.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include <ddk/ntddk.h>

#include "pcidef.h"
#include "pci.h"

#define NDEBUG
#include <debug.h>


#ifdef  ALLOC_PRAGMA

// Make the initialization routines discardable, so that they 
// don't waste space

#pragma  alloc_text(init, DriverEntry)

#endif  /*  ALLOC_PRAGMA  */

/*** PUBLIC ******************************************************************/


/*** PRIVATE *****************************************************************/

NTSTATUS
STDCALL
PciDispatchDeviceControl(
  IN PDEVICE_OBJECT DeviceObject, 
  IN PIRP Irp) 
{
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called. IRP is at (0x%X)\n", Irp);

  Irp->IoStatus.Information = 0;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
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
PciPnpControl(
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

  DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  DPRINT("IsFDO %d\n", DeviceExtension->IsFDO);

  if (DeviceExtension->IsFDO) {
    Status = FdoPnpControl(DeviceObject, Irp);
  } else {
    Status = PdoPnpControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PciPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PCOMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

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
PciAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_OBJECT Fdo;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(DriverObject, sizeof(FDO_DEVICE_EXTENSION),
    NULL, FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SECURE_OPEN, TRUE, &Fdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    return Status;
  }

  DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;

  RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

  DeviceExtension->Common.IsFDO = TRUE;

  DeviceExtension->Ldo =
    IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);

  DeviceExtension->State = dsStopped;

  Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

  //Fdo->Flags |= DO_POWER_PAGABLE;

  DPRINT("Done AddDevice\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("Peripheral Component Interconnect Bus Driver\n");

  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_PNP] = PciPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = PciPowerControl;
  DriverObject->DriverExtension->AddDevice = PciAddDevice;

  return STATUS_SUCCESS;
}


BOOLEAN
PciCreateUnicodeString(
  PUNICODE_STRING Destination,
  PWSTR Source,
  POOL_TYPE PoolType)
{
  ULONG Length;

  if (!Source)
  {
    RtlInitUnicodeString(Destination, NULL);
    return TRUE;
  }

  Length = (wcslen(Source) + 1) * sizeof(WCHAR);

  Destination->Buffer = ExAllocatePool(PoolType, Length);

  if (Destination->Buffer == NULL)
  {
    return FALSE;
  }

  RtlCopyMemory(Destination->Buffer, Source, Length);

  Destination->MaximumLength = Length;

  Destination->Length = Length - sizeof(WCHAR);

  return TRUE;
}


NTSTATUS
PciDuplicateUnicodeString(
  PUNICODE_STRING Destination,
  PUNICODE_STRING Source,
  POOL_TYPE PoolType)
{
  if (Source == NULL)
  {
    RtlInitUnicodeString(Destination, NULL);
    return STATUS_SUCCESS;
  }

  Destination->Buffer = ExAllocatePool(PoolType, Source->MaximumLength);
  if (Destination->Buffer == NULL)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  Destination->MaximumLength = Source->MaximumLength;
  Destination->Length = Source->Length;
  RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->MaximumLength);

  return STATUS_SUCCESS;
}


BOOLEAN
PciCreateDeviceIDString(PUNICODE_STRING DeviceID,
                        PPCI_DEVICE Device)
{
  WCHAR Buffer[256];

  swprintf(Buffer,
           L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           (Device->PciConfig.u.type0.SubSystemID << 16) +
           Device->PciConfig.u.type0.SubVendorID,
           Device->PciConfig.RevisionID);

  if (!PciCreateUnicodeString(DeviceID, Buffer, PagedPool))
  {
    return FALSE;
  }

  return TRUE;
}


BOOLEAN
PciCreateInstanceIDString(PUNICODE_STRING InstanceID,
                          PPCI_DEVICE Device)
{
#if 0
  WCHAR Buffer[32];
  ULONG Length;
  ULONG Index;

  Index = swprintf(Buffer,
                   L"%lX&%02lX",
                   Device->BusNumber,
                   (Device->SlotNumber.u.bits.DeviceNumber << 3) +
                   Device->SlotNumber.u.bits.FunctionNumber);
  Index++;
  Buffer[Index] = UNICODE_NULL;

  Length = (Index + 1) * sizeof(WCHAR);
  InstanceID->Buffer = ExAllocatePool(PagedPool, Length);
  if (InstanceID->Buffer == NULL)
  {
    return FALSE;
  }

  InstanceID->Length = Length - sizeof(WCHAR);
  InstanceID->MaximumLength = Length;
  RtlCopyMemory(InstanceID->Buffer, Buffer, Length);

  return TRUE;
#endif
  return PciCreateUnicodeString(InstanceID, L"0000", PagedPool);
}


BOOLEAN
PciCreateHardwareIDsString(PUNICODE_STRING HardwareIDs,
                           PPCI_DEVICE Device)
{
  WCHAR Buffer[256];
  ULONG Length;
  ULONG Index;

  Index = 0;
  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           (Device->PciConfig.u.type0.SubSystemID << 16) +
           Device->PciConfig.u.type0.SubVendorID,
           Device->PciConfig.RevisionID);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           (Device->PciConfig.u.type0.SubSystemID << 16) +
           Device->PciConfig.u.type0.SubVendorID);
  Index++;

  Buffer[Index] = UNICODE_NULL;

  Length = (Index + 1) * sizeof(WCHAR);
  HardwareIDs->Buffer = ExAllocatePool(PagedPool, Length);
  if (HardwareIDs->Buffer == NULL)
  {
    return FALSE;
  }

  HardwareIDs->Length = Length - sizeof(WCHAR);
  HardwareIDs->MaximumLength = Length;
  RtlCopyMemory(HardwareIDs->Buffer, Buffer, Length);

  return TRUE;
}


BOOLEAN
PciCreateCompatibleIDsString(PUNICODE_STRING CompatibleIDs,
                             PPCI_DEVICE Device)
{
  WCHAR Buffer[256];
  ULONG Length;
  ULONG Index;

  Index = 0;
  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&DEV_%04X&REV_%02X&CC_%02X%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           Device->PciConfig.RevisionID,
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&DEV_%04X&CC_%02X%02X%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass,
           Device->PciConfig.ProgIf);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&DEV_%04X&CC_%02X%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.DeviceID,
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&CC_%02X%02X%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass,
           Device->PciConfig.ProgIf);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X&CC_%02X%02X",
           Device->PciConfig.VendorID,
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\VEN_%04X",
           Device->PciConfig.VendorID);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\CC_%02X%02X%02X",
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass,
           Device->PciConfig.ProgIf);
  Index++;

  Index += swprintf(&Buffer[Index],
           L"PCI\\CC_%02X%02X",
           Device->PciConfig.BaseClass,
           Device->PciConfig.SubClass);
  Index++;

  Buffer[Index] = UNICODE_NULL;

  Length = (Index + 1) * sizeof(WCHAR);
  CompatibleIDs->Buffer = ExAllocatePool(PagedPool, Length);
  if (CompatibleIDs->Buffer == NULL)
  {
    return FALSE;
  }

  CompatibleIDs->Length = Length - sizeof(WCHAR);
  CompatibleIDs->MaximumLength = Length;
  RtlCopyMemory(CompatibleIDs->Buffer, Buffer, Length);

  return TRUE;
}


BOOLEAN
PciCreateDeviceDescriptionString(PUNICODE_STRING DeviceDescription,
                                 PPCI_DEVICE Device)
{
  PWSTR Description;
  ULONG Length;

  switch (Device->PciConfig.BaseClass)
  {
    case PCI_CLASS_MASS_STORAGE_CTLR:
      switch (Device->PciConfig.BaseClass)
      {
        case PCI_SUBCLASS_MSC_SCSI_BUS_CTLR:
          Description = L"SCSI controller";
          break;

        case PCI_SUBCLASS_MSC_IDE_CTLR:
          Description = L"IDE controller";
          break;

        case PCI_SUBCLASS_MSC_FLOPPY_CTLR:
          Description = L"Floppy disk controller";
          break;

        case PCI_SUBCLASS_MSC_IPI_CTLR:
          Description = L"IPI controller";
          break;

        case PCI_SUBCLASS_MSC_RAID_CTLR:
          Description = L"RAID controller";
          break;

        default:
          Description = L"Mass storage controller";
      }
      break;

    case PCI_CLASS_NETWORK_CTLR:
      switch (Device->PciConfig.BaseClass)
      {

        default:
          Description = L"Network controller";
      }
      break;

    default:
      Description = L"PCI-Device";
  }

  Length = (wcslen(Description) + 1) * sizeof(WCHAR);
  DeviceDescription->Buffer = ExAllocatePool(PagedPool, Length);
  if (DeviceDescription->Buffer == NULL)
  {
    return FALSE;
  }

  DeviceDescription->Length = Length - sizeof(WCHAR);
  DeviceDescription->MaximumLength = Length;
  RtlCopyMemory(DeviceDescription->Buffer, Description, Length);

  return TRUE;
}


BOOLEAN
PciCreateDeviceLocationString(PUNICODE_STRING DeviceLocation,
                              PPCI_DEVICE Device)
{
  WCHAR Buffer[256];
  ULONG Length;
  ULONG Index;

  Index = 0;
  Index += swprintf(&Buffer[Index],
                    L"PCI-Bus %lu, Device %u, Function %u",
                    Device->BusNumber,
                    Device->SlotNumber.u.bits.DeviceNumber,
                    Device->SlotNumber.u.bits.FunctionNumber);
  Index++;

  Buffer[Index] = UNICODE_NULL;

  Length = (Index + 1) * sizeof(WCHAR);
  DeviceLocation->Buffer = ExAllocatePool(PagedPool, Length);
  if (DeviceLocation->Buffer == NULL)
  {
    return FALSE;
  }

  DeviceLocation->Length = Length - sizeof(WCHAR);
  DeviceLocation->MaximumLength = Length;
  RtlCopyMemory(DeviceLocation->Buffer, Buffer, Length);

  return TRUE;
}

/* EOF */
