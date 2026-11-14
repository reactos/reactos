/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     uACPI-NT Registry handling code
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <debug.h>

NTSTATUS
NTAPI
UacpiNtRegOpenKey(
    _In_opt_ HANDLE ParentKeyHandle,
    _In_z_ PCWSTR KeyName,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE KeyHandle)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, KeyName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               ParentKeyHandle,
                               NULL);
    return ZwOpenKey(KeyHandle,
                     DesiredAccess,
                     &ObjectAttributes);
}

NTSTATUS
NTAPI
UacpiNtRegQueryValue(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Out_opt_ PULONG Type,
    _Out_writes_bytes_to_opt_(*DataLength, *DataLength) PVOID Data,
    _Inout_opt_ PULONG DataLength)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    KEY_VALUE_PARTIAL_INFORMATION Probe;
    ULONG OriginalDataLength = 0;
    UNICODE_STRING Name;
    ULONG InfoLength;
    ULONG ResultLength = 0;
    NTSTATUS Status;

    RtlInitUnicodeString(&Name, ValueName);

    if (DataLength)
        OriginalDataLength = *DataLength;

    /* Without a buffer only the type and size are returned */
    if (Data == NULL || OriginalDataLength == 0)
    {
        Status = ZwQueryValueKey(KeyHandle,
                                 &Name,
                                 KeyValuePartialInformation,
                                 &Probe,
                                 sizeof(Probe),
                                 &ResultLength);
        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
            return Status;

        if (Type)
            *Type = Probe.Type;
        if (DataLength)
            *DataLength = Probe.DataLength;
        return STATUS_SUCCESS;
    }

    InfoLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + OriginalDataLength;
    ValueInfo = ExAllocatePoolWithTag(PagedPool, InfoLength, UACPINT_POOL_TAG);
    if (!ValueInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = ZwQueryValueKey(KeyHandle,
                             &Name,
                             KeyValuePartialInformation,
                             ValueInfo,
                             InfoLength,
                             &ResultLength);
    if (NT_SUCCESS(Status))
    {
        if (Type)
            *Type = ValueInfo->Type;

        RtlCopyMemory(Data, ValueInfo->Data, ValueInfo->DataLength);
        *DataLength = ValueInfo->DataLength;

        /* Terminate strings the writer left open when there is room */
        if ((ValueInfo->Type == REG_SZ ||
             ValueInfo->Type == REG_EXPAND_SZ ||
             ValueInfo->Type == REG_MULTI_SZ) &&
            ValueInfo->DataLength + sizeof(WCHAR) <= OriginalDataLength &&
            ValueInfo->DataLength >= sizeof(WCHAR))
        {
            PWCHAR End = (PWCHAR)((PUCHAR)Data + ValueInfo->DataLength);

            if (End[-1] != UNICODE_NULL)
                *End = UNICODE_NULL;
        }
    }
    else if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Report the size that would have fit */
        *DataLength = ValueInfo->DataLength;
    }

    ExFreePoolWithTag(ValueInfo, UACPINT_POOL_TAG);
    return Status;
}

NTSTATUS
NTAPI
UacpiNtRegQueryUlong(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Inout_ PULONG Value)
{
    ULONG Type;
    ULONG Data;
    ULONG Length = sizeof(Data);
    NTSTATUS Status;

    Status = UacpiNtRegQueryValue(KeyHandle, ValueName, &Type, &Data, &Length);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Type != REG_DWORD || Length != sizeof(Data))
        return STATUS_OBJECT_TYPE_MISMATCH;

    *Value = Data;
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
UacpiNtRegQueryAnsiString(
    _In_ HANDLE KeyHandle,
    _In_z_ PCWSTR ValueName,
    _Out_writes_z_(BufferSize) PCHAR Buffer,
    _In_ ULONG BufferSize)
{
    WCHAR Wide[128];
    ULONG Length = sizeof(Wide) - sizeof(WCHAR);
    ULONG Type;
    ULONG Chars;
    ULONG i;

    if (BufferSize == 0)
        return FALSE;

    Buffer[0] = ANSI_NULL;

    if (!NT_SUCCESS(UacpiNtRegQueryValue(KeyHandle, ValueName, &Type, Wide, &Length)) ||
        Type != REG_SZ)
    {
        return FALSE;
    }

    /* Non ASCII characters become '?' since the result feeds PnP IDs */
    Chars = Length / sizeof(WCHAR);
    for (i = 0; i < Chars && i + 1 < BufferSize && Wide[i] != UNICODE_NULL; i++)
        Buffer[i] = (Wide[i] < 0x80) ? (CHAR)Wide[i] : '?';

    Buffer[i] = ANSI_NULL;
    return TRUE;
}
