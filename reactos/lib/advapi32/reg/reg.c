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

/************************************************************************
 *	RegCloseKey
 */
LONG
STDCALL
RegCloseKey(
	HKEY	hKey
	)
{
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
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
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
	LPCSTR	lpValueName,
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

