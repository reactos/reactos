/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  19990309 EA Stubs
 */
#include <windows.h>
#include <ddk/ntddk.h>
#include <wchar.h>

/************************************************************************
 *	RegCloseKey
 */
LONG
STDCALL
RegCloseKey(
	HKEY	hKey
	)
{
	if (!hKey)
		return ERROR_INVALID_HANDLE;

	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegConnectRegistryA
 */
LONG
STDCALL
RegConnectRegistryA(
	LPSTR	lpMachineName,
	HKEY	hKey,
	PHKEY	phkResult
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegCreateKeyA
 */
LONG
STDCALL
RegCreateKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	PHKEY	phkResult
	)
{
	return RegCreateKeyExA(hKey,
		lpSubKey,
		0,
		NULL,
		0,
		MAXIMUM_ALLOWED,
		NULL,
		phkResult,
		NULL);
}


/************************************************************************
 *	RegCreateKeyW
 */
LONG
STDCALL
RegCreateKeyW(
	HKEY	hKey,
	LPCWSTR lpSubKey,
	PHKEY	phkResult
	)
{
	return RegCreateKeyExW(hKey,
		lpSubKey,
		0,
		NULL,
		0,
		MAXIMUM_ALLOWED,
		NULL,
		phkResult,
		NULL);
}


/************************************************************************
 *	RegCreateKeyExA
 */
LONG
STDCALL
RegCreateKeyExA(
	HKEY			hKey,
	LPCSTR			lpSubKey,
	DWORD			Reserved,
	LPSTR			lpClass,
	DWORD			dwOptions,
	REGSAM			samDesired,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
	PHKEY			phkResult,
	LPDWORD			lpdwDisposition
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegCreateKeyExW
 */
LONG
STDCALL
RegCreateKeyExW(
	HKEY			hKey,
	LPCWSTR			lpSubKey,
	DWORD			Reserved,
	LPWSTR			lpClass,
	DWORD			dwOptions,
	REGSAM			samDesired,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes,
	PHKEY			phkResult,
	LPDWORD			lpdwDisposition
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegDeleteKeyA
 */
LONG
STDCALL
RegDeleteKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegDeleteKeyW
 */
LONG
STDCALL
RegDeleteKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegDeleteValueA
 */
LONG
STDCALL
RegDeleteValueA(
	HKEY	hKey,
	LPCSTR	lpValueName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegEnumKeyA
 */
LONG
STDCALL
RegEnumKeyA(
	HKEY	hKey,
	DWORD	dwIndex,
	LPSTR	lpName,
	DWORD	cbName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegEnumKeyExA
 */
LONG
STDCALL
RegEnumKeyExA(
	HKEY		hKey,
	DWORD		dwIndex,
	LPSTR		lpName,
	LPDWORD		lpcbName,
	LPDWORD		lpReserved,
	LPSTR		lpClass,
	LPDWORD		lpcbClass,
	PFILETIME	lpftLastWriteTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegEnumValueA
 */
LONG
STDCALL
RegEnumValueA(
	HKEY	hKey,
	DWORD	dwIndex,
	LPSTR	lpValueName,
	LPDWORD	lpcbValueName,
	LPDWORD	lpReserved,
	LPDWORD	lpType,
	LPBYTE	lpData,
	LPDWORD	lpcbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegFlushKey
 */
LONG
STDCALL
RegFlushKey(
	HKEY	hKey
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegGetKeySecurity
 */
#if 0
LONG
STDCALL
RegGetKeySecurity (
	HKEY			hKey,
	SECURITY_INFORMATION	SecurityInformation,	/* FIXME: ULONG ? */
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPDWORD			lpcbSecurityDescriptor
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}
#endif


/************************************************************************
 *	RegLoadKeyA
 */
LONG
STDCALL
RegLoadKey(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	LPCSTR	lpFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegLoadKeyW
 */
LONG
STDCALL
RegLoadKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	LPCWSTR	lpFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegNotifyChangeKeyValue
 */
LONG
STDCALL
RegNotifyChangeKeyValue(
	HKEY	hKey,
	BOOL	bWatchSubtree,
	DWORD	dwNotifyFilter,
	HANDLE	hEvent,
	BOOL	fAsynchronous
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}



/************************************************************************
 *	RegOpenKeyA
 */
LONG
STDCALL
RegOpenKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	PHKEY	phkResult
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegOpenKeyW
 *
 *	19981101 Ariadne
 *	19990525 EA
 */
LONG
STDCALL
RegOpenKeyW (
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	PHKEY	phkResult
	)
{
	NTSTATUS		errCode;
	UNICODE_STRING		SubKeyString;
	OBJECT_ATTRIBUTES	ObjectAttributes;

	SubKeyString.Buffer = (LPWSTR)lpSubKey;
	SubKeyString.Length = wcslen(lpSubKey);
	SubKeyString.MaximumLength = SubKeyString.Length;

	ObjectAttributes.RootDirectory =  hKey;
	ObjectAttributes.ObjectName = & SubKeyString;
	ObjectAttributes.Attributes = OBJ_CASE_INSENSITIVE; 
	errCode = NtOpenKey(
			phkResult,
			GENERIC_ALL,
			& ObjectAttributes
			);
	if ( !NT_SUCCESS(errCode) )
	{
		LONG LastError = RtlNtStatusToDosError(errCode);
		
		SetLastError(LastError);
		return LastError;
	}
	return ERROR_SUCCESS;
}


/************************************************************************
 *	RegOpenKeyExA
 */
LONG
STDCALL
RegOpenKeyExA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	DWORD	ulOptions,
	REGSAM	samDesired,
	PHKEY	phkResult
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegOpenKeyExW
 */
LONG
STDCALL
RegOpenKeyExW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	DWORD	ulOptions,
	REGSAM	samDesired,
	PHKEY	phkResult
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryInfoKeyA
 */
LONG
STDCALL
RegQueryInfoKeyA(
	HKEY		hKey,
	LPSTR		lpClass,
	LPDWORD		lpcbClass,
	LPDWORD		lpReserved,
	LPDWORD		lpcSubKeys,
	LPDWORD		lpcbMaxSubKeyLen,
	LPDWORD		lpcbMaxClassLen,
	LPDWORD		lpcValues,
	LPDWORD		lpcbMaxValueNameLen,
	LPDWORD		lpcbMaxValueLen,
	LPDWORD		lpcbSecurityDescriptor,
	PFILETIME	lpftLastWriteTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryInfoKeyW
 */
LONG
STDCALL
RegQueryInfoKeyW(
	HKEY		hKey,
	LPWSTR		lpClass,
	LPDWORD		lpcbClass,
	LPDWORD		lpReserved,
	LPDWORD		lpcSubKeys,
	LPDWORD		lpcbMaxSubKeyLen,
	LPDWORD		lpcbMaxClassLen,
	LPDWORD		lpcValues,
	LPDWORD		lpcbMaxValueNameLen,
	LPDWORD		lpcbMaxValueLen,
	LPDWORD		lpcbSecurityDescriptor,
	PFILETIME	lpftLastWriteTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryMultipleValuesA
 */
LONG
STDCALL
RegQueryMultipleValuesA(
	HKEY	hKey,
	PVALENT	val_list,
	DWORD	num_vals,
	LPSTR	lpValueBuf,
	LPDWORD	ldwTotsize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryValueA
 */
LONG
STDCALL
RegQueryValueA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	LPSTR	lpValue,
	PLONG	lpcbValue
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryValueExA
 */
LONG
STDCALL
RegQueryValueExA(
	HKEY	hKey,
	LPSTR	lpValueName,
	LPDWORD	lpReserved,
	LPDWORD	lpType,
	LPBYTE	lpData,
	LPDWORD	lpcbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegQueryValueExW
 */
LONG
STDCALL
RegQueryValueExW(
	HKEY	hKey,
	LPWSTR	lpValueName,
	LPDWORD	lpReserved,
	LPDWORD	lpType,
	LPBYTE	lpData,
	LPDWORD	lpcbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegReplaceKeyA
 */
LONG
STDCALL
RegReplaceKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	LPCSTR	lpNewFile,
	LPCSTR	lpOldFile
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegRestoreKeyA
 */
LONG
STDCALL
RegRestoreKeyA(
	HKEY	hKey,
	LPCSTR	lpFile,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegSaveKeyA
 */
LONG
STDCALL
RegSaveKeyA(
	HKEY			hKey,
	LPCSTR			lpFile,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes 
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegSetKeySecurity
 */
#if 0
LONG
STDCALL
RegSetKeySecurity(
	HKEY			hKey,
	SECURITY_INFORMATION	SecurityInformation,	/* FIXME: ULONG? */
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}
#endif

/************************************************************************
 *	RegSetValueA
 */
LONG
STDCALL
RegSetValueA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	DWORD	dwType,
	LPCSTR	lpData,
	DWORD	cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegSetValueExA
 */
LONG
STDCALL
RegSetValueExA(
	HKEY		hKey,
	LPCSTR		lpValueName,
	DWORD		Reserved,
	DWORD		dwType,
	CONST BYTE	*lpData,
	DWORD		cbData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *	RegUnLoadKeyA
 */
LONG
STDCALL
RegUnLoadKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}


/* EOF */
