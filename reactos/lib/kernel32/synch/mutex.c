/* $Id: mutex.c,v 1.6 2003/07/10 18:50:51 chorns Exp $
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
#include <kernel32/kernel32.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes,
	     WINBOOL bInitialOwner,
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
	     WINBOOL bInitialOwner,
	     LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;
   UNICODE_STRING NameString;
   HANDLE MutantHandle;

   RtlInitUnicodeString(&NameString,
			(LPWSTR)lpName);

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = hBaseDir;
   ObjectAttributes.ObjectName = &NameString;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   if (lpMutexAttributes != NULL)
     {
	ObjectAttributes.SecurityDescriptor = lpMutexAttributes->lpSecurityDescriptor;
	if (lpMutexAttributes->bInheritHandle == TRUE)
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
	   WINBOOL bInheritHandle,
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

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = hBaseDir;
   ObjectAttributes.ObjectName = &NameU;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   if (bInheritHandle == TRUE)
     {
	ObjectAttributes.Attributes |= OBJ_INHERIT;
     }

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
	   WINBOOL bInheritHandle,
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

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = hBaseDir;
   ObjectAttributes.ObjectName = &Name;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   if (bInheritHandle == TRUE)
     {
	ObjectAttributes.Attributes |= OBJ_INHERIT;
     }

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
WINBOOL STDCALL
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
