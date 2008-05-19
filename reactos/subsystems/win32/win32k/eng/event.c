/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Win32k event functions
 * FILE:             subsys/win32k/eng/event.c
 * PROGRAMER:        Jason Filby
 * REVISION HISTORY:
 *        2/10/1999: Created
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL
STDCALL
EngCreateEvent ( OUT PEVENT *Event )
{
  (*Event) = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
  if ((*Event) == NULL)
    {
      return FALSE;
    }
  KeInitializeEvent((PKEVENT)(*Event), SynchronizationEvent, FALSE);
  return TRUE;
}

BOOL
STDCALL
EngDeleteEvent ( IN PEVENT Event)
{
  ExFreePool(Event);
  return TRUE;
}

PEVENT
STDCALL
EngMapEvent(IN HDEV    Dev,
	    IN HANDLE  UserObject,
	    IN PVOID   Reserved1,
	    IN PVOID   Reserved2,
	    IN PVOID   Reserved3)
{
  NTSTATUS Status;
  PKEVENT Event;

  Status = ObReferenceObjectByHandle(UserObject,
				     EVENT_MODIFY_STATE,
				     ExEventObjectType,
				     KernelMode,
				     (PVOID*)&Event,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return NULL;
    }
  return (PEVENT)Event;
}

LONG
STDCALL
EngSetEvent ( IN PEVENT Event )
{
  return KeSetEvent((PKEVENT)Event, IO_NO_INCREMENT, FALSE);
}

BOOL
STDCALL
EngUnmapEvent ( IN PEVENT Event )
{
  ObDereferenceObject((PVOID)Event);
  return TRUE;
}

BOOL
STDCALL
EngWaitForSingleObject (IN PEVENT          Event,
			IN PLARGE_INTEGER  TimeOut)
{
  NTSTATUS Status;

  Status = KeWaitForSingleObject(Event,
				 Executive,
				 KernelMode,
				 FALSE,
				 TimeOut);
  if (Status != STATUS_SUCCESS)
    {
      return FALSE;
    }
  return TRUE;
}
