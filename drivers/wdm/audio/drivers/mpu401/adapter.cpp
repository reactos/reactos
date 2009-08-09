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

#include <ntddk.h>
#include <portcls.h>
#include <debug.h>

NTSTATUS
NTAPI
StartDevice(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIRP pIrp,
    IN  PRESOURCELIST ResourceList)
{
    PPORT port;
    PMINIPORT miniport;
    NTSTATUS Status;

    DPRINT1("MPU401_KS StartDevice called\n");

    if (!ResourceList)
        return STATUS_INVALID_PARAMETER_3;

    if (ResourceList->NumberOfEntries() == 0 )
    {
        return STATUS_INVALID_PARAMETER_3;
    }


    DPRINT1("Calling PcNewPort with CLSID_PortMidi\n");
    Status = PcNewPort(&port, CLSID_PortMidi);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PcNewPort FAILED with status 0x%08x\n", Status);
        return Status;
    }

    DPRINT1("Calling PcNewMiniport with CLSID_MiniportDriverUart\n");
    Status = PcNewMiniport(&miniport, CLSID_MiniportDriverUart);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcNewMiniport FAILED with status 0x%08x\n", Status);
        return Status;
    }

    DPRINT1("Calling Init of port object\n");
    Status = port->Init(pDeviceObject, pIrp, miniport, NULL, ResourceList);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Init FAILED with status 0x%08x\n", Status);
        return Status;
    }

    DPRINT1("Registering subdevice via PcRegisterSubdevice\n");
    Status = PcRegisterSubdevice(pDeviceObject, L"Uart", port);

    if (!NT_SUCCESS(Status))
    {
        /* just print an error here */
        DPRINT1("PcRegisterSubdevice FAILED with status 0x%08x\n", Status);
    }

    miniport->Release();
    port->Release();

    DPRINT1("Device started\n");

    return Status;
}


NTSTATUS
NTAPI
AddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    DPRINT1("MPU401_KS AddDevice called\n");
    return PcAddAdapterDevice(DriverObject,
                              PhysicalDeviceObject,
                              (PCPFNSTARTDEVICE)StartDevice,
                              MAX_MINIPORTS,
                              0);
}
extern "C"
{
NTSTATUS
NTAPI
DriverEntry(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    DPRINT1("MPU401_KS DriverEntry\n");

    Status = PcInitializeAdapterDriver(DriverObject,
                                       RegistryPath,
                                       AddDevice);

    DPRINT1("PcInitializeAdapterDriver result 0x%08x\n", Status);

    return Status;
};
}

