/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/reg/reg.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 *                  Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *                  Created 01/11/98
 *                  19990309 EA Stubs
 *                  20050502 Fireball imported some stuff from WINE
 */

/* INCLUDES *****************************************************************/

#include <advapi32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(reg);

/* DEFINES ******************************************************************/

#define MAX_DEFAULT_HANDLES   6
#define REG_MAX_NAME_SIZE     256
#define REG_MAX_DATA_SIZE     2048

/* GLOBALS ******************************************************************/

static RTL_CRITICAL_SECTION HandleTableCS;
static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];
static HANDLE ProcessHeap;
static BOOLEAN DefaultHandlesDisabled = FALSE;
static BOOLEAN DefaultHandleHKUDisabled = FALSE;
static BOOLEAN DllInitialized = FALSE; /* HACK */

/* PROTOTYPES ***************************************************************/

static NTSTATUS MapDefaultKey (PHANDLE ParentKey, HKEY Key);
static VOID CloseDefaultKeys(VOID);
#define ClosePredefKey(Handle)                                                 \
    if ((ULONG_PTR)Handle & 0x1) {                                             \
        NtClose(Handle);                                                       \
    }
#define IsPredefKey(HKey)                                                      \
    (((ULONG_PTR)(HKey) & 0xF0000000) == 0x80000000)
#define GetPredefKeyIndex(HKey)                                                \
    ((ULONG_PTR)(HKey) & 0x0FFFFFFF)

static NTSTATUS OpenClassesRootKey(PHANDLE KeyHandle);
static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);
static NTSTATUS OpenUsersKey (PHANDLE KeyHandle);
static NTSTATUS OpenCurrentConfigKey(PHANDLE KeyHandle);


/* FUNCTIONS ****************************************************************/
/* check if value type needs string conversion (Ansi<->Unicode) */
__inline static int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
}

/************************************************************************
 *  RegInitDefaultHandles
 */
BOOL
RegInitialize(VOID)
{
    TRACE("RegInitialize()\n");

    /* Lazy init hack */
    if (!DllInitialized)
    {
        ProcessHeap = RtlGetProcessHeap();
        RtlZeroMemory(DefaultHandleTable,
                      MAX_DEFAULT_HANDLES * sizeof(HANDLE));
        RtlInitializeCriticalSection(&HandleTableCS);

        DllInitialized = TRUE;
    }

    return TRUE;
}


/************************************************************************
 *  RegInit
 */
BOOL
RegCleanup(VOID)
{
    TRACE("RegCleanup()\n");

    CloseDefaultKeys();
    RtlDeleteCriticalSection(&HandleTableCS);

    return TRUE;
}


static NTSTATUS
OpenPredefinedKey(IN ULONG Index,
                  OUT HANDLE Handle)
{
    NTSTATUS Status;

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
            WARN("MapDefaultHandle() no handle creator\n");
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}


static NTSTATUS
MapDefaultKey(OUT PHANDLE RealKey,
              IN HKEY Key)
{
    PHANDLE Handle;
    ULONG Index;
    BOOLEAN DoOpen, DefDisabled;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("MapDefaultKey (Key %x)\n", Key);

    if (!IsPredefKey(Key))
    {
        *RealKey = (HANDLE)((ULONG_PTR)Key & ~0x1);
        return STATUS_SUCCESS;
    }

    /* Handle special cases here */
    Index = GetPredefKeyIndex(Key);
    if (Index >= MAX_DEFAULT_HANDLES)
    {
        return STATUS_INVALID_PARAMETER;
    }
    RegInitialize(); /* HACK until delay-loading is implemented */
    RtlEnterCriticalSection (&HandleTableCS);

    if (Key == HKEY_CURRENT_USER)
        DefDisabled = DefaultHandleHKUDisabled;
    else
        DefDisabled = DefaultHandlesDisabled;

    if (!DefDisabled)
    {
        Handle = &DefaultHandleTable[Index];
        DoOpen = (*Handle == NULL);
    }
    else
    {
        Handle = RealKey;
        DoOpen = TRUE;
    }

    if (DoOpen)
    {
        /* create/open the default handle */
        Status = OpenPredefinedKey(Index,
                                   Handle);
    }

    if (NT_SUCCESS(Status))
    {
        if (!DefDisabled)
            *RealKey = *Handle;
        else
            *(PULONG_PTR)Handle |= 0x1;
    }

    RtlLeaveCriticalSection (&HandleTableCS);

    return Status;
}


static VOID
CloseDefaultKeys(VOID)
{
    ULONG i;
    RegInitialize(); /* HACK until delay-loading is implemented */
    RtlEnterCriticalSection(&HandleTableCS);

    for (i = 0; i < MAX_DEFAULT_HANDLES; i++)
    {
        if (DefaultHandleTable[i] != NULL)
        {
            NtClose(DefaultHandleTable[i]);
            DefaultHandleTable[i] = NULL;
        }
    }

    RtlLeaveCriticalSection(&HandleTableCS);
}


static NTSTATUS
OpenClassesRootKey(PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\CLASSES");

    TRACE("OpenClassesRootKey()\n");

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    return NtOpenKey(KeyHandle,
                     MAXIMUM_ALLOWED,
                     &Attributes);
}


static NTSTATUS
OpenLocalMachineKey(PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine");
    NTSTATUS Status;

    TRACE("OpenLocalMachineKey()\n");

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(KeyHandle,
                       MAXIMUM_ALLOWED,
                       &Attributes);

    TRACE("NtOpenKey(%wZ) => %08x\n", &KeyName, Status);

    return Status;
}


static NTSTATUS
OpenUsersKey(PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\User");

    TRACE("OpenUsersKey()\n");

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    return NtOpenKey(KeyHandle,
                     MAXIMUM_ALLOWED,
                     &Attributes);
}


static NTSTATUS
OpenCurrentConfigKey (PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES Attributes;
    UNICODE_STRING KeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current");

    TRACE("OpenCurrentConfigKey()\n");

    InitializeObjectAttributes(&Attributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    return NtOpenKey(KeyHandle,
                     MAXIMUM_ALLOWED,
                     &Attributes);
}


/************************************************************************
 *  RegDisablePredefinedCache
 *
 * @implemented
 */
LONG WINAPI
RegDisablePredefinedCache(VOID)
{
    RegInitialize(); /* HACK until delay-loading is implemented */
    RtlEnterCriticalSection(&HandleTableCS);
    DefaultHandleHKUDisabled = TRUE;
    RtlLeaveCriticalSection(&HandleTableCS);
    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDisablePredefinedCacheEx
 *
 * @implemented
 */
LONG WINAPI
RegDisablePredefinedCacheEx(VOID)
{
    RegInitialize(); /* HACK until delay-loading is implemented */
    RtlEnterCriticalSection(&HandleTableCS);
    DefaultHandlesDisabled = TRUE;
    DefaultHandleHKUDisabled = TRUE;
    RtlLeaveCriticalSection(&HandleTableCS);
    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegOverridePredefKey
 *
 * @implemented
 */
LONG WINAPI
RegOverridePredefKey(IN HKEY hKey,
                     IN HKEY hNewHKey  OPTIONAL)
{
    LONG ErrorCode = ERROR_SUCCESS;

    if ((hKey == HKEY_CLASSES_ROOT ||
         hKey == HKEY_CURRENT_CONFIG ||
         hKey == HKEY_CURRENT_USER ||
         hKey == HKEY_LOCAL_MACHINE ||
         hKey == HKEY_PERFORMANCE_DATA ||
         hKey == HKEY_USERS) &&
        !IsPredefKey(hNewHKey))
    {
        PHANDLE Handle;
        ULONG Index;

        Index = GetPredefKeyIndex(hKey);
        Handle = &DefaultHandleTable[Index];

        if (hNewHKey == NULL)
        {
            /* restore the default mapping */
            NTSTATUS Status = OpenPredefinedKey(Index,
                                                &hNewHKey);
            if (!NT_SUCCESS(Status))
            {
                return RtlNtStatusToDosError(Status);
            }

            ASSERT(hNewHKey != NULL);
        }
        RegInitialize(); /* HACK until delay-loading is implemented */
        RtlEnterCriticalSection(&HandleTableCS);

        /* close the currently mapped handle if existing */
        if (*Handle != NULL)
        {
            NtClose(*Handle);
        }

        /* update the mapping */
        *Handle = hNewHKey;

        RtlLeaveCriticalSection(&HandleTableCS);
    }
    else
        ErrorCode = ERROR_INVALID_HANDLE;

    return ErrorCode;
}


/************************************************************************
 *  RegCloseKey
 *
 * @implemented
 */
LONG WINAPI
RegCloseKey(HKEY hKey)
{
    NTSTATUS Status;

    /* don't close null handle or a pseudo handle */
    if ((!hKey) || (((ULONG_PTR)hKey & 0xF0000000) == 0x80000000))
    {
        return ERROR_INVALID_HANDLE;
    }

    Status = NtClose(hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

  return ERROR_SUCCESS;
}


static NTSTATUS
RegpCopyTree(IN HKEY hKeySrc,
             IN HKEY hKeyDest)
{
    typedef struct
    {
        LIST_ENTRY ListEntry;
        HANDLE hKeySrc;
        HANDLE hKeyDest;
    } REGP_COPY_KEYS, *PREGP_COPY_KEYS;

    LIST_ENTRY copyQueueHead;
    PREGP_COPY_KEYS copyKeys, newCopyKeys;
    union
    {
        KEY_VALUE_FULL_INFORMATION *KeyValue;
        KEY_NODE_INFORMATION *KeyNode;
        PVOID Buffer;
    } Info;
    ULONG Index, BufferSizeRequired, BufferSize = 0x200;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS Status2 = STATUS_SUCCESS;

    InitializeListHead(&copyQueueHead);

    Info.Buffer = RtlAllocateHeap(ProcessHeap,
                                  0,
                                  BufferSize);
    if (Info.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    copyKeys = RtlAllocateHeap(ProcessHeap,
                               0,
                               sizeof(REGP_COPY_KEYS));
    if (copyKeys != NULL)
    {
        copyKeys->hKeySrc = hKeySrc;
        copyKeys->hKeyDest = hKeyDest;
        InsertHeadList(&copyQueueHead,
                       &copyKeys->ListEntry);

        /* FIXME - copy security from hKeySrc to hKeyDest or just for the subkeys? */

        do
        {
            copyKeys = CONTAINING_RECORD(copyQueueHead.Flink,
                                         REGP_COPY_KEYS,
                                         ListEntry);

            /* enumerate all values and copy them */
            Index = 0;
            for (;;)
            {
                Status2 = NtEnumerateValueKey(copyKeys->hKeySrc,
                                              Index,
                                              KeyValueFullInformation,
                                              Info.KeyValue,
                                              BufferSize,
                                              &BufferSizeRequired);
                if (NT_SUCCESS(Status2))
                {
                    UNICODE_STRING ValueName;
                    PVOID Data;

                    /* don't use RtlInitUnicodeString as the string is not NULL-terminated! */
                    ValueName.Length = Info.KeyValue->NameLength;
                    ValueName.MaximumLength = ValueName.Length;
                    ValueName.Buffer = Info.KeyValue->Name;

                    Data = (PVOID)((ULONG_PTR)Info.KeyValue + Info.KeyValue->DataOffset);

                    Status2 = NtSetValueKey(copyKeys->hKeyDest,
                                            &ValueName,
                                            Info.KeyValue->TitleIndex,
                                            Info.KeyValue->Type,
                                            Data,
                                            Info.KeyValue->DataLength);

                    /* don't break, let's try to copy as many values as possible */
                    if (!NT_SUCCESS(Status2) && NT_SUCCESS(Status))
                    {
                        Status = Status2;
                    }

                    Index++;
                }
                else if (Status2 == STATUS_BUFFER_OVERFLOW)
                {
                    PVOID Buffer;

                    ASSERT(BufferSize < BufferSizeRequired);

                    Buffer = RtlReAllocateHeap(ProcessHeap,
                                               0,
                                               Info.Buffer,
                                               BufferSizeRequired);
                    if (Buffer != NULL)
                    {
                        Info.Buffer = Buffer;
                        /* try again */
                    }
                    else
                    {
                        /* don't break, let's try to copy as many values as possible */
                        Status2 = STATUS_INSUFFICIENT_RESOURCES;
                        Index++;

                        if (NT_SUCCESS(Status))
                        {
                            Status = Status2;
                        }
                    }
                }
                else
                {
                    /* break to avoid an infinite loop in case of denied access or
                       other errors! */
                    if (Status2 != STATUS_NO_MORE_ENTRIES && NT_SUCCESS(Status))
                    {
                        Status = Status2;
                    }

                    break;
                }
            }

            /* enumerate all subkeys and open and enqueue them */
            Index = 0;
            for (;;)
            {
                Status2 = NtEnumerateKey(copyKeys->hKeySrc,
                                         Index,
                                         KeyNodeInformation,
                                         Info.KeyNode,
                                         BufferSize,
                                         &BufferSizeRequired);
                if (NT_SUCCESS(Status2))
                {
                    HANDLE KeyHandle, NewKeyHandle;
                    OBJECT_ATTRIBUTES ObjectAttributes;
                    UNICODE_STRING SubKeyName, ClassName;

                    /* don't use RtlInitUnicodeString as the string is not NULL-terminated! */
                    SubKeyName.Length = Info.KeyNode->NameLength;
                    SubKeyName.MaximumLength = SubKeyName.Length;
                    SubKeyName.Buffer = Info.KeyNode->Name;
                    ClassName.Length = Info.KeyNode->ClassLength;
                    ClassName.MaximumLength = ClassName.Length;
                    ClassName.Buffer = (PWSTR)((ULONG_PTR)Info.KeyNode + Info.KeyNode->ClassOffset);

                    /* open the subkey with sufficient rights */

                    InitializeObjectAttributes(&ObjectAttributes,
                                               &SubKeyName,
                                               OBJ_CASE_INSENSITIVE,
                                               copyKeys->hKeySrc,
                                               NULL);

                    Status2 = NtOpenKey(&KeyHandle,
                                        KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                                        &ObjectAttributes);
                    if (NT_SUCCESS(Status2))
                    {
                        /* FIXME - attempt to query the security information */

                        InitializeObjectAttributes(&ObjectAttributes,
                                               &SubKeyName,
                                               OBJ_CASE_INSENSITIVE,
                                               copyKeys->hKeyDest,
                                               NULL);

                        Status2 = NtCreateKey(&NewKeyHandle,
                                              KEY_ALL_ACCESS,
                                              &ObjectAttributes,
                                              Info.KeyNode->TitleIndex,
                                              &ClassName,
                                              0,
                                              NULL);
                        if (NT_SUCCESS(Status2))
                        {
                            newCopyKeys = RtlAllocateHeap(ProcessHeap,
                                                          0,
                                                          sizeof(REGP_COPY_KEYS));
                            if (newCopyKeys != NULL)
                            {
                                /* save the handles and enqueue the subkey */
                                newCopyKeys->hKeySrc = KeyHandle;
                                newCopyKeys->hKeyDest = NewKeyHandle;
                                InsertTailList(&copyQueueHead,
                                               &newCopyKeys->ListEntry);
                            }
                            else
                            {
                                NtClose(KeyHandle);
                                NtClose(NewKeyHandle);

                                Status2 = STATUS_INSUFFICIENT_RESOURCES;
                            }
                        }
                        else
                        {
                            NtClose(KeyHandle);
                        }
                    }

                    if (!NT_SUCCESS(Status2) && NT_SUCCESS(Status))
                    {
                        Status = Status2;
                    }

                    Index++;
                }
                else if (Status2 == STATUS_BUFFER_OVERFLOW)
                {
                    PVOID Buffer;

                    ASSERT(BufferSize < BufferSizeRequired);

                    Buffer = RtlReAllocateHeap(ProcessHeap,
                                               0,
                                               Info.Buffer,
                                               BufferSizeRequired);
                    if (Buffer != NULL)
                    {
                        Info.Buffer = Buffer;
                        /* try again */
                    }
                    else
                    {
                        /* don't break, let's try to copy as many keys as possible */
                        Status2 = STATUS_INSUFFICIENT_RESOURCES;
                        Index++;

                        if (NT_SUCCESS(Status))
                        {
                            Status = Status2;
                        }
                    }
                }
                else
                {
                    /* break to avoid an infinite loop in case of denied access or
                       other errors! */
                    if (Status2 != STATUS_NO_MORE_ENTRIES && NT_SUCCESS(Status))
                    {
                        Status = Status2;
                    }

                    break;
                }
            }

            /* close the handles and remove the entry from the list */
            if (copyKeys->hKeySrc != hKeySrc)
            {
                NtClose(copyKeys->hKeySrc);
            }
            if (copyKeys->hKeyDest != hKeyDest)
            {
                NtClose(copyKeys->hKeyDest);
            }

            RemoveEntryList(&copyKeys->ListEntry);

            RtlFreeHeap(ProcessHeap,
                        0,
                        copyKeys);
        } while (!IsListEmpty(&copyQueueHead));
    }
    else
        Status = STATUS_INSUFFICIENT_RESOURCES;

    RtlFreeHeap(ProcessHeap,
                0,
                Info.Buffer);

    return Status;
}


/************************************************************************
 *  RegCopyTreeW
 *
 * @implemented
 */
LONG WINAPI
RegCopyTreeW(IN HKEY hKeySrc,
             IN LPCWSTR lpSubKey  OPTIONAL,
             IN HKEY hKeyDest)
{
    HANDLE DestKeyHandle, KeyHandle, CurKey, SubKeyHandle = NULL;
    NTSTATUS Status;

    Status = MapDefaultKey(&KeyHandle,
                           hKeySrc);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    Status = MapDefaultKey(&DestKeyHandle,
                           hKeyDest);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup2;
    }

    if (lpSubKey != NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING SubKeyName;

        RtlInitUnicodeString(&SubKeyName,
                             (LPWSTR)lpSubKey);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle,
                           KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        CurKey = SubKeyHandle;
    }
    else
        CurKey = KeyHandle;

    Status = RegpCopyTree(CurKey,
                          hKeyDest);

    if (SubKeyHandle != NULL)
    {
        NtClose(SubKeyHandle);
    }

Cleanup:
    ClosePredefKey(DestKeyHandle);
Cleanup2:
    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegCopyTreeA
 *
 * @implemented
 */
LONG WINAPI
RegCopyTreeA(IN HKEY hKeySrc,
             IN LPCSTR lpSubKey  OPTIONAL,
             IN HKEY hKeyDest)
{
    UNICODE_STRING SubKeyName = { 0, 0, NULL };
    LONG Ret;

    if (lpSubKey != NULL &&
        !RtlCreateUnicodeStringFromAsciiz(&SubKeyName,
                                          (LPSTR)lpSubKey))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = RegCopyTreeW(hKeySrc,
                       SubKeyName.Buffer,
                       hKeyDest);

    RtlFreeUnicodeString(&SubKeyName);

    return Ret;
}


/************************************************************************
 *  RegConnectRegistryA
 *
 * @implemented
 */
LONG WINAPI
RegConnectRegistryA(IN LPCSTR lpMachineName,
                    IN HKEY hKey,
                    OUT PHKEY phkResult)
{
    UNICODE_STRING MachineName = { 0, 0, NULL };
    LONG Ret;

    if (lpMachineName != NULL &&
        !RtlCreateUnicodeStringFromAsciiz(&MachineName,
                                          (LPSTR)lpMachineName))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = RegConnectRegistryW(MachineName.Buffer,
                              hKey,
                              phkResult);

    RtlFreeUnicodeString(&MachineName);

    return Ret;
}


/************************************************************************
 *  RegConnectRegistryW
 *
 * @unimplemented
 */
LONG WINAPI
RegConnectRegistryW(LPCWSTR lpMachineName,
                    HKEY hKey,
                    PHKEY phkResult)
{
    LONG ret;

    TRACE("(%s,%p,%p): stub\n",debugstr_w(lpMachineName),hKey,phkResult);

    if (!lpMachineName || !*lpMachineName)
    {
        /* Use the local machine name */
        ret = RegOpenKeyW( hKey, NULL, phkResult );
    }
    else
    {
        WCHAR compName[MAX_COMPUTERNAME_LENGTH + 1];
        DWORD len = sizeof(compName) / sizeof(WCHAR);

        /* MSDN says lpMachineName must start with \\ : not so */
        if( lpMachineName[0] == '\\' &&  lpMachineName[1] == '\\')
            lpMachineName += 2;

        if (GetComputerNameW(compName, &len))
        {
            if (!_wcsicmp(lpMachineName, compName))
                ret = RegOpenKeyW(hKey, NULL, phkResult);
            else
            {
                FIXME("Connect to %s is not supported.\n",debugstr_w(lpMachineName));
                ret = ERROR_BAD_NETPATH;
            }
        }
        else
            ret = GetLastError();
    }

    return ret;
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
    TRACE("NtCreateKey(%wZ) called (Status %lx)\n", ObjectAttributes->ObjectName, Status);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        return Status;

    /* Copy object attributes */
    RtlCopyMemory(&LocalObjectAttributes,
                  ObjectAttributes,
                  sizeof(OBJECT_ATTRIBUTES));
    RtlCreateUnicodeString(&LocalKeyName,
                           ObjectAttributes->ObjectName->Buffer);
    LocalObjectAttributes.ObjectName = &LocalKeyName;
    FullNameLength = LocalKeyName.Length / sizeof(WCHAR);

  LocalKeyHandle = NULL;

    /* Remove the last part of the key name and try to create the key again. */
    while (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Ptr = wcsrchr(LocalKeyName.Buffer, '\\');
        if (Ptr == NULL || Ptr == LocalKeyName.Buffer)
        {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        *Ptr = (WCHAR)0;
        LocalKeyName.Length = wcslen(LocalKeyName.Buffer) * sizeof(WCHAR);

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_CREATE_SUB_KEY,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        TRACE("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
    }

    if (!NT_SUCCESS(Status))
    {
        RtlFreeUnicodeString(&LocalKeyName);
        return Status;
    }

    /* Add removed parts of the key name and create them too. */
    Length = wcslen(LocalKeyName.Buffer);
    while (TRUE)
    {
        if (LocalKeyHandle)
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

        Status = NtCreateKey(&LocalKeyHandle,
                             KEY_CREATE_SUB_KEY,
                             &LocalObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
        TRACE("NtCreateKey(%wZ) called (Status %lx)\n", &LocalKeyName, Status);
        if (!NT_SUCCESS(Status))
            break;
    }

    RtlFreeUnicodeString(&LocalKeyName);

    return Status;
}


/************************************************************************
 *  RegCreateKeyExA
 *
 * @implemented
 */
LONG WINAPI
RegCreateKeyExA(HKEY hKey,
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

    TRACE("RegCreateKeyExA() called\n");

    if (lpSecurityAttributes && lpSecurityAttributes->nLength != sizeof(SECURITY_ATTRIBUTES))
        return ERROR_INVALID_USER_BUFFER;

    /* get the real parent key */
    Status = MapDefaultKey(&ParentKey,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    TRACE("ParentKey %p\n", ParentKey);

    if (lpClass != NULL)
    {
        RtlCreateUnicodeStringFromAsciiz(&ClassString,
                                         lpClass);
    }

    RtlCreateUnicodeStringFromAsciiz(&SubKeyString,
                                     (LPSTR)lpSubKey);
    InitializeObjectAttributes(&Attributes,
                               &SubKeyString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE)ParentKey,
                               lpSecurityAttributes ? (PSECURITY_DESCRIPTOR)lpSecurityAttributes->lpSecurityDescriptor : NULL);
    Status = CreateNestedKey(phkResult,
                             &Attributes,
                             (lpClass == NULL)? NULL : &ClassString,
                             dwOptions,
                             samDesired,
                             lpdwDisposition);
    RtlFreeUnicodeString(&SubKeyString);
    if (lpClass != NULL)
    {
        RtlFreeUnicodeString(&ClassString);
    }

    ClosePredefKey(ParentKey);

    TRACE("Status %x\n", Status);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegCreateKeyExW
 *
 * @implemented
 */
LONG WINAPI
RegCreateKeyExW(HKEY hKey,
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

    TRACE("RegCreateKeyExW() called\n");

    if (lpSecurityAttributes && lpSecurityAttributes->nLength != sizeof(SECURITY_ATTRIBUTES))
        return ERROR_INVALID_USER_BUFFER;

    /* get the real parent key */
    Status = MapDefaultKey(&ParentKey,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    TRACE("ParentKey %p\n", ParentKey);

    RtlInitUnicodeString(&ClassString,
                         lpClass);
    RtlInitUnicodeString(&SubKeyString,
                         lpSubKey);
    InitializeObjectAttributes(&Attributes,
                               &SubKeyString,
                               OBJ_CASE_INSENSITIVE,
                               (HANDLE)ParentKey,
                               lpSecurityAttributes ? (PSECURITY_DESCRIPTOR)lpSecurityAttributes->lpSecurityDescriptor : NULL);
    Status = CreateNestedKey(phkResult,
                             &Attributes,
                             (lpClass == NULL)? NULL : &ClassString,
                             dwOptions,
                             samDesired,
                             lpdwDisposition);

    ClosePredefKey(ParentKey);

    TRACE("Status %x\n", Status);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegCreateKeyA
 *
 * @implemented
 */
LONG WINAPI
RegCreateKeyA(HKEY hKey,
              LPCSTR lpSubKey,
              PHKEY phkResult)
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
 *  RegCreateKeyW
 *
 * @implemented
 */
LONG WINAPI
RegCreateKeyW(HKEY hKey,
              LPCWSTR lpSubKey,
              PHKEY phkResult)
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
 *  RegDeleteKeyA
 *
 * @implemented
 */
LONG WINAPI
RegDeleteKeyA(HKEY hKey,
              LPCSTR lpSubKey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyName;
    HANDLE ParentKey;
    HANDLE TargetKey;
    NTSTATUS Status;

    /* Make sure we got a subkey */
    if (!lpSubKey)
    {
        /* Fail */
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&ParentKey,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlCreateUnicodeStringFromAsciiz(&SubKeyName,
                                     (LPSTR)lpSubKey);
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKey,
                               NULL);

    Status = NtOpenKey(&TargetKey,
                       DELETE,
                       &ObjectAttributes);
    RtlFreeUnicodeString(&SubKeyName);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtDeleteKey(TargetKey);
    NtClose (TargetKey);

Cleanup:
    ClosePredefKey(ParentKey);

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
LONG WINAPI
RegDeleteKeyW(HKEY hKey,
              LPCWSTR lpSubKey)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyName;
    HANDLE ParentKey;
    HANDLE TargetKey;
    NTSTATUS Status;

    /* Make sure we got a subkey */
    if (!lpSubKey)
    {
        /* Fail */
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&ParentKey,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlInitUnicodeString(&SubKeyName,
                         (LPWSTR)lpSubKey);
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKey,
                               NULL);
    Status = NtOpenKey(&TargetKey,
                       DELETE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtDeleteKey(TargetKey);
    NtClose(TargetKey);

Cleanup:
    ClosePredefKey(ParentKey);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteKeyExA
 *
 * @unimplemented
 */
LONG
WINAPI
RegDeleteKeyExA(HKEY hKey,
                LPCSTR lpSubKey,
                REGSAM samDesired,
                DWORD Reserved)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegDeleteKeyExW
 *
 * @unimplemented
 */
LONG
WINAPI
RegDeleteKeyExW(HKEY hKey,
                LPCWSTR lpSubKey,
                REGSAM samDesired,
                DWORD Reserved)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegDeleteKeyValueW
 *
 * @implemented
 */
LONG WINAPI
RegDeleteKeyValueW(IN HKEY hKey,
                   IN LPCWSTR lpSubKey  OPTIONAL,
                   IN LPCWSTR lpValueName  OPTIONAL)
{
    UNICODE_STRING ValueName;
    HANDLE KeyHandle, CurKey, SubKeyHandle = NULL;
    NTSTATUS Status;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpSubKey != NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING SubKeyName;

        RtlInitUnicodeString(&SubKeyName,
                             (LPWSTR)lpSubKey);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle,
                           KEY_SET_VALUE,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        CurKey = SubKeyHandle;
    }
    else
        CurKey = KeyHandle;

    RtlInitUnicodeString(&ValueName,
                         (LPWSTR)lpValueName);

    Status = NtDeleteValueKey(CurKey,
                              &ValueName);

    if (SubKeyHandle != NULL)
    {
        NtClose(SubKeyHandle);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteKeyValueA
 *
 * @implemented
 */
LONG WINAPI
RegDeleteKeyValueA(IN HKEY hKey,
                   IN LPCSTR lpSubKey  OPTIONAL,
                   IN LPCSTR lpValueName  OPTIONAL)
{
    UNICODE_STRING SubKey = { 0, 0, NULL }, ValueName = { 0, 0, NULL };
    LONG Ret;

    if (lpSubKey != NULL &&
        !RtlCreateUnicodeStringFromAsciiz(&SubKey,
                                          (LPSTR)lpSubKey))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (lpValueName != NULL &&
        !RtlCreateUnicodeStringFromAsciiz(&ValueName,
                                          (LPSTR)lpValueName))
    {
        RtlFreeUnicodeString(&SubKey);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = RegDeleteKeyValueW(hKey,
                             SubKey.Buffer,
                             SubKey.Buffer);

    RtlFreeUnicodeString(&SubKey);
    RtlFreeUnicodeString(&ValueName);

    return Ret;
}

#if 0
// Non-recursive RegDeleteTreeW implementation by Thomas, however it needs bugfixing
static NTSTATUS
RegpDeleteTree(IN HKEY hKey)
{
    typedef struct
    {
        LIST_ENTRY ListEntry;
        HANDLE KeyHandle;
    } REGP_DEL_KEYS, *PREG_DEL_KEYS;

    LIST_ENTRY delQueueHead;
    PREG_DEL_KEYS delKeys, newDelKeys;
    HANDLE ProcessHeap;
    ULONG BufferSize;
    PKEY_BASIC_INFORMATION BasicInfo;
    PREG_DEL_KEYS KeyDelRoot;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS Status2 = STATUS_SUCCESS;

    InitializeListHead(&delQueueHead);

    ProcessHeap = RtlGetProcessHeap();

    /* NOTE: no need to allocate enough memory for an additional KEY_BASIC_INFORMATION
             structure for the root key, we only do that for subkeys as we need to
             allocate REGP_DEL_KEYS structures anyway! */
    KeyDelRoot = RtlAllocateHeap(ProcessHeap,
                                 0,
                                 sizeof(REGP_DEL_KEYS));
    if (KeyDelRoot != NULL)
    {
        KeyDelRoot->KeyHandle = hKey;
        InsertTailList(&delQueueHead,
                       &KeyDelRoot->ListEntry);

        do
        {
            delKeys = CONTAINING_RECORD(delQueueHead.Flink,
                                        REGP_DEL_KEYS,
                                        ListEntry);

            BufferSize = 0;
            BasicInfo = NULL;
            newDelKeys = NULL;

ReadFirstSubKey:
            /* check if this key contains subkeys and delete them first by queuing
               them at the head of the list */
            Status2 = NtEnumerateKey(delKeys->KeyHandle,
                                     0,
                                     KeyBasicInformation,
                                     BasicInfo,
                                     BufferSize,
                                     &BufferSize);

            if (NT_SUCCESS(Status2))
            {
                OBJECT_ATTRIBUTES ObjectAttributes;
                UNICODE_STRING SubKeyName;

                ASSERT(newDelKeys != NULL);
                ASSERT(BasicInfo != NULL);

                /* don't use RtlInitUnicodeString as the string is not NULL-terminated! */
                SubKeyName.Length = BasicInfo->NameLength;
                SubKeyName.MaximumLength = BasicInfo->NameLength;
                SubKeyName.Buffer = BasicInfo->Name;

                InitializeObjectAttributes(&ObjectAttributes,
                                           &SubKeyName,
                                           OBJ_CASE_INSENSITIVE,
                                           delKeys->KeyHandle,
                                           NULL);

                /* open the subkey */
                Status2 = NtOpenKey(&newDelKeys->KeyHandle,
                                    DELETE | KEY_ENUMERATE_SUB_KEYS,
                                    &ObjectAttributes);
                if (!NT_SUCCESS(Status2))
                {
                    goto SubKeyFailure;
                }

                /* enqueue this key to the head of the deletion queue */
                InsertHeadList(&delQueueHead,
                               &newDelKeys->ListEntry);

                /* try again from the head of the list */
                continue;
            }
            else
            {
                if (Status2 == STATUS_BUFFER_TOO_SMALL)
                {
                    newDelKeys = RtlAllocateHeap(ProcessHeap,
                                                 0,
                                                 BufferSize + sizeof(REGP_DEL_KEYS));
                    if (newDelKeys != NULL)
                    {
                        BasicInfo = (PKEY_BASIC_INFORMATION)(newDelKeys + 1);

                        /* try again */
                        goto ReadFirstSubKey;
                    }
                    else
                    {
                        /* don't break, let's try to delete as many keys as possible */
                        Status2 = STATUS_INSUFFICIENT_RESOURCES;
                        goto SubKeyFailureNoFree;
                    }
                }
                else if (Status2 == STATUS_BUFFER_OVERFLOW)
                {
                    PREG_DEL_KEYS newDelKeys2;

                    ASSERT(newDelKeys != NULL);

                    /* we need more memory to query the key name */
                    newDelKeys2 = RtlReAllocateHeap(ProcessHeap,
                                                    0,
                                                    newDelKeys,
                                                    BufferSize + sizeof(REGP_DEL_KEYS));
                    if (newDelKeys2 != NULL)
                    {
                        newDelKeys = newDelKeys2;
                        BasicInfo = (PKEY_BASIC_INFORMATION)(newDelKeys + 1);

                        /* try again */
                        goto ReadFirstSubKey;
                    }
                    else
                    {
                        /* don't break, let's try to delete as many keys as possible */
                        Status2 = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else if (Status2 == STATUS_NO_MORE_ENTRIES)
                {
                    /* in some race conditions where another thread would delete
                       the same tree at the same time, newDelKeys could actually
                       be != NULL! */
                    if (newDelKeys != NULL)
                    {
                        RtlFreeHeap(ProcessHeap,
                                    0,
                                    newDelKeys);
                    }
                    break;
                }

SubKeyFailure:
                /* newDelKeys can be NULL here when NtEnumerateKey returned an
                   error other than STATUS_BUFFER_TOO_SMALL or STATUS_BUFFER_OVERFLOW! */
                if (newDelKeys != NULL)
                {
                    RtlFreeHeap(ProcessHeap,
                                0,
                                newDelKeys);
                }

SubKeyFailureNoFree:
                /* don't break, let's try to delete as many keys as possible */
                if (NT_SUCCESS(Status))
                {
                    Status = Status2;
                }
            }

            Status2 = NtDeleteKey(delKeys->KeyHandle);

            /* NOTE: do NOT close the handle anymore, it's invalid already! */

            if (!NT_SUCCESS(Status2))
            {
                /* close the key handle so we don't leak handles for keys we were
                   unable to delete. But only do this for handles not supplied
                   by the caller! */

                if (delKeys->KeyHandle != hKey)
                {
                    NtClose(delKeys->KeyHandle);
                }

                if (NT_SUCCESS(Status))
                {
                    /* don't break, let's try to delete as many keys as possible */
                    Status = Status2;
                }
            }

            /* remove the entry from the list */
            RemoveEntryList(&delKeys->ListEntry);

            RtlFreeHeap(ProcessHeap,
                        0,
                        delKeys);
        } while (!IsListEmpty(&delQueueHead));
    }
    else
        Status = STATUS_INSUFFICIENT_RESOURCES;

    return Status;
}


/************************************************************************
 *  RegDeleteTreeW
 *
 * @implemented
 */
LONG WINAPI
RegDeleteTreeW(IN HKEY hKey,
               IN LPCWSTR lpSubKey  OPTIONAL)
{
    HANDLE KeyHandle, CurKey, SubKeyHandle = NULL;
    NTSTATUS Status;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpSubKey != NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING SubKeyName;

        RtlInitUnicodeString(&SubKeyName,
                             (LPWSTR)lpSubKey);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle,
                           DELETE | KEY_ENUMERATE_SUB_KEYS,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        CurKey = SubKeyHandle;
    }
    else
        CurKey = KeyHandle;

    Status = RegpDeleteTree(CurKey);

    if (NT_SUCCESS(Status))
    {
        /* make sure we only close hKey (KeyHandle) when the caller specified a
           subkey, because the handle would be invalid already! */
        if (CurKey != KeyHandle)
        {
            ClosePredefKey(KeyHandle);
        }

        return ERROR_SUCCESS;
    }
    else
    {
        /* make sure we close all handles we created! */
        if (SubKeyHandle != NULL)
        {
            NtClose(SubKeyHandle);
        }

Cleanup:
        ClosePredefKey(KeyHandle);

        return RtlNtStatusToDosError(Status);
    }
}
#endif


/************************************************************************
 *  RegDeleteTreeW
 *
 * @implemented
 */
LSTATUS
WINAPI
RegDeleteTreeW(HKEY hKey,
               LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    NTSTATUS Status;
    HANDLE KeyHandle;
    HKEY hSubKey;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;

    TRACE("(hkey=%p,%p %s)\n", hKey, lpszSubKey, debugstr_w(lpszSubKey));

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    hSubKey = KeyHandle;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(KeyHandle, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret)
        {
            ClosePredefKey(KeyHandle);
            return ret;
        }
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = RtlAllocateHeap( RtlGetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = RegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(KeyHandle, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(KeyHandle, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(KeyHandle, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        RtlFreeHeap( RtlGetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);

    ClosePredefKey(KeyHandle);

    return ret;
}


/************************************************************************
 *  RegDeleteTreeA
 *
 * @implemented
 */
LONG WINAPI
RegDeleteTreeA(IN HKEY hKey,
               IN LPCSTR lpSubKey  OPTIONAL)
{
    UNICODE_STRING SubKeyName = { 0, 0, NULL };
    LONG Ret;

    if (lpSubKey != NULL &&
        !RtlCreateUnicodeStringFromAsciiz(&SubKeyName,
                                          (LPSTR)lpSubKey))
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Ret = RegDeleteTreeW(hKey,
                         SubKeyName.Buffer);

    RtlFreeUnicodeString(&SubKeyName);

    return Ret;
}


/************************************************************************
 *  RegDisableReflectionKey
 *
 * @unimplemented
 */
LONG WINAPI
RegDisableReflectionKey(IN HKEY hBase)
{
    FIXME("RegDisableReflectionKey(0x%p) UNIMPLEMENTED!\n", hBase);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegEnableReflectionKey
 *
 * @unimplemented
 */
LONG WINAPI
RegEnableReflectionKey(IN HKEY hBase)
{
    FIXME("RegEnableReflectionKey(0x%p) UNIMPLEMENTED!\n", hBase);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/******************************************************************************
 * RegpApplyRestrictions   [internal]
 *
 * Helper function for RegGetValueA/W.
 */
static VOID
RegpApplyRestrictions(DWORD dwFlags,
                      DWORD dwType,
                      DWORD cbData,
                      PLONG ret)
{
    /* Check if the type is restricted by the passed flags */
    if (*ret == ERROR_SUCCESS || *ret == ERROR_MORE_DATA)
    {
        DWORD dwMask = 0;

        switch (dwType)
        {
        case REG_NONE: dwMask = RRF_RT_REG_NONE; break;
        case REG_SZ: dwMask = RRF_RT_REG_SZ; break;
        case REG_EXPAND_SZ: dwMask = RRF_RT_REG_EXPAND_SZ; break;
        case REG_MULTI_SZ: dwMask = RRF_RT_REG_MULTI_SZ; break;
        case REG_BINARY: dwMask = RRF_RT_REG_BINARY; break;
        case REG_DWORD: dwMask = RRF_RT_REG_DWORD; break;
        case REG_QWORD: dwMask = RRF_RT_REG_QWORD; break;
        }

        if (dwFlags & dwMask)
        {
            /* Type is not restricted, check for size mismatch */
            if (dwType == REG_BINARY)
            {
                DWORD cbExpect = 0;

                if ((dwFlags & RRF_RT_DWORD) == RRF_RT_DWORD)
                    cbExpect = 4;
                else if ((dwFlags & RRF_RT_QWORD) == RRF_RT_QWORD)
                    cbExpect = 8;

                if (cbExpect && cbData != cbExpect)
                    *ret = ERROR_DATATYPE_MISMATCH;
            }
        }
        else *ret = ERROR_UNSUPPORTED_TYPE;
    }
}


/******************************************************************************
 * RegGetValueW   [ADVAPI32.@]
 *
 * Retrieves the type and data for a value name associated with a key,
 * optionally expanding its content and restricting its type.
 *
 * PARAMS
 *  hKey      [I] Handle to an open key.
 *  pszSubKey [I] Name of the subkey of hKey.
 *  pszValue  [I] Name of value under hKey/szSubKey to query.
 *  dwFlags   [I] Flags restricting the value type to retrieve.
 *  pdwType   [O] Destination for the values type, may be NULL.
 *  pvData    [O] Destination for the values content, may be NULL.
 *  pcbData   [I/O] Size of pvData, updated with the size in bytes required to
 *                  retrieve the whole content, including the trailing '\0'
 *                  for strings.
 *
 * RETURNS
 *  Success: ERROR_SUCCESS
 *  Failure: nonzero error code from Winerror.h
 *
 * NOTES
 *  - Unless RRF_NOEXPAND is specified, REG_EXPAND_SZ values are automatically
 *    expanded and pdwType is set to REG_SZ instead.
 *  - Restrictions are applied after expanding, using RRF_RT_REG_EXPAND_SZ 
 *    without RRF_NOEXPAND is thus not allowed.
 *    An exception is the case where RRF_RT_ANY is specified, because then
 *    RRF_NOEXPAND is allowed.
 */
LSTATUS WINAPI
RegGetValueW(HKEY hKey,
             LPCWSTR pszSubKey,
             LPCWSTR pszValue,
             DWORD dwFlags,
             LPDWORD pdwType,
             PVOID pvData,
             LPDWORD pcbData)
{
    DWORD dwType, cbData = pcbData ? *pcbData : 0;
    PVOID pvBuf = NULL;
    LONG ret;

    TRACE("(%p,%s,%s,%ld,%p,%p,%p=%ld)\n",
          hKey, debugstr_w(pszSubKey), debugstr_w(pszValue), dwFlags, pdwType,
          pvData, pcbData, cbData);

    if (pvData && !pcbData)
        return ERROR_INVALID_PARAMETER;
    if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
            ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
        return ERROR_INVALID_PARAMETER;

    if (pszSubKey && pszSubKey[0])
    {
        ret = RegOpenKeyExW(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
        if (ret != ERROR_SUCCESS) return ret;
    }

    ret = RegQueryValueExW(hKey, pszValue, NULL, &dwType, pvData, &cbData);

    /* If we are going to expand we need to read in the whole the value even
     * if the passed buffer was too small as the expanded string might be
     * smaller than the unexpanded one and could fit into cbData bytes. */
    if ((ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) &&
        dwType == REG_EXPAND_SZ && !(dwFlags & RRF_NOEXPAND))
    {
        do
        {
            HeapFree(GetProcessHeap(), 0, pvBuf);

            pvBuf = HeapAlloc(GetProcessHeap(), 0, cbData);
            if (!pvBuf)
            {
                ret = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (ret == ERROR_MORE_DATA || !pvData)
                ret = RegQueryValueExW(hKey, pszValue, NULL,
                                       &dwType, pvBuf, &cbData);
            else
            {
                /* Even if cbData was large enough we have to copy the
                 * string since ExpandEnvironmentStrings can't handle
                 * overlapping buffers. */
                CopyMemory(pvBuf, pvData, cbData);
            }

            /* Both the type or the value itself could have been modified in
             * between so we have to keep retrying until the buffer is large
             * enough or we no longer have to expand the value. */
        }
        while (dwType == REG_EXPAND_SZ && ret == ERROR_MORE_DATA);

        if (ret == ERROR_SUCCESS)
        {
            /* Recheck dwType in case it changed since the first call */
            if (dwType == REG_EXPAND_SZ)
            {
                cbData = ExpandEnvironmentStringsW(pvBuf, pvData,
                                                   pcbData ? *pcbData : 0) * sizeof(WCHAR);
                dwType = REG_SZ;
                if (pvData && pcbData && cbData > *pcbData)
                    ret = ERROR_MORE_DATA;
            }
            else if (pvData)
                CopyMemory(pvData, pvBuf, *pcbData);
        }

        HeapFree(GetProcessHeap(), 0, pvBuf);
    }

    if (pszSubKey && pszSubKey[0])
        RegCloseKey(hKey);

    RegpApplyRestrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType)
        *pdwType = dwType;

    if (pcbData)
        *pcbData = cbData;

    return ret;
}


/******************************************************************************
 * RegGetValueA   [ADVAPI32.@]
 *
 * See RegGetValueW.
 */
LSTATUS WINAPI
RegGetValueA(HKEY hKey,
             LPCSTR pszSubKey,
             LPCSTR pszValue,
             DWORD dwFlags,
             LPDWORD pdwType,
             PVOID pvData,
             LPDWORD pcbData)
{
    DWORD dwType, cbData = pcbData ? *pcbData : 0;
    PVOID pvBuf = NULL;
    LONG ret;

    TRACE("(%p,%s,%s,%ld,%p,%p,%p=%ld)\n",
          hKey, pszSubKey, pszValue, dwFlags, pdwType, pvData, pcbData,
          cbData);

    if (pvData && !pcbData)
        return ERROR_INVALID_PARAMETER;
    if ((dwFlags & RRF_RT_REG_EXPAND_SZ) && !(dwFlags & RRF_NOEXPAND) &&
            ((dwFlags & RRF_RT_ANY) != RRF_RT_ANY))
        return ERROR_INVALID_PARAMETER;

    if (pszSubKey && pszSubKey[0])
    {
        ret = RegOpenKeyExA(hKey, pszSubKey, 0, KEY_QUERY_VALUE, &hKey);
        if (ret != ERROR_SUCCESS) return ret;
    }

    ret = RegQueryValueExA(hKey, pszValue, NULL, &dwType, pvData, &cbData);

    /* If we are going to expand we need to read in the whole the value even
     * if the passed buffer was too small as the expanded string might be
     * smaller than the unexpanded one and could fit into cbData bytes. */
    if ((ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA) &&
        (dwType == REG_EXPAND_SZ && !(dwFlags & RRF_NOEXPAND)))
    {
        do {
            HeapFree(GetProcessHeap(), 0, pvBuf);

            pvBuf = HeapAlloc(GetProcessHeap(), 0, cbData);
            if (!pvBuf)
            {
                ret = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            if (ret == ERROR_MORE_DATA || !pvData)
                ret = RegQueryValueExA(hKey, pszValue, NULL,
                                       &dwType, pvBuf, &cbData);
            else
            {
                /* Even if cbData was large enough we have to copy the
                 * string since ExpandEnvironmentStrings can't handle
                 * overlapping buffers. */
                CopyMemory(pvBuf, pvData, cbData);
            }

            /* Both the type or the value itself could have been modified in
             * between so we have to keep retrying until the buffer is large
             * enough or we no longer have to expand the value. */
        } while (dwType == REG_EXPAND_SZ && ret == ERROR_MORE_DATA);

        if (ret == ERROR_SUCCESS)
        {
            /* Recheck dwType in case it changed since the first call */
            if (dwType == REG_EXPAND_SZ)
            {
                cbData = ExpandEnvironmentStringsA(pvBuf, pvData,
                                                   pcbData ? *pcbData : 0);
                dwType = REG_SZ;
                if(pvData && pcbData && cbData > *pcbData)
                    ret = ERROR_MORE_DATA;
            }
            else if (pvData)
                CopyMemory(pvData, pvBuf, *pcbData);
        }

        HeapFree(GetProcessHeap(), 0, pvBuf);
    }

    if (pszSubKey && pszSubKey[0])
        RegCloseKey(hKey);

    RegpApplyRestrictions(dwFlags, dwType, cbData, &ret);

    if (pvData && ret != ERROR_SUCCESS && (dwFlags & RRF_ZEROONFAILURE))
        ZeroMemory(pvData, *pcbData);

    if (pdwType) *pdwType = dwType;
    if (pcbData) *pcbData = cbData;

    return ret;
}


/************************************************************************
 *  RegSetKeyValueW
 *
 * @implemented
 */
LONG WINAPI
RegSetKeyValueW(IN HKEY hKey,
                IN LPCWSTR lpSubKey  OPTIONAL,
                IN LPCWSTR lpValueName  OPTIONAL,
                IN DWORD dwType,
                IN LPCVOID lpData  OPTIONAL,
                IN DWORD cbData)
{
    HANDLE KeyHandle, CurKey, SubKeyHandle = NULL;
    NTSTATUS Status;
    LONG Ret;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpSubKey != NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING SubKeyName;

        RtlInitUnicodeString(&SubKeyName,
                             (LPWSTR)lpSubKey);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle,
                           KEY_SET_VALUE,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            Ret = RtlNtStatusToDosError(Status);
            goto Cleanup;
        }

        CurKey = SubKeyHandle;
    }
    else
        CurKey = KeyHandle;

    Ret = RegSetValueExW(CurKey,
                         lpValueName,
                         0,
                         dwType,
                         lpData,
                         cbData);

    if (SubKeyHandle != NULL)
    {
        NtClose(SubKeyHandle);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return Ret;
}


/************************************************************************
 *  RegSetKeyValueA
 *
 * @implemented
 */
LONG WINAPI
RegSetKeyValueA(IN HKEY hKey,
                IN LPCSTR lpSubKey  OPTIONAL,
                IN LPCSTR lpValueName  OPTIONAL,
                IN DWORD dwType,
                IN LPCVOID lpData  OPTIONAL,
                IN DWORD cbData)
{
    HANDLE KeyHandle, CurKey, SubKeyHandle = NULL;
    NTSTATUS Status;
    LONG Ret;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpSubKey != NULL)
    {
        OBJECT_ATTRIBUTES ObjectAttributes;
        UNICODE_STRING SubKeyName;

        if (!RtlCreateUnicodeStringFromAsciiz(&SubKeyName,
                                              (LPSTR)lpSubKey))
        {
            Ret = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);

        Status = NtOpenKey(&SubKeyHandle,
                           KEY_SET_VALUE,
                           &ObjectAttributes);

        RtlFreeUnicodeString(&SubKeyName);

        if (!NT_SUCCESS(Status))
        {
            Ret = RtlNtStatusToDosError(Status);
            goto Cleanup;
        }

        CurKey = SubKeyHandle;
    }
    else
        CurKey = KeyHandle;

    Ret = RegSetValueExA(CurKey,
                         lpValueName,
                         0,
                         dwType,
                         lpData,
                         cbData);

    if (SubKeyHandle != NULL)
    {
        NtClose(SubKeyHandle);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return Ret;
}


/************************************************************************
 *  RegDeleteValueA
 *
 * @implemented
 */
LONG WINAPI
RegDeleteValueA(HKEY hKey,
                LPCSTR lpValueName)
{
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlCreateUnicodeStringFromAsciiz(&ValueName,
                                     (LPSTR)lpValueName);
    Status = NtDeleteValueKey(KeyHandle,
                              &ValueName);
    RtlFreeUnicodeString (&ValueName);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegDeleteValueW
 *
 * @implemented
 */
LONG WINAPI
RegDeleteValueW(HKEY hKey,
                LPCWSTR lpValueName)
{
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    HANDLE KeyHandle;

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlInitUnicodeString(&ValueName,
                         (LPWSTR)lpValueName);

    Status = NtDeleteValueKey(KeyHandle,
                              &ValueName);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegEnumKeyA
 *
 * @implemented
 */
LONG WINAPI
RegEnumKeyA(HKEY hKey,
            DWORD dwIndex,
            LPSTR lpName,
            DWORD cbName)
{
    DWORD dwLength;

    dwLength = cbName;
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
 *  RegEnumKeyW
 *
 * @implemented
 */
LONG WINAPI
RegEnumKeyW(HKEY hKey,
            DWORD dwIndex,
            LPWSTR lpName,
            DWORD cbName)
{
    DWORD dwLength;

    dwLength = cbName;
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
 *  RegEnumKeyExA
 *
 * @implemented
 */
LONG WINAPI
RegEnumKeyExA(HKEY hKey,
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
    ULONG ResultSize;
    HANDLE KeyHandle;
    NTSTATUS Status;

    TRACE("RegEnumKeyExA(hKey 0x%x, dwIndex %d, lpName 0x%x, *lpcbName %d, lpClass 0x%x, lpcbClass %d)\n",
          hKey, dwIndex, lpName, *lpcbName, lpClass, lpcbClass ? *lpcbClass : 0);

    if ((lpClass) && (!lpcbClass))
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&KeyHandle, hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
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
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    Status = NtEnumerateKey(KeyHandle,
                            (ULONG)dwIndex,
                            lpClass == NULL ? KeyBasicInformation : KeyNodeInformation,
                            KeyInfo,
                            BufferSize,
                            &ResultSize);
    TRACE("NtEnumerateKey() returned status 0x%X\n", Status);
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

    /*TRACE("Key Namea0 Length %d\n", StringU.Length);*/ /* BUGBUG could be uninitialized */
    TRACE("Key Name1 Length %d\n", NameLength);
    TRACE("Key Name Length %d\n", *lpcbName);
    TRACE("Key Name %s\n", lpName);

    RtlFreeHeap(ProcessHeap,
                0,
                KeyInfo);

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegEnumKeyExW
 *
 * @implemented
 */
LONG WINAPI
RegEnumKeyExW(HKEY hKey,
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
        return RtlNtStatusToDosError(Status);
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

    KeyInfo = RtlAllocateHeap(ProcessHeap,
                              0,
                              BufferSize);
    if (KeyInfo == NULL)
    {
        ErrorCode = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    Status = NtEnumerateKey(KeyHandle,
                            (ULONG)dwIndex,
                            lpClass ? KeyNodeInformation : KeyBasicInformation,
                            KeyInfo,
                            BufferSize,
                            &ResultSize);
    TRACE("NtEnumerateKey() returned status 0x%X\n", Status);
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
                RtlCopyMemory(lpName,
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
                RtlCopyMemory(lpName,
                              KeyInfo->Node.Name,
                              KeyInfo->Node.NameLength);
                *lpcbName = KeyInfo->Node.NameLength / sizeof(WCHAR);
                lpName[*lpcbName] = 0;
                RtlCopyMemory(lpClass,
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

    RtlFreeHeap(ProcessHeap,
                0,
                KeyInfo);

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegEnumValueA
 *
 * @implemented
 */
LONG WINAPI
RegEnumValueA(HKEY hKey,
              DWORD index,
              LPSTR value,
              LPDWORD val_count,
              LPDWORD reserved,
              LPDWORD type,
              LPBYTE data,
              LPDWORD count)
{
    HANDLE KeyHandle;
    NTSTATUS status;
    ULONG total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name );

    //TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
      //    hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved)
        return ERROR_INVALID_PARAMETER;

    status = MapDefaultKey(&KeyHandle, hKey);
    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
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
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto done;
            }
            info = (KEY_VALUE_FULL_INFORMATION *)buf_ptr;
            status = NtEnumerateValueKey( KeyHandle, index, KeyValueFullInformation,
                                          buf_ptr, total_size, &total_size );
        }

        if (status) goto done;

        if (is_string(info->Type))
        {
            ULONG len;
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
            ULONG len;

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
    ClosePredefKey(KeyHandle);
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
LONG WINAPI
RegEnumValueW(HKEY hKey,
              DWORD index,
              LPWSTR value,
              PDWORD val_count,
              PDWORD reserved,
              PDWORD type,
              LPBYTE data,
              PDWORD count)
{
    HANDLE KeyHandle;
    NTSTATUS status;
    ULONG total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_FULL_INFORMATION *info = (KEY_VALUE_FULL_INFORMATION *)buffer;
    static const int info_size = FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name );

    //TRACE("(%p,%ld,%p,%p,%p,%p,%p,%p)\n",
    //      hkey, index, value, val_count, reserved, type, data, count );

    /* NT only checks count, not val_count */
    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    status = MapDefaultKey(&KeyHandle, hKey);
    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
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
            {
                status = ERROR_NOT_ENOUGH_MEMORY;
                goto done;
            }
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
    ClosePredefKey(KeyHandle);
    return RtlNtStatusToDosError(status);
}


/************************************************************************
 *  RegFlushKey
 *
 * @implemented
 */
LONG WINAPI
RegFlushKey(HKEY hKey)
{
    HANDLE KeyHandle;
    NTSTATUS Status;

    if (hKey == HKEY_PERFORMANCE_DATA)
    {
        return ERROR_SUCCESS;
    }

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    Status = NtFlushKey(KeyHandle);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegGetKeySecurity
 *
 * @implemented
 */
LONG WINAPI
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
        TRACE("MapDefaultKey() failed (Status %lx)\n", Status);
        return RtlNtStatusToDosError(Status);
    }

#if 0
    Status = NtQuerySecurityObject(KeyHandle,
                                   SecurityInformation,
                                   pSecurityDescriptor,
                                   *lpcbSecurityDescriptor,
                                   lpcbSecurityDescriptor);
#endif

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
         WARN("NtQuerySecurityObject() failed (Status %lx)\n", Status);
         return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegLoadKeyA
 *
 * @implemented
 */
LONG WINAPI
RegLoadKeyA(HKEY hKey,
            LPCSTR lpSubKey,
            LPCSTR lpFile)
{
    UNICODE_STRING FileName;
    UNICODE_STRING KeyName;
    LONG ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&KeyName,
                                     (LPSTR)lpSubKey);
    RtlCreateUnicodeStringFromAsciiz(&FileName,
                                     (LPSTR)lpFile);

    ErrorCode = RegLoadKeyW(hKey,
                            KeyName.Buffer,
                            FileName.Buffer);

    RtlFreeUnicodeString(&FileName);
    RtlFreeUnicodeString(&KeyName);

    return ErrorCode;
}


/************************************************************************
 *  RegLoadKeyW
 *
 * @implemented
 */
LONG WINAPI
RegLoadKeyW(HKEY hKey,
            LPCWSTR lpSubKey,
            LPCWSTR lpFile)
{
    OBJECT_ATTRIBUTES FileObjectAttributes;
    OBJECT_ATTRIBUTES KeyObjectAttributes;
    UNICODE_STRING FileName;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LONG ErrorCode = ERROR_SUCCESS;

    if (hKey == HKEY_PERFORMANCE_DATA)
    {
        return ERROR_INVALID_HANDLE;
    }

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (!RtlDosPathNameToNtPathName_U(lpFile,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
      ErrorCode = ERROR_BAD_PATHNAME;
      goto Cleanup;
    }

    InitializeObjectAttributes(&FileObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    RtlInitUnicodeString(&KeyName,
                         (LPWSTR)lpSubKey);

    InitializeObjectAttributes(&KeyObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               KeyHandle,
                               NULL);

    Status = NtLoadKey(&KeyObjectAttributes,
                       &FileObjectAttributes);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        ErrorCode = RtlNtStatusToDosError(Status);
        goto Cleanup;
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegNotifyChangeKeyValue
 *
 * @unimplemented
 */
LONG WINAPI
RegNotifyChangeKeyValue(HKEY hKey,
                        BOOL bWatchSubtree,
                        DWORD dwNotifyFilter,
                        HANDLE hEvent,
                        BOOL fAsynchronous)
{
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LONG ErrorCode = ERROR_SUCCESS;

    if (hKey == HKEY_PERFORMANCE_DATA)
    {
        return ERROR_INVALID_HANDLE;
    }

    if (fAsynchronous == TRUE && hEvent == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    /* FIXME: Remote key handles must fail */

    Status = NtNotifyChangeKey(KeyHandle,
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
        ErrorCode = RtlNtStatusToDosError(Status);
    }

    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegOpenCurrentUser
 *
 * @implemented
 */
LONG WINAPI
RegOpenCurrentUser(IN REGSAM samDesired,
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
LONG WINAPI
RegOpenKeyA(HKEY hKey,
            LPCSTR lpSubKey,
            PHKEY phkResult)
{
    TRACE("RegOpenKeyA hKey 0x%x lpSubKey %s phkResult %p\n",
          hKey, lpSubKey, phkResult);

    if (!hKey && lpSubKey && phkResult)
    {
        return ERROR_INVALID_HANDLE;
    }

    if (!lpSubKey || !*lpSubKey)
    {
        *phkResult = hKey;
        return ERROR_SUCCESS;
    }

    return RegOpenKeyExA(hKey,
                         lpSubKey,
                         0,
                         MAXIMUM_ALLOWED,
                         phkResult);
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
LONG WINAPI
RegOpenKeyW(HKEY hKey,
            LPCWSTR lpSubKey,
            PHKEY phkResult)
{
    TRACE("RegOpenKeyW hKey 0x%x lpSubKey %S phkResult %p\n",
          hKey, lpSubKey, phkResult);

    if (!hKey && lpSubKey && phkResult)
    {
        return ERROR_INVALID_HANDLE;
    }

    if (!lpSubKey || !*lpSubKey)
    {
        *phkResult = hKey;
        return ERROR_SUCCESS;
    }

    return RegOpenKeyExW(hKey,
                         lpSubKey,
                         0,
                         MAXIMUM_ALLOWED,
                         phkResult);
}


/************************************************************************
 *  RegOpenKeyExA
 *
 * @implemented
 */
LONG WINAPI
RegOpenKeyExA(HKEY hKey,
              LPCSTR lpSubKey,
              DWORD ulOptions,
              REGSAM samDesired,
              PHKEY phkResult)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyString;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LONG ErrorCode = ERROR_SUCCESS;

    TRACE("RegOpenKeyExA hKey 0x%x lpSubKey %s ulOptions 0x%x samDesired 0x%x phkResult %p\n",
          hKey, lpSubKey, ulOptions, samDesired, phkResult);
    if (!phkResult)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlCreateUnicodeStringFromAsciiz(&SubKeyString,
                                     (LPSTR)lpSubKey);
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyString,
                               OBJ_CASE_INSENSITIVE,
                               KeyHandle,
                               NULL);

    Status = NtOpenKey((PHANDLE)phkResult,
                       samDesired,
                       &ObjectAttributes);
    RtlFreeUnicodeString(&SubKeyString);
    if (!NT_SUCCESS(Status))
    {
        ErrorCode = RtlNtStatusToDosError(Status);
    }

    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegOpenKeyExW
 *
 * @implemented
 */
LONG WINAPI
RegOpenKeyExW(HKEY hKey,
              LPCWSTR lpSubKey,
              DWORD ulOptions,
              REGSAM samDesired,
              PHKEY phkResult)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyString;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LONG ErrorCode = ERROR_SUCCESS;

    TRACE("RegOpenKeyExW hKey 0x%x lpSubKey %S ulOptions 0x%x samDesired 0x%x phkResult %p\n",
          hKey, lpSubKey, ulOptions, samDesired, phkResult);
    if (!phkResult)
    {
        return ERROR_INVALID_PARAMETER;
    }

    Status = MapDefaultKey(&KeyHandle, hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpSubKey != NULL)
        RtlInitUnicodeString(&SubKeyString, (LPWSTR)lpSubKey);
    else
        RtlInitUnicodeString(&SubKeyString, (LPWSTR)L"");

    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyString,
                               OBJ_CASE_INSENSITIVE,
                               KeyHandle,
                               NULL);

    Status = NtOpenKey((PHANDLE)phkResult,
                       samDesired,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        ErrorCode = RtlNtStatusToDosError(Status);
    }

    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegOpenUserClassesRoot
 *
 * @implemented
 */
LONG WINAPI
RegOpenUserClassesRoot(IN HANDLE hToken,
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
        return RtlNtStatusToDosError(Status);
    }
    RegInitialize(); /* HACK until delay-loading is implemented */
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
        return RtlNtStatusToDosError(Status);
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
        return RtlNtStatusToDosError(Status);
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
        return RtlNtStatusToDosError(Status);
    }

    /* build the string */
    RtlAppendUnicodeToString(&UserClassesKeyRoot,
                             UserClassesKeyPrefix);
    RtlAppendUnicodeStringToString(&UserClassesKeyRoot,
                                   &UserSidString);
    RtlAppendUnicodeToString(&UserClassesKeyRoot,
                             UserClassesKeySuffix);

    TRACE("RegOpenUserClassesRoot: Absolute path: %wZ\n", &UserClassesKeyRoot);

    /*
     * Open the key
     */
    InitializeObjectAttributes(&ObjectAttributes,
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
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegQueryInfoKeyA
 *
 * @implemented
 */
LONG WINAPI
RegQueryInfoKeyA(HKEY hKey,
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

    RtlInitUnicodeString(&UnicodeString,
                         NULL);
    if (lpClass != NULL)
    {
        UnicodeString.Buffer = &ClassName[0];
        UnicodeString.MaximumLength = sizeof(ClassName);
        AnsiString.MaximumLength = *lpcbClass;
    }

    ErrorCode = RegQueryInfoKeyW(hKey,
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
        RtlUnicodeStringToAnsiString(&AnsiString,
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
LONG WINAPI
RegQueryInfoKeyW(HKEY hKey,
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

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
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
        FullInfo = RtlAllocateHeap(ProcessHeap,
                                   0,
                                   FullInfoSize);
        if (FullInfo == NULL)
        {
            ErrorCode = ERROR_OUTOFMEMORY;
            goto Cleanup;
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

    Status = NtQueryKey(KeyHandle,
                        KeyFullInformation,
                        FullInfo,
                        FullInfoSize,
                        &Length);
    TRACE("NtQueryKey() returned status 0x%X\n", Status);
    if (!NT_SUCCESS(Status))
    {
        if (lpClass != NULL)
        {
            RtlFreeHeap(ProcessHeap,
                        0,
                        FullInfo);
        }

        ErrorCode = RtlNtStatusToDosError(Status);
        goto Cleanup;
    }

    TRACE("SubKeys %d\n", FullInfo->SubKeys);
    if (lpcSubKeys != NULL)
    {
        *lpcSubKeys = FullInfo->SubKeys;
    }

    TRACE("MaxNameLen %lu\n", FullInfo->MaxNameLen);
    if (lpcbMaxSubKeyLen != NULL)
    {
        *lpcbMaxSubKeyLen = FullInfo->MaxNameLen / sizeof(WCHAR) + 1;
    }

    TRACE("MaxClassLen %lu\n", FullInfo->MaxClassLen);
    if (lpcbMaxClassLen != NULL)
    {
        *lpcbMaxClassLen = FullInfo->MaxClassLen / sizeof(WCHAR) + 1;
    }

    TRACE("Values %lu\n", FullInfo->Values);
    if (lpcValues != NULL)
    {
        *lpcValues = FullInfo->Values;
    }

    TRACE("MaxValueNameLen %lu\n", FullInfo->MaxValueNameLen);
    if (lpcbMaxValueNameLen != NULL)
    {
        *lpcbMaxValueNameLen = FullInfo->MaxValueNameLen / sizeof(WCHAR) + 1;
    }

    TRACE("MaxValueDataLen %lu\n", FullInfo->MaxValueDataLen);
    if (lpcbMaxValueLen != NULL)
    {
        *lpcbMaxValueLen = FullInfo->MaxValueDataLen;
    }

#if 0
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

            ErrorCode = RtlNtStatusToDosError(Status);
            goto Cleanup;
        }
    }
#endif

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
            RtlCopyMemory(lpClass,
                          FullInfo->Class,
                          FullInfo->ClassLength);
            *lpcbClass = FullInfo->ClassLength / sizeof(WCHAR);
            lpClass[*lpcbClass] = 0;
        }

        RtlFreeHeap(ProcessHeap,
                    0,
                    FullInfo);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegQueryMultipleValuesA
 *
 * @implemented
 */
LONG WINAPI
RegQueryMultipleValuesA(HKEY hKey,
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

    TRACE("RegQueryMultipleValuesA(%p,%p,%ld,%p,%p=%ld)\n",
          hKey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

    for (i = 0; i < num_vals; i++)
    {
        val_list[i].ve_valuelen = 0;
        ErrorCode = RegQueryValueExA(hKey,
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
            ErrorCode = RegQueryValueExA(hKey,
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
LONG WINAPI
RegQueryMultipleValuesW(HKEY hKey,
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

    TRACE("RegQueryMultipleValuesW(%p,%p,%ld,%p,%p=%ld)\n",
          hKey, val_list, num_vals, lpValueBuf, ldwTotsize, *ldwTotsize);

    for (i = 0; i < num_vals; i++)
    {
        val_list[i].ve_valuelen = 0;
        ErrorCode = RegQueryValueExW(hKey,
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
            ErrorCode = RegQueryValueExW(hKey,
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
 *  RegQueryReflectionKey
 *
 * @unimplemented
 */
LONG WINAPI
RegQueryReflectionKey(IN HKEY hBase,
                      OUT BOOL* bIsReflectionDisabled)
{
    FIXME("RegQueryReflectionKey(0x%p, 0x%p) UNIMPLEMENTED!\n",
          hBase, bIsReflectionDisabled);
    return ERROR_CALL_NOT_IMPLEMENTED;
}


/************************************************************************
 *  RegQueryValueExA
 *
 * @implemented
 */
LONG WINAPI
RegQueryValueExA(HKEY hKey,
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

    TRACE("hKey 0x%X  lpValueName %s  lpData 0x%X  lpcbData %d\n",
          hKey, lpValueName, lpData, lpcbData ? *lpcbData : 0);

    if (lpData != NULL && lpcbData == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (lpData)
    {
        ValueData.Length = 0;
        ValueData.MaximumLength = (*lpcbData + 1) * sizeof(WCHAR);
        ValueData.Buffer = RtlAllocateHeap(ProcessHeap,
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

        if (lpcbData)
            *lpcbData = 0;
    }

    RtlCreateUnicodeStringFromAsciiz(&ValueName,
                                     (LPSTR)lpValueName);

    Length = (lpcbData == NULL) ? 0 : *lpcbData * sizeof(WCHAR);
    ErrorCode = RegQueryValueExW(hKey,
                                 ValueName.Buffer,
                                 lpReserved,
                                 &Type,
                                 (lpData == NULL) ? NULL : (LPBYTE)ValueData.Buffer,
                                 &Length);
    TRACE("ErrorCode %lu\n", ErrorCode);
    RtlFreeUnicodeString(&ValueName);

    if (ErrorCode == ERROR_SUCCESS ||
        ErrorCode == ERROR_MORE_DATA)
    {

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

    if (lpType != NULL)
    {
        *lpType = Type;
    }

    if (ValueData.Buffer != NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, ValueData.Buffer);
    }

    return ErrorCode;
}


/************************************************************************
 *  RegQueryValueExW
 *
 * @implemented
 */
LONG
WINAPI
RegQueryValueExW(HKEY hkeyorg,
                 LPCWSTR name,
                 LPDWORD reserved,
                 LPDWORD type,
                 LPBYTE data,
                 LPDWORD count)
{
    HANDLE hkey;
    NTSTATUS status;
    UNICODE_STRING name_str;
    DWORD total_size;
    char buffer[256], *buf_ptr = buffer;
    KEY_VALUE_PARTIAL_INFORMATION *info = (KEY_VALUE_PARTIAL_INFORMATION *)buffer;
    static const int info_size = offsetof( KEY_VALUE_PARTIAL_INFORMATION, Data );

    TRACE("(%p,%s,%p,%p,%p,%p=%d)\n",
          hkey, debugstr_w(name), reserved, type, data, count,
          (count && data) ? *count : 0 );

    if ((data && !count) || reserved) return ERROR_INVALID_PARAMETER;

    status = MapDefaultKey(&hkey, hkeyorg);
    if (!NT_SUCCESS(status))
    {
        return RtlNtStatusToDosError(status);
    }

    RtlInitUnicodeString( &name_str, name );

    if (data) total_size = min( sizeof(buffer), *count + info_size );
    else
    {
        total_size = info_size;
        if (count) *count = 0;
    }

    /* this matches Win9x behaviour - NT sets *type to a random value */
    if (type) *type = REG_NONE;

    status = NtQueryValueKey( hkey, &name_str, KeyValuePartialInformation,
                              buffer, total_size, &total_size );
    if (status && status != STATUS_BUFFER_OVERFLOW) goto done;

    if (data)
    {
        /* retry with a dynamically allocated buffer */
        while (status == STATUS_BUFFER_OVERFLOW && total_size - info_size <= *count)
        {
            if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
            if (!(buf_ptr = HeapAlloc( GetProcessHeap(), 0, total_size )))
            {
                ClosePredefKey(hkey);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
            info = (KEY_VALUE_PARTIAL_INFORMATION *)buf_ptr;
            status = NtQueryValueKey( hkey, &name_str, KeyValuePartialInformation,
                                      buf_ptr, total_size, &total_size );
        }

        if (!status)
        {
            memcpy( data, buf_ptr + info_size, total_size - info_size );
            /* if the type is REG_SZ and data is not 0-terminated
             * and there is enough space in the buffer NT appends a \0 */
            if (total_size - info_size <= *count-sizeof(WCHAR) && is_string(info->Type))
            {
                WCHAR *ptr = (WCHAR *)(data + total_size - info_size);
                if (ptr > (WCHAR *)data && ptr[-1]) *ptr = 0;
            }
        }
        else if (status != STATUS_BUFFER_OVERFLOW) goto done;
    }
    else status = STATUS_SUCCESS;

    if (type) *type = info->Type;
    if (count) *count = total_size - info_size;

 done:
    if (buf_ptr != buffer) HeapFree( GetProcessHeap(), 0, buf_ptr );
    ClosePredefKey(hkey);
    return RtlNtStatusToDosError(status);
}


/************************************************************************
 *  RegQueryValueA
 *
 * @implemented
 */
LSTATUS WINAPI RegQueryValueA( HKEY hkey, LPCSTR name, LPSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%p,%s,%p,%d)\n", hkey, debugstr_a(name), data, count ? *count : 0 );

    if (name && name[0])
    {
    if ((ret = RegOpenKeyA( hkey, name, &subkey )) != ERROR_SUCCESS) return ret;
    }
    ret = RegQueryValueExA( subkey, NULL, NULL, NULL, (LPBYTE)data, (LPDWORD)count );
    if (subkey != hkey) RegCloseKey( subkey );
    if (ret == ERROR_FILE_NOT_FOUND)
    {
    /* return empty string if default value not found */
    if (data) *data = 0;
    if (count) *count = 1;
    ret = ERROR_SUCCESS;
    }
    return ret;
}


/************************************************************************
 *  RegQueryValueW
 *
 * @implemented
 */
LSTATUS WINAPI RegQueryValueW( HKEY hkey, LPCWSTR name, LPWSTR data, LPLONG count )
{
    DWORD ret;
    HKEY subkey = hkey;

    TRACE("(%p,%s,%p,%d)\n", hkey, debugstr_w(name), data, count ? *count : 0 );
    if (hkey == NULL)
    {
       return ERROR_INVALID_HANDLE;
    }
    if (name && name[0])
    {
        ret = RegOpenKeyW( hkey, name, &subkey);
        if (ret != ERROR_SUCCESS) 
        {
            return ret;
        }
    }

    ret = RegQueryValueExW( subkey, NULL, NULL, NULL, (LPBYTE)data, (LPDWORD)count );
    
    if (subkey != hkey) 
    {
        RegCloseKey( subkey );
    }

    if (ret == ERROR_FILE_NOT_FOUND)
    {
        /* return empty string if default value not found */
        if (data) 
            *data = 0;
        if (count) 
            *count = sizeof(WCHAR);
        ret = ERROR_SUCCESS;
    }
    return ret;
}


/************************************************************************
 *  RegReplaceKeyA
 *
 * @implemented
 */
LONG WINAPI
RegReplaceKeyA(HKEY hKey,
               LPCSTR lpSubKey,
               LPCSTR lpNewFile,
               LPCSTR lpOldFile)
{
    UNICODE_STRING SubKey;
    UNICODE_STRING NewFile;
    UNICODE_STRING OldFile;
    LONG ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&SubKey,
                                     (PCSZ)lpSubKey);
    RtlCreateUnicodeStringFromAsciiz(&OldFile,
                                     (PCSZ)lpOldFile);
    RtlCreateUnicodeStringFromAsciiz(&NewFile,
                                     (PCSZ)lpNewFile);

    ErrorCode = RegReplaceKeyW(hKey,
                               SubKey.Buffer,
                               NewFile.Buffer,
                               OldFile.Buffer);

    RtlFreeUnicodeString(&OldFile);
    RtlFreeUnicodeString(&NewFile);
    RtlFreeUnicodeString(&SubKey);

    return ErrorCode;
}


/************************************************************************
 *  RegReplaceKeyW
 *
 * @unimplemented
 */
LONG WINAPI
RegReplaceKeyW(HKEY hKey,
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
    LONG ErrorCode = ERROR_SUCCESS;

    if (hKey == HKEY_PERFORMANCE_DATA)
    {
        return ERROR_INVALID_HANDLE;
    }

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    /* Open the real key */
    if (lpSubKey != NULL && *lpSubKey != (WCHAR)0)
    {
        RtlInitUnicodeString(&SubKeyName,
                             (PWSTR)lpSubKey);
        InitializeObjectAttributes(&KeyObjectAttributes,
                                   &SubKeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);
        Status = NtOpenKey(&RealKeyHandle,
                           MAXIMUM_ALLOWED,
                           &KeyObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            ErrorCode = RtlNtStatusToDosError(Status);
            goto Cleanup;
        }

        CloseRealKey = TRUE;
    }
    else
    {
        RealKeyHandle = KeyHandle;
        CloseRealKey = FALSE;
    }

    /* Convert new file name */
    if (!RtlDosPathNameToNtPathName_U(lpNewFile,
                                      &NewFileName,
                                      NULL,
                                      NULL))
    {
        if (CloseRealKey)
        {
            NtClose(RealKeyHandle);
        }

        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    InitializeObjectAttributes(&NewObjectAttributes,
                               &NewFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Convert old file name */
    if (!RtlDosPathNameToNtPathName_U(lpOldFile,
                                      &OldFileName,
                                      NULL,
                                      NULL))
    {
        RtlFreeHeap(RtlGetProcessHeap (),
                    0,
                    NewFileName.Buffer);
        if (CloseRealKey)
        {
            NtClose(RealKeyHandle);
        }

        ErrorCode = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    InitializeObjectAttributes(&OldObjectAttributes,
                               &OldFileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtReplaceKey(&NewObjectAttributes,
                          RealKeyHandle,
                          &OldObjectAttributes);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                OldFileName.Buffer);
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                NewFileName.Buffer);

    if (CloseRealKey)
    {
        NtClose(RealKeyHandle);
    }

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}


/************************************************************************
 *  RegRestoreKeyA
 *
 * @implemented
 */
LONG WINAPI
RegRestoreKeyA(HKEY hKey,
               LPCSTR lpFile,
               DWORD dwFlags)
{
    UNICODE_STRING FileName;
    LONG ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&FileName,
                                     (PCSZ)lpFile);

    ErrorCode = RegRestoreKeyW(hKey,
                               FileName.Buffer,
                               dwFlags);

    RtlFreeUnicodeString(&FileName);

    return ErrorCode;
}


/************************************************************************
 *  RegRestoreKeyW
 *
 * @implemented
 */
LONG WINAPI
RegRestoreKeyW(HKEY hKey,
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

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (!RtlDosPathNameToNtPathName_U(lpFile,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtRestoreKey(KeyHandle,
                          FileHandle,
                          (ULONG)dwFlags);
    NtClose (FileHandle);

Cleanup:
    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSaveKeyA
 *
 * @implemented
 */
LONG WINAPI
RegSaveKeyA(HKEY hKey,
            LPCSTR lpFile,
            LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    UNICODE_STRING FileName;
    LONG ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&FileName,
                                     (LPSTR)lpFile);
    ErrorCode = RegSaveKeyW(hKey,
                            FileName.Buffer,
                            lpSecurityAttributes);
    RtlFreeUnicodeString(&FileName);

    return ErrorCode;
}


/************************************************************************
 *  RegSaveKeyW
 *
 * @implemented
 */
LONG WINAPI
RegSaveKeyW(HKEY hKey,
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

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (!RtlDosPathNameToNtPathName_U(lpFile,
                                      &FileName,
                                      NULL,
                                      NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (lpSecurityAttributes != NULL)
    {
        SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               SecurityDescriptor);
    Status = NtCreateFile(&FileHandle,
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
    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                FileName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = NtSaveKey(KeyHandle,
                       FileHandle);
    NtClose (FileHandle);

Cleanup:
    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSaveKeyExA
 *
 * @implemented
 */
LONG
WINAPI
RegSaveKeyExA(HKEY hKey,
              LPCSTR lpFile,
              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
              DWORD Flags)
{
    UNICODE_STRING FileName;
    LONG ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&FileName,
                                     (LPSTR)lpFile);
    ErrorCode = RegSaveKeyExW(hKey,
                              FileName.Buffer,
                              lpSecurityAttributes,
                              Flags);
    RtlFreeUnicodeString(&FileName);

    return ErrorCode;
}


/************************************************************************
 *  RegSaveKeyExW
 *
 * @unimplemented
 */
LONG
WINAPI
RegSaveKeyExW(HKEY hKey,
              LPCWSTR lpFile,
              LPSECURITY_ATTRIBUTES lpSecurityAttributes,
              DWORD Flags)
{
    switch (Flags)
    {
        case REG_STANDARD_FORMAT:
        case REG_LATEST_FORMAT:
        case REG_NO_COMPRESSION:
            break;
        default:
            return ERROR_INVALID_PARAMETER;
    }

    FIXME("RegSaveKeyExW(): Flags ignored!\n");

    return RegSaveKeyW(hKey,
                       lpFile,
                       lpSecurityAttributes);
}


/************************************************************************
 *  RegSetKeySecurity
 *
 * @implemented
 */
LONG WINAPI
RegSetKeySecurity(HKEY hKey,
                  SECURITY_INFORMATION SecurityInformation,
                  PSECURITY_DESCRIPTOR pSecurityDescriptor)
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
        return RtlNtStatusToDosError(Status);
    }

    Status = NtSetSecurityObject(KeyHandle,
                                 SecurityInformation,
                                 pSecurityDescriptor);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSetValueExA
 *
 * @implemented
 */
LONG WINAPI
RegSetValueExA(HKEY hKey,
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
        RtlCreateUnicodeStringFromAsciiz(&ValueName,
                                         (PSTR)lpValueName);
    }
    else
    {
        ValueName.Buffer = NULL;
    }

    pValueName = (LPWSTR)ValueName.Buffer;

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

        RtlInitAnsiString(&AnsiString,
                          NULL);
        AnsiString.Buffer = (PSTR)lpData;
        AnsiString.Length = cbData - 1;
        AnsiString.MaximumLength = cbData;
        RtlAnsiStringToUnicodeString(&Data,
                                     &AnsiString,
                                     TRUE);
        pData = (LPBYTE)Data.Buffer;
        DataSize = cbData * sizeof(WCHAR);
    }
    else
    {
        RtlInitUnicodeString(&Data,
                             NULL);
        pData = (LPBYTE)lpData;
        DataSize = cbData;
    }

    ErrorCode = RegSetValueExW(hKey,
                               pValueName,
                               Reserved,
                               dwType,
                               pData,
                               DataSize);
    if (pValueName != NULL)
    {
        RtlFreeHeap(ProcessHeap,
                    0,
                    ValueName.Buffer);
    }

    if (Data.Buffer != NULL)
    {
        RtlFreeHeap(ProcessHeap,
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
LONG WINAPI
RegSetValueExW(HKEY hKey,
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

    Status = MapDefaultKey(&KeyHandle,
                           hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    if (lpValueName != NULL)
    {
        RtlInitUnicodeString(&ValueName,
                             lpValueName);
    }
    else
    {
        RtlInitUnicodeString(&ValueName, L"");
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

    Status = NtSetValueKey(KeyHandle,
                           pValueName,
                           0,
                           dwType,
                           (PVOID)lpData,
                           (ULONG)cbData);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/************************************************************************
 *  RegSetValueA
 *
 * @implemented
 */
LONG WINAPI
RegSetValueA(HKEY hKeyOriginal,
             LPCSTR lpSubKey,
             DWORD dwType,
             LPCSTR lpData,
             DWORD cbData)
{
    HKEY subkey;
    HANDLE hKey;
    DWORD ret;
    NTSTATUS Status;

    TRACE("(%p,%s,%d,%s,%d)\n", hKey, debugstr_a(lpSubKey), dwType, debugstr_a(lpData), cbData );

    if (dwType != REG_SZ || !lpData) return ERROR_INVALID_PARAMETER;

    Status = MapDefaultKey(&hKey, hKeyOriginal);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError (Status);
    }
    subkey = hKey;

    if (lpSubKey && lpSubKey[0])  /* need to create the subkey */
    {
        ret = RegCreateKeyA(hKey, lpSubKey, &subkey);
        if (ret != ERROR_SUCCESS)
            goto Cleanup;
    }

    ret = RegSetValueExA( subkey, NULL, 0, REG_SZ, (const BYTE*)lpData, strlen(lpData)+1 );
    if (subkey != hKey)
        RegCloseKey(subkey);

Cleanup:
    ClosePredefKey(hKey);

    return ret;
}


/************************************************************************
 *  RegSetValueW
 *
 * @implemented
 */
LONG WINAPI
RegSetValueW(HKEY hKeyOriginal,
             LPCWSTR lpSubKey,
             DWORD dwType,
             LPCWSTR lpData,
             DWORD cbData)
{
    HKEY subkey;
    HANDLE hKey;
    DWORD ret;
    NTSTATUS Status;

    TRACE("(%p,%s,%d,%s,%d)\n", hKeyOriginal, debugstr_w(lpSubKey), dwType, debugstr_w(lpData), cbData );

    if (dwType != REG_SZ || !lpData)
        return ERROR_INVALID_PARAMETER;

    Status = MapDefaultKey(&hKey,
                           hKeyOriginal);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }
    subkey = hKey;

    if (lpSubKey && lpSubKey[0])  /* need to create the subkey */
    {
        ret = RegCreateKeyW(hKey, lpSubKey, &subkey);
        if (ret != ERROR_SUCCESS)
            goto Cleanup;
    }

    ret = RegSetValueExW( subkey, NULL, 0, REG_SZ, (const BYTE*)lpData,
                          (wcslen( lpData ) + 1) * sizeof(WCHAR) );
    if (subkey != hKey)
        RegCloseKey(subkey);

Cleanup:
    ClosePredefKey(hKey);

    return ret;
}


/************************************************************************
 *  RegUnLoadKeyA
 *
 * @implemented
 */
LONG WINAPI
RegUnLoadKeyA(HKEY hKey,
              LPCSTR lpSubKey)
{
    UNICODE_STRING KeyName;
    DWORD ErrorCode;

    RtlCreateUnicodeStringFromAsciiz(&KeyName,
                                     (LPSTR)lpSubKey);

    ErrorCode = RegUnLoadKeyW(hKey,
                              KeyName.Buffer);

    RtlFreeUnicodeString (&KeyName);

    return ErrorCode;
}


/************************************************************************
 *  RegUnLoadKeyW
 *
 * @implemented
 */
LONG WINAPI
RegUnLoadKeyW(HKEY hKey,
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

    Status = MapDefaultKey(&KeyHandle, hKey);
    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    RtlInitUnicodeString(&KeyName,
                         (LPWSTR)lpSubKey);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               KeyHandle,
                               NULL);

    Status = NtUnloadKey(&ObjectAttributes);

    ClosePredefKey(KeyHandle);

    if (!NT_SUCCESS(Status))
    {
        return RtlNtStatusToDosError(Status);
    }

    return ERROR_SUCCESS;
}


/******************************************************************************
 * load_string [Internal]
 *
 * This is basically a copy of user32/resource.c's LoadStringW. Necessary to
 * avoid importing user32, which is higher level than advapi32. Helper for
 * RegLoadMUIString.
 */
static int load_string(HINSTANCE hModule, UINT resId, LPWSTR pwszBuffer, INT cMaxChars)
{
    HGLOBAL hMemory;
    HRSRC hResource;
    WCHAR *pString;
    int idxString;

    /* Negative values have to be inverted. */
    if (HIWORD(resId) == 0xffff)
        resId = (UINT)(-((INT)resId));

    /* Load the resource into memory and get a pointer to it. */
    hResource = FindResourceW(hModule, MAKEINTRESOURCEW(LOWORD(resId >> 4) + 1), (LPWSTR)RT_STRING);
    if (!hResource) return 0;
    hMemory = LoadResource(hModule, hResource);
    if (!hMemory) return 0;
    pString = LockResource(hMemory);

    /* Strings are length-prefixed. Lowest nibble of resId is an index. */
    idxString = resId & 0xf;
    while (idxString--) pString += *pString + 1;

    /* If no buffer is given, return length of the string. */
    if (!pwszBuffer) return *pString;

    /* Else copy over the string, respecting the buffer size. */
    cMaxChars = (*pString < cMaxChars) ? *pString : (cMaxChars - 1);
    if (cMaxChars >= 0)
    {
        memcpy(pwszBuffer, pString+1, cMaxChars * sizeof(WCHAR));
        pwszBuffer[cMaxChars] = L'\0';
    }

    return cMaxChars;
}


/************************************************************************
 *  RegLoadMUIStringW
 *
 * @implemented
 */
LONG WINAPI
RegLoadMUIStringW(IN HKEY hKey,
                  IN LPCWSTR pszValue  OPTIONAL,
                  OUT LPWSTR pszOutBuf,
                  IN DWORD cbOutBuf,
                  OUT LPDWORD pcbData OPTIONAL,
                  IN DWORD Flags,
                  IN LPCWSTR pszDirectory  OPTIONAL)
{
    DWORD dwValueType, cbData;
    LPWSTR pwszTempBuffer = NULL, pwszExpandedBuffer = NULL;
    LONG result;

    /* Parameter sanity checks. */
    if (!hKey || !pszOutBuf)
        return ERROR_INVALID_PARAMETER;

    if (pszDirectory && *pszDirectory)
    {
        FIXME("BaseDir parameter not yet supported!\n");
        return ERROR_INVALID_PARAMETER;
    }

    /* Check for value existence and correctness of it's type, allocate a buffer and load it. */
    result = RegQueryValueExW(hKey, pszValue, NULL, &dwValueType, NULL, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;
    if (!(dwValueType == REG_SZ || dwValueType == REG_EXPAND_SZ) || !cbData)
    {
        result = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }
    pwszTempBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
    if (!pwszTempBuffer)
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    result = RegQueryValueExW(hKey, pszValue, NULL, &dwValueType, (LPBYTE)pwszTempBuffer, &cbData);
    if (result != ERROR_SUCCESS) goto cleanup;

    /* Expand environment variables, if appropriate, or copy the original string over. */
    if (dwValueType == REG_EXPAND_SZ)
    {
        cbData = ExpandEnvironmentStringsW(pwszTempBuffer, NULL, 0) * sizeof(WCHAR);
        if (!cbData) goto cleanup;
        pwszExpandedBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
        if (!pwszExpandedBuffer)
        {
            result = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        ExpandEnvironmentStringsW(pwszTempBuffer, pwszExpandedBuffer, cbData);
    }
    else
    {
        pwszExpandedBuffer = HeapAlloc(GetProcessHeap(), 0, cbData);
        memcpy(pwszExpandedBuffer, pwszTempBuffer, cbData);
    }

    /* If the value references a resource based string, parse the value and load the string.
     * Else just copy over the original value. */
    result = ERROR_SUCCESS;
    if (*pwszExpandedBuffer != L'@') /* '@' is the prefix for resource based string entries. */
    {
        lstrcpynW(pszOutBuf, pwszExpandedBuffer, cbOutBuf / sizeof(WCHAR));
    }
    else
    {
        WCHAR *pComma = wcsrchr(pwszExpandedBuffer, L',');
        UINT uiStringId;
        HMODULE hModule;

        /* Format of the expanded value is 'path_to_dll,-resId' */
        if (!pComma || pComma[1] != L'-')
        {
            result = ERROR_BADKEY;
            goto cleanup;
        }

        uiStringId = _wtoi(pComma+2);
        *pComma = L'\0';

        hModule = LoadLibraryExW(pwszExpandedBuffer + 1, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (!hModule || !load_string(hModule, uiStringId, pszOutBuf, cbOutBuf / sizeof(WCHAR)))
            result = ERROR_BADKEY;
        FreeLibrary(hModule);
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, pwszTempBuffer);
    HeapFree(GetProcessHeap(), 0, pwszExpandedBuffer);
    return result;
}


/************************************************************************
 *  RegLoadMUIStringA
 *
 * @implemented
 */
LONG WINAPI
RegLoadMUIStringA(IN HKEY hKey,
                  IN LPCSTR pszValue  OPTIONAL,
                  OUT LPSTR pszOutBuf,
                  IN DWORD cbOutBuf,
                  OUT LPDWORD pcbData OPTIONAL,
                  IN DWORD Flags,
                  IN LPCSTR pszDirectory  OPTIONAL)
{
    UNICODE_STRING valueW, baseDirW;
    WCHAR *pwszBuffer;
    DWORD cbData = cbOutBuf * sizeof(WCHAR);
    LONG result;

    valueW.Buffer = baseDirW.Buffer = pwszBuffer = NULL;
    if (!RtlCreateUnicodeStringFromAsciiz(&valueW, pszValue) ||
        !RtlCreateUnicodeStringFromAsciiz(&baseDirW, pszDirectory) ||
        !(pwszBuffer = HeapAlloc(GetProcessHeap(), 0, cbData)))
    {
        result = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }

    result = RegLoadMUIStringW(hKey, valueW.Buffer, pwszBuffer, cbData, NULL, Flags,
                               baseDirW.Buffer);

    if (result == ERROR_SUCCESS)
    {
        cbData = WideCharToMultiByte(CP_ACP, 0, pwszBuffer, -1, pszOutBuf, cbOutBuf, NULL, NULL);
        if (pcbData)
            *pcbData = cbData;
    }

cleanup:
    HeapFree(GetProcessHeap(), 0, pwszBuffer);
    RtlFreeUnicodeString(&baseDirW);
    RtlFreeUnicodeString(&valueW);

    return result;
}

/* EOF */
