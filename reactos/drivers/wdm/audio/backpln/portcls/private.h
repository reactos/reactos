/*
    PortCls FDO Extension

    by Andrew Greenwood
*/

#ifndef PORTCLS_PRIVATE_H
#define PORTCLS_PRIVATE_H

#include <ntddk.h>
#include <portcls.h>
#define NDEBUG
#include <debug.h>

#include <dmusicks.h>

#include "interfaces.h"
#include <ks.h>
#include <ksmedia.h>
#include <stdio.h>
#include <math.h>
#include <intrin.h>

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_PORTCLASS TAG('P', 'C', 'L', 'S')

#ifdef _MSC_VER
  #define STDCALL
  #define DDKAPI
#endif

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

NTSTATUS
NTAPI
PortClsCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS
NTAPI
PortClsSysControl(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

NTSTATUS NewMiniportDMusUART(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS NewMiniportFmSynth(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

NTSTATUS NewPortDMus(
    OUT PPORT* OutPort);

NTSTATUS NewPortTopology(
    OUT PPORT* OutPort);

NTSTATUS NewPortWaveCyclic(
    OUT PPORT* OutPort);

NTSTATUS NewPortWavePci(
    OUT PPORT* OutPort);

NTSTATUS NewIDrmPort(
    OUT PDRMPORT2 *OutPort);

NTSTATUS NewPortClsVersion(
    OUT PPORTCLSVERSION * OutVersion);

NTSTATUS NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter);

NTSTATUS NewPortPinWaveCyclic(
    OUT IPortPinWaveCyclic ** OutPin);

NTSTATUS 
NewPortFilterWavePci(
    OUT IPortFilterWavePci ** OutFilter);

NTSTATUS NewPortPinWavePci(
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

NTSTATUS NewPortPinDMus(
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

NTSTATUS NewPortPinWaveRT(
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
NTAPI
NewDispatchObject(
    IN PIRP Irp,
    IN IIrpTarget * Target);

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


typedef struct
{
    LIST_ENTRY Entry;
    KSOBJECT_HEADER ObjectHeader;
}SUBDEVICE_ENTRY;

typedef struct
{
    LIST_ENTRY Entry;
    ISubdevice * FromSubDevice;
    LPWSTR FromUnicodeString;
    ULONG FromPin;
    ISubdevice * ToSubDevice;
    LPWSTR ToUnicodeString;
    ULONG ToPin;
}PHYSICAL_CONNECTION;

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
    LIST_ENTRY SubDeviceList;
    LIST_ENTRY PhysicalConnectionList;

} PCLASS_DEVICE_EXTENSION, *PPCLASS_DEVICE_EXTENSION;


typedef struct
{
    KSSTREAM_HEADER Header;
    PIRP Irp;
}CONTEXT_WRITE, *PCONTEXT_WRITE;

typedef struct
{
    PVOID Pin;
    PIO_WORKITEM WorkItem;
    PIRP Irp;
}CLOSESTREAM_CONTEXT, *PCLOSESTREAM_CONTEXT;

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

NTSTATUS
NTAPI
PcDmaSlaveDescription(
    IN PRESOURCELIST  ResourceList OPTIONAL,
    IN ULONG DmaIndex,
    IN BOOL DemandMode,
    IN ULONG AutoInitialize,
    IN DMA_SPEED DmaSpeed,
    IN ULONG MaximumLength,
    IN ULONG DmaPort,
    OUT PDEVICE_DESCRIPTION DeviceDescription);

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

NTSTATUS
NTAPI
PcPropertyHandler(
    IN PIRP Irp,
    IN PSUBDEVICE_DESCRIPTOR Descriptor);

NTSTATUS
NTAPI
FastPropertyHandler(
    IN PFILE_OBJECT  FileObject,
    IN PKSPROPERTY UNALIGNED  Property,
    IN ULONG  PropertyLength,
    IN OUT PVOID UNALIGNED  Data,
    IN ULONG  DataLength,
    OUT PIO_STATUS_BLOCK  IoStatus,
    IN ULONG  PropertySetsCount,
    IN const KSPROPERTY_SET *PropertySet,
    IN PSUBDEVICE_DESCRIPTOR Descriptor,
    IN ISubdevice *SubDevice);

PDEVICE_OBJECT
GetDeviceObject(
    IPortWaveCyclic* iface);

IIrpQueue*
NTAPI
IPortWavePciStream_GetIrpQueue(
    IN IPortWavePciStream *iface);

NTSTATUS
NTAPI
NewIPortWavePciStream(
    OUT PPORTWAVEPCISTREAM *Stream);

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
    DEFINE_KSPROPERTY_ITEM_PIN_CATEGORY(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_NAME(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_CONSTRAINEDDATARANGES(PropGeneral),\
    DEFINE_KSPROPERTY_ITEM_PIN_PROPOSEDATAFORMAT(PropGeneral)\
}

#endif
