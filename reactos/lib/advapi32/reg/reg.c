/* $Id: reg.c,v 1.26 2003/07/12 00:03:42 ekohl Exp $
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

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

#define MAX_DEFAULT_HANDLES   6

static CRITICAL_SECTION HandleTableCS;
static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];


/* PROTOTYPES ***************************************************************/

static NTSTATUS MapDefaultKey (PHKEY ParentKey, HKEY Key);
static VOID CloseDefaultKeys(VOID);

static NTSTATUS OpenClassesRootKey(PHANDLE KeyHandle);
static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);
static NTSTATUS OpenUsersKey (PHANDLE KeyHandle);
static NTSTATUS OpenCurrentConfigKey(PHANDLE KeyHandle);


/* FUNCTIONS ****************************************************************/

/************************************************************************
 *  RegInitDefaultHandles
 */
BOOL
RegInitialize (VOID)
{
  DPRINT("RegInitialize()\n");

  RtlZeroMemory (DefaultHandleTable,
		 MAX_DEFAULT_HANDLES * sizeof(HANDLE));
  RtlInitializeCriticalSection (&HandleTableCS);

  return TRUE;
}


/************************************************************************
 *  RegInit
 */
BOOL
RegCleanup (VOID)
{
  DPRINT("RegCleanup()\n");

  CloseDefaultKeys ();
  RtlDeleteCriticalSection (&HandleTableCS);

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
    {
      return STATUS_INVALID_PARAMETER;
    }

  RtlEnterCriticalSection (&HandleTableCS);
  Handle = &DefaultHandleTable[Index];
  if (*Handle == NULL)
    {
      /* create/open the default handle */
      switch (Index)
	{
	  case 0: /* HKEY_CLASSES_ROOT */
	    Status = OpenClassesRootKey (Handle);
	    break;

	  case 1: /* HKEY_CURRENT_USER */
	    Status = RtlOpenCurrentUser (KEY_ALL_ACCESS,
					 Handle);
	    break;

	  case 2: /* HKEY_LOCAL_MACHINE */
	    Status = OpenLocalMachineKey (Handle);
	    break;

	  case 3: /* HKEY_USERS */
	    Status = OpenUsersKey (Handle);
	    break;
#if 0
	  case 4: /* HKEY_PERFORMANCE_DATA */
	    Status = OpenPerformanceDataKey (Handle);
	    break;
#endif
	  case 5: /* HKEY_CURRENT_CONFIG */
	    Status = OpenCurrentConfigKey (Handle);
	    break;

	  case 6: /* HKEY_DYN_DATA */
	    Status = STATUS_NOT_IMPLEMENTED;
	    break;

	  default:
	    DPRINT("MapDefaultHandle() no handle creator\n");
	    Status = STATUS_INVALID_PARAMETER;
	}
    }
  RtlLeaveCriticalSection (&HandleTableCS);

  if (NT_SUCCESS(Status))
    {
      *RealKey = (HKEY)*Handle;
    }

   return Status;
}


static VOID
CloseDefaultKeys (VOID)
{
  ULONG i;

  RtlEnterCriticalSection (&HandleTableCS);
  for (i = 0; i < MAX_DEFAULT_HANDLES; i++)
    {
      if (DefaultHandleTable[i] != NULL)
	{
	  NtClose (DefaultHandleTable[i]);
	  DefaultHandleTable[i] = NULL;
	}
    }
  RtlLeaveCriticalSection (&HandleTableCS);
}


static NTSTATUS
OpenClassesRootKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName = UNICODE_STRING_INITIALIZER(L"\\Registry\\Machine\\Software\\CLASSES");

  DPRINT("OpenClassesRootKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    KEY_ALL_ACCESS,
		    &Attributes);
}


static NTSTATUS
OpenLocalMachineKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName = UNICODE_STRING_INITIALIZER(L"\\Registry\\Machine");

  DPRINT("OpenLocalMachineKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    KEY_ALL_ACCESS,
		    &Attributes);
}


static NTSTATUS
OpenUsersKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName = UNICODE_STRING_INITIALIZER(L"\\Registry\\User");

  DPRINT("OpenUsersKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    KEY_ALL_ACCESS,
		    &Attributes);
}


static NTSTATUS
OpenCurrentConfigKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName =
  UNICODE_STRING_INITIALIZER(L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");

  DPRINT("OpenCurrentConfigKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    KEY_ALL_ACCESS,
		    &Attributes);
}


/************************************************************************
 *  RegCloseKey
 *
 * @implemented
 */
LONG STDCALL
RegCloseKey (HKEY hKey)
{
  LONG ErrorCode;
  NTSTATUS Status;

  /* don't close null handle or a pseudo handle */
  if ((!hKey) || (((ULONG)hKey & 0xF0000000) == 0x80000000))
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = NtClose (hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegConnectRegistryA
 *
 * @unimplemented
 */
LONG STDCALL
RegConnectRegistryA (LPCSTR lpMachineName,
		     HKEY hKey,
		     PHKEY phkResult)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegConnectRegistryW
 *
 * @unimplemented
 */
LONG STDCALL
RegConnectRegistryW (LPCWSTR lpMachineName,
		     HKEY hKey,
		     PHKEY phkResult)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegCreateKeyExA
 *
 * @implemented
 */
LONG STDCALL
RegCreateKeyExA (HKEY hKey,
		 LPCSTR lpSubKey,
		 DWORD Reserved,
		 LPSTR lpClass,
		 DWORD dwOptions,
		 REGSAM samDesired,
		 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
		 PHKEY phkResult,
		 LPDWORD lpdwDisposition)
{
  UNICODE_STRING SubKeyString;
  UNICODE_STRING ClassString;
  OBJECT_ATTRIBUTES Attributes;
  HKEY ParentKey;
  LONG ErrorCode;
  NTSTATUS Status;

  DPRINT("RegCreateKeyExW() called\n");

  /* get the real parent key */
  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }
  DPRINT("ParentKey %x\n", (ULONG)ParentKey);

  if (lpClass != NULL)
    {
      RtlCreateUnicodeStringFromAsciiz (&ClassString,
					lpClass);
    }
  RtlCreateUnicodeStringFromAsciiz (&SubKeyString,
				    (LPSTR)lpSubKey);
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
  RtlFreeUnicodeString (&SubKeyString);
  if (lpClass != NULL)
    {
      RtlFreeUnicodeString (&ClassString);
    }
  DPRINT("Status %x\n", Status);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegCreateKeyExW
 *
 * @implemented
 */
LONG STDCALL
RegCreateKeyExW(HKEY hKey,
    LPCWSTR   lpSubKey,
    DWORD     Reserved,
    LPWSTR    lpClass,
    DWORD     dwOptions,
    REGSAM    samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY     phkResult,
    LPDWORD   lpdwDisposition)
{
  UNICODE_STRING SubKeyString;
  UNICODE_STRING ClassString;
  OBJECT_ATTRIBUTES Attributes;
  HKEY ParentKey;
  LONG ErrorCode;
  NTSTATUS Status;

  DPRINT("RegCreateKeyExW() called\n");

  /* get the real parent key */
  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }
  DPRINT("ParentKey %x\n", (ULONG)ParentKey);

  RtlInitUnicodeString (&ClassString,
			lpClass);
  RtlInitUnicodeString (&SubKeyString,
			lpSubKey);
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
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegCreateKeyA
 *
 * @implemented
 */
LONG STDCALL
RegCreateKeyA (HKEY hKey,
	       LPCSTR lpSubKey,
	       PHKEY phkResult)
{
  return RegCreateKeyExA (hKey,
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
 *  RegCreateKeyW
 *
 * @implemented
 */
LONG STDCALL
RegCreateKeyW (HKEY hKey,
	       LPCWSTR lpSubKey,
	       PHKEY phkResult)
{
  return RegCreateKeyExW (hKey,
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
 *  RegDeleteKeyA
 *
 * @implemented
 */
LONG STDCALL
RegDeleteKeyA (HKEY hKey,
	       LPCSTR lpSubKey)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyName;
  HKEY ParentKey;
  HANDLE TargetKey;
  NTSTATUS Status;
  LONG ErrorCode;

  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlCreateUnicodeStringFromAsciiz (&SubKeyName,
				    (LPSTR)lpSubKey);
  InitializeObjectAttributes(&ObjectAttributes,
			     &SubKeyName,
			     OBJ_CASE_INSENSITIVE,
			     (HANDLE)ParentKey,
			     NULL);

  Status = NtOpenKey (&TargetKey,
		      DELETE,
		      &ObjectAttributes);
  RtlFreeUnicodeString (&SubKeyName);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtDeleteKey (TargetKey);
  NtClose (TargetKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError(Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteKeyW
 *
 * @implemented
 */
LONG STDCALL
RegDeleteKeyW (HKEY hKey,
	       LPCWSTR lpSubKey)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyName;
  HKEY ParentKey;
  HANDLE TargetKey;
  NTSTATUS Status;
  LONG ErrorCode;

  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlInitUnicodeString (&SubKeyName,
			(LPWSTR)lpSubKey);
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyName,
			      OBJ_CASE_INSENSITIVE,
			      (HANDLE)ParentKey,
			      NULL);
  Status = NtOpenKey (&TargetKey,
		      DELETE,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtDeleteKey (TargetKey);
  NtClose (TargetKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteValueA
 *
 * @implemented
 */
LONG STDCALL
RegDeleteValueA (HKEY hKey,
		 LPCSTR lpValueName)
{
  UNICODE_STRING ValueName;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlCreateUnicodeStringFromAsciiz (&ValueName,
				    (LPSTR)lpValueName);
  Status = NtDeleteValueKey (KeyHandle,
			     &ValueName);
  RtlFreeUnicodeString (&ValueName);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteValueW
 *
 * @implemented
 */
LONG STDCALL
RegDeleteValueW (HKEY hKey,
		 LPCWSTR lpValueName)
{
  UNICODE_STRING ValueName;
  NTSTATUS Status;
  LONG ErrorCode;
  HKEY KeyHandle;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlInitUnicodeString (&ValueName,
			(LPWSTR)lpValueName);

  Status = NtDeleteValueKey (KeyHandle,
			     &ValueName);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegEnumKeyA
 *
 * @implemented
 */
LONG STDCALL
RegEnumKeyA (HKEY hKey,
	     DWORD dwIndex,
	     LPSTR lpName,
	     DWORD cbName)
{
  DWORD dwLength;

  dwLength = cbName;
  return RegEnumKeyExA (hKey,
			dwIndex,
			lpName,
			&dwLength,
			NULL,
			NULL,
			NULL,
			NULL);
}


/************************************************************************
 *  RegEnumKeyW
 *
 * @implemented
 */
LONG STDCALL
RegEnumKeyW (HKEY hKey,
	     DWORD dwIndex,
	     LPWSTR lpName,
	     DWORD cbName)
{
  DWORD dwLength;

  dwLength = cbName;
  return RegEnumKeyExW (hKey,
			dwIndex,
			lpName,
			&dwLength,
			NULL,
			NULL,
			NULL,
			NULL);
}


/************************************************************************
 *  RegEnumKeyExA
 *
 * @implemented
 */
LONG STDCALL
RegEnumKeyExA (HKEY hKey,
	       DWORD dwIndex,
	       LPSTR lpName,
	       LPDWORD lpcbName,
	       LPDWORD lpReserved,
	       LPSTR lpClass,
	       LPDWORD lpcbClass,
	       PFILETIME lpftLastWriteTime)
{
  WCHAR Name[MAX_PATH+1];
  UNICODE_STRING UnicodeStringName;
  WCHAR Class[MAX_PATH+1];
  UNICODE_STRING UnicodeStringClass;
  ANSI_STRING AnsiString;
  LONG ErrorCode;
  DWORD NameLength;
  DWORD ClassLength;

  DPRINT("hKey 0x%x  dwIndex %d  lpName 0x%x  *lpcbName %d  lpClass 0x%x  lpcbClass %d\n",
	 hKey, dwIndex, lpName, *lpcbName, lpClass, lpcbClass);

  if ((lpClass) && (!lpcbClass))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  RtlInitUnicodeString (&UnicodeStringName,
			NULL);
  UnicodeStringName.Buffer = &Name[0];
  UnicodeStringName.MaximumLength = sizeof(Name);
  RtlInitUnicodeString (&UnicodeStringClass,
			NULL);
  if (lpClass != NULL)
    {
      UnicodeStringClass.Buffer = &Class[0];
      UnicodeStringClass.MaximumLength = sizeof(Class);
      ClassLength = *lpcbClass;
    }
  else
    {
      ClassLength = 0;
    }
  NameLength = *lpcbName;

  ErrorCode = RegEnumKeyExW (hKey,
			     dwIndex,
			     UnicodeStringName.Buffer,
			     &NameLength,
			     lpReserved,
			     UnicodeStringClass.Buffer,
			     &ClassLength,
			     lpftLastWriteTime);
  if (ErrorCode != ERROR_SUCCESS)
    {
      return ErrorCode;
    }

  UnicodeStringName.Length = NameLength * sizeof(WCHAR);
  UnicodeStringClass.Length = ClassLength * sizeof(WCHAR);
  RtlInitAnsiString (&AnsiString,
		     NULL);
  AnsiString.Buffer = lpName;
  AnsiString.MaximumLength = *lpcbName;
  RtlUnicodeStringToAnsiString (&AnsiString,
				&UnicodeStringName,
				FALSE);
  *lpcbName = AnsiString.Length;

  DPRINT("Key Namea0 Length %d\n", UnicodeStringName.Length);
  DPRINT("Key Namea1 Length %d\n", NameLength);
  DPRINT("Key Namea Length %d\n", *lpcbName);
  DPRINT("Key Namea %s\n", lpName);

  if (lpClass != NULL)
    {
      RtlInitAnsiString (&AnsiString,
			 NULL);
      AnsiString.Buffer = lpClass;
      AnsiString.MaximumLength = *lpcbClass;
      RtlUnicodeStringToAnsiString (&AnsiString,
				    &UnicodeStringClass,
				    FALSE);
      *lpcbClass = AnsiString.Length;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegEnumKeyExW
 *
 * @implemented
 */
LONG STDCALL
RegEnumKeyExW (HKEY hKey,
	       DWORD dwIndex,
	       LPWSTR lpName,
	       LPDWORD lpcbName,
	       LPDWORD lpReserved,
	       LPWSTR lpClass,
	       LPDWORD lpcbClass,
	       PFILETIME lpftLastWriteTime)
{
  PKEY_NODE_INFORMATION KeyInfo;
  ULONG BufferSize;
  ULONG ResultSize;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey(&KeyHandle,
			 hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  BufferSize = sizeof (KEY_NODE_INFORMATION) + *lpcbName * sizeof(WCHAR);
  if (lpClass != NULL)
    {
      BufferSize += *lpcbClass;
    }

  KeyInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			     0,
			     BufferSize);
  if (KeyInfo == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  /* We don't know the exact size of the data returned, so call
     NtEnumerateKey() with a buffer size determined from parameters
     to this function. If that call fails with a status code of
     STATUS_BUFFER_OVERFLOW, allocate a new buffer and try again */
  while (TRUE)
    {
      Status = NtEnumerateKey (KeyHandle,
			       (ULONG)dwIndex,
			       KeyNodeInformation,
			       KeyInfo,
			       BufferSize,
			       &ResultSize);
      DPRINT("NtEnumerateKey() returned status 0x%X\n", Status);
      if (Status == STATUS_BUFFER_OVERFLOW)
	{
	  BufferSize = ResultSize;
	  continue;
	}
      if (!NT_SUCCESS(Status))
	{
	  ErrorCode = RtlNtStatusToDosError (Status);
	  SetLastError (ErrorCode);
	  break;
	}
      else
	{
	  if ((lpClass != NULL) &&
	      (*lpcbClass != 0) &&
	      (KeyInfo->ClassLength > *lpcbClass))
	    {
	      ErrorCode = ERROR_MORE_DATA;
	      SetLastError (ErrorCode);
	      break;
	    }
	  RtlMoveMemory (lpName,
			 KeyInfo->Name,
			 KeyInfo->NameLength);
	  *lpcbName = (DWORD)(KeyInfo->NameLength / sizeof(WCHAR));
	  lpName[KeyInfo->NameLength / sizeof(WCHAR)] = 0;
	  if (lpClass)
	    {
	      RtlMoveMemory (lpClass,
			     (PVOID)((ULONG_PTR)KeyInfo->Name + KeyInfo->ClassOffset),
			     KeyInfo->ClassLength);
	      *lpcbClass = (DWORD)(KeyInfo->ClassLength / sizeof(WCHAR));
	    }
	  if (lpftLastWriteTime != NULL)
	    {
	      /* FIXME: Fill lpftLastWriteTime */
	    }
	  break;
	}
    }

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       KeyInfo);

  return ErrorCode;
}


/************************************************************************
 *  RegEnumValueA
 *
 * @implemented
 */
LONG STDCALL
RegEnumValueA (HKEY hKey,
	       DWORD dwIndex,
	       LPSTR lpValueName,
	       LPDWORD lpcbValueName,
	       LPDWORD lpReserved,
	       LPDWORD lpType,
	       LPBYTE lpData,
	       LPDWORD lpcbData)
{
  WCHAR ValueName[MAX_PATH+1];
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;
  LONG ErrorCode;
  DWORD ValueNameLength;
  BYTE* lpDataBuffer = NULL;
  DWORD cbData = 0;
  DWORD Type;
  ANSI_STRING AnsiDataString;
  UNICODE_STRING UnicodeDataString;

  ErrorCode = ERROR_SUCCESS;
  if (lpData != NULL /*&& lpcbData != NULL*/)
    {
      cbData = *lpcbData; // this should always be valid if lpData is valid
      lpDataBuffer = RtlAllocateHeap (RtlGetProcessHeap (),
				      0,
				      (*lpcbData) * sizeof(WCHAR));
      if (lpDataBuffer == NULL)
	{
	  SetLastError (ERROR_OUTOFMEMORY);
	  return ERROR_OUTOFMEMORY;
	}
    }
  RtlInitUnicodeString (&UnicodeString,
			NULL);
  UnicodeString.Buffer = &ValueName[0];
  UnicodeString.MaximumLength = sizeof(ValueName);
  ValueNameLength = *lpcbValueName;
  ErrorCode = RegEnumValueW (hKey,
			     dwIndex,
			     UnicodeString.Buffer,
			     &ValueNameLength,
			     lpReserved,
			     &Type,
			     lpDataBuffer,
			     &cbData);
  if (ErrorCode != ERROR_SUCCESS)
    {
      if (lpDataBuffer != NULL)
	{
	  RtlFreeHeap (RtlGetProcessHeap (),
		       0,
		       lpDataBuffer);
	}

      return ErrorCode;
    }

  UnicodeString.Length = ValueNameLength * sizeof(WCHAR);
  RtlInitAnsiString (&AnsiString,
		     NULL);
  AnsiString.Buffer = lpValueName;
  AnsiString.MaximumLength = *lpcbValueName;
  RtlUnicodeStringToAnsiString (&AnsiString,
				&UnicodeString,
				FALSE);
  *lpcbValueName = AnsiString.Length;
  if (lpDataBuffer != NULL)
    {
      /* Did we use a temp buffer */
      if ((Type == REG_SZ) || (Type == REG_MULTI_SZ) || (Type == REG_EXPAND_SZ))
	{
	  RtlInitUnicodeString (&UnicodeDataString,
				NULL);
	  UnicodeDataString.Buffer = (WCHAR*)lpDataBuffer;
	  UnicodeDataString.MaximumLength = (*lpcbData) * sizeof(WCHAR);
	  UnicodeDataString.Length = cbData /* * sizeof(WCHAR)*/;
	  RtlInitAnsiString (&AnsiDataString,
			     NULL);
	  AnsiDataString.Buffer = lpData;
	  AnsiDataString.MaximumLength = *lpcbData;
	  RtlUnicodeStringToAnsiString (&AnsiDataString,
					&UnicodeDataString,
					FALSE);
	  *lpcbData = AnsiDataString.Length;
	}
      else
	{
	  RtlCopyMemory (lpData,
			 lpDataBuffer,
			 min(*lpcbData, cbData));
	  *lpcbData = cbData;
	}
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   lpDataBuffer);
    }

  if (lpType != NULL)
    {
      *lpType = Type;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegEnumValueW
 *
 * @implemented
 */
LONG STDCALL
RegEnumValueW (HKEY hKey,
	       DWORD dwIndex,
	       LPWSTR lpValueName,
	       LPDWORD lpcbValueName,
	       LPDWORD lpReserved,
	       LPDWORD lpType,
	       LPBYTE lpData,
	       LPDWORD lpcbData)
{
  PKEY_VALUE_FULL_INFORMATION ValueInfo;
  ULONG BufferSize;
  ULONG ResultSize;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  ErrorCode = ERROR_SUCCESS;
  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  BufferSize = sizeof (KEY_VALUE_FULL_INFORMATION) + *lpcbValueName * sizeof(WCHAR);
  if (lpcbData)
    {
      BufferSize += *lpcbData;
    }

  /* We don't know the exact size of the data returned, so call
     NtEnumerateValueKey() with a buffer size determined from parameters
     to this function. If that call fails with a status code of
     STATUS_BUFFER_OVERFLOW, allocate a new buffer and try again */
  ValueInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			       0,
			       BufferSize);
  if (ValueInfo == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  while (TRUE)
    {
      Status = NtEnumerateValueKey (KeyHandle,
				    (ULONG)dwIndex,
				    KeyValueFullInformation,
				    ValueInfo,
				    BufferSize,
				    &ResultSize);
      DPRINT("NtEnumerateValueKey() returned status 0x%X\n", Status);
      if (Status == STATUS_BUFFER_OVERFLOW)
	{
	  BufferSize = ResultSize;
	  continue;
	}
      if (!NT_SUCCESS(Status))
	{
	  ErrorCode = RtlNtStatusToDosError (Status);
	  SetLastError (ErrorCode);
	  break;
	}
      else
	{
	  if ((lpData != NULL) &&
	      (*lpcbData != 0) &&
	      (ValueInfo->DataLength > *lpcbData))
	    {
	      ErrorCode = ERROR_MORE_DATA;
	      SetLastError (ErrorCode);
	      break;
	    }
	  RtlCopyMemory (lpValueName,
			 ValueInfo->Name,
			 ValueInfo->NameLength);
	  *lpcbValueName = (DWORD)(ValueInfo->NameLength / sizeof(WCHAR));
	  lpValueName[ValueInfo->NameLength / sizeof(WCHAR)] = 0;
	  if (lpType != NULL)
	    {
	      *lpType = ValueInfo->Type;
	    }
	  if (lpData != NULL)
	    {
	      RtlCopyMemory (lpData,
			     (PVOID)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset),
			     ValueInfo->DataLength);
			    *lpcbData = (DWORD)ValueInfo->DataLength;
	    }
	  break;
	}
    }

  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       ValueInfo);

  return ErrorCode;
}


/************************************************************************
 *  RegFlushKey
 *
 * @implemented
 */
LONG STDCALL
RegFlushKey(HKEY hKey)
{
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_SUCCESS;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtFlushKey (KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegGetKeySecurity
 *
 * @unimplemented
 */
LONG STDCALL
RegGetKeySecurity (HKEY hKey,
		   SECURITY_INFORMATION SecurityInformation,
		   PSECURITY_DESCRIPTOR pSecurityDescriptor,
		   LPDWORD lpcbSecurityDescriptor)
{
#if 0
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  if (hKey = HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtQuerySecurityObject ()
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
#endif

  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegLoadKeyA
 *
 * @implemented
 */
LONG STDCALL
RegLoadKeyA (HKEY hKey,
	     LPCSTR lpSubKey,
	     LPCSTR lpFile)
{
  UNICODE_STRING FileName;
  UNICODE_STRING KeyName;
  DWORD ErrorCode;

  RtlCreateUnicodeStringFromAsciiz (&KeyName,
				    (LPSTR)lpSubKey);
  RtlCreateUnicodeStringFromAsciiz (&FileName,
				    (LPSTR)lpFile);

  ErrorCode = RegLoadKeyW (hKey,
			   KeyName.Buffer,
			   FileName.Buffer);

  RtlFreeUnicodeString (&FileName);
  RtlFreeUnicodeString (&KeyName);

  return ErrorCode;
}


/************************************************************************
 *  RegLoadKeyW
 *
 * @implemented
 */
LONG STDCALL
RegLoadKeyW (HKEY hKey,
	     LPCWSTR lpSubKey,
	     LPCWSTR lpFile)
{
  OBJECT_ATTRIBUTES FileObjectAttributes;
  OBJECT_ATTRIBUTES KeyObjectAttributes;
  UNICODE_STRING FileName;
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
  DWORD ErrorCode;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFile,
				     &FileName,
				     NULL,
				     NULL))
    {
      SetLastError (ERROR_BAD_PATHNAME);
      return ERROR_BAD_PATHNAME;
    }

  InitializeObjectAttributes (&FileObjectAttributes,
			      &FileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  RtlInitUnicodeString (&KeyName,
			(LPWSTR)lpSubKey);

  InitializeObjectAttributes (&KeyObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);

  Status = NtLoadKey (&KeyObjectAttributes,
		      &FileObjectAttributes);

  RtlFreeUnicodeString (&FileName);

  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegNotifyChangeKeyValue
 *
 * @unimplemented
 */
LONG STDCALL
RegNotifyChangeKeyValue (HKEY hKey,
			 BOOL bWatchSubtree,
			 DWORD dwNotifyFilter,
			 HANDLE hEvent,
			 BOOL fAsynchronous)
{
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE KeyHandle;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  if (fAsynchronous == TRUE && hEvent == NULL)
    {
      return ERROR_INVALID_PARAMETER;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  /* FIXME: Remote key handles must fail */

  Status = NtNotifyChangeKey (KeyHandle,
			      hEvent,
			      0,
			      0,
			      &IoStatusBlock,
			      dwNotifyFilter,
			      bWatchSubtree,
			      0,
			      0,
			      fAsynchronous);
  if (!NT_SUCCESS(Status) && Status != STATUS_TIMEOUT)
    {
      return RtlNtStatusToDosError (Status);
    }

  return ERROR_SUCCESS;
}



/************************************************************************
 *  RegOpenKeyA
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyA (HKEY hKey,
	     LPCSTR lpSubKey,
	     PHKEY phkResult)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlCreateUnicodeStringFromAsciiz (&SubKeyString,
				    (LPSTR)lpSubKey);
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyString,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);
  Status = NtOpenKey (phkResult,
		      KEY_ALL_ACCESS,
		      &ObjectAttributes);
  RtlFreeUnicodeString (&SubKeyString);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOpenKeyW
 *
 *  19981101 Ariadne
 *  19990525 EA
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyW (HKEY hKey,
	     LPCWSTR lpSubKey,
	     PHKEY phkResult)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlInitUnicodeString (&SubKeyString,
			(LPWSTR)lpSubKey);
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyString,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);
  Status = NtOpenKey (phkResult,
		      KEY_ALL_ACCESS,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError(ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOpenKeyExA
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyExA (HKEY hKey,
	       LPCSTR lpSubKey,
	       DWORD ulOptions,
	       REGSAM samDesired,
	       PHKEY phkResult)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlCreateUnicodeStringFromAsciiz (&SubKeyString,
				    (LPSTR)lpSubKey);
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyString,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);
  Status = NtOpenKey (phkResult,
		      samDesired,
		      &ObjectAttributes);
  RtlFreeUnicodeString (&SubKeyString);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
   }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOpenKeyExW
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyExW (HKEY hKey,
	       LPCWSTR lpSubKey,
	       DWORD ulOptions,
	       REGSAM samDesired,
	       PHKEY phkResult)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpSubKey != NULL)
    {
      RtlInitUnicodeString (&SubKeyString,
			    (LPWSTR)lpSubKey);
    }
  else
    {
      RtlInitUnicodeString (&SubKeyString,
			    (LPWSTR)L"");
    }
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyString,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);
  Status = NtOpenKey (phkResult,
		      samDesired,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegQueryInfoKeyA
 *
 * @implemented
 */
LONG STDCALL
RegQueryInfoKeyA (HKEY hKey,
		  LPSTR lpClass,
		  LPDWORD lpcbClass,
		  LPDWORD lpReserved,
		  LPDWORD lpcSubKeys,
		  LPDWORD lpcbMaxSubKeyLen,
		  LPDWORD lpcbMaxClassLen,
		  LPDWORD lpcValues,
		  LPDWORD lpcbMaxValueNameLen,
		  LPDWORD lpcbMaxValueLen,
		  LPDWORD lpcbSecurityDescriptor,
		  PFILETIME lpftLastWriteTime)
{
  WCHAR ClassName[MAX_PATH];
  UNICODE_STRING UnicodeString;
  ANSI_STRING AnsiString;
  LONG ErrorCode;

  RtlInitUnicodeString (&UnicodeString,
			NULL);
  if (lpClass != NULL)
    {
      UnicodeString.Buffer = &ClassName[0];
      UnicodeString.MaximumLength = sizeof(ClassName);
    }

  ErrorCode = RegQueryInfoKeyW (hKey,
				UnicodeString.Buffer,
				lpcbClass,
				lpReserved,
				lpcSubKeys,
				lpcbMaxSubKeyLen,
				lpcbMaxClassLen,
				lpcValues,
				lpcbMaxValueNameLen,
				lpcbMaxValueLen,
				lpcbSecurityDescriptor,
				lpftLastWriteTime);
  if ((ErrorCode == ERROR_SUCCESS) && (lpClass != NULL))
    {
      RtlInitAnsiString (&AnsiString,
			 NULL);
      AnsiString.Buffer = lpClass;
      AnsiString.MaximumLength = *lpcbClass;
      RtlUnicodeStringToAnsiString (&AnsiString,
				    &UnicodeString,
				    FALSE);
      *lpcbClass = AnsiString.Length;
    }

  return ErrorCode;
}


/************************************************************************
 *  RegQueryInfoKeyW
 *
 * @implemented
 */
LONG STDCALL
RegQueryInfoKeyW (HKEY hKey,
		  LPWSTR lpClass,
		  LPDWORD lpcbClass,
		  LPDWORD lpReserved,
		  LPDWORD lpcSubKeys,
		  LPDWORD lpcbMaxSubKeyLen,
		  LPDWORD lpcbMaxClassLen,
		  LPDWORD lpcValues,
		  LPDWORD lpcbMaxValueNameLen,
		  LPDWORD lpcbMaxValueLen,
		  LPDWORD lpcbSecurityDescriptor,
		  PFILETIME lpftLastWriteTime)
{
  KEY_FULL_INFORMATION FullInfoBuffer;
  PKEY_FULL_INFORMATION FullInfo;
  ULONG FullInfoSize;
  HKEY KeyHandle;
  NTSTATUS Status;
  LONG ErrorCode;
  ULONG Length;

  if ((lpClass) && (!lpcbClass))
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpClass != NULL)
    {
      FullInfoSize = sizeof(KEY_FULL_INFORMATION) + *lpcbClass;
      FullInfo = RtlAllocateHeap (RtlGetProcessHeap (),
				  0,
				  FullInfoSize);
      if (FullInfo == NULL)
	{
	  SetLastError (ERROR_OUTOFMEMORY);
	  return ERROR_OUTOFMEMORY;
	}
      FullInfo->ClassLength = *lpcbClass;
    }
  else
    {
      FullInfoSize = sizeof(KEY_FULL_INFORMATION);
      FullInfo = &FullInfoBuffer;
      FullInfo->ClassLength = 1;
    }
  FullInfo->ClassOffset = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

  Status = NtQueryKey (KeyHandle,
		       KeyFullInformation,
		       FullInfo,
		       FullInfoSize,
		       &Length);
  if (!NT_SUCCESS(Status))
    {
      if (lpClass != NULL)
	{
	  RtlFreeHeap (RtlGetProcessHeap (),
		       0,
		       FullInfo);
	}
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpcSubKeys != NULL)
    {
      *lpcSubKeys = FullInfo->SubKeys;
    }

  if (lpcbMaxSubKeyLen != NULL)
    {
      *lpcbMaxSubKeyLen = FullInfo->MaxNameLen;
    }

  if (lpcbMaxClassLen != NULL)
    {
      *lpcbMaxClassLen = FullInfo->MaxClassLen;
    }

  if (lpcValues)
    {
      *lpcValues = FullInfo->Values;
    }

  if (lpcbMaxValueNameLen)
    {
      *lpcbMaxValueNameLen = FullInfo->MaxValueNameLen;
    }

  if (lpcbMaxValueLen)
    {
      *lpcbMaxValueLen = FullInfo->MaxValueDataLen;
    }

  if (lpcbSecurityDescriptor)
    {
      *lpcbSecurityDescriptor = 0;
      /* FIXME */
    }

  if (lpftLastWriteTime != NULL)
    {
      lpftLastWriteTime->dwLowDateTime = FullInfo->LastWriteTime.u.LowPart;
      lpftLastWriteTime->dwHighDateTime = FullInfo->LastWriteTime.u.HighPart;
    }

  if (lpClass != NULL)
    {
      wcsncpy (lpClass,
	       FullInfo->Class,
	       *lpcbClass);
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   FullInfo);
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegQueryMultipleValuesA
 *
 * @unimplemented
 */
LONG STDCALL
RegQueryMultipleValuesA (HKEY hKey,
			 PVALENTA val_list,
			 DWORD num_vals,
			 LPSTR lpValueBuf,
			 LPDWORD ldwTotsize)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegQueryMultipleValuesW
 *
 * @unimplemented
 */
LONG STDCALL
RegQueryMultipleValuesW (HKEY hKey,
			 PVALENTW val_list,
			 DWORD num_vals,
			 LPWSTR lpValueBuf,
			 LPDWORD ldwTotsize)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegQueryValueExW
 *
 * @implemented
 */
LONG STDCALL
RegQueryValueExW (HKEY hKey,
		  LPCWSTR lpValueName,
		  LPDWORD lpReserved,
		  LPDWORD lpType,
		  LPBYTE lpData,
		  LPDWORD lpcbData)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  UNICODE_STRING ValueName;
  NTSTATUS Status;
  LONG ErrorCode = ERROR_SUCCESS;
  ULONG BufferSize;
  ULONG ResultSize;
  HKEY KeyHandle;

  DPRINT("hKey 0x%X  lpValueName %S  lpData 0x%X  lpcbData %d\n",
	 hKey, lpValueName, lpData, lpcbData ? *lpcbData : 0);

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpData != NULL && lpcbData == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  RtlInitUnicodeString (&ValueName,
			lpValueName);
  BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + *lpcbData;
  ValueInfo = RtlAllocateHeap (RtlGetProcessHeap (),
			       0,
			       BufferSize);
  if (ValueInfo == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  Status = NtQueryValueKey (hKey,
			    &ValueName,
			    KeyValuePartialInformation,
			    ValueInfo,
			    BufferSize,
			    &ResultSize);
  DPRINT("Status 0x%X\n", Status);
  if (Status == STATUS_BUFFER_TOO_SMALL)
    {
      /* Return ERROR_SUCCESS and the buffer space needed for a successful call */
      ErrorCode = ERROR_SUCCESS;
    }
  else if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
    }
  else
    {
      if (lpType != NULL)
	{
	  *lpType = ValueInfo->Type;
	}
      RtlMoveMemory (lpData,
		     ValueInfo->Data,
		     ValueInfo->DataLength);
      if ((ValueInfo->Type == REG_SZ) ||
	  (ValueInfo->Type == REG_MULTI_SZ) ||
	  (ValueInfo->Type == REG_EXPAND_SZ))
	{
	  ((PWSTR)lpData)[ValueInfo->DataLength / sizeof(WCHAR)] = 0;
	}
    }
  DPRINT("Type %d  Size %d\n", ValueInfo->Type, ValueInfo->DataLength);

  if (NULL != lpcbData)
    {
      *lpcbData = (DWORD)ValueInfo->DataLength;
    }
  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       ValueInfo);

  return ErrorCode;
}


/************************************************************************
 *  RegQueryValueExA
 *
 * @unimplemented
 */
LONG
STDCALL
RegQueryValueExA(
  HKEY  hKey,
  LPCSTR  lpValueName,
  LPDWORD lpReserved,
  LPDWORD lpType,
  LPBYTE  lpData,
  LPDWORD lpcbData)
{
  WCHAR ValueNameBuffer[MAX_PATH+1];
  UNICODE_STRING ValueName;
  UNICODE_STRING ValueData;
  ANSI_STRING AnsiString;
  LONG ErrorCode;
  DWORD ResultSize;
  DWORD Type;

  /* FIXME: HKEY_PERFORMANCE_DATA is special, see MS SDK */

  if ((lpData) && (!lpcbData)) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return ERROR_INVALID_PARAMETER;
  }
  RtlInitUnicodeString(&ValueData, NULL);
  if (lpData) {
    ValueData.MaximumLength = *lpcbData * sizeof(WCHAR);
    ValueData.Buffer = RtlAllocateHeap(
      RtlGetProcessHeap(),
      0,
      ValueData.MaximumLength);
    if (!ValueData.Buffer) {
      SetLastError(ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }
  }
  RtlInitAnsiString(&AnsiString, (LPSTR)lpValueName);
  RtlInitUnicodeString(&ValueName, NULL);
  ValueName.Buffer = &ValueNameBuffer[0];
  ValueName.MaximumLength = sizeof(ValueNameBuffer);
  RtlAnsiStringToUnicodeString(&ValueName, &AnsiString, FALSE);
  if (lpcbData) {
    ResultSize = *lpcbData;
  } else {
    ResultSize = 0;
  }
  ErrorCode = RegQueryValueExW(
    hKey,
    ValueName.Buffer,
    lpReserved,
    &Type,
    (LPBYTE)ValueData.Buffer,
    &ResultSize);
  if ((ErrorCode == ERROR_SUCCESS) && (ValueData.Buffer != NULL)) {
    if (lpType) {
      *lpType = Type;
    }
    if ((Type == REG_SZ) || (Type == REG_MULTI_SZ) || (Type == REG_EXPAND_SZ)) {
      ValueData.Length = ResultSize;
      RtlInitAnsiString(&AnsiString, NULL);
      AnsiString.Buffer = lpData;
      AnsiString.MaximumLength = *lpcbData;
      RtlUnicodeStringToAnsiString(&AnsiString, &ValueData, FALSE);
    } else {
      RtlMoveMemory(lpData, ValueData.Buffer, ResultSize);
    }
  }
  if (lpcbData) {
    *lpcbData = ResultSize;
  }
  if (ValueData.Buffer) {
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueData.Buffer);
  }
  return ErrorCode;
}


/************************************************************************
 *  RegQueryValueA
 *
 * @implemented
 */
LONG STDCALL
RegQueryValueA (HKEY hKey,
		LPCSTR lpSubKey,
		LPSTR lpValue,
		PLONG lpcbValue)
{
  WCHAR SubKeyNameBuffer[MAX_PATH+1];
  UNICODE_STRING SubKeyName;
  UNICODE_STRING Value;
  ANSI_STRING AnsiString;
  LONG ValueSize;
  LONG ErrorCode;

  if (lpValue != NULL &&
      lpcbValue == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  RtlInitUnicodeString (&SubKeyName,
			NULL);
  RtlInitUnicodeString (&Value,
			NULL);
  if (lpSubKey != NULL &&
      strlen(lpSubKey) != 0)
    {
      RtlInitAnsiString (&AnsiString,
			 (LPSTR)lpSubKey);
      SubKeyName.Buffer = &SubKeyNameBuffer[0];
      SubKeyName.MaximumLength = sizeof(SubKeyNameBuffer);
      RtlAnsiStringToUnicodeString (&SubKeyName,
				    &AnsiString,
				    FALSE);
    }

  if (lpValue != NULL)
    {
      ValueSize = *lpcbValue * sizeof(WCHAR);
      Value.MaximumLength = ValueSize;
      Value.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
				      0,
				      ValueSize);
      if (Value.Buffer == NULL)
	{
	  SetLastError(ERROR_OUTOFMEMORY);
	  return ERROR_OUTOFMEMORY;
	}
    }
  else
    {
      ValueSize = 0;
    }

  ErrorCode = RegQueryValueW (hKey,
			      (LPCWSTR)SubKeyName.Buffer,
			      Value.Buffer,
			      &ValueSize);
  if (ErrorCode == ERROR_SUCCESS)
    {
      Value.Length = ValueSize;
      RtlInitAnsiString (&AnsiString,
			 NULL);
      AnsiString.Buffer = lpValue;
      AnsiString.MaximumLength = *lpcbValue;
      RtlUnicodeStringToAnsiString (&AnsiString,
				    &Value,
				    FALSE);
    }

  *lpcbValue = ValueSize;
  if (Value.Buffer != NULL)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   Value.Buffer);
    }

  return ErrorCode;
}


/************************************************************************
 *  RegQueryValueW
 *
 * @implemented
 */
LONG STDCALL
RegQueryValueW (HKEY hKey,
		LPCWSTR lpSubKey,
		LPWSTR lpValue,
		PLONG lpcbValue)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  HANDLE RealKey;
  LONG ErrorCode;
  BOOL CloseRealKey;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpSubKey != NULL &&
      wcslen(lpSubKey) != 0)
    {
      RtlInitUnicodeString (&SubKeyString,
			    (LPWSTR)lpSubKey);
      InitializeObjectAttributes (&ObjectAttributes,
				  &SubKeyString,
				  OBJ_CASE_INSENSITIVE,
				  KeyHandle,
				  NULL);
      Status = NtOpenKey (&RealKey,
			  KEY_ALL_ACCESS,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  ErrorCode = RtlNtStatusToDosError (Status);
	  SetLastError (ErrorCode);
	  return ErrorCode;
	}
      CloseRealKey = TRUE;
    }
  else
    {
      RealKey = hKey;
      CloseRealKey = FALSE;
    }

  ErrorCode = RegQueryValueExW (RealKey,
				NULL,
				NULL,
				NULL,
				(LPBYTE)lpValue,
				(LPDWORD)lpcbValue);
  if (CloseRealKey)
    {
      NtClose (RealKey);
    }

  return ErrorCode;
}


/************************************************************************
 *  RegReplaceKeyA
 *
 * @unimplemented
 */
LONG STDCALL
RegReplaceKeyA (HKEY hKey,
		LPCSTR lpSubKey,
		LPCSTR lpNewFile,
		LPCSTR lpOldFile)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegReplaceKeyW
 *
 * @unimplemented
 */
LONG STDCALL
RegReplaceKeyW (HKEY hKey,
		LPCWSTR lpSubKey,
		LPCWSTR lpNewFile,
		LPCWSTR lpOldFile)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegRestoreKeyA
 *
 * @unimplemented
 */
LONG STDCALL
RegRestoreKeyA (HKEY hKey,
		LPCSTR lpFile,
		DWORD dwFlags)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegRestoreKeyW
 *
 * @unimplemented
 */
LONG STDCALL
RegRestoreKeyW (HKEY hKey,
		LPCWSTR lpFile,
		DWORD dwFlags)
{
  UNIMPLEMENTED;
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegSaveKeyA
 *
 * @unimplemented
 */
LONG STDCALL
RegSaveKeyA(HKEY hKey,
	    LPCSTR lpFile,
	    LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  UNICODE_STRING FileName;
  LONG ErrorCode;

  RtlCreateUnicodeStringFromAsciiz (&FileName,
				    (LPSTR)lpFile);
  ErrorCode = RegSaveKeyW (hKey,
			   FileName.Buffer,
			   lpSecurityAttributes);
  RtlFreeUnicodeString (&FileName);

  return ErrorCode;
}


/************************************************************************
 *  RegSaveKeyW
 *
 * @implemented
 */
LONG STDCALL
RegSaveKeyW (HKEY hKey,
	     LPCWSTR lpFile,
	     LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
  PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING NtName;
  IO_STATUS_BLOCK IoStatusBlock;
  HANDLE FileHandle;
  HKEY KeyHandle;
  NTSTATUS Status;
  LONG ErrorCode;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFile,
				     &NtName,
				     NULL,
				     NULL))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  if (lpSecurityAttributes != NULL)
    {
      SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
    }

  InitializeObjectAttributes (&ObjectAttributes,
			      &NtName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      SecurityDescriptor);
  Status = NtCreateFile (&FileHandle,
			 GENERIC_WRITE | SYNCHRONIZE,
			 &ObjectAttributes,
			 &IoStatusBlock,
			 NULL,
			 FILE_ATTRIBUTE_NORMAL,
			 FILE_SHARE_READ,
			 FILE_CREATE,
			 FILE_OPEN_FOR_BACKUP_INTENT | FILE_SYNCHRONOUS_IO_NONALERT,
			 NULL,
			 0);
  RtlFreeUnicodeString (&NtName);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtSaveKey (KeyHandle,
		      FileHandle);
  NtClose (FileHandle);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSetKeySecurity
 *
 * @implemented
 */
LONG STDCALL
RegSetKeySecurity (HKEY hKey,
		   SECURITY_INFORMATION SecurityInformation,
		   PSECURITY_DESCRIPTOR pSecurityDescriptor)
{
  HKEY KeyHandle;
  NTSTATUS Status;
  LONG ErrorCode;

  if (hKey == HKEY_PERFORMANCE_DATA)
    return ERROR_INVALID_HANDLE;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtSetSecurityObject (KeyHandle,
				SecurityInformation,
				pSecurityDescriptor);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSetValueExA
 *
 * @implemented
 */
LONG STDCALL
RegSetValueExA (HKEY hKey,
		LPCSTR lpValueName,
		DWORD Reserved,
		DWORD dwType,
		CONST BYTE* lpData,
		DWORD cbData)
{
  UNICODE_STRING ValueName;
  LPWSTR pValueName;
  ANSI_STRING AnsiString;
  UNICODE_STRING Data;
  LONG ErrorCode;
  LPBYTE pData;
  DWORD DataSize;

  if (lpData == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  if (lpValueName != NULL &&
      strlen(lpValueName) != 0)
    {
      RtlCreateUnicodeStringFromAsciiz (&ValueName,
					(LPSTR)lpValueName);
      pValueName = (LPWSTR)ValueName.Buffer;
    }
  else
    {
      pValueName = NULL;
    }

  if ((dwType == REG_SZ) ||
      (dwType == REG_MULTI_SZ) ||
      (dwType == REG_EXPAND_SZ))
    {
      RtlInitAnsiString (&AnsiString,
			 NULL);
      AnsiString.Buffer = (LPSTR)lpData;
      AnsiString.Length = cbData;
      AnsiString.MaximumLength = cbData;
      RtlAnsiStringToUnicodeString (&Data,
				    &AnsiString,
				    TRUE);
      pData = (LPBYTE)Data.Buffer;
      DataSize = cbData * sizeof(WCHAR);
    }
  else
    {
      RtlInitUnicodeString (&Data,
			    NULL);
      pData = (LPBYTE)lpData;
      DataSize = cbData;
    }

  ErrorCode = RegSetValueExW (hKey,
			      pValueName,
			      Reserved,
			      dwType,
			      pData,
			      DataSize);
  if (pValueName != NULL)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   ValueName.Buffer);
    }

  if (Data.Buffer != NULL)
    {
      RtlFreeHeap (RtlGetProcessHeap (),
		   0,
		   Data.Buffer);
    }

  return ErrorCode;
}


/************************************************************************
 *  RegSetValueExW
 *
 * @implemented
 */
LONG STDCALL
RegSetValueExW (HKEY hKey,
		LPCWSTR lpValueName,
		DWORD Reserved,
		DWORD dwType,
		CONST BYTE* lpData,
		DWORD cbData)
{
  UNICODE_STRING ValueName;
  PUNICODE_STRING pValueName;
  HKEY KeyHandle;
  NTSTATUS Status;
  LONG ErrorCode;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (lpValueName != NULL)
    {
      RtlInitUnicodeString (&ValueName,
			    lpValueName);
      pValueName = &ValueName;
    }
  else
    {
      pValueName = NULL;
    }

  Status = NtSetValueKey (KeyHandle,
			  pValueName,
			  0,
			  dwType,
			  (PVOID)lpData,
			  (ULONG)cbData);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSetValueA
 *
 * @implemented
 */
LONG STDCALL
RegSetValueA (HKEY hKey,
	      LPCSTR lpSubKey,
	      DWORD dwType,
	      LPCSTR lpData,
	      DWORD cbData)
{
  WCHAR SubKeyNameBuffer[MAX_PATH+1];
  UNICODE_STRING SubKeyName;
  UNICODE_STRING Data;
  ANSI_STRING AnsiString;
  LONG DataSize;
  LONG ErrorCode;

  if (lpData == NULL)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  RtlInitUnicodeString (&SubKeyName, NULL);
  RtlInitUnicodeString (&Data, NULL);
  if (lpSubKey != NULL && (strlen(lpSubKey) != 0))
    {
      RtlInitAnsiString (&AnsiString, (LPSTR)lpSubKey);
      SubKeyName.Buffer = &SubKeyNameBuffer[0];
      SubKeyName.MaximumLength = sizeof(SubKeyNameBuffer);
      RtlAnsiStringToUnicodeString (&SubKeyName, &AnsiString, FALSE);
    }

  DataSize = cbData * sizeof(WCHAR);
  Data.MaximumLength = DataSize;
  Data.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
				 0,
				 DataSize);
  if (Data.Buffer == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  ErrorCode = RegSetValueW (hKey,
			    (LPCWSTR)SubKeyName.Buffer,
			    dwType,
			    Data.Buffer,
			    DataSize);
  RtlFreeHeap (RtlGetProcessHeap (),
	       0,
	       Data.Buffer);

  return ErrorCode;
}


/************************************************************************
 *  RegSetValueW
 *
 * @implemented
 */
LONG STDCALL
RegSetValueW (HKEY hKey,
	      LPCWSTR lpSubKey,
	      DWORD dwType,
	      LPCWSTR lpData,
	      DWORD cbData)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING SubKeyString;
  HKEY KeyHandle;
  HANDLE RealKey;
  LONG ErrorCode;
  BOOL CloseRealKey;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if ((lpSubKey) && (wcslen(lpSubKey) != 0))
    {
      RtlInitUnicodeString (&SubKeyString,
			    (LPWSTR)lpSubKey);
      InitializeObjectAttributes (&ObjectAttributes,
				  &SubKeyString,
				  OBJ_CASE_INSENSITIVE,
				  KeyHandle,
				  NULL);
      Status = NtOpenKey (&RealKey,
			  KEY_ALL_ACCESS,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  ErrorCode = RtlNtStatusToDosError (Status);
	  SetLastError (ErrorCode);
	  return ErrorCode;
	}
      CloseRealKey = TRUE;
    }
  else
    {
      RealKey = hKey;
      CloseRealKey = FALSE;
    }

  ErrorCode = RegSetValueExW (RealKey,
			      NULL,
			      0,
			      dwType,
			      (LPBYTE)lpData,
			      cbData);
  if (CloseRealKey == TRUE)
    {
      NtClose (RealKey);
    }

  return ErrorCode;
}


/************************************************************************
 *  RegUnLoadKeyA
 *
 * @implemented
 */
LONG STDCALL
RegUnLoadKeyA (HKEY hKey,
	       LPCSTR lpSubKey)
{
  UNICODE_STRING KeyName;
  DWORD ErrorCode;

  RtlCreateUnicodeStringFromAsciiz (&KeyName,
				    (LPSTR)lpSubKey);

  ErrorCode = RegUnLoadKeyW (hKey,
			     KeyName.Buffer);

  RtlFreeUnicodeString (&KeyName);

  return ErrorCode;
}


/************************************************************************
 *  RegUnLoadKeyW
 *
 * @implemented
 */
LONG STDCALL
RegUnLoadKeyW (HKEY  hKey,
	       LPCWSTR lpSubKey)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  HANDLE KeyHandle;
  DWORD ErrorCode;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    return ERROR_INVALID_HANDLE;

  Status = MapDefaultKey (&KeyHandle, hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  RtlInitUnicodeString (&KeyName,
			(LPWSTR)lpSubKey);

  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      KeyHandle,
			      NULL);

  Status = NtUnloadKey (&ObjectAttributes);

  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}

/* EOF */
