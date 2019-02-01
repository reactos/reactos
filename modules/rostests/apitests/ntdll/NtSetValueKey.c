/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Test for NtSetValueKey
 * COPYRIGHT:   Copyright 2016-2019 Thomas Faber (thomas.faber@reactos.org)
 */

#include "precomp.h"

#include <winreg.h>

START_TEST(NtSetValueKey)
{
    NTSTATUS Status;
    HANDLE ParentKeyHandle;
    HANDLE KeyHandle;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"SOFTWARE\\ntdll-apitest-NtSetValueKey");
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ValueName;
    WCHAR Default[] = L"Default";
    WCHAR Hello[] = L"Hello";
    WCHAR Empty[] = L"";
    NTSTATUS QueryStatus;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    ULONG PartialInfoLength;
    ULONG ResultLength;
    ULONG DataLength;
    PWCHAR LargeBuffer;
    ULONG LargeBufferLength;
    const struct
    {
        ULONG Type;
        PVOID Data;
        ULONG DataSize;
        NTSTATUS StatusExisting;
        NTSTATUS StatusNew;
        NTSTATUS StatusExisting2;
        NTSTATUS StatusNew2;
    } Tests[] =
    {
        { REG_NONE,   NULL,                 0,             STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Empty REG_NONE value */
        { REG_SZ,     Hello,                sizeof(Hello), STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Regular string */
        { REG_SZ,     Empty,                sizeof(Empty), STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Empty string */
        { REG_SZ,     NULL,                 0,             STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Zero length */
        { REG_SZ,     Hello,                0,             STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Zero length, non-null data */
        { REG_SZ,     (PVOID)(LONG_PTR)-4,  0,             STATUS_SUCCESS,                STATUS_SUCCESS                }, /* Zero length, kernel data */
        { REG_SZ,     NULL,                 1,             STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION       }, /* Non-zero length (odd), null data */
        { REG_SZ,     NULL,                 2,             STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION       }, /* Non-zero length (even), null data */
        { REG_SZ,     NULL,                 4,             STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION       }, /* CM_KEY_VALUE_SMALL, null data */
        { REG_SZ,     NULL,                 5,             STATUS_INVALID_PARAMETER,      STATUS_ACCESS_VIOLATION,         /* CM_KEY_VALUE_SMALL+1, null data */
                                                           STATUS_ACCESS_VIOLATION,       STATUS_INSUFFICIENT_RESOURCES },        /* win7 */
        { REG_SZ,     NULL,                 6,             STATUS_INVALID_PARAMETER,      STATUS_ACCESS_VIOLATION,         /* CM_KEY_VALUE_SMALL+2, null data */
                                                           STATUS_ACCESS_VIOLATION,       STATUS_INSUFFICIENT_RESOURCES },        /* win7 */
        { REG_SZ,     NULL,                 0x7fff0000,    STATUS_INVALID_PARAMETER,      STATUS_INSUFFICIENT_RESOURCES,   /* MI_USER_PROBE_ADDRESS, null data */
                                                           STATUS_INSUFFICIENT_RESOURCES, STATUS_INSUFFICIENT_RESOURCES },        /* win7 */
        { REG_SZ,     NULL,                 0x7fff0001,    STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION,         /* MI_USER_PROBE_ADDRESS+1, null data */
                                                           STATUS_INSUFFICIENT_RESOURCES, STATUS_INSUFFICIENT_RESOURCES },        /* win7 */
        { REG_SZ,     NULL,                 0x7fffffff,    STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION,         /* <2GB, null data */
                                                           STATUS_INVALID_PARAMETER,      STATUS_INVALID_PARAMETER      },        /* win7 */
        { REG_SZ,     NULL,                 0x80000000,    STATUS_ACCESS_VIOLATION,       STATUS_ACCESS_VIOLATION,         /* 2GB, null data */
                                                           STATUS_INVALID_PARAMETER,      STATUS_INVALID_PARAMETER      },        /* win7 */
        { REG_BINARY, NULL,                 5,             STATUS_INVALID_PARAMETER,      STATUS_ACCESS_VIOLATION,         /* ROSTESTS-200 */
                                                           STATUS_ACCESS_VIOLATION,       STATUS_INSUFFICIENT_RESOURCES },        /* win7 */
    };
    ULONG i;

    Status = RtlOpenCurrentUser(READ_CONTROL, &ParentKeyHandle);
    ok(Status == STATUS_SUCCESS, "RtlOpenCurrentUser returned %lx\n", Status);
    if (!NT_SUCCESS(Status))
    {
        skip("No user key handle\n");
        return;
    }

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKeyHandle,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_QUERY_VALUE | KEY_SET_VALUE | DELETE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE,
                         NULL);
    ok(Status == STATUS_SUCCESS, "NtCreateKey returned %lx\n", Status);
    if (!NT_SUCCESS(Status))
    {
        NtClose(ParentKeyHandle);
        skip("No key handle\n");
        return;
    }

    LargeBufferLength = 0x20000;
    LargeBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, LargeBufferLength);

    PartialInfoLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[LargeBufferLength]);
    PartialInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, PartialInfoLength);

    if (LargeBuffer == NULL || PartialInfo == NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, LargeBuffer);
        RtlFreeHeap(GetProcessHeap(), 0, PartialInfo);
        NtDeleteKey(KeyHandle);
        NtClose(KeyHandle);
        NtClose(ParentKeyHandle);
        skip("Could not allocate buffers\n");
        return;
    }

    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        /*
         * Existing value
         */
        /* Make sure it exists */
        RtlInitUnicodeString(&ValueName, L"ExistingValue");
        Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_SZ, Default, sizeof(Default));
        ok(Status == STATUS_SUCCESS, "[%lu] NtSetValueKey failed with %lx", i, Status);

        /* Set it */
        Status = NtSetValueKey(KeyHandle, &ValueName, 0, Tests[i].Type, Tests[i].Data, Tests[i].DataSize);
        if (Status == Tests[i].StatusExisting2)
            ok(Status == Tests[i].StatusExisting || Status == Tests[i].StatusExisting2, "[%lu, %p, %lu] NtSetValueKey returned %lx, expected %lx or %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, Status, Tests[i].StatusExisting, Tests[i].StatusExisting2);
        else
            ok(Status == Tests[i].StatusExisting, "[%lu, %p, %lu] NtSetValueKey returned %lx, expected %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, Status, Tests[i].StatusExisting);

        /* Check it */
        RtlZeroMemory(PartialInfo, PartialInfoLength);
        QueryStatus = NtQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation, PartialInfo, PartialInfoLength, &ResultLength);
        ok(QueryStatus == STATUS_SUCCESS, "[%lu, %p, %lu] NtQueryValueKey failed with %lx\n",
           Tests[i].Type, Tests[i].Data, Tests[i].DataSize, QueryStatus);
        if (NT_SUCCESS(QueryStatus))
        {
            if (NT_SUCCESS(Status))
            {
                ok(PartialInfo->TitleIndex == 0, "[%lu, %p, %lu] TitleIndex = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->TitleIndex);
                ok(PartialInfo->Type == Tests[i].Type, "[%lu, %p, %lu] Type = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->Type);
                ok(PartialInfo->DataLength == Tests[i].DataSize, "[%lu, %p, %lu] DataLength = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->DataLength);
                ok(!memcmp(PartialInfo->Data, Tests[i].Data, Tests[i].DataSize), "[%lu, %p, %lu] Data does not match set value\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize);
            }
            else
            {
                ok(PartialInfo->TitleIndex == 0, "[%lu, %p, %lu] TitleIndex = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->TitleIndex);
                ok(PartialInfo->Type == REG_SZ, "[%lu, %p, %lu] Type = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->Type);
                ok(PartialInfo->DataLength == sizeof(Default), "[%lu, %p, %lu] DataLength = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->DataLength);
                ok(!memcmp(PartialInfo->Data, Default, sizeof(Default)), "[%lu, %p, %lu] Data does not match default\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize);
            }
        }

        /*
         * New value
         */
        /* Make sure it doesn't exist */
        RtlInitUnicodeString(&ValueName, L"NewValue");
        Status = NtDeleteValueKey(KeyHandle, &ValueName);
        ok(Status == STATUS_SUCCESS || Status == STATUS_OBJECT_NAME_NOT_FOUND,
           "[%lu] NtDeleteValueKey failed with %lx", i, Status);

        /* Set it */
        Status = NtSetValueKey(KeyHandle, &ValueName, 0, Tests[i].Type, Tests[i].Data, Tests[i].DataSize);
        if (Tests[i].StatusNew2)
            ok(Status == Tests[i].StatusNew || Status == Tests[i].StatusNew2, "[%lu, %p, %lu] NtSetValueKey returned %lx, expected %lx or %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, Status, Tests[i].StatusNew, Tests[i].StatusNew2);
        else
            ok(Status == Tests[i].StatusNew, "[%lu, %p, %lu] NtSetValueKey returned %lx, expected %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, Status, Tests[i].StatusNew);

        /* Check it */
        RtlZeroMemory(PartialInfo, PartialInfoLength);
        QueryStatus = NtQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation, PartialInfo, PartialInfoLength, &ResultLength);
        if (NT_SUCCESS(Status))
        {
            ok(QueryStatus == STATUS_SUCCESS, "[%lu, %p, %lu] NtQueryValueKey failed with %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, QueryStatus);
            if (NT_SUCCESS(QueryStatus))
            {
                ok(PartialInfo->TitleIndex == 0, "[%lu, %p, %lu] TitleIndex = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->TitleIndex);
                ok(PartialInfo->Type == Tests[i].Type, "[%lu, %p, %lu] Type = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->Type);
                ok(PartialInfo->DataLength == Tests[i].DataSize, "[%lu, %p, %lu] DataLength = %lu\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize, PartialInfo->DataLength);
                ok(!memcmp(PartialInfo->Data, Tests[i].Data, Tests[i].DataSize), "[%lu, %p, %lu] Data does not match set value\n",
                   Tests[i].Type, Tests[i].Data, Tests[i].DataSize);
            }
        }
        else
        {
            ok(QueryStatus == STATUS_OBJECT_NAME_NOT_FOUND, "[%lu, %p, %lu] QueryStatus = %lx\n",
               Tests[i].Type, Tests[i].Data, Tests[i].DataSize, QueryStatus);
        }
    }

    /* String value larger than MAXUSHORT */
    {
    const ULONG DataLengths[] = { 0x10000, 0x10002, 0x20000 };

    RtlInitUnicodeString(&ValueName, L"ExistingValue");
    for (i = 0; i < RTL_NUMBER_OF(DataLengths); i++)
    {
        DataLength = DataLengths[i];
        RtlFillMemoryUlong(LargeBuffer, DataLength, '\0B\0A');
        LargeBuffer[DataLength / sizeof(WCHAR) - 2] = L'C';
        LargeBuffer[DataLength / sizeof(WCHAR) - 1] = UNICODE_NULL;
        Status = NtSetValueKey(KeyHandle, &ValueName, 0, REG_SZ, LargeBuffer, DataLength);
        ok(Status == STATUS_SUCCESS, "[0x%lx] NtSetValueKey failed with %lx", DataLength, Status);

        RtlZeroMemory(PartialInfo, PartialInfoLength);
        Status = NtQueryValueKey(KeyHandle, &ValueName, KeyValuePartialInformation, PartialInfo, PartialInfoLength, &ResultLength);
        ok(Status == STATUS_SUCCESS, "[0x%lx] NtQueryValueKey failed with %lx\n", DataLength, Status);
        ok(PartialInfo->TitleIndex == 0, "[0x%lx] TitleIndex = %lu\n", DataLength, PartialInfo->TitleIndex);
        ok(PartialInfo->Type == REG_SZ, "[0x%lx] Type = %lu\n", DataLength, PartialInfo->Type);
        ok(PartialInfo->DataLength == DataLength, "[0x%lx] DataLength = %lu\n", DataLength, PartialInfo->DataLength);
        ok(!memcmp(PartialInfo->Data, LargeBuffer, DataLength), "[0x%lx] Data does not match set value\n", DataLength);
    }
    }

    RtlFreeHeap(GetProcessHeap(), 0, LargeBuffer);
    RtlFreeHeap(GetProcessHeap(), 0, PartialInfo);
    Status = NtDeleteKey(KeyHandle);
    ok(Status == STATUS_SUCCESS, "NtDeleteKey returned %lx\n", Status);
    Status = NtClose(KeyHandle);
    ok(Status == STATUS_SUCCESS, "NtClose returned %lx\n", Status);
    Status = NtClose(ParentKeyHandle);
    ok(Status == STATUS_SUCCESS, "NtClose returned %lx\n", Status);
}
