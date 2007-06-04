/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnproot.c
 * PURPOSE:         PnP manager root device
 *
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Copyright 2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

#define ENUM_NAME_ROOT L"Root"

/* DATA **********************************************************************/

typedef struct _PNPROOT_DEVICE
{
  // Entry on device list
  LIST_ENTRY ListEntry;
  // Physical Device Object of device
  PDEVICE_OBJECT Pdo;
  // Device ID
  UNICODE_STRING DeviceID;
  // Instance ID
  UNICODE_STRING InstanceID;
  // Device description
  UNICODE_STRING DeviceDescription;
  // Resource requirement list
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
  ULONG ResourceRequirementsListSize;
  // Associated resource list
  PCM_RESOURCE_LIST ResourceList;
  ULONG ResourceListSize;
} PNPROOT_DEVICE, *PPNPROOT_DEVICE;

typedef enum
{
  dsStopped,
  dsStarted,
  dsPaused,
  dsRemoved,
  dsSurpriseRemoved
} PNPROOT_DEVICE_STATE;

typedef struct _PNPROOT_COMMON_DEVICE_EXTENSION
{
  // Wether this device extension is for an FDO or PDO
  BOOLEAN IsFDO;
} PNPROOT_COMMON_DEVICE_EXTENSION, *PPNPROOT_COMMON_DEVICE_EXTENSION;

/* Physical Device Object device extension for a child device */
typedef struct _PNPROOT_PDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Informations about the device
  PPNPROOT_DEVICE DeviceInfo;
} PNPROOT_PDO_DEVICE_EXTENSION, *PPNPROOT_PDO_DEVICE_EXTENSION;

/* Physical Device Object device extension for the Root bus device object */
typedef struct _PNPROOT_FDO_DEVICE_EXTENSION
{
  // Common device data
  PNPROOT_COMMON_DEVICE_EXTENSION Common;
  // Lower device object
  PDEVICE_OBJECT Ldo;
  // Current state of the driver
  PNPROOT_DEVICE_STATE State;
  // Namespace device list
  LIST_ENTRY DeviceListHead;
  // Number of (not removed) devices in device list
  ULONG DeviceListCount;
  // Lock for namespace device list
  KGUARDED_MUTEX DeviceListLock;
} PNPROOT_FDO_DEVICE_EXTENSION, *PPNPROOT_FDO_DEVICE_EXTENSION;

typedef struct _BUFFER
{
  PVOID *Buffer;
  PULONG Length;
} BUFFER, *PBUFFER;

static PDEVICE_OBJECT PnpRootDeviceObject = NULL;

/* FUNCTIONS *****************************************************************/

static NTSTATUS
LocateChildDevice(
	IN PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension,
	IN PCWSTR DeviceId,
	IN PCWSTR InstanceId,
	OUT PPNPROOT_DEVICE* ChildDevice)
{
	PPNPROOT_DEVICE Device;
	UNICODE_STRING DeviceIdU, InstanceIdU;

	RtlInitUnicodeString(&DeviceIdU, DeviceId);
	RtlInitUnicodeString(&InstanceIdU, InstanceId);

	LIST_FOR_EACH(Device, &DeviceExtension->DeviceListHead, PNPROOT_DEVICE, ListEntry)
	{
		if (RtlEqualUnicodeString(&DeviceIdU, &Device->DeviceID, TRUE)
		 && RtlEqualUnicodeString(&InstanceIdU, &Device->InstanceID, TRUE))
		{
			*ChildDevice = Device;
			return STATUS_SUCCESS;
		}
	}

	return STATUS_NO_SUCH_DEVICE;
}

/* Creates a new PnP device for a legacy driver */
NTSTATUS
PnpRootCreateDevice(
  IN PUNICODE_STRING ServiceName,
  IN PDEVICE_OBJECT *PhysicalDeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  UNICODE_STRING UnknownServiceName = RTL_CONSTANT_STRING(L"UNKNOWN");
  PUNICODE_STRING LocalServiceName;
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  WCHAR DevicePath[MAX_PATH + 1];
  WCHAR InstancePath[5];
  PPNPROOT_DEVICE Device = NULL;
  NTSTATUS Status;
  ULONG i;

  DeviceExtension = PnpRootDeviceObject->DeviceExtension;
  KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);

  if (ServiceName)
    LocalServiceName = ServiceName;
  else
    LocalServiceName = &UnknownServiceName;

  DPRINT("Creating a PnP root device for service '%wZ'\n", LocalServiceName);

  /* Search for a free instance ID */
  _snwprintf(DevicePath, sizeof(DevicePath) / sizeof(WCHAR), L"%s\\LEGACY_%wZ", REGSTR_KEY_ROOTENUM, LocalServiceName);
  for (i = 0; i < 9999; i++)
  {
    _snwprintf(InstancePath, sizeof(InstancePath) / sizeof(WCHAR), L"%04lu", i);
    Status = LocateChildDevice(DeviceExtension, DevicePath, InstancePath, &Device);
    if (Status == STATUS_NO_SUCH_DEVICE)
      break;
  }
  if (i > 9999)
  {
    DPRINT1("Too much legacy devices reported for service '%wZ'\n", &LocalServiceName);
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto cleanup;
  }

  /* FIXME: save the new device to registry? */

  /* Initialize a PNPROOT_DEVICE structure */
  Device = ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
  if (!Device)
  {
    DPRINT("ExAllocatePoolWithTag() failed\n");
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }
  RtlZeroMemory(Device, sizeof(Device));
  if (!RtlCreateUnicodeString(&Device->DeviceID, DevicePath))
  {
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }
  if (!RtlCreateUnicodeString(&Device->InstanceID, InstancePath))
  {
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }

  /* Initialize a device object */
  Status = IoCreateDevice(
    PnpRootDeviceObject->DriverObject,
    sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_CONTROLLER,
    FILE_AUTOGENERATED_DEVICE_NAME,
    FALSE,
    &Device->Pdo);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }

  PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;
  RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));
  PdoDeviceExtension->Common.IsFDO = FALSE;
  PdoDeviceExtension->DeviceInfo = Device;

  Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
  Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

  InsertTailList(
    &DeviceExtension->DeviceListHead,
    &Device->ListEntry);
    DeviceExtension->DeviceListCount++;

  *PhysicalDeviceObject = Device->Pdo;
  DPRINT("Created PDO %p (%wZ\\%wZ)\n", *PhysicalDeviceObject, &Device->DeviceID, &Device->InstanceID);
  Device = NULL;
  Status = STATUS_SUCCESS;

cleanup:
  KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);
  if (Device)
  {
    if (Device->Pdo)
      IoDeleteDevice(Device->Pdo);
    RtlFreeUnicodeString(&Device->DeviceID);
    RtlFreeUnicodeString(&Device->InstanceID);
    ExFreePoolWithTag(Device, TAG_PNP_ROOT);
  }
  return Status;
}

static NTSTATUS STDCALL
ForwardIrpAndWaitCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
	if (Irp->PendingReturned)
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice;
	KEVENT Event;
	NTSTATUS Status;

	ASSERT(((PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->IsFDO);
	LowerDevice = ((PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->Ldo;

	ASSERT(LowerDevice);

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	DPRINT("Calling lower device %p [%wZ]\n", LowerDevice, &LowerDevice->DriverObject->DriverName);
	IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

	Status = IoCallDriver(LowerDevice, Irp);
	if (Status == STATUS_PENDING)
	{
		Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		if (NT_SUCCESS(Status))
			Status = Irp->IoStatus.Status;
	}

	return Status;
}

static NTSTATUS NTAPI
QueryStringCallback(
	IN PWSTR ValueName,
	IN ULONG ValueType,
	IN PVOID ValueData,
	IN ULONG ValueLength,
	IN PVOID Context,
	IN PVOID EntryContext)
{
	PUNICODE_STRING String = (PUNICODE_STRING)EntryContext;

	if (ValueType != REG_SZ || ValueLength == 0 || ValueLength % sizeof(WCHAR) != 0)
		return STATUS_SUCCESS;

	String->Buffer = ExAllocatePoolWithTag(PagedPool, ValueLength, TAG_PNP_ROOT);
	if (String->Buffer == NULL)
		return STATUS_NO_MEMORY;
	String->Length = String->MaximumLength = ValueLength;
	RtlCopyMemory(String->Buffer, ValueData, ValueLength);
	if (ValueLength > 0 && String->Buffer[ValueLength / sizeof(WCHAR) - 1] == L'\0')
		String->Length -= sizeof(WCHAR);
	return STATUS_SUCCESS;
}

static NTSTATUS NTAPI
QueryBinaryValueCallback(
	IN PWSTR ValueName,
	IN ULONG ValueType,
	IN PVOID ValueData,
	IN ULONG ValueLength,
	IN PVOID Context,
	IN PVOID EntryContext)
{
	PBUFFER Buffer = (PBUFFER)EntryContext;
	PVOID BinaryValue;

	if (ValueLength == 0)
	{
		*Buffer->Buffer = NULL;
		return STATUS_SUCCESS;
	}

	BinaryValue = ExAllocatePoolWithTag(PagedPool, ValueLength, TAG_PNP_ROOT);
	if (BinaryValue == NULL)
		return STATUS_NO_MEMORY;
	RtlCopyMemory(BinaryValue, ValueData, ValueLength);
	*Buffer->Buffer = BinaryValue;
	if (Buffer->Length) *Buffer->Length = ValueLength;
	return STATUS_SUCCESS;
}

static NTSTATUS
EnumerateDevices(
	IN PDEVICE_OBJECT DeviceObject)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_BASIC_INFORMATION KeyInfo = NULL, SubKeyInfo = NULL;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\" REGSTR_PATH_SYSTEMENUM "\\" REGSTR_KEY_ROOTENUM);
  UNICODE_STRING SubKeyName;
  WCHAR DevicePath[MAX_PATH + 1];
  RTL_QUERY_REGISTRY_TABLE QueryTable[4];
  PPNPROOT_DEVICE Device = NULL;
  HANDLE KeyHandle = INVALID_HANDLE_VALUE;
  HANDLE SubKeyHandle = INVALID_HANDLE_VALUE;
  HANDLE DeviceKeyHandle = INVALID_HANDLE_VALUE;
  ULONG BufferSize;
  ULONG ResultSize;
  ULONG Index1, Index2;
  BUFFER Buffer1, Buffer2;
  NTSTATUS Status = STATUS_UNSUCCESSFUL;

  DPRINT("EnumerateDevices(FDO %p)\n", DeviceObject);

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);

  BufferSize = sizeof(KEY_BASIC_INFORMATION) + (MAX_PATH + 1) * sizeof(WCHAR);
  KeyInfo = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_PNP_ROOT);
  if (!KeyInfo)
  {
    DPRINT("ExAllocatePoolWithTag() failed\n");
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }
  SubKeyInfo = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_PNP_ROOT);
  if (!SubKeyInfo)
  {
    DPRINT("ExAllocatePoolWithTag() failed\n");
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }

  InitializeObjectAttributes(
    &ObjectAttributes,
    &KeyName,
    OBJ_CASE_INSENSITIVE,
    NULL,
    NULL);

  Status = ZwOpenKey(&KeyHandle, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("ZwOpenKey(%wZ) failed with status 0x%08lx\n", &KeyName, Status);
    goto cleanup;
  }

  /* Devices are sub-sub-keys of 'KeyName'. KeyName is already opened as
   * KeyHandle. We'll first do a first enumeration to have first level keys,
   * and an inner one to have the real devices list.
   */
  Index1 = 0;
  while (TRUE)
  {
    Status = ZwEnumerateKey(
      KeyHandle,
      Index1,
      KeyBasicInformation,
      KeyInfo,
      BufferSize,
      &ResultSize);
    if (Status == STATUS_NO_MORE_ENTRIES)
    {
      Status = STATUS_SUCCESS;
      break;
    }
    else if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
      goto cleanup;
    }

    /* Terminate the string */
    KeyInfo->Name[KeyInfo->NameLength / sizeof(WCHAR)] = 0;

    /* Open the key */
    RtlInitUnicodeString(&SubKeyName, KeyInfo->Name);
    InitializeObjectAttributes(
      &ObjectAttributes,
      &SubKeyName,
      0, /* Attributes */
      KeyHandle,
      NULL); /* Security descriptor */
    Status = ZwOpenKey(&SubKeyHandle, KEY_ENUMERATE_SUB_KEYS, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
      break;
    }

    /* Enumerate the sub-keys */
    Index2 = 0;
    while (TRUE)
    {
      Status = ZwEnumerateKey(
        SubKeyHandle,
        Index2,
        KeyBasicInformation,
        SubKeyInfo,
        BufferSize,
        &ResultSize);
      if (Status == STATUS_NO_MORE_ENTRIES)
        break;
      else if (!NT_SUCCESS(Status))
      {
        DPRINT("ZwEnumerateKey() failed with status 0x%08lx\n", Status);
        break;
      }

      /* Terminate the string */
      SubKeyInfo->Name[SubKeyInfo->NameLength / sizeof(WCHAR)] = 0;

      _snwprintf(
        DevicePath,
        sizeof(DevicePath) / sizeof(WCHAR),
        L"%s\\%s", REGSTR_KEY_ROOTENUM, KeyInfo->Name);
      DPRINT("Found device %S\\%s!\n", DevicePath, SubKeyInfo->Name);
      if (LocateChildDevice(DeviceExtension, DevicePath, SubKeyInfo->Name, &Device) == STATUS_NO_SUCH_DEVICE)
      {
        /* Create a PPNPROOT_DEVICE object, and add if in the list of known devices */
        Device = (PPNPROOT_DEVICE)ExAllocatePoolWithTag(PagedPool, sizeof(PNPROOT_DEVICE), TAG_PNP_ROOT);
        if (!Device)
        {
          DPRINT("ExAllocatePoolWithTag() failed\n");
          Status = STATUS_NO_MEMORY;
          goto cleanup;
        }
        RtlZeroMemory(Device, sizeof(PNPROOT_DEVICE));

        /* Fill device ID and instance ID */
        if (!RtlCreateUnicodeString(&Device->DeviceID, DevicePath))
        {
          DPRINT("RtlCreateUnicodeString() failed\n");
          Status = STATUS_NO_MEMORY;
          goto cleanup;
        }

        if (!RtlCreateUnicodeString(&Device->InstanceID, SubKeyInfo->Name))
        {
          DPRINT("RtlCreateUnicodeString() failed\n");
          Status = STATUS_NO_MEMORY;
          goto cleanup;
        }

        /* Open registry key to fill other informations */
        InitializeObjectAttributes(
          &ObjectAttributes,
          &Device->InstanceID,
          0, /* Attributes */
          SubKeyHandle,
          NULL); /* Security descriptor */
        Status = ZwOpenKey(&DeviceKeyHandle, KEY_READ, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
          DPRINT("ZwOpenKey() failed with status 0x%08lx\n", Status);
          break;
        }

        /* Fill other informations */
        Buffer1.Buffer = (PVOID *)&Device->ResourceRequirementsList;
        Buffer1.Length = NULL;
        Buffer2.Buffer = (PVOID *)&Device->ResourceList;
        Buffer2.Length = &Device->ResourceListSize;
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].QueryRoutine = QueryStringCallback;
        QueryTable[0].Name = L"DeviceDesc";
        QueryTable[0].EntryContext = &Device->DeviceDescription;
        QueryTable[1].Flags = RTL_QUERY_REGISTRY_SUBKEY;
        QueryTable[1].Name = L"LogConf";
        QueryTable[2].QueryRoutine = QueryBinaryValueCallback;
        QueryTable[2].Name = L"BasicConfigVector";
        QueryTable[2].EntryContext = &Buffer1;
        QueryTable[2].QueryRoutine = QueryBinaryValueCallback;
        QueryTable[2].Name = L"BootConfig";
        QueryTable[2].EntryContext = &Buffer2;

        Status = RtlQueryRegistryValues(
          RTL_REGISTRY_HANDLE,
          (PCWSTR)DeviceKeyHandle,
          QueryTable,
          NULL,
          NULL);
        if (!NT_SUCCESS(Status))
        {
          DPRINT("RtlQueryRegistryValues() failed with status 0x%08lx\n", Status);
          break;
        }

        ZwClose(DeviceKeyHandle);
        DeviceKeyHandle = INVALID_HANDLE_VALUE;

        /* Insert the newly created device into the list */
        InsertTailList(
          &DeviceExtension->DeviceListHead,
          &Device->ListEntry);
        DeviceExtension->DeviceListCount++;
        Device = NULL;
      }

      Index2++;
    }

    ZwClose(SubKeyHandle);
    SubKeyHandle = INVALID_HANDLE_VALUE;
    Index1++;
  }

cleanup:
  if (Device)
  {
    /* We have a device that has not been added to device list. We need to clean it up */
    /* FIXME */
    ExFreePoolWithTag(Device, TAG_PNP_ROOT);
  }
  if (DeviceKeyHandle != INVALID_HANDLE_VALUE)
    ZwClose(DeviceKeyHandle);
  if (SubKeyHandle != INVALID_HANDLE_VALUE)
    ZwClose(SubKeyHandle);
  if (KeyHandle != INVALID_HANDLE_VALUE)
    ZwClose(KeyHandle);
  ExFreePoolWithTag(KeyInfo, TAG_PNP_ROOT);
  ExFreePoolWithTag(SubKeyInfo, TAG_PNP_ROOT);
  KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);
  return Status;
}

/* FUNCTION: Handle IRP_MN_QUERY_DEVICE_RELATIONS IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootQueryDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION PdoDeviceExtension;
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations = NULL, OtherRelations = (PDEVICE_RELATIONS)Irp->IoStatus.Information;
  PPNPROOT_DEVICE Device = NULL;
  ULONG Size;
  NTSTATUS Status;

  DPRINT("PnpRootQueryDeviceRelations(FDO %p, Irp %p)\n", DeviceObject, Irp);

  Status = EnumerateDevices(DeviceObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("EnumerateDevices() failed with status 0x%08lx\n", Status);
    return Status;
  }

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) + sizeof(PDEVICE_OBJECT) * DeviceExtension->DeviceListCount;
  if (OtherRelations)
  {
    /* Another bus driver has already created a DEVICE_RELATIONS
     * structure so we must merge this structure with our own */
    
    Size += sizeof(PDEVICE_OBJECT) * OtherRelations->Count;
  }
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
  {
    DPRINT("ExAllocatePoolWithTag() failed\n");
    Status = STATUS_NO_MEMORY;
    goto cleanup;
  }
  RtlZeroMemory(Relations, Size);
  if (OtherRelations)
  {
    Relations->Count = OtherRelations->Count;
    RtlCopyMemory(Relations->Objects, OtherRelations->Objects, sizeof(PDEVICE_OBJECT) * OtherRelations->Count);
  }

  KeAcquireGuardedMutex(&DeviceExtension->DeviceListLock);
  LIST_FOR_EACH(Device, &DeviceExtension->DeviceListHead, PNPROOT_DEVICE, ListEntry)
  {
    if (!Device->Pdo)
    {
      /* Create a physical device object for the
       * device as it does not already have one */
      Status = IoCreateDevice(
        DeviceObject->DriverObject,
        sizeof(PNPROOT_PDO_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_CONTROLLER,
        FILE_AUTOGENERATED_DEVICE_NAME,
        FALSE,
        &Device->Pdo);
      if (!NT_SUCCESS(Status))
      {
        DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
        break;
      }

      PdoDeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;
      RtlZeroMemory(PdoDeviceExtension, sizeof(PNPROOT_PDO_DEVICE_EXTENSION));
      PdoDeviceExtension->Common.IsFDO = FALSE;
      PdoDeviceExtension->DeviceInfo = Device;

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;
      Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[Relations->Count++] = Device->Pdo;
  }
  KeReleaseGuardedMutex(&DeviceExtension->DeviceListLock);

  Irp->IoStatus.Information = (ULONG_PTR)Relations;

cleanup:
  if (!NT_SUCCESS(Status))
  {
    if (OtherRelations)
      ExFreePool(OtherRelations);
    if (Relations)
      ExFreePool(Relations);
    if (Device && Device->Pdo)
    {
      IoDeleteDevice(Device->Pdo);
      Device->Pdo = NULL;
    }
  }

  return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs for the root bus device object
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the root bus driver
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootFdoPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  Status = Irp->IoStatus.Status;
  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction)
  {
    case IRP_MN_QUERY_DEVICE_RELATIONS:
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS\n");
      Status = PnpRootQueryDeviceRelations(DeviceObject, Irp);
      break;

    case IRP_MN_START_DEVICE:
      DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
      Status = ForwardIrpAndWait(DeviceObject, Irp);
      if (NT_SUCCESS(Status))
        DeviceExtension->State = dsStarted;
      Status = STATUS_SUCCESS;
      break;

    case IRP_MN_STOP_DEVICE:
      DPRINT("IRP_MJ_PNP / IRP_MN_STOP_DEVICE\n");
      /* Root device cannot be stopped */
      Status = STATUS_NOT_SUPPORTED;
      break;

    default:
      DPRINT("IRP_MJ_PNP / Unknown minor function 0x%lx\n", IrpSp->MinorFunction);
      Status = STATUS_NOT_IMPLEMENTED;
      break;
  }

  if (Status != STATUS_PENDING)
  {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  return Status;
}

static NTSTATUS
PdoQueryDeviceRelations(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	//PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_RELATIONS Relations;
	DEVICE_RELATION_TYPE RelationType;
	NTSTATUS Status = Irp->IoStatus.Status;

	//DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	RelationType = IrpSp->Parameters.QueryDeviceRelations.Type;

	switch (RelationType)
	{
		/* FIXME: remove */
		case BusRelations:
		{
			if (IoGetAttachedDevice(DeviceObject) != DeviceObject)
			{
				/* We're not alone in the stack */
				PDEVICE_NODE DeviceNode;
				DeviceNode = IopGetDeviceNode(IopGetDeviceNode(DeviceObject)->PhysicalDeviceObject);
				DPRINT1("Device stack for '%wZ' (%wZ) is misbehaving ; shouldn't receive IRP_MN_QUERY_DEVICE_RELATIONS / BusRelations\n",
					&DeviceNode->InstancePath, &DeviceNode->ServiceName);
			}
			break;
		}

		case TargetDeviceRelation:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / TargetDeviceRelation\n");
			Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));
			if (!Relations)
			{
				DPRINT("ExAllocatePoolWithTag() failed\n");
				Status = STATUS_NO_MEMORY;
			}
			else
			{
				ObReferenceObject(DeviceObject);
				Relations->Count = 1;
				Relations->Objects[0] = DeviceObject;
				Status = STATUS_SUCCESS;
				Irp->IoStatus.Information = (ULONG_PTR)Relations;
			}
			break;
		}

		default:
		{
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_RELATIONS / unknown relation type 0x%lx\n", RelationType);
		}
	}

	return Status;
}

static NTSTATUS
PdoQueryCapabilities(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNPROOT_FDO_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_CAPABILITIES DeviceCapabilities;

	DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

	if (DeviceCapabilities->Version != 1)
		return STATUS_REVISION_MISMATCH;

	DeviceCapabilities->UniqueID = TRUE;
	/* FIXME: Fill other fields */

	return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryResources(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
	PCM_RESOURCE_LIST ResourceList;

	DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (DeviceExtension->DeviceInfo->ResourceList == NULL)
	{
		/* Create an empty resource list */
		ResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));
		if (!ResourceList)
			return STATUS_NO_MEMORY;

		ResourceList->Count = 0;

		Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
	}
	else
	{
		/* Copy existing resource requirement list */
		ResourceList = ExAllocatePool(
			PagedPool,
			FIELD_OFFSET(CM_RESOURCE_LIST, List) + DeviceExtension->DeviceInfo->ResourceListSize);
		if (!ResourceList)
			return STATUS_NO_MEMORY;

		ResourceList->Count = 1;
		RtlCopyMemory(
			&ResourceList->List,
			DeviceExtension->DeviceInfo->ResourceList,
			DeviceExtension->DeviceInfo->ResourceListSize);
			Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryResourceRequirements(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
	PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;
	ULONG ResourceListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List);

	DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	if (DeviceExtension->DeviceInfo->ResourceRequirementsList == NULL)
	{
		/* Create an empty resource list */
		ResourceList = ExAllocatePool(PagedPool, ResourceListSize);
		if (!ResourceList)
			return STATUS_NO_MEMORY;

		RtlZeroMemory(ResourceList, ResourceListSize);
		ResourceList->ListSize = ResourceListSize;

		Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
	}
	else
	{
		/* Copy existing resource requirement list */
		ResourceList = ExAllocatePool(PagedPool, DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);
		if (!ResourceList)
			return STATUS_NO_MEMORY;

		RtlCopyMemory(
			ResourceList,
			DeviceExtension->DeviceInfo->ResourceRequirementsList,
			DeviceExtension->DeviceInfo->ResourceRequirementsList->ListSize);
			Irp->IoStatus.Information = (ULONG_PTR)ResourceList;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryDeviceText(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
	DEVICE_TEXT_TYPE DeviceTextType;
	NTSTATUS Status = Irp->IoStatus.Status;

	DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	DeviceTextType = IrpSp->Parameters.QueryDeviceText.DeviceTextType;

	switch (DeviceTextType)
	{
		case DeviceTextDescription:
		{
			UNICODE_STRING String;
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");

			Status = RtlDuplicateUnicodeString(
				RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
				&DeviceExtension->DeviceInfo->DeviceDescription,
				&String);
			Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
			break;
		}

		case DeviceTextLocationInformation:
		{
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextLocationInformation\n");
			Status = STATUS_NOT_SUPPORTED;
			break;
		}

		default:
		{
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown query id type 0x%lx\n", DeviceTextType);
		}
	}

	return Status;
}

static NTSTATUS
PdoQueryId(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
	BUS_QUERY_ID_TYPE IdType;
	NTSTATUS Status = Irp->IoStatus.Status;

	DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	IdType = IrpSp->Parameters.QueryId.IdType;

	switch (IdType)
	{
		case BusQueryDeviceID:
		{
			UNICODE_STRING String;
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");

			Status = RtlDuplicateUnicodeString(
				RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
				&DeviceExtension->DeviceInfo->DeviceID,
				&String);
			Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
			break;
		}

		case BusQueryHardwareIDs:
		case BusQueryCompatibleIDs:
		{
			/* Optional, do nothing */
			break;
		}

		case BusQueryInstanceID:
		{
			UNICODE_STRING String;
			DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");

			Status = RtlDuplicateUnicodeString(
				RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
				&DeviceExtension->DeviceInfo->InstanceID,
				&String);
			Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
			break;
		}

		default:
		{
			DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
		}
	}

	return Status;
}

static NTSTATUS
PdoQueryBusInformation(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PIO_STACK_LOCATION IrpSp)
{
	PPNP_BUS_INFORMATION BusInfo;
	NTSTATUS Status;

	BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), TAG_PNP_ROOT);
	if (!BusInfo)
		Status = STATUS_NO_MEMORY;
	else
	{
		RtlCopyMemory(
			&BusInfo->BusTypeGuid,
			&GUID_BUS_TYPE_INTERNAL,
			sizeof(BusInfo->BusTypeGuid));
		BusInfo->LegacyBusType = PNPBus;
		/* We're the only root bus enumerator on the computer */
		BusInfo->BusNumber = 0;
		Irp->IoStatus.Information = (ULONG_PTR)BusInfo;
		Status = STATUS_SUCCESS;
	}

	return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS
PnpRootPdoPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_PDO_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  Status = Irp->IoStatus.Status;
  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction)
  {
    case IRP_MN_START_DEVICE: /* 0x00 */
      DPRINT("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
      Status = STATUS_SUCCESS;
      break;

    case IRP_MN_QUERY_DEVICE_RELATIONS: /* 0x07 */
      Status = PdoQueryDeviceRelations(DeviceObject, Irp, IrpSp);
      break;

    case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");
      Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
      break;

    case IRP_MN_QUERY_RESOURCES: /* 0x0a */
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
      Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
      break;

   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
      Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
      break;

    case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
      Status = PdoQueryDeviceText(DeviceObject, Irp, IrpSp);
      break;

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS: /* 0x0d */
        DPRINT("IRP_MJ_PNP / IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n");
        break;

    case IRP_MN_QUERY_ID: /* 0x13 */
      Status = PdoQueryId(DeviceObject, Irp, IrpSp);
      break;

    case IRP_MN_QUERY_BUS_INFORMATION: /* 0x15 */
      DPRINT("IRP_MJ_PNP / IRP_MN_QUERY_BUS_INFORMATION\n");
      Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
      break;

    default:
      DPRINT1("IRP_MJ_PNP / Unknown minor function 0x%lx\n", IrpSp->MinorFunction);
      Status = STATUS_NOT_IMPLEMENTED;
      break;
  }

  if (Status != STATUS_PENDING)
  {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  return Status;
}

/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
static NTSTATUS NTAPI
PnpRootPnpControl(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PPNPROOT_COMMON_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DeviceExtension = (PPNPROOT_COMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (DeviceExtension->IsFDO)
    Status = PnpRootFdoPnpControl(DeviceObject, Irp);
  else
    Status = PnpRootPdoPnpControl(DeviceObject, Irp);

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

  DPRINT("PnpRootAddDevice(DriverObject %p, Pdo %p)\n", DriverObject, PhysicalDeviceObject);

  if (!PhysicalDeviceObject)
  {
    DPRINT("PhysicalDeviceObject 0x%p\n", PhysicalDeviceObject);
    Status = STATUS_INSUFFICIENT_RESOURCES;
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }

  Status = IoCreateDevice(
    DriverObject,
    sizeof(PNPROOT_FDO_DEVICE_EXTENSION),
    NULL,
    FILE_DEVICE_BUS_EXTENDER,
    FILE_DEVICE_SECURE_OPEN,
    TRUE,
    &PnpRootDeviceObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("IoCreateDevice() failed with status 0x%08lx\n", Status);
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }
  DPRINT("Created FDO %p\n", PnpRootDeviceObject);

  DeviceExtension = (PPNPROOT_FDO_DEVICE_EXTENSION)PnpRootDeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceExtension, sizeof(PNPROOT_FDO_DEVICE_EXTENSION));

  DeviceExtension->Common.IsFDO = TRUE;
  DeviceExtension->State = dsStopped;
  InitializeListHead(&DeviceExtension->DeviceListHead);
  DeviceExtension->DeviceListCount = 0;
  KeInitializeGuardedMutex(&DeviceExtension->DeviceListLock);

  Status = IoAttachDeviceToDeviceStackSafe(
    PnpRootDeviceObject,
    PhysicalDeviceObject,
    &DeviceExtension->Ldo);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
    KEBUGCHECKEX(PHASE1_INITIALIZATION_FAILED, Status, 0, 0, 0);
  }

  PnpRootDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("Done AddDevice()\n");

  return STATUS_SUCCESS;
}

NTSTATUS NTAPI
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath)
{
  DPRINT("PnpRootDriverEntry(%p %wZ)\n", DriverObject, RegistryPath);

  DriverObject->DriverExtension->AddDevice = PnpRootAddDevice;

  DriverObject->MajorFunction[IRP_MJ_PNP] = PnpRootPnpControl;
  //DriverObject->MajorFunction[IRP_MJ_POWER] = PnpRootPowerControl;

  return STATUS_SUCCESS;
}

/* EOF */
