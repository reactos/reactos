/* $Id: wait.c,v 1.10 2000/03/16 21:50:11 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/wait.c
 * PURPOSE:         Wait  functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <windows.h>
#include <wchar.h>

HANDLE
STDCALL
CreateSemaphoreA (
	LPSECURITY_ATTRIBUTES	lpSemaphoreAttributes,
	LONG			lInitialCount,
	LONG			lMaximumCount,
	LPCSTR			lpName
	)
{
	UNICODE_STRING NameU;
	ANSI_STRING Name;
	HANDLE Handle;

	RtlInitAnsiString (&Name,
	                   lpName);
	RtlAnsiStringToUnicodeString (&NameU,
	                              &Name,
	                              TRUE);

	Handle = CreateSemaphoreW (lpSemaphoreAttributes,
	                           lInitialCount,
	                           lMaximumCount,
	                           NameU.Buffer);

	RtlFreeUnicodeString (&NameU);

	return Handle;
}


HANDLE
STDCALL
CreateSemaphoreW(
		 LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
		 LONG lInitialCount,
		 LONG lMaximumCount,
		 LPCWSTR lpName
		 )
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS errCode;
	UNICODE_STRING NameString;
	HANDLE SemaphoreHandle;

	NameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	NameString.Buffer = (WCHAR *)lpName;
	NameString.MaximumLength = NameString.Length;

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	if ( lpSemaphoreAttributes != NULL ) {
		ObjectAttributes.SecurityDescriptor = lpSemaphoreAttributes->lpSecurityDescriptor;
		if ( lpSemaphoreAttributes->bInheritHandle == TRUE )
			ObjectAttributes.Attributes |= OBJ_INHERIT;
	}
	

	errCode = NtCreateSemaphore(
	&SemaphoreHandle,
	GENERIC_ALL,
	&ObjectAttributes,
	lInitialCount,
	lMaximumCount
	);
	if (!NT_SUCCESS(errCode))
		{
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
		}

	return SemaphoreHandle;
}

HANDLE
STDCALL
CreateMutexA (
	LPSECURITY_ATTRIBUTES	lpMutexAttributes,
	WINBOOL			bInitialOwner,
	LPCSTR			lpName
	)
{
	UNICODE_STRING NameU;
	ANSI_STRING Name;
	HANDLE Handle;

	RtlInitAnsiString (&Name,
	                   lpName);
	RtlAnsiStringToUnicodeString (&NameU,
	                              &Name,
	                              TRUE);

	Handle = CreateMutexW (lpMutexAttributes,
	                       bInitialOwner,
	                       NameU.Buffer);

	RtlFreeUnicodeString (&NameU);

	return Handle;
}


HANDLE
STDCALL
CreateMutexW (
	LPSECURITY_ATTRIBUTES	lpMutexAttributes,
	WINBOOL			bInitialOwner,
	LPCWSTR			lpName
	)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS errCode;
	UNICODE_STRING NameString;
	HANDLE MutantHandle;

	NameString.Length = lstrlenW(lpName)*sizeof(WCHAR);
	NameString.Buffer = (WCHAR *)lpName;
	NameString.MaximumLength = NameString.Length;

	ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
	ObjectAttributes.RootDirectory = NULL;
	ObjectAttributes.ObjectName = &NameString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE;
	ObjectAttributes.SecurityDescriptor = NULL;
	ObjectAttributes.SecurityQualityOfService = NULL;

	if ( lpMutexAttributes != NULL ) {
		ObjectAttributes.SecurityDescriptor = lpMutexAttributes->lpSecurityDescriptor;
		if ( lpMutexAttributes->bInheritHandle == TRUE )
			ObjectAttributes.Attributes |= OBJ_INHERIT;
	}
	

	errCode = NtCreateMutant(&MutantHandle,GENERIC_ALL, &ObjectAttributes,(BOOLEAN)bInitialOwner);
	if (!NT_SUCCESS(errCode))
	{
		SetLastError(RtlNtStatusToDosError(errCode));
		return NULL;
	}

	return MutantHandle;

}


DWORD
STDCALL
WaitForSingleObject (
	HANDLE	hHandle,
	DWORD	dwMilliseconds
	)
{
	return WaitForSingleObjectEx(hHandle,dwMilliseconds,FALSE);
}


DWORD
STDCALL
WaitForSingleObjectEx(HANDLE hHandle,
                      DWORD  dwMilliseconds,
                      BOOL   bAlertable)
{
   NTSTATUS  errCode;
   PLARGE_INTEGER TimePtr;
   LARGE_INTEGER Time;
   DWORD retCode;

   if (dwMilliseconds == INFINITE)
     {
	TimePtr = NULL;
     }
   else
     {
        Time.QuadPart = -10000 * dwMilliseconds;
	TimePtr = &Time;
     }

   errCode = NtWaitForSingleObject(hHandle,
				   (BOOLEAN) bAlertable,
				   TimePtr);
   if (errCode == STATUS_TIMEOUT)
     {
         return WAIT_TIMEOUT;
     }
   else if (errCode == WAIT_OBJECT_0)
     {
        return WAIT_OBJECT_0;
     }

   retCode = RtlNtStatusToDosError(errCode);
   SetLastError(retCode);

   return 0xFFFFFFFF;
}


DWORD
STDCALL
WaitForMultipleObjects(DWORD nCount,
                       CONST HANDLE *lpHandles,
                       BOOL  bWaitAll,
                       DWORD dwMilliseconds)
{
    return WaitForMultipleObjectsEx( nCount, lpHandles, bWaitAll ? WaitAll : WaitAny, dwMilliseconds, FALSE );
}


DWORD
STDCALL
WaitForMultipleObjectsEx(DWORD nCount,
                         CONST HANDLE *lpHandles,
                         BOOL  bWaitAll,
                         DWORD dwMilliseconds,
                         BOOL  bAlertable)
{
   NTSTATUS  errCode;
   LARGE_INTEGER Time;
   PLARGE_INTEGER TimePtr;
   DWORD retCode;

   if (dwMilliseconds == INFINITE)
     {
        TimePtr = NULL;
     }
   else
     {
        Time.QuadPart = -10000 * dwMilliseconds;
        TimePtr = &Time;
     }

   errCode = NtWaitForMultipleObjects (nCount,
                                       (PHANDLE)lpHandles,
                                       (CINT)bWaitAll,
                                       (BOOLEAN)bAlertable,
                                       TimePtr);

   if (errCode == STATUS_TIMEOUT)
     {
         return WAIT_TIMEOUT;
     }
   else if ((errCode >= WAIT_OBJECT_0) &&
            (errCode <= WAIT_OBJECT_0 + nCount - 1))
     {
        return errCode;
     }

   retCode = RtlNtStatusToDosError(errCode);
   SetLastError(retCode);

   return 0xFFFFFFFF;
}

/* EOF */
