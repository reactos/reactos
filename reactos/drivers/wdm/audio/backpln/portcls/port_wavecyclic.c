#include "private.h"

typedef struct
{
    IPortWaveCyclicVtbl *lpVtbl;
    IPortClsVersion  *lpVtblPortClsVersion;
#if 0
    IUnregisterSubdevice *lpVtblUnregisterSubDevice;
#endif

    LONG ref;

    BOOL bInitialized;
    PDEVICE_OBJECT pDeviceObject;
    PMINIPORTWAVECYCLIC pMiniport;

}IPortWaveCyclicImpl;

const GUID IID_IMiniportWaveCyclic;
const GUID IID_IPortWaveCyclic;
const GUID IID_IUnknown;

//---------------------------------------------------------------
// IUnknown interface functions
//

NTSTATUS
NTAPI
IPortWaveCyclic_fnQueryInterface(
    IPortWaveCyclic* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;
    if (IsEqualGUIDAligned(refiid, &IID_IPortWaveCyclic) ||
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
IPortWaveCyclic_fnAddRef(
    IPortWaveCyclic* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    return _InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortWaveCyclic_fnRelease(
    IPortWaveCyclic* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

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
IPortWaveCyclic_fnGetDeviceProperty(
    IN IPortWaveCyclic * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortWaveCyclic_fnInit(
    IN IPortWaveCyclic * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWaveCyclic * Miniport;
    NTSTATUS Status;
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportWaveCyclic, (PVOID*)&Miniport);
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
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortWaveCyclic_fnNewRegistryKey(
    IN IPortWaveCyclic * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_UNSUCCESSFUL;
}


//---------------------------------------------------------------
// IPortWaveCyclic interface functions
//

NTSTATUS
NTAPI
IPortWaveCyclic_fnNewMasterDmaChannel(
    IN IPortWaveCyclic * iface,
    OUT PDMACHANNEL* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG MaximumLength,
    IN  BOOL Dma32BitAddresses,
    IN  BOOL Dma64BitAddresses,
    IN  DMA_WIDTH DmaWidth,
    IN  DMA_SPEED DmaSpeed)
{
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IPortWaveCyclic_fnNewSlaveDmaChannel(
    IN IPortWaveCyclic * iface,
    OUT PDMACHANNELSLAVE* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG DmaIndex,
    IN  ULONG MaximumLength,
    IN  BOOL DemandMode,
    IN  DMA_SPEED DmaSpeed)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IPortWaveCyclic_fnNotify(
    IN IPortWaveCyclic * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
}


static const IPortWaveCyclicVtbl vt_IPortWaveCyclicVtbl =
{
    IPortWaveCyclic_fnQueryInterface,
    IPortWaveCyclic_fnAddRef,
    IPortWaveCyclic_fnRelease,
    IPortWaveCyclic_fnInit,
    IPortWaveCyclic_fnGetDeviceProperty,
    IPortWaveCyclic_fnNewRegistryKey,
    IPortWaveCyclic_fnNotify,
    IPortWaveCyclic_fnNewMasterDmaChannel,
    IPortWaveCyclic_fnNewSlaveDmaChannel,
};

//---------------------------------------------------------------
// IPortWaveCyclic constructor
//

NTSTATUS
NewPortWaveCyclic(
    OUT PPORT* OutPort)
{
    IPortWaveCyclicImpl * This;

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPortWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = (IPortWaveCyclicVtbl*)&vt_IPortWaveCyclicVtbl;
    This->ref = 1;
    This->bInitialized = FALSE;
    *OutPort = (PPORT)(&This->lpVtbl);

    return STATUS_SUCCESS;
}

