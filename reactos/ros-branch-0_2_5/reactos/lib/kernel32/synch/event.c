/* $Id: event.c,v 1.20 2004/10/24 15:26:14 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/event.c
 * PURPOSE:         Local string functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes,
	     BOOL bManualReset,
	     BOOL bInitialState,
	     LPCSTR lpName)
{
   UNICODE_STRING EventNameU;
   ANSI_STRING EventName;
   HANDLE EventHandle;

   if (lpName)
     {
	RtlInitAnsiString(&EventName,
			  (LPSTR)lpName);
	RtlAnsiStringToUnicodeString(&EventNameU,
				     &EventName,
				     TRUE);
     }

   EventHandle = CreateEventW(lpEventAttributes,
			      bManualReset,
			      bInitialState,
			      (lpName ? EventNameU.Buffer : NULL));

   if (lpName)
     {
	RtlFreeUnicodeString(&EventNameU);
     }

   return EventHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes,
	     BOOL bManualReset,
	     BOOL bInitialState,
	     LPCWSTR lpName)
{
   NTSTATUS Status;
   HANDLE hEvent;
   UNICODE_STRING UnicodeName;
   OBJECT_ATTRIBUTES ObjectAttributes;

   if (lpName != NULL)
     {
	RtlInitUnicodeString(&UnicodeName, (LPWSTR)lpName);
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      (lpName ? &UnicodeName : NULL),
			      0,
			      hBaseDir,
			      NULL);

   if (lpEventAttributes != NULL)
     {
	ObjectAttributes.SecurityDescriptor = lpEventAttributes->lpSecurityDescriptor;
	if (lpEventAttributes->bInheritHandle)
	  {
	     ObjectAttributes.Attributes |= OBJ_INHERIT;
	  }
     }

   Status = NtCreateEvent(&hEvent,
			  STANDARD_RIGHTS_ALL | EVENT_READ_ACCESS | EVENT_WRITE_ACCESS,
			  &ObjectAttributes,
			  (bManualReset ? NotificationEvent : SynchronizationEvent),
			  bInitialState);
   DPRINT( "Called\n" );
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return hEvent;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenEventA(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCSTR lpName)
{
   UNICODE_STRING EventNameU;
   ANSI_STRING EventName;
   HANDLE EventHandle;

   if (lpName == NULL)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitUnicodeString(&EventNameU,
			NULL);

   RtlInitAnsiString(&EventName,
                     (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&EventNameU,
                                &EventName,
                                TRUE);

   EventHandle = OpenEventW(dwDesiredAccess,
			    bInheritHandle,
			    EventNameU.Buffer);

   RtlFreeUnicodeString(&EventNameU);

   return EventHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenEventW(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING EventNameString;
   NTSTATUS Status;
   HANDLE hEvent = NULL;

   if (lpName == NULL)
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitUnicodeString(&EventNameString, (LPWSTR)lpName);

   InitializeObjectAttributes(&ObjectAttributes,
			      &EventNameString,
			      (bInheritHandle ? OBJ_INHERIT : 0),
			      hBaseDir,
			      NULL);

   Status = NtOpenEvent(&hEvent,
			dwDesiredAccess,
			&ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return hEvent;
}


/*
 * @implemented
 */
BOOL STDCALL
PulseEvent(HANDLE hEvent)
{
   NTSTATUS Status;

   Status = NtPulseEvent(hEvent, NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ResetEvent(HANDLE hEvent)
{
   NTSTATUS Status;

   Status = NtClearEvent(hEvent);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetEvent(HANDLE hEvent)
{
   NTSTATUS Status;

   Status = NtSetEvent(hEvent, NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}

/* EOF */
