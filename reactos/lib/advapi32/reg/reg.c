/* $Id: reg.c,v 1.45 2004/04/01 13:53:08 ekohl Exp $
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
#include <rosrtl/string.h>
#include <ntdll/rtl.h>
#include <windows.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

/* DEFINES ******************************************************************/

#define MAX_DEFAULT_HANDLES   6
#define REG_MAX_NAME_SIZE     256
#define REG_MAX_DATA_SIZE     2048	

/* GLOBALS ******************************************************************/

static CRITICAL_SECTION HandleTableCS;
static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];
static HANDLE ProcessHeap;

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

  ProcessHeap = RtlGetProcessHeap();
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
	    Status = RtlOpenCurrentUser (MAXIMUM_ALLOWED,
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
  UNICODE_STRING KeyName = ROS_STRING_INITIALIZER(L"\\Registry\\Machine\\Software\\CLASSES");

  DPRINT("OpenClassesRootKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    MAXIMUM_ALLOWED,
		    &Attributes);
}


static NTSTATUS
OpenLocalMachineKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName = ROS_STRING_INITIALIZER(L"\\Registry\\Machine");
  NTSTATUS Status;

  DPRINT("OpenLocalMachineKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtOpenKey (KeyHandle,
		      MAXIMUM_ALLOWED,
		      &Attributes);

  DPRINT("NtOpenKey(%wZ) => %08x\n", &KeyName, Status);
  return Status;
}


static NTSTATUS
OpenUsersKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName = ROS_STRING_INITIALIZER(L"\\Registry\\User");

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
  ROS_STRING_INITIALIZER(L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");

  DPRINT("OpenCurrentConfigKey()\n");

  InitializeObjectAttributes (&Attributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  return NtOpenKey (KeyHandle,
		    MAXIMUM_ALLOWED,
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

  DPRINT("RegCreateKeyExA() called\n");

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
RegCreateKeyExW (HKEY hKey,
		 LPCWSTR lpSubKey,
		 DWORD Reserved,
		 LPWSTR lpClass,
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
  union
  {
    KEY_NODE_INFORMATION Node;
    KEY_BASIC_INFORMATION Basic;
  } *KeyInfo;

  UNICODE_STRING StringU;
  ANSI_STRING StringA;
  LONG ErrorCode = ERROR_SUCCESS;
  DWORD NameLength;
  DWORD ClassLength;
  DWORD BufferSize;
  DWORD ResultSize;
  HKEY KeyHandle;
  NTSTATUS Status;

  DPRINT("RegEnumKeyExA(hKey 0x%x, dwIndex %d, lpName 0x%x, *lpcbName %d, lpClass 0x%x, lpcbClass %d)\n",
         hKey, dwIndex, lpName, *lpcbName, lpClass, lpcbClass ? *lpcbClass : 0);

  if ((lpClass) && (!lpcbClass))
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  Status = MapDefaultKey(&KeyHandle,
			 hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (*lpcbName > 0)
    {
      NameLength = min (*lpcbName - 1 , REG_MAX_NAME_SIZE) * sizeof (WCHAR);
    }
  else
    {
      NameLength = 0;
    }

  if (lpClass)
    {
      if (*lpcbClass > 0)
	{
	  ClassLength = min (*lpcbClass -1, REG_MAX_NAME_SIZE) * sizeof(WCHAR);
	}
      else
	{
	  ClassLength = 0;
	}

      /* The class name should start at a dword boundary */
      BufferSize = ((sizeof(KEY_NODE_INFORMATION) + NameLength + 3) & ~3) + ClassLength;
    }
  else
    {
      BufferSize = sizeof(KEY_BASIC_INFORMATION) + NameLength;
    }

  KeyInfo = RtlAllocateHeap (ProcessHeap,
			     0,
			     BufferSize);
  if (KeyInfo == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  Status = NtEnumerateKey (KeyHandle,
			   (ULONG)dwIndex,
			   lpClass == NULL ? KeyBasicInformation : KeyNodeInformation,
			   KeyInfo,
			   BufferSize,
			   &ResultSize);
  DPRINT("NtEnumerateKey() returned status 0x%X\n", Status);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
    }
  else
    {
      if (lpClass == NULL)
	{
	  if (KeyInfo->Basic.NameLength > NameLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      StringU.Buffer = KeyInfo->Basic.Name;
	      StringU.Length = KeyInfo->Basic.NameLength;
	      StringU.MaximumLength = KeyInfo->Basic.NameLength;
	    }
	}
      else
	{
	  if (KeyInfo->Node.NameLength > NameLength ||
	      KeyInfo->Node.ClassLength > ClassLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      StringA.Buffer = lpClass;
	      StringA.Length = 0;
	      StringA.MaximumLength = *lpcbClass;
	      StringU.Buffer = (PWCHAR)((ULONG_PTR)KeyInfo->Node.Name + KeyInfo->Node.ClassOffset);
	      StringU.Length = KeyInfo->Node.ClassLength;
	      StringU.MaximumLength = KeyInfo->Node.ClassLength;
	      RtlUnicodeStringToAnsiString (&StringA, &StringU, FALSE);
	      lpClass[StringA.Length] = 0;
	      *lpcbClass = StringA.Length;
	      StringU.Buffer = KeyInfo->Node.Name;
	      StringU.Length = KeyInfo->Node.NameLength;
	      StringU.MaximumLength = KeyInfo->Node.NameLength;
	    }
	}

      if (ErrorCode == ERROR_SUCCESS)
	{
	  StringA.Buffer = lpName;
	  StringA.Length = 0;
	  StringA.MaximumLength = *lpcbName;
	  RtlUnicodeStringToAnsiString (&StringA, &StringU, FALSE);
	  lpName[StringA.Length] = 0;
	  *lpcbName = StringA.Length;
	  if (lpftLastWriteTime != NULL)
	    {
	      if (lpClass == NULL)
		{
		  lpftLastWriteTime->dwLowDateTime = KeyInfo->Basic.LastWriteTime.u.LowPart;
		  lpftLastWriteTime->dwHighDateTime = KeyInfo->Basic.LastWriteTime.u.HighPart;
		}
	      else
		{
		  lpftLastWriteTime->dwLowDateTime = KeyInfo->Node.LastWriteTime.u.LowPart;
		  lpftLastWriteTime->dwHighDateTime = KeyInfo->Node.LastWriteTime.u.HighPart;
		}
	    }
	}
    }

  DPRINT("Key Namea0 Length %d\n", StringU.Length);
  DPRINT("Key Namea1 Length %d\n", NameLength);
  DPRINT("Key Namea Length %d\n", *lpcbName);
  DPRINT("Key Namea %s\n", lpName);

  RtlFreeHeap (ProcessHeap,
	       0,
	       KeyInfo);

  if (ErrorCode != ERROR_SUCCESS)
    {
      SetLastError(ErrorCode);
    }

  return ErrorCode;
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
  union
  {
    KEY_NODE_INFORMATION Node;
    KEY_BASIC_INFORMATION Basic;
  } *KeyInfo;

  ULONG BufferSize;
  ULONG ResultSize;
  ULONG NameLength;
  ULONG ClassLength;
  HKEY KeyHandle;
  LONG ErrorCode = ERROR_SUCCESS;
  NTSTATUS Status;

  Status = MapDefaultKey(&KeyHandle,
			 hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (*lpcbName > 0)
    {
      NameLength = min (*lpcbName - 1, REG_MAX_NAME_SIZE) * sizeof (WCHAR);
    }
  else
    {
      NameLength = 0;
    }

  if (lpClass)
    {
      if (*lpcbClass > 0)
	{
	  ClassLength = min (*lpcbClass - 1, REG_MAX_NAME_SIZE) * sizeof(WCHAR);
	}
      else
	{
	  ClassLength = 0;
	}

      BufferSize = ((sizeof(KEY_NODE_INFORMATION) + NameLength + 3) & ~3) + ClassLength;
    }
  else
    {
      BufferSize = sizeof(KEY_BASIC_INFORMATION) + NameLength;
    }

  KeyInfo = RtlAllocateHeap (ProcessHeap,
			     0,
			     BufferSize);
  if (KeyInfo == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  Status = NtEnumerateKey (KeyHandle,
			   (ULONG)dwIndex,
			   lpClass ? KeyNodeInformation : KeyBasicInformation,
			   KeyInfo,
			   BufferSize,
			   &ResultSize);
  DPRINT("NtEnumerateKey() returned status 0x%X\n", Status);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
    }
  else
    {
      if (lpClass == NULL)
	{
	  if (KeyInfo->Basic.NameLength > NameLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      RtlCopyMemory (lpName,
			     KeyInfo->Basic.Name,
			     KeyInfo->Basic.NameLength);
	      *lpcbName = (DWORD)(KeyInfo->Basic.NameLength / sizeof(WCHAR));
	      lpName[*lpcbName] = 0;
	    }
	}
      else
	{
	  if (KeyInfo->Node.NameLength > NameLength ||
	      KeyInfo->Node.ClassLength > ClassLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      RtlCopyMemory (lpName,
			     KeyInfo->Node.Name,
			     KeyInfo->Node.NameLength);
	      *lpcbName = KeyInfo->Node.NameLength / sizeof(WCHAR);
	      lpName[*lpcbName] = 0;
	      RtlCopyMemory (lpClass,
			     (PVOID)((ULONG_PTR)KeyInfo->Node.Name + KeyInfo->Node.ClassOffset),
			     KeyInfo->Node.ClassLength);
	      *lpcbClass = (DWORD)(KeyInfo->Node.ClassLength / sizeof(WCHAR));
	      lpClass[*lpcbClass] = 0;
	    }
	}

      if (ErrorCode == ERROR_SUCCESS && lpftLastWriteTime != NULL)
	{
	  if (lpClass == NULL)
	    {
	      lpftLastWriteTime->dwLowDateTime = KeyInfo->Basic.LastWriteTime.u.LowPart;
	      lpftLastWriteTime->dwHighDateTime = KeyInfo->Basic.LastWriteTime.u.HighPart;
	    }
	  else
	    {
	      lpftLastWriteTime->dwLowDateTime = KeyInfo->Node.LastWriteTime.u.LowPart;
	      lpftLastWriteTime->dwHighDateTime = KeyInfo->Node.LastWriteTime.u.HighPart;
	    }
	}
    }

  RtlFreeHeap (ProcessHeap,
	       0,
	       KeyInfo);

  if (ErrorCode != ERROR_SUCCESS)
    {
      SetLastError(ErrorCode);
    }

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
  union
  {
    KEY_VALUE_FULL_INFORMATION Full;
    KEY_VALUE_BASIC_INFORMATION Basic;
  } *ValueInfo;

  ULONG NameLength;
  ULONG BufferSize;
  ULONG DataLength;
  ULONG ResultSize;
  HKEY KeyHandle;
  LONG ErrorCode;
  NTSTATUS Status;
  UNICODE_STRING StringU;
  ANSI_STRING StringA;
  BOOL IsStringType;

  ErrorCode = ERROR_SUCCESS;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  if (*lpcbValueName > 0)
    {
      NameLength = min (*lpcbValueName - 1, REG_MAX_NAME_SIZE) * sizeof(WCHAR);
    }
  else
    {
      NameLength = 0;
    }

  if (lpData)
    {
      DataLength = min (*lpcbData * sizeof(WCHAR), REG_MAX_DATA_SIZE);
      BufferSize = ((sizeof(KEY_VALUE_FULL_INFORMATION) + NameLength + 3) & ~3) + DataLength;
    }
  else
    {
      BufferSize = sizeof(KEY_VALUE_BASIC_INFORMATION) + NameLength;
    }

  ValueInfo = RtlAllocateHeap (ProcessHeap,
			       0,
			       BufferSize);
  if (ValueInfo == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }

  Status = NtEnumerateValueKey (KeyHandle,
				(ULONG)dwIndex,
				lpData ? KeyValueFullInformation : KeyValueBasicInformation,
				ValueInfo,
				BufferSize,
				&ResultSize);

  DPRINT("NtEnumerateValueKey() returned status 0x%X\n", Status);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
    }
  else
    {
      if (lpData)
        {
	  IsStringType = (ValueInfo->Full.Type == REG_SZ) ||
			 (ValueInfo->Full.Type == REG_MULTI_SZ) ||
			 (ValueInfo->Full.Type == REG_EXPAND_SZ);
	  if (ValueInfo->Full.NameLength > NameLength ||
	      (!IsStringType && ValueInfo->Full.DataLength > *lpcbData) ||
	      ValueInfo->Full.DataLength > DataLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      if (IsStringType)
	        {
		  StringU.Buffer = (PWCHAR)((ULONG_PTR)ValueInfo + ValueInfo->Full.DataOffset);
		  StringU.Length = ValueInfo->Full.DataLength;
		  StringU.MaximumLength = DataLength;
		  StringA.Buffer = (PCHAR)lpData;
		  StringA.Length = 0;
		  StringA.MaximumLength = *lpcbData;
		  RtlUnicodeStringToAnsiString (&StringA,
						&StringU,
						FALSE);
		  *lpcbData = StringA.Length;
		}
	      else
		{
		  RtlCopyMemory (lpData,
				 (PVOID)((ULONG_PTR)ValueInfo + ValueInfo->Full.DataOffset),
				 ValueInfo->Full.DataLength);
		  *lpcbData = ValueInfo->Full.DataLength;
		}

	      StringU.Buffer = ValueInfo->Full.Name;
	      StringU.Length = ValueInfo->Full.NameLength;
	      StringU.MaximumLength = NameLength;
	    }
	}
      else
	{
	  if (ValueInfo->Basic.NameLength > NameLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      StringU.Buffer = ValueInfo->Basic.Name;
	      StringU.Length = ValueInfo->Basic.NameLength;
	      StringU.MaximumLength = NameLength;
	    }
	}

      if (ErrorCode == ERROR_SUCCESS)
        {
	  StringA.Buffer = (PCHAR)lpValueName;
	  StringA.Length = 0;
	  StringA.MaximumLength = *lpcbValueName;
	  RtlUnicodeStringToAnsiString (&StringA,
					&StringU,
					FALSE);
	  StringA.Buffer[StringA.Length] = 0;
	  *lpcbValueName = StringA.Length;
	  if (lpType)
	    {
	      *lpType = lpData ? ValueInfo->Full.Type : ValueInfo->Basic.Type;
	    }
	}
    }

  RtlFreeHeap (ProcessHeap,
	       0,
	       ValueInfo);
  if (ErrorCode != ERROR_SUCCESS)
    {
      SetLastError(ErrorCode);
    }

  return ErrorCode;
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
  union
  {
    KEY_VALUE_FULL_INFORMATION Full;
    KEY_VALUE_BASIC_INFORMATION Basic;
  } *ValueInfo;

  ULONG NameLength;
  ULONG BufferSize;
  ULONG DataLength;
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

  if (*lpcbValueName > 0)
    {
      NameLength = min (*lpcbValueName - 1, REG_MAX_NAME_SIZE) * sizeof(WCHAR);
    }
  else
    {
      NameLength = 0;
    }

  if (lpData)
    {
      DataLength = min(*lpcbData, REG_MAX_DATA_SIZE);
      BufferSize = ((sizeof(KEY_VALUE_FULL_INFORMATION) + NameLength + 3) & ~3) + DataLength;
    }
  else
    {
      BufferSize = sizeof(KEY_VALUE_BASIC_INFORMATION) + NameLength;
    }
  ValueInfo = RtlAllocateHeap (ProcessHeap,
			       0,
			       BufferSize);
  if (ValueInfo == NULL)
    {
      SetLastError (ERROR_OUTOFMEMORY);
      return ERROR_OUTOFMEMORY;
    }
  Status = NtEnumerateValueKey (KeyHandle,
				(ULONG)dwIndex,
				lpData ? KeyValueFullInformation : KeyValueBasicInformation,
				ValueInfo,
				BufferSize,
				&ResultSize);

  DPRINT("NtEnumerateValueKey() returned status 0x%X\n", Status);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
    }
  else
    {
      if (lpData)
	{
	  if (ValueInfo->Full.DataLength > DataLength ||
	      ValueInfo->Full.NameLength > NameLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      RtlCopyMemory (lpValueName,
			     ValueInfo->Full.Name,
			     ValueInfo->Full.NameLength);
	      *lpcbValueName = (DWORD)(ValueInfo->Full.NameLength / sizeof(WCHAR));
	      lpValueName[*lpcbValueName] = 0;
	      RtlCopyMemory (lpData,
			     (PVOID)((ULONG_PTR)ValueInfo + ValueInfo->Full.DataOffset),
			     ValueInfo->Full.DataLength);
	      *lpcbData = (DWORD)ValueInfo->Full.DataLength;
	    }
	}
      else
	{
	  if (ValueInfo->Basic.NameLength > NameLength)
	    {
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	    }
	  else
	    {
	      RtlCopyMemory (lpValueName,
			     ValueInfo->Basic.Name,
			     ValueInfo->Basic.NameLength);
	      *lpcbValueName = (DWORD)(ValueInfo->Basic.NameLength / sizeof(WCHAR));
	      lpValueName[*lpcbValueName] = 0;
	    }
	}

      if (ErrorCode == ERROR_SUCCESS && lpType != NULL)
	{
	  *lpType = lpData ? ValueInfo->Full.Type : ValueInfo->Basic.Type;
	}
    }

  RtlFreeHeap (ProcessHeap,
	       0,
	       ValueInfo);

  if (ErrorCode != ERROR_SUCCESS)
    {
      SetLastError (ErrorCode);
    }

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
  LONG ErrorCode;

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
  LONG ErrorCode;
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
		      MAXIMUM_ALLOWED,
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
		      MAXIMUM_ALLOWED,
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
      AnsiString.MaximumLength = *lpcbClass;
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
      AnsiString.Buffer = lpClass;
      AnsiString.Length = 0;
      UnicodeString.Length = *lpcbClass * sizeof(WCHAR);
      RtlUnicodeStringToAnsiString (&AnsiString,
				    &UnicodeString,
				    FALSE);
      *lpcbClass = AnsiString.Length;
      lpClass[AnsiString.Length] = 0;
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
  ULONG ClassLength;
  HKEY KeyHandle;
  NTSTATUS Status;
  LONG ErrorCode = ERROR_SUCCESS;
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
      if (*lpcbClass > 0)
	{
	  ClassLength = min(*lpcbClass - 1, REG_MAX_NAME_SIZE) * sizeof(WCHAR);
	}
      else
	{
	  ClassLength = 0;
	}

      FullInfoSize = sizeof(KEY_FULL_INFORMATION) + ((ClassLength + 3) & ~3);
      FullInfo = RtlAllocateHeap (ProcessHeap,
				  0,
				  FullInfoSize);
      if (FullInfo == NULL)
	{
	  SetLastError (ERROR_OUTOFMEMORY);
	  return ERROR_OUTOFMEMORY;
	}

      FullInfo->ClassLength = ClassLength;
    }
  else
    {
      FullInfoSize = sizeof(KEY_FULL_INFORMATION);
      FullInfo = &FullInfoBuffer;
      FullInfo->ClassLength = 0;
    }
  FullInfo->ClassOffset = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

  Status = NtQueryKey (KeyHandle,
		       KeyFullInformation,
		       FullInfo,
		       FullInfoSize,
		       &Length);
  DPRINT("NtQueryKey() returned status 0x%X\n", Status);
  if (!NT_SUCCESS(Status))
    {
      if (lpClass != NULL)
	{
	  RtlFreeHeap (ProcessHeap,
		       0,
		       FullInfo);
	}

      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  DPRINT("SubKeys %d\n", FullInfo->SubKeys);
  if (lpcSubKeys != NULL)
    {
      *lpcSubKeys = FullInfo->SubKeys;
    }

  DPRINT("MaxNameLen %lu\n", FullInfo->MaxNameLen);
  if (lpcbMaxSubKeyLen != NULL)
    {
      *lpcbMaxSubKeyLen = FullInfo->MaxNameLen / sizeof(WCHAR) + 1;
    }

  DPRINT("MaxClassLen %lu\n", FullInfo->MaxClassLen);
  if (lpcbMaxClassLen != NULL)
    {
      *lpcbMaxClassLen = FullInfo->MaxClassLen / sizeof(WCHAR) + 1;
    }

  DPRINT("Values %lu\n", FullInfo->Values);
  if (lpcValues != NULL)
    {
      *lpcValues = FullInfo->Values;
    }

  DPRINT("MaxValueNameLen %lu\n", FullInfo->MaxValueNameLen);
  if (lpcbMaxValueNameLen != NULL)
    {
      *lpcbMaxValueNameLen = FullInfo->MaxValueNameLen / sizeof(WCHAR) + 1;
    }

  DPRINT("MaxValueDataLen %lu\n", FullInfo->MaxValueDataLen);
  if (lpcbMaxValueLen != NULL)
    {
      *lpcbMaxValueLen = FullInfo->MaxValueDataLen;
    }

  if (lpcbSecurityDescriptor != NULL)
    {
      /* FIXME */
      *lpcbSecurityDescriptor = 0;
    }

  if (lpftLastWriteTime != NULL)
    {
      lpftLastWriteTime->dwLowDateTime = FullInfo->LastWriteTime.u.LowPart;
      lpftLastWriteTime->dwHighDateTime = FullInfo->LastWriteTime.u.HighPart;
    }

  if (lpClass != NULL)
    {
      if (FullInfo->ClassLength > ClassLength)
	{
	      ErrorCode = ERROR_BUFFER_OVERFLOW;
	}
      else
	{
	  RtlCopyMemory (lpClass,
			 FullInfo->Class,
			 FullInfo->ClassLength);
	  *lpcbClass = FullInfo->ClassLength / sizeof(WCHAR);
	  lpClass[*lpcbClass] = 0;
	}

      RtlFreeHeap (ProcessHeap,
		   0,
		   FullInfo);
    }

  if (ErrorCode != ERROR_SUCCESS)
    {
      SetLastError (ErrorCode);
    }

  return ErrorCode;
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
 * @implemented
 */
LONG STDCALL
RegQueryMultipleValuesW (HKEY hKey,
			 PVALENTW val_list,
			 DWORD num_vals,
			 LPWSTR lpValueBuf,
			 LPDWORD ldwTotsize)
{
  ULONG i;
  DWORD maxBytes = *ldwTotsize;
  LPSTR bufptr = (LPSTR)lpValueBuf;
  LONG ErrorCode;

  if (maxBytes >= (1024*1024))
    return ERROR_TRANSFER_TOO_LONG;

  *ldwTotsize = 0;

  DPRINT ("RegQueryMultipleValuesW(%p,%p,%ld,%p,%p=%ld)\n",
	  hKey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

  for (i = 0; i < num_vals; ++i)
    {
      val_list[i].ve_valuelen = 0;
      ErrorCode = RegQueryValueExW (hKey,
				    val_list[i].ve_valuename,
				    NULL,
				    NULL,
				    NULL,
				    &val_list[i].ve_valuelen);
      if(ErrorCode != ERROR_SUCCESS)
	{
	  return ErrorCode;
	}

      if(lpValueBuf != NULL && *ldwTotsize + val_list[i].ve_valuelen <= maxBytes)
	{
	  ErrorCode = RegQueryValueExW (hKey,
					val_list[i].ve_valuename,
					NULL,
					&val_list[i].ve_type,
					bufptr,
					&val_list[i].ve_valuelen);
	  if (ErrorCode != ERROR_SUCCESS)
	    {
	      return ErrorCode;
	    }

	  val_list[i].ve_valueptr = (DWORD_PTR)bufptr;

	  bufptr += val_list[i].ve_valuelen;
	}

      *ldwTotsize += val_list[i].ve_valuelen;
    }

  return (lpValueBuf != NULL && *ldwTotsize <= maxBytes) ? ERROR_SUCCESS : ERROR_MORE_DATA;
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
  ULONG MaxCopy = lpcbData ? *lpcbData : 0;

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
  BufferSize = sizeof (KEY_VALUE_PARTIAL_INFORMATION) + MaxCopy;
  ValueInfo = RtlAllocateHeap (ProcessHeap,
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
      MaxCopy = 0;
      ErrorCode = lpData ? ERROR_MORE_DATA : ERROR_SUCCESS;
    }
  else if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      MaxCopy = 0;
      if (lpcbData != NULL)
	{
	  ResultSize = sizeof(*ValueInfo) + *lpcbData;
	}
    }

  if (lpType != NULL)
    {
      *lpType = ValueInfo->Type;
    }

  if (NT_SUCCESS(Status) && lpData != NULL)
    {
      RtlMoveMemory (lpData,
		     ValueInfo->Data,
		     min(ValueInfo->DataLength, MaxCopy));
    }

  if ((ValueInfo->Type == REG_SZ) ||
      (ValueInfo->Type == REG_MULTI_SZ) ||
      (ValueInfo->Type == REG_EXPAND_SZ))
    {
      if (lpData != NULL && MaxCopy > ValueInfo->DataLength)
	{
	  ((PWSTR)lpData)[ValueInfo->DataLength / sizeof(WCHAR)] = 0;
	}

      if (lpcbData != NULL)
	{
	  *lpcbData = (ResultSize - sizeof(*ValueInfo));
	  DPRINT("(string) Returning Size: %lu\n", *lpcbData);
	}
    }
  else
    {
      if (lpcbData != NULL)
	{
	  *lpcbData = ResultSize - sizeof(*ValueInfo);
	  DPRINT("(other) Returning Size: %lu\n", *lpcbData);
	}
    }

  DPRINT("Type %d  Size %d\n", ValueInfo->Type, ValueInfo->DataLength);

  RtlFreeHeap (ProcessHeap,
	       0,
	       ValueInfo);

  return ErrorCode;
}


/************************************************************************
 *  RegQueryValueExA
 *
 * @implemented
 */
LONG STDCALL
RegQueryValueExA (HKEY hKey,
		  LPCSTR lpValueName,
		  LPDWORD lpReserved,
		  LPDWORD lpType,
		  LPBYTE  lpData,
		  LPDWORD lpcbData)
{
  UNICODE_STRING ValueName;
  UNICODE_STRING ValueData;
  ANSI_STRING AnsiString;
  LONG ErrorCode;
  DWORD Length;
  DWORD Type;

  if (lpData != NULL && lpcbData == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  if (lpData)
    {
      ValueData.Length = *lpcbData * sizeof(WCHAR);
      ValueData.MaximumLength = ValueData.Length + sizeof(WCHAR);
      ValueData.Buffer = RtlAllocateHeap (ProcessHeap,
					  0,
					  ValueData.MaximumLength);
      if (!ValueData.Buffer)
	{
	  SetLastError(ERROR_OUTOFMEMORY);
	  return ERROR_OUTOFMEMORY;
	}
    }
  else
    {
      ValueData.Buffer = NULL;
      ValueData.Length = 0;
      ValueData.MaximumLength = 0;
    }

  RtlCreateUnicodeStringFromAsciiz (&ValueName,
				    (LPSTR)lpValueName);

  Length = *lpcbData * sizeof(WCHAR);
  ErrorCode = RegQueryValueExW (hKey,
				ValueName.Buffer,
				lpReserved,
				&Type,
				(LPBYTE)ValueData.Buffer,
				&Length);
  DPRINT("ErrorCode %lu\n", ErrorCode);

  if (ErrorCode == ERROR_SUCCESS ||
      ErrorCode == ERROR_MORE_DATA)
    {
      if (lpType != NULL)
	{
	  *lpType = Type;
	}

      if ((Type == REG_SZ) || (Type == REG_MULTI_SZ) || (Type == REG_EXPAND_SZ))
	{
	  if (ErrorCode == ERROR_SUCCESS && ValueData.Buffer != NULL)
	    {
	      RtlInitAnsiString(&AnsiString, NULL);
	      AnsiString.Buffer = lpData;
	      AnsiString.MaximumLength = *lpcbData;
	      ValueData.Length = Length;
	      ValueData.MaximumLength = ValueData.Length + sizeof(WCHAR);
	      RtlUnicodeStringToAnsiString(&AnsiString, &ValueData, FALSE);
	    }
	  Length = Length / sizeof(WCHAR);
	}
      else
	{
	  Length = min(*lpcbData, Length);
	  if (ErrorCode == ERROR_SUCCESS && ValueData.Buffer != NULL)
	    {
	      RtlMoveMemory(lpData, ValueData.Buffer, Length);
	    }
	}

      if (lpcbData != NULL)
	{
	  *lpcbData = Length;
	}
    }

  if (ValueData.Buffer != NULL)
    {
      RtlFreeHeap(ProcessHeap, 0, ValueData.Buffer);
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
      Value.Buffer = RtlAllocateHeap (ProcessHeap,
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
      RtlFreeHeap (ProcessHeap,
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
 * @implemented
 */
LONG STDCALL
RegReplaceKeyA (HKEY hKey,
		LPCSTR lpSubKey,
		LPCSTR lpNewFile,
		LPCSTR lpOldFile)
{
  UNICODE_STRING SubKey;
  UNICODE_STRING NewFile;
  UNICODE_STRING OldFile;
  LONG ErrorCode;

  RtlCreateUnicodeStringFromAsciiz (&SubKey,
				    (PCSZ)lpSubKey);
  RtlCreateUnicodeStringFromAsciiz (&OldFile,
				    (PCSZ)lpOldFile);
  RtlCreateUnicodeStringFromAsciiz (&NewFile,
				    (PCSZ)lpNewFile);

  ErrorCode = RegReplaceKeyW (hKey,
			      SubKey.Buffer,
			      NewFile.Buffer,
			      OldFile.Buffer);

  RtlFreeUnicodeString (&OldFile);
  RtlFreeUnicodeString (&NewFile);
  RtlFreeUnicodeString (&SubKey);

  return ErrorCode;
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
  OBJECT_ATTRIBUTES KeyObjectAttributes;
  OBJECT_ATTRIBUTES NewObjectAttributes;
  OBJECT_ATTRIBUTES OldObjectAttributes;
  UNICODE_STRING SubKeyName;
  UNICODE_STRING NewFileName;
  UNICODE_STRING OldFileName;
  BOOLEAN CloseRealKey;
  HANDLE RealKeyHandle;
  HANDLE KeyHandle;
  LONG ErrorCode;
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

  /* Open the real key */
  if (lpSubKey != NULL && *lpSubKey != (WCHAR)0)
    {
      RtlInitUnicodeString (&SubKeyName,
			    (PWSTR)lpSubKey);
      InitializeObjectAttributes (&KeyObjectAttributes,
				  &SubKeyName,
				  OBJ_CASE_INSENSITIVE,
				  KeyHandle,
				  NULL);
      Status = NtOpenKey (&RealKeyHandle,
			  KEY_ALL_ACCESS,
			  &KeyObjectAttributes);
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
      RealKeyHandle = KeyHandle;
      CloseRealKey = FALSE;
    }

  /* Convert new file name */
  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewFile,
				     &NewFileName,
				     NULL,
				     NULL))
    {
      if (CloseRealKey)
	{
	  NtClose (RealKeyHandle);
	}
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  InitializeObjectAttributes (&NewObjectAttributes,
			      &NewFileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  /* Convert old file name */
  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpOldFile,
				     &OldFileName,
				     NULL,
				     NULL))
    {
      RtlFreeUnicodeString (&NewFileName);
      if (CloseRealKey)
	{
	  NtClose (RealKeyHandle);
	}
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  InitializeObjectAttributes (&OldObjectAttributes,
			      &OldFileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  Status = NtReplaceKey (&NewObjectAttributes,
			 RealKeyHandle,
			 &OldObjectAttributes);

  RtlFreeUnicodeString (&OldFileName);
  RtlFreeUnicodeString (&NewFileName);

  if (CloseRealKey)
    {
      NtClose (RealKeyHandle);
    }

  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegRestoreKeyA
 *
 * @implemented
 */
LONG STDCALL
RegRestoreKeyA (HKEY hKey,
		LPCSTR lpFile,
		DWORD dwFlags)
{
  UNICODE_STRING FileName;
  LONG ErrorCode;

  RtlCreateUnicodeStringFromAsciiz (&FileName,
				    (PCSZ)lpFile);

  ErrorCode = RegRestoreKeyW (hKey,
			      FileName.Buffer,
			      dwFlags);

  RtlFreeUnicodeString (&FileName);

  return ErrorCode;
}


/************************************************************************
 *  RegRestoreKeyW
 *
 * @implemented
 */
LONG STDCALL
RegRestoreKeyW (HKEY hKey,
		LPCWSTR lpFile,
		DWORD dwFlags)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING FileName;
  HANDLE FileHandle;
  HANDLE KeyHandle;
  LONG ErrorCode;
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
      SetLastError (ERROR_INVALID_PARAMETER);
      return ERROR_INVALID_PARAMETER;
    }

  InitializeObjectAttributes (&ObjectAttributes,
			      &FileName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  Status = NtOpenFile (&FileHandle,
		       FILE_GENERIC_READ,
		       &ObjectAttributes,
		       &IoStatusBlock,
		       FILE_SHARE_READ,
		       FILE_SYNCHRONOUS_IO_NONALERT);
  RtlFreeUnicodeString (&FileName);
  if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      SetLastError (ErrorCode);
      return ErrorCode;
    }

  Status = NtRestoreKey (KeyHandle,
			 FileHandle,
			 (ULONG)dwFlags);
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
 *  RegSaveKeyA
 *
 * @implemented
 */
LONG STDCALL
RegSaveKeyA (HKEY hKey,
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
  UNICODE_STRING FileName;
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

  if (!RtlDosPathNameToNtPathName_U ((PWSTR)lpFile,
				     &FileName,
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
			      &FileName,
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
  RtlFreeUnicodeString (&FileName);
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
  LONG ErrorCode;
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
					(PSTR)lpValueName);
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
      AnsiString.Buffer = (PSTR)lpData;
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
      RtlFreeHeap (ProcessHeap,
		   0,
		   ValueName.Buffer);
    }

  if (Data.Buffer != NULL)
    {
      RtlFreeHeap (ProcessHeap,
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
    }
  else
    {
      RtlInitUnicodeString (&ValueName, L"");
    }
  pValueName = &ValueName;

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
  Data.Buffer = RtlAllocateHeap (ProcessHeap,
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

  RtlFreeHeap (ProcessHeap,
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
RegUnLoadKeyW (HKEY hKey,
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
