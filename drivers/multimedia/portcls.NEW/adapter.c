/*
    ReactOS Kernel Streaming
    Port Class API: Adapter initialization

    Author: Andrew Greenwood

*/

#include <portcls.h>


NTSTATUS
PcStartIo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    /* Internal function */
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
PcUnload(
    IN  PDRIVER_OBJECT DriverObject)
{
    /* Internal function */
    return STATUS_UNSUCCESSFUL;
}


/*
 * @unimplemented
 */
PORTCLASSAPI NTSTATUS NTAPI
PcInitializeAdapterDriver(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName,
    IN  PDRIVER_ADD_DEVICE AddDevice)
{
    /*
        This is effectively a common DriverEntry function for PortCls drivers.
        So it has similar responsibilities to a normal driver.

        First 2 parameters are from DriverEntry.
        Installs the supplied AddDevice routine in the driver object?s driver extension and installs the PortCls driver?s IRP handlers in the driver object itself.
    */

    DriverObject->DriverExtension->AddDevice = AddDevice;

    /*
        TODO: (* = implement here, otherwise KS default)
        IRP_MJ_CLOSE
        * IRP_MJ_CREATE
        IRP_MJ_DEVICE_CONTROL
        IRP_MJ_FLUSH_BUFFERS
        * IRP_MJ_PNP
        * IRP_MJ_POWER
        IRP_MJ_QUERY_SECURITY
        IRP_MJ_READ
        IRP_MJ_SET_SECURITY
        * IRP_MJ_SYSTEM_CONTROL
        IRP_MJ_WRITE
    */

    UNIMPLEMENTED;

    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
PORTCLASSAPI NTSTATUS NTAPI
PcAddAdapterDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  PCPFNSTARTDEVICE StartDevice,
    IN  ULONG MaxObjects,
    IN  ULONG DeviceExtensionSize)
{
    /*
        Note - after this has been called, we can
        handle IRP_MN_START_DEVICE by calling StartDevice
    */

    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
