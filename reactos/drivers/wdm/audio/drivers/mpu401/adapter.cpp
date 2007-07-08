/*
    ReactOS Operating System
    MPU401 Example KS Driver

    AUTHORS:
        Andrew Greenwood

    NOTES:
        This is an example MPU401 driver. You can use DirectMusic instead with
        this, by changing the CLSIDs accordingly.
*/

#define MAX_MINIPORTS 1

#define PUT_GUIDS_HERE

#define INITGUID
#include <debug.h>
#include <ntddk.h>

#include <portcls.h>

extern "C"
NTSTATUS
StartDevice(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIRP pIrp,
    IN  PRESOURCELIST ResourceList)
{
    DPRINT1("MPU401_KS StartDevice called\n");

    if ( ! ResourceList )
        return STATUS_INVALID_PARAMETER;

    if ( ResourceList->NumberOfEntries() == 0 )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DPRINT1("Sufficient resources available :)\n");

    PPORT port;
    PMINIPORT miniport;

    NTSTATUS status;

    DPRINT1("Calling PcNewPort with CLSID_PortMidi\n");
    status = PcNewPort(&port, CLSID_PortMidi);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT("PcNewPort FAILED with status 0x%08x\n", status);
        return status;
    }

    DPRINT1("Calling PcNewMiniport with CLSID_MiniportDriverUart\n");
    status = PcNewMiniport(&miniport, CLSID_MiniportDriverUart);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT1("PcNewMiniport FAILED with status 0x%08x\n", status);
        return status;
    }

    DPRINT1("Calling Init of port object\n");
    status = port->Init(pDeviceObject, pIrp, miniport, NULL, ResourceList);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT1("Init FAILED with status 0x%08x\n", status);
        return status;
    }

    DPRINT1("Registering subdevice via PcRegisterSubdevice\n");
    status = PcRegisterSubdevice(pDeviceObject, L"Uart", port);

    if ( ! NT_SUCCESS(status) )
    {
        /* just print an error here */
        DPRINT1("PcRegisterSubdevice FAILED with status 0x%08x\n", status);
    }

    miniport->Release();
    port->Release();

    DPRINT1("Device started\n");

    return status;
}


extern "C"
NTSTATUS
AddDevice(
    IN  PVOID Context1,
    IN  PVOID Context2)
{
    DPRINT1("MPU401_KS AddDevice called, redirecting to PcAddAdapterDevice\n");
    return PcAddAdapterDevice((PDRIVER_OBJECT) Context1,
                              (PDEVICE_OBJECT) Context2,
                              StartDevice,
                              MAX_MINIPORTS,
                              0);
}

extern "C"
{

NTSTATUS NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT Context1,
    IN  PUNICODE_STRING Context2)
{
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MPU401Static");

//    KeBugCheck(0x0000007F);
    DPRINT1("MPU401_KS DriverEntry called, redirecting to PcInitializeAdapterDriver\n");

    NTSTATUS status = PcInitializeAdapterDriver((PDRIVER_OBJECT) Context1,
                                     (PUNICODE_STRING) Context2,
                                     (PDRIVER_ADD_DEVICE) AddDevice);
    DPRINT1("Result was 0x%08x\n", status);

    /* Create a device (this will be handled by PnP manager really but we fake for now */

/*
    DPRINT1("Creating device\n");
    status = IoCreateDevice(Context1,
                            0,
                            &DeviceName,
                            FILE_DEVICE_SOUND,
                            0,
                            FALSE,
                            &DeviceObject);

    DPRINT1("Result was 0x%08x\n", status);
*/

    return status;
};

}
