/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          Registry loading and storing
 * FILE:             win32ss/user/ntuser/misc/registry.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
RegOpenKey(
    LPCWSTR pwszKeyName,
    PHKEY phkey)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ustrKeyName;
    HKEY hkey;

    /* Initialize the key name */
    RtlInitUnicodeString(&ustrKeyName, pwszKeyName);

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &ustrKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey((PHANDLE)&hkey, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        *phkey = hkey;
    }

    return Status;
}

NTSTATUS
NTAPI
RegQueryValue(
    IN HKEY hkey,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN OUT PULONG pcbValue)
{
    NTSTATUS Status;
    UNICODE_STRING ustrValueName;
    BYTE ajBuffer[100];
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;
    ULONG cbInfoSize, cbDataSize;

    /* Check if the local buffer is sufficient */
    cbInfoSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[*pcbValue]);
    if (cbInfoSize <= sizeof(ajBuffer))
    {
        pInfo = (PVOID)ajBuffer;
    }
    else
    {
        /* It's not, allocate a sufficient buffer */
        pInfo = ExAllocatePoolWithTag(PagedPool, cbInfoSize, TAG_TEMP);
        if (!pInfo)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Query the value */
    RtlInitUnicodeString(&ustrValueName, pwszValueName);
    Status = ZwQueryValueKey(hkey,
                             &ustrValueName,
                             KeyValuePartialInformation,
                             (PVOID)pInfo,
                             cbInfoSize,
                             &cbInfoSize);

    /* Note: STATUS_BUFFER_OVERFLOW is not a success */
    if (NT_SUCCESS(Status))
    {
        cbDataSize = pInfo->DataLength;

        /* Did we get the right type */
        if (pInfo->Type != ulType)
        {
            Status = STATUS_OBJECT_TYPE_MISMATCH;
        }
        else if (cbDataSize > *pcbValue)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            /* Copy the contents to the caller */
            RtlCopyMemory(pvData, pInfo->Data, cbDataSize);
        }
    }
    else if ((Status == STATUS_BUFFER_OVERFLOW) || (Status == STATUS_BUFFER_TOO_SMALL))
    {
        _PRAGMA_WARNING_SUPPRESS(6102); /* cbInfoSize is initialized here! */
        cbDataSize = cbInfoSize - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);
    }
    else
    {
        cbDataSize = 0;
    }

    /* Return the data size to the caller */
    *pcbValue = cbDataSize;

    /* Cleanup */
    if (pInfo != (PVOID)ajBuffer)
        ExFreePoolWithTag(pInfo, TAG_TEMP);

    return Status;
}

VOID
NTAPI
RegWriteSZ(HKEY hkey, PCWSTR pwszValue, PWSTR pwszData)
{
    UNICODE_STRING ustrValue;
    UNICODE_STRING ustrData;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    RtlInitUnicodeString(&ustrData, pwszData);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_SZ, &ustrData, ustrData.Length + sizeof(WCHAR));
}

VOID
NTAPI
RegWriteDWORD(HKEY hkey, PCWSTR pwszValue, DWORD dwData)
{
    UNICODE_STRING ustrValue;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_DWORD, &dwData, sizeof(DWORD));
}

BOOL
NTAPI
RegReadDWORD(HKEY hkey, PCWSTR pwszValue, PDWORD pdwData)
{
    NTSTATUS Status;
    ULONG cbSize = sizeof(DWORD);
    Status = RegQueryValue(hkey, pwszValue, REG_DWORD, pdwData, &cbSize);
    return NT_SUCCESS(Status);
}

NTSTATUS
NTAPI
RegOpenSectionKey(
    LPCWSTR pszSection,
    PHKEY phkey)
{
    WCHAR szKey[MAX_PATH] =
        L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\";

    RtlStringCchCatW(szKey, _countof(szKey), pszSection);
    return RegOpenKey(szKey, phkey);
}

DWORD
NTAPI
RegGetSectionDWORD(LPCWSTR pszSection, PCWSTR pszValue, DWORD dwDefault)
{
    HKEY hKey;
    DWORD dwValue;

    if (NT_ERROR(RegOpenSectionKey(pszSection, &hKey)))
        return dwDefault;

    if (!RegReadDWORD(hKey, pszValue, &dwValue))
        dwValue = dwDefault;

    ZwClose(hKey);
    return dwValue;
}

_Success_(return!=FALSE)
BOOL
NTAPI
RegReadUserSetting(
    _In_z_ PCWSTR pwszKeyName,
    _In_z_ PCWSTR pwszValueName,
    _In_ ULONG ulType,
    _Out_writes_(cbDataSize) _When_(ulType == REG_SZ, _Post_z_) PVOID pvData,
    _In_ ULONG cbDataSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING usCurrentUserKey, usKeyName, usValueName;
    WCHAR awcBuffer[MAX_PATH];
    HKEY hkey;
    PKEY_VALUE_PARTIAL_INFORMATION pInfo;
    ULONG cbInfoSize, cbReqSize;

    /* Get the path of the current user's profile */
    Status = RtlFormatCurrentUserKeyPath(&usCurrentUserKey);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Initialize empty key name */
    RtlInitEmptyUnicodeString(&usKeyName, awcBuffer, sizeof(awcBuffer));

    /* Append the current user key name */
    Status = RtlAppendUnicodeStringToString(&usKeyName, &usCurrentUserKey);

    /* Free the current user key name */
    RtlFreeUnicodeString(&usCurrentUserKey);

    /* Check for success */
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Append a '\', we can trust in enough space left. */
    usKeyName.Buffer[usKeyName.Length / sizeof(WCHAR)] = '\\';
    usKeyName.Length += sizeof(WCHAR);

    /* Append the subkey name */
    Status = RtlAppendUnicodeToString(&usKeyName, pwszKeyName);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &usKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey((PHANDLE)&hkey, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Check if the local buffer is sufficient */
    cbInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + cbDataSize;
    if (cbInfoSize <= sizeof(awcBuffer))
    {
        pInfo = (PVOID)awcBuffer;
    }
    else
    {
        /* It's not, allocate a sufficient buffer */
        pInfo = ExAllocatePoolWithTag(PagedPool, cbInfoSize, TAG_TEMP);
        if (!pInfo)
        {
            ZwClose(hkey);
            return FALSE;
        }
    }

    /* Query the value */
    RtlInitUnicodeString(&usValueName, pwszValueName);
    Status = ZwQueryValueKey(hkey,
                             &usValueName,
                             KeyValuePartialInformation,
                             (PVOID)pInfo,
                             cbInfoSize,
                             &cbReqSize);
    if (NT_SUCCESS(Status))
    {
        /* Did we get the right type */
        if (pInfo->Type == ulType)
        {
            /* Copy the contents to the caller */
            RtlCopyMemory(pvData, pInfo->Data, cbDataSize);
        }
    }

    /* Cleanup */
    ZwClose(hkey);
    if (pInfo != (PVOID)awcBuffer)
        ExFreePoolWithTag(pInfo, TAG_TEMP);

    return NT_SUCCESS(Status);
}

_Success_(return != FALSE)
BOOL
NTAPI
RegWriteUserSetting(
    _In_z_ PCWSTR pwszKeyName,
    _In_z_ PCWSTR pwszValueName,
    _In_ ULONG ulType,
    _In_reads_bytes_(cjDataSize) const VOID *pvData,
    _In_ ULONG cbDataSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING usCurrentUserKey, usKeyName, usValueName;
    WCHAR awcBuffer[MAX_PATH];
    HKEY hkey;

    // FIXME: Logged in user versus current process user?
    /* Get the path of the current user's profile */
    Status = RtlFormatCurrentUserKeyPath(&usCurrentUserKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlFormatCurrentUserKeyPath failed\n");
        return FALSE;
    }

    /* Initialize empty key name */
    RtlInitEmptyUnicodeString(&usKeyName, awcBuffer, sizeof(awcBuffer));

    /* Append the current user key name */
    Status = RtlAppendUnicodeStringToString(&usKeyName, &usCurrentUserKey);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Free the current user key name */
    RtlFreeUnicodeString(&usCurrentUserKey);

    /* Append a '\', we can trust in enough space left. */
    usKeyName.Buffer[usKeyName.Length / sizeof(WCHAR)] = '\\';
    usKeyName.Length += sizeof(WCHAR);

    /* Append the subkey name */
    Status = RtlAppendUnicodeToString(&usKeyName, pwszKeyName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlAppendUnicodeToString failed with Status=0x%lx, buf:%u,%u\n",
                Status, usKeyName.Length, usKeyName.MaximumLength);
        return FALSE;
    }

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &usKeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Open or create the key */
    Status = ZwCreateKey((PHANDLE)&hkey,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create key: 0x%x\n", Status);
        return FALSE;
    }

    /* Initialize the value name string */
    RtlInitUnicodeString(&usValueName, pwszValueName);

    Status = ZwSetValueKey(hkey, &usValueName, 0, ulType, (PVOID)pvData, cbDataSize);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write reg key '%S' value '%S', Status = %lx\n",
                pwszKeyName, pwszValueName, Status);
    }

    /* Cleanup */
    ZwClose(hkey);

    return NT_SUCCESS(Status);
}

static inline BOOL
IsStringType(ULONG Type)
{
    return (Type == REG_SZ) || (Type == REG_EXPAND_SZ) || (Type == REG_MULTI_SZ);
}

NTSTATUS
NTAPI
RegEnumValueW(
    _In_ HKEY hKey,
    _In_ ULONG Index,
    _Out_opt_ LPWSTR Name,
    _Out_opt_ PULONG NameLength,
    _Out_opt_ PULONG Type,
    _Out_opt_ PVOID Data,
    _Out_opt_ PULONG DataLength)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    ULONG BufferLength = 0;
    ULONG ReturnedLength;
    NTSTATUS Status;

    /* Calculate the required buffer length */
    BufferLength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
    BufferLength += (MAX_PATH + 1) * sizeof(WCHAR);
    if (Data != NULL)
        BufferLength += *DataLength;

    /* Allocate the value buffer */
    ValueInfo = ExAllocatePoolWithTag(PagedPool, BufferLength, TAG_TEMP);
    if (ValueInfo == NULL)
        return STATUS_NO_MEMORY;

    /* Enumerate the value*/
    Status = ZwEnumerateValueKey(hKey,
                                 Index,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 BufferLength,
                                 &ReturnedLength);
    if (NT_SUCCESS(Status))
    {
        if (Name)
        {
            /* Check if the name fits */
            if ((ValueInfo->NameLength + sizeof(WCHAR)) <= (*NameLength * sizeof(WCHAR)))
            {
                /* Copy it */
                RtlMoveMemory(Name, ValueInfo->Name, ValueInfo->NameLength);

                /* Terminate the string */
                Name[ValueInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            }
            else
            {
                /* Otherwise, we ran out of buffer space */
                Status = STATUS_BUFFER_OVERFLOW;
                goto done;
            }
        }

        if (Data)
        {
            /* Check if the data fits */
            if (ValueInfo->DataLength <= *DataLength)
            {
                /* Copy it */
                RtlMoveMemory(Data,
                              (PVOID)((ULONG_PTR)ValueInfo + ValueInfo->DataOffset),
                              ValueInfo->DataLength);

                /* if the type is REG_SZ and data is not 0-terminated
                 * and there is enough space in the buffer NT appends a \0 */
                if (IsStringType(ValueInfo->Type) &&
                    ValueInfo->DataLength <= *DataLength - sizeof(WCHAR))
                {
                    WCHAR *ptr = (WCHAR *)((ULONG_PTR)Data + ValueInfo->DataLength);
                    if ((ptr > (WCHAR *)Data) && ptr[-1])
                        *ptr = UNICODE_NULL;
                }
            }
            else
            {
                Status = STATUS_BUFFER_OVERFLOW;
                goto done;
            }
        }
    }

done:
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
    {
        if (Type)
            *Type = ValueInfo->Type;

        if (NameLength)
            *NameLength = ValueInfo->NameLength;

        if (DataLength)
            *DataLength = ValueInfo->DataLength;
    }

    /* Free the buffer and return status */
    if (ValueInfo)
        ExFreePoolWithTag(ValueInfo, TAG_TEMP);

    return Status;
}

NTSTATUS NTAPI
RegDeleteValueW(_In_ HKEY hKey, _In_ LPCWSTR pszValueName)
{
    UNICODE_STRING ustrName;
    RtlInitUnicodeString(&ustrName, pszValueName);
    return ZwDeleteValueKey(hKey, &ustrName);
}
