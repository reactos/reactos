/* $Id: curdir.c,v 1.35 2003/03/23 10:46:59 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/curdir.c
 * PURPOSE:         Current directory functions
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */


/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBAL VARIABLES **********************************************************/

UNICODE_STRING SystemDirectory;
UNICODE_STRING WindowsDirectory;


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

	Length = RtlGetCurrentDirectory_U (nBufferLength * sizeof(WCHAR),
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
		SetLastErrorByStatus (Status);
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
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


DWORD
STDCALL
GetTempPathA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
	UNICODE_STRING UnicodeString;
	ANSI_STRING AnsiString;

	AnsiString.Length = 0;
	AnsiString.MaximumLength = nBufferLength;
	AnsiString.Buffer = lpBuffer;

	/* initialize allocate unicode string */
	UnicodeString.Length = 0;
	UnicodeString.MaximumLength = nBufferLength * sizeof(WCHAR);
	UnicodeString.Buffer = RtlAllocateHeap (RtlGetProcessHeap(),
	                                        0,
	                                        UnicodeString.MaximumLength);

	UnicodeString.Length = GetTempPathW (nBufferLength,
	                                     UnicodeString.Buffer) * sizeof(WCHAR);

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
		RtlUnicodeStringToAnsiString (&AnsiString,
		                              &UnicodeString,
		                              FALSE);
	else
		RtlUnicodeStringToOemString (&AnsiString,
		                             &UnicodeString,
		                             FALSE);

	/* free unicode string buffer */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             UnicodeString.Buffer);

	return AnsiString.Length;
}


DWORD
STDCALL
GetTempPathW (
	DWORD	nBufferLength,
	LPWSTR	lpBuffer
	)
{
	UNICODE_STRING Name;
	UNICODE_STRING Value;
	NTSTATUS Status;

	Value.Length = 0;
	Value.MaximumLength = (nBufferLength - 1) * sizeof(WCHAR);
	Value.Buffer = lpBuffer;

	RtlInitUnicodeStringFromLiteral (&Name,
	                      L"TMP");

	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &Name,
	                                        &Value);
	if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
	{
		RtlInitUnicodeStringFromLiteral (&Name,
		                      L"TEMP");

		Status = RtlQueryEnvironmentVariable_U (NULL,
		                                        &Name,
		                                        &Value);

		if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
		{
			Value.Length = RtlGetCurrentDirectory_U (Value.MaximumLength,
			                                         Value.Buffer);
		}
	}

	if (NT_SUCCESS(Status))
	{
		lpBuffer[Value.Length / sizeof(WCHAR)] = L'\\';
		lpBuffer[Value.Length / sizeof(WCHAR) + 1] = 0;
	}

	return Value.Length / sizeof(WCHAR) + 1;
}


UINT
STDCALL
GetSystemDirectoryA (
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
	ANSI_STRING String;
	ULONG Length;
	NTSTATUS Status;

	if (lpBuffer == NULL)
		return 0;

	Length = RtlUnicodeStringToAnsiSize (&SystemDirectory);	  //len of ansi str incl. nullchar

	if (uSize >= Length){
		String.Length = 0;
		String.MaximumLength = uSize;
		String.Buffer = lpBuffer;

		/* convert unicode string to ansi (or oem) */
		if (bIsFileApiAnsi)
			Status = RtlUnicodeStringToAnsiString (&String,
			                              &SystemDirectory,
			                              FALSE);
		else
			Status = RtlUnicodeStringToOemString (&String,
			                             &SystemDirectory,
			                             FALSE);
		if (!NT_SUCCESS(Status) )
			return 0;

		return Length-1;  //good: ret chars excl. nullchar

	}

	return Length;	 //bad: ret space needed incl. nullchar
}


UINT
STDCALL
GetSystemDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	if (lpBuffer == NULL)
		return 0;

	Length = SystemDirectory.Length / sizeof (WCHAR);
	if (uSize > Length)	{
		memmove (lpBuffer,
		         SystemDirectory.Buffer,
		         SystemDirectory.Length);
		lpBuffer[Length] = 0;

		return Length;	  //good: ret chars excl. nullchar
	}

	return Length+1;	 //bad: ret space needed incl. nullchar
}


UINT
STDCALL
GetWindowsDirectoryA (
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
	ANSI_STRING String;
	ULONG Length;
	NTSTATUS Status;

	if (lpBuffer == NULL)
		return 0;

	Length = RtlUnicodeStringToAnsiSize (&WindowsDirectory); //len of ansi str incl. nullchar
	
	if (uSize >= Length){

		String.Length = 0;
		String.MaximumLength = uSize;
		String.Buffer = lpBuffer;

		/* convert unicode string to ansi (or oem) */
		if (bIsFileApiAnsi)
			Status = RtlUnicodeStringToAnsiString (&String,
			                              &WindowsDirectory,
			                              FALSE);
		else
			Status = RtlUnicodeStringToOemString (&String,
			                             &WindowsDirectory,
			                             FALSE);

		if (!NT_SUCCESS(Status))
			return 0;

		return Length-1;	//good: ret chars excl. nullchar
	}

	return Length;	//bad: ret space needed incl. nullchar
}


UINT
STDCALL
GetWindowsDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	if (lpBuffer == NULL)
		return 0;

	Length = WindowsDirectory.Length / sizeof (WCHAR);
	if (uSize > Length)
	{
		memmove (lpBuffer,
		         WindowsDirectory.Buffer,
		         WindowsDirectory.Length);
		lpBuffer[Length] = 0;

		return Length;	  //good: ret chars excl. nullchar
	}

	return Length+1;	//bad: ret space needed incl. nullchar
}

/* EOF */
