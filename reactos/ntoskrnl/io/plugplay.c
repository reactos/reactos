/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/plugplay.c
 * PURPOSE:         Plug-and-play interface routines
 *
 * PROGRAMMERS:     Eric Kohl <eric.kohl@t-online.de>
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


typedef struct _PNP_EVENT_ENTRY
{
  LIST_ENTRY ListEntry;
  PLUGPLAY_EVENT_BLOCK Event;
} PNP_EVENT_ENTRY, *PPNP_EVENT_ENTRY;


/* GLOBALS *******************************************************************/

static LIST_ENTRY IopPnpEventQueueHead;
static KEVENT IopPnpNotifyEvent;

/* FUNCTIONS *****************************************************************/

NTSTATUS INIT_FUNCTION
IopInitPlugPlayEvents(VOID)
{
    InitializeListHead(&IopPnpEventQueueHead);

    KeInitializeEvent(&IopPnpNotifyEvent,
                      SynchronizationEvent,
                      FALSE);

    return STATUS_SUCCESS;
}


NTSTATUS
IopQueueTargetDeviceEvent(const GUID *Guid,
                          PUNICODE_STRING DeviceIds)
{
    PPNP_EVENT_ENTRY EventEntry;
    DWORD TotalSize;

    TotalSize =
        FIELD_OFFSET(PLUGPLAY_EVENT_BLOCK, TargetDevice.DeviceIds) +
        DeviceIds->MaximumLength;

    EventEntry = ExAllocatePool(NonPagedPool,
                                TotalSize + FIELD_OFFSET(PNP_EVENT_ENTRY, Event));
    if (EventEntry == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    memcpy(&EventEntry->Event.EventGuid,
           Guid,
           sizeof(GUID));
    EventEntry->Event.EventCategory = TargetDeviceChangeEvent;
    EventEntry->Event.TotalSize = TotalSize;

    memcpy(&EventEntry->Event.TargetDevice.DeviceIds,
           DeviceIds->Buffer,
           DeviceIds->MaximumLength);

    InsertHeadList(&IopPnpEventQueueHead,
                   &EventEntry->ListEntry);
    KeSetEvent(&IopPnpNotifyEvent,
               0,
               FALSE);

    return STATUS_SUCCESS;
}


/*
 * Remove the current PnP event from the tail of the event queue
 * and signal IopPnpNotifyEvent if there is yet another event in the queue.
 */
static NTSTATUS
IopRemovePlugPlayEvent(VOID)
{
  /* Remove a pnp event entry from the tail of the queue */
  if (!IsListEmpty(&IopPnpEventQueueHead))
  {
    ExFreePool(RemoveTailList(&IopPnpEventQueueHead));
  }

  /* Signal the next pnp event in the queue */
  if (!IsListEmpty(&IopPnpEventQueueHead))
  {
    KeSetEvent(&IopPnpNotifyEvent,
               0,
               FALSE);
  }

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtGetPlugPlayEvent(IN ULONG Reserved1,
                   IN ULONG Reserved2,
                   OUT PPLUGPLAY_EVENT_BLOCK Buffer,
                   IN ULONG BufferLength)
{
  PPNP_EVENT_ENTRY Entry;
  NTSTATUS Status;

  DPRINT("NtGetPlugPlayEvent() called\n");

  /* Function can only be called from user-mode */
  if (KeGetPreviousMode() != UserMode)
  {
    DPRINT1("NtGetPlugPlayEvent cannot be called from kernel mode!\n");
    return STATUS_ACCESS_DENIED;
  }

  /* Check for Tcb privilege */
  if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                              UserMode))
  {
    DPRINT1("NtGetPlugPlayEvent: Caller does not hold the SeTcbPrivilege privilege!\n");
    return STATUS_PRIVILEGE_NOT_HELD;
  }

  /* Wait for a PnP event */
  DPRINT("Waiting for pnp notification event\n");
  Status = KeWaitForSingleObject(&IopPnpNotifyEvent,
                                 UserRequest,
                                 KernelMode,
                                 FALSE,
                                 NULL);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("KeWaitForSingleObject() failed (Status %lx)\n", Status);
    return Status;
  }

  /* Get entry from the tail of the queue */
  Entry = CONTAINING_RECORD(IopPnpEventQueueHead.Blink,
                            PNP_EVENT_ENTRY,
                            ListEntry);

  /* Check the buffer size */
  if (BufferLength < Entry->Event.TotalSize)
  {
    DPRINT1("Buffer is too small for the pnp-event\n");
    return STATUS_BUFFER_TOO_SMALL;
  }

  /* Copy event data to the user buffer */
  memcpy(Buffer,
         &Entry->Event,
         Entry->Event.TotalSize);

  DPRINT("NtGetPlugPlayEvent() done\n");

  return STATUS_SUCCESS;
}


static PDEVICE_OBJECT
IopGetDeviceObjectFromDeviceInstance(PUNICODE_STRING DeviceInstance)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    LPWSTR KeyNameBuffer;
    HANDLE InstanceKeyHandle;
    HANDLE ControlKeyHandle;
    NTSTATUS Status;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInformation;
    ULONG ValueInformationLength;
    PDEVICE_OBJECT DeviceObject = NULL;

    DPRINT("IopGetDeviceObjectFromDeviceInstance(%wZ) called\n", DeviceInstance);

    KeyNameBuffer = ExAllocatePool(PagedPool,
                                   (49 * sizeof(WCHAR)) + DeviceInstance->Length);
    if (KeyNameBuffer == NULL)
    {
        DPRINT1("Failed to allocate key name buffer!\n");
        return NULL;
    }

    wcscpy(KeyNameBuffer, L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\");
    wcscat(KeyNameBuffer, DeviceInstance->Buffer);

    RtlInitUnicodeString(&KeyName,
                         KeyNameBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&InstanceKeyHandle,
                       KEY_READ,
                       &ObjectAttributes);
    ExFreePool(KeyNameBuffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the instance key (Status %lx)\n", Status);
        return NULL;
    }

    /* Open the 'Control' subkey */
    RtlInitUnicodeString(&KeyName,
                         L"Control");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               InstanceKeyHandle,
                               NULL);

    Status = ZwOpenKey(&ControlKeyHandle,
                       KEY_READ,
                       &ObjectAttributes);
    ZwClose(InstanceKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the 'Control' key (Status %lx)\n", Status);
        return NULL;
    }

    /* Query the 'DeviceReference' value */
    RtlInitUnicodeString(&ValueName,
                         L"DeviceReference");
    ValueInformationLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION,
                             Data[0]) + sizeof(ULONG);
    ValueInformation = ExAllocatePool(PagedPool, ValueInformationLength);
    if (ValueInformation == NULL)
    {
        DPRINT1("Failed to allocate the name information buffer!\n");
        ZwClose(ControlKeyHandle);
        return NULL;
    }

    Status = ZwQueryValueKey(ControlKeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueInformation,
                             ValueInformationLength,
                             &ValueInformationLength);
    ZwClose(ControlKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open the 'Control' key (Status %lx)\n", Status);
        return NULL;
    }

    /* Check the device object */
    RtlCopyMemory(&DeviceObject,
                  ValueInformation->Data,
                  sizeof(PDEVICE_OBJECT));

    DPRINT("DeviceObject: %p\n", DeviceObject);

    if (DeviceObject->Type != IO_TYPE_DEVICE ||
        DeviceObject->DeviceObjectExtension == NULL ||
        DeviceObject->DeviceObjectExtension->DeviceNode == NULL ||
        !RtlEqualUnicodeString(&DeviceObject->DeviceObjectExtension->DeviceNode->InstancePath,
                               DeviceInstance, TRUE))
    {
        DPRINT1("Invalid object type!\n");
        return NULL;
    }

    DPRINT("Instance path: %wZ\n", &DeviceObject->DeviceObjectExtension->DeviceNode->InstancePath);

    ObReferenceObject(DeviceObject);

    DPRINT("IopGetDeviceObjectFromDeviceInstance() done\n");

    return DeviceObject;
}


static NTSTATUS
IopGetRelatedDevice(PPLUGPLAY_RELATED_DEVICE_DATA RelatedDeviceData)
{
    UNICODE_STRING RootDeviceName;
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_NODE DeviceNode = NULL;
    PDEVICE_NODE RelatedDeviceNode;

    DPRINT("IopGetRelatedDevice() called\n");

    DPRINT("Device name: %wZ\n", &RelatedDeviceData->DeviceInstance);

    RtlInitUnicodeString(&RootDeviceName,
                         L"HTREE\\ROOT\\0");
    if (RtlEqualUnicodeString(&RelatedDeviceData->DeviceInstance,
                              &RootDeviceName,
                              TRUE))
    {
        DeviceNode = IopRootDeviceNode;
    }
    else
    {
        /* Get the device object */
        DeviceObject = IopGetDeviceObjectFromDeviceInstance(&RelatedDeviceData->DeviceInstance);
        if (DeviceObject == NULL)
            return STATUS_NO_SUCH_DEVICE;

        DeviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;
    }

    switch (RelatedDeviceData->Relation)
    {
        case PNP_GET_PARENT_DEVICE:
            RelatedDeviceNode = DeviceNode->Parent;
            break;

        case PNP_GET_CHILD_DEVICE:
            RelatedDeviceNode = DeviceNode->Child;
            break;

        case PNP_GET_SIBLING_DEVICE:
            RelatedDeviceNode = DeviceNode->NextSibling;
            break;

        default:
            if (DeviceObject != NULL)
            {
                ObDereferenceObject(DeviceObject);
            }

            return STATUS_INVALID_PARAMETER;
    }

    if (RelatedDeviceNode == NULL)
    {
        if (DeviceObject)
        {
            ObDereferenceObject(DeviceObject);
        }

        return STATUS_NO_SUCH_DEVICE;
    }

    if (RelatedDeviceNode->InstancePath.Length >
        RelatedDeviceData->RelatedDeviceInstance.MaximumLength)
    {
        if (DeviceObject)
        {
            ObDereferenceObject(DeviceObject);
        }

        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy related device instance name */
    RtlCopyMemory(RelatedDeviceData->RelatedDeviceInstance.Buffer,
                  RelatedDeviceNode->InstancePath.Buffer,
                  RelatedDeviceNode->InstancePath.Length);
    RelatedDeviceData->RelatedDeviceInstance.Length =
        RelatedDeviceNode->InstancePath.Length;

    if (DeviceObject != NULL)
    {
        ObDereferenceObject(DeviceObject);
    }

    DPRINT("IopGetRelatedDevice() done\n");

    return STATUS_SUCCESS;
}


static NTSTATUS
IopDeviceStatus(PPLUGPLAY_DEVICE_STATUS_DATA DeviceStatusData)
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_NODE DeviceNode;

    DPRINT("IopDeviceStatus() called\n");

    DPRINT("Device name: %wZ\n", &DeviceStatusData->DeviceInstance);

    /* Get the device object */
    DeviceObject = IopGetDeviceObjectFromDeviceInstance(&DeviceStatusData->DeviceInstance);
    if (DeviceObject == NULL)
        return STATUS_NO_SUCH_DEVICE;

    DeviceNode = DeviceObject->DeviceObjectExtension->DeviceNode;

    switch (DeviceStatusData->Action)
    {
        case PNP_GET_DEVICE_STATUS:
            DPRINT("Get status data\n");
            DeviceStatusData->Problem = DeviceNode->Problem;
            DeviceStatusData->Flags = DeviceNode->Flags;
            break;

        case PNP_SET_DEVICE_STATUS:
            DPRINT("Set status data\n");
            DeviceNode->Problem = DeviceStatusData->Problem;
            DeviceNode->Flags = DeviceStatusData->Flags;
            break;

        case PNP_CLEAR_DEVICE_STATUS:
            DPRINT1("FIXME: Clear status data!\n");
            break;
    }

    ObDereferenceObject(DeviceObject);

    return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtPlugPlayControl(IN ULONG ControlCode,
                  IN OUT PVOID Buffer,
                  IN ULONG BufferLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("NtPlugPlayControl(%lu %p %lu) called\n",
           ControlCode, Buffer, BufferLength);

    /* Function can only be called from user-mode */
    if (KeGetPreviousMode() != UserMode)
    {
        DPRINT1("NtGetPlugPlayEvent cannot be called from kernel mode!\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check for Tcb privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                UserMode))
    {
        DPRINT1("NtGetPlugPlayEvent: Caller does not hold the SeTcbPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Probe the buffer */
    _SEH_TRY
    {
        ProbeForWrite(Buffer,
                      BufferLength,
                      sizeof(ULONG));
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    switch (ControlCode)
    {
        case PLUGPLAY_USER_RESPONSE:
            if (Buffer || BufferLength != 0)
                return STATUS_INVALID_PARAMETER;
            return IopRemovePlugPlayEvent();

        case PLUGPLAY_GET_RELATED_DEVICE:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_RELATED_DEVICE_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopGetRelatedDevice((PPLUGPLAY_RELATED_DEVICE_DATA)Buffer);

        case PLUGPLAY_DEVICE_STATUS:
            if (!Buffer || BufferLength < sizeof(PLUGPLAY_DEVICE_STATUS_DATA))
                return STATUS_INVALID_PARAMETER;
            return IopDeviceStatus((PPLUGPLAY_DEVICE_STATUS_DATA)Buffer);
    }

    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
