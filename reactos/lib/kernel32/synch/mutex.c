/* $Id: mutex.c,v 1.9 2004/10/24 12:16:54 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/mutex.c
 * PURPOSE:         Mutex functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 01/20/2001
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes,
	     BOOL bInitialOwner,
	     LPCSTR lpName)
{
   UNICODE_STRING NameU;
   ANSI_STRING Name;
   HANDLE Handle;

   RtlInitAnsiString(&Name,
		     (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&NameU,
				&Name,
				TRUE);

   Handle = CreateMutexW(lpMutexAttributes,
			 bInitialOwner,
			 NameU.Buffer);

   RtlFreeUnicodeString(&NameU);

   return Handle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateMutexW(LPSECURITY_ATTRIBUTES lpMutexAttributes,
	     BOOL bInitialOwner,
	     LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   UNICODE_STRING UnicodeName;
   HANDLE MutantHandle;

   if (lpName != NULL)
     {
       RtlInitUnicodeString(&UnicodeName,
			    (LPWSTR)lpName);
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      (lpName ? &UnicodeName : NULL),
			      0,
			      hBaseDir,
			      NULL);

   if (lpMutexAttributes != NULL)
     {
	ObjectAttributes.SecurityDescriptor = lpMutexAttributes->lpSecurityDescriptor;
	if (lpMutexAttributes->bInheritHandle)
	  {
	     ObjectAttributes.Attributes |= OBJ_INHERIT;
	  }
     }

   Status = NtCreateMutant(&MutantHandle,
			   MUTEX_ALL_ACCESS,
			   &ObjectAttributes,
			   (BOOLEAN)bInitialOwner);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return MutantHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenMutexA(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING NameU;
   ANSI_STRING Name;
   HANDLE Handle;
   NTSTATUS Status;

   if (lpName == NULL)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitAnsiString(&Name,
		     (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&NameU,
				&Name,
				TRUE);

   InitializeObjectAttributes(&ObjectAttributes,
			      &NameU,
			      (bInheritHandle ? OBJ_INHERIT : 0),
			      hBaseDir,
			      NULL);

   Status = NtOpenMutant(&Handle,
			 (ACCESS_MASK)dwDesiredAccess,
			 &ObjectAttributes);

   RtlFreeUnicodeString(&NameU);

   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return Handle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenMutexW(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING Name;
   HANDLE Handle;
   NTSTATUS Status;

   if (lpName == NULL)
     {
	SetLastErrorByStatus(STATUS_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitUnicodeString(&Name,
			(LPWSTR)lpName);

   InitializeObjectAttributes(&ObjectAttributes,
			      &Name,
			      (bInheritHandle ? OBJ_INHERIT : 0),
			      hBaseDir,
			      NULL);

   Status = NtOpenMutant(&Handle,
			 (ACCESS_MASK)dwDesiredAccess,
			 &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return Handle;
}


/*
 * @implemented
 */
BOOL STDCALL
ReleaseMutex(HANDLE hMutex)
{
   NTSTATUS Status;

   Status = NtReleaseMutant(hMutex,
			    NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/* EOF */
