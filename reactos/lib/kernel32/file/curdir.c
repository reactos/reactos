/* $Id$
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
#include "../include/debug.h"


/* GLOBAL VARIABLES **********************************************************/

UNICODE_STRING SystemDirectory;
UNICODE_STRING WindowsDirectory;


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
DWORD
STDCALL
GetCurrentDirectoryA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	ULONG Length;

	/* allocate buffer for unicode string */
	UnicodeString.Length = 0;
	UnicodeString.MaximumLength = nBufferLength * sizeof(WCHAR);
	if (nBufferLength > 0)
	{
		UnicodeString.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
		                                        0,
		                                        UnicodeString.MaximumLength);

		/* initialize ansi string */
		AnsiString.Length = 0;
		AnsiString.MaximumLength = nBufferLength;
		AnsiString.Buffer = lpBuffer;
	}
	else
	{
		UnicodeString.Buffer = NULL;
	}

	/* get current directory */
	UnicodeString.Length = RtlGetCurrentDirectory_U (UnicodeString.MaximumLength,
	                                                 UnicodeString.Buffer);
	DPRINT("UnicodeString.Buffer %wZ\n", &UnicodeString);

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
	{
		Length = RtlUnicodeStringToAnsiSize (&UnicodeString);
		if (Length > nBufferLength)
		{
			RtlFreeHeap (RtlGetProcessHeap (),
			             0,
			             UnicodeString.Buffer);
			return Length-1;
		}
		RtlUnicodeStringToAnsiString (&AnsiString,
		                              &UnicodeString,
		                              FALSE);
	}
	else
	{
		Length = RtlUnicodeStringToOemSize (&UnicodeString);
		if (Length > nBufferLength)
		{
			RtlFreeHeap (RtlGetProcessHeap (),
			             0,
			             UnicodeString.Buffer);
			return Length-1;
		}
		RtlUnicodeStringToOemString (&AnsiString,
		                             &UnicodeString,
		                             FALSE);
	}
	DPRINT("AnsiString.Buffer %s\n", AnsiString.Buffer);

	/* free unicode string */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             UnicodeString.Buffer);

	return AnsiString.Length;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
BOOL
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


/*
 * @implemented
 */
DWORD
STDCALL
GetTempPathA (
	DWORD	nBufferLength,
	LPSTR	lpBuffer
	)
{
	UNICODE_STRING UnicodeString;
	ANSI_STRING AnsiString;
	DWORD Length;

	AnsiString.Length = 0;
	AnsiString.MaximumLength = nBufferLength;
	AnsiString.Buffer = lpBuffer;

	/* initialize allocate unicode string */
	UnicodeString.Length = 0;
	if(nBufferLength > 0)
	{
	  UnicodeString.MaximumLength = (nBufferLength + 1) * sizeof(WCHAR);
	  UnicodeString.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                          0,
	                                          UnicodeString.MaximumLength);
	  if (UnicodeString.Buffer == NULL)
	  {
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	  }
	}
	else
	{
          UnicodeString.MaximumLength = 0;
          UnicodeString.Buffer = NULL;
	}

	Length = GetTempPathW (nBufferLength,
	                       UnicodeString.Buffer);

	if (nBufferLength >= Length)
	{
                /* only touch the buffer if the supplied buffer length is at least
                   the length that GetTempPathW returned! */
		UnicodeString.Length = Length * sizeof(WCHAR);

		/* convert unicode string to ansi (or oem) */
		if (bIsFileApiAnsi)
			RtlUnicodeStringToAnsiString (&AnsiString,
			                              &UnicodeString,
			                              FALSE);
		else
			RtlUnicodeStringToOemString (&AnsiString,
			                             &UnicodeString,
			                             FALSE);
	}

	/* free unicode string buffer */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             UnicodeString.Buffer);

	return Length;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetTempPathW (
	DWORD	nBufferLength,
	LPWSTR	lpBuffer
	)
{
	UNICODE_STRING Name;
	PUNICODE_STRING Value;
	PTEB Teb;
	DWORD Length;
	NTSTATUS Status;

	Teb = NtCurrentTeb();
	Teb->StaticUnicodeString.Length = 0;
	Teb->StaticUnicodeString.MaximumLength = MAX_PATH * sizeof(WCHAR);
	Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;
	Value = &Teb->StaticUnicodeString;

	RtlRosInitUnicodeStringFromLiteral (&Name,
	                                    L"TMP");

	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &Name,
	                                        Value);
	if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
	{
		RtlRosInitUnicodeStringFromLiteral (&Name,
		                                    L"TEMP");

		Status = RtlQueryEnvironmentVariable_U (NULL,
		                                        &Name,
		                                        Value);
		if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL)
		{
			Value->Length = RtlGetCurrentDirectory_U(Value->MaximumLength,
			                                         Value->Buffer);
		}
	}

	if (!NT_SUCCESS(Status))
	{
		SetLastError(RtlNtStatusToDosError(Status));
		return 0;
	}

	Length = Value->Length / sizeof(WCHAR) + 1;
	if (nBufferLength < Value->Length / sizeof(WCHAR) + 2)
		Length++;

	if (lpBuffer != NULL)
	{
		if (nBufferLength < Value->Length / sizeof(WCHAR) + 2)
		{
			memcpy (lpBuffer,
			        Value->Buffer,
			        nBufferLength * sizeof(WCHAR));
		}
		else
		{
			memcpy (lpBuffer,
			        Value->Buffer,
			        Value->Length);
			lpBuffer[Value->Length / sizeof(WCHAR)] = L'\\';
			lpBuffer[Value->Length / sizeof(WCHAR) + 1] = 0;
		}
	}

	return Length;
}


/*
 * @implemented
 */
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

	Length = RtlUnicodeStringToAnsiSize (&SystemDirectory);	  //len of ansi str incl. nullchar

	if (lpBuffer == NULL)
		return Length;

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


/*
 * @implemented
 */
UINT
STDCALL
GetSystemDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	Length = SystemDirectory.Length / sizeof (WCHAR);

	if (lpBuffer == NULL)
		return Length + 1;

	if (uSize > Length)	{
		memmove (lpBuffer,
		         SystemDirectory.Buffer,
		         SystemDirectory.Length);
		lpBuffer[Length] = 0;

		return Length;	  //good: ret chars excl. nullchar
	}

	return Length+1;	 //bad: ret space needed incl. nullchar
}

/*
 * @implemented
 */
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

	Length = RtlUnicodeStringToAnsiSize (&WindowsDirectory); //len of ansi str incl. nullchar
	
	if (lpBuffer == NULL)
		return Length;

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


/*
 * @implemented
 */
UINT
STDCALL
GetWindowsDirectoryW (
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
	ULONG Length;

	Length = WindowsDirectory.Length / sizeof (WCHAR);

	if (lpBuffer == NULL)
		return Length + 1;

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

/*
 * @implemented
 */
UINT
STDCALL
GetSystemWindowsDirectoryA(
	LPSTR	lpBuffer,
	UINT	uSize
	)
{
    return GetWindowsDirectoryA( lpBuffer, uSize );
}

/*
 * @implemented
 */
UINT
STDCALL
GetSystemWindowsDirectoryW(
	LPWSTR	lpBuffer,
	UINT	uSize
	)
{
    return GetWindowsDirectoryW( lpBuffer, uSize );
}

/* EOF */
