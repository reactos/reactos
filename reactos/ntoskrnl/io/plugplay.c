/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/plugplay.c
 * PURPOSE:         Plug-and-play interface routines
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
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
static VOID
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
  Entry = (PPNP_EVENT_ENTRY)IopPnpEventQueueHead.Blink;

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


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtPlugPlayControl(IN ULONG ControlCode,
                  IN OUT PVOID Buffer,
                  IN ULONG BufferLength)
{
  DPRINT("NtPlugPlayControl(%lu %p %lu) called\n",
         ControlCode, Buffer, BufferLength);

  switch (ControlCode)
  {
    case PLUGPLAY_USER_RESPONSE:
      IopRemovePlugPlayEvent();
      return STATUS_SUCCESS;
  }

  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
