/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_dmus.c
 * PURPOSE:         DirectMusic Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortDMusVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;
    BOOL bInitialized;
    IMiniportDMus *pMiniport;
    IMiniportMidi *pMiniportMidi;
    DEVICE_OBJECT *pDeviceObject;
    PSERVICEGROUP ServiceGroup;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PPORTFILTERDMUS Filter;

    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;

}IPortDMusImpl;

static GUID InterfaceGuids[3] = 
{
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KS_CATEGORY_RENDER
        0x65E8773E, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KS_CATEGORY_CAPTURE
        0x65E8773D, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    }
};

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
    UNICODE_STRING GuidString;
    IPortDMusImpl * This = (IPortDMusImpl*)iface;


    if (IsEqualGUIDAligned(refiid, &IID_IPortDMus) ||
        IsEqualGUIDAligned(refiid, &IID_IPortMidi) ||
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
    else if (IsEqualGUIDAligned(refiid, &IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, &IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IUnregisterSubdevice))
    {
        return NewIUnregisterSubdevice((PUNREGISTERSUBDEVICE*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPortMidi_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
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
IPortDMus_fnGetDeviceProperty(
    IN IPortDMus * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

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
    IMiniportDMus * Miniport = NULL;
    IMiniportMidi * MidiMiniport = NULL;
    NTSTATUS Status;
    PSERVICEGROUP ServiceGroup = NULL;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->bInitialized)
    {
        DPRINT("IPortDMus_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportDMus, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        /* check for legacy interface */
        Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportMidi, (PVOID*)&MidiMiniport);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IPortDMus_Init called with invalid IMiniport adapter\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (Miniport)
    {
        /* initialize IMiniportDMus */
        Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &ServiceGroup);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IMiniportDMus_Init failed with %x\n", Status);
            return Status;
        }

        /* get the miniport device descriptor */
        Status = Miniport->lpVtbl->GetDescription(Miniport, &This->pDescriptor);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("failed to get description\n");
            Miniport->lpVtbl->Release(Miniport);
            return Status;
        }

        /* increment reference on miniport adapter */
        Miniport->lpVtbl->AddRef(Miniport);

    }
    else
    {
        /* initialize IMiniportMidi */
        Status = MidiMiniport->lpVtbl->Init(MidiMiniport, UnknownAdapter, ResourceList, (IPortMidi*)iface, &ServiceGroup);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IMiniportMidi_Init failed with %x\n", Status);
            return Status;
        }

        /* get the miniport device descriptor */
        Status = MidiMiniport->lpVtbl->GetDescription(MidiMiniport, &This->pDescriptor);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("failed to get description\n");
            MidiMiniport->lpVtbl->Release(MidiMiniport);
            return Status;
        }

        /* increment reference on miniport adapter */
        MidiMiniport->lpVtbl->AddRef(MidiMiniport);
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
        DPRINT1("Failed to create descriptior\n");

        if (Miniport)
            Miniport->lpVtbl->Release(Miniport);
        else
            MidiMiniport->lpVtbl->Release(MidiMiniport);

        return Status;
    }

    if (This->ServiceGroup == NULL && ServiceGroup)
    {
        /* register service group */
        This->ServiceGroup = ServiceGroup;
    }

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pMiniportMidi = MidiMiniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

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

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortDMus_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IPortDMus_fnNotify(
    IN IPortDMus * iface,
    IN PSERVICEGROUP  ServiceGroup  OPTIONAL)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    if (ServiceGroup)
    {
        ServiceGroup->lpVtbl->RequestService (ServiceGroup);
        return;
    }

    ASSERT(This->ServiceGroup);

    /* notify miniport service group */
    This->ServiceGroup->lpVtbl->RequestService(This->ServiceGroup);

    /* notify stream miniport service group */
    if (This->Filter)
    {
        This->Filter->lpVtbl->NotifyPins(This->Filter);
    }
}

VOID
NTAPI
IPortDMus_fnRegisterServiceGroup(
    IN IPortDMus * iface,
    IN PSERVICEGROUP  ServiceGroup)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    This->ServiceGroup = ServiceGroup;

    ServiceGroup->lpVtbl->AddRef(ServiceGroup);
    ServiceGroup->lpVtbl->AddMember(ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);
}

static IPortDMusVtbl vt_IPortDMus =
{
    /* IUnknown methods */
    IPortDMus_fnQueryInterface,
    IPortDMus_fnAddRef,
    IPortDMus_fnRelease,
    /* IPort methods */
    IPortDMus_fnInit,
    IPortDMus_fnGetDeviceProperty,
    IPortDMus_fnNewRegistryKey,
    IPortDMus_fnNotify,
    IPortDMus_fnRegisterServiceGroup
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
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

    return IPortDMus_fnQueryInterface((IPortDMus*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

    return IPortDMus_fnAddRef((IPortDMus*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

    return IPortDMus_fnRelease((IPortDMus*)This);
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
    PPORTFILTERDMUS Filter;
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_NewIrpTarget this %p\n", This);

    if (This->Filter)
    {
        *OutTarget = (IIrpTarget*)This->Filter;
        return STATUS_SUCCESS;
    }


    Status = NewPortFilterDMus(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->lpVtbl->Init(Filter, (PPORTDMUS)This);
    if (!NT_SUCCESS(Status))
    {
        Filter->lpVtbl->Release(Filter);
        return Status;
    }

    *OutTarget = (IIrpTarget*)Filter;
    return Status;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    //IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

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
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_GetDescriptor this %p\n", This);
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
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

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
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

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
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblSubDevice);

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

static ISubdeviceVtbl vt_ISubdevice = 
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


static
NTSTATUS
NTAPI
IServiceSink_fnQueryInterface(
    IServiceSink* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblServiceSink);

    if (IsEqualGUIDAligned(refiid, &IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    DPRINT("Unknown interface requested\n");
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IServiceSink_fnAddRef(
    IServiceSink* iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblServiceSink);
    return IPortDMus_fnAddRef((IPortDMus*)This);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortDMusImpl * This = (IPortDMusImpl*)CONTAINING_RECORD(iface, IPortDMusImpl, lpVtblServiceSink);
    return IPortDMus_fnRelease((IPortDMus*)This);
}

static
VOID
NTAPI
IServiceSink_fnRequestService(
    IServiceSink* iface)
{
    UNIMPLEMENTED
}

static IServiceSinkVtbl vt_IServiceSink = 
{
    IServiceSink_fnQueryInterface,
    IServiceSink_fnAddRef,
    IServiceSink_fnRelease,
    IServiceSink_fnRequestService
};

NTSTATUS
NewPortDMus(
    OUT PPORT* OutPort)
{
    IPortDMusImpl * This = AllocateItem(NonPagedPool, sizeof(IPortDMusImpl), TAG_PORTCLASS);

    if (!This)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    This->ref = 1;
    This->lpVtbl = &vt_IPortDMus;
    This->lpVtblServiceSink = &vt_IServiceSink;
    This->lpVtblSubDevice = &vt_ISubdevice;

    *OutPort = (PPORT)&This->lpVtbl;

    return STATUS_SUCCESS;
}

VOID
GetDMusMiniport(
    IN IPortDMus * iface, 
    IN PMINIPORTDMUS * Miniport,
    IN PMINIPORTMIDI * MidiMiniport)
{
    IPortDMusImpl * This = (IPortDMusImpl*)iface;

    *Miniport = This->pMiniport;
    *MidiMiniport = This->pMiniportMidi;
}
