/*
 * COPYRIGHT:        GPL, see COPYING in the top level directory
 * PROJECT:          ReactOS win32 kernel mode subsystem server
 * PURPOSE:          Registry loading and storing
 * FILE:             subsystem/win32/win32k/misc/registry.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

BOOL
NTAPI
RegReadUserSetting(
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
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the key */
    Status = ZwOpenKey(&hkey, KEY_READ, &ObjectAttributes);
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

    // FIXME: logged in user versus current process user?
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
        DPRINT1("RtlAppendUnicodeToString failed with Status=%lx, buf:%d,%d\n", Status, usKeyName.Length, usKeyName.MaximumLength);
        return FALSE;
    }

    /* Initialize object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &usKeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open or create the key */
    Status = ZwCreateKey(&hkey,
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

