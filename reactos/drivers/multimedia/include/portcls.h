/*
    ReactOS Kernel Streaming
    Port Class

    Andrew Greenwood

    NOTE: Obsolete macros are not implemented. For more info:
    http://www.osronline.com/ddkx/stream/audpc-struct_167n.htm


    == EXPORTS ==
    DRM:
    * PcAddContentHandlers 
    * PcCreateContentMixed
    * PcDestroyContent
    * PcForwardContentToDeviceObject 
    * PcForwardContentToFileObject
    * PcForwardContentToInterface
    * PcGetContentRights

    IRP HANDLING:
    * PcCompleteIrp 
    * PcDispatchIrp 
    * PcForwardIrpSynchronous

    ADAPTER:
    * PcAddAdapterDevice 
    * PcInitializeAdapterDriver 

    FACTORIES:
    * PcNewDmaChannel 
    * PcNewInterruptSync 
    * PcNewMiniport 
    * PcNewPort 
    * PcNewRegistryKey 
    * PcNewResourceList 
    * PcNewResourceSublist 
    * PcNewServiceGroup 

    POWER MANAGEMENT:
    * PcRegisterAdapterPowerManagement 
    * PcRequestNewPowerState

    PROPERTIES:
    * PcCompletePendingPropertyRequest 
    * PcGetDeviceProperty 

    IO TIMEOUTS:
    * PcRegisterIoTimeout
    * PcUnregisterIoTimeout

    PHYSICAL CONNECTIONS:
    * PcRegisterPhysicalConnection 
    * PcRegisterPhysicalConnectionFromExternal 
    * PcRegisterPhysicalConnectionToExternal 

    MISC:
    * PcGetTimeInterval 
    * PcRegisterSubdevice 


    == INTERFACES ==
    IDmaChannel 
    IDmaChannelSlave 
    IDmaOperations 
    IDrmPort 
    IDrmPort2 
    IInterruptSync 
    IMasterClock 
    IPortClsVersion 
    IPortEvents 
    IPreFetchOffset 
    IRegistryKey
    IResourceList 
    IServiceGroup 
    IServiceSink

    == AUDIO PORT OBJECT INTERFACES ==
    IPort 
    IPortDMus 
    IPortMidi 
    IPortTopology 
    IPortWaveCyclic 
    IPortWavePci

    == AUDIO MINIPORT OBJECT INTERFACES ==
    IMiniport 
    IMiniportDMus 
    IMiniportMidi 
    IMiniportTopology 
    IMiniportWaveCyclic 
    IMiniportWavePci

    == AUDIO MINIPORT AUXILIARY INTERFACES ==
    IMusicTechnology
    IPinCount

    == AUDIO STREAM OBJECT INTERFACES ==
    IAllocatorMXF 
    IDrmAudioStream 
    IMiniportMidiStream 
    IMiniportWaveCyclicStream 
    IMiniportWavePciStream 
    IMXF 
    IPortWavePciStream 
    ISynthSinkDMus

    == DIRECTMUSIC USERMODE SYNTH AND SYNTH SINK INTERFACES ==
    IDirectMusicSynth 
    IDirectMusicSynthSink

    == AUDIO POWER MANAGEMENT INTERFACES ==
    IAdapterPowerManagement
    IPowerNotify
*/

#ifndef PORTCLS_H
#define PORTCLS_H

#include <haxor.h>
#include <ks.h>
#include <drmk.h>

/* TODO */
#define PORTCLASSAPI


const ULONG PCFILTER_NODE = ((ULONG) -1);



/* ===============================================================
    Class IDs - TODO
*/
//#define CLSID_PortDMus    /* dmusicks.h */
#define CLSID_PortMidi
#define CLSID_PortTopology
#define CLSID_PortWaveCyclic
#define CLSID_PortWavePci

/* first 2 are dmusicks.h */
#define CLSID_MiniportDriverDMusUART
#define CLSID_MiniportDriverDMusUARTCapture
#define CLSID_MiniportDriverFmSynth
#define CLSID_MiniportDriverFmSynthWithVol
#define CLSID_MiniportDriverUart


/* ===============================================================
    Property Item Flags - TODO
*/
#define PCPROPERTY_ITEM_FLAG_GET
#define PCPROPERTY_ITEM_FLAG_SET
#define PCPROPERTY_ITEM_FLAG_DEFAULTVALUES
#define PCPROPERTY_ITEM_FLAG_BASICSUPPORT
#define PCPROPERTY_ITEM_FLAG_SERIALIZESIZE
#define PCPROPERTY_ITEM_FLAG_SERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_SERIALIZE


/* ===============================================================
    Event Item Flags - TODO
*/
#define PCEVENT_ITEM_FLAG_ENABLE
#define PCEVENT_ITEM_FLAG_ONESHOT
#define PCEVENT_ITEM_FLAG_BASICSUPPORT


/* ===============================================================
    Event Verbs - TODO
*/
#define PCEVENT_VERB_ADD
#define PCEVENT_VERB_REMOVE
#define PCEVENT_VERB_SUPPORT
#define PCEVENT_VERB_NONE


/* ===============================================================
    Method Item Flags - TODO
*/
#define PCMETHOD_ITEM_FLAG_MODIFY
#define PCMETHOD_ITEM_FLAG_NONE
#define PCMETHOD_ITEM_FLAG_READ
#define PCMETHOD_ITEM_FLAG_SOURCE
#define PCMETHOD_ITEM_FLAG_WRITE


/* ===============================================================
    Method Verbs - TODO
*/
#define PCMETHOD_ITEM_FLAG_BASICSUPPORT
#define PCMETHOD_ITEM_FLAG_SEND
#define PCMETHOD_ITEM_FLAG_SETSUPPORT


/* ===============================================================
    Callback Functions
*/

struct _PCPROPERTY_REQUEST;

typedef NTSTATUS (*PCPFNPROPERTY_HANDLER)(
    IN struct _PCPROPERTY_REQUEST* PropertyRequest);

typedef struct _PCPROPERTY_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNPROPERTY_HANDLER Handler;
} PCPROPERTY_ITEM, *PPCPROPERTY_ITEM;

typedef struct _PCPROPERTY_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCPROPERTY_ITEM* PropertyItem;
    ULONG Verb;
    ULONG InstanceSize;
    PVOID Instance;
    ULONG ValueSize;
    PVOID Value;
    PIRP Irp;
} PCPROPERTY_REQUEST, *PPCPROPERTY_REQUEST;


struct _PCEVENT_REQUEST;

typedef NTSTATUS (*PCPFNEVENT_HANDLER)(
    IN  struct _PCEVENT_REQUEST* EventRequest);

typedef struct _PCEVENT_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNEVENT_HANDLER Handler;
} PCEVENT_ITEM, *PPCEVENT_ITEM;

typedef struct _PCEVENT_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCEVENT_ITEM* EventItem;
    PKSEVENT_ENTRY EventEntry;
    ULONG Verb;
    PIRP Irp;
} PCEVENT_REQUEST, *PPCEVENT_REQUEST;


struct _PCMETHOD_REQUEST;

typedef NTSTATUS (*PCPFNMETHOD_HANDLER)(
    IN  struct _PCMETHOD_REQUEST* MethodRequest);

typedef struct _PCMETHOD_ITEM
{
    const GUID* Set;
    ULONG Id;
    ULONG Flags;
    PCPFNMETHOD_HANDLER Handler;
} PCMETHOD_ITEM, *PPCMETHOD_ITEM;

typedef struct _PCMETHOD_REQUEST
{
    PUNKNOWN MajorTarget;
    PUNKNOWN MinorTarget;
    ULONG Node;
    const PCMETHOD_ITEM* MethodItem;
    ULONG Verb;
} PCMETHOD_REQUEST, *PPCMETHOD_REQUEST;


/* ===============================================================
    Structures (unsorted)
*/

typedef struct
{
    ULONG PropertyItemSize;
    ULONG PropertyCount;
    const PCPROPERTY_ITEM* Properties;
    ULONG MethodItemSize;
    ULONG MethodCount;
    const PCMETHOD_ITEM* Methods;
    ULONG EventItemSize;
    ULONG EventCount;
    const PCEVENT_ITEM* Events;
    ULONG Reserved;
} PCAUTOMATION_TABLE, *PPCAUTOMATION_TABLE;

typedef struct
{
    ULONG FromNode;
    ULONG FromNodePin;
    ULONG ToNode;
    ULONG ToNodePin;
} PCCONNECTION_DESCRIPTOR, *PPCCONNECTIONDESCRIPTOR;

typedef struct
{
    ULONG MaxGlobalInstanceCount;
    ULONG MaxFilterInstanceCount;
    ULONG MinFilterInstanceCount;
    const PCAUTOMATION_TABLE* AutomationTable;
    KSPIN_DESCRIPTOR KsPinDescriptor;
} PCPIN_DESCRIPTOR, *PPCPIN_DESCRIPTOR;

typedef struct
{
    ULONG Flags;
    const PCAUTOMATION_TABLE* AutomationTable;
    const GUID* Type;
    const GUID* Name;
} PCNODE_DESCRIPTOR, *PPCNODE_DESCRIPTOR;

typedef struct
{
    ULONG Version;
    const PCAUTOMATION_TABLE* AutomationTable;
    ULONG PinSize;
    ULONG PinCount;
    const PCPIN_DESCRIPTOR* Pins;
    ULONG NodeSize;
    ULONG NodeCount;
    const PCNODE_DESCRIPTOR* Nodes;
    ULONG ConnectionCount;
    const PCCONNECTION_DESCRIPTOR* Connections;
    ULONG CategoryCount;
    const GUID* Categories;
} PCFILTER_DESCRIPTOR, *PPCFILTER_DESCRIPTOR;


/* ===============================================================
    IPort Interface
*/

typedef struct
{
    /* TODO */
} IPort;


/* ===============================================================
    PortCls API Functions
*/

typedef NTSTATUS (*PCPFNSTARTDEVICE)(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PRESOURCELIST ResourceList);

/* This should be in NTDDK.H */
typedef NTSTATUS (*PDRIVER_ADD_DEVICE)(
    IN struct _DRIVER_OBJECT* DriverObject,
    IN struct _DEVICE_OBJECT* PhysicalDeviceObject);

PORTCLASSAPI NTSTATUS NTAPI
PcAddAdapterDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  PCPFNSTARTDEVICE StartDevice,
    IN  ULONG MaxObjects,
    IN  ULONG DeviceExtensionSize);

PORTCLASSAPI NTSTATUS NTAPI
PcInitializeAdapterDriver(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PUNICODE_STRING RegistryPathName,
    IN  PDRIVER_ADD_DEVICE AddDevice);


/* ===============================================================
    Factories (TODO: Move elsewhere)
*/

PORTCLASSAPI NTSTATUS NTAPI
PcNewDmaChannel(
    OUT PDMACHANNEL* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PDEVICE_DESCRIPTION DeviceDescription,
    IN  PDEVICE_OBJECT DeviceObject);

PORTCLASSAPI NTSTATUS NTAPI
PcNewInterruptSync(
    OUT PINTERRUPTSYNC* OUtInterruptSync,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  PRESOURCELIST ResourceList,
    IN  ULONG ResourceIndex,
    IN  INTERRUPTSYNCMODE Mode);

PORTCLASSAPI NTSTATUS NTAPI
PcNewMiniport(
    OUT PMINIPORT* OutMiniport,
    IN  REFCLSID ClassId);

PORTCLASSAPI NTSTATUS NTAPI
PcNewPort(
    OUT PPORT* OutPort,
    IN  REFCLSID ClassId);

PORTCLASSAPI NTSTATUS NTAPI
PcNewRegistryKey(
    OUT PREGISTRYKEY* OutRegistryKey,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  ULONG RegistryKeyType,
    IN  ACCESS_MASK DesiredAccess,
    IN  PVOID DeviceObject OPTIONAL,
    IN  PVOID SubDevice OPTIONAL,
    IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN  ULONG CreateOptions OPTIONAL,
    OUT PULONG Disposition OPTIONAL);

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceList(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PCM_RESOURCE_LIST TranslatedResources,
    IN  PCM_RESOURCE_LIST UntranslatedResources);

PORTCLASSAPI NTSTATUS NTAPI
PcNewResourceSublist(
    OUT PRESOURCELIST* OutResourceList,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ParentList,
    IN  ULONG MaximumEntries);

PORTCLASSAPI NTSTATUS NTAPI
PcNewServiceGroup(
    OUT PSERVICEGROUP* OutServiceGroup,
    IN  PUNKNOWN OuterUnknown OPTIONAL);


/* ===============================================================
    IRP Handling
*/

PORTCLASSAPI NTSTATUS NTAPI
PcDispatchirp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

PORTCLASSAPI NTSTATUS NTAPI
PcCompleteIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  NTSTATUS Status);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardIrpSynchronous(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);


/* ===============================================================
    Power Management
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterAdapterPowerManagement(
    IN  PUNKNOWN pUnknown,
    IN  PVOID pvContext1);

PORTCLASSAPI NTSTATUS NTAPI
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  DEVICE_POWER_STATE RequestedNewState);


/* ===============================================================
    Properties
*/

PORTCLASSAPI NTSTATUS NTAPI
PcGetDeviceProperty(
    IN  PVOID DeviceObject,
    IN  DEVICE_REGISTRY_PROPERTY DeviceProperty,
    IN  ULONG BufferLength,
    OUT PVOID PropertyBuffer,
    OUT PULONG ResultLength);

PORTCLASSAPI NTSTATUS NTAPI
PcCompletePendingPropertyRequest(
    IN  PPCPROPERTY_REQUEST PropertyRequest,
    IN  NTSTATUS NtStatus);


/* ===============================================================
    I/O Timeouts
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext);

PORTCLASSAPI NTSTATUS NTAPI
PcUnregisterIoTimeout(
    IN  PDEVICE_OBJECT pDeviceObject,
    IN  PIO_TIMER_ROUTINE pTimerRoutine,
    IN  PVOID pContext);


/* ===============================================================
    Physical Connections
*/

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnection(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnectionFromExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNICODE_STRING FromString,
    IN  ULONG FromPin,
    IN  PUNKNOWN ToUnknown,
    IN  ULONG ToPin);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterPhysicalConnectionToExternal(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PUNKNOWN FromUnknown,
    IN  ULONG FromPin,
    IN  PUNICODE_STRING ToString,
    IN  ULONG ToPin);


/* ===============================================================
    Misc
*/

PORTCLASSAPI ULONGLONG NTAPI
PcGetTimeInterval(
    IN  ULONGLONG Since);

PORTCLASSAPI NTSTATUS NTAPI
PcRegisterSubdevice(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PWCHAR Name,
    IN  PUNKNOWN Unknown);


/* ===============================================================
    Digital Rights Management Functions
*/

PORTCLASSAPI NTSTATUS NTAPI
PcAddContentHandlers(
    IN  ULONG ContentId,
    IN  PVOID *paHandlers,
    IN  ULONG NumHandlers);

PORTCLASSAPI NTSTATUS NTAPI
PcCreateContentMixed(
    IN  PULONG paContentId,
    IN  ULONG cContentId,
    OUT PULONG pMixedContentId);

PORTCLASSAPI NTSTATUS NTAPI
PcDestroyContent(
    IN  ULONG ContentId);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToDeviceObject(
    IN  ULONG ContentId,
    IN  PVOID Reserved,
    IN  PCDRMFORWARD DrmForward);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToFileObject(
    IN  ULONG ContentId,
    IN  PFILE_OBJECT FileObject);

PORTCLASSAPI NTSTATUS NTAPI
PcForwardContentToInterface(
    IN  ULONG ContentId,
    IN  PUNKNOWN pUnknown,
    IN  ULONG NumMethods);

PORTCLASSAPI NTSTATUS NTAPI
PcGetContentRights(
    IN  ULONG ContentId,
    OUT PDRMRIGHTS DrmRights);


#endif
