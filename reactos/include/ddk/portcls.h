/*
    ReactOS Kernel Streaming
    Port Class

    Andrew Greenwood

    NOTES:
    Does not support PC_OLD_NAMES (which is required for backwards-compatibility
    with older code)

    Obsolete macros are not implemented. For more info:
    http://www.osronline.com/ddkx/stream/audpc-struct_167n.htm


    == EXPORTS ==
    DRM (new in XP):
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


    == AUDIO HELPER OBJECT INTERFACES ==
    IDmaChannel
    IDmaChannelSlave
    IDmaOperations
    IDrmPort                        (XP)
    IDrmPort2                       (XP)
    IInterruptSync
    IMasterClock
    IPortClsVersion                 (XP)
    IPortEvents
    IPreFetchOffset                 (XP)
    IRegistryKey
    IResourceList
    IServiceGroup
    IServiceSink
    IUnregisterPhysicalConnection   (Vista)
    IUnregisterSubdevice            (Vista)

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
    IMusicTechnology                (XP)
    IPinCount                       (XP)

    == AUDIO STREAM OBJECT INTERFACES ==
    IAllocatorMXF
    IDrmAudioStream                 (XP)
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

#ifdef __cplusplus
extern "C"
{
    #include <wdm.h>
}
#else
    #include <wdm.h>
#endif

#include <windef.h>

#include <ks.h>
#include <ksmedia.h>
#include <punknown.h>
#include <drmk.h>

#ifdef __cplusplus
extern "C"
{
    #include <wdm.h>
}
#else
    #include <wdm.h>
#endif

#ifndef PC_NO_IMPORTS
#define PORTCLASSAPI EXTERN_C __declspec(dllimport)
#else
#define PORTCLASSAPI EXTERN_C
#endif

/* TODO */
#define PCFILTER_NODE ((ULONG) -1)

/* HACK */
/* typedef PVOID CM_RESOURCE_TYPE; */

#define PORT_CLASS_DEVICE_EXTENSION_SIZE ( 64 * sizeof(ULONG_PTR) )


DEFINE_GUID(CLSID_MiniportDriverFmSynth, 0xb4c90ae0L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_MiniportDriverFmSynthWithVol, 0xe5a3c139L, 0xf0f2, 0x11d1, 0x81, 0xaf, 0x00, 0x60, 0x08, 0x33, 0x16, 0xc1);

/* ===============================================================
    Event Item Flags - TODO
*/
#define PCEVENT_ITEM_FLAG_ENABLE            KSEVENT_TYPE_ENABLE
#define PCEVENT_ITEM_FLAG_ONESHOT           KSEVENT_TYPE_ONESHOT
#define PCEVENT_ITEM_FLAG_BASICSUPPORT      KSEVENT_TYPE_BASICSUPPORT


/* ===============================================================
    Event Verbs - TODO
*/
#define PCEVENT_VERB_NONE       0
#define PCEVENT_VERB_ADD        1
#define PCEVENT_VERB_REMOVE     2
#define PCEVENT_VERB_SUPPORT    4


/* ===============================================================
    Method Item Flags - TODO
*/
#define PCMETHOD_ITEM_FLAG_NONE             KSMETHOD_TYPE_NONE
#define PCMETHOD_ITEM_FLAG_READ             KSMETHOD_TYPE_READ
#define PCMETHOD_ITEM_FLAG_WRITE            KSMETHOD_TYPE_WRITE
#define PCMETHOD_ITEM_FLAG_MODIFY           KSMETHOD_TYPE_MODIFY
#define PCMETHOD_ITEM_FLAG_SOURCE           KSMETHOD_TYPE_SOURCE


/* ===============================================================
    Method Verbs - TODO
*/
#define PCMETHOD_ITEM_FLAG_BASICSUPPORT     KSMETHOD_TYPE_BASICSUPPORT
#define PCMETHOD_ITEM_FLAG_SEND
#define PCMETHOD_ITEM_FLAG_SETSUPPORT


/* ===============================================================
    Versions
    IoIsWdmVersionAvailable may also be used by older drivers.
*/

enum
{
    kVersionInvalid = -1,

    kVersionWin98,
    kVersionWin98SE,
    kVersionWin2K,
    kVersionWin98SE_QFE2,
    kVersionWin2K_SP2,
    kVersionWinME,
    kVersionWin98SE_QFE3,
    kVersionWinME_QFE1,
    kVersionWinXP,
    kVersionWinXPSP1,
    kVersionWinServer2003,
    kVersionWin2K_UAAQFE,           /* These support IUnregister* interface */
    kVersionWinXP_UAAQFE,
    kVersionWinServer2003_UAAQFE
};

/* ===============================================================
    Properties
*/

struct _PCPROPERTY_REQUEST;

typedef NTSTATUS (*PCPFNPROPERTY_HANDLER)(
    IN  struct _PCPROPERTY_REQUEST* PropertyRequest);

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

#define PCPROPERTY_ITEM_FLAG_DEFAULTVALUES KSPROPERTY_TYPE_DEFAULTVALUES
#define PCPROPERTY_ITEM_FLAG_GET            KSPROPERTY_TYPE_GET
#define PCPROPERTY_ITEM_FLAG_SET            KSPROPERTY_TYPE_SET
#define PCPROPERTY_ITEM_FLAG_BASICSUPPORT   KSPROPERTY_TYPE_BASICSUPPORT
#define PCPROPERTY_ITEM_FLAG_SERIALIZESIZE  KSPROPERTY_TYPE_SERIALIZESIZE
#define PCPROPERTY_ITEM_FLAG_SERIALIZERAW   KSPROPERTY_TYPE_SERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW KSPROPERTY_TYPE_UNSERIALIZERAW
#define PCPROPERTY_ITEM_FLAG_SERIALIZE      ( PCPROPERTY_ITEM_FLAG_SERIALIZERAW \
                                            | PCPROPERTY_ITEM_FLAG_UNSERIALIZERAW \
                                            | PCPROPERTY_ITEM_FLAG_SERIALIZESIZE)


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
    IResourceList Interface
*/

#undef INTERFACE
#define INTERFACE IResourceList

DEFINE_GUID(IID_IResourceList, 0x22C6AC60L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);

DECLARE_INTERFACE_(IResourceList, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(ULONG, NumberOfEntries)( THIS ) PURE;

    STDMETHOD_(ULONG, NumberOfEntriesOfType)( THIS_
        IN  CM_RESOURCE_TYPE Type) PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR, FindTranslatedEntry)( THIS_
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(PCM_PARTIAL_RESOURCE_DESCRIPTOR, FindUntranslatedEntry)( THIS_
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(NTSTATUS, AddEntry)( THIS_
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated) PURE;

    STDMETHOD_(NTSTATUS, AddEntryFromParent)( THIS_
        IN  IResourceList* Parent,
        IN  CM_RESOURCE_TYPE Type,
        IN  ULONG Index) PURE;

    STDMETHOD_(PCM_RESOURCE_LIST, TranslatedList)( THIS ) PURE;
    STDMETHOD_(PCM_RESOURCE_LIST, UntranslatedList)( THIS ) PURE;
};

#define IMP_IResourceList \
    STDMETHODIMP_(ULONG) NumberOfEntries(void); \
\
    STDMETHODIMP_(ULONG) NumberOfEntriesOfType( \
        IN  CM_RESOURCE_TYPE Type); \
\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindTranslatedEntry( \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(PCM_PARTIAL_RESOURCE_DESCRIPTOR) FindUntranslatedEntry( \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(NTSTATUS) AddEntry( \
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated, \
        IN  PCM_PARTIAL_RESOURCE_DESCRIPTOR Untranslated); \
\
    STDMETHODIMP_(NTSTATUS) AddEntryFromParent( \
        IN  IResourceList* Parent, \
        IN  CM_RESOURCE_TYPE Type, \
        IN  ULONG Index); \
\
    STDMETHODIMP_(PCM_RESOURCE_LIST) TranslatedList(void); \
    STDMETHODIMP_(PCM_RESOURCE_LIST) UntranslatedList(void);

typedef IResourceList *PRESOURCELIST;

#define NumberOfPorts() \
    NumberOfEntriesOfType(CmResourceTypePort)

#define FindTranslatedPort(n) \
    FindTranslatedEntry(CmResourceTypePort, (n))

#define FindUntranslatedPort(n) \
    FindUntranslatedEntry(CmResourceTypePort, (n))

#define AddPortFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypePort, (n))

#define NumberOfInterrupts() \
    NumberOfEntriesOfType(CmResourceTypeInterrupt)

#define FindTranslatedInterrupt(n) \
    FindTranslatedEntry(CmResourceTypeInterrupt, (n))

#define FindUntranslatedInterrupt(n) \
    FindUntranslatedEntry(CmResourceTypeInterrupt, (n))

#define AddInterruptFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeInterrupt, (n))

#define NumberOfMemories() \
    NumberOfEntriesOfType(CmResourceTypeMemory)

#define FindTranslatedMemory(n) \
    FindTranslatedEntry(CmResourceTypeMemory, (n))

#define FindUntranslatedMemory(n) \
    FindUntranslatedEntry(CmResourceTypeMemory, (n))

#define AddMemoryFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeMemory, (n))

#define NumberOfDmas() \
    NumberOfEntriesOfType(CmResourceTypeDma)

#define FindTranslatedDma(n) \
    FindTranslatedEntry(CmResourceTypeDma, (n))

#define FindUntranslatedDma(n) \
    FindUntranslatedEntry(CmResourceTypeDma, (n))

#define AddDmaFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeInterrupt, (n))

#define NumberOfDeviceSpecifics() \
    NumberOfEntriesOfType(CmResourceTypeDeviceSpecific)

#define FindTranslatedDeviceSpecific(n) \
    FindTranslatedEntry(CmResourceTypeDeviceSpecific, (n))

#define FindUntranslatedDeviceSpecific(n) \
    FindUntranslatedEntry(CmResourceTypeDeviceSpecific, (n))

#define AddDeviceSpecificFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeDeviceSpecific, (n))

#define NumberOfBusNumbers() \
    NumberOfEntriesOfType(CmResourceTypeBusNumber)

#define FindTranslatedBusNumber(n) \
    FindTranslatedEntry(CmResourceTypeBusNumber, (n))

#define FindUntranslatedBusNumber(n) \
    FindUntranslatedEntry(CmResourceTypeBusNumber, (n))

#define AddBusNumberFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeBusNumber, (n))

#define NumberOfDevicePrivates() \
    NumberOfEntriesOfType(CmResourceTypeDevicePrivate)

#define FindTranslatedDevicePrivate(n) \
    FindTranslatedEntry(CmResourceTypeDevicePrivate, (n))

#define FindUntranslatedDevicePrivate(n) \
    FindUntranslatedEntry(CmResourceTypeDevicePrivate, (n))

#define AddDevicePrivateFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeDevicePrivate, (n))

#define NumberOfAssignedResources() \
    NumberOfEntriesOfType(CmResourceTypeAssignedResource)

#define FindTranslatedAssignedResource(n) \
    FindTranslatedEntry(CmResourceTypeAssignedResource, (n))

#define FindUntranslatedAssignedResource(n) \
    FindUntranslatedEntry(CmResourceTypeAssignedResource, (n))

#define AddAssignedResourceFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeAssignedResource, (n))

#define NumberOfSubAllocateFroms() \
    NumberOfEntriesOfType(CmResourceTypeSubAllocateFrom)

#define FindTranslatedSubAllocateFrom(n) \
    FindTranslatedEntry(CmResourceTypeSubAllocateFrom, (n))

#define FindUntranslatedSubAllocateFrom(n) \
    FindUntranslatedEntry(CmResourceTypeSubAllocateFrom, (n))

#define AddSubAllocateFromFromParent(p, n) \
    AddEntryFromParent((p), CmResourceTypeSubAllocateFrom, (n))

#undef INTERFACE


/* ===============================================================
    IServiceSink Interface
*/
#define INTERFACE IServiceSink

DEFINE_GUID(IID_IServiceSink, 0x22C6AC64L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);

DECLARE_INTERFACE_(IServiceSink, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    STDMETHOD_(void, RequestService)( THIS ) PURE;
};

#define IMP_IServiceSink \
    STDMETHODIMP_(void) RequestService(void);

typedef IServiceSink *PSERVICESINK;


/* ===============================================================
    IServiceGroup Interface
*/
#undef INTERFACE
#define INTERFACE IServiceGroup

DEFINE_GUID(IID_IServiceGroup, 0x22C6AC65L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);

DECLARE_INTERFACE_(IServiceGroup, IServiceSink)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void, RequestService)( THIS ) PURE;  /* IServiceSink */

    STDMETHOD_(NTSTATUS, AddMember)( THIS_
        IN  PSERVICESINK pServiceSink) PURE;

    STDMETHOD_(void, RemoveMember)( THIS_
        IN  PSERVICESINK pServiceSink) PURE;

    STDMETHOD_(void, SupportDelayedService)( THIS ) PURE;

    STDMETHOD_(void, RequestDelayedService)( THIS_
        IN  ULONGLONG ullDelay) PURE;

    STDMETHOD_(void, CancelDelayedService)( THIS ) PURE;
};

#define IMP_IServiceGroup \
    IMP_IServiceSink; \
\
    STDMETHODIMP_(NTSTATUS) AddMember( \
        IN  PSERVICESINK pServiceSink); \
\
    STDMETHODIMP_(void) RemoveMember( \
        IN  PSERVICESINK pServiceSink); \
\
    STDMETHODIMP_(void) SupportDelayedService(void); \
\
    STDMETHODIMP_(void) RequestDelayedService( \
        IN  ULONGLONG ullDelay); \
\
    STDMETHODIMP_(void) CancelDelayedService(void);

typedef IServiceGroup *PSERVICEGROUP;


/* ===============================================================
    IDmaChannel Interface
*/

#define DEFINE_ABSTRACT_DMACHANNEL() \
    STDMETHOD_(NTSTATUS, AllocateBuffer)( THIS_ \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL) PURE; \
\
    STDMETHOD_(void, FreeBuffer)( THIS ) PURE; \
    STDMETHOD_(ULONG, TransferCount)( THIS ) PURE; \
    STDMETHOD_(ULONG, MaximumBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, AllocatedBufferSize)( THIS ) PURE; \
    STDMETHOD_(ULONG, BufferSize)( THIS ) PURE; \
\
    STDMETHOD_(void, SetBufferSize)( THIS_ \
        IN  ULONG BufferSize) PURE; \
\
    STDMETHOD_(PVOID, SystemAddress)( THIS ) PURE; \
    STDMETHOD_(PHYSICAL_ADDRESS, PhysicalAddress)( THIS ) PURE; \
    STDMETHOD_(PADAPTER_OBJECT, GetAdapterObject)( THIS ) PURE; \
\
    STDMETHOD_(void, CopyTo)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE; \
\
    STDMETHOD_(void, CopyFrom)( THIS_ \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount) PURE;

#define IMP_IDmaChannel() \
    STDMETHODIMP_(NTSTATUS) AllocateBuffer( \
        IN  ULONG BufferSize, \
        IN  PPHYSICAL_ADDRESS PhysicalAddressConstraint OPTIONAL); \
\
    STDMETHODIMP_(void) FreeBuffer(void); \
    STDMETHODIMP_(ULONG) TransferCount(void); \
    STDMETHODIMP_(ULONG) MaximumBufferSize(void); \
    STDMETHODIMP_(ULONG) AllocatedBufferSize(void); \
    STDMETHODIMP_(ULONG) BufferSize(void); \
\
    STDMETHODIMP_(void) SetBufferSize)( \
        IN  ULONG BufferSize); \
\
    STDMETHODIMP_(PVOID) SystemAddress(void); \
    STDMETHODIMP_(PHYSICAL_ADDRESS) PhysicalAddress(void); \
    STDMETHODIMP_(PADAPTER_OBJECT) GetAdapterObject(void); \
\
    STDMETHODIMP_(void) CopyTo( \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount); \
\
    STDMETHODIMP_(void) CopyFrom( \
        IN  PVOID Destination, \
        IN  PVOID Source, \
        IN  ULONG ByteCount);

DECLARE_INTERFACE_(IDmaChannel, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DMACHANNEL()
};

typedef IDmaChannel *PDMACHANNEL;


/* ===============================================================
    IDmaChannelSlave Interface
*/

#define DEFINE_ABSTRACT_DMACHANNELSLAVE() \
    STDMETHOD_(NTSTATUS, Start)( THIS_ \
        IN  ULONG MapSize, \
        IN  BOOLEAN WriteToDevice) PURE; \
\
    STDMETHOD_(NTSTATUS, Stop)( THIS ) PURE; \
    STDMETHOD_(ULONG, ReadCounter)( THIS ) PURE; \
\
    STDMETHOD_(NTSTATUS, WaitForTC)( THIS_ \
        ULONG Timeout) PURE;

#define IMP_IDmaChannelSlave \
    STDMETHODIMP_(NTSTATUS) Start( \
        IN  ULONG MapSize, \
        IN  BOOLEAN WriteToDevice); \
\
    STDMETHODIMP_(NTSTATUS) Stop(void); \
    STDMETHODIMP_(ULONG) ReadCounter)(void); \
\
    STDMETHODIMP_(NTSTATUS, WaitForTC)( \
        ULONG Timeout);

#undef INTERFACE
#define INTERFACE IDmaChannelSlave

DECLARE_INTERFACE_(IDmaChannelSlave, IDmaChannel)
{
    DEFINE_ABSTRACT_UNKNOWN();
    DEFINE_ABSTRACT_DMACHANNEL();
    DEFINE_ABSTRACT_DMACHANNELSLAVE();
};

typedef IDmaChannelSlave *PDMACHANNELSLAVE;


/* ===============================================================
    IInterruptSync Interface
*/

typedef enum
{
    InterruptSyncModeNormal = 1,
    InterruptSyncModeAll,
    InterruptSyncModeRepeat
} INTERRUPTSYNCMODE;

struct IInterruptSync;

typedef NTSTATUS (*PINTERRUPTSYNCROUTINE)(
    IN  struct IInterruptSync* InterruptSync,
    IN  PVOID DynamicContext);

#undef INTERFACE
#define INTERFACE IInterruptSync

DECLARE_INTERFACE_(IInterruptSync, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, CallSynchronizedRoutine)( THIS_
        IN  PINTERRUPTSYNCROUTINE Routine,
        IN  PVOID DynamicContext) PURE;

    STDMETHOD_(PKINTERRUPT, GetKInterrupt)( THIS ) PURE;
    STDMETHOD_(NTSTATUS, Connect)( THIS ) PURE;
    STDMETHOD_(void, Disconnect)( THIS ) PURE;

    STDMETHOD_(NTSTATUS, RegisterServiceRoutine)( THIS_
        IN  PINTERRUPTSYNCROUTINE Routine,
        IN  PVOID DynamicContext,
        IN  BOOLEAN First) PURE;
};

DEFINE_GUID(IID_IInterruptSync, 0x22C6AC63L, 0x851B, 0x11D0, 0x9A, 0x7F, 0x00, 0xAA, 0x00, 0x38, 0xAC, 0xFE);

#define IMP_IInterruptSync \
    STDMETHODIMP_(NTSTATUS, CallSynchronizedRoutine)( \
        IN  PINTERRUPTSYNCROUTINE Routine, \
        IN  PVOID DynamicContext); \
\
    STDMETHODIMP_(PKINTERRUPT, GetKInterrupt)(void); \
    STDMETHODIMP_(NTSTATUS, Connect)(void); \
    STDMETHODIMP_(void, Disconnect)(void); \
\
    STDMETHODIMP_(NTSTATUS, RegisterServiceRoutine)( \
        IN  PINTERRUPTSYNCROUTINE Routine, \
        IN  PVOID DynamicContext, \
        IN  BOOLEAN First);

typedef IInterruptSync *PINTERRUPTSYNC;


/* ===============================================================
    IRegistryKey Interface
*/

#undef INTERFACE
#define INTERFACE IRegistryKey

enum
{
    GeneralRegistryKey,
    DeviceRegistryKey,
    DriverRegistryKey,
    HwProfileRegistryKey,
    DeviceInterfaceRegistryKey
};

DEFINE_GUID(IID_IRegistryKey, 0xE8DA4302l, 0xF304, 0x11D0, 0x95, 0x8B, 0x00, 0xC0, 0x4F, 0xB9, 0x25, 0xD3);

DECLARE_INTERFACE_(IRegistryKey, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, QueryKey)( THIS_
        IN  KEY_INFORMATION_CLASS KeyInformationClass,
        OUT PVOID KeyInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, EnumerateKey)( THIS_
        IN  ULONG Index,
        IN  KEY_INFORMATION_CLASS KeyInformationClass,
        OUT PVOID KeyInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, QueryValueKey)( THIS_
        IN  PUNICODE_STRING ValueName,
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT PVOID KeyValueInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, EnumerateValueKey)( THIS_
        IN  ULONG Index,
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
        OUT PVOID KeyValueInformation,
        IN  ULONG Length,
        OUT PULONG ResultLength) PURE;

    STDMETHOD_(NTSTATUS, SetValueKey)( THIS_
        IN  PUNICODE_STRING ValueName OPTIONAL,
        IN  ULONG Type,
        IN  PVOID Data,
        IN  ULONG DataSize) PURE;

    STDMETHOD_(NTSTATUS, QueryRegistryValues)( THIS_
        IN  PRTL_QUERY_REGISTRY_TABLE QueryTable,
        IN  PVOID Context OPTIONAL) PURE;

    STDMETHOD_(NTSTATUS, NewSubKey)( THIS_
        OUT IRegistryKey** RegistrySubKey,
        IN  PUNKNOWN OuterUnknown,
        IN  ACCESS_MASK DesiredAccess,
        IN  PUNICODE_STRING SubKeyName,
        IN  ULONG CreateOptions,
        OUT PULONG Disposition OPTIONAL) PURE;

    STDMETHOD_(NTSTATUS, DeleteKey)( THIS ) PURE;
};

#define IMP_IRegistryKey \
    STDMETHODIMP_(NTSTATUS) QueryKey( \
        IN  KEY_INFORMATION_CLASS KeyInformationClass, \
        OUT PVOID KeyInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) EnumerateKey( \
        IN  ULONG Index, \
        IN  KEY_INFORMATION_CLASS KeyInformationClass, \
        OUT PVOID KeyInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) QueryValueKey( \
        IN  PUNICODE_STRING ValueName, \
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, \
        OUT PVOID KeyValueInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) EnumerateValueKey( \
        IN  ULONG Index, \
        IN  KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass, \
        OUT PVOID KeyValueInformation, \
        IN  ULONG Length, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) SetValueKey( \
        IN  PUNICODE_STRING ValueName OPTIONAL, \
        IN  ULONG Type, \
        IN  PVOID Data, \
        IN  ULONG DataSize); \
\
    STDMETHODIMP_(NTSTATUS) QueryRegistryValues( \
        IN  PRTL_QUERY_REGISTRY_TABLE QueryTable, \
        IN  PVOID Context OPTIONAL); \
\
    STDMETHODIMP_(NTSTATUS) NewSubKey( \
        OUT IRegistryKey** RegistrySubKey, \
        IN  PUNKNOWN OuterUnknown, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  PUNICODE_STRING SubKeyName, \
        IN  ULONG CreateOptions, \
        OUT PULONG Disposition OPTIONAL); \
\
    STDMETHODIMP_(NTSTATUS) DeleteKey(void);

typedef IRegistryKey *PREGISTRYKEY;


/* ===============================================================
    IMusicTechnology Interface
*/

DECLARE_INTERFACE_(IMusicTechnology, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, SetTechnology)( THIS_
        IN  const GUID* Technology) PURE;
};

#define IMP_IMusicTechnology \
    STDMETHODIMP_(NTSTATUS) SetTechnology( \
        IN  const GUID* Technology);

typedef IMusicTechnology *PMUSICTECHNOLOGY;


/* ===============================================================
    IPort Interface
*/

#if 0
#define STATIC_IPort 0xb4c90a25L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44
DEFINE_GUIDSTRUCT("0xB4C90A25-5791-11d0-86f9-00a0c911b544", IID_IPort);
#define IID_IPort DEFINE_GUIDNAMED(IID_IPort)
#endif
DEFINE_GUID(IID_IPort,
    0xb4c90a25L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

#define DEFINE_ABSTRACT_PORT() \
    STDMETHOD_(NTSTATUS, Init)( THIS_ \
        IN  PDEVICE_OBJECT DeviceObject, \
        IN  PIRP Irp, \
        IN  PUNKNOWN UnknownMiniport, \
        IN  PUNKNOWN UnknownAdapter OPTIONAL, \
        IN  PRESOURCELIST ResourceList) PURE; \
\
    STDMETHOD_(NTSTATUS, GetDeviceProperty)( THIS_ \
        IN  DEVICE_REGISTRY_PROPERTY DeviceProperty, \
        IN  ULONG BufferLength, \
        OUT PVOID PropertyBuffer, \
        OUT PULONG ResultLength) PURE; \
\
    STDMETHOD_(NTSTATUS, NewRegistryKey)( THIS_ \
        OUT PREGISTRYKEY* OutRegistryKey, \
        IN  PUNKNOWN OuterUnknown OPTIONAL, \
        IN  ULONG RegistryKeyType, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, \
        IN  ULONG CreateOptiona OPTIONAL, \
        OUT PULONG Disposition OPTIONAL) PURE;

#define IMP_IPort() \
    STDMETHODIMP_(NTSTATUS) Init( \
        IN  PDEVICE_OBJECT DeviceObject, \
        IN  PIRP Irp, \
        IN  PUNKNOWN UnknownMiniport, \
        IN  PUNKNOWN UnknownAdapter OPTIONAL, \
        IN  PRESOURCELIST ResourceList); \
\
    STDMETHODIMP_(NTSTATUS) GetDeviceProperty( \
        IN  DEVICE_REGISTRY_PROPERTY DeviceProperty, \
        IN  ULONG BufferLength, \
        OUT PVOID PropertyBuffer, \
        OUT PULONG ResultLength); \
\
    STDMETHODIMP_(NTSTATUS) NewRegistryKey( \
        OUT PREGISTRYKEY* OutRegistryKey, \
        IN  PUNKNOWN OuterUnknown OPTIONAL, \
        IN  ULONG RegistryKeyType, \
        IN  ACCESS_MASK DesiredAccess, \
        IN  POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL, \
        IN  ULONG CreateOptiona OPTIONAL, \
        OUT PULONG Disposition OPTIONAL);

DECLARE_INTERFACE_(IPort, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_PORT()
};

typedef IPort *PPORT;


/* ===============================================================
    IPortMidi Interface
*/

#if 0
#define STATIC_IID_IPortMidi \
    0xb4c90a43L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44
DEFINE_GUIDSTRUCT("0xB4C90A43-5791-11d0-86f9-00a0c911b544", IID_IPortMidi);
#define IID_IPortMidi DEFINE_GUIDNAMED(IID_IPortMidi)
#endif

DEFINE_GUID(IID_IPortMidi,
    0xb4c90a40L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortMidi,
    0xb4c90a43L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IPortMidi, IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_PORT()

    STDMETHOD_(VOID, Notify)(THIS_
        IN  PSERVICEGROUP ServiceGroup OPTIONAL) PURE;

    STDMETHOD_(NTSTATUS, RegisterServiceGroup)(THIS_
        IN  PSERVICEGROUP ServiceGroup) PURE;
};

typedef IPortMidi *PPORTMIDI;

#define IMP_IPortMidi() \
    STDMETHODIMP_(VOID) Notify( \
        IN  PSERVICEGROUP ServiceGroup OPTIONAL); \
\
    STDMETHODIMP_(NTSTATUS) RegisterServiceGroup( \
        IN  PSERVICEGROUP ServiceGroup);

#undef INTERFACE

/* ===============================================================
    IPortWaveCyclic Interface
*/

DEFINE_GUID(IID_IPortWaveCyclic,
    0xb4c90a26L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortWaveCyclic,
    0xb4c90a2aL, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

#define INTERFACE IPortWaveCyclic

DECLARE_INTERFACE_(IPortWaveCyclic, IPort)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;


    STDMETHOD_(NTSTATUS,Init)(THIS_
        IN      PDEVICE_OBJECT  DeviceObject,
        IN      PIRP            Irp,
        IN      PUNKNOWN        UnknownMiniport,
        IN      PUNKNOWN        UnknownAdapter      OPTIONAL,
        IN      PRESOURCELIST   ResourceList 
    )   PURE;
    STDMETHOD_(NTSTATUS,GetDeviceProperty)(THIS_
        IN      DEVICE_REGISTRY_PROPERTY    DeviceProperty,
        IN      ULONG                       BufferLength,
        OUT     PVOID                       PropertyBuffer,
        OUT     PULONG                      ResultLength
    )   PURE;
    STDMETHOD_(NTSTATUS,NewRegistryKey)(THIS_
        OUT     PREGISTRYKEY *      OutRegistryKey,
        IN      PUNKNOWN            OuterUnknown,
        IN      ULONG               RegistryKeyType,
        IN      ACCESS_MASK         DesiredAccess,
        IN      POBJECT_ATTRIBUTES  ObjectAttributes    OPTIONAL,
        IN      ULONG               CreateOptions       OPTIONAL,
        OUT     PULONG              Disposition         OPTIONAL 
    )   PURE;

    STDMETHOD_(VOID, Notify)(THIS_
        IN  PSERVICEGROUP ServiceGroup) PURE;


    STDMETHOD_(NTSTATUS, NewMasterDmaChannel)(THIS_
        OUT PDMACHANNEL* DmaChannel,
        IN  PUNKNOWN OuterUnknown,
        IN  PRESOURCELIST ResourceList OPTIONAL,
        IN  ULONG MaximumLength,
        IN  BOOL Dma32BitAddresses,
        IN  BOOL Dma64BitAddresses,
        IN  DMA_WIDTH DmaWidth,
        IN  DMA_SPEED DmaSpeed) PURE;

    STDMETHOD_(NTSTATUS, NewSlaveDmaChannel)(THIS_
        OUT PDMACHANNELSLAVE* DmaChannel,
        IN  PUNKNOWN OuterUnknown,
        IN  PRESOURCELIST ResourceList OPTIONAL,
        IN  ULONG DmaIndex,
        IN  ULONG MaximumLength,
        IN  BOOL DemandMode,
        IN  DMA_SPEED DmaSpeed) PURE;


};

typedef IPortWaveCyclic *PPORTWAVECYCLIC;

#undef INTERFACE
/* ===============================================================
    IPortWavePci Interface
*/
#undef INTERFACE
#define INTERFACE IPortWavePci

DEFINE_GUID(IID_IPortWavePci,
    0xb4c90a50L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortWavePci,
    0xb4c90a54L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IPortWavePci, IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_PORT()

    STDMETHOD_(NTSTATUS, NewMasterDmaChannel)(THIS_
        OUT PDMACHANNEL* DmaChannel,
        IN  PUNKNOWN OuterUnknown,
        IN  POOL_TYPE PoolType,
        IN  PRESOURCELIST ResourceList OPTIONAL,
        IN  BOOL ScatterGather,
        IN  BOOL Dma32BitAddresses,
        IN  BOOL Dma64BitAddresses,
        IN  DMA_WIDTH DmaWidth,
        IN  DMA_SPEED DmaSpeed,
        IN  ULONG MaximumLength,
        IN  ULONG DmaPort) PURE;

    STDMETHOD_(VOID, Notify)(THIS_
        IN  PSERVICEGROUP ServiceGroup) PURE;
};

typedef IPortWavePci *PPORTWAVEPCI;

/* ===============================================================
    IMiniPort Interface
*/

DEFINE_GUID(IID_IMiniPort,
    0xb4c90a24L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

#define DEFINE_ABSTRACT_MINIPORT() \
    STDMETHOD_(NTSTATUS, GetDescription)( THIS_ \
        OUT  PPCFILTER_DESCRIPTOR* Description) PURE; \
\
    STDMETHOD_(NTSTATUS, DataRangeIntersection)( THIS_ \
        IN  ULONG PinId, \
        IN  PKSDATARANGE DataRange, \
        IN  PKSDATARANGE MatchingDataRange, \
        IN  ULONG OutputBufferLength, \
        OUT PVOID ResultantFormat OPTIONAL, \
        OUT PULONG ResultantFormatLength) PURE;

#define IMP_IMiniport \
    STDMETHODIMP_(NTSTATUS) GetDescription( \
        OUT  PPCFILTER_DESCRIPTOR* Description); \
\
    STDMETHODIMP_(NTSTATUS) DataRangeIntersection( \
        IN  ULONG PinId, \
        IN  PKSDATARANGE DataRange, \
        IN  PKSDATARANGE MatchingDataRange, \
        IN  ULONG OutputBufferLength, \
        OUT PVOID ResultantFormat OPTIONAL, \
        OUT PULONG ResultantFormatLength);

DECLARE_INTERFACE_(IMiniport, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_MINIPORT()
};

typedef IMiniport *PMINIPORT;


/* ===============================================================
    IMiniportMidiStream Interface
*/

DECLARE_INTERFACE_(IMiniportMidiStream, IUnknown)
{
    /* TODO - Read, SetFormat, SetState, Write */
};

typedef IMiniportMidiStream* PMINIPORTMIDISTREAM;


/* ===============================================================
    IMiniportMidi Interface
*/

DECLARE_INTERFACE_(IMiniportMidi, IMiniport)
{
    STDMETHOD_(NTSTATUS, Init)(THIS_
    IN  PUNKNOWN UnknownAdapter,
    IN  PRESOURCELIST ResourceList,
    IN  PPORTMIDI Port,
    OUT PSERVICEGROUP* ServiceGroup) PURE;

    STDMETHOD_(NTSTATUS, NewStream)(THIS_
        OUT PMINIPORTMIDISTREAM Stream,
        IN  PUNKNOWN OuterUnknown OPTIONAL,
        IN  POOL_TYPE PoolType,
        IN  ULONG Pin,
        IN  BOOLEAN Capture,
        IN  PKSDATAFORMAT DataFormat,
        OUT PSERVICEGROUP* ServiceGroup) PURE;

    STDMETHOD_(void, Service)(THIS) PURE;
};

/* TODO ... */


/* ===============================================================
    IMiniportDriverUart Interface
*/

DEFINE_GUID(IID_MiniportDriverUart,
    0xb4c90ae1L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_MiniportDriverUart,
    0xb4c90ae1L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

/* ===============================================================
    IPortTopology Interface
*/
#if 0
#define STATIC_IPortTopology \
    0xb4c90a30L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44
DEFINE_GUIDSTRUCT("0xB4C90A30-5791-11d0-86f9-00a0c911b544", IID_IPortTopology);
#define IID_IPortTopology DEFINE_GUIDNAMED(IID_IPortTopology)
#endif

#undef INTERFACE
#define INTERFACE IPortTopology

DEFINE_GUID(IID_IPortTopology, 0xb4c90a30L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);
DEFINE_GUID(CLSID_PortTopology, 0xb4c90a32L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IPortTopology, IPort)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_PORT()
};

typedef IPortTopology *PPORTTOPOLOGY;

#define IMP_IPortTopology IMP_IPort


/* ===============================================================
    IMiniportTopology Interface
*/

#undef INTERFACE
#define INTERFACE IMiniportTopology

DEFINE_GUID(IID_IMiniportTopology, 0xb4c90a31L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IMiniportTopology,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_MINIPORT()

    STDMETHOD_(NTSTATUS,Init)(THIS_
        IN PUNKNOWN UnknownAdapter,
        IN PRESOURCELIST ResourceList,
        IN PPORTTOPOLOGY Port)PURE;
};

typedef IMiniportTopology *PMINIPORTTOPOLOGY;

/* ===============================================================
    IMiniportWaveCyclicStream Interface
*/

#undef INTERFACE
#define INTERFACE IMiniportWaveCyclicStream

DECLARE_INTERFACE_(IMiniportWaveCyclicStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,SetFormat)(THIS_
        IN PKSDATAFORMAT DataFormat)PURE;

    STDMETHOD_(ULONG,SetNotificationFreq)(THIS_
        IN ULONG Interval,
        OUT PULONG FrameSize) PURE;

    STDMETHOD_(NTSTATUS,SetState)(THIS_
        IN KSSTATE State) PURE;

    STDMETHOD_(NTSTATUS,GetPosition)( THIS_
        OUT PULONG Position) PURE;

    STDMETHOD_(NTSTATUS,NormalizePhysicalPosition)(THIS_
        IN OUT PLONGLONG PhysicalPosition) PURE;

    STDMETHOD_(void, Silence)( THIS_
        IN PVOID Buffer,
        IN ULONG ByteCount) PURE;
};

typedef IMiniportWaveCyclicStream *PMINIPORTWAVECYCLICSTREAM;

/* ===============================================================
    IMiniportWaveCyclic Interface
*/
#undef INTERFACE

DEFINE_GUID(IID_IMiniportWaveCyclic,
    0xb4c90a27L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

#define INTERFACE IMiniportWaveCyclic

DECLARE_INTERFACE_(IMiniportWaveCyclic, IMiniport)
{
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_
        REFIID InterfaceId,
        PVOID* Interface
        ) PURE;
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;
    STDMETHOD_(ULONG,Release)(THIS) PURE;


    DEFINE_ABSTRACT_MINIPORT()

    STDMETHOD_(NTSTATUS, Init)(THIS_
        IN PUNKNOWN  UnknownAdapter,
        IN PRESOURCELIST  ResourceList,
        IN PPORTWAVECYCLIC  Port) PURE;

    STDMETHOD_(NTSTATUS, NewStream)(THIS_
        OUT PMINIPORTWAVECYCLICSTREAM  *Stream,
        IN PUNKNOWN  OuterUnknown  OPTIONAL,
        IN POOL_TYPE  PoolType,
        IN ULONG  Pin,
        IN BOOL  Capture,
        IN PKSDATAFORMAT  DataFormat,
        OUT PDMACHANNEL  *DmaChannel,
        OUT PSERVICEGROUP  *ServiceGroup) PURE;
};

typedef IMiniportWaveCyclic *PMINIPORTWAVECYCLIC;
#undef INTERFACE


/* ===============================================================
    IPortWavePciStream Interface
*/
#undef INTERFACE
#define INTERFACE IPortWavePciStream

DEFINE_GUID(IID_IPortWavePciStream, 0xb4c90a51L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IPortWavePciStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()   //  For IUnknown

    STDMETHOD_(NTSTATUS,GetMapping)(THIS_
        IN PVOID Tag,
        OUT PPHYSICAL_ADDRESS PhysicalAddress,
        OUT PVOID * VirtualAddress,
        OUT PULONG ByteCount,
        OUT PULONG Flags)PURE;

    STDMETHOD_(NTSTATUS,ReleaseMapping)(THIS_
        IN PVOID Tag)PURE;

    STDMETHOD_(NTSTATUS,TerminatePacket)(THIS)PURE;
};

typedef IPortWavePciStream *PPORTWAVEPCISTREAM;

/* ===============================================================
    IMiniportWavePciStream Interface
*/
#undef INTERFACE
#define INTERFACE IMiniportWavePciStream

DEFINE_GUID(IID_IMiniportWavePciStream, 0xb4c90a53L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IMiniportWavePciStream,IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS,SetFormat)(THIS_
        IN PKSDATAFORMAT DataFormat)PURE;

    STDMETHOD_(NTSTATUS,SetState)(THIS_
        IN KSSTATE State)PURE;

    STDMETHOD_(NTSTATUS,GetPosition)(THIS_
        OUT PULONGLONG Position)PURE;

    STDMETHOD_(NTSTATUS,NormalizePhysicalPosition)(THIS_
        IN OUT PLONGLONG PhysicalPosition)PURE;

    STDMETHOD_(NTSTATUS,GetAllocatorFraming)(THIS_
        OUT PKSALLOCATOR_FRAMING AllocatorFraming) PURE;

    STDMETHOD_(NTSTATUS,RevokeMappings)(THIS_
        IN PVOID FirstTag,
        IN PVOID LastTag,
        OUT PULONG MappingsRevoked)PURE;

    STDMETHOD_(void,MappingAvailable)(THIS)PURE;

    STDMETHOD_(void,Service)(THIS)PURE;
};

typedef IMiniportWavePciStream *PMINIPORTWAVEPCISTREAM;

/* ===============================================================
    IMiniportWavePci Interface
*/
#undef INTERFACE
#define INTERFACE IMiniportWavePci

DEFINE_GUID(IID_IMiniportWavePci, 0xb4c90a52L, 0x5791, 0x11d0, 0x86, 0xf9, 0x00, 0xa0, 0xc9, 0x11, 0xb5, 0x44);

DECLARE_INTERFACE_(IMiniportWavePci,IMiniport)
{
    DEFINE_ABSTRACT_UNKNOWN()

    DEFINE_ABSTRACT_MINIPORT()

    STDMETHOD_(NTSTATUS,Init)(THIS_
        IN PUNKNOWN UnknownAdapter,
        IN PRESOURCELIST ResourceList,
        IN PPORTWAVEPCI Port,
        OUT PSERVICEGROUP * ServiceGroup)PURE;

    STDMETHOD_(NTSTATUS,NewStream)(THIS_
        OUT PMINIPORTWAVEPCISTREAM *    Stream,
        IN PUNKNOWN OuterUnknown    OPTIONAL,
        IN POOL_TYPE PoolType,
        IN PPORTWAVEPCISTREAM PortStream,
        IN ULONG Pin,
        IN BOOLEAN Capture,
        IN PKSDATAFORMAT DataFormat,
        OUT PDMACHANNEL * DmaChannel,
        OUT PSERVICEGROUP * ServiceGroup)PURE;

    STDMETHOD_(void,Service)(THIS)PURE;
};

typedef IMiniportWavePci *PMINIPORTWAVEPCI;

/* ===============================================================
    IAdapterPowerManagement Interface
*/

#undef INTERFACE
#define INTERFACE IAdapterPowerManagement

DEFINE_GUID(IID_IAdapterPowerManagement, 0x793417D0L, 0x35FE, 0x11D1, 0xAD, 0x08, 0x00, 0xA0, 0xC9, 0x0A, 0xB1, 0xB0);

DECLARE_INTERFACE_(IAdapterPowerManagement, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(void,PowerChangeState)(THIS_
        IN POWER_STATE NewState) PURE;

    STDMETHOD_(NTSTATUS,QueryPowerChangeState)(THIS_
        IN POWER_STATE NewStateQuery) PURE;

    STDMETHOD_(NTSTATUS,QueryDeviceCapabilities)(THIS_
        IN PDEVICE_CAPABILITIES PowerDeviceCaps) PURE;
};

#define IMP_IAdapterPowerManagement

/* ===============================================================
    IPowerNotify Interface
*/

/* ===============================================================
    IPinCount Interface
*/

/* ===============================================================
    IPortEvents Interface
*/

DECLARE_INTERFACE_(IPortEvents, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    /* TODO */
};

typedef IPortEvents *PPORTEVENTS;


/* ===============================================================
    IDrmPort / IDrmPort2 Interfaces
    These are almost identical, except for the addition of two extra methods.
*/

#undef INTERFACE
#define INTERFACE IDrmPort

#if (NTDDI_VERSION >= NTDDI_WINXP)
DEFINE_GUID(IID_IDrmPort, 0x286D3DF8L, 0xCA22, 0x4E2E, 0xB9, 0xBC, 0x20, 0xB4, 0xF0, 0xE2, 0x01, 0xCE);
#endif

#define DEFINE_ABSTRACT_DRMPORT()                          \
    STDMETHOD_(NTSTATUS,CreateContentMixed)(THIS_          \
        IN  PULONG paContentId,                            \
        IN  ULONG cContentId,                              \
        OUT PULONG pMixedContentId)PURE;                   \
                                                           \
    STDMETHOD_(NTSTATUS,DestroyContent)(THIS_              \
        IN ULONG ContentId)PURE;                           \
                                                           \
    STDMETHOD_(NTSTATUS,ForwardContentToFileObject)(THIS_  \
        IN ULONG        ContentId,                         \
        IN PFILE_OBJECT FileObject)PURE;                   \
                                                           \
    STDMETHOD_(NTSTATUS,ForwardContentToInterface)(THIS_   \
        IN ULONG ContentId,                                \
        IN PUNKNOWN pUnknown,                              \
        IN ULONG NumMethods)PURE;                          \
                                                           \
    STDMETHOD_(NTSTATUS,GetContentRights)(THIS_            \
        IN  ULONG ContentId,                               \
        OUT PDRMRIGHTS  DrmRights)PURE;

DECLARE_INTERFACE_(IDrmPort, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DRMPORT()
};

typedef IDrmPort *PDRMPORT;

/* ===============================================================
    IDrmPort2 Interface
*/

#undef INTERFACE
#define INTERFACE IDrmPort2

#if (NTDDI_VERSION >= NTDDI_WINXP)
DEFINE_GUID(IID_IDrmPort2, 0x1ACCE59CL, 0x7311, 0x4B6B, 0x9F, 0xBA, 0xCC, 0x3B, 0xA5, 0x9A, 0xCD, 0xCE);
#endif

DECLARE_INTERFACE_(IDrmPort2, IDrmPort)
{
    DEFINE_ABSTRACT_UNKNOWN()
    DEFINE_ABSTRACT_DRMPORT()

    STDMETHOD_(NTSTATUS,AddContentHandlers)(THIS_
        IN ULONG ContentId,
        IN PVOID * paHandlers,
        IN ULONG NumHandlers)PURE;

    STDMETHOD_(NTSTATUS,ForwardContentToDeviceObject)(THIS_
        IN ULONG ContentId,
        IN PVOID Reserved,
        IN PCDRMFORWARD DrmForward)PURE;
};

typedef IDrmPort2 *PDRMPORT2;

/* ===============================================================
    IPortClsVersion Interface
*/
#undef INTERFACE
#define INTERFACE IPortClsVersion

DECLARE_INTERFACE_(IPortClsVersion, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(DWORD, GetVersion)(THIS) PURE;
};

#define IMP_IPortClsVersion \
    STDMETHODIMP_(DWORD) GetVersion(void);

typedef IPortClsVersion *PPORTCLSVERSION;


/* ===============================================================
    IDmaOperations Interface
*/

/* ===============================================================
    IPreFetchOffset Interface
*/



/* ===============================================================
    PortCls API Functions
*/

typedef NTSTATUS (*PCPFNSTARTDEVICE)(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PRESOURCELIST ResourceList);

/* This is in NTDDK.H */
/*
typedef NTSTATUS (*PDRIVER_ADD_DEVICE)(
    IN struct _DRIVER_OBJECT* DriverObject,
    IN struct _DEVICE_OBJECT* PhysicalDeviceObject);
*/

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
PcDispatchIrp(
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
PcRequestNewPowerState(
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
    Implemented in XP and above
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
