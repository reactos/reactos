/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/pnpreport.c
 * PURPOSE:         Device Changes Reporting functions
 * 
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
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
  DPRINT("IoReportDetectedDevice called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
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
  DPRINT("IoReportResourceForDetection called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoReportTargetDeviceChange(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PVOID NotificationStructure)
{
  DPRINT("IoReportTargetDeviceChange called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
IoReportTargetDeviceChangeAsynchronous(
  IN PDEVICE_OBJECT PhysicalDeviceObject,
  IN PVOID NotificationStructure,
  IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback  OPTIONAL,
  IN PVOID Context  OPTIONAL)
{
  DPRINT("IoReportTargetDeviceChangeAsynchronous called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
