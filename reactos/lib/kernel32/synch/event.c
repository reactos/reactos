/*
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
#include <wchar.h>


WINBOOL STDCALL SetEvent(HANDLE hEvent)
{
   NTSTATUS errCode;
   ULONG Count;
   
   errCode = NtSetEvent(hEvent,&Count);
   if (!NT_SUCCESS(errCode))
     {
	SetLastError(RtlNtStatusToDosError(errCode));
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
	SetLastError(RtlNtStatusToDosError(errCode));
	return FALSE;
     }
   return TRUE;
}



HANDLE STDCALL CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes,
			    WINBOOL bManualReset,
			    WINBOOL bInitialState,
			    LPCWSTR lpName)
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

	EventNameString.Buffer = (WCHAR *)lpName;
	EventNameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	EventNameString.MaximumLength = EventNameString.Length;
	ObjectAttributes.ObjectName = &EventNameString;
     }
   else
     {
	PtrObjectAttributes = NULL;
     }	
    
   dprintf( "Calling NtCreateEvent\n" );
   errCode = NtCreateEvent(&hEvent,
			   STANDARD_RIGHTS_ALL|EVENT_READ_ACCESS|EVENT_WRITE_ACCESS,
			   PtrObjectAttributes,
			   bManualReset,
			   bInitialState);
   dprintf( "Called\n" );
   if (!NT_SUCCESS(errCode)) 
     {
	SetLastError(RtlNtStatusToDosError(errCode));
	return INVALID_HANDLE_VALUE;
     }
 
   return hEvent;
}


HANDLE
STDCALL
OpenEventW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
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
	EventNameString.Buffer = (WCHAR *)lpName;
	EventNameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	EventNameString.MaximumLength = EventNameString.Length;
	ObjectAttributes.ObjectName = &EventNameString;
    	
	if (bInheritHandle == TRUE )
		ObjectAttributes.Attributes |= OBJ_INHERIT;
	

    	errCode = NtOpenEvent(hEvent,dwDesiredAccess,&ObjectAttributes);
    	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
	}
  
	return hEvent;
}


HANDLE
STDCALL
CreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    WINBOOL bManualReset,
    WINBOOL bInitialState,
    LPCSTR lpName
    )
{	
	int i;
	WCHAR EventNameW[MAX_PATH];
   	i = 0;
   	if( lpName )
		while ((*lpName)!=0 && i < MAX_PATH)
     	{
		   EventNameW[i] = *lpName;
		   lpName++;
		   i++;
     	}
   	EventNameW[i] = 0;
   
    return CreateEventW( lpEventAttributes, bManualReset, bInitialState, lpName ? EventNameW : 0 );
}


HANDLE
STDCALL
OpenEventA(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCSTR lpName
    )
{
	ULONG i;	
	WCHAR EventNameW[MAX_PATH];
	
	
   
	i = 0;
   	while ((*lpName)!=0 && i < MAX_PATH)
     	{
		EventNameW[i] = *lpName;
		lpName++;
		i++;
     	}
   	EventNameW[i] = 0;
   
  
	return OpenEventW(dwDesiredAccess,bInheritHandle,EventNameW);
}

WINBOOL
STDCALL
PulseEvent(
	   HANDLE hEvent
	   )
{
	ULONG Count;
	NTSTATUS errCode;
	errCode = NtPulseEvent(hEvent,&Count);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;
}
