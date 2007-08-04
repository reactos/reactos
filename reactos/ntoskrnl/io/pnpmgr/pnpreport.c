/* $Id$
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
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
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
  PDEVICE_OBJECT Pdo;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("IoReportDetectedDevice (DeviceObject %p, *DeviceObject %p)\n",
    DeviceObject, DeviceObject ? *DeviceObject : NULL);

  /* if *DeviceObject is not NULL, we must use it as a PDO,
   * and don't create a new one.
   */
  if (DeviceObject && *DeviceObject)
    Pdo = *DeviceObject;
  else
  {
    UNICODE_STRING ServiceName;
    ServiceName.Buffer = DriverObject->DriverName.Buffer + sizeof(DRIVER_ROOT_NAME) / sizeof(WCHAR) - 1;
    ServiceName.Length = ServiceName.MaximumLength = DriverObject->DriverName.Length - sizeof(DRIVER_ROOT_NAME) + sizeof(WCHAR);
    
    /* create a new PDO and return it in *DeviceObject */
    Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &ServiceName, &DeviceNode);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("IopCreateDeviceNode() failed (Status 0x%08lx)\n", Status);
      return Status;
    }
    Pdo = DeviceNode->PhysicalDeviceObject;
    if (DeviceObject)
      *DeviceObject = Pdo;
  }

  /* we don't need to call AddDevice and send IRP_MN_START_DEVICE */

  /* FIXME: save this device into the root-enumerated list, so this
   * device would be detected as a PnP device during next startups.
   */

  return Status;
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
  static int warned = 0;
  if (!warned)
  {
    DPRINT1("IoReportResourceForDetection partly implemented\n");
    warned = 1;
  }

  *ConflictDetected = FALSE;

  if (PopSystemPowerDeviceNode != NULL && DriverListSize > 0)
  {
    /* We hope legacy devices will be enumerated by ACPI */
    *ConflictDetected = TRUE;
    return STATUS_CONFLICTING_ADDRESSES;
  }
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
