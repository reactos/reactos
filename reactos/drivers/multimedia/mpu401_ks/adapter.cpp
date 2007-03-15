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
#include <portcls.h>

extern "C"
NTSTATUS
StartDevice(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIRP pIrp,
    IN  PRESOURCELIST ResourceList)
{
    if ( ! ResourceList )
        return STATUS_INVALID_PARAMETER;

    if ( ResourceList->NumberOfEntries() == 0 )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    PPORT port;
    PMINIPORT miniport;

    NTSTATUS status;

    status = PcNewPort(&port, CLSID_PortMidi);

    if ( ! NT_SUCCESS(status) )
    {
        return status;
    }

    status = PcNewMiniport(&miniport, CLSID_MiniportDriverUart);

    if ( ! NT_SUCCESS(status) )
    {
        return status;
    }

    status = port->Init(pDeviceObject, pIrp, miniport, NULL, ResourceList);

    if ( ! NT_SUCCESS(status) )
    {
        return status;
    }

    status = PcRegisterSubdevice(pDeviceObject, L"Uart", port);

    if ( ! NT_SUCCESS(status) )
    {
        /* just print an error here */
    }

    miniport->Release();
    port->Release();

    return status;
}


extern "C"
NTSTATUS
AddDevice(
    IN  PVOID Context1,
    IN  PVOID Context2)
{
    return PcAddAdapterDevice((PDRIVER_OBJECT) Context1,
                              (PDEVICE_OBJECT) Context2,
                              StartDevice,
                              MAX_MINIPORTS,
                              0);
}


extern "C"
NTSTATUS
DriverEntry(
    IN  PVOID Context1,
    IN  PVOID Context2)
{
    return PcInitializeAdapterDriver((PDRIVER_OBJECT) Context1,
                                     (PUNICODE_STRING) Context2,
                                     (PDRIVER_ADD_DEVICE) AddDevice);
};
