/* $Id: reg.c,v 1.11 2000/09/27 01:21:27 ekohl Exp $
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
static VOID CloseDefaultKeys(VOID);

static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);


/* FUNCTIONS *****************************************************************/

/************************************************************************
 *	RegInitDefaultHandles
 */

BOOL
RegInitialize (VOID)
{
   DPRINT("RegInitialize()\n");

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
   DPRINT("RegCleanup()\n");

   CloseDefaultKeys();
   RtlDeleteCriticalSection(&HandleTableCS);
   return TRUE;
}


static NTSTATUS
MapDefaultKey (PHKEY RealKey,
               HKEY Key)
{
   PHANDLE Handle;
   ULONG Index;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("MapDefaultKey (Key %x)\n", Key);

   if (((ULONG)Key & 0xF0000000) != 0x80000000)
     {
        *RealKey = Key;
        return STATUS_SUCCESS;
     }

   /* Handle special cases here */
   Index = (ULONG)Key & 0x0FFFFFFF;

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
              DPRINT("MapDefaultHandle() no handle creator\n");
              Status = STATUS_INVALID_PARAMETER;
          }
     }

   RtlLeaveCriticalSection(&HandleTableCS);

   if (NT_SUCCESS(Status))
     {
        *RealKey = (HKEY)*Handle;
     }

   return Status;
}


static VOID CloseDefaultKeys (VOID)
{
   ULONG i;

   RtlEnterCriticalSection(&HandleTableCS);

   for (i = 0; i < MAX_DEFAULT_HANDLES; i++)
     {
        if (DefaultHandleTable[i] != NULL)
          {
             NtClose (DefaultHandleTable[i]);
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

   DPRINT("OpenLocalMachineKey()\n");

   RtlInitUnicodeString(&KeyName,
                        L"\\Registry\\Machine");

   InitializeObjectAttributes(&Attributes,
                              &KeyName,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   return (NtOpenKey (KeyHandle,
                      KEY_ALL_ACCESS,
                      &Attributes));
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
	NTSTATUS Status;

	/* don't close null handle or a pseudo handle */
	if ((!hKey) || (((ULONG)hKey & 0xF0000000) == 0x80000000))
		return ERROR_INVALID_HANDLE;

	Status = NtClose (hKey);
	if (!NT_SUCCESS(Status))
	{
		LONG ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	return ERROR_SUCCESS;
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
 *	RegConnectRegistryW
 */
LONG
STDCALL
RegConnectRegistryW(
	LPWSTR	lpMachineName,
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

	DPRINT("RegCreateKeyExW() called\n");

	/* get the real parent key */
	Status = MapDefaultKey (&ParentKey, hKey);
	if (!NT_SUCCESS(Status))
	{
		LONG ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	DPRINT("ParentKey %x\n", (ULONG)ParentKey);

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
	DPRINT("Status %x\n", Status);
	if (!NT_SUCCESS(Status))
	{
		LONG ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

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
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING SubKeyStringW;
	ANSI_STRING SubKeyStringA;
	HANDLE ParentKey;
	HANDLE TargetKey;
	NTSTATUS Status;
	LONG ErrorCode;

	Status = MapDefaultKey(&ParentKey,
			       hKey);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	RtlInitAnsiString(&SubKeyStringA,
			  (LPSTR)lpSubKey);
	RtlAnsiStringToUnicodeString(&SubKeyStringW,
				     &SubKeyStringA,
				     TRUE);

	InitializeObjectAttributes (&ObjectAttributes,
				    &SubKeyStringW,
				    OBJ_CASE_INSENSITIVE,
				    (HANDLE)ParentKey,
				    NULL);

	Status = NtOpenKey (&TargetKey,
			    DELETE,
			    &ObjectAttributes);

	RtlFreeUnicodeString (&SubKeyStringW);

	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	Status = NtDeleteKey(TargetKey);

	NtClose(TargetKey);

	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}
	return ERROR_SUCCESS;
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
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING SubKeyString;
	HANDLE ParentKey;
	HANDLE TargetKey;
	NTSTATUS Status;
	LONG ErrorCode;

	Status = MapDefaultKey(&ParentKey,
			       hKey);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	RtlInitUnicodeString(&SubKeyString,
			     (LPWSTR)lpSubKey);

	InitializeObjectAttributes (&ObjectAttributes,
				    &SubKeyString,
				    OBJ_CASE_INSENSITIVE,
				    (HANDLE)ParentKey,
				    NULL);

	Status = NtOpenKey (&TargetKey,
			    DELETE,
			    &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	Status = NtDeleteKey(TargetKey);

	NtClose(TargetKey);

	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}
	return ERROR_SUCCESS;
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
	UNICODE_STRING ValueNameW;
	ANSI_STRING ValueNameA;
	NTSTATUS Status;
	LONG ErrorCode;
	HANDLE KeyHandle;

	Status = MapDefaultKey(&KeyHandle,
			       hKey);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	RtlInitAnsiString(&ValueNameA,
			  (LPSTR)lpValueName);
	RtlAnsiStringToUnicodeString(&ValueNameW,
				     &ValueNameA,
				     TRUE);

	Status = NtDeleteValueKey(KeyHandle,
				  &ValueNameW);

	RtlFreeUnicodeString (&ValueNameW);

	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	return ERROR_SUCCESS;
}


/************************************************************************
 *	RegDeleteValueW
 */
LONG
STDCALL
RegDeleteValueW(
	HKEY	hKey,
	LPCWSTR	lpValueName
	)
{
	UNICODE_STRING ValueName;
	NTSTATUS Status;
	LONG ErrorCode;
	HANDLE KeyHandle;

	Status = MapDefaultKey(&KeyHandle,
			       hKey);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	RtlInitUnicodeString(&ValueName,
			     (LPWSTR)lpValueName);

	Status = NtDeleteValueKey(KeyHandle,
				  &ValueName);
	if (!NT_SUCCESS(Status))
	{
		ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	return ERROR_SUCCESS;
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
	DWORD dwLength = cbName;

	return RegEnumKeyExA(hKey,
			     dwIndex,
			     lpName,
			     &dwLength,
			     NULL,
			     NULL,
			     NULL,
			     NULL);
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
	PKEY_NODE_INFORMATION KeyInfo;
	NTSTATUS Status;
	DWORD dwError = ERROR_SUCCESS;
	ULONG BufferSize;
	ULONG ResultSize;
	HANDLE KeyHandle;

	Status = MapDefaultKey(&KeyHandle,
			       hKey);
	if (!NT_SUCCESS(Status))
	{
		dwError = RtlNtStatusToDosError(Status);

		SetLastError (dwError);
		return dwError;
	}

	BufferSize = sizeof (KEY_NODE_INFORMATION) +
		*lpcbName * sizeof(WCHAR);
	if (lpClass)
		BufferSize += *lpcbClass;
	KeyInfo = RtlAllocateHeap (RtlGetProcessHeap(),
				   0,
				   BufferSize);
	if (KeyInfo == NULL)
		return ERROR_OUTOFMEMORY;

	Status = NtEnumerateKey (KeyHandle,
				 (ULONG)dwIndex,
				 KeyNodeInformation,
				 KeyInfo,
				 BufferSize,
				 &ResultSize);
	if (!NT_SUCCESS(Status))
	{
		dwError = RtlNtStatusToDosError(Status);
		
		SetLastError(dwError);
	}
	else
	{
		memcpy (lpName, KeyInfo->Name, KeyInfo->NameLength);
		*lpcbName = (DWORD)(KeyInfo->NameLength / sizeof(WCHAR)) - 1;

		if (lpClass)
		{
			memcpy (lpClass,
				KeyInfo->Name + KeyInfo->ClassOffset,
				KeyInfo->ClassLength);
			*lpcbClass = (DWORD)(KeyInfo->ClassLength / sizeof(WCHAR)) - 1;
		}

		if (lpftLastWriteTime)
		{

		}
	}

	RtlFreeHeap (RtlGetProcessHeap(), 0, KeyInfo);

	return dwError;
}


/************************************************************************
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
	DWORD dwLength = cbName;

	return RegEnumKeyExW(hKey,
			     dwIndex,
			     lpName,
			     &dwLength,
			     NULL,
			     NULL,
			     NULL,
			     NULL);
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
	PKEY_VALUE_FULL_INFORMATION ValueInfo;
	NTSTATUS Status;
	DWORD dwError = ERROR_SUCCESS;
	ULONG BufferSize;
	ULONG ResultSize;

	BufferSize = sizeof (KEY_VALUE_FULL_INFORMATION) +
		*lpcbValueName * sizeof(WCHAR);
	if (lpcbData)
		BufferSize += *lpcbData;
	ValueInfo = RtlAllocateHeap (RtlGetProcessHeap(),
				     0,
				     BufferSize);
	if (ValueInfo == NULL)
		return ERROR_OUTOFMEMORY;

	Status = NtEnumerateValueKey (hKey,
				      (ULONG)dwIndex,
				      KeyValueFullInformation,
				      ValueInfo,
				      BufferSize,
				      &ResultSize);
	if (!NT_SUCCESS(Status))
	{
		dwError = RtlNtStatusToDosError(Status);
		
		SetLastError(dwError);
	}
	else
	{
		memcpy (lpValueName, ValueInfo->Name, ValueInfo->NameLength);
		*lpcbValueName = (DWORD)(ValueInfo->NameLength / sizeof(WCHAR)) - 1;

		if (lpType)
			*lpType = ValueInfo->Type;

		if (lpData)
		{
			memcpy (lpData,
				ValueInfo->Name + ValueInfo->DataOffset,
				ValueInfo->DataLength);
			*lpcbValueName = (DWORD)ValueInfo->DataLength;
		}
	}

	RtlFreeHeap (RtlGetProcessHeap(), 0, ValueInfo);

	return dwError;
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
	HANDLE			KeyHandle;
	LONG			ErrorCode;

	errCode = MapDefaultKey(&KeyHandle,
				hKey);
	if (!NT_SUCCESS(errCode))
	{
		ErrorCode = RtlNtStatusToDosError(errCode);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	RtlInitUnicodeString(&SubKeyString,
			     (LPWSTR)lpSubKey);

	InitializeObjectAttributes(&ObjectAttributes,
				   &SubKeyString,
				   OBJ_CASE_INSENSITIVE,
				   KeyHandle,
				   NULL);

	errCode = NtOpenKey(
			phkResult,
			KEY_ALL_ACCESS,
			& ObjectAttributes
			);
	if ( !NT_SUCCESS(errCode) )
	{
		ErrorCode = RtlNtStatusToDosError(errCode);
		
		SetLastError(ErrorCode);
		return ErrorCode;
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
	NTSTATUS		errCode;
	UNICODE_STRING		SubKeyString;
	OBJECT_ATTRIBUTES	ObjectAttributes;

	RtlInitUnicodeString(&SubKeyString,
			     (LPWSTR)lpSubKey);

	InitializeObjectAttributes(&ObjectAttributes,
				   &SubKeyString,
				   OBJ_CASE_INSENSITIVE,
				   (HANDLE)hKey,
				   NULL);

	errCode = NtOpenKey(
			phkResult,
			samDesired,
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
	PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
	UNICODE_STRING ValueName;
	NTSTATUS Status;
	DWORD dwError = ERROR_SUCCESS;
	ULONG BufferSize;
	ULONG ResultSize;

	RtlInitUnicodeString (&ValueName,
			      lpValueName);

	BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + *lpcbData;
	ValueInfo = RtlAllocateHeap (RtlGetProcessHeap(),
				     0,
				     BufferSize);
	if (ValueInfo == NULL)
		return ERROR_OUTOFMEMORY;

	Status = NtQueryValueKey (hKey,
				  &ValueName,
				  KeyValuePartialInformation,
				  ValueInfo,
				  BufferSize,
				  &ResultSize);
	if (!NT_SUCCESS(Status))
	{
		dwError = RtlNtStatusToDosError(Status);
		
		SetLastError(dwError);
	}
	else
	{
		*lpType = ValueInfo->Type;
		memcpy (lpData, ValueInfo->Data, ValueInfo->DataLength);
		if (ValueInfo->Type == REG_SZ)
			((PWSTR)lpData)[ValueInfo->DataLength / sizeof(WCHAR)] = 0;
	}
	*lpcbData = (DWORD)ResultSize;

	RtlFreeHeap (RtlGetProcessHeap(), 0, ValueInfo);

	return dwError;
}


/************************************************************************
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
	UNICODE_STRING ValueName;
	NTSTATUS Status;

	RtlInitUnicodeString (&ValueName,
			      lpValueName);

	Status = NtSetValueKey (hKey,
				&ValueName,
				0,
				dwType,
				(PVOID)lpData,
				(ULONG)cbData);
	if (!NT_SUCCESS(Status))
	{
		LONG ErrorCode = RtlNtStatusToDosError(Status);

		SetLastError (ErrorCode);
		return ErrorCode;
	}

	return ERROR_SUCCESS;
}


/************************************************************************
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


/************************************************************************
 *	RegUnLoadKeyW
 */
LONG
STDCALL
RegUnLoadKeyW(
	HKEY	hKey,
	LPCWSTR	lpSubKey
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return ERROR_CALL_NOT_IMPLEMENTED;
}

/* EOF */
