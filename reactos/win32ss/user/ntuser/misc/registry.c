/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          Registry loading and storing
 * FILE:             subsystem/win32/win32k/misc/registry.c
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
RegWriteSZ(HKEY hkey, PWSTR pwszValue, PWSTR pwszData)
{
    UNICODE_STRING ustrValue;
    UNICODE_STRING ustrData;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    RtlInitUnicodeString(&ustrData, pwszData);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_SZ, &ustrData, ustrData.Length + sizeof(WCHAR));
}

VOID
NTAPI
RegWriteDWORD(HKEY hkey, PWSTR pwszValue, DWORD dwData)
{
    UNICODE_STRING ustrValue;

    RtlInitUnicodeString(&ustrValue, pwszValue);
    ZwSetValueKey(hkey, &ustrValue, 0, REG_DWORD, &dwData, sizeof(DWORD));
}

BOOL
NTAPI
RegReadDWORD(HKEY hkey, PWSTR pwszValue, PDWORD pdwData)
{
    NTSTATUS Status;
    ULONG cbSize = sizeof(DWORD);
    Status = RegQueryValue(hkey, pwszValue, REG_DWORD, pdwData, &cbSize);
    return NT_SUCCESS(Status);
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

BOOL
NTAPI
RegWriteUserSetting(
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN ULONG ulType,
    OUT PVOID pvData,
    IN ULONG cbDataSize)
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

    Status = ZwSetValueKey(hkey, &usValueName, 0, ulType, pvData, cbDataSize);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to write reg key '%S' value '%S', Status = %lx\n",
                pwszKeyName, pwszValueName, Status);
    }

    /* Cleanup */
    ZwClose(hkey);

    return NT_SUCCESS(Status);
}

