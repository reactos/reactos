#include "private.h"

typedef struct
{
    IPortWaveCyclicVtbl *lpVtbl;
    IPortEventsVtbl *lpVbtlPortEvents;
    IUnregisterSubdeviceVtbl *lpVtblUnregisterSubdevice;
    IUnregisterPhysicalConnectionVtbl *lpVtblPhysicalConnection;
    IPortEventsVtbl *lpVtblPortEvents;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;

    BOOL bInitialized;
    PDEVICE_OBJECT pDeviceObject;
    PMINIPORTWAVECYCLIC pMiniport;
    PRESOURCELIST pResourceList;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
}IPortWaveCyclicImpl;

static GUID InterfaceGuids[3] = 
{
    {
        /// KSCATEGORY_RENDER
        0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_CAPTURE
        0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    }
};

#if 0
static const KSIDENTIFIER Identifiers[] = 
{
    {
        &KSINTERFACESETID_Standard,
        0,
        0
    },
    {
        &KSINTERFACESETID_Standard,
        1,
        0
    }
};
#endif

//---------------------------------------------------------------
// IPortEvents
//

static
NTSTATUS
NTAPI
IPortEvents_fnQueryInterface(
    IPortEvents* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblPortEvents);

    DPRINT1("IPortEvents_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortEvents) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVbtlPortEvents;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortEvents_fnAddRef(
    IPortEvents* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblPortEvents);
    DPRINT1("IPortEvents_fnQueryInterface entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortEvents_fnRelease(
    IPortEvents* iface)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblPortEvents);
    DPRINT1("IPortEvents_fnRelease entered\n");
    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
void
NTAPI
IPortEvents_fnAddEventToEventList(
    IPortEvents* iface,
    IN PKSEVENT_ENTRY EventEntry)
{
    DPRINT1("IPortEvents_fnAddEventToEventList stub\n");
}


static
void
NTAPI
IPortEvents_fnGenerateEventList(
    IPortEvents* iface,
    IN  GUID* Set OPTIONAL,
    IN  ULONG EventId,
    IN  BOOL PinEvent,
    IN  ULONG PinId,
    IN  BOOL NodeEvent,
    IN  ULONG NodeId)
{
    DPRINT1("IPortEvents_fnGenerateEventList stub\n");
}

static IPortEventsVtbl vt_IPortEvents = 
{
    IPortEvents_fnQueryInterface,
    IPortEvents_fnAddRef,
    IPortEvents_fnRelease,
    IPortEvents_fnAddEventToEventList,
    IPortEvents_fnGenerateEventList
};

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
    WCHAR Buffer[100];
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;
    if (IsEqualGUIDAligned(refiid, &IID_IPortWaveCyclic) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortEvents))
    {
        *Output = &This->lpVtblPortEvents;
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

    StringFromCLSID(refiid, Buffer);
    DPRINT1("IPortWaveCyclic_fnQueryInterface no interface!!! iface %S\n", Buffer);
    KeBugCheckEx(0, 0, 0, 0, 0);

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

    DPRINT1("IPortWaveCyclic_Init entered %p\n", This);

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

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;
    This->pResourceList = ResourceList;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveCyclic_Init failed with %x\n", Status);
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
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
                                         3,
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

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcCreateSubdeviceDescriptor failed with %x\n", Status);
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
        return Status;
    }

    /* check if it supports IPinCount interface */
    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPinCount, (PVOID*)&PinCount);
    if (NT_SUCCESS(Status))
    {
        /* store IPinCount interface */
        This->pPinCount = PinCount;
    }

    /* does the Miniport adapter support IPowerNotify interface*/
    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPowerNotify, (PVOID*)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        /* store reference */
        This->pPowerNotify = PowerNotify;
    }

    /* increment reference on resource list */
    ResourceList->lpVtbl->AddRef(ResourceList);


    DPRINT1("IPortWaveCyclic successfully initialized\n");
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
    NTSTATUS Status;
    DEVICE_DESCRIPTION DeviceDescription;
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewSlaveDmaChannel called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    Status = PcDmaMasterDescription(ResourceList, (Dma32BitAddresses | Dma64BitAddresses), Dma32BitAddresses, 0, Dma64BitAddresses, DmaWidth, DmaSpeed, MaximumLength, 0, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        return PcNewDmaChannel(DmaChannel, OuterUnknown, NonPagedPool, &DeviceDescription, This->pDeviceObject);
    }

    return Status;
}

NTSTATUS
NTAPI
IPortWaveCyclic_fnNewSlaveDmaChannel(
    IN IPortWaveCyclic * iface,
    OUT PDMACHANNELSLAVE* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG DmaIndex,
    IN  ULONG MaximumLength,
    IN  BOOL DemandMode,
    IN  DMA_SPEED DmaSpeed)
{
    DEVICE_DESCRIPTION DeviceDescription;
    PDMACHANNEL DmaChannel;
    NTSTATUS Status;

    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewSlaveDmaChannel called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    // FIXME
    // Check for F-Type DMA Support
    //

    Status = PcDmaSlaveDescription(ResourceList, DmaIndex, DemandMode, TRUE, DmaSpeed, MaximumLength, 0, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        Status = PcNewDmaChannel(&DmaChannel, OuterUnknown, NonPagedPool, &DeviceDescription, This->pDeviceObject);
        if (NT_SUCCESS(Status))
        {
            Status = DmaChannel->lpVtbl->QueryInterface(DmaChannel, &IID_IDmaChannelSlave, (PVOID*)OutDmaChannel);
            DmaChannel->lpVtbl->Release(DmaChannel);
        }
    }

    return Status;
}

VOID
NTAPI
IPortWaveCyclic_fnNotify(
    IN IPortWaveCyclic * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
    ServiceGroup->lpVtbl->RequestService (ServiceGroup);
}

static IPortWaveCyclicVtbl vt_IPortWaveCyclicVtbl =
{
    IPortWaveCyclic_fnQueryInterface,
    IPortWaveCyclic_fnAddRef,
    IPortWaveCyclic_fnRelease,
    IPortWaveCyclic_fnInit,
    IPortWaveCyclic_fnGetDeviceProperty,
    IPortWaveCyclic_fnNewRegistryKey,
    IPortWaveCyclic_fnNotify,
    IPortWaveCyclic_fnNewSlaveDmaChannel,
    IPortWaveCyclic_fnNewMasterDmaChannel
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
    NTSTATUS Status;
    IPortFilterWaveCyclic * Filter;
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_NewIrpTarget this %p\n", This);

    Status = NewPortFilterWaveCyclic(&Filter, (IPortWaveCyclic*)This);
    if (NT_SUCCESS(Status))
    {
        *OutTarget = (IIrpTarget*)Filter;
    }

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
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    IPortWaveCyclicImpl * This = (IPortWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortWaveCyclicImpl, lpVtblSubDevice);

    ASSERT(This->SubDeviceDescriptor != NULL);

    *Descriptor = This->SubDeviceDescriptor;

    DPRINT1("ISubDevice_GetDescriptor this %p desc %p\n", This, This->SubDeviceDescriptor);
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

    This = AllocateItem(NonPagedPool, sizeof(IPortWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IPortWaveCyclicVtbl;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->lpVtblPortEvents = &vt_IPortEvents;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);

    DPRINT1("NewPortWaveCyclic %p\n", *OutPort);

    return STATUS_SUCCESS;
}

