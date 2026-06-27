/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver Verifier test driver
 * COPYRIGHT:   Copyright 2026 ReactOS Contributors
 */

#include <ntddk.h>

PVOID LeakedBuffer = NULL;

/*
 * TestMode registry value under HKLM\SYSTEM\CurrentControlSet\Services\vf_test:
 *   0 = off (default) -- driver loads and unloads cleanly
 *   1 = leak pool on unload -- triggers 0xC4 / 0x62
 *   2 = free with wrong tag -- triggers 0xC4 / 0x16
 *   3 = pool type mismatch (not synthetically triggerable -- see VfFreePool)
 *       falls back to leak on unload
 */
static ULONG VfTestMode = 0;

static VOID VfTestReadMode(PUNICODE_STRING RegistryPath)
{
    OBJECT_ATTRIBUTES ObjAttrs;
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
    UCHAR Buf[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION Info = (PVOID)Buf;
    ULONG ResultLen;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjAttrs, RegistryPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL, NULL);

    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjAttrs);
    if (!NT_SUCCESS(Status))
        return;

    RtlInitUnicodeString(&ValueName, L"TestMode");
    Status = ZwQueryValueKey(KeyHandle, &ValueName,
                             KeyValuePartialInformation,
                             Info, sizeof(Buf), &ResultLen);

    if (NT_SUCCESS(Status) && Info->Type == REG_DWORD)
        VfTestMode = *(PULONG)Info->Data;

    ZwClose(KeyHandle);
}

VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
    if (VfTestMode == 1)
    {
        /* intentionally leak -- VF should catch this with 0xC4 / 0x62 */
        DbgPrint("VF_TEST: Unloading, leaking pool at %p\n", LeakedBuffer);
    }
    else
    {
        if (LeakedBuffer)
            ExFreePoolWithTag(LeakedBuffer, 'tseT');
    }
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject,
                           PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = DriverUnload;

    VfTestReadMode(RegistryPath);

    LeakedBuffer = ExAllocatePoolWithTag(NonPagedPool, 1024, 'tseT');
    if (!LeakedBuffer)
    {
        DbgPrint("VF_TEST: Failed to allocate pool\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DbgPrint("VF_TEST: Driver loaded at %p, TestMode=%lu\n", DriverObject, VfTestMode);

    if (VfTestMode == 2)
    {
        /* free with wrong tag -- VF should catch this with 0xC4 / 0x16 */
        DbgPrint("VF_TEST: Freeing with wrong tag\n");
        ExFreePoolWithTag(LeakedBuffer, 'XRWT');
        LeakedBuffer = NULL;
    }
    else if (VfTestMode == 3)
    {
        /*
         * Pool type mismatch (0xC4 / 0x17) cannot be triggered synthetically:
         * the pool header always reflects the actual allocation type, so
         * VfFreePool will always see matching types. Detection fires only on
         * pool header corruption. Fall through -- buffer will leak on unload.
         */
        DbgPrint("VF_TEST: Mode 3 -- pool type mismatch not synthetically triggerable, will leak instead\n");
    }

    return STATUS_SUCCESS;
}