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


/* GLOBALS *******************************************************************/

static LIST_ENTRY IopPnpEventListHead;
static KEVENT IopPnpNotifyEvent;


/* FUNCTIONS *****************************************************************/

NTSTATUS INIT_FUNCTION
IopInitPlugPlayEvents(VOID)
{

  InitializeListHead(&IopPnpEventListHead);

  KeInitializeEvent(&IopPnpNotifyEvent,
		    NotificationEvent,
		    FALSE);

  return STATUS_SUCCESS;
}


#if 0
/* Insert a new pnp event at the head of the event queue */
VOID
IopEnqueuePlugPlayEvent(VOID)
{

}
#endif


#if 0
/*
 * Remove the current PnP event from the tail of the event queue
 * and signal IopPnpNotifyEvent if there is yet another event in the queue.
 */
VOID
IopDequeuePlugPlayEvent(VOID)
{

}
#endif


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtGetPlugPlayEvent(IN ULONG Reserved1,
                   IN ULONG Reserved2,
                   OUT PVOID Buffer,
                   IN ULONG BufferLength)
{
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
    DPRINT1("NtGetPlugPlayEvent: Caller requires the SeTcbPrivilege privilege!\n");
    return STATUS_PRIVILEGE_NOT_HELD;
  }

  /* Wait for a PnP event */
  DPRINT("Waiting for pnp notification event\n");
  Status = KeWaitForSingleObject(&IopPnpNotifyEvent,
                                 UserRequest,
                                 KernelMode,
                                 FALSE,
                                 NULL);
  if (NT_SUCCESS(Status))
  {
    DPRINT("Waiting done\n");

#if 0
    /* Get entry from the tail of the list */
    Entry = IopPnpEventListHead.Blink;

    /* Check the buffer size */
    if (BufferLength < Entry->Event.Size)
    {
      DPRINT1("Buffer is too small for the pnp-event\n");
      return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy event data to user buffer */
    memcpy(Buffer,
           &Entry->Event,
           &Entry->Event.Size);
#endif
  }

  DPRINT("NtGetPlugPlayEvent() done\n");

  return Status;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtPlugPlayControl(IN ULONG ControlCode,
                  IN OUT PVOID Buffer,
                  IN ULONG BufferLength)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
