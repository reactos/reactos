/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  19990309 EA Stubs
 *                  20050502 Fireball imported some stuff from WINE
 */

/* INCLUDES *****************************************************************/

#include <advapi32.h>
#define NDEBUG
#include <debug.h>

/* DEFINES ******************************************************************/

#define MAX_DEFAULT_HANDLES   6
#define REG_MAX_NAME_SIZE     256
#define REG_MAX_DATA_SIZE     2048

/* FIXME: should go into msvcrt.h header? */
#define offsetof(s,m)       (size_t)&(((s*)NULL)->m)

/* GLOBALS ******************************************************************/

static RTL_CRITICAL_SECTION HandleTableCS;
static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];
static HANDLE ProcessHeap;

/* PROTOTYPES ***************************************************************/

static NTSTATUS MapDefaultKey (PHANDLE ParentKey, HKEY Key);
static VOID CloseDefaultKeys(VOID);

static NTSTATUS OpenClassesRootKey(PHANDLE KeyHandle);
static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);
static NTSTATUS OpenUsersKey (PHANDLE KeyHandle);
static NTSTATUS OpenCurrentConfigKey(PHANDLE KeyHandle);


/* FUNCTIONS ****************************************************************/
/* check if value type needs string conversion (Ansi<->Unicode) */
inline static int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

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
MapDefaultKey (PHANDLE RealKey,
	       HKEY Key)
{
  PHANDLE Handle;
  ULONG Index;
  NTSTATUS Status = STATUS_SUCCESS;

  DPRINT("MapDefaultKey (Key %x)\n", Key);

  if (((ULONG)Key & 0xF0000000) != 0x80000000)
    {
      *RealKey = (HANDLE)Key;
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
      *RealKey = *Handle;
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
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\CLASSES");

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
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine");
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
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\User");

  DPRINT("OpenUsersKey()\n");

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
OpenCurrentConfigKey (PHANDLE KeyHandle)
{
  OBJECT_ATTRIBUTES Attributes;
  UNICODE_STRING KeyName =
  RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");

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
  NTSTATUS Status;

  /* don't close null handle or a pseudo handle */
  if ((!hKey) || (((ULONG)hKey & 0xF0000000) == 0x80000000))
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = NtClose (hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
 *  CreateNestedKey
 *
 *  Create key and all necessary intermediate keys
 */
static NTSTATUS
CreateNestedKey(PHKEY KeyHandle,
		POBJECT_ATTRIBUTES ObjectAttributes,
                PUNICODE_STRING ClassString,
                DWORD dwOptions,
                REGSAM samDesired,
                DWORD *lpdwDisposition)
{
  OBJECT_ATTRIBUTES LocalObjectAttributes;
  UNICODE_STRING LocalKeyName;
  ULONG Disposition;
  NTSTATUS Status;
  ULONG FullNameLength;
  ULONG Length;
  PWCHAR Ptr;
  HANDLE LocalKeyHandle;

  Status = NtCreateKey((PHANDLE) KeyHandle,
                       samDesired,
                       ObjectAttributes,
                       0,
                       ClassString,
                       dwOptions,
                       (PULONG)lpdwDisposition);
  DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", ObjectAttributes->ObjectName, Status);
  if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    return Status;

  /* Copy object attributes */
  RtlCopyMemory (&LocalObjectAttributes,
		 ObjectAttributes,
		 sizeof(OBJECT_ATTRIBUTES));
  RtlCreateUnicodeString (&LocalKeyName,
			  ObjectAttributes->ObjectName->Buffer);
  LocalObjectAttributes.ObjectName = &LocalKeyName;
  FullNameLength = LocalKeyName.Length / sizeof(WCHAR);

  /* Remove the last part of the key name and try to create the key again. */
  while (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      Ptr = wcsrchr (LocalKeyName.Buffer, '\\');
      if (Ptr == NULL || Ptr == LocalKeyName.Buffer)
	{
	  Status = STATUS_UNSUCCESSFUL;
	  break;
	}
      *Ptr = (WCHAR)0;
      LocalKeyName.Length = wcslen (LocalKeyName.Buffer) * sizeof(WCHAR);

      Status = NtCreateKey (&LocalKeyHandle,
			    KEY_ALL_ACCESS,
			    &LocalObjectAttributes,
			    0,
			    NULL,
			    0,
			    &Disposition);
      DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
    }

  if (!NT_SUCCESS(Status))
    {
      RtlFreeUnicodeString (&LocalKeyName);
      return Status;
    }

  /* Add removed parts of the key name and create them too. */
  Length = wcslen (LocalKeyName.Buffer);
  while (TRUE)
    {
      NtClose (LocalKeyHandle);

      LocalKeyName.Buffer[Length] = L'\\';
      Length = wcslen (LocalKeyName.Buffer);
      LocalKeyName.Length = Length * sizeof(WCHAR);

      if (Length == FullNameLength)
        {
          Status = NtCreateKey((PHANDLE) KeyHandle,
                               samDesired,
                               ObjectAttributes,
                               0,
                               ClassString,
                               dwOptions,
                               (PULONG)lpdwDisposition);
          break;
        }
      Status = NtCreateKey (&LocalKeyHandle,
			    KEY_CREATE_SUB_KEY,
			    &LocalObjectAttributes,
			    0,
			    NULL,
			    0,
			    &Disposition);
      DPRINT("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
      if (!NT_SUCCESS(Status))
	break;
    }

  RtlFreeUnicodeString (&LocalKeyName);

  return Status;
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
  HANDLE ParentKey;
  NTSTATUS Status;

  DPRINT("RegCreateKeyExA() called\n");

  /* get the real parent key */
  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }
  DPRINT("ParentKey %x\n", (ULONG)ParentKey);

  if (lpClass != NULL)
    {
      RtlCreateUnicodeStringFromAsciiz (&ClassString,
					lpClass);
    }

  RtlCreateUnicodeStringFromAsciiz(&SubKeyString,
				   (LPSTR)lpSubKey);
  InitializeObjectAttributes (&Attributes,
			      &SubKeyString,
			      OBJ_CASE_INSENSITIVE,
			      (HANDLE)ParentKey,
			      (PSECURITY_DESCRIPTOR)lpSecurityAttributes);
  Status = CreateNestedKey(phkResult,
			   &Attributes,
			   (lpClass == NULL)? NULL : &ClassString,
			   dwOptions,
			   samDesired,
			   lpdwDisposition);
  RtlFreeUnicodeString (&SubKeyString);
  if (lpClass != NULL)
    {
      RtlFreeUnicodeString (&ClassString);
    }

  DPRINT("Status %x\n", Status);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  HANDLE ParentKey;
  NTSTATUS Status;

  DPRINT("RegCreateKeyExW() called\n");

  /* get the real parent key */
  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError(Status);
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
  Status = CreateNestedKey(phkResult,
		           &Attributes,
                           (lpClass == NULL)? NULL : &ClassString,
                           dwOptions,
                           samDesired,
                           lpdwDisposition);
  DPRINT("Status %x\n", Status);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
			  MAXIMUM_ALLOWED,
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
			  MAXIMUM_ALLOWED,
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
  HANDLE ParentKey;
  HANDLE TargetKey;
  NTSTATUS Status;

  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  RtlCreateUnicodeStringFromAsciiz (&SubKeyName,
				    (LPSTR)lpSubKey);
  InitializeObjectAttributes(&ObjectAttributes,
			     &SubKeyName,
			     OBJ_CASE_INSENSITIVE,
			     ParentKey,
			     NULL);

  Status = NtOpenKey (&TargetKey,
		      DELETE,
		      &ObjectAttributes);
  RtlFreeUnicodeString (&SubKeyName);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  Status = NtDeleteKey (TargetKey);
  NtClose (TargetKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError(Status);
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
  HANDLE ParentKey;
  HANDLE TargetKey;
  NTSTATUS Status;

  Status = MapDefaultKey (&ParentKey,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  RtlInitUnicodeString (&SubKeyName,
			(LPWSTR)lpSubKey);
  InitializeObjectAttributes (&ObjectAttributes,
			      &SubKeyName,
			      OBJ_CASE_INSENSITIVE,
			      ParentKey,
			      NULL);
  Status = NtOpenKey (&TargetKey,
		      DELETE,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  Status = NtDeleteKey (TargetKey);
  NtClose (TargetKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  RtlCreateUnicodeStringFromAsciiz (&ValueName,
				    (LPSTR)lpValueName);
  Status = NtDeleteValueKey (KeyHandle,
			     &ValueName);
  RtlFreeUnicodeString (&ValueName);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  HANDLE KeyHandle;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  RtlInitUnicodeString (&ValueName,
			(LPWSTR)lpValueName);

  Status = NtDeleteValueKey (KeyHandle,
			     &ValueName);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
	DWORD ClassLength = 0;
	DWORD BufferSize;
	DWORD ResultSize;
	HANDLE KeyHandle;
	NTSTATUS Status;

	DPRINT("RegEnumKeyExA(hKey 0x%x, dwIndex %d, lpName 0x%x, *lpcbName %d, lpClass 0x%x, lpcbClass %d)\n",
		hKey, dwIndex, lpName, *lpcbName, lpClass, lpcbClass ? *lpcbClass : 0);

	if ((lpClass) && (!lpcbClass))
	{
		return ERROR_INVALID_PARAMETER;
	}

	Status = MapDefaultKey(&KeyHandle, hKey);
	if (!NT_SUCCESS(Status))
	{
		return RtlNtStatusToDosError (Status);
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

	KeyInfo = RtlAllocateHeap (ProcessHeap, 0, BufferSize);
	if (KeyInfo == NULL)
	{
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
  ULONG ClassLength = 0;
  HANDLE KeyHandle;
  LONG ErrorCode = ERROR_SUCCESS;
  NTSTATUS Status;

  Status = MapDefaultKey(&KeyHandle,
			 hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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

  return ErrorCode;
}

/************************************************************************
 *  RegEnumValueA
 *
 * @implemented
 */
LONG STDCALL
RegEnumValueA( HKEY hKey, DWORD index, LPSTR value, LPDWORD val_count,
               LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD count )
{
	HANDLE KeyHandle;
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    //TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
      //    hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;
	status = MapDefaultKey (&KeyHandle, hKey);
	if (!NT_SUCCESS(status))
	{
		return RtlNtStatusToDosError (status);
	}

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( KeyHandle, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    /* we need to fetch the contents for a string type even if not requested,
     * because we need to compute the length of the ASCII string. */
    if (value || data || is_string(info->Type))
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( KeyHandle, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (is_string(info->Type))
        {
            DWORD len;
            RtlUnicodeToMultiByteSize( &len, (WCHAR *)(buf_ptr + info->DataOffset),
                                       total_size - info->DataOffset );
            if (data && len)
            {
                if (len > *count) status = STATUS_BUFFER_OVERFLOW;
                else
                {
                    RtlUnicodeToMultiByteN( (PCHAR)data, len, NULL, (WCHAR *)(buf_ptr + info->DataOffset),
                                            total_size - info->DataOffset );
                    /* if the type is REG_SZ and data is not 0-terminated
                     * and there is enough space in the buffer NT appends a \0 */
                    if (len < *count && data[len-1]) data[len] = 0;
                }
            }
            info->DataLength = len;
        }
        else if (data)
        {
            if (total_size - info->DataOffset > *count) status = STATUS_BUFFER_OVERFLOW;
            else memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
        }

        if (value && !status)
        {
            DWORD len;

            RtlUnicodeToMultiByteSize( &len, info->Name, info->NameLength );
            if (len >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                if (*val_count)
                {
                    len = *val_count - 1;
                    RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                    value[len] = 0;
                }
            }
            else
            {
                RtlUnicodeToMultiByteN( value, len, NULL, info->Name, info->NameLength );
                value[len] = 0;
                *val_count = len;
            }
        }
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError(status);
}

/******************************************************************************
 * RegEnumValueW   [ADVAPI32.@]
 * @implemented
 *
 * PARAMS
 *  hkey       [I] Handle to key to query
 *  index      [I] Index of value to query
 *  value      [O] Value string
 *  val_count  [I/O] Size of value buffer (in wchars)
 *  reserved   [I] Reserved
 *  type       [O] Type code
 *  data       [O] Value data
 *  count      [I/O] Size of data buffer (in bytes)
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 */
LONG STDCALL
RegEnumValueW( HKEY hKey, DWORD index, LPWSTR value, PDWORD val_count,
               PDWORD reserved, PDWORD type, LPBYTE data, PDWORD count )
{
	HANDLE KeyHandle;
    NTSTATUS status;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_FULL_INFORMATION, Name );

    //TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
    //      hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

	status = MapDefaultKey (&KeyHandle, hKey);
	if (!NT_SUCCESS(status))
	{
		return RtlNtStatusToDosError (status);
	}

    total_size = info_size + (MAX_PATH + 1) * sizeof(WCHAR);
    if (data) total_size += *count;
    total_size = min( sizeof(buffer), total_size );

    status = NtEnumerateValueKey( KeyHandle, index, KeyValueFullInformation,
                                  buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (value || data)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
                return ERROR_NOT_ENOUGH_MEMORY;
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( KeyHandle, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (value)
        {
            if (info->NameLength/sizeof(WCHAR) >= *val_count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                goto overflow;
            }
            memcpy( value, info->Name, info->NameLength );
            *val_count = info->NameLength / sizeof(WCHAR);
            value[*val_count] = 0;
        }

        if (data)
        {
            if (total_size - info->DataOffset > *count)
            {
                status = STATUS_BUFFER_OVERFLOW;
                goto overflow;
            }
            memcpy( data, buf_ptr + info->DataOffset, total_size - info->DataOffset );
            if (total_size - info->DataOffset <= *count-sizeof(WCHAR) && is_string(info->Type))
            {
                /* if the type is REG_SZ and data is not 0-terminated
                 * and there is enough space in the buffer NT appends a \0 */
                WCHAR *ptr = (WCHAR *)(data + total_size - info->DataOffset);
                if (ptr > (WCHAR *)data && ptr[-1]) *ptr = 0;
            }
        }
    }
    else status = STATUS_SUCCESS;

 overflow:
    if (type) *type = info->Type;
    if (count) *count = info->DataLength;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    return RtlNtStatusToDosError(status);
}

/************************************************************************
 *  RegFlushKey
 *
 * @implemented
 */
LONG STDCALL
RegFlushKey(HKEY hKey)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_SUCCESS;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  Status = NtFlushKey (KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegGetKeySecurity
 *
 * @implemented
 */
LONG STDCALL
RegGetKeySecurity(HKEY hKey,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR pSecurityDescriptor,
		  LPDWORD lpcbSecurityDescriptor)
{
  HANDLE KeyHandle;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey(&KeyHandle,
			 hKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("MapDefaultKey() failed (Status %lx)\n", Status);
      return RtlNtStatusToDosError (Status);
    }

  Status = NtQuerySecurityObject(KeyHandle,
				 SecurityInformation,
				 pSecurityDescriptor,
				 *lpcbSecurityDescriptor,
				 lpcbSecurityDescriptor);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtQuerySecurityObject() failed (Status %lx)\n", Status);
      return RtlNtStatusToDosError (Status);
    }

  return ERROR_SUCCESS;
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
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFile,
				     &FileName,
				     NULL,
				     NULL))
    {
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
      return RtlNtStatusToDosError (Status);
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
 *  RegOpenCurrentUser
 *
 * @implemented
 */
LONG STDCALL
RegOpenCurrentUser (IN REGSAM samDesired,
                    OUT PHKEY phkResult)
{
  NTSTATUS Status;

  Status = RtlOpenCurrentUser((ACCESS_MASK)samDesired,
                              (PHANDLE)phkResult);
  if (!NT_SUCCESS(Status))
  {
    /* NOTE - don't set the last error code! just return the error! */
    return RtlNtStatusToDosError(Status);
  }

  return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOpenKeyA
 *
 *  20050503 Fireball - imported from WINE
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyA (HKEY hKey,
	     LPCSTR lpSubKey,
	     PHKEY phkResult)
{
	DPRINT("RegOpenKeyA hKey 0x%x lpSubKey %s phkResult %p\n", hKey, lpSubKey, phkResult);

	if (!lpSubKey || !*lpSubKey)
	{
		*phkResult = hKey;
		return ERROR_SUCCESS;
	}

	return RegOpenKeyExA( hKey, lpSubKey, 0, MAXIMUM_ALLOWED, phkResult);
}


/************************************************************************
 *  RegOpenKeyW
 *
 *  19981101 Ariadne
 *  19990525 EA
 *  20050503 Fireball - imported from WINE
 *
 * @implemented
 */
LONG STDCALL
RegOpenKeyW (HKEY hKey,
	     LPCWSTR lpSubKey,
	     PHKEY phkResult)
{
	DPRINT("RegOpenKeyW hKey 0x%x lpSubKey %S phkResult %p\n", hKey, lpSubKey, phkResult);

	if (!lpSubKey || !*lpSubKey)
	{
		*phkResult = hKey;
		return ERROR_SUCCESS;
	}
	return RegOpenKeyExW(hKey, lpSubKey, 0, MAXIMUM_ALLOWED, phkResult);
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
	HANDLE KeyHandle;
	NTSTATUS Status;

	DPRINT("RegOpenKeyExA hKey 0x%x lpSubKey %s ulOptions 0x%x samDesired 0x%x phkResult %p\n",
		hKey, lpSubKey, ulOptions, samDesired, phkResult);

	Status = MapDefaultKey (&KeyHandle, hKey);
	if (!NT_SUCCESS(Status))
	{
		return RtlNtStatusToDosError (Status);
	}

	RtlCreateUnicodeStringFromAsciiz (&SubKeyString, (LPSTR)lpSubKey);
	InitializeObjectAttributes (&ObjectAttributes,
		&SubKeyString,
		OBJ_CASE_INSENSITIVE,
		KeyHandle,
		NULL);

	Status = NtOpenKey ((PHANDLE)phkResult, samDesired, &ObjectAttributes);
	RtlFreeUnicodeString (&SubKeyString);
	if (!NT_SUCCESS(Status))
	{
		return RtlNtStatusToDosError (Status);
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
	HANDLE KeyHandle;
	NTSTATUS Status;

	DPRINT("RegOpenKeyExW hKey 0x%x lpSubKey %S ulOptions 0x%x samDesired 0x%x phkResult %p\n",
		hKey, lpSubKey, ulOptions, samDesired, phkResult);

	Status = MapDefaultKey (&KeyHandle, hKey);
	if (!NT_SUCCESS(Status))
	{
		return RtlNtStatusToDosError (Status);
	}

	if (lpSubKey != NULL)
		RtlInitUnicodeString (&SubKeyString, (LPWSTR)lpSubKey);
	else
		RtlInitUnicodeString (&SubKeyString, (LPWSTR)L"");

	InitializeObjectAttributes (&ObjectAttributes,
		&SubKeyString,
		OBJ_CASE_INSENSITIVE,
		KeyHandle,
		NULL);

	Status = NtOpenKey ((PHANDLE)phkResult,	samDesired,	&ObjectAttributes);

	if (!NT_SUCCESS(Status))
	{
		return RtlNtStatusToDosError (Status);
	}

	return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOpenUserClassesRoot
 *
 * @implemented
 */
LONG STDCALL
RegOpenUserClassesRoot (IN HANDLE hToken,
                        IN DWORD dwOptions,
                        IN REGSAM samDesired,
                        OUT PHKEY phkResult)
{
  const WCHAR UserClassesKeyPrefix[] = L"\\Registry\\User\\";
  const WCHAR UserClassesKeySuffix[] = L"_Classes";
  PTOKEN_USER TokenUserData;
  ULONG RequiredLength;
  UNICODE_STRING UserSidString, UserClassesKeyRoot;
  OBJECT_ATTRIBUTES ObjectAttributes;
  LONG ErrorCode;
  NTSTATUS Status;

  /* check parameters */
  if (hToken == NULL || dwOptions != 0 || phkResult == NULL)
  {
    return ERROR_INVALID_PARAMETER;
  }

  /*
   * Get the user sid from the token
   */

ReadTokenSid:
  /* determine how much memory we need */
  Status = NtQueryInformationToken(hToken,
                                   TokenUser,
                                   NULL,
                                   0,
                                   &RequiredLength);
  if (!NT_SUCCESS(Status) && (Status != STATUS_BUFFER_TOO_SMALL))
  {
    /* NOTE - as opposed to all other registry functions windows does indeed
              change the last error code in case the caller supplied a invalid
              handle for example! */
    ErrorCode = RtlNtStatusToDosError (Status);
    return ErrorCode;
  }

  TokenUserData = RtlAllocateHeap(ProcessHeap,
                                  0,
                                  RequiredLength);
  if (TokenUserData == NULL)
  {
    return ERROR_NOT_ENOUGH_MEMORY;
  }

  /* attempt to read the information */
  Status = NtQueryInformationToken(hToken,
                                   TokenUser,
                                   TokenUserData,
                                   RequiredLength,
                                   &RequiredLength);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeHeap(ProcessHeap,
                0,
                TokenUserData);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
      /* the information appears to have changed?! try again */
      goto ReadTokenSid;
    }

    /* NOTE - as opposed to all other registry functions windows does indeed
              change the last error code in case the caller supplied a invalid
              handle for example! */
    ErrorCode = RtlNtStatusToDosError (Status);
    return ErrorCode;
  }

  /*
   * Build the absolute path for the user's registry in the form
   * "\Registry\User\<SID>_Classes"
   */
  Status = RtlConvertSidToUnicodeString(&UserSidString,
                                        TokenUserData->User.Sid,
                                        TRUE);

  /* we don't need the user data anymore, free it */
  RtlFreeHeap(ProcessHeap,
              0,
              TokenUserData);

  if (!NT_SUCCESS(Status))
  {
    return RtlNtStatusToDosError (Status);
  }

  /* allocate enough memory for the entire key string */
  UserClassesKeyRoot.Length = 0;
  UserClassesKeyRoot.MaximumLength = UserSidString.Length +
                                     sizeof(UserClassesKeyPrefix) +
                                     sizeof(UserClassesKeySuffix);
  UserClassesKeyRoot.Buffer = RtlAllocateHeap(ProcessHeap,
                                              0,
                                              UserClassesKeyRoot.MaximumLength);
  if (UserClassesKeyRoot.Buffer == NULL)
  {
    RtlFreeUnicodeString(&UserSidString);
    return RtlNtStatusToDosError (Status);
  }

  /* build the string */
  RtlAppendUnicodeToString(&UserClassesKeyRoot,
                           UserClassesKeyPrefix);
  RtlAppendUnicodeStringToString(&UserClassesKeyRoot,
                                 &UserSidString);
  RtlAppendUnicodeToString(&UserClassesKeyRoot,
                           UserClassesKeySuffix);

  DPRINT("RegOpenUserClassesRoot: Absolute path: %wZ\n", &UserClassesKeyRoot);

  /*
   * Open the key
   */

  InitializeObjectAttributes (&ObjectAttributes,
			      &UserClassesKeyRoot,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

  Status = NtOpenKey((PHANDLE)phkResult,
                     samDesired,
                     &ObjectAttributes);

  RtlFreeUnicodeString(&UserSidString);
  RtlFreeUnicodeString(&UserClassesKeyRoot);

  if (!NT_SUCCESS(Status))
  {
    return RtlNtStatusToDosError (Status);
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
  ULONG ClassLength = 0;
  HANDLE KeyHandle;
  NTSTATUS Status;
  ULONG Length;
  LONG ErrorCode = ERROR_SUCCESS;

  if ((lpClass) && (!lpcbClass))
    {
      return ERROR_INVALID_PARAMETER;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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

      return RtlNtStatusToDosError (Status);
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
      Status = NtQuerySecurityObject(KeyHandle,
				     OWNER_SECURITY_INFORMATION |
				     GROUP_SECURITY_INFORMATION |
				     DACL_SECURITY_INFORMATION,
				     NULL,
				     0,
				     lpcbSecurityDescriptor);
      if (!NT_SUCCESS(Status))
	{
	  if (lpClass != NULL)
	    {
	      RtlFreeHeap(ProcessHeap,
			  0,
			  FullInfo);
	    }

	  return RtlNtStatusToDosError (Status);
	}
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

  return ErrorCode;
}


/************************************************************************
 *  RegQueryMultipleValuesA
 *
 * @implemented
 */
LONG STDCALL
RegQueryMultipleValuesA (HKEY hKey,
			 PVALENTA val_list,
			 DWORD num_vals,
			 LPSTR lpValueBuf,
			 LPDWORD ldwTotsize)
{
  ULONG i;
  DWORD maxBytes = *ldwTotsize;
  LPSTR bufptr = (LPSTR)lpValueBuf;
  LONG ErrorCode;

  if (maxBytes >= (1024*1024))
    return ERROR_TRANSFER_TOO_LONG;

  *ldwTotsize = 0;

  DPRINT ("RegQueryMultipleValuesA(%p,%p,%ld,%p,%p=%ld)\n",
	  hKey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

  for (i = 0; i < num_vals; i++)
    {
      val_list[i].ve_valuelen = 0;
      ErrorCode = RegQueryValueExA (hKey,
				    val_list[i].ve_valuename,
				    NULL,
				    NULL,
				    NULL,
				    &val_list[i].ve_valuelen);
      if (ErrorCode != ERROR_SUCCESS)
	{
	  return ErrorCode;
	}

      if (lpValueBuf != NULL && *ldwTotsize + val_list[i].ve_valuelen <= maxBytes)
	{
	  ErrorCode = RegQueryValueExA (hKey,
					val_list[i].ve_valuename,
					NULL,
					&val_list[i].ve_type,
					(LPBYTE)bufptr,
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

  for (i = 0; i < num_vals; i++)
    {
      val_list[i].ve_valuelen = 0;
      ErrorCode = RegQueryValueExW (hKey,
				    val_list[i].ve_valuename,
				    NULL,
				    NULL,
				    NULL,
				    &val_list[i].ve_valuelen);
      if (ErrorCode != ERROR_SUCCESS)
	{
	  return ErrorCode;
	}

      if (lpValueBuf != NULL && *ldwTotsize + val_list[i].ve_valuelen <= maxBytes)
	{
	  ErrorCode = RegQueryValueExW (hKey,
					val_list[i].ve_valuename,
					NULL,
					&val_list[i].ve_type,
					(LPBYTE)bufptr,
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
  ULONG BufferSize;
  ULONG ResultSize;
  HANDLE KeyHandle;
  LONG ErrorCode = ERROR_SUCCESS;
  ULONG MaxCopy = lpcbData != NULL && lpData != NULL ? *lpcbData : 0;

  DPRINT("hKey 0x%X  lpValueName %S  lpData 0x%X  lpcbData %d\n",
	 hKey, lpValueName, lpData, lpcbData ? *lpcbData : 0);

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  if (lpData != NULL && lpcbData == NULL)
    {
      return ERROR_INVALID_PARAMETER;
    }

  RtlInitUnicodeString (&ValueName,
			lpValueName);
  BufferSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) + MaxCopy;
  ValueInfo = RtlAllocateHeap (ProcessHeap,
			       0,
			       BufferSize);
  if (ValueInfo == NULL)
    {
      return ERROR_OUTOFMEMORY;
    }

  Status = NtQueryValueKey (KeyHandle,
			    &ValueName,
			    KeyValuePartialInformation,
			    ValueInfo,
			    BufferSize,
			    &ResultSize);
  DPRINT("Status 0x%X\n", Status);
  if (Status == STATUS_BUFFER_OVERFLOW)
    {
      /* Return ERROR_SUCCESS and the buffer space needed for a successful call */
      MaxCopy = 0;
      ErrorCode = lpData ? ERROR_MORE_DATA : ERROR_SUCCESS;
    }
  else if (!NT_SUCCESS(Status))
    {
      ErrorCode = RtlNtStatusToDosError (Status);
      MaxCopy = 0;
      if (lpcbData != NULL)
	{
	  ResultSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) + *lpcbData;
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
	  *lpcbData = (ResultSize - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]));
	  DPRINT("(string) Returning Size: %lu\n", *lpcbData);
	}
    }
  else
    {
      if (lpcbData != NULL)
	{
	  *lpcbData = ResultSize - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
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

  DPRINT("hKey 0x%X  lpValueName %s  lpData 0x%X  lpcbData %d\n",
	 hKey, lpValueName, lpData, lpcbData ? *lpcbData : 0);

  if (lpData != NULL && lpcbData == NULL)
    {
      return ERROR_INVALID_PARAMETER;
    }

  if (lpData)
    {
      ValueData.Length = 0;
      ValueData.MaximumLength = (*lpcbData + 1) * sizeof(WCHAR);
      ValueData.Buffer = RtlAllocateHeap (ProcessHeap,
					  0,
					  ValueData.MaximumLength);
      if (!ValueData.Buffer)
	{
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

  Length = (lpcbData == NULL) ? 0 : *lpcbData * sizeof(WCHAR);
  ErrorCode = RegQueryValueExW (hKey,
				ValueName.Buffer,
				lpReserved,
				&Type,
				(lpData == NULL) ? NULL : (LPBYTE)ValueData.Buffer,
				&Length);
  DPRINT("ErrorCode %lu\n", ErrorCode);
  RtlFreeUnicodeString(&ValueName);

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
	      AnsiString.Buffer = (LPSTR)lpData;
	      AnsiString.MaximumLength = *lpcbData;
	      ValueData.Length = Length;
	      ValueData.MaximumLength = ValueData.Length + sizeof(WCHAR);
	      RtlUnicodeStringToAnsiString(&AnsiString, &ValueData, FALSE);
	    }
	  Length = Length / sizeof(WCHAR);
	}
      else if (ErrorCode == ERROR_SUCCESS && ValueData.Buffer != NULL)
	{
          if (*lpcbData < Length)
            {
              ErrorCode = ERROR_MORE_DATA;
            }
          else
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

  DPRINT("hKey 0x%X lpSubKey %s lpValue %p lpcbValue %d\n",
	 hKey, lpSubKey, lpValue, lpcbValue ? *lpcbValue : 0);

  if (lpValue != NULL &&
      lpcbValue == NULL)
    {
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
  HANDLE KeyHandle;
  HANDLE RealKey;
  LONG ErrorCode;
  BOOL CloseRealKey;
  NTSTATUS Status;

  DPRINT("hKey 0x%X lpSubKey %S lpValue %p lpcbValue %d\n",
	 hKey, lpSubKey, lpValue, lpcbValue ? *lpcbValue : 0);

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
			  KEY_QUERY_VALUE,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  return RtlNtStatusToDosError (Status);
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
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
			  MAXIMUM_ALLOWED,
			  &KeyObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  return RtlNtStatusToDosError (Status);
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
      return RtlNtStatusToDosError (Status);
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
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpFile,
				     &FileName,
				     NULL,
				     NULL))
    {
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
      return RtlNtStatusToDosError (Status);
    }

  Status = NtRestoreKey (KeyHandle,
			 FileHandle,
			 (ULONG)dwFlags);
  NtClose (FileHandle);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  if (!RtlDosPathNameToNtPathName_U ((PWSTR)lpFile,
				     &FileName,
				     NULL,
				     NULL))
    {
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
      return RtlNtStatusToDosError (Status);
    }

  Status = NtSaveKey (KeyHandle,
		      FileHandle);
  NtClose (FileHandle);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  HANDLE KeyHandle;
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
    }

  Status = NtSetSecurityObject (KeyHandle,
				SecurityInformation,
				pSecurityDescriptor);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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

  if (((dwType == REG_SZ) ||
       (dwType == REG_MULTI_SZ) ||
       (dwType == REG_EXPAND_SZ)) &&
      (cbData != 0))
    {
      /* NT adds one if the caller forgot the NULL-termination character */
      if (lpData[cbData - 1] != '\0')
      {
         cbData++;
      }

      RtlInitAnsiString (&AnsiString,
			 NULL);
      AnsiString.Buffer = (PSTR)lpData;
      AnsiString.Length = cbData - 1;
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
  HANDLE KeyHandle;
  NTSTATUS Status;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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

  if (((dwType == REG_SZ) ||
       (dwType == REG_MULTI_SZ) ||
       (dwType == REG_EXPAND_SZ)) &&
      (cbData != 0) && (*(((PWCHAR)lpData) + (cbData / sizeof(WCHAR)) - 1) != L'\0'))
    {
      /* NT adds one if the caller forgot the NULL-termination character */
      cbData += sizeof(WCHAR);
    }

  Status = NtSetValueKey (KeyHandle,
			  pValueName,
			  0,
			  dwType,
			  (PVOID)lpData,
			  (ULONG)cbData);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
  LONG ret;
  HKEY hSubKey;

  if (dwType != REG_SZ)
  {
     return ERROR_INVALID_PARAMETER;
  }

  if (lpSubKey != NULL && lpSubKey[0] != '\0')
  {
     ret = RegCreateKeyA(hKey,
                         lpSubKey,
                         &hSubKey);

     if (ret != ERROR_SUCCESS)
     {
        return ret;
     }
  }
  else
     hSubKey = hKey;

  ret = RegSetValueExA(hSubKey,
                       NULL,
                       0,
                       REG_SZ,
                       (CONST BYTE*)lpData,
                       strlen(lpData) + 1);

  if (hSubKey != hKey)
  {
     RegCloseKey(hSubKey);
  }

  return ret;
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
  HANDLE KeyHandle;
  HANDLE RealKey;
  BOOL CloseRealKey;
  NTSTATUS Status;
  LONG ErrorCode;

  Status = MapDefaultKey (&KeyHandle,
			  hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
			  KEY_SET_VALUE,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  return RtlNtStatusToDosError (Status);
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
  NTSTATUS Status;

  if (hKey == HKEY_PERFORMANCE_DATA)
    {
      return ERROR_INVALID_HANDLE;
    }

  Status = MapDefaultKey (&KeyHandle, hKey);
  if (!NT_SUCCESS(Status))
    {
      return RtlNtStatusToDosError (Status);
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
      return RtlNtStatusToDosError (Status);
    }

  return ERROR_SUCCESS;
}

/* EOF */
