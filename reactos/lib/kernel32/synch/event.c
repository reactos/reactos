/* $Id: event.c,v 1.9 2000/10/08 12:56:45 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/event.c
 * PURPOSE:         Local string functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>

WINBOOL STDCALL SetEvent(HANDLE hEvent)
{
   NTSTATUS errCode;
   ULONG Count;
   
   errCode = NtSetEvent(hEvent,&Count);
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   return TRUE;
}

WINBOOL STDCALL ResetEvent(HANDLE hEvent)
{
   NTSTATUS errCode;
   ULONG Count;
   
   errCode = NtResetEvent(hEvent, &Count);
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastErrorByStatus (errCode);
	return FALSE;
     }
   return TRUE;
}



HANDLE
STDCALL
CreateEventW (
	LPSECURITY_ATTRIBUTES	lpEventAttributes,
	WINBOOL			bManualReset,
	WINBOOL			bInitialState,
	LPCWSTR			lpName
	)
{
   NTSTATUS errCode;
   HANDLE hEvent;
   UNICODE_STRING EventNameString;
   POBJECT_ATTRIBUTES PtrObjectAttributes;
   OBJECT_ATTRIBUTES ObjectAttributes;
   
   if (lpName != NULL)
     {
	PtrObjectAttributes = &ObjectAttributes;
	ObjectAttributes.Attributes = 0;
	if (lpEventAttributes != NULL)
	  {
	     ObjectAttributes.SecurityDescriptor = 
	       lpEventAttributes->lpSecurityDescriptor;
	     if ( lpEventAttributes->bInheritHandle == TRUE )
	       ObjectAttributes.Attributes |= OBJ_INHERIT;
	  }

	RtlInitUnicodeString(&EventNameString, (LPWSTR)lpName);
	ObjectAttributes.ObjectName = &EventNameString;
     }
   else
     {
	PtrObjectAttributes = NULL;
     }
    
   DPRINT( "Calling NtCreateEvent\n" );
   errCode = NtCreateEvent(&hEvent,
			   STANDARD_RIGHTS_ALL|EVENT_READ_ACCESS|EVENT_WRITE_ACCESS,
			   PtrObjectAttributes,
			   bManualReset,
			   bInitialState);
   DPRINT( "Called\n" );
   if (!NT_SUCCESS(errCode))
     {
	SetLastErrorByStatus (errCode);
	return INVALID_HANDLE_VALUE;
     }

   return hEvent;
}


HANDLE
STDCALL
OpenEventW (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCWSTR	lpName
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING EventNameString;
	NTSTATUS errCode;
	HANDLE hEvent = NULL;

	if(lpName == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return NULL;
	}
	
	ObjectAttributes.Attributes = 0;
	ObjectAttributes.SecurityDescriptor = NULL;
	RtlInitUnicodeString(&EventNameString, (LPWSTR)lpName);
	ObjectAttributes.ObjectName = &EventNameString;

	if (bInheritHandle == TRUE )
		ObjectAttributes.Attributes |= OBJ_INHERIT;
	

	errCode = NtOpenEvent(hEvent,dwDesiredAccess,&ObjectAttributes);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastErrorByStatus (errCode);
		return NULL;
	}

	return hEvent;
}


HANDLE
STDCALL
CreateEventA (
	LPSECURITY_ATTRIBUTES	lpEventAttributes,
	WINBOOL			bManualReset,
	WINBOOL			bInitialState,
	LPCSTR			lpName
	)
{
	UNICODE_STRING EventNameU;
	ANSI_STRING EventName;
	HANDLE EventHandle;

	RtlInitUnicodeString (&EventNameU, NULL);

	if (lpName)
	{
		RtlInitAnsiString (&EventName,
		                   (LPSTR)lpName);
		RtlAnsiStringToUnicodeString (&EventNameU,
		                              &EventName,
		                              TRUE);
	}

	EventHandle = CreateEventW (lpEventAttributes,
	                            bManualReset,
	                            bInitialState,
	                            EventNameU.Buffer);

	if (lpName)
		RtlFreeUnicodeString (&EventNameU);

	return EventHandle;
}


HANDLE
STDCALL
OpenEventA (
	DWORD	dwDesiredAccess,
	WINBOOL	bInheritHandle,
	LPCSTR	lpName
	)
{
	UNICODE_STRING EventNameU;
	ANSI_STRING EventName;
	HANDLE EventHandle;

	RtlInitUnicodeString (&EventNameU, NULL);

	if (lpName)
	{
		RtlInitAnsiString (&EventName,
		                   (LPSTR)lpName);
		RtlAnsiStringToUnicodeString (&EventNameU,
		                              &EventName,
		                              TRUE);
	}

	EventHandle = OpenEventW (dwDesiredAccess,
	                          bInheritHandle,
	                          EventNameU.Buffer);

	if (lpName)
		RtlFreeUnicodeString (&EventNameU);

	return EventHandle;
}


WINBOOL
STDCALL
PulseEvent (
	HANDLE	hEvent
	)
{
	ULONG Count;
	NTSTATUS errCode;
	errCode = NtPulseEvent(hEvent,&Count);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastErrorByStatus (errCode);
		return FALSE;
	}
	return TRUE;
}

/* EOF */
