/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/sec/rtlsec.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */
#include <windows.h>
#include <wstring.h>
#undef WIN32_LEAN_AND_MEAN

LONG
STDCALL
RegOpenKeyW (
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
    )
{
	NTSTATUS errCode;
	UNICODE_STRING SubKeyString;

	SubKeyString.Buffer = lpSubKey;
	SubKeyString.Length = wcslen(lpSubKey);
	SubKeyString.MaximumLength = SubKeyString.Length;

	ObjectAttributes.RootDirectory =  hKey;
	ObjectAttributes.ObjectName = &SubKeyString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE; 
	errCode = NtOpenKey(
		phkResult,
		GENERIC_ALL,
		&ObjectAttributes
		);
	if ( !NT_SUCCESS(errCode) ) {
		SetLastError(RtlNtStatusToDosError(errCode));
		return FALSE;
	}
	return TRUE;	
}