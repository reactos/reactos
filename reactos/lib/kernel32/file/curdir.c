/* $Id: curdir.c,v 1.20 2000/02/18 00:49:39 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBAL VARIABLES **********************************************************/

static WCHAR SystemDirectoryW[MAX_PATH];
static WCHAR WindowsDirectoryW[MAX_PATH];


/* FUNCTIONS *****************************************************************/

DWORD
STDCALL
GetCurrentDirectoryA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;

	/* initialize ansi string */
	AnsiString.Length = 0;
	AnsiString.MaximumLength = nBufferLength;
	AnsiString.Buffer = lpBuffer;

	/* allocate buffer for unicode string */
	UnicodeString.Length = 0;
	UnicodeString.MaximumLength = nBufferLength * sizeof(WCHAR);
	UnicodeString.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                        0,
	                                        UnicodeString.MaximumLength);

	/* get current directory */
	UnicodeString.Length = RtlGetCurrentDirectory_U (UnicodeString.MaximumLength,
	                                                 UnicodeString.Buffer);
	DPRINT("UnicodeString.Buffer %S\n", UnicodeString.Buffer);

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&AnsiString,
		                              &UnicodeString,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&AnsiString,
		                             &UnicodeString,
		                             FALSE);
	DPRINT("AnsiString.Buffer %s\n", AnsiString.Buffer);

	/* free unicode string */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             UnicodeString.Buffer);

	return AnsiString.Length;
}


DWORD
STDCALL
GetCurrentDirectoryW (
	DWORD	nBufferLength,
	LPWSTR	lpBuffer
	)
{
	ULONG Length;

	Length = RtlGetCurrentDirectory_U (nBufferLength,
	                                   lpBuffer);

	return (Length / sizeof (WCHAR));
}


WINBOOL
STDCALL
SetCurrentDirectoryA (
	LPCSTR	lpPathName
	)
{
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	NTSTATUS Status;

	RtlInitAnsiString (&AnsiString,
	                   (LPSTR)lpPathName);

	/* convert ansi (or oem) to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&UnicodeString,
		                              &AnsiString,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&UnicodeString,
		                             &AnsiString,
		                             TRUE);

	Status = RtlSetCurrentDirectory_U (&UnicodeString);

	RtlFreeUnicodeString (&UnicodeString);

	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


WINBOOL
STDCALL
SetCurrentDirectoryW (
	LPCWSTR	lpPathName
	)
{
	UNICODE_STRING UnicodeString;
	NTSTATUS Status;

	RtlInitUnicodeString (&UnicodeString,
	                      lpPathName);

	Status = RtlSetCurrentDirectory_U (&UnicodeString);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError (Status));
		return FALSE;
	}

	return TRUE;
}


DWORD STDCALL GetTempPathA (DWORD nBufferLength, LPSTR lpBuffer)
{
	WCHAR BufferW[MAX_PATH];
	DWORD retCode;
	UINT i;
	retCode = GetTempPathW(nBufferLength,BufferW);
	i = 0;
	while ((BufferW[i])!=0 && i < MAX_PATH)
	{
		lpBuffer[i] = (unsigned char)BufferW[i];
		i++;
	}
	lpBuffer[i] = 0;
	return retCode;
}

DWORD STDCALL GetTempPathW(DWORD nBufferLength, LPWSTR lpBuffer)
{
	WCHAR EnvironmentBufferW[MAX_PATH];
	UINT i;

	EnvironmentBufferW[0] = 0;
	i = GetEnvironmentVariableW(L"TMP",EnvironmentBufferW,MAX_PATH);
	if ( i==0 )
		i = GetEnvironmentVariableW(L"TEMP",EnvironmentBufferW,MAX_PATH);
		if ( i==0 )
			i = GetCurrentDirectoryW(MAX_PATH,EnvironmentBufferW);

	return i;
}

UINT STDCALL GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize)
{
   UINT uPathSize,i;

   if ( lpBuffer == NULL )
	return 0;
   uPathSize = lstrlenW(SystemDirectoryW);
   if ( uSize > uPathSize ) {
   	i = 0;
   	while ((SystemDirectoryW[i])!=0 && i < uSize)
     	{
		lpBuffer[i] = (unsigned char)SystemDirectoryW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
   }
   
   return uPathSize;
}

UINT STDCALL GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize)
{
   UINT uPathSize,i;
   if ( lpBuffer == NULL )
	return 0;
   uPathSize = lstrlenW(WindowsDirectoryW);
   if ( uSize > uPathSize ) {
   	i = 0;
   	while ((WindowsDirectoryW[i])!=0 && i < uSize)
     	{
		lpBuffer[i] = (unsigned char)WindowsDirectoryW[i];
		i++;
     	}
   	lpBuffer[i] = 0;
   }
   return uPathSize;
}

UINT
STDCALL
GetSystemDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
   UINT uPathSize;
   if ( lpBuffer == NULL )
	return 0;
   uPathSize = lstrlenW(SystemDirectoryW);
   if ( uSize > uPathSize )
   	lstrcpynW(lpBuffer,SystemDirectoryW,uPathSize);

   return uPathSize;
}

UINT
STDCALL
GetWindowsDirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
   UINT uPathSize;
   if ( lpBuffer == NULL )
	return 0;
   uPathSize = lstrlenW(WindowsDirectoryW);
   if ( uSize > uPathSize );
   	lstrcpynW(lpBuffer,WindowsDirectoryW,uPathSize);

   return uPathSize;
}

/* EOF */
