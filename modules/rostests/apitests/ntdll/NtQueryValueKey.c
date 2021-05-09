/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtQueryValueKey
 * COPYRIGHT:   Copyright 2020 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"
#include <winreg.h>

START_TEST(NtQueryValueKey)
{
    NTSTATUS Status;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion");
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"SystemRoot");
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 Info;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64 InfoUnaligned;
    ULONG InfoLength;
    ULONG ResultLength;
    const WCHAR *StringData;
    ULONG StringLength;

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        skip("Test key unavailable\n");
        return;
    }

    /* Specify zero-size buffer, get the length */
    ResultLength = 0x55555555;
    Status = NtQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformationAlign64,
                             NULL,
                             0,
                             &ResultLength);
    ok_hex(Status, STATUS_BUFFER_TOO_SMALL);
    ok(ResultLength > FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data) && ResultLength < 0x10000,
       "Invalid result length %lu\n", ResultLength);
    if (ResultLength < FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data))
    {
        skip("Result length %lu too small\n", ResultLength);
        goto Exit;
    }
    InfoLength = ResultLength;
    Info = RtlAllocateHeap(RtlGetProcessHeap(),
                           0,
                           InfoLength + 4);
    if (Info == NULL)
    {
        skip("Could not alloc %lu bytes\n", InfoLength);
        goto Exit;
    }

    /* Successful call */
    Status = NtQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformationAlign64,
                             Info,
                             InfoLength,
                             &ResultLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_int(ResultLength, InfoLength);

    ok_int(Info->Type, REG_SZ);
    StringLength = InfoLength - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION_ALIGN64, Data);
    ok_int(Info->DataLength, StringLength);

    StringData = (PWCHAR)Info->Data;
    ok(Info->DataLength >= 5 * sizeof(WCHAR), "DataLength %lu is too small for path\n", Info->DataLength);
    if (Info->DataLength >= 5 * sizeof(WCHAR))
    {
        trace("SystemRoot: %.*ls\n", (int)(Info->DataLength / sizeof(WCHAR) - 1), StringData);
        ok(StringData[0] >= 'A' && StringData[0] <= 'Z', "Data[0] = %x\n", StringData[0]);
        ok(StringData[1] == ':', "Data[1] = %x\n", StringData[1]);
        ok(StringData[2] == '\\', "Data[2] = %x\n", StringData[2]);
        ok(iswalnum(StringData[3]), "Data[3] = %x\n", StringData[3]);
        ok(StringData[Info->DataLength / sizeof(WCHAR) - 1] == UNICODE_NULL,
           "Data[%lu] = %x\n", Info->DataLength / sizeof(WCHAR) - 1, StringData[Info->DataLength / sizeof(WCHAR) - 1]);
    }

    /* If the buffer isn't 64 bit aligned, the data won't be, either */
    InfoUnaligned = (PVOID)((PUCHAR)Info + 4);
    Status = NtQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformationAlign64,
                             InfoUnaligned,
                             InfoLength,
                             &ResultLength);
    ok_hex(Status, STATUS_SUCCESS);
    ok_int(ResultLength, InfoLength);
    ok_int(InfoUnaligned->Type, REG_SZ);
    StringData = (PWCHAR)InfoUnaligned->Data;
    ok(InfoUnaligned->DataLength >= 2 * sizeof(WCHAR), "DataLength %lu is too small for path\n", InfoUnaligned->DataLength);
    if (InfoUnaligned->DataLength >= 2 * sizeof(WCHAR))
    {
        ok(StringData[1] == ':', "Data[1] = %x\n", StringData[1]);
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, Info);

Exit:
    Status = NtClose(KeyHandle);
    ok_hex(Status, STATUS_SUCCESS);
}
