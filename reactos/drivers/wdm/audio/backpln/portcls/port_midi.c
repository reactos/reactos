#include "private.h"

typedef struct
{
    IPortMidiVtbl *lpVtbl;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;
    BOOL bInitialized;

    PMINIPORTMIDI pMiniport;
    PDEVICE_OBJECT pDeviceObject;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PSERVICEGROUP pServiceGroup;

    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
}IPortMidiImpl;

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
IPortMidi_fnQueryInterface(
    IPortMidi* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    WCHAR Buffer[100];
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

    DPRINT1("IPortMidi_fnQueryInterface\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortMidi) ||
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
    else if (IsEqualGUIDAligned(refiid, &IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, &IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }

    StringFromCLSID(refiid, Buffer);
    DPRINT1("IPortMidi_fnQueryInterface no iface %S\n", Buffer);
    KeBugCheckEx(0, 0, 0, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortMidi_fnAddRef(
    IPortMidi* iface)
{
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortMidi_fnRelease(
    IPortMidi* iface)
{
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

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
IPortMidi_fnGetDeviceProperty(
    IN IPortMidi * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortMidi_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortMidi_fnInit(
    IN IPortMidi * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportMidi * Miniport;
    IServiceGroup * ServiceGroup = NULL;
    NTSTATUS Status;
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

    DPRINT1("IPortMidi_fnInit entered This %p DeviceObject %p Irp %p UnknownMiniport %p UnknownAdapter %p ResourceList %p\n",
            This, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);

    if (This->bInitialized)
    {
        DPRINT1("IPortMidi_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportMidi, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortMidi_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    DbgBreakPoint();
    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortMidi_Init failed with %x\n", Status);
        This->bInitialized = FALSE;
        Miniport->lpVtbl->Release(Miniport);
        return Status;
    }

	DPRINT1("IMiniportMidi sucessfully init\n");

    /* get the miniport device descriptor */
    Status = Miniport->lpVtbl->GetDescription(Miniport, &This->pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("failed to get description\n");
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
        return Status;
    }

    This->pServiceGroup = ServiceGroup;

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


    DPRINT1("IPortMidi_fnInit success\n");
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortMidi_fnNewRegistryKey(
    IN IPortMidi * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortMidiImpl * This = (IPortMidiImpl*)iface;

    if (!This->bInitialized)
    {
        DPRINT("IPortMidi_fnNewRegistryKey called w/o initialized\n");
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


VOID
NTAPI
IPortMidi_fnNotify(
    IN IPortMidi * iface,
    IN PSERVICEGROUP  ServiceGroup  OPTIONAL)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
IPortMidi_fnRegisterServiceGroup(
    IN IPortMidi * iface,
    IN PSERVICEGROUP  ServiceGroup)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}

static IPortMidiVtbl vt_IPortMidi =
{
    /* IUnknown methods */
    IPortMidi_fnQueryInterface,
    IPortMidi_fnAddRef,
    IPortMidi_fnRelease,
    /* IPort methods */
    IPortMidi_fnInit,
    IPortMidi_fnGetDeviceProperty,
    IPortMidi_fnNewRegistryKey,
    IPortMidi_fnNotify,
    IPortMidi_fnRegisterServiceGroup
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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

    return IPortMidi_fnQueryInterface((IPortMidi*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

    return IPortMidi_fnAddRef((IPortMidi*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

    return IPortMidi_fnRelease((IPortMidi*)This);
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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_NewIrpTarget this %p\n", This);
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

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
    IPortMidiImpl * This = (IPortMidiImpl*)CONTAINING_RECORD(iface, IPortMidiImpl, lpVtblSubDevice);

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
NewPortMidi(
    OUT PPORT* OutPort)
{
    IPortMidiImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortMidiImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IPortMidi;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);

    DPRINT1("NewPortMidi result %p\n", *OutPort);

    return STATUS_SUCCESS;
}
