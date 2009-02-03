#include "private.h"

typedef struct
{
    IPortTopologyVtbl *lpVtbl;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;
    BOOL bInitialized;

    PMINIPORTTOPOLOGY pMiniport;
    PDEVICE_OBJECT pDeviceObject;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;

    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
}IPortTopologyImpl;


static GUID InterfaceGuids[2] = 
{
    {
        /// KS_CATEGORY_TOPOLOGY
        0xDDA54A40, 0x1E4C, 0x11D1, {0xA0, 0x50, 0x40, 0x57, 0x05, 0xC1, 0x00, 0x00}
    },
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    }
};

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
    WCHAR Buffer[100];
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    DPRINT1("IPortTopology_fnQueryInterface\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortTopology) ||
        IsEqualGUIDAligned(refiid, &IID_IPort) ||
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

    StringFromCLSID(refiid, Buffer);
    DPRINT1("IPortTopology_fnQueryInterface no iface %S\n", Buffer);
    KeBugCheckEx(0, 0, 0, 0, 0);
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
        FreeItem(This, TAG_PORTCLASS);
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
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initiazed\n");
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

    DPRINT1("IPortTopology_fnInit entered This %p DeviceObject %p Irp %p UnknownMiniport %p UnknownAdapter %p ResourceList %p\n",
            This, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);

    if (This->bInitialized)
    {
        DPRINT1("IPortTopology_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortTopology_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortTopology_Init failed with %x\n", Status);
        This->bInitialized = FALSE;
        Miniport->lpVtbl->Release(Miniport);
        return Status;
    }

    /* get the miniport device descriptor */
    Status = Miniport->lpVtbl->GetDescription(Miniport, &This->pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("failed to get description\n");
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
        return Status;
    }

    /* create the subdevice descriptor */
    Status = PcCreateSubdeviceDescriptor(&This->SubDeviceDescriptor, 
                                         2,
                                         InterfaceGuids, 
                                         0, 
                                         NULL,
                                         0, 
                                         NULL,
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         This->pDescriptor);


    DPRINT1("IPortTopology_fnInit success\n");
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
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initialized\n");
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnQueryInterface((IPortTopology*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnAddRef((IPortTopology*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnRelease((IPortTopology*)This);
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_NewIrpTarget this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_ReleaseChildren this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnGetDescriptor(
    IN ISubdevice *iface,
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_GetDescriptor this %p\n", This);
    *Descriptor = This->SubDeviceDescriptor;
    return STATUS_SUCCESS;
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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


NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort)
{
    IPortTopologyImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortTopologyImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IPortTopology;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);
    DPRINT1("NewPortTopology result %p\n", *OutPort);

    return STATUS_SUCCESS;
}
