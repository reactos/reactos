/* $Id: pnpmgr.c,v 1.3 2001/09/01 15:36:44 chorns Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/pnpmgr.c
 * PURPOSE:        Initializes the PnP manager
 * PROGRAMMER:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *  16/04/2001 CSH Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/module.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

PDEVICE_NODE IopRootDeviceNode;
KSPIN_LOCK IopDeviceTreeLock;

/* DATA **********************************************************************/

PDRIVER_OBJECT IopRootDriverObject;

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
IoInitializeRemoveLockEx(
  IN PIO_REMOVE_LOCK Lock,
  IN ULONG AllocateTag,
  IN ULONG MaxLockedMinutes,
  IN ULONG HighWatermark,
  IN ULONG RemlockSize)
{
}

NTSTATUS
STDCALL
IoAcquireRemoveLockEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN OPTIONAL PVOID Tag,
  IN LPCSTR File,
  IN ULONG Line,
  IN ULONG RemlockSize)
{
  return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
IoReleaseRemoveLockEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN PVOID Tag,
  IN ULONG RemlockSize)
{
}

VOID
STDCALL
IoReleaseRemoveLockAndWaitEx(
  IN PIO_REMOVE_LOCK RemoveLock,
  IN PVOID Tag,
  IN ULONG RemlockSize)
{
}

VOID
STDCALL
IoAdjustPagingPathCount(
  IN PLONG Count,
  IN BOOLEAN Increment)
{
}

NTSTATUS
STDCALL
IoGetDeviceInterfaceAlias(
  IN PUNICODE_STRING SymbolicLinkName,
  IN CONST GUID *AliasInterfaceClassGuid,
  OUT PUNICODE_STRING AliasSymbolicLinkName)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoGetDeviceInterfaces(
  IN CONST GUID *InterfaceClassGuid,
  IN PDEVICE_OBJECT PhysicalDeviceObject  OPTIONAL,
  IN ULONG Flags,
  OUT PWSTR *SymbolicLinkList)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoGetDeviceProperty(
  IN PDEVICE_OBJECT DeviceObject,
  IN DEVICE_REGISTRY_PROPERTY DeviceProperty,
  IN ULONG BufferLength,
  OUT PVOID PropertyBuffer,
  OUT PULONG ResultLength)
{
  return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
IoInvalidateDeviceRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN DEVICE_RELATION_TYPE Type)
{
}

VOID
STDCALL
IoInvalidateDeviceState(
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
}

NTSTATUS
STDCALL
IoOpenDeviceInterfaceRegistryKey(
  IN PUNICODE_STRING SymbolicLinkName,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE DeviceInterfaceKey)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoOpenDeviceRegistryKey(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG DevInstKeyType,
  IN ACCESS_MASK DesiredAccess,
  OUT PHANDLE DevInstRegKey)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoRegisterDeviceInterface(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN CONST GUID *InterfaceClassGuid,
  IN PUNICODE_STRING ReferenceString  OPTIONAL,
  OUT PUNICODE_STRING SymbolicLinkName)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoRegisterPlugPlayNotification(
  IN IO_NOTIFICATION_EVENT_CATEGORY EventCategory,
  IN ULONG EventCategoryFlags,
  IN PVOID EventCategoryData  OPTIONAL,
  IN PDRIVER_OBJECT DriverObject,
  IN PDRIVER_NOTIFICATION_CALLBACK_ROUTINE CallbackRoutine,
  IN PVOID Context,
  OUT PVOID *NotificationEntry)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoReportDetectedDevice(
  IN PDRIVER_OBJECT DriverObject,
  IN INTERFACE_TYPE LegacyBusType,
  IN ULONG BusNumber,
  IN ULONG SlotNumber,
  IN PCM_RESOURCE_LIST ResourceList,
  IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements  OPTIONAL,
  IN BOOLEAN ResourceAssigned,
  IN OUT PDEVICE_OBJECT *DeviceObject)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoReportResourceForDetection(
  IN PDRIVER_OBJECT DriverObject,
  IN PCM_RESOURCE_LIST DriverList   OPTIONAL,
  IN ULONG DriverListSize    OPTIONAL,
  IN PDEVICE_OBJECT DeviceObject    OPTIONAL,
  IN PCM_RESOURCE_LIST DeviceList   OPTIONAL,
  IN ULONG DeviceListSize   OPTIONAL,
  OUT PBOOLEAN ConflictDetected)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoReportTargetDeviceChange(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PVOID NotificationStructure)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoReportTargetDeviceChangeAsynchronous(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PVOID NotificationStructure,
  IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback  OPTIONAL,
  IN PVOID Context  OPTIONAL)
{
  return STATUS_NOT_IMPLEMENTED;
}

VOID
STDCALL
IoRequestDeviceEject(
  IN PDEVICE_OBJECT PhysicalDeviceObject)
{
}

NTSTATUS
STDCALL
IoSetDeviceInterfaceState(
  IN PUNICODE_STRING SymbolicLinkName,
  IN BOOLEAN Enable)
{
  return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
IoUnregisterPlugPlayNotification(
  IN PVOID NotificationEntry)
{
  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject)
{
  KIRQL OldIrql;

  assert(PopSystemPowerDeviceNode);

  KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
  *DeviceObject = PopSystemPowerDeviceNode->Pdo;
  KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

  return STATUS_SUCCESS;
}

/**********************************************************************
 * DESCRIPTION
 * 	Creates a device node
 *
 * ARGUMENTS
 *   ParentNode           = Pointer to parent device node
 *   PhysicalDeviceObject = Pointer to PDO for device object. Pass NULL
 *                          to have the root device node create one
 *                          (eg. for legacy drivers)
 *   DeviceNode           = Pointer to storage for created device node
 *
 * RETURN VALUE
 * 	Status
 */
NTSTATUS
IopCreateDeviceNode(PDEVICE_NODE ParentNode,
  PDEVICE_OBJECT PhysicalDeviceObject,
  PDEVICE_NODE *DeviceNode)
{
  PDEVICE_NODE Node;
  NTSTATUS Status;
  KIRQL OldIrql;

  DPRINT("ParentNode %x PhysicalDeviceObject %x\n",
    ParentNode, PhysicalDeviceObject);

  Node = (PDEVICE_NODE)ExAllocatePool(PagedPool, sizeof(DEVICE_NODE));
  if (!Node)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  RtlZeroMemory(Node, sizeof(DEVICE_NODE));

  if (!PhysicalDeviceObject)
    {
      Status = PnpRootCreateDevice(&PhysicalDeviceObject);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(Node);
          return Status;
        }
    }

  Node->Pdo = PhysicalDeviceObject;

  if (ParentNode)
    {
      KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);
      Node->Parent = ParentNode;
      Node->NextSibling = ParentNode->Child;
      ParentNode->Child->PrevSibling = Node;
      ParentNode->Child = Node;
      KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);
    }

  *DeviceNode = Node;

  return STATUS_SUCCESS;
}

NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode)
{
  KIRQL OldIrql;

  /* All children must be deleted before a parent is deleted */
  assert(!DeviceNode->Child);

  KeAcquireSpinLock(&IopDeviceTreeLock, &OldIrql);

  assert(DeviceNode->Pdo);

  ObDereferenceObject(DeviceNode->Pdo);

  /* Unlink from parent if it exists */

  if ((DeviceNode->Parent) && (DeviceNode->Parent->Child == DeviceNode))
    {
      DeviceNode->Parent->Child = DeviceNode->NextSibling;
    }

  /* Unlink from sibling list */

  if (DeviceNode->PrevSibling)
    {
      DeviceNode->PrevSibling->NextSibling = DeviceNode->NextSibling;
    }

  if (DeviceNode->NextSibling)
    {
  DeviceNode->NextSibling->PrevSibling = DeviceNode->PrevSibling;
    }

  KeReleaseSpinLock(&IopDeviceTreeLock, OldIrql);

  RtlFreeUnicodeString(&DeviceNode->InstancePath);

  RtlFreeUnicodeString(&DeviceNode->ServiceName);

  /* FIXME: Other fields may need to be released */

  ExFreePool(DeviceNode);

  return STATUS_SUCCESS;
}

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack)
{
  PDEVICE_OBJECT TopDeviceObject;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  /* Always call the top of the device stack */
  TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

  KeInitializeEvent(
    &Event,
	  NotificationEvent,
	  FALSE);

  /* PNP IRPs are always initialized with a status code of
     STATUS_NOT_IMPLEMENTED */
  IoStatusBlock->Status = STATUS_NOT_IMPLEMENTED;

  Irp = IoBuildSynchronousFsdRequest(
    IRP_MJ_PNP,
    TopDeviceObject,
	  NULL,
	  0,
	  NULL,
	  &Event,
	  IoStatusBlock);

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = MinorFunction;
  RtlMoveMemory(
    &IrpSp->Parameters,
    &Stack->Parameters,
    sizeof(Stack->Parameters));

	Status = IoCallDriver(TopDeviceObject, Irp);
	if (Status == STATUS_PENDING)
	  {
		  KeWaitForSingleObject(
        &Event,
        Executive,
		    KernelMode,
		    FALSE,
		    NULL);
      Status = IoStatusBlock->Status;
    }

  return Status;
}

NTSTATUS
IopInterrogateBusExtender(
  PDEVICE_NODE DeviceNode,
  PDEVICE_OBJECT FunctionDeviceObject,
  BOOLEAN BootDriversOnly)
{
  PDEVICE_RELATIONS DeviceRelations;
	IO_STATUS_BLOCK	IoStatusBlock;
  UNICODE_STRING DriverName;
  IO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT("Sending IRP_MN_QUERY_DEVICE_RELATIONS to bus driver\n");

  Stack.Parameters.QueryDeviceRelations.Type = BusRelations;

  Status = IopInitiatePnpIrp(
    FunctionDeviceObject,
    &IoStatusBlock,
    IRP_MN_QUERY_DEVICE_RELATIONS,
    &Stack);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("IopInitiatePnpIrp() failed\n");
    }

  DeviceRelations = (PDEVICE_RELATIONS)IoStatusBlock.Information;

  DPRINT("Got %d PDOs\n", DeviceRelations->Count);

  if (DeviceRelations->Count <= 0)
    {
      DPRINT("No PDOs\n");
      ExFreePool(DeviceRelations);
      return STATUS_SUCCESS;
    }

  Status = IopCreateDeviceNode(DeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("No resources\n");
      ExFreePool(DeviceRelations);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* FIXME: Use registry to find name of driver and only load driver if not
            already loaded. If loaded, just call AddDevice() */
  Status = LdrLoadDriver(&DriverName, DeviceNode, FALSE);
  if (!NT_SUCCESS(Status))
    {
      /* Don't free the device node, just log the error and return */
      /* FIXME: Log the error */
	    CPRINT("Driver load failed, status (%x)\n", Status);
      ExFreePool(DeviceRelations);
      return Status;
    }

  ExFreePool(DeviceRelations);

  return Status;
}

VOID IopLoadBootStartDrivers(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_BASIC_INFORMATION KeyInfo;
  PDEVICE_NODE DeviceNode;
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  ULONG Index;

  DPRINT("Loading boot start drivers\n");

  BufferSize = sizeof(KEY_BASIC_INFORMATION) + (MAX_PATH+1) * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(PagedPool, BufferSize);
  if (!KeyInfo)
  {
    return;
  }

  RtlInitUnicodeString(
    &KeyName,
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");

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
  return;
    }

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

    /* Use IopRootDeviceNode for now */
    Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
    if (!NT_SUCCESS(Status))
    {
      CPRINT("IopCreateDeviceNode() failed with status 0x%X\n", Status);
      break;
    }

    if (!RtlCreateUnicodeString(&DeviceNode->ServiceName, KeyInfo->Name))
    {
      CPRINT("RtlCreateUnicodeString() failed\n");
      IopFreeDeviceNode(DeviceNode);
      break;
    }

    Status = IopInitializeDeviceNodeService(DeviceNode);
    if (!NT_SUCCESS(Status))
    {
      /* FIXME: Write an entry in the system error log */
      CPRINT("Could not load boot start driver: %wZ, status 0x%X\n",
        &DeviceNode->ServiceName, Status);

      IopFreeDeviceNode(DeviceNode);
    }

    Index++;
  } while (TRUE);

  DPRINT("Services found: %d\n", Index);

  NtClose(KeyHandle);

  ExFreePool(KeyInfo);
}

VOID PnpInit(VOID)
{
  NTSTATUS Status;

  DPRINT("Called\n");

  KeInitializeSpinLock(&IopDeviceTreeLock);

  Status = IopCreateDriverObject(&IopRootDriverObject);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateDriverObject() failed\n");
      KeBugCheck(0);
    }

  PnpRootDriverEntry(IopRootDriverObject, NULL);

  Status = IoCreateDevice(IopRootDriverObject, 0, NULL,
    FILE_DEVICE_CONTROLLER, 0, FALSE, &IopRootDeviceNode->Pdo);
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCreateDevice() failed\n");
      KeBugCheck(0);
    }

  IopRootDeviceNode->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

  IopRootDriverObject->DriverExtension->AddDevice(
    IopRootDriverObject, IopRootDeviceNode->Pdo);
}

/* EOF */
