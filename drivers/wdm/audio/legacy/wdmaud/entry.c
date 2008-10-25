/*
    This doesn't do much yet...
*/

#include <debug.h>

#define InPassiveIrql() \
    (KeGetCurrentIrql() == IRQL_PASSIVE_LEVEL)


NTSTATUS AudioDeviceControl(
    IN PDEVICE_OBJECT device,
    IN PIRP irp
)
{
    return STATUS_SUCCESS;
}


NTSTATUS AudioAddDevice(
    IN PDRIVER_OBJECT driver,
    IN PDEVICE_OBJECT device
)
{
    DPRINT("AudioAddDevice called\n");

    if ( ! IsPassiveIrql() )
    {
        /* What do we do?! */
        /* RtlAssert("FAIL", __FILE__, __LINE__, "?" */
    }

    return STATUS_SUCCESS;
}

VOID AudioUnload(
    IN PDRIVER_OBJECT driver
)
{
    DPRINT("AudioUnload called\n");
}



NTSTATUS STDCALL
DriverEntry(
    IN PDRIVER_OBJECT driver,
    IN PUNICODE_STRING registry_path
)
{
    DPRINT("Wdmaud.sys loaded\n");

    driver->DriverExtension->AddDevice = AudioAddDevice;
    driver->DriverUnload = AudioUnload;

    driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AudioDeviceControl;

    return STATUS_SUCCESS;
}
