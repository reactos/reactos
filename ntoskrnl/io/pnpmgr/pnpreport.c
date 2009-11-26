/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/pnpmgr/pnpreport.c
 * PURPOSE:         Device Changes Reporting Functions
 * PROGRAMMERS:     Filip Navara (xnavara@volny.cz)
 *                  Pierre Schweitzer
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
PpSetCustomTargetEvent(IN PDEVICE_OBJECT DeviceObject,
                       IN OUT PKEVENT NotifyEvent,
                       IN OUT PNTSTATUS NotifyStatus,
                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback,
                       IN PVOID Context,
                       IN PTARGET_DEVICE_CUSTOM_NOTIFICATION NotificationStructure)
{
  ASSERT(NotificationStructure);
  ASSERT(DeviceObject);

  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}

VOID
IopReportTargetDeviceChangeAsyncWorker(PVOID Context)
{
  PINTERNAL_WORK_QUEUE_ITEM Item;

  Item = (PINTERNAL_WORK_QUEUE_ITEM)Context;
  PpSetCustomTargetEvent(Item->PhysicalDeviceObject, NULL, NULL, Item->Callback, Item->Context, Item->NotificationStructure);
  ObDereferenceObject(Item->PhysicalDeviceObject);
  ExFreePoolWithTag(Context, '  pP');
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportDetectedDevice(IN PDRIVER_OBJECT DriverObject,
                       IN INTERFACE_TYPE LegacyBusType,
                       IN ULONG BusNumber,
                       IN ULONG SlotNumber,
                       IN PCM_RESOURCE_LIST ResourceList,
                       IN PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirements OPTIONAL,
                       IN BOOLEAN ResourceAssigned,
                       IN OUT PDEVICE_OBJECT *DeviceObject OPTIONAL)
{
    PDEVICE_NODE DeviceNode;
    PDEVICE_OBJECT Pdo;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("__FUNCTION__ (DeviceObject %p, *DeviceObject %p)\n",
      DeviceObject, DeviceObject ? *DeviceObject : NULL);

    /* if *DeviceObject is not NULL, we must use it as a PDO, and don't create a new one */
    if (DeviceObject && *DeviceObject)
    {
        Pdo = *DeviceObject;
    }
    else
    {
        UNICODE_STRING ServiceName;
        ServiceName.Buffer = DriverObject->DriverName.Buffer +
          sizeof(DRIVER_ROOT_NAME) / sizeof(WCHAR) - 1;
        ServiceName.Length = DriverObject->DriverName.Length -
          sizeof(DRIVER_ROOT_NAME) + sizeof(WCHAR);
        ServiceName.MaximumLength = ServiceName.Length;

        /* create a new PDO and return it in *DeviceObject */
        Status = IopCreateDeviceNode(IopRootDeviceNode,
                                     NULL,
                                     &ServiceName,
                                     &DeviceNode);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("IopCreateDeviceNode() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        Pdo = DeviceNode->PhysicalDeviceObject;
        if (DeviceObject) *DeviceObject = Pdo;
  }

    /* we don't need to call AddDevice and send IRP_MN_START_DEVICE */
    return Status;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IoReportResourceForDetection(IN PDRIVER_OBJECT DriverObject,
                             IN PCM_RESOURCE_LIST DriverList OPTIONAL,
                             IN ULONG DriverListSize OPTIONAL,
                             IN PDEVICE_OBJECT DeviceObject OPTIONAL,
                             IN PCM_RESOURCE_LIST DeviceList OPTIONAL,
                             IN ULONG DeviceListSize OPTIONAL,
                             OUT PBOOLEAN ConflictDetected)
{
    static int warned = 0;
    if (!warned)
    {
        DPRINT1("IoReportResourceForDetection partly implemented\n");
        warned = 1;
    }

    *ConflictDetected = FALSE;

    if (PopSystemPowerDeviceNode && DriverListSize > 0)
    {
        /* We hope legacy devices will be enumerated by ACPI */
        *ConflictDetected = TRUE;
        return STATUS_CONFLICTING_ADDRESSES;
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
IopSetEvent(IN PVOID Context)
{
    PKEVENT Event = Context;

    /* Set the event */
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChange(IN PDEVICE_OBJECT PhysicalDeviceObject,
                           IN PVOID NotificationStructure)
{
  KEVENT NotifyEvent;
  NTSTATUS Status, NotifyStatus;
  PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

  ASSERT(notifyStruct);

  /* Check for valid PDO */
  if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
  {
    KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG)PhysicalDeviceObject, 0, 0);
  }

  /* FileObject must be null. PnP will fill in it */
  ASSERT(notifyStruct->FileObject == NULL);

  /* Do not handle system PnP events */
  if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
      (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
      (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
  {
    return STATUS_INVALID_DEVICE_REQUEST;
  }

  /* FIXME: Other checks to do on notify struct */

  /* Initialize even that will let us know when PnP will have finished notify */
  KeInitializeEvent(&NotifyEvent, NotificationEvent, FALSE);

  Status = PpSetCustomTargetEvent(PhysicalDeviceObject, &NotifyEvent, &NotifyStatus, NULL, NULL, notifyStruct);
  /* If no error, wait for the notify to end and return the status of the notify and not of the event */
  if (NT_SUCCESS(Status))
  {
    KeWaitForSingleObject(&NotifyEvent, Executive, KernelMode, FALSE, NULL);
    Status = NotifyStatus;
  }

  return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoReportTargetDeviceChangeAsynchronous(IN PDEVICE_OBJECT PhysicalDeviceObject,
                                       IN PVOID NotificationStructure,
                                       IN PDEVICE_CHANGE_COMPLETE_CALLBACK Callback OPTIONAL,
                                       IN PVOID Context OPTIONAL)
{
  PINTERNAL_WORK_QUEUE_ITEM Item = NULL;
  PTARGET_DEVICE_CUSTOM_NOTIFICATION notifyStruct = (PTARGET_DEVICE_CUSTOM_NOTIFICATION)NotificationStructure;

  ASSERT(notifyStruct);

  /* Check for valid PDO */
  if (!IopIsValidPhysicalDeviceObject(PhysicalDeviceObject))
  {
    KeBugCheckEx(PNP_DETECTED_FATAL_ERROR, 0x2, (ULONG)PhysicalDeviceObject, 0, 0);
  }

  /* FileObject must be null. PnP will fill in it */
  ASSERT(notifyStruct->FileObject == NULL);

  /* Do not handle system PnP events */
  if ((RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_QUERY_REMOVE), sizeof(GUID)) != sizeof(GUID)) ||
      (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_CANCELLED), sizeof(GUID)) != sizeof(GUID)) ||
      (RtlCompareMemory(&(notifyStruct->Event), &(GUID_TARGET_DEVICE_REMOVE_COMPLETE), sizeof(GUID)) != sizeof(GUID)))
  {
    return STATUS_INVALID_DEVICE_REQUEST;
  }

  /* FIXME: Other checks to do on notify struct */

  /* We need to store all the data given by the caller with the WorkItem, so use our own struct */
  Item = ExAllocatePoolWithTag(NonPagedPool, sizeof(INTERNAL_WORK_QUEUE_ITEM), '  pP');
  if (!Item)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Initialize all stuff */
  ObReferenceObject(PhysicalDeviceObject);
  Item->NotificationStructure = notifyStruct;
  Item->PhysicalDeviceObject = PhysicalDeviceObject;
  Item->Callback = Callback;
  Item->Context = Context;
  ExInitializeWorkItem(&(Item->WorkItem), (PWORKER_THREAD_ROUTINE)IopReportTargetDeviceChangeAsyncWorker, Item);

  /* Finally, queue the item, our work here is done */
  ExQueueWorkItem(&(Item->WorkItem), DelayedWorkQueue);

  return STATUS_PENDING;
}
