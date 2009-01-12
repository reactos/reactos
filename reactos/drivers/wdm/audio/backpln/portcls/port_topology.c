#include "private.h"

typedef struct
{
    IPortTopologyVtbl *lpVtbl;

    LONG ref;
    BOOL bInitialized;

    PMINIPORTTOPOLOGY Miniport;
    PDEVICE_OBJECT pDeviceObject;
    PRESOURCELIST pResourceList;

}IPortTopologyImpl;

const GUID IID_IMiniportTopology;
const GUID IID_IPortTopology;
#if 0
static
KSPROPERTY_SET PinPropertySet =
{
    &KSPROPSETID_Pin,
    0,
    NULL,
    0,
    NULL
};

static
KSPROPERTY_SET TopologyPropertySet =
{
    &KSPROPSETID_Topology,
    4,
    NULL,
    0,
    NULL
};
#endif


//---------------------------------------------------------------
// IUnknown interface functions
//

NTSTATUS
NTAPI
IPortTopology_fnQueryInterface(
    IPortTopology* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IPortTopology) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortTopology_fnAddRef(
    IPortTopology* iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortTopology_fnRelease(
    IPortTopology* iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
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
IPortTopology_fnGetDeviceProperty(
    IN IPortTopology * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortTopology_fnInit(
    IN IPortTopology * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportTopology * Miniport;
    NTSTATUS Status;
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    if (This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWaveCyclic_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveCyclic_Init failed with %x\n", Status);
        return Status;
    }

    /* Initialize port object */
    This->Miniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;
    This->pResourceList = ResourceList;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);
    /* increment reference on resource list */
    ResourceList->lpVtbl->AddRef(ResourceList);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortTopology_fnNewRegistryKey(
    IN IPortTopology * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return PcNewRegistryKey(OutRegistryKey, 
                            OuterUnknown,
                            RegistryKeyType,
                            DesiredAccess,
                            This->pDeviceObject,
                            NULL,//FIXME
                            ObjectAttributes,
                            CreateOptions,
                            Disposition);
}

static IPortTopologyVtbl vt_IPortTopology =
{
    /* IUnknown methods */
    IPortTopology_fnQueryInterface,
    IPortTopology_fnAddRef,
    IPortTopology_fnRelease,
    /* IPort methods */
    IPortTopology_fnInit,
    IPortTopology_fnGetDeviceProperty,
    IPortTopology_fnNewRegistryKey
};

NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort)
{
    IPortTopologyImpl * This;

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPortTopologyImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(This, sizeof(IPortTopologyImpl));
    This->lpVtbl = &vt_IPortTopology;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);

    return STATUS_SUCCESS;
}
