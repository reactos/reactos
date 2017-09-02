#ifndef _FLTMGR_INTERNAL_H
#define _FLTMGR_INTERNAL_H


#define MAX_CONTEXT_TYPES   6


typedef enum _FLT_OBJECT_FLAGS
{
    FLT_OBFL_DRAINING = 1,
    FLT_OBFL_ZOMBIED = 2,
    FLT_OBFL_TYPE_INSTANCE = 0x1000000,
    FLT_OBFL_TYPE_FILTER = 0x2000000,
    FLT_OBFL_TYPE_VOLUME = 0x4000000

} FLT_OBJECT_FLAGS, *PFLT_OBJECT_FLAGS;

typedef enum _FLT_FILTER_FLAGS
{
    FLTFL_MANDATORY_UNLOAD_IN_PROGRESS = 1,
    FLTFL_FILTERING_INITIATED = 2

} FLT_FILTER_FLAGS, *PFLT_FILTER_FLAGS;

typedef struct _FLT_OBJECT   // size = 0x14
{
    volatile FLT_OBJECT_FLAGS Flags;
    ULONG PointerCount;
    EX_RUNDOWN_REF RundownRef;
    LIST_ENTRY PrimaryLink;

} FLT_OBJECT, *PFLT_OBJECT;

typedef struct _ALLOCATE_CONTEXT_HEADER
{
    PFLT_FILTER Filter;
    PFLT_CONTEXT_CLEANUP_CALLBACK ContextCleanupCallback;
    struct _ALLOCATE_CONTEXT_HEADER *Next;
    FLT_CONTEXT_TYPE ContextType;
    char Flags;
    char AllocationType;

} ALLOCATE_CONTEXT_HEADER, *PALLOCATE_CONTEXT_HEADER;

typedef struct _FLT_RESOURCE_LIST_HEAD
{
    ERESOURCE rLock;
    LIST_ENTRY rList;
    ULONG rCount;

} FLT_RESOURCE_LIST_HEAD, *PFLT_RESOURCE_LIST_HEAD;

typedef struct _FLT_MUTEX_LIST_HEAD
{
    FAST_MUTEX mLock;
    LIST_ENTRY mList;
    ULONG mCount;

} FLT_MUTEX_LIST_HEAD, *PFLT_MUTEX_LIST_HEAD;

typedef struct _FLT_FILTER   // size = 0x120
{
    FLT_OBJECT Base;
    PVOID Frame;  //FLTP_FRAME
    UNICODE_STRING Name;
    UNICODE_STRING DefaultAltitude;
    FLT_FILTER_FLAGS Flags;
    PDRIVER_OBJECT DriverObject;
    FLT_RESOURCE_LIST_HEAD InstanceList;
    PVOID VerifierExtension;
    PFLT_FILTER_UNLOAD_CALLBACK FilterUnload;
    PFLT_INSTANCE_SETUP_CALLBACK InstanceSetup;
    PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK InstanceQueryTeardown;
    PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownStart;
    PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownComplete;
    PALLOCATE_CONTEXT_HEADER SupportedContextsListHead;
    PALLOCATE_CONTEXT_HEADER SupportedContexts[MAX_CONTEXT_TYPES];
    PVOID PreVolumeMount;
    PVOID PostVolumeMount;
    PFLT_GENERATE_FILE_NAME GenerateFileName;
    PFLT_NORMALIZE_NAME_COMPONENT NormalizeNameComponent;
    PFLT_NORMALIZE_CONTEXT_CLEANUP NormalizeContextCleanup;
    PFLT_OPERATION_REGISTRATION Operations;
    PFLT_FILTER_UNLOAD_CALLBACK OldDriverUnload;
    FLT_MUTEX_LIST_HEAD ActiveOpens;
    FLT_MUTEX_LIST_HEAD ConnectionList;
    FLT_MUTEX_LIST_HEAD PortList;
    EX_PUSH_LOCK PortLock;

}  FLT_FILTER, *PFLT_FILTER;

typedef enum _FLT_yINSTANCE_FLAGS
{
    INSFL_CAN_BE_DETACHED = 0x01,
    INSFL_DELETING = 0x02,
    INSFL_INITING = 0x04

} FLT_INSTANCE_FLAGS, *PFLT_INSTANCE_FLAGS;

typedef struct _FLT_TYPE
{
    USHORT Signature;
    USHORT Size;

} FLT_TYPE, *PFLT_TYPE;

typedef struct _FLT_INSTANCE   // size = 0x144 (324)
{
    FLT_OBJECT Base;
    ULONG OperationRundownRef;
    PVOID Volume; //PFLT_VOLUME
    PFLT_FILTER Filter;
    FLT_INSTANCE_FLAGS Flags;
    UNICODE_STRING Altitude;
    UNICODE_STRING Name;
    LIST_ENTRY FilterLink;
    ERESOURCE ContextLock;
    PVOID Context; //PCONTEXT_NODE
    PVOID TrackCompletionNodes; //PRACK_COMPLETION_NODES
    PVOID CallbackNodes[50]; //PCALLBACK_NODE

} FLT_INSTANCE, *PFLT_INSTANCE;

// http://fsfilters.blogspot.co.uk/2010/02/filter-manager-concepts-part-1.html
typedef struct _FLTP_FRAME
{
    FLT_TYPE Type;
    LIST_ENTRY Links;
    unsigned int FrameID;
    ERESOURCE AltitudeLock;
    UNICODE_STRING AltitudeIntervalLow;
    UNICODE_STRING AltitudeIntervalHigh;
    char LargeIrpCtrlStackSize;
    char SmallIrpCtrlStackSize;
    FLT_RESOURCE_LIST_HEAD RegisteredFilters;
    FLT_RESOURCE_LIST_HEAD AttachedVolumes;
    LIST_ENTRY MountingVolumes;
    FLT_MUTEX_LIST_HEAD AttachedFileSystems;
    FLT_MUTEX_LIST_HEAD ZombiedFltObjectContexts;
    ERESOURCE FilterUnloadLock;
    FAST_MUTEX DeviceObjectAttachLock;
    //FLT_PRCB *Prcb;
    void *PrcbPoolToFree;
    void *LookasidePoolToFree;
    //FLTP_IRPCTRL_STACK_PROFILER IrpCtrlStackProfiler;
    NPAGED_LOOKASIDE_LIST SmallIrpCtrlLookasideList;
    NPAGED_LOOKASIDE_LIST LargeIrpCtrlLookasideList;
    //STATIC_IRP_CONTROL GlobalSIC;

} FLTP_FRAME, *PFLTP_FRAME;


// http://fsfilters.blogspot.co.uk/2010/02/filter-manager-concepts-part-6.html
typedef struct _STREAM_LIST_CTRL // size = 0xC8 (200)
{
    FLT_TYPE Type;
    FSRTL_PER_STREAM_CONTEXT ContextCtrl;
    LIST_ENTRY VolumeLink;
    //STREAM_LIST_CTRL_FLAGS Flags;
    int UseCount;
    ERESOURCE ContextLock;
    //CONTEXT_LIST_CTRL StreamContexts;
    //CONTEXT_LIST_CTRL StreamHandleContexts;
    ERESOURCE NameCacheLock;
    LARGE_INTEGER LastRenameCompleted;
    //NAME_CACHE_LIST_CTRL NormalizedNameCache;
   // NAME_CACHE_LIST_CTRL ShortNameCache;
   // NAME_CACHE_LIST_CTRL OpenedNameCache;
    int AllNameContextsTemporary;

} STREAM_LIST_CTRL, *PSTREAM_LIST_CTRL;


typedef struct _FLT_SERVER_PORT_OBJECT
{
    LIST_ENTRY FilterLink;
    PFLT_CONNECT_NOTIFY ConnectNotify;
    PFLT_DISCONNECT_NOTIFY DisconnectNotify;
    PFLT_MESSAGE_NOTIFY MessageNotify;
    PFLT_FILTER Filter;
    PVOID Cookie;
    ULONG Flags;
    LONG NumberOfConnections;
    LONG MaxConnections;

} FLT_SERVER_PORT_OBJECT, *PFLT_SERVER_PORT_OBJECT;


typedef struct _FLT_PORT_OBJECT
{
    LIST_ENTRY FilterLink;
    PFLT_SERVER_PORT_OBJECT ServerPort;
    PVOID Cookie;
    EX_RUNDOWN_REF MsgNotifRundownRef;
    FAST_MUTEX Lock;
    PVOID MsgQ; // FLT_MESSAGE_WAITER_QUEUE MsgQ;
    ULONGLONG MessageId;
    KEVENT DisconnectEvent;
    BOOLEAN Disconnected;

} FLT_PORT_OBJECT, *PFLT_PORT_OBJECT;





VOID
FltpExInitializeRundownProtection(
    _Out_ PEX_RUNDOWN_REF RundownRef
);

BOOLEAN
FltpExAcquireRundownProtection(
    _Inout_ PEX_RUNDOWN_REF RundownRef
);

BOOLEAN
FltpExReleaseRundownProtection(
    _Inout_ PEX_RUNDOWN_REF RundownRef
);

NTSTATUS
NTAPI
FltpObjectRundownWait(
    _Inout_ PEX_RUNDOWN_REF RundownRef
);

BOOLEAN
FltpExRundownCompleted(
    _Inout_ PEX_RUNDOWN_REF RundownRef
);


NTSTATUS
FltpGetBaseDeviceObjectName(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PUNICODE_STRING ObjectName
);

NTSTATUS
FltpGetObjectName(
    _In_ PVOID Object,
    _Inout_ PUNICODE_STRING ObjectName
);

ULONG
FltpObjectPointerReference(
    _In_ PFLT_OBJECT Object
);

VOID
FltpObjectPointerDereference(
    _In_ PFLT_OBJECT Object
);

NTSTATUS
FltpReallocateUnicodeString(
    _In_ PUNICODE_STRING String,
    _In_ SIZE_T NewLength,
    _In_ BOOLEAN CopyExisting
);

VOID
FltpFreeUnicodeString(
    _In_ PUNICODE_STRING String
);



NTSTATUS
FltpDeviceControlHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
);

NTSTATUS
FltpDispatchHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
);



#endif /* _FLTMGR_INTERNAL_H */
