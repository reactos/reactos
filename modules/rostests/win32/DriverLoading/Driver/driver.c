#include <ntddk.h>

VOID
DriverUnload(PDRIVER_OBJECT pDriverObject)
{
    DbgPrint("Test driver unloaded sucessfully\n");
}

NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    DriverObject->DriverUnload = DriverUnload;

    DbgPrint("Test driver loaded sucessfully\n");

    return STATUS_SUCCESS;
}
