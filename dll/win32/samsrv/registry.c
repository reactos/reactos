/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/registry.c
 * PURPOSE:         Registry helper functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

#include "samsrv.h"

#include <ndk/cmfuncs.h>
#include <ndk/obfuncs.h>

/* FUNCTIONS ***************************************************************/

static
BOOLEAN
IsStringType(ULONG Type)
{
    return (Type == REG_SZ) || (Type == REG_EXPAND_SZ) || (Type == REG_MULTI_SZ);
}


NTSTATUS
SampRegCloseKey(IN OUT PHANDLE KeyHandle)
{
    NTSTATUS Status;

    if (KeyHandle == NULL || *KeyHandle == NULL)
        return STATUS_SUCCESS;

    Status = NtClose(*KeyHandle);
    if (NT_SUCCESS(Status))
        *KeyHandle = NULL;

    return Status;
}


NTSTATUS
SampRegCreateKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    ULONG Disposition;

    RtlInitUnicodeString(&Name, KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               ParentKeyHandle,
                               NULL);

    /* Create the key */
    return ZwCreateKey(KeyHandle,
                       DesiredAccess,
                       &ObjectAttributes,
                       0,
                       NULL,
                       0,
                       &Disposition);
}


NTSTATUS
SampRegDeleteKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING SubKeyName;
    HANDLE TargetKey;
    NTSTATUS Status;

    RtlInitUnicodeString(&SubKeyName,
                         (LPWSTR)KeyName);
    InitializeObjectAttributes(&ObjectAttributes,
                               &SubKeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKeyHandle,
                               NULL);
    Status = NtOpenKey(&TargetKey,
                       DELETE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtDeleteKey(TargetKey);

    NtClose(TargetKey);

    return Status;
}


NTSTATUS
SampRegEnumerateSubKey(IN HANDLE KeyHandle,
                       IN ULONG Index,
                       IN ULONG Length,
                       OUT LPWSTR Buffer)
{
    PKEY_BASIC_INFORMATION KeyInfo = NULL;
    ULONG BufferLength = 0;
    ULONG ReturnedLength;
    NTSTATUS Status;

    /* Check if we have a name */
    if (Length)
    {
        /* Allocate a buffer for it */
        BufferLength = sizeof(KEY_BASIC_INFORMATION) + Length * sizeof(WCHAR);

        KeyInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
        if (KeyInfo == NULL)
            return STATUS_NO_MEMORY;
    }

    /* Enumerate the key */
    Status = ZwEnumerateKey(KeyHandle,
                            Index,
                            KeyBasicInformation,
                            KeyInfo,
                            BufferLength,
                            &ReturnedLength);
    if (NT_SUCCESS(Status))
    {
        /* Check if the name fits */
        if (KeyInfo->NameLength < (Length * sizeof(WCHAR)))
        {
            /* Copy it */
            RtlMoveMemory(Buffer,
                          KeyInfo->Name,
                          KeyInfo->NameLength);

            /* Terminate the string */
            Buffer[KeyInfo->NameLength / sizeof(WCHAR)] = 0;
        }
        else
        {
            /* Otherwise, we ran out of buffer space */
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }

    /* Free the buffer and return status */
    if (KeyInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, KeyInfo);

    return Status;
}


NTSTATUS
SampRegOpenKey(IN HANDLE ParentKeyHandle,
               IN LPCWSTR KeyName,
               IN ACCESS_MASK DesiredAccess,
               OUT PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               ParentKeyHandle,
                               NULL);

    return NtOpenKey(KeyHandle,
                     DesiredAccess,
                     &ObjectAttributes);
}


NTSTATUS
SampRegQueryKeyInfo(IN HANDLE KeyHandle,
                    OUT PULONG SubKeyCount,
                    OUT PULONG ValueCount)
{
    KEY_FULL_INFORMATION FullInfoBuffer;
    ULONG Length;
    NTSTATUS Status;

    FullInfoBuffer.ClassLength = 0;
    FullInfoBuffer.ClassOffset = FIELD_OFFSET(KEY_FULL_INFORMATION, Class);

    Status = NtQueryKey(KeyHandle,
                        KeyFullInformation,
                        &FullInfoBuffer,
                        sizeof(KEY_FULL_INFORMATION),
                        &Length);
    TRACE("NtQueryKey() returned status 0x%08lX\n", Status);
    if (!NT_SUCCESS(Status))
        return Status;

    if (SubKeyCount != NULL)
        *SubKeyCount = FullInfoBuffer.SubKeys;

    if (ValueCount != NULL)
        *ValueCount = FullInfoBuffer.Values;

    return Status;
}


NTSTATUS
SampRegDeleteValue(IN HANDLE KeyHandle,
                   IN LPCWSTR ValueName)
{
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name,
                         ValueName);

    return NtDeleteValueKey(KeyHandle,
                            &Name);
}


NTSTATUS
SampRegEnumerateValue(IN HANDLE KeyHandle,
                      IN ULONG Index,
                      OUT LPWSTR Name,
                      IN OUT PULONG NameLength,
                      OUT PULONG Type OPTIONAL,
                      OUT PVOID Data OPTIONAL,
                      IN OUT PULONG DataLength OPTIONAL)
{
    PKEY_VALUE_FULL_INFORMATION ValueInfo = NULL;
    ULONG BufferLength = 0;
    ULONG ReturnedLength;
    NTSTATUS Status;

    TRACE("Index: %lu\n", Index);

    /* Calculate the required buffer length */
    BufferLength = FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name);
    BufferLength += (MAX_PATH + 1) * sizeof(WCHAR);
    if (Data != NULL)
        BufferLength += *DataLength;

    /* Allocate the value buffer */
    ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (ValueInfo == NULL)
        return STATUS_NO_MEMORY;

    /* Enumerate the value*/
    Status = ZwEnumerateValueKey(KeyHandle,
                                 Index,
                                 KeyValueFullInformation,
                                 ValueInfo,
                                 BufferLength,
                                 &ReturnedLength);
    if (NT_SUCCESS(Status))
    {
        if (Name != NULL)
        {
            /* Check if the name fits */
            if (ValueInfo->NameLength < (*NameLength * sizeof(WCHAR)))
            {
                /* Copy it */
                RtlMoveMemory(Name,
                              ValueInfo->Name,
                              ValueInfo->NameLength);

                /* Terminate the string */
                Name[ValueInfo->NameLength / sizeof(WCHAR)] = 0;
            }
            else
            {
                /* Otherwise, we ran out of buffer space */
                Status = STATUS_BUFFER_OVERFLOW;
                goto done;
            }
        }

        if (Data != NULL)
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
                        *ptr = 0;
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
        if (Type != NULL)
            *Type = ValueInfo->Type;

        if (NameLength != NULL)
            *NameLength = ValueInfo->NameLength;

        if (DataLength != NULL)
            *DataLength = ValueInfo->DataLength;
    }

    /* Free the buffer and return status */
    if (ValueInfo)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);

    return Status;
}


NTSTATUS
SampRegQueryValue(IN HANDLE KeyHandle,
                  IN LPCWSTR ValueName,
                  OUT PULONG Type OPTIONAL,
                  OUT PVOID Data OPTIONAL,
                  IN OUT PULONG DataLength OPTIONAL)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING Name;
    ULONG BufferLength = 0;
    NTSTATUS Status;

    RtlInitUnicodeString(&Name,
                         ValueName);

    if (DataLength != NULL)
        BufferLength = *DataLength;

    BufferLength += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    /* Allocate memory for the value */
    ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (ValueInfo == NULL)
        return STATUS_NO_MEMORY;

    /* Query the value */
    Status = ZwQueryValueKey(KeyHandle,
                             &Name,
                             KeyValuePartialInformation,
                             ValueInfo,
                             BufferLength,
                             &BufferLength);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
    {
        if (Type != NULL)
            *Type = ValueInfo->Type;

        if (DataLength != NULL)
            *DataLength = ValueInfo->DataLength;
    }

    /* Check if the caller wanted data back, and we got it */
    if ((NT_SUCCESS(Status)) && (Data != NULL))
    {
        /* Copy it */
        RtlMoveMemory(Data,
                      ValueInfo->Data,
                      ValueInfo->DataLength);

        /* if the type is REG_SZ and data is not 0-terminated
         * and there is enough space in the buffer NT appends a \0 */
        if (IsStringType(ValueInfo->Type) &&
            ValueInfo->DataLength <= *DataLength - sizeof(WCHAR))
        {
            WCHAR *ptr = (WCHAR *)((ULONG_PTR)Data + ValueInfo->DataLength);
            if ((ptr > (WCHAR *)Data) && ptr[-1])
                *ptr = 0;
        }
    }

    /* Free the memory and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);

    if ((Data == NULL) && (Status == STATUS_BUFFER_OVERFLOW))
        Status = STATUS_SUCCESS;

    return Status;
}


NTSTATUS
SampRegSetValue(HANDLE KeyHandle,
                LPCWSTR ValueName,
                ULONG Type,
                LPVOID Data,
                ULONG DataLength)
{
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name,
                         ValueName);

    return ZwSetValueKey(KeyHandle,
                         &Name,
                         0,
                         Type,
                         Data,
                         DataLength);
}
