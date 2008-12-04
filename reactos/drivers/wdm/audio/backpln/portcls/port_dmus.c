#include "private.h"

typedef struct
{
    IPortDMusVtbl *lpVtbl;

    LONG ref;
    BOOL bInitialized;
    IMiniportDMus *pMiniport;
    DEVICE_OBJECT *pDeviceObject;
    PSERVICEGROUP ServiceGroup;

}IPortDMusImpl;

const GUID IID_IPortDMus;

//---------------------------------------------------------------
// IUnknown interface functions
//

NTSTATUS
NTAPI
IPortDMus_fnQueryInterface(
    IPortDMus* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;
    if (IsEqualGUIDAligned(refiid, &IID_IPortDMus) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortDMus_fnAddRef(
    IPortDMus* iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    return _InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortDMus_fnRelease(
    IPortDMus* iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    _InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        if (This->bInitialized)
        {
            This->pMiniport->lpVtbl->Release(This->pMiniport);
        }
        ExFreePoolWithTag(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}


//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
IPortDMus_fnGetDeviceProperty(
    IN IPortDMus * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortDMus_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortDMus_fnInit(
    IN IPortDMus * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportDMus * Miniport;
    NTSTATUS Status;
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    if (This->bInitialized)
    {
        DPRINT("IPortDMus_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportDMus, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortDMus_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &This->ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMinIPortDMus_Init failed with %x\n", Status);
        return Status;
    }

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortDMus_fnNewRegistryKey(
    IN IPortDMus * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortDMus_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_UNSUCCESSFUL;
}








NTSTATUS
NewPortDMus(
    OUT PPORT* OutPort)
{
    return STATUS_UNSUCCESSFUL;
}

