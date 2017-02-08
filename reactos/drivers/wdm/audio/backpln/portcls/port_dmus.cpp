/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_dmus.cpp
 * PURPOSE:         DirectMusic Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortDMus : public IPortDMus,
                  public ISubdevice
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IPortDMus;
    IMP_ISubdevice;
    CPortDMus(IUnknown *OuterUnknown){}
    virtual ~CPortDMus(){}

protected:

    BOOL m_bInitialized;
    IMiniportDMus * m_pMiniport;
    IMiniportMidi * m_pMiniportMidi;
    DEVICE_OBJECT * m_pDeviceObject;
    PSERVICEGROUP m_ServiceGroup;
    PPINCOUNT m_pPinCount;
    PPOWERNOTIFY m_pPowerNotify;
    PPORTFILTERDMUS m_Filter;

    PPCFILTER_DESCRIPTOR m_pDescriptor;
    PSUBDEVICE_DESCRIPTOR m_SubDeviceDescriptor;

    LONG m_Ref;

    friend VOID GetDMusMiniport(IN IPortDMus * iface, IN PMINIPORTDMUS * Miniport, IN PMINIPORTMIDI * MidiMiniport);

};

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

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterDMusTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterDMusPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET PortDMusPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterDMusTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterDMusTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterDMusPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterDMusPinSet,
        0,
        NULL
    }
};


//---------------------------------------------------------------
// IUnknown interface functions
//

NTSTATUS
NTAPI
CPortDMus::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IPortDMus) ||
        IsEqualGUIDAligned(refiid, IID_IPortMidi) ||
        IsEqualGUIDAligned(refiid, IID_IPort) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN((IPortDMus*)this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_ISubdevice))
    {
        *Output = PVOID(PSUBDEVICE(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IUnregisterSubdevice))
    {
        return NewIUnregisterSubdevice((PUNREGISTERSUBDEVICE*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IUnregisterPhysicalConnection))
    {
        return NewIUnregisterPhysicalConnection((PUNREGISTERPHYSICALCONNECTION*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT("IPortMidi_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
CPortDMus::GetDeviceProperty(
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_bInitialized)
    {
        DPRINT("IPortDMus_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(m_pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
CPortDMus::Init(
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

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_bInitialized)
    {
        DPRINT("IPortDMus_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->QueryInterface(IID_IMiniportDMus, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        // check for legacy interface
        Status = UnknownMiniport->QueryInterface(IID_IMiniportMidi, (PVOID*)&MidiMiniport);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IPortDMus_Init called with invalid IMiniport adapter\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    // Initialize port object
    m_pMiniport = Miniport;
    m_pMiniportMidi = MidiMiniport;
    m_pDeviceObject = DeviceObject;
    m_bInitialized = TRUE;

    if (Miniport)
    {
        // initialize IMiniportDMus
        Status = Miniport->Init(UnknownAdapter, ResourceList, this, &ServiceGroup);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IMiniportDMus_Init failed with %x\n", Status);
            m_bInitialized = FALSE;
            return Status;
        }

        // get the miniport device descriptor
        Status = Miniport->GetDescription(&m_pDescriptor);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("failed to get description\n");
            Miniport->Release();
            m_bInitialized = FALSE;
            return Status;
        }

        // increment reference on miniport adapter
        Miniport->AddRef();

    }
    else
    {
        // initialize IMiniportMidi
        Status = MidiMiniport->Init(UnknownAdapter, ResourceList, (IPortMidi*)this, &ServiceGroup);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("IMiniportMidi_Init failed with %x\n", Status);
            m_bInitialized = FALSE;
            return Status;
        }

        // get the miniport device descriptor
        Status = MidiMiniport->GetDescription(&m_pDescriptor);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("failed to get description\n");
            MidiMiniport->Release();
            m_bInitialized = FALSE;
            return Status;
        }

        // increment reference on miniport adapter
        MidiMiniport->AddRef();
    }

    // create the subdevice descriptor
    Status = PcCreateSubdeviceDescriptor(&m_SubDeviceDescriptor, 
                                         3,
                                         InterfaceGuids, 
                                         0, 
                                         NULL,
                                         2, 
                                         PortDMusPropertySet,
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         m_pDescriptor);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to create descriptor\n");

        if (Miniport)
            Miniport->Release();
        else
            MidiMiniport->Release();

        m_bInitialized = FALSE;
        return Status;
    }

    if (m_ServiceGroup == NULL && ServiceGroup)
    {
        // register service group
        m_ServiceGroup = ServiceGroup;
    }

    // check if it supports IPinCount interface
    Status = UnknownMiniport->QueryInterface(IID_IPinCount, (PVOID*)&PinCount);
    if (NT_SUCCESS(Status))
    {
        // store IPinCount interface
        m_pPinCount = PinCount;
    }

    // does the Miniport adapter support IPowerNotify interface*/
    Status = UnknownMiniport->QueryInterface(IID_IPowerNotify, (PVOID*)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        // store reference
        m_pPowerNotify = PowerNotify;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CPortDMus::NewRegistryKey(
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_bInitialized)
    {
        DPRINT("IPortDMus_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    return PcNewRegistryKey(OutRegistryKey,
                            OuterUnknown,
                            RegistryKeyType,
                            DesiredAccess,
                            m_pDeviceObject,
                            (ISubdevice*)this,
                            ObjectAttributes,
                            CreateOptions,
                            Disposition);
}

VOID
NTAPI
CPortDMus::Notify(
    IN PSERVICEGROUP  ServiceGroup  OPTIONAL)
{
    if (ServiceGroup)
    {
        ServiceGroup->RequestService ();
        return;
    }

    PC_ASSERT(m_ServiceGroup);

    // notify miniport service group
    m_ServiceGroup->RequestService();

    // notify stream miniport service group
    if (m_Filter)
    {
        m_Filter->NotifyPins();
    }
}

VOID
NTAPI
CPortDMus::RegisterServiceGroup(
    IN PSERVICEGROUP  ServiceGroup)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    m_ServiceGroup = ServiceGroup;

    ServiceGroup->AddMember(PSERVICESINK(this));
}
//---------------------------------------------------------------
// ISubdevice interface
//

NTSTATUS
NTAPI
CPortDMus::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    PPORTFILTERDMUS Filter;

    DPRINT("ISubDevice_NewIrpTarget this %p\n", this);

    if (m_Filter)
    {
        *OutTarget = (IIrpTarget*)m_Filter;
        return STATUS_SUCCESS;
    }


    Status = NewPortFilterDMus(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->Init(PPORTDMUS(this));
    if (!NT_SUCCESS(Status))
    {
        Filter->Release();
        return Status;
    }

    *OutTarget = (IIrpTarget*)Filter;
    return Status;
}

NTSTATUS
NTAPI
CPortDMus::ReleaseChildren()
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortDMus::GetDescriptor(
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    DPRINT("ISubDevice_GetDescriptor this %p\n", this);
    *Descriptor = m_SubDeviceDescriptor;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortDMus::DataRangeIntersection(
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    DPRINT("ISubDevice_DataRangeIntersection this %p\n", this);

    if (m_pMiniport)
    {
        return m_pMiniport->DataRangeIntersection (PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortDMus::PowerChangeNotify(
    IN POWER_STATE PowerState)
{
    if (m_pPowerNotify)
    {
        m_pPowerNotify->PowerChangeNotify(PowerState);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CPortDMus::PinCount(
    IN ULONG  PinId,
    IN OUT PULONG  FilterNecessary,
    IN OUT PULONG  FilterCurrent,
    IN OUT PULONG  FilterPossible,
    IN OUT PULONG  GlobalCurrent,
    IN OUT PULONG  GlobalPossible)
{
    if (m_pPinCount)
    {
       m_pPinCount->PinCount(PinId, FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible);
       return STATUS_SUCCESS;
    }

    // FIXME
    // scan filter descriptor 
    
    return STATUS_UNSUCCESSFUL;
}



NTSTATUS
NewPortDMus(
    OUT PPORT* OutPort)
{
    NTSTATUS Status;
    CPortDMus * Port = new(NonPagedPool, TAG_PORTCLASS) CPortDMus(NULL);
    if (!Port)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = Port->QueryInterface(IID_IPort, (PVOID*)OutPort);

    if (!NT_SUCCESS(Status))
    {
        delete Port;
    }

    DPRINT("NewPortDMus %p Status %u\n", Port, Status);
    return Status;

}



VOID
GetDMusMiniport(
    IN IPortDMus * iface, 
    IN PMINIPORTDMUS * Miniport,
    IN PMINIPORTMIDI * MidiMiniport)
{
    CPortDMus * This = (CPortDMus*)iface;

    *Miniport = This->m_pMiniport;
    *MidiMiniport = This->m_pMiniportMidi;
}
