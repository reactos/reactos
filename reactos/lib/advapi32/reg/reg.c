/* $Id: reg.c,v 1.8 2000/09/05 23:00:27 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  19990309 EA Stubs
 */
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MAX_DEFAULT_HANDLES   7

static CRITICAL_SECTION HandleTableCS;
static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];


/* PROTOTYPES ****************************************************************/

static NTSTATUS MapDefaultKey (PHKEY ParentKey, HKEY Key);
static VOID CloseDefaultHandles(VOID);

static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);


/* FUNCTIONS *****************************************************************/

/************************************************************************
 *	RegInitDefaultHandles
 */

BOOL
RegInitialize (VOID)
{
   DPRINT1("RegInitialize()\n");

   RtlZeroMemory (DefaultHandleTable,
		  MAX_DEFAULT_HANDLES * sizeof(HANDLE));

   RtlInitializeCriticalSection(&HandleTableCS);
   return TRUE;
}


/************************************************************************
 *	RegInit
 */
BOOL
RegCleanup(VOID)
{
   DPRINT1("RegCleanup()\n");

   CloseDefaultHandles();
   RtlDeleteCriticalSection(&HandleTableCS);
   return TRUE;
}


static NTSTATUS
MapDefaultKey (PHKEY ParentKey,
               HKEY Key)
{
   PHANDLE Handle;
   ULONG Index;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT1("MapDefaultKey (Key %x)\n", Key);

   if (((ULONG)Key & 0xF0000000) != 0x80000000)
     {
        *ParentKey = Key;
        return STATUS_SUCCESS;
     }

   /* Handle special cases here */
   Index = (ULONG)Key & 0x0FFFFFFF;
DPRINT1("Index %x\n", Index);

   if (Index >= MAX_DEFAULT_HANDLES)
     return STATUS_INVALID_PARAMETER;

   RtlEnterCriticalSection(&HandleTableCS);

   Handle = &DefaultHandleTable[Index];
   if (*Handle == NULL)
     {
        /* create/open the default handle */
        switch (Index)
          {
            case 2: /*HKEY_LOCAL_MACHINE */
              Status = OpenLocalMachineKey(Handle);
              break;

            default:
              DPRINT1("MapDefaultHandle() no handle creator\n");
              Status = STATUS_INVALID_PARAMETER;
          }
     }

   RtlLeaveCriticalSection(&HandleTableCS);

DPRINT1("Status %x\n", Status);

   if (NT_SUCCESS(Status))
     {
        *ParentKey = (HKEY)*Handle;
     }

   return Status;
}


static VOID CloseDefaultHandles(VOID)
{
   ULONG i;

   RtlEnterCriticalSection(&HandleTableCS);

   for (i = 0; i < MAX_DEFAULT_HANDLES; i++)
     {
        if (DefaultHandleTable[i] != NULL)
          {
//            NtClose (DefaultHandleTable[i]);
             DefaultHandleTable[i] = NULL;
          }
     }

   RtlLeaveCriticalSection(&HandleTableCS);
}


static NTSTATUS
OpenLocalMachineKey (PHANDLE KeyHandle)
{
   OBJECT_ATTRIBUTES Attributes;
   UNICODE_STRING KeyName;

   DPRINT1("OpenLocalMachineKey()\n");

   RtlInitUnicodeString(&KeyName,
                        L"\\Registry\\Machine");

   InitializeObjectAttributes(&Attributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   return (NtOpenKey (KeyHandle, 0x200000, &Attributes));
}


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
		KEY_ALL_ACCESS,
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
		KEY_ALL_ACCESS,
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
	UNICODE_STRING SubKeyString;
	UNICODE_STRING ClassString;
	OBJECT_ATTRIBUTES Attributes;
	NTSTATUS Status;
	HKEY ParentKey;

	DPRINT1("RegCreateKeyExW() called\n");

	/* get the real parent key */
	Status = MapDefaultKey (&ParentKey, hKey);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError(Status));
		return (RtlNtStatusToDosError(Status));
	}

	DPRINT1("ParentKey %x\n", (ULONG)ParentKey);

	RtlInitUnicodeString (&ClassString, lpClass);
	RtlInitUnicodeString (&SubKeyString, lpSubKey);

	InitializeObjectAttributes (&Attributes,
				    &SubKeyString,
				    OBJ_CASE_INSENSITIVE,
				    (HANDLE)ParentKey,
				    (PSECURITY_DESCRIPTOR)lpSecurityAttributes);

	Status = NtCreateKey (phkResult,
			      samDesired,
			      &Attributes,
			      0,
			      (lpClass == NULL)? NULL : &ClassString,
			      dwOptions,
			      (PULONG)lpdwDisposition);
	DPRINT1("Status %x\n", Status);
	if (!NT_SUCCESS(Status))
	{
		SetLastError (RtlNtStatusToDosError(Status));
		return (RtlNtStatusToDosError(Status));
	}
	DPRINT1("Returned handle %x\n", (ULONG)*phkResult);

	return ERROR_SUCCESS;
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
	SubKeyString.Length = wcslen(SubKeyString.Buffer);
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
