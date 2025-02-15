/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

#include <stdio.h>

//#define _KS_NO_ANONYMOUS_STRUCTURES_
#define PC_IMPLEMENTATION
#define COM_STDMETHOD_CAN_THROW
#define PC_NO_IMPORTS

#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <dmusicks.h>
#include <kcom.h>
#include <pseh/pseh2.h>

#include "interfaces.hpp"

#define TAG_PORTCLASS 'SLCP'

#define PC_ASSERT(exp) \
  (VOID)((!(exp)) ? \
    RtlAssert((PVOID) #exp, (PVOID)__FILE__, __LINE__, NULL ), FALSE : TRUE)

#define PC_ASSERT_IRQL(x) PC_ASSERT(KeGetCurrentIrql() <= (x))
#define PC_ASSERT_IRQL_EQUAL(x) PC_ASSERT(KeGetCurrentIrql()==(x))

PVOID
__cdecl
operator new(
    size_t Size,
    POOL_TYPE PoolType,
    ULONG Tag);

extern
"C"
NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

extern
"C"
NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS
NewMiniportFmSynth(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS
NewPortDMus(
    OUT PPORT* OutPort);

NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort);

NTSTATUS
NewPortWaveCyclic(
    OUT PPORT* OutPort);

NTSTATUS
NewPortWavePci(
    OUT PPORT* OutPort);

NTSTATUS
NewIDrmPort(
    OUT PDRMPORT2 *OutPort);

NTSTATUS
NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion);

NTSTATUS
NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter);

NTSTATUS
NewPortPinWaveCyclic(
    OUT IPortPinWaveCyclic ** OutPin);

NTSTATUS
NewPortFilterWavePci(
    OUT IPortFilterWavePci ** OutFilter);

NTSTATUS
NewPortPinWavePci(
    OUT IPortPinWavePci ** OutPin);

PDEVICE_OBJECT
GetDeviceObjectFromWaveCyclic(
    IPortWavePci* iface);

PDEVICE_OBJECT
GetDeviceObjectFromPortWavePci(
    IPortWavePci* iface);

PMINIPORTWAVEPCI
GetWavePciMiniport(
    PPORTWAVEPCI Port);


NTSTATUS
NewPortFilterDMus(
    OUT PPORTFILTERDMUS * OutFilter);


NTSTATUS
NewPortPinDMus(
    OUT PPORTPINDMUS * OutPin);

VOID
GetDMusMiniport(
    IN IPortDMus * iface,
    IN PMINIPORTDMUS * Miniport,
    IN PMINIPORTMIDI * MidiMiniport);

#if (NTDDI_VERSION >= NTDDI_VISTA)

NTSTATUS
NewPortFilterWaveRT(
    OUT IPortFilterWaveRT ** OutFilter);

NTSTATUS
NewPortPinWaveRT(
    OUT IPortPinWaveRT ** OutPin);

PMINIPORTWAVERT
GetWaveRTMiniport(
    IN IPortWaveRT* iface);

PDEVICE_OBJECT
GetDeviceObjectFromPortWaveRT(
    IPortWaveRT* iface);


NTSTATUS
NewPortWaveRTStream(
    PPORTWAVERTSTREAM *OutStream);

NTSTATUS
NewPortWaveRT(
    OUT PPORT* OutPort);


#endif

NTSTATUS
NewPortFilterTopology(
    OUT IPortFilterTopology ** OutFilter);

PMINIPORTTOPOLOGY
GetTopologyMiniport(
    PPORTTOPOLOGY Port);

NTSTATUS
NTAPI
NewDispatchObject(
    IN PIRP Irp,
    IN IIrpTarget * Target,
    IN ULONG ObjectCreateItemCount,
    IN PKSOBJECT_CREATE_ITEM ObjectCreateItem);

PMINIPORTWAVECYCLIC
GetWaveCyclicMiniport(
    IN IPortWaveCyclic* iface);

PVOID
AllocateItem(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag);

VOID
FreeItem(
    IN PVOID Item,
    IN ULONG Tag);

NTSTATUS
NTAPI
NewIrpQueue(
    IN IIrpQueue **Queue);

NTSTATUS
NTAPI
TopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

NTSTATUS
NTAPI
PinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data);

extern
"C"
NTSTATUS
NTAPI
PcDmaMasterDescription(
    IN PRESOURCELIST ResourceList OPTIONAL,
    IN BOOLEAN ScatterGather,
    IN BOOLEAN Dma32BitAddresses,
    IN BOOLEAN IgnoreCount,
    IN BOOLEAN Dma64BitAddresses,
    IN DMA_WIDTH DmaWidth,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription);

extern
"C"
NTSTATUS
NTAPI
PcDmaSlaveDescription(
    IN PRESOURCELIST  ResourceList OPTIONAL,
    IN ULONG DmaIndex,
    IN BOOLEAN DemandMode,
    IN BOOLEAN AutoInitialize,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription);

extern
"C"
NTSTATUS
NTAPI
PcCreateSubdeviceDescriptor(
    OUT SUBDEVICE_DESCRIPTOR ** OutSubdeviceDescriptor,
    IN ULONG InterfaceCount,
    IN GUID * InterfaceGuids,
    IN ULONG IdentifierCount,
    IN KSIDENTIFIER *Identifier,
    IN ULONG FilterPropertiesCount,
    IN KSPROPERTY_SET * FilterProperties,
    IN ULONG Unknown1,
    IN ULONG Unknown2,
    IN ULONG PinPropertiesCount,
    IN KSPROPERTY_SET * PinProperties,
    IN ULONG EventSetCount,
    IN KSEVENT_SET * EventSet,
    IN PPCFILTER_DESCRIPTOR FilterDescription);

extern
"C"
NTSTATUS
NTAPI
PcValidateConnectRequest(
    IN PIRP Irp,
    IN KSPIN_FACTORY * Descriptor,
    OUT PKSPIN_CONNECT* Connect);

NTSTATUS
NTAPI
PcCreateItemDispatch(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

PDEVICE_OBJECT
GetDeviceObject(
    IPortWaveCyclic* iface);

VOID
NTAPI
PcIoTimerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context);

NTSTATUS
NTAPI
NewIUnregisterSubdevice(
    OUT PUNREGISTERSUBDEVICE *OutDevice);

NTSTATUS
NTAPI
NewIUnregisterPhysicalConnection(
    OUT PUNREGISTERPHYSICALCONNECTION *OutConnection);

NTSTATUS
NTAPI
PcHandlePropertyWithTable(
    IN PIRP Irp,
    IN ULONG PropertySetCount,
    IN PKSPROPERTY_SET PropertySet,
    IN PSUBDEVICE_DESCRIPTOR Descriptor);

NTSTATUS
NTAPI
PcHandleEnableEventWithTable(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor);

NTSTATUS
NTAPI
PcHandleDisableEventWithTable(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor);

IIrpTarget *
NTAPI
KsoGetIrpTargetFromIrp(
    PIRP Irp);

#define DEFINE_KSPROPERTY_RTAUDIOSET(PinSet,\
   GetAudioBuffer, GetHwLatency, GetRTAudioPosition,\
   GetClockRegister, GetBufferWithNotification, RegisterNotificationEvent, UnregisterNotificationEvent)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_BUFFER, (GetAudioBuffer), sizeof(KSRTAUDIO_BUFFER_PROPERTY), sizeof(KSRTAUDIO_BUFFER), NULL, NULL, 0, NULL, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_HWLATENCY, (GetHwLatency), sizeof(KSPROPERTY), sizeof(KSRTAUDIO_HWLATENCY), NULL, NULL, NULL, 0, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_POSITIONREGISTER, (GetRTAudioPosition), sizeof(KSRTAUDIO_HWREGISTER_PROPERTY), sizeof(KSRTAUDIO_HWREGISTER), NULL, NULL, NULL, 0, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_CLOCKREGISTER, (GetClockRegister), sizeof(KSRTAUDIO_HWREGISTER_PROPERTY), sizeof(KSRTAUDIO_HWREGISTER), NULL, NULL, NULL, 0, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_BUFFER_WITH_NOTIFICATION, (GetBufferWithNotification), sizeof(KSRTAUDIO_BUFFER_PROPERTY_WITH_NOTIFICATION), sizeof(KSRTAUDIO_BUFFER), NULL, NULL, NULL, 0, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_REGISTER_NOTIFICATION_EVENT, (RegisterNotificationEvent), sizeof(KSRTAUDIO_NOTIFICATION_EVENT_PROPERTY), 0, (RegisterNotificationEvent), NULL, NULL, 0, NULL, 0), \
    DEFINE_KSPROPERTY_ITEM(KSPROPERTY_RTAUDIO_UNREGISTER_NOTIFICATION_EVENT, (UnregisterNotificationEvent), sizeof(KSRTAUDIO_NOTIFICATION_EVENT_PROPERTY), 0, (UnregisterNotificationEvent), NULL, NULL, 0, NULL, 0) \
}

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align)-1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align)-1, (align))

#define DEFINE_KSPROPERTY_CONNECTIONSET(PinSet,\
    PropStateHandler, PropDataFormatHandler, PropAllocatorFraming)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_CONNECTION_STATE(PropStateHandler, PropStateHandler),\
    DEFINE_KSPROPERTY_ITEM_CONNECTION_DATAFORMAT(PropDataFormatHandler, PropDataFormatHandler),\
    DEFINE_KSPROPERTY_ITEM_CONNECTION_ALLOCATORFRAMING(PropAllocatorFraming)\
}

#define DEFINE_KSPROPERTY_ITEM_AUDIO_POSITION(GetHandler, SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_AUDIO_POSITION,\
        (GetHandler),\
        sizeof(KSPROPERTY),\
        sizeof(KSAUDIO_POSITION),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_AUDIOSET(PinSet,\
    PropPositionHandler)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_AUDIO_POSITION(PropPositionHandler, PropPositionHandler)\
}


#define DEFINE_KSPROPERTY_ITEM_DRMAUDIOSTREAM_CONTENTID(SetHandler)\
    DEFINE_KSPROPERTY_ITEM(\
        KSPROPERTY_DRMAUDIOSTREAM_CONTENTID,\
        NULL,\
        sizeof(KSPROPERTY),\
        sizeof(ULONG),\
        (SetHandler),\
        NULL, 0, NULL, NULL, 0)

#define DEFINE_KSPROPERTY_DRMSET(PinSet,\
    PropPositionHandler)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_DRMAUDIOSTREAM_CONTENTID(PropPositionHandler)\
}

#define DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PinSet,\
    PropGeneral, PropInstances, PropIntersection)\
DEFINE_KSPROPERTY_TABLE(PinSet) {\
    DEFINE_KSPROPERTY_ITEM_PIN_CINSTANCES(PropInstances),\
    DEFINE_KSPROPERTY_ITEM_PIN_CTYPES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAFLOW(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_DATAINTERSECTION(PropIntersection),\
    DEFINE_KSPROPERTY_ITEM_PIN_INTERFACES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_MEDIUMS(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_COMMUNICATION(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_GLOBALCINSTANCES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NECESSARYINSTANCES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PHYSICALCONNECTION(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CONSTRAINEDDATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PROPOSEDATAFORMAT(PropGeneral)\
}

typedef struct
{
    KSDEVICE_HEADER KsDeviceHeader;
    PDEVICE_OBJECT PhysicalDeviceObject;
    PDEVICE_OBJECT PrevDeviceObject;
    PCPFNSTARTDEVICE StartDevice;
    ULONG_PTR Unused[4];
    IAdapterPowerManagement * AdapterPowerManagement;
    ULONG MaxSubDevices;
    KSOBJECT_CREATE_ITEM * CreateItems;

    IResourceList* resources;

    LIST_ENTRY TimerList;
    KSPIN_LOCK TimerListLock;

    LIST_ENTRY PowerNotifyList;
    KSPIN_LOCK PowerNotifyListLock;

    DEVICE_POWER_STATE DevicePowerState;
    SYSTEM_POWER_STATE  SystemPowerState;

} PCLASS_DEVICE_EXTENSION, *PPCLASS_DEVICE_EXTENSION;


typedef struct
{
    PVOID Pin;
    PIO_WORKITEM WorkItem;
    PIRP Irp;
}CLOSESTREAM_CONTEXT, *PCLOSESTREAM_CONTEXT;

typedef struct
{
    LIST_ENTRY Entry;
    PIO_TIMER_ROUTINE pTimerRoutine;
    PVOID Context;
}TIMER_CONTEXT, *PTIMER_CONTEXT;

typedef struct
{
    KSOBJECT_HEADER ObjectHeader;
    IIrpTarget * Target;
    PKSOBJECT_CREATE_ITEM CreateItem;
}DISPATCH_CONTEXT, *PDISPATCH_CONTEXT;

typedef struct
{
    LIST_ENTRY Entry;
    PPOWERNOTIFY PowerNotify;
}ENTRY_POWER_NOTIFY, *PENTRY_POWER_NOTIFY;

template<typename... Interfaces>
class CUnknownImpl : public Interfaces...
{
private:
    volatile LONG m_Ref;
protected:
    CUnknownImpl() :
        m_Ref(0)
    {
    }
    virtual ~CUnknownImpl()
    {
    }
public:
    STDMETHODIMP_(ULONG) AddRef()
    {
        ULONG Ref = InterlockedIncrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        return Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        ULONG Ref = InterlockedDecrement(&m_Ref);
        ASSERT(Ref < 0x10000);
        if (!Ref)
        {
            delete this;
            return 0;
        }
        return Ref;
    }
};

#endif /* PORTCLS_PRIVATE_H */
