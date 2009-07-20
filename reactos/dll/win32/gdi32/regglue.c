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

#include <stdio.h>
#define WIN32_NO_STATUS
#define _WMI_SOURCE_
#include <windows.h>
#include <string.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>
#include "wine/debug.h" 

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define ClosePredefKey(Handle)                                                 \
    if ((ULONG_PTR)Handle & 0x1) {                                             \
        NtClose(Handle);                                                       \
    }
#define IsPredefKey(HKey)                                                      \
    (((ULONG)(HKey) & 0xF0000000) == 0x80000000)
#define GetPredefKeyIndex(HKey)                                                \
    ((ULONG)(HKey) & 0x0FFFFFFF)

#define MAX_DEFAULT_HANDLES   6
#define REG_MAX_NAME_SIZE     256
#define REG_MAX_DATA_SIZE     2048

static NTSTATUS MapDefaultKey(OUT PHANDLE, IN HKEY);
static NTSTATUS OpenClassesRootKey(PHANDLE KeyHandle);
static NTSTATUS OpenLocalMachineKey (PHANDLE KeyHandle);
static NTSTATUS OpenUsersKey (PHANDLE KeyHandle);
static NTSTATUS OpenCurrentConfigKey(PHANDLE KeyHandle);

LONG WINAPI RegOpenKeyA(HKEY, LPCSTR, PHKEY);
LONG WINAPI RegOpenKeyW(HKEY, LPCWSTR, PHKEY);
LONG WINAPI RegOpenKeyExA(HKEY, LPCSTR, DWORD, REGSAM, PHKEY);
LONG WINAPI RegOpenKeyExW(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY);

static HANDLE DefaultHandleTable[MAX_DEFAULT_HANDLES];
static BOOLEAN DefaultHandlesDisabled = FALSE;
static BOOLEAN DefaultHandleHKUDisabled = FALSE;


__inline static int is_string( DWORD type )
{
    return (type == REG_SZ) || (type == REG_EXPAND_SZ) || (type == REG_MULTI_SZ);
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
MapDefaultKey(PHANDLE RealKey,
              HKEY Key)
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

    return Status;
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
        FullInfo = RtlAllocateHeap(RtlGetProcessHeap(),
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
            RtlFreeHeap(RtlGetProcessHeap(),
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

        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    FullInfo);
    }

Cleanup:
    ClosePredefKey(KeyHandle);

    return ErrorCode;
}

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
        ValueData.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
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
        RtlFreeHeap(RtlGetProcessHeap(), 0, ValueData.Buffer);
    }

    return ErrorCode;
}

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
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    ValueName.Buffer);
    }

    if (Data.Buffer != NULL)
    {
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    Data.Buffer);
    }

    return ErrorCode;
}

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
