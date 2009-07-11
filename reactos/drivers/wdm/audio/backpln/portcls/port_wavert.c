/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavert.c
 * PURPOSE:         WaveRT Port Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortWaveRTVtbl *lpVtbl;
    IPortEventsVtbl *lpVbtlPortEvents;
    IUnregisterSubdeviceVtbl *lpVtblUnregisterSubdevice;
    IUnregisterPhysicalConnectionVtbl *lpVtblPhysicalConnection;
    IPortEventsVtbl *lpVtblPortEvents;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;

    BOOL bInitialized;
    PDEVICE_OBJECT pDeviceObject;
    PMINIPORTWAVERT pMiniport;
    PRESOURCELIST pResourceList;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
    IPortFilterWaveRT * Filter;
}IPortWaveRTImpl;

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

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterWaveRTTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterWaveRTPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET WaveRTPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterWaveRTTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveRTTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterWaveRTPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveRTPinSet,
        0,
        NULL
    }
};

//KSEVENTSETID_LoopedStreaming, Type = KSEVENT_LOOPEDSTREAMING_POSITION
//KSEVENTSETID_Connection, Type = KSEVENT_CONNECTION_ENDOFSTREAM,



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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblPortEvents);

    DPRINT("IPortEvents_fnQueryInterface entered\n");

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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblPortEvents);
    DPRINT("IPortEvents_fnQueryInterface entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortEvents_fnRelease(
    IPortEvents* iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblPortEvents);

    DPRINT("IPortEvents_fnRelease entered\n");
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
    UNIMPLEMENTED
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
    UNIMPLEMENTED
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
IPortWaveRT_fnQueryInterface(
    IPortWaveRT* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IPortWaveRT) ||
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
    else if (IsEqualGUIDAligned(refiid, &IID_IUnregisterSubdevice))
    {
        return NewIUnregisterSubdevice((PUNREGISTERSUBDEVICE*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPortWaveRT_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortWaveRT_fnAddRef(
    IPortWaveRT* iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortWaveRT_fnRelease(
    IPortWaveRT* iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;

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
IPortWaveRT_fnGetDeviceProperty(
    IN IPortWaveRT * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveRT_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortWaveRT_fnInit(
    IN IPortWaveRT * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWaveRT * Miniport;
    NTSTATUS Status;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;

    DPRINT("IPortWaveRT_Init entered %p\n", This);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->bInitialized)
    {
        DPRINT("IPortWaveRT_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportWaveRT, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWaveRT_Init called with invalid IMiniport adapter\n");
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
        DPRINT("IMiniportWaveRT_Init failed with %x\n", Status);
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
                                         2, 
                                         WaveRTPropertySet,
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


    DPRINT("IPortWaveRT successfully initialized\n");
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortWaveRT_fnNewRegistryKey(
    IN IPortWaveRT * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveRT_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return PcNewRegistryKey(OutRegistryKey, OuterUnknown, RegistryKeyType, DesiredAccess, This->pDeviceObject, NULL /*FIXME*/, ObjectAttributes, CreateOptions, Disposition);
}

static IPortWaveRTVtbl vt_IPortWaveRTVtbl =
{
    IPortWaveRT_fnQueryInterface,
    IPortWaveRT_fnAddRef,
    IPortWaveRT_fnRelease,
    IPortWaveRT_fnInit,
    IPortWaveRT_fnGetDeviceProperty,
    IPortWaveRT_fnNewRegistryKey
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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    return IPortWaveRT_fnQueryInterface((IPortWaveRT*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    return IPortWaveRT_fnAddRef((IPortWaveRT*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    return IPortWaveRT_fnRelease((IPortWaveRT*)This);
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
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterWaveRT * Filter;
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_NewIrpTarget this %p\n", This);

    if (This->Filter)
    {
        *OutTarget = (IIrpTarget*)This->Filter;
        return STATUS_SUCCESS;
    }


    Status = NewPortFilterWaveRT(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->lpVtbl->Init(Filter, (IPortWaveRT*)This);
    if (!NT_SUCCESS(Status))
    {
        Filter->lpVtbl->Release(Filter);
        return Status;
    }

    *OutTarget = (IIrpTarget*)Filter;
    This->Filter = Filter;
    return Status;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    //IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnGetDescriptor(
    IN ISubdevice *iface,
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

    ASSERT(This->SubDeviceDescriptor != NULL);

    *Descriptor = This->SubDeviceDescriptor;

    DPRINT("ISubDevice_GetDescriptor this %p desc %p\n", This, This->SubDeviceDescriptor);
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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

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
    IPortWaveRTImpl * This = (IPortWaveRTImpl*)CONTAINING_RECORD(iface, IPortWaveRTImpl, lpVtblSubDevice);

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


///--------------------------------------------------------------
PMINIPORTWAVERT
GetWaveRTMiniport(
    IN IPortWaveRT* iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl *)iface;
    return This->pMiniport;
}

PDEVICE_OBJECT
GetDeviceObjectFromPortWaveRT(
    PPORTWAVERT iface)
{
    IPortWaveRTImpl * This = (IPortWaveRTImpl *)iface;
    return This->pDeviceObject;
}

//---------------------------------------------------------------
// IPortWaveRT constructor
//

NTSTATUS
NewPortWaveRT(
    OUT PPORT* OutPort)
{
    IPortWaveRTImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortWaveRTImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IPortWaveRTVtbl;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->lpVtblPortEvents = &vt_IPortEvents;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);

    DPRINT("NewPortWaveRT %p\n", *OutPort);

    return STATUS_SUCCESS;
}

