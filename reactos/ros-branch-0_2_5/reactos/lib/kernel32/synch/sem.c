/* $Id: sem.c,v 1.10 2004/10/24 12:26:27 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/sem.c
 * PURPOSE:         Semaphore functions
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 01/20/2001
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
CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCSTR lpName)
{
   UNICODE_STRING NameU;
   ANSI_STRING Name;
   HANDLE Handle;

   if (lpName != NULL)
     {
        RtlInitAnsiString(&Name,
                          (LPSTR)lpName);
        RtlAnsiStringToUnicodeString(&NameU,
                                     &Name,
                                     TRUE);
     }

   Handle = CreateSemaphoreW(lpSemaphoreAttributes,
			     lInitialCount,
			     lMaximumCount,
			     (lpName ? NameU.Buffer : NULL));

   if (lpName != NULL)
     {
        RtlFreeUnicodeString (&NameU);
     }

   return Handle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   UNICODE_STRING UnicodeName;
   HANDLE SemaphoreHandle;

   if (lpName != NULL)
     {
       RtlInitUnicodeString(&UnicodeName, lpName);
     }

   InitializeObjectAttributes(&ObjectAttributes,
			      (lpName ? &UnicodeName : NULL),
			      0,
			      hBaseDir,
			      NULL);

   if (lpSemaphoreAttributes != NULL)
     {
	ObjectAttributes.SecurityDescriptor = lpSemaphoreAttributes->lpSecurityDescriptor;
	if (lpSemaphoreAttributes->bInheritHandle)
	  {
	     ObjectAttributes.Attributes |= OBJ_INHERIT;
	  }
     }

   Status = NtCreateSemaphore(&SemaphoreHandle,
			      SEMAPHORE_ALL_ACCESS,
			      &ObjectAttributes,
			      lInitialCount,
			      lMaximumCount);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }
   return SemaphoreHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenSemaphoreA(DWORD dwDesiredAccess,
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

   Status = NtOpenSemaphore(&Handle,
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
OpenSemaphoreW(DWORD dwDesiredAccess,
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

   Status = NtOpenSemaphore(&Handle,
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
ReleaseSemaphore(HANDLE hSemaphore,
		 LONG lReleaseCount,
		 LPLONG lpPreviousCount)
{
   NTSTATUS Status;

   Status = NtReleaseSemaphore(hSemaphore,
			       lReleaseCount,
			       lpPreviousCount);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}

/* EOF */
