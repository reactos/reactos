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

#define NDEBUG
#include <ntoskrnl.h>
#include <internal/debug.h>

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
    /* create a new PDO and return it in *DeviceObject */
    Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
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
  *ConflictDetected = FALSE;
  DPRINT1("IoReportResourceForDetection partly implemented\n");
  
  /* HACK: check if serial debug output is enabled. If yes,
   * prevent serial port driver to detect this serial port
   * by indicating a conflict
   */
  if ((KdDebugState & KD_DEBUG_SERIAL) && DriverList != NULL)
  {
    ULONG ComPortBase = 0;
    ULONG i;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
    
    switch (LogPortInfo.ComPort)
    {
      case 1: ComPortBase = 0x3f8; break;
      case 2: ComPortBase = 0x2f8; break;
      case 3: ComPortBase = 0x3e8; break;
      case 4: ComPortBase = 0x2e8; break;
    }
    
    /* search for this port address in DriverList */
    for (i = 0; i < DriverList->List[0].PartialResourceList.Count; i++)
    {
      ResourceDescriptor = &DriverList->List[0].PartialResourceList.PartialDescriptors[i];
      if (ResourceDescriptor->Type == CmResourceTypePort)
      {
        if (ResourceDescriptor->u.Port.Start.u.LowPart <= ComPortBase
         && ResourceDescriptor->u.Port.Start.u.LowPart + ResourceDescriptor->u.Port.Length > ComPortBase)
        {
          *ConflictDetected = TRUE;
          return STATUS_CONFLICTING_ADDRESSES;
        }
      }
    }
  }
  
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
