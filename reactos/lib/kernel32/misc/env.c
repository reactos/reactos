/* $Id: env.c,v 1.13 2002/04/01 22:08:20 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <wchar.h>
#include <string.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>


/* FUNCTIONS ******************************************************************/

DWORD
STDCALL
GetEnvironmentVariableA (
	LPCSTR	lpName,
	LPSTR	lpBuffer,
	DWORD	nSize
	)
{
	ANSI_STRING VarName;
	ANSI_STRING VarValue;
	UNICODE_STRING VarNameU;
	UNICODE_STRING VarValueU;
	NTSTATUS Status;

	/* initialize unicode variable name string */
	RtlInitAnsiString (&VarName,
	                   (LPSTR)lpName);
	RtlAnsiStringToUnicodeString (&VarNameU,
	                              &VarName,
	                              TRUE);

	/* initialize ansi variable value string */
	VarValue.Length = 0;
	VarValue.MaximumLength = nSize;
	VarValue.Buffer = lpBuffer;

	/* initialize unicode variable value string and allocate buffer */
	VarValueU.Length = 0;
	VarValueU.MaximumLength = nSize * sizeof(WCHAR);
	VarValueU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                    0,
	                                    VarValueU.MaximumLength);

	/* get unicode environment variable */
	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &VarNameU,
	                                        &VarValueU);
	if (!NT_SUCCESS(Status))
	{
		/* free unicode buffer */
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             VarValueU.Buffer);

		/* free unicode variable name string */
		RtlFreeUnicodeString (&VarNameU);

		SetLastErrorByStatus (Status);
		if (Status == STATUS_BUFFER_TOO_SMALL)
		{
			return VarValueU.Length / sizeof(WCHAR) + 1;
		}
		else
		{
			return 0;
		}
	}

	/* convert unicode value string to ansi */
	RtlUnicodeStringToAnsiString (&VarValue,
	                              &VarValueU,
	                              FALSE);

	/* free unicode buffer */
	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             VarValueU.Buffer);

	/* free unicode variable name string */
	RtlFreeUnicodeString (&VarNameU);

	return (VarValueU.Length / sizeof(WCHAR));
}


DWORD
STDCALL
GetEnvironmentVariableW (
	LPCWSTR	lpName,
	LPWSTR	lpBuffer,
	DWORD	nSize
	)
{
	UNICODE_STRING VarName;
	UNICODE_STRING VarValue;
	NTSTATUS Status;

	RtlInitUnicodeString (&VarName,
	                      lpName);

	VarValue.Length = 0;
	VarValue.MaximumLength = nSize * sizeof(WCHAR);
	VarValue.Buffer = lpBuffer;

	Status = RtlQueryEnvironmentVariable_U (NULL,
	                                        &VarName,
	                                        &VarValue);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		if (Status == STATUS_BUFFER_TOO_SMALL)
		{
			return (VarValue.Length / sizeof(WCHAR)) + 1;
		}
		else
		{
			return 0;
		}
	}

	return (VarValue.Length / sizeof(WCHAR));
}


WINBOOL
STDCALL
SetEnvironmentVariableA (
	LPCSTR	lpName,
	LPCSTR	lpValue
	)
{
	ANSI_STRING VarName;
	ANSI_STRING VarValue;
	UNICODE_STRING VarNameU;
	UNICODE_STRING VarValueU;
	NTSTATUS Status;

	DPRINT("SetEnvironmentVariableA(Name '%s', Value '%s')\n", lpName, lpValue);

	RtlInitAnsiString (&VarName,
	                   (LPSTR)lpName);
	RtlAnsiStringToUnicodeString (&VarNameU,
	                              &VarName,
	                              TRUE);

	RtlInitAnsiString (&VarValue,
	                   (LPSTR)lpValue);
	RtlAnsiStringToUnicodeString (&VarValueU,
	                              &VarValue,
	                              TRUE);

	Status = RtlSetEnvironmentVariable (NULL,
	                                    &VarNameU,
	                                    &VarValueU);

	RtlFreeUnicodeString (&VarNameU);
	RtlFreeUnicodeString (&VarValueU);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


WINBOOL
STDCALL
SetEnvironmentVariableW (
	LPCWSTR	lpName,
	LPCWSTR	lpValue
	)
{
	UNICODE_STRING VarName;
	UNICODE_STRING VarValue;
	NTSTATUS Status;

	DPRINT("SetEnvironmentVariableW(Name '%S', Value '%S')\n", lpName, lpValue);

	RtlInitUnicodeString (&VarName,
	                      lpName);

	RtlInitUnicodeString (&VarValue,
	                      lpValue);

	Status = RtlSetEnvironmentVariable (NULL,
	                                    &VarName,
	                                    &VarValue);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}

	return TRUE;
}


DWORD
STDCALL
GetVersion(VOID)
{
	DWORD Version = 0;
	OSVERSIONINFO VersionInformation;
	GetVersionExW(&VersionInformation);

	Version |= ( VersionInformation.dwMajorVersion << 8 );
	Version |= VersionInformation.dwMinorVersion;

	Version |= ( VersionInformation.dwPlatformId << 16 );

	return Version;
}


WINBOOL
STDCALL
GetVersionExW(
    LPOSVERSIONINFO lpVersionInformation
    )
{
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	lpVersionInformation->dwMajorVersion = 4;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwBuildNumber = 12;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	lstrcpyW((WCHAR *)lpVersionInformation->szCSDVersion,L"Ariadne was here...");
	return TRUE;
}


WINBOOL
STDCALL
GetVersionExA(
    LPOSVERSIONINFO lpVersionInformation
    )
{
	lpVersionInformation->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	lpVersionInformation->dwMajorVersion = 4;
	lpVersionInformation->dwMinorVersion = 0;
	lpVersionInformation->dwBuildNumber = 12;
	lpVersionInformation->dwPlatformId = VER_PLATFORM_WIN32_NT;
	lstrcpyA((char *)lpVersionInformation->szCSDVersion,"ReactOs Pre-Alpha");
	return TRUE;
}


LPSTR
STDCALL
GetEnvironmentStringsA (
	VOID
	)
{
	UNICODE_STRING UnicodeString;
	ANSI_STRING AnsiString;
	PWCHAR EnvU;
	PWCHAR PtrU;
	ULONG  Length;
	PCHAR EnvPtr = NULL;

	EnvU = (PWCHAR)(NtCurrentPeb ()->ProcessParameters->Environment);

	if (EnvU == NULL)
		return NULL;

	if (*EnvU == 0)
		return NULL;

	/* get environment size */
	PtrU = EnvU;
	while (*PtrU)
	{
		while (*PtrU)
			PtrU++;
		PtrU++;
	}
	Length = (ULONG)(PtrU - EnvU);
	DPRINT("Length %lu\n", Length);

	/* allocate environment buffer */
	EnvPtr = RtlAllocateHeap (RtlGetProcessHeap (),
	                          0,
	                          Length + 1);
	DPRINT("EnvPtr %p\n", EnvPtr);

	/* convert unicode environment to ansi */
	UnicodeString.MaximumLength = Length * sizeof(WCHAR) + sizeof(WCHAR);
	UnicodeString.Buffer = EnvU;

	AnsiString.MaximumLength = Length + 1;
	AnsiString.Length = 0;
	AnsiString.Buffer = EnvPtr;

	DPRINT ("UnicodeString.Buffer \'%S\'\n", UnicodeString.Buffer);

	while (*(UnicodeString.Buffer))
	{
		UnicodeString.Length = wcslen (UnicodeString.Buffer) * sizeof(WCHAR);
		UnicodeString.MaximumLength = UnicodeString.Length + sizeof(WCHAR);
		if (UnicodeString.Length > 0)
		{
			AnsiString.Length = 0;
			AnsiString.MaximumLength = Length + 1 - (AnsiString.Buffer - EnvPtr);

			RtlUnicodeStringToAnsiString (&AnsiString,
			                              &UnicodeString,
			                              FALSE);

			AnsiString.Buffer += (AnsiString.Length + 1);
			UnicodeString.Buffer += ((UnicodeString.Length / sizeof(WCHAR)) + 1);
		}
	}
	*(AnsiString.Buffer) = 0;

	return EnvPtr;
}


LPWSTR
STDCALL
GetEnvironmentStringsW (
	VOID
	)
{
	return (LPWSTR)(NtCurrentPeb ()->ProcessParameters->Environment);
}


WINBOOL
STDCALL
FreeEnvironmentStringsA (
	LPSTR	EnvironmentStrings
	)
{
	if (EnvironmentStrings == NULL)
		return FALSE;

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             EnvironmentStrings);

	return TRUE;
}


WINBOOL
STDCALL
FreeEnvironmentStringsW (
	LPWSTR	EnvironmentStrings
	)
{
	return TRUE;
}


DWORD
STDCALL
ExpandEnvironmentStringsA (
	LPCSTR	lpSrc,
	LPSTR	lpDst,
	DWORD	nSize
	)
{
	ANSI_STRING Source;
	ANSI_STRING Destination;
	UNICODE_STRING SourceU;
	UNICODE_STRING DestinationU;
	NTSTATUS Status;
	ULONG Length = 0;

	RtlInitAnsiString (&Source,
	                   (LPSTR)lpSrc);
	RtlAnsiStringToUnicodeString (&SourceU,
	                              &Source,
	                              TRUE);

	Destination.Length = 0;
	Destination.MaximumLength = nSize;
	Destination.Buffer = lpDst,

	DestinationU.Length = 0;
	DestinationU.MaximumLength = nSize * sizeof(WCHAR);
	DestinationU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                                       0,
	                                       DestinationU.MaximumLength);

	Status = RtlExpandEnvironmentStrings_U (NULL,
	                                        &SourceU,
	                                        &DestinationU,
	                                        &Length);

	RtlFreeUnicodeString (&SourceU);

	if (!NT_SUCCESS(Status))
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             DestinationU.Buffer);
		SetLastErrorByStatus (Status);
		return 0;
	}

	RtlUnicodeStringToAnsiString (&Destination,
	                              &DestinationU,
	                              FALSE);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             DestinationU.Buffer);

	return (Length / sizeof(WCHAR));
}


DWORD
STDCALL
ExpandEnvironmentStringsW (
	LPCWSTR	lpSrc,
	LPWSTR	lpDst,
	DWORD	nSize
	)
{
	UNICODE_STRING Source;
	UNICODE_STRING Destination;
	NTSTATUS Status;
	ULONG Length = 0;

	RtlInitUnicodeString (&Source,
	                      (LPWSTR)lpSrc);

	Destination.Length = 0;
	Destination.MaximumLength = nSize * sizeof(WCHAR);
	Destination.Buffer = lpDst;

	Status = RtlExpandEnvironmentStrings_U (NULL,
	                                        &Source,
	                                        &Destination,
	                                        &Length);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return 0;
	}

	return (Length / sizeof(WCHAR));
}

/* EOF */
