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
#include <wstring.h>
#include <ddk/ntddk.h>
#undef WIN32_LEAN_AND_MEAN

#ifndef ERROR_INVALID_FUNCTION
#error "ERROR_INVALID_FUNCTION undefined!"
#endif


/*---------------------------------------------------------------------
 *	RegCloseKey
 */
LONG
STDCALL
RegCloseKey(
	HKEY	hKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION;	/* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegConnectRegistryW
 */
LONG
STDCALL
RegConnectRegistry(
	LPWSTR	lpMachineName,
	HKEY	hKey,
	PHKEY	phkResult
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegDeleteKeyA
 */
LONG
STDCALL
RegDeleteKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegDeleteKeyW
 */
LONG
STDCALL
RegDeleteKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegDeleteValueA
 */
LONG
STDCALL
RegDeleteValueA(
	HKEY	hKey,
	LPCSTR	lpValueName
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegDeleteValueW
 */
LONG
STDCALL
RegDeleteValueW(
	HKEY	hKey,
	LPCWSTR	lpValueName
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegEnumKeyW
 */
LONG
STDCALL
RegEnumKeyW(
	HKEY	hKey,
	DWORD	dwIndex,
	LPWSTR	lpName,
	DWORD	cbName
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegEnumKeyExW
 */
LONG
STDCALL
RegEnumKeyExW(
	HKEY		hKey,
	DWORD		dwIndex,
	LPWSTR		lpName,
	LPDWORD		lpcbName,
	LPDWORD		lpReserved,
	LPWSTR		lpClass,
	LPDWORD		lpcbClass,
	PFILETIME	lpftLastWriteTime
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegEnumValueW
 */
LONG
STDCALL
RegEnumValueW(
	HKEY	hKey,
	DWORD	dwIndex,
	LPWSTR	lpValueName,
	LPDWORD	lpcbValueName,
	LPDWORD	lpReserved,
	LPDWORD	lpType,
	LPBYTE	lpData,
	LPDWORD	lpcbData
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegFlushKey
 */
LONG
STDCALL
RegFlushKey(
	HKEY	hKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegGetKeySecurity
 */
LONG
STDCALL
RegGetKeySecurity(
	HKEY			hKey,
	SECURITY_INFORMATION	SecurityInformation,
	PSECURITY_DESCRIPTOR	pSecurityDescriptor,
	LPDWORD			lpcbSecurityDescriptor               
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegLoadKeyA
 */
LONG
STDCALL
RegLoadKey(
	HKEY	hKey,
	LPCSTR	lpFile
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegLoadKeyW
 */
LONG
STDCALL
RegLoadKeyW(
	HKEY	hKey,
	LPCWSTR	lpFile
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}



/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegOpenKeyW
 *
 *	19991101 Ariadne
 */
LONG
STDCALL
RegOpenKeyW (
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	PHKEY	phkResult
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


/*---------------------------------------------------------------------
 *	RegOpenKeyExA
 */
LONG
STDCALL
RegOpenKeyExA(
	HKEY	hKey,
	LPCSTR	lpSubKey,
	DWORD	ulOptions,
	PHKEY	phkResult
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegOpenKeyExW
 */
LONG
STDCALL
RegOpenKeyExW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	DWORD	ulOptions,
	PHKEY	phkResult
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
	LPDWORD 	lpcbMaxSubKeyLen,
	LPDWORD 	lpcbMaxClassLen,
	LPDWORD 	lpcValues,
	LPDWORD 	lpcbMaxValueNameLen,
	LPDWORD 	lpcbMaxValueLen,
	PFILETIME	lpftLastWriteTime
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
	LPDWORD 	lpcbMaxSubKeyLen,
	LPDWORD 	lpcbMaxClassLen,
	LPDWORD 	lpcValues,
	LPDWORD 	lpcbMaxValueNameLen,
	LPDWORD 	lpcbMaxValueLen,
	PFILETIME	lpftLastWriteTime
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegQueryMultipleValuesW
 */
LONG
STDCALL
RegQueryMultipleValuesW(
	HKEY	hKey,
	PVALENT	val_list,
	DWORD	num_vals,
	LPWSTR	lpValueBuf,
	LPDWORD	ldwTotsize
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegQueryValueW
 */
LONG
STDCALL
RegQueryValueW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	LPWSTR	lpValue,
	PLONG	lpcbValue
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegReplaceKeyW
 */
LONG
STDCALL
RegReplaceKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	LPCWSTR	lpNewFile,
	LPCWSTR	lpOldFile
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegRestoreKeyW
 */
LONG
STDCALL
RegRestoreKeyW(
	HKEY	hKey,
	LPCWSTR	lpFile,
	DWORD	dwFlags
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegSaveKeyW
 */
LONG
STDCALL
RegSaveKeyW(
	HKEY			hKey,
	LPCWSTR			lpFile,
	LPSECURITY_ATTRIBUTES	lpSecurityAttributes 
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegSetKeySecurity
 */
LONG
STDCALL
RegSetKeySecurity(
	HKEY			hKey,
	SECURITY_INFORMATION	SecurityInformation,
	PSECURITY_DESCRIPTOR	pSecurityDescriptor
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegSetValueW
 */
LONG
STDCALL
RegSetValueW(
	HKEY	hKey,
	LPCWSTR	lpSubKey,
	DWORD	dwType,
	LPCWSTR	lpData,
	DWORD	cbData
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
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
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegSetValueExW
 */
LONG
STDCALL
RegSetValueExW(
	HKEY		hKey,
	LPCWSTR		lpValueName,
	DWORD		Reserved,
	DWORD		dwType,
	CONST BYTE	*lpData,
	DWORD		cbData
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegUnLoadKeyA
 */
LONG
STDCALL
RegUnLoadKeyA(
	HKEY	hKey,
	LPCSTR	lpSubKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/*---------------------------------------------------------------------
 *	RegUnLoadKeyW
 */
LONG
STDCALL
RegUnLoadKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey
	)
{
/* TO DO */
	return ERROR_INVALID_FUNCTION; /* FIXME */
}


/* EOF */
