/* $Id: pnpmgr.c,v 1.2 2001/05/01 23:08:19 chorns Exp $
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
/* */
{
  PDEVICE_NODE Node;
  NTSTATUS Status;
  KIRQL OldIrql;

  DPRINT("ParentNode %x PhysicalDeviceObject %x\n");

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

  ExFreePool(DeviceNode);

  return STATUS_SUCCESS;
}

NTSTATUS
IopInterrogateBusExtender(PDEVICE_NODE DeviceNode,
  PDEVICE_OBJECT FunctionDeviceObject,
  BOOLEAN BootDriversOnly)
{
	IO_STATUS_BLOCK	IoStatusBlock;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  DPRINT("Sending IRP_MN_QUERY_DEVICE_RELATIONS to bus driver\n");

  KeInitializeEvent(&Event,
	  NotificationEvent,
	  FALSE);

  Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
    FunctionDeviceObject,
	  NULL,
	  0,
	  NULL,
	  &Event,
	  &IoStatusBlock);

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
  IrpSp->Parameters.QueryDeviceRelations.Type = BusRelations;

	Status = IoCallDriver(FunctionDeviceObject, Irp);
	if (Status == STATUS_PENDING)
	  {
		  KeWaitForSingleObject(&Event,
        Executive,
		    KernelMode,
		    FALSE,
		    NULL);
        Status = IoStatusBlock.Status;
    }
  if (!NT_SUCCESS(Status))
    {
      CPRINT("IoCallDriver() failed\n");
    }

  return Status;
}

VOID IopLoadBootStartDrivers(VOID)
{
  UNICODE_STRING DriverName;
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;

  DPRINT("Loading boot start drivers\n");

return;

  /* FIXME: Get these from registry */

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
  return;
    }

  /*
   * ISA Plug and Play driver
   */
  RtlInitUnicodeString(&DriverName,
    L"\\SystemRoot\\system32\\drivers\\isapnp.sys");
  Status = LdrLoadDriver(&DriverName, DeviceNode, TRUE);
  if (!NT_SUCCESS(Status))
    {
	    IopFreeDeviceNode(DeviceNode);

      /* FIXME: Write an entry in the system error log */
      DbgPrint("Could not load boot start driver: %wZ, status 0x%X\n",
        &DriverName, Status);
      return;
    }
}

VOID PnpInit(VOID)
{
  NTSTATUS Status;

  DPRINT("Called\n");

  KeInitializeSpinLock(&IopDeviceTreeLock);

  Status = IopCreateDriverObject(&IopRootDriverObject);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IoCreateDriverObject() failed\n");
      KeBugCheck(0);
    }

  PnpRootDriverEntry(IopRootDriverObject, NULL);

  Status = IoCreateDevice(IopRootDriverObject, 0, NULL,
    FILE_DEVICE_CONTROLLER, 0, FALSE, &IopRootDeviceNode->Pdo);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IoCreateDevice() failed\n");
      KeBugCheck(0);
    }

  IopRootDeviceNode->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

  IopRootDriverObject->DriverExtension->AddDevice(
    IopRootDriverObject, IopRootDeviceNode->Pdo);
}

/* EOF */
