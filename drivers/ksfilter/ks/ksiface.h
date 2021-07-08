#pragma once

#if !defined(DEFINE_ABSTRACT_UNKNOWN)

#define DEFINE_ABSTRACT_UNKNOWN()                               \
    STDMETHOD_(NTSTATUS, QueryInterface)(THIS_                  \
        REFIID InterfaceId,                                     \
        PVOID* Interface)PURE;                                  \
    STDMETHOD_(ULONG,AddRef)(THIS) PURE;                        \
    STDMETHOD_(ULONG,Release)(THIS) PURE;
#endif

typedef struct
{
    LIST_ENTRY Entry;
    LIST_ENTRY ObjectList;
    PRKMUTEX BagMutex;
    PVOID DeviceHeader;
}KSIOBJECT_BAG, *PKSIOBJECT_BAG;


/*****************************************************************************
 * IKsAllocator
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsAllocator

DECLARE_INTERFACE_(IKsAllocator, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, DispatchDeviceIoControl)(THIS_
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp) PURE;

    STDMETHOD_(NTSTATUS, Close)(THIS) PURE;

    STDMETHOD_(NTSTATUS, AllocateFrame)(THIS_
        IN PVOID * OutFrame) PURE;

    STDMETHOD_(VOID, FreeFrame)(THIS_
        IN PVOID OutFrame) PURE;
};


/*****************************************************************************
 * IKsPin
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsClock

DECLARE_INTERFACE_(IKsClock, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
};

/*****************************************************************************
 * IKsTransport
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsTransport

DECLARE_INTERFACE_(IKsTransport, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()
};


/*****************************************************************************
 * IKsPin
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsPin

struct KSPTRANSPORTCONFIG;

DECLARE_INTERFACE_(IKsPin, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, TransferKsIrp)(THIS_
        IN PIRP Irp,
        IN IKsTransport **OutTransport) PURE;

    STDMETHOD_(VOID, DiscardKsIrp)(THIS_
        IN PIRP Irp,
        IN IKsTransport * *OutTransport) PURE;

    STDMETHOD_(NTSTATUS, Connect)(THIS_
        IN IKsTransport * TransportIn,
        OUT IKsTransport ** OutTransportIn,
        OUT IKsTransport * *OutTransportOut,
        IN KSPIN_DATAFLOW DataFlow) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_
        IN KSSTATE OldState,
        IN KSSTATE NewState,
        IN IKsTransport * *OutTransport) PURE;

    STDMETHOD_(VOID, SetResetState)(THIS_
        IN KSRESET ResetState,
        OUT IKsTransport * * OutTransportOut) PURE;

    STDMETHOD_(NTSTATUS, GetTransportConfig)(THIS_
        IN struct KSPTRANSPORTCONFIG * TransportConfig,
        OUT IKsTransport ** OutTransportIn,
        OUT IKsTransport ** OutTransportOut) PURE;

    STDMETHOD_(NTSTATUS, SetTransportConfig)(THIS_
        IN struct KSPTRANSPORTCONFIG const * TransportConfig,
        OUT IKsTransport ** OutTransportIn,
        OUT IKsTransport ** OutTransportOut) PURE;

    STDMETHOD_(NTSTATUS, ResetTransportConfig)(THIS_
        OUT IKsTransport ** OutTransportIn,
        OUT IKsTransport ** OutTransportOut) PURE;

    STDMETHOD_(PKSPIN, GetStruct)(THIS) PURE;
    STDMETHOD_(PKSPROCESSPIN, GetProcessPin)(THIS) PURE;
    STDMETHOD_(NTSTATUS, AttemptBypass)(THIS) PURE;
    STDMETHOD_(NTSTATUS, AttemptUnbypass)(THIS) PURE;

    STDMETHOD_(VOID, GenerateConnectionEvents)(THIS_
        IN ULONG EventMask) PURE;

    STDMETHOD_(NTSTATUS, ClientSetDeviceState)(THIS_
        IN KSSTATE StateIn,
        IN KSSTATE StateOut) PURE;
};

/*****************************************************************************
 * IKsQueue
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsQueue

enum KSPSTREAM_POINTER_MOTION {
    KSPSTREAM_POINTER_MOTION_NONE = 0,
    KSPSTREAM_POINTER_MOTION_ADVANCE = 1,
    KSPSTREAM_POINTER_MOTION_CLEAR = 2,
    KSPSTREAM_POINTER_MOTION_FLUSH = 3
};

enum KSPSTREAM_POINTER_STATE {
    KSPSTREAM_POINTER_STATE_UNLOCKED = 0,
    KSPSTREAM_POINTER_STATE_LOCKED = 1,
    KSPSTREAM_POINTER_STATE_CANCELLED = 2,
    KSPSTREAM_POINTER_STATE_DELETED = 3,
    KSPSTREAM_POINTER_STATE_CANCEL_PENDING = 4,
    KSPSTREAM_POINTER_STATE_DEAD = 5,
    KSPSTREAM_POINTER_STATE_TIMED_OUT = 6,
    KSPSTREAM_POINTER_STATE_TIMER_RESCHEDULE = 7
};

enum KSPSTREAM_POINTER_TYPE {
    KSPSTREAM_POINTER_TYPE_NORMAL = 0,
    KSPSTREAM_POINTER_TYPE_INTERNAL = 1
};

typedef struct _KSPTRANSPORTCONFIG {
	UCHAR TransportType; 
	USHORT IrpDisposition;
	UCHAR StackDepth; 
} KSPTRANSPORTCONFIG;

typedef enum _KSPFRAME_HEADER_TYPE {
	KSPFRAME_HEADER_TYPE_NORMAL = 0,
	KSPFRAME_HEADER_TYPE_GHOST = 1
} KSPFRAME_HEADER_TYPE;

typedef struct _KSPIRP_FRAMING_ {
	ULONG  OutputBufferLength; 
	ULONG  RefCount; 
	ULONG  QueuedFrameHeaderCount; 
	PVOID FrameHeaders; 
}KSPIRP_FRAMING;

typedef struct _KSPFRAME_HEADER {
	LIST_ENTRY ListEntry; 
	struct _KSPFRAME_HEADER * NextFrameHeaderInIrp; 
	PVOID Queue; 
	PIRP OriginalIrp; 
	PMDL Mdl; 
	PIRP Irp; 
	KSPIRP_FRAMING* IrpFraming; 
	PVOID StreamHeader; 
	PVOID FrameBuffer; 
	PVOID MappingsTable; 
	ULONG StreamHeaderSize; 
	ULONG FrameBufferSize; 
	PVOID Context; 
	ULONG RefCount; 
	PVOID OriginalData; 
	PVOID BufferedData; 
	NTSTATUS Status; 
	UCHAR DismissalCall; 
	KSPFRAME_HEADER_TYPE Type; 
	PVOID FrameHolder; 
}KSPFRAME_HEADER;

typedef struct _KSPSTREAM_POINTER {
    LIST_ENTRY ListEntry; 
    LIST_ENTRY TimeoutListEntry; 
    ULONGLONG TimeoutTime; 
    PFNKSSTREAMPOINTER CancelCallback; 
    PDRIVER_CANCEL TimeoutCallback; 
    enum KSPSTREAM_POINTER_STATE State; 
    enum KSPSTREAM_POINTER_TYPE Type; 
    ULONG Stride; 
    PVOID CKsQueue;
    KSPFRAME_HEADER* FrameHeader; 
    KSPFRAME_HEADER* FrameHeaderStarted; 
    KSSTREAM_POINTER StreamPointer;
} KSPSTREAM_POINTER;

DECLARE_INTERFACE_(IKsQueue, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(NTSTATUS, TransferKsIrp)(THIS_
        IN PIRP Irp,
        OUT IKsTransport ** Transport) PURE;

    STDMETHOD_(NTSTATUS, DiscardKsIrp)(THIS_
        IN PIRP Irp,
        OUT IKsTransport ** Transport) PURE;

    STDMETHOD_(NTSTATUS, Connect)(THIS_
        IN IKsTransport *T1,
        OUT IKsTransport **OutTransport1,
        OUT IKsTransport **OutTransport2,
        IN KSPIN_DATAFLOW DataFlow) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceState)(THIS_
        IN KSSTATE ToState,
        IN KSSTATE FromState,
        IN PCHAR Message) PURE;

    STDMETHOD_(NTSTATUS, SetResetState)(THIS_
        IN KSRESET Reset,
        IN PCHAR Message) PURE;

    STDMETHOD_(NTSTATUS, GetTransportConfig)(THIS_
        IN KSPTRANSPORTCONFIG * TransportConfig,
        OUT IKsTransport **OutTransport,
        OUT IKsTransport **OutTransport2) PURE;

    STDMETHOD_(NTSTATUS, SetTransportConfig)(THIS_
        IN KSPTRANSPORTCONFIG *a2,
        IN PCHAR Message,
        OUT IKsTransport **OutTransport,
        OUT IKsTransport **OutTransport2) PURE;

    STDMETHOD_(VOID, ResetTransportConfig)(THIS_
        IN IKsTransport ** NextTransport,
        IN IKsTransport ** PrevTransport) PURE;

    STDMETHOD_(NTSTATUS, CloneStreamPointer)(THIS_
        OUT KSPSTREAM_POINTER ** CloneStreamPointer,
        IN PFNKSSTREAMPOINTER CancelCallback,
        IN ULONG ContextSize,
        IN KSPSTREAM_POINTER* StreamPointer,
        IN ULONG Unknown8) PURE;

    STDMETHOD_(VOID, DeleteStreamPointer)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer);

    STDMETHOD_(KSPSTREAM_POINTER*, LockStreamPointer)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer) PURE;

    STDMETHOD_(VOID, UnlockStreamPointer)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer,
        IN enum KSPSTREAM_POINTER_MOTION Motion) PURE;

    STDMETHOD_(VOID, AdvanceUnlockedStreamPointer)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer) PURE;

    STDMETHOD_(KSPSTREAM_POINTER *, GetLeadingStreamPointer)(THIS_
        IN KSSTREAM_POINTER_STATE State) PURE;

    STDMETHOD_(KSPSTREAM_POINTER *, GetTrailingStreamPointer)(THIS_
        IN KSSTREAM_POINTER_STATE State) PURE;

    STDMETHOD_(VOID, ScheduleTimeout)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer,
        IN PFNKSSTREAMPOINTER CancelRoutine,
        IN ULONGLONG TimeOut) PURE;

    STDMETHOD_(VOID, CancelTimeout)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer) PURE;

    STDMETHOD_(KSPSTREAM_POINTER*, GetFirstClone)(THIS) PURE;

    STDMETHOD_(KSPSTREAM_POINTER*, GetNextClone)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer) PURE;

    STDMETHOD_(VOID, GetAvailableByteCount)(THIS_
        IN PULONG InputDataBytes,
        IN PULONG OutputBufferBytes) PURE;

    STDMETHOD_(VOID, UpdateByteAvailability)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer,
        IN ULONG Unknown1,
        IN ULONG Unknown2) PURE;

    STDMETHOD_(NTSTATUS, SetStreamPointerStatusCode)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer,
        IN NTSTATUS StatusCode) PURE;

    STDMETHOD_(VOID, RegisterFrameDismissalCallback)(THIS_
        IN KSPSTREAM_POINTER * StreamPointer) PURE;

    STDMETHOD_(UCHAR, GeneratesMappings)(THIS) PURE;

    STDMETHOD_(VOID, CopyFrame)(THIS_
        IN KSPSTREAM_POINTER * Src,
        IN KSPSTREAM_POINTER * Target) PURE;
};

/*****************************************************************************
 * IKsFilterFactory
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsFilter

struct KSPROCESSPIPESECTION;


DECLARE_INTERFACE_(IKsFilter, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(PKSFILTER, GetStruct)(THIS) PURE;

    STDMETHOD_(BOOL, DoAllNecessaryPinsExist)(THIS) PURE;

    STDMETHOD_(NTSTATUS, CreateNode)(THIS_
        IN PIRP Irp,
        IN IKsPin * Pin,
        IN PLIST_ENTRY ListEntry) PURE;

    STDMETHOD_(NTSTATUS, BindProcessPinsToPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *Section,
        IN PVOID Create,
        IN PKSPIN KsPin,
        OUT IKsPin **Pin,
        OUT PKSGATE *OutGate) PURE;

    STDMETHOD_(NTSTATUS, UnbindProcessPinsFromPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *Section) PURE;

    STDMETHOD_(NTSTATUS, AddProcessPin)(THIS_
        IN PKSPROCESSPIN ProcessPin) PURE;

    STDMETHOD_(NTSTATUS, RemoveProcessPin)(THIS_
        IN PKSPROCESSPIN ProcessPin) PURE;

    STDMETHOD_(BOOL, ReprepareProcessPipeSection)(THIS_
        IN struct KSPROCESSPIPESECTION *PipeSection,
        IN PULONG Data) PURE;

    STDMETHOD_(VOID, DeliverResetState)(THIS_
        IN struct KSPROCESSPIPESECTION *PipeSection,
        IN KSRESET ResetState) PURE;

    STDMETHOD_(BOOL, IsFrameHolding)(THIS);

    STDMETHOD_(VOID, RegisterForCopyCallbacks)(THIS_
        IKsQueue * Queue,
        IN BOOL Enable) PURE;

    STDMETHOD_(PKSPROCESSPIN_INDEXENTRY, GetProcessDispatch)(THIS);
};

/*****************************************************************************
 * IKsFilterFactory
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsFilterFactory

DECLARE_INTERFACE_(IKsFilterFactory, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSFILTERFACTORY*, GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS, SetDeviceClassesState)(THIS_
        IN BOOLEAN Enable)PURE;

    STDMETHOD_(NTSTATUS, Initialize)(THIS_
        IN PDEVICE_OBJECT  DeviceObject,
        IN const KSFILTER_DESCRIPTOR  *Descriptor,
        IN PWSTR  RefString OPTIONAL,
        IN PSECURITY_DESCRIPTOR  SecurityDescriptor OPTIONAL,
        IN ULONG  CreateItemFlags,
        IN PFNKSFILTERFACTORYPOWER  SleepCallback OPTIONAL,
        IN PFNKSFILTERFACTORYPOWER  WakeCallback OPTIONAL,
        OUT PKSFILTERFACTORY  *FilterFactory OPTIONAL)PURE;
};


/*****************************************************************************
 * IKsPowerNotify
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsPowerNotify

DECLARE_INTERFACE_(IKsPowerNotify, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(VOID,Sleep)(THIS_
        IN DEVICE_POWER_STATE State) PURE;

    STDMETHOD_(VOID,Wake)(THIS) PURE;
};


/*****************************************************************************
 * IKsDevice
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsDevice

struct KSPOWER_ENTRY;

DECLARE_INTERFACE_(IKsDevice, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(KSDEVICE*,GetStruct)(THIS) PURE;

    STDMETHOD_(NTSTATUS, InitializeObjectBag)(THIS_
        IN PKSIOBJECT_BAG Bag,
        IN PRKMUTEX Mutex) PURE;

    STDMETHOD_(NTSTATUS,AcquireDevice)(THIS) PURE;
    STDMETHOD_(NTSTATUS,ReleaseDevice)(THIS) PURE;

    STDMETHOD_(NTSTATUS, GetAdapterObject)(THIS_
        IN PADAPTER_OBJECT * Object,
        IN PULONG MaxMappingsByteCount,
        IN PULONG MappingTableStride) PURE;

    STDMETHOD_(NTSTATUS, AddPowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry,
        IN IKsPowerNotify* Notify)PURE;

    STDMETHOD_(NTSTATUS, RemovePowerEntry)(THIS_
        IN struct KSPOWER_ENTRY * Entry)PURE;

    STDMETHOD_(NTSTATUS, PinStateChange)(THIS_
        IN KSPIN Pin,
        IN PIRP Irp,
        IN KSSTATE OldState,
        IN KSSTATE NewState)PURE;

    STDMETHOD_(NTSTATUS, ArbitrateAdapterChannel)(THIS_
        IN ULONG NumberOfMapRegisters,
        IN PDRIVER_CONTROL ExecutionRoutine,
        IN PVOID Context)PURE;

    STDMETHOD_(NTSTATUS, CheckIoCapability)(THIS_
        IN ULONG Unknown)PURE;
};

#undef INTERFACE


/*****************************************************************************
 * IKsProcessingObject
 *****************************************************************************
 */

#undef INTERFACE
#define INTERFACE IKsProcessingObject

DECLARE_INTERFACE_(IKsProcessingObject, IUnknown)
{
    DEFINE_ABSTRACT_UNKNOWN()

    STDMETHOD_(VOID, ProcessingObjectWork)(THIS) PURE;

    STDMETHOD_(PKSGATE, GetAndGate)(THIS) PURE;

    STDMETHOD_(VOID, Process)(THIS_
        IN BOOLEAN Asynchronous)PURE;

    STDMETHOD_(VOID, Reset)(THIS) PURE;

    STDMETHOD_(VOID, TriggerNotification)(THIS) PURE;

};

#undef INTERFACE

