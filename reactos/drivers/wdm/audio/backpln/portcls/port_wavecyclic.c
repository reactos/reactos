#include "private.h"

typedef struct
{
    IPortWaveCyclicVtbl *lpVtbl;
    IPortEventsVtbl *lpVbtlPortEvents;
    IUnregisterSubdeviceVtbl *lpVtblUnregisterSubdevice;
    IUnregisterPhysicalConnectionVtbl *lpVtblPhysicalConnection;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;

    BOOL bInitialized;
    PDEVICE_OBJECT pDeviceObject;
    PMINIPORTWAVECYCLIC pMiniport;
    PRESOURCELIST pResourceList;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PPCFILTER_DESCRIPTOR pDescriptor;

}IPortWaveCyclicImpl;

const GUID IID_IMiniportWaveCyclic;
const GUID IID_IPortWaveCyclic;
const GUID IID_IUnknown;
const GUID IID_IIrpTarget;
const GUID IID_IPinCount;
const GUID IID_IPowerNotify;

const GUID GUID_DEVCLASS_SOUND; //FIXME
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
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_ISubdevice))
    {
        *Output = &This->lpVtblSubDevice;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, &IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortWaveCyclic_fnAddRef(
    IPortWaveCyclic* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortWaveCyclic_fnRelease(
    IPortWaveCyclic* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        if (This->bInitialized)
        {
            This->pMiniport->lpVtbl->Release(This->pMiniport);
        }
        if (This->pPinCount)
            This->pPinCount->lpVtbl->Release(This->pPinCount);

        if (This->pPowerNotify)
            This->pPowerNotify->lpVtbl->Release(This->pPowerNotify);

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
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;
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
        Miniport->lpVtbl->Release(Miniport);
        return Status;
    }

    /* check if it supports IPinCount interface */
    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPinCount, (PVOID*)&PinCount);
    if (NT_SUCCESS(Status))
    {
        This->pPinCount = PinCount;
        This->pDescriptor = NULL;
    }
    else
    {
        Status = Miniport->lpVtbl->GetDescription(Miniport, &This->pDescriptor);
        if (!NT_SUCCESS(Status))
        {
            Miniport->lpVtbl->Release(Miniport);
            return Status;
        }
        This->pPinCount = NULL;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPowerNotify, (PVOID*)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        This->pPowerNotify = PowerNotify;
    }
    else
    {
        This->pPowerNotify = NULL;
    }


    /* Initialize port object */
    This->pMiniport = Miniport;
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
    return PcNewRegistryKey(OutRegistryKey, OuterUnknown, RegistryKeyType, DesiredAccess, This->pDeviceObject, NULL /*FIXME*/, ObjectAttributes, CreateOptions, Disposition);
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
    DEVICE_DESCRIPTION DeviceDesc;
    INTERFACE_TYPE BusType;
    ULONG ResultLength;
    NTSTATUS Status;
    ULONG MapRegisters;
    PDMA_ADAPTER Adapter;

    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewSlaveDmaChannel called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    Status = IoGetDeviceProperty(This->pDeviceObject, DevicePropertyLegacyBusType, sizeof(BusType), (PVOID)&BusType, &ResultLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoGetDeviceProperty failed with %x\n", Status);
        return Status;
    }

    RtlZeroMemory(&DeviceDesc, sizeof(DeviceDesc));
    DeviceDesc.Version = DEVICE_DESCRIPTION_VERSION;
    DeviceDesc.Master = FALSE;
    DeviceDesc.InterfaceType = BusType;
    DeviceDesc.MaximumLength = MaximumLength;
    DeviceDesc.DemandMode = DemandMode;
    DeviceDesc.DmaSpeed = DmaSpeed;
    DeviceDesc.DmaChannel = DmaIndex;

    Adapter = IoGetDmaAdapter(This->pDeviceObject, &DeviceDesc, &MapRegisters);
    if (!Adapter)
    {
        DPRINT("IoGetDmaAdapter failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return NewDmaChannelSlave(&DeviceDesc, Adapter, MapRegisters, DmaChannel);



    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IPortWaveCyclic_fnNotify(
    IN IPortWaveCyclic * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
    ServiceGroup->lpVtbl->RequestService (ServiceGroup);
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
// ISubdevice interface
//

static
NTSTATUS
NTAPI
ISubDevice_fnQueryInterface(
    IN ISubdevice *iface,
    IN REFIID InterfaceId,
    IN PVOID* Interface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    return IPortWaveCyclic_fnQueryInterface((IPortWaveCyclic*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    return IPortWaveCyclic_fnAddRef((IPortWaveCyclic*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    return IPortWaveCyclic_fnRelease((IPortWaveCyclic*)This);
}

static
NTSTATUS
NTAPI
ISubDevice_fnNewIrpTarget(
    IN ISubdevice *iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT * DeviceObject,
    IN PIRP Irp, 
    IN KSOBJECT_CREATE *CreateObject)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_NewIrpTarget this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_ReleaseChildren this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnGetDescriptor(
    IN ISubdevice *iface,
    IN struct SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_GetDescriptor this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnDataRangeIntersection(
    IN ISubdevice *iface,
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_DataRangeIntersection this %p\n", This);

    if (This->pMiniport)
    {
        return This->pMiniport->lpVtbl->DataRangeIntersection (This->pMiniport, PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
    }

    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnPowerChangeNotify(
    IN ISubdevice *iface,
    IN POWER_STATE PowerState)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    if (This->pPowerNotify)
    {
        This->pPowerNotify->lpVtbl->PowerChangeNotify(This->pPowerNotify, PowerState);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
ISubDevice_fnPinCount(
    IN ISubdevice *iface,
    IN ULONG  PinId,
    IN OUT PULONG  FilterNecessary,
    IN OUT PULONG  FilterCurrent,
    IN OUT PULONG  FilterPossible,
    IN OUT PULONG  GlobalCurrent,
    IN OUT PULONG  GlobalPossible)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    if (This->pPinCount)
    {
       This->pPinCount->lpVtbl->PinCount(This->pPinCount, PinId, FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible);
       return STATUS_SUCCESS;
    }

    /* FIXME
     * scan filter descriptor 
     */
    return STATUS_UNSUCCESSFUL;
}

static ISubdeviceVtbl vt_ISubdeviceVtbl = 
{
    ISubDevice_fnQueryInterface,
    ISubDevice_fnAddRef,
    ISubDevice_fnRelease,
    ISubDevice_fnNewIrpTarget,
    ISubDevice_fnReleaseChildren,
    ISubDevice_fnGetDescriptor,
    ISubDevice_fnDataRangeIntersection,
    ISubDevice_fnPowerChangeNotify,
    ISubDevice_fnPinCount
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

    RtlZeroMemory(This, sizeof(IPortWaveCyclicImpl));
    This->lpVtbl = (IPortWaveCyclicVtbl*)&vt_IPortWaveCyclicVtbl;
    This->lpVtblSubDevice = (ISubdeviceVtbl*)&vt_ISubdeviceVtbl;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);

    return STATUS_SUCCESS;
}

