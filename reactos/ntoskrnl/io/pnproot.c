/* $Id: pnproot.c,v 1.13 2003/07/11 01:23:14 royce Exp $
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
#include <reactos/bugcodes.h>
#include <internal/io.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ENUM_NAME_ROOT L"Root"

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE {
  // Entry on device list
  LIST_ENTRY ListEntry;
  // Physical Device Object of device
  PDEVICE_OBJECT Pdo;
  // Service name
  UNICODE_STRING ServiceName;
  // Device ID
  UNICODE_STRING DeviceID;
  // Instance ID
  UNICODE_STRING InstanceID;
  // Device description
  UNICODE_STRING DeviceDescription;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

typedef enum {
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} PNPROOT_DEVICE_STATE;


typedef struct _PNPROOT_COMMON_DEVICE_EXTENSION
{
  // Pointer to device object, this device extension is associated with
  PDEVICE_OBJECT DeviceObject;
  // Wether this device extension is for an FDO or PDO
  BOOLEAN IsFDO;
  // Wether the device is removed
  BOOLEAN Removed;
  // Current device power state for the device
  DEVICE_POWER_STATE DevicePowerState;
} __attribute((packed)) PNPROOT_COMMON_DEVICE_EXTENSION, *PPNPROOT_COMMON_DEVICE_EXTENSION;

/* Physical Device Object device extension for a child device */
typedef struct _PNPROOT_PDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Device ID
  UNICODE_STRING DeviceID;
  // Instance ID
  UNICODE_STRING InstanceID;
} __attribute((packed)) PNPROOT_PDO_DEVICE_EXTENSION, *PPNPROOT_PDO_DEVICE_EXTENSION;

/* Functional Device Object device extension for the PCI driver device object */
typedef struct _PNPROOT_FDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Physical Device Object
  PDEVICE_OBJECT Pdo;
  // Lower device object
  PDEVICE_OBJECT Ldo;
  // Current state of the driver
  PNPROOT_DEVICE_STATE State;
  // Namespace device list
  LIST_ENTRY DeviceListHead;
  // Number of (not removed) devices in device list
  ULONG DeviceListCount;
  // Lock for namespace device list
  // FIXME: Use fast mutex instead?
  KSPIN_LOCK DeviceListLock;
} __attribute((packed)) PNPROOT_FDO_DEVICE_EXTENSION, *PPNPROOT_FDO_DEVICE_EXTENSION;


PDEVICE_OBJECT PnpRootDeviceObject;


/* FUNCTIONS *****************************************************************/

/* Physical Device Object routines */

NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject)
{
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;

  /* This function should be obsoleted soon */

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;

  Device = (PPNPROOT_DEVICE)ExAllocatePool(PagedPool, sizeof(PNPROOT_DEVICE));
  if (!Device)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

  Status = IoCreateDevice(
    PnpRootDeviceObject->DriverObject,
    sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_CONTROLLER,
    0,
    FALSE,
    &Device->Pdo);
  if (!NT_SUCCESS(Status)) {
    DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    ExFreePool(Device);
    return Status;
  }

  Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

  Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

  //Device->Pdo->Flags |= DO_POWER_PAGABLE;

  PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

  RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));

  PdoDeviceExtension->Common.IsFDO = FALSE;

  PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

  PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

  if (!IopCreateUnicodeString(
    &PdoDeviceExtension->DeviceID,
    ENUM_NAME_ROOT \
    L"\\LEGACY_UNKNOWN",
    PagedPool))
  {
    /* FIXME: */
    DPRINT("IopCreateUnicodeString() failed\n");
  }

  if (!IopCreateUnicodeString(
    &PdoDeviceExtension->InstanceID,
    L"0000",
    PagedPool))
  {
    /* FIXME: */
    DPRINT("IopCreateUnicodeString() failed\n");
  }

  ExInterlockedInsertTailList(
    &DeviceExtension->DeviceListHead,
    &Device->ListEntry,
    &DeviceExtension->DeviceListLock);

  DeviceExtension->DeviceListCount++;

  *PhysicalDeviceObject = Device->Pdo;

  return STATUS_SUCCESS;
}


NTSTATUS
PdoQueryId(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
  UNICODE_STRING String;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

//  Irp->IoStatus.Information = 0;

  Status = STATUS_SUCCESS;

  RtlInitUnicodeString(&String, NULL);

  switch (IrpSp->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
      Status = IopCreateUnicodeString(
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
      Status = IopCreateUnicodeString(
        &String,
        DeviceExtension->InstanceID.Buffer,
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
PnpRootPdoPnpControl(
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

  case IRP_MN_QUERY_BUS_INFORMATION:
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    break;

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

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

NTSTATUS
PnpRootPdoPowerControl(
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

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    break;
  }

  Irp->IoStatus.Status = Status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


/* Functional Device Object routines */

NTSTATUS
PnpRootFdoReadDeviceInfo(
  PPNPROOT_DEVICE Device)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  PUNICODE_STRING DeviceDesc;
  WCHAR KeyName[MAX_PATH];
  HANDLE KeyHandle;
  NTSTATUS Status;

  DPRINT("Called\n");

  /* Retrieve configuration from Enum key */

  DeviceDesc = &Device->DeviceDescription;

  wcscpy(KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
  wcscat(KeyName, ENUM_NAME_ROOT);
  wcscat(KeyName, L"\\");
  wcscat(KeyName, Device->ServiceName.Buffer);
  wcscat(KeyName, L"\\");
  wcscat(KeyName, Device->InstanceID.Buffer);

  DPRINT("KeyName %S\n", KeyName);

  Status = RtlpGetRegistryHandle(
    RTL_REGISTRY_ABSOLUTE,
	  KeyName,
		FALSE,
		&KeyHandle);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("RtlpGetRegistryHandle() failed (Status %x)\n", Status);
    return Status;
  }

  RtlZeroMemory(QueryTable, sizeof(QueryTable));

  RtlInitUnicodeString(DeviceDesc, NULL);

  QueryTable[0].Name = L"DeviceDesc";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = DeviceDesc;

  Status = RtlQueryRegistryValues(
    RTL_REGISTRY_HANDLE,
	 	(PWSTR)KeyHandle,
	 	QueryTable,
	 	NULL,
	 	NULL);

  NtClose(KeyHandle);

  DPRINT("RtlQueryRegistryValues() returned status %x\n", Status);

  if (!NT_SUCCESS(Status))
  {
    /* FIXME: */
  }

  DPRINT("Got device description: %S\n", DeviceDesc->Buffer);

  return STATUS_SUCCESS;
}


NTSTATUS
PnpRootFdoEnumerateDevices(
  PDEVICE_OBJECT DeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_BASIC_INFORMATION KeyInfo;
  UNICODE_STRING KeyName;
  PPNPROOT_DEVICE Device;
  WCHAR Buffer[MAX_PATH];
  HANDLE KeyHandle;
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  ULONG Index;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  BufferSize = sizeof(KEY_BASIC_INFORMATION) + (MAX_PATH+1) * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(PagedPool, BufferSize);
  if (!KeyInfo)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlInitUnicodeStringFromLiteral(
    &KeyName,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\" \
    ENUM_NAME_ROOT);

  InitializeObjectAttributes(
    &ObjectAttributes,
		&KeyName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

  Status = NtOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("NtOpenKey() failed (Status %x)\n", Status);
    ExFreePool(KeyInfo);
    return Status;
  }

  /* FIXME: Disabled due to still using the old method of auto loading drivers e.g.
            there are more entries in the list than found in the registry as some 
            drivers are passed on the command line */
//  DeviceExtension->DeviceListCount = 0;

  Index = 0;
  do {
    Status = ZwEnumerateKey(
      KeyHandle,
      Index,
      KeyBasicInformation,
      KeyInfo,
      BufferSize,
      &ResultSize);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwEnumerateKey() (Status %x)\n", Status);
      break;
    }

    /* Terminate the string */
    KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = 0;

    Device = (PPNPROOT_DEVICE)ExAllocatePool(PagedPool, sizeof(PNPROOT_DEVICE));
    if (!Device)
    {
      /* FIXME: */
      break;
    }

    RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

    if (!IopCreateUnicodeString(&Device->ServiceName, KeyInfo->Name, PagedPool))
    {
      /* FIXME: */
      DPRINT("IopCreateUnicodeString() failed\n");
    }

    wcscpy(Buffer, ENUM_NAME_ROOT);
    wcscat(Buffer, L"\\");
    wcscat(Buffer, KeyInfo->Name);

    if (!IopCreateUnicodeString(&Device->DeviceID, Buffer, PagedPool))
    {
      /* FIXME: */
      DPRINT("IopCreateUnicodeString() failed\n");
    }

    DPRINT("Got entry: %S\n", Device->DeviceID.Buffer);

    if (!IopCreateUnicodeString(
      &Device->InstanceID,
      L"0000",
      PagedPool))
    {
      /* FIXME: */
      DPRINT("IopCreateUnicodeString() failed\n");
    }

    Status = PnpRootFdoReadDeviceInfo(Device);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("PnpRootFdoReadDeviceInfo() failed with status %x\n", Status);
      /* FIXME: */      
    }

    ExInterlockedInsertTailList(
      &DeviceExtension->DeviceListHead,
      &Device->ListEntry,
      &DeviceExtension->DeviceListLock);

    DeviceExtension->DeviceListCount++;

    Index++;
  } while (TRUE);

  DPRINT("Entries found: %d\n", Index);

  NtClose(KeyHandle);

  ExFreePool(KeyInfo);

  return STATUS_SUCCESS;
}


NTSTATUS
PnpRootQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  IN PIO_STACK_LOCATION IrpSp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  PPNPROOT_DEVICE Device;
  NTSTATUS Status;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  Status = PnpRootFdoEnumerateDevices(DeviceObject);
  if (!NT_SUCCESS(Status))
    return Status;

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (Irp->IoStatus.Information)
  {
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
  while (CurrentEntry != &DeviceExtension->DeviceListHead)
  {
    Device = CONTAINING_RECORD(CurrentEntry, PNPROOT_DEVICE, ListEntry);

    if (!Device->Pdo)
    {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(
        DeviceObject->DriverObject,
        sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_CONTROLLER,
        0,
        FALSE,
        &Device->Pdo);
      if (!NT_SUCCESS(Status))
      {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        ExFreePool(Relations);
        return Status;
      }

      DPRINT("Created PDO %x\n", Device->Pdo);

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

      //Device->Pdo->Flags |= DO_POWER_PAGABLE;

      PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

      RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));

      PdoDeviceExtension->Common.IsFDO = FALSE;

      PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

      PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

      if (!IopCreateUnicodeString(
        &PdoDeviceExtension->DeviceID,
        Device->DeviceID.Buffer,
        PagedPool))
      {
        DPRINT("Insufficient resources\n");
        /* FIXME: */
      }

      DPRINT("DeviceID: %S  PDO %x\n",
        PdoDeviceExtension->DeviceID.Buffer,
        Device->Pdo);

      if (!IopCreateUnicodeString(
        &PdoDeviceExtension->InstanceID,
        Device->InstanceID.Buffer,
        PagedPool))
      {
        DPRINT("Insufficient resources\n");
        /* FIXME: */
      }

    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[i] = Device->Pdo;

    i++;

    CurrentEntry = CurrentEntry->Flink;
  }

  if (NT_SUCCESS(Status))
  {
    Irp->IoStatus.Information = (ULONG_PTR)Relations;
  }
  else
  {
    Irp->IoStatus.Information = 0;
  }

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

  switch (IrpSp->Parameters.QueryDeviceRelations.Type) {
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
PnpRootFdoPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Status = Irp->IoStatus.Status;

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = PnpRootQueryDeviceRelations(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_START_DEVICE:
    DeviceExtension->State = dsStarted;
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

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}


NTSTATUS
STDCALL
PnpRootFdoPowerControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
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


NTSTATUS
STDCALL
PnpRootPnpControl(
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
  PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  DPRINT("DeviceObject %x  DeviceExtension %x  IsFDO %d\n",
    DeviceObject,
    DeviceExtension,
    DeviceExtension->IsFDO);

  if (DeviceExtension->IsFDO) {
    Status = PnpRootFdoPnpControl(DeviceObject, Irp);
  } else {
    Status = PnpRootPdoPnpControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PnpRootPowerControl(
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
  PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO) {
    Status = PnpRootFdoPowerControl(DeviceObject, Irp);
  } else {
    Status = PnpRootPdoPowerControl(DeviceObject, Irp);
  }

  return Status;
}


NTSTATUS
STDCALL
PnpRootAddDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  Status = IoCreateDevice(
    DriverObject,
    sizeof(PNPROOT_FDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_BUS_EXTENDER,
    FILE_DEVICE_SECURE_OPEN,
    TRUE,
    &PnpRootDeviceObject);
  if (!NT_SUCCESS(Status)) {
    CPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
    KeBugCheck(PHASE1_INITIALIZATION_FAILED);
  }

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;

  RtlZeroMemory(DeviceExtension, sizeof(PNPROOT_FDO_DEVICE_EXTENSION));

  DeviceExtension->Common.IsFDO = TRUE;

  DeviceExtension->State = dsStopped;

  DeviceExtension->Ldo = IoAttachDeviceToDeviceStack(
    PnpRootDeviceObject,
    PhysicalDeviceObject);

  if (!PnpRootDeviceObject) {
    CPRINT("PnpRootDeviceObject 0x%X\n", PnpRootDeviceObject);
    KeBugCheck(PHASE1_INITIALIZATION_FAILED);
  }

  if (!PhysicalDeviceObject) {
    CPRINT("PhysicalDeviceObject 0x%X\n", PhysicalDeviceObject);
    KeBugCheck(PHASE1_INITIALIZATION_FAILED);
  }

  InitializeListHead(&DeviceExtension->DeviceListHead);

  DeviceExtension->DeviceListCount = 0;

  KeInitializeSpinLock(&DeviceExtension->DeviceListLock);

  PnpRootDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  //PnpRootDeviceObject->Flags |= DO_POWER_PAGABLE;

  DPRINT("Done AddDevice()\n");

  return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DPRINT("Called\n");

  DriverObject->MajorFunction[IRP_MJ_PNP] = (PDRIVER_DISPATCH) PnpRootPnpControl;
  DriverObject->MajorFunction[IRP_MJ_POWER] = (PDRIVER_DISPATCH) PnpRootPowerControl;
  DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

  return STATUS_SUCCESS;
}

/* EOF */
