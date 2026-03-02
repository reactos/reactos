#include <ntddk.h>

PVOID LeakedBuffer = NULL;
PDEVICE_OBJECT TestDeviceObject = NULL;

VOID NTAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
    /* intentionally NOT freeing LeakedBuffer (VF should catch this) */
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
    LeakedBuffer = ExAllocatePoolWithTag(NonPagedPool, 1024, 'tseT');
    DbgPrint("VF_TEST: Driver loaded, will return failure\n");
    return STATUS_UNSUCCESSFUL;
}