/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/event.c
 * PURPOSE:         Named event support
 * PROGRAMMER:      Philip Susi and David Welch
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventObjectType = NULL;

static GENERIC_MAPPING ExpEventMapping = {
	STANDARD_RIGHTS_READ | SYNCHRONIZE | EVENT_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | SYNCHRONIZE | EVENT_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | EVENT_QUERY_STATE,
	EVENT_ALL_ACCESS};

static const INFORMATION_CLASS_INFO ExEventInfoClass[] =
{
  ICI_SQ_SAME( sizeof(EVENT_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ), /* EventBasicInformation */
};

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtpCreateEvent(PVOID ObjectBody,
	       PVOID Parent,
	       PWSTR RemainingPath,
	       POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("NtpCreateEvent(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID INIT_FUNCTION
ExpInitializeEventImplementation(VOID)
{
   ExEventObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlCreateUnicodeString(&ExEventObjectType->TypeName, L"Event");
   
   ExEventObjectType->Tag = TAG('E', 'V', 'T', 'T');
   ExEventObjectType->PeakObjects = 0;
   ExEventObjectType->PeakHandles = 0;
   ExEventObjectType->TotalObjects = 0;
   ExEventObjectType->TotalHandles = 0;
   ExEventObjectType->PagedPoolCharge = 0;
   ExEventObjectType->NonpagedPoolCharge = sizeof(KEVENT);
   ExEventObjectType->Mapping = &ExpEventMapping;
   ExEventObjectType->Dump = NULL;
   ExEventObjectType->Open = NULL;
   ExEventObjectType->Close = NULL;
   ExEventObjectType->Delete = NULL;
   ExEventObjectType->Parse = NULL;
   ExEventObjectType->Security = NULL;
   ExEventObjectType->QueryName = NULL;
   ExEventObjectType->OkayToClose = NULL;
   ExEventObjectType->Create = NtpCreateEvent;
   ExEventObjectType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExEventObjectType);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtClearEvent(IN HANDLE EventHandle)
{
   PKEVENT Event;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      ExGetPreviousMode(),
				      (PVOID*)&Event,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     KeClearEvent(Event);
     ObDereferenceObject(Event);
   }
   
   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateEvent(OUT PHANDLE EventHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	      IN EVENT_TYPE EventType,
	      IN BOOLEAN InitialState)
{
   KPROCESSOR_MODE PreviousMode;
   PKEVENT Event;
   HANDLE hEvent;
   NTSTATUS Status = STATUS_SUCCESS;
 
   PreviousMode = ExGetPreviousMode();
 
   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(EventHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
 
   Status = ObCreateObject(PreviousMode,
                           ExEventObjectType,
                           ObjectAttributes,
                           PreviousMode,
                           NULL,
                           sizeof(KEVENT),
                           0,
                           0,
                           (PVOID*)&Event);
   if(NT_SUCCESS(Status))
   {
     KeInitializeEvent(Event,
                       EventType,
                       InitialState);
 
 
     Status = ObInsertObject((PVOID)Event,
                             NULL,
                             DesiredAccess,
                             0,
                             NULL,
                             &hEvent);
     ObDereferenceObject(Event);
 
     if(NT_SUCCESS(Status))
     {
       _SEH_TRY
       {
         *EventHandle = hEvent;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }
 
   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtOpenEvent(OUT PHANDLE EventHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   HANDLE hEvent;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   DPRINT("NtOpenEvent(0x%x, 0x%x, 0x%x)\n", EventHandle, DesiredAccess, ObjectAttributes);

   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(EventHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
     
     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExEventObjectType,
			       NULL,
			       PreviousMode,
			       DesiredAccess,
			       NULL,
			       &hEvent);
             
   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *EventHandle = hEvent;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
   
   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtPulseEvent(IN HANDLE EventHandle,
	     OUT PLONG PreviousState  OPTIONAL)
{
   PKEVENT Event;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtPulseEvent(EventHandle 0%x PreviousState 0%x)\n",
	  EventHandle, PreviousState);

   PreviousMode = ExGetPreviousMode();
   
   if(PreviousState != NULL && PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(PreviousState,
                     sizeof(LONG),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      PreviousMode,
				      (PVOID*)&Event,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     LONG Prev = KePulseEvent(Event, EVENT_INCREMENT, FALSE);
     ObDereferenceObject(Event);
     
     if(PreviousState != NULL)
     {
       _SEH_TRY
       {
         *PreviousState = Prev;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryEvent(IN HANDLE EventHandle,
	     IN EVENT_INFORMATION_CLASS EventInformationClass,
	     OUT PVOID EventInformation,
	     IN ULONG EventInformationLength,
	     OUT PULONG ReturnLength  OPTIONAL)
{
   PKEVENT Event;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   PreviousMode = ExGetPreviousMode();
   
   DefaultQueryInfoBufferCheck(EventInformationClass,
                               ExEventInfoClass,
                               EventInformation,
                               EventInformationLength,
                               ReturnLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQueryEvent() failed, Status: 0x%x\n", Status);
     return Status;
   }

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_QUERY_STATE,
				      ExEventObjectType,
				      PreviousMode,
				      (PVOID*)&Event,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     switch(EventInformationClass)
     {
       case EventBasicInformation:
       {
         PEVENT_BASIC_INFORMATION BasicInfo = (PEVENT_BASIC_INFORMATION)EventInformation;
         
         _SEH_TRY
         {
           if (Event->Header.Type == InternalNotificationEvent)
             BasicInfo->EventType = NotificationEvent;
           else
             BasicInfo->EventType = SynchronizationEvent;
           BasicInfo->EventState = KeReadStateEvent(Event);

           if(ReturnLength != NULL)
           {
             *ReturnLength = sizeof(EVENT_BASIC_INFORMATION);
           }
         }
         _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
         _SEH_END;
         break;
       }

       default:
         Status = STATUS_NOT_IMPLEMENTED;
         break;
     }

     ObDereferenceObject(Event);
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtResetEvent(IN HANDLE EventHandle,
	     OUT PLONG PreviousState  OPTIONAL)
{
   PKEVENT Event;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtResetEvent(EventHandle 0%x PreviousState 0%x)\n",
	  EventHandle, PreviousState);

   PreviousMode = ExGetPreviousMode();

   if(PreviousState != NULL && PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(PreviousState,
                     sizeof(LONG),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      PreviousMode,
				      (PVOID*)&Event,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     LONG Prev = KeResetEvent(Event);
     ObDereferenceObject(Event);
     
     if(PreviousState != NULL)
     {
       _SEH_TRY
       {
         *PreviousState = Prev;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtSetEvent(IN HANDLE EventHandle,
	   OUT PLONG PreviousState  OPTIONAL)
{
   PKEVENT Event;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("NtSetEvent(EventHandle 0%x PreviousState 0%x)\n",
	  EventHandle, PreviousState);

   PreviousMode = ExGetPreviousMode();

   if(PreviousState != NULL && PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(PreviousState,
                     sizeof(LONG),
                     sizeof(ULONG));
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   Status = ObReferenceObjectByHandle(EventHandle,
				      EVENT_MODIFY_STATE,
				      ExEventObjectType,
				      PreviousMode,
				      (PVOID*)&Event,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     LONG Prev = KeSetEvent(Event, EVENT_INCREMENT, FALSE);
     ObDereferenceObject(Event);

     if(PreviousState != NULL)
     {
       _SEH_TRY
       {
         *PreviousState = Prev;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   }

   return Status;
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtTraceEvent(
	IN ULONG TraceHandle,
	IN ULONG Flags,
	IN ULONG TraceHeaderLength,
	IN struct _EVENT_TRACE_HEADER* TraceHeader
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


/* EOF */
