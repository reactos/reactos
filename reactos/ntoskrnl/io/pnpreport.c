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

//#define NDEBUG
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
  PDEVICE_NODE DeviceNode;
  NTSTATUS Status;
  
  DPRINT("IoReportDetectedDevice called (partly implemented)\n");
  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("IopCreateDeviceNode() failed (Status 0x%08x)\n", Status);
    return Status;
  }
  
  Status = IopInitializePnpServices(DeviceNode, FALSE);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("IopInitializePnpServices() failed (Status 0x%08x)\n", Status);
    return Status;
  }
  return IopInitializeDevice(DeviceNode, DriverObject);
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
  DPRINT("IoReportResourceForDetection UNIMPLEMENTED but returns success.\n");
  *ConflictDetected = FALSE;
  return STATUS_SUCCESS;
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
  DPRINT1("IoReportTargetDeviceChange called (UNIMPLEMENTED)\n");
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
  DPRINT1("IoReportTargetDeviceChangeAsynchronous called (UNIMPLEMENTED)\n");
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
