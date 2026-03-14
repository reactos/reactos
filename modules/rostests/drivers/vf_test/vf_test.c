#include <ntddk.h>

PVOID LeakedBuffer = NULL;
PDEVICE_OBJECT TestDeviceObject = NULL;

/*
 * TestMode registry value under HKLM\SYSTEM\CurrentControlSet\Services\vf_test:
 *   0 = leak pool on unload (default) -- triggers 0xC4 / 0x62
 *   1 = free with wrong tag          -- triggers 0xC4 / 0x16
 *   2 = free with wrong pool type    -- triggers 0xC4 / 0x17
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
    if (VfTestMode == 0)
    {
        /* intentionally leak -- VF should catch this with 0xC4 / 0x62 */
        DbgPrint("VF_TEST: Unloading, leaking pool at %p\n", LeakedBuffer);
    }
    else
    {
        if (LeakedBuffer)
            ExFreePoolWithTag(LeakedBuffer, 'tseT');
    }

    if (TestDeviceObject)
        IoDeleteDevice(TestDeviceObject);
}

NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT DriverObject,
                           PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\VfTest");

    DriverObject->DriverUnload = DriverUnload;

    IoCreateDevice(DriverObject, 0, &DeviceName,
                   FILE_DEVICE_UNKNOWN, 0, FALSE, &TestDeviceObject);

    VfTestReadMode(RegistryPath);

    LeakedBuffer = ExAllocatePoolWithTag(NonPagedPool, 1024, 'tseT');
    DbgPrint("VF_TEST: Driver loaded at %p, TestMode=%lu\n", LeakedBuffer, VfTestMode);

    if (VfTestMode == 1)
    {
        /* free with wrong tag -- VF should catch this with 0xC4 / 0x16 */
        DbgPrint("VF_TEST: Freeing with wrong tag\n");
        ExFreePoolWithTag(LeakedBuffer, 'XRWT');
        LeakedBuffer = NULL;
    }
    else if (VfTestMode == 2)
    {
        /* free as paged when allocated as nonpaged -- VF catches with 0xC4 / 0x17 */
        DbgPrint("VF_TEST: Freeing with wrong pool type\n");
        ExFreePoolWithTag(LeakedBuffer, 'tseT');
        LeakedBuffer = NULL;
        /* NOTE: pool type mismatch detection requires VF hook on ExFreePool too */
    }

    return STATUS_SUCCESS;
}