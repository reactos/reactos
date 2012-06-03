/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Security Account Manager (SAM) Server
 * FILE:            reactos/dll/win32/samsrv/registry.c
 * PURPOSE:         Registry helper functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);

/* FUNCTIONS ***************************************************************/

NTSTATUS
SampRegCloseKey(IN HANDLE KeyHandle)
{
    return NtClose(KeyHandle);
}


NTSTATUS
SampRegCreateKey(IN HANDLE ParentKeyHandle,
                 IN LPCWSTR KeyName,
                 IN ACCESS_MASK DesiredAccess,
                 OUT HANDLE KeyHandle)
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
               OUT HANDLE KeyHandle)
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
SampRegSetValue(HANDLE KeyHandle,
                LPWSTR ValueName,
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


NTSTATUS
SampRegQueryValue(HANDLE KeyHandle,
                  LPWSTR ValueName,
                  PULONG Type OPTIONAL,
                  LPVOID Data OPTIONAL,
                  PULONG DataLength OPTIONAL)
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
    }

    /* Free the memory and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);

    if ((Data == NULL) && (Status == STATUS_BUFFER_OVERFLOW))
        Status = STATUS_SUCCESS;

    return Status;
}
