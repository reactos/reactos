/* $Id: pnpmgr.c,v 1.1 2001/04/16 00:51:19 chorns Exp $
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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

/* DATA **********************************************************************/

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

VOID PnpInit(VOID)
{
}

/* EOF */
