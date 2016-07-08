#ifndef _FLTMGR_H
#define _FLTMGR_H

// Hack - our SDK reports NTDDI_VERSION as 0x05020100 (from _WIN32_WINNT 0x502)
// which doesn't pass the FLT_MGR_BASELINE check in fltkernel.h
#define NTDDI_VERSION NTDDI_WS03SP1

#include <ntifs.h>
#include <fltkernel.h>
#include <pseh/pseh2.h>

#define DRIVER_NAME     L"RosFltMgr"

#define FLT_MAJOR_VERSION   0x02
#define FLT_MINOR_VERSION   0x00 //win2k3

#define FM_TAG_DISPATCH_TABLE   'ifMF'
#define FM_TAG_REGISTRY_DATA    'rtMF'
#define FM_TAG_DEV_OBJ_PTRS     'ldMF'
#define FM_TAG_UNICODE_STRING   'suMF'
#define FM_TAG_FILTER           'lfMF'


typedef struct _DRIVER_DATA
{
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING ServiceKey;

    PFAST_IO_DISPATCH FastIoDispatch;

    FAST_MUTEX FilterAttachLock;

} DRIVER_DATA, *PDRIVER_DATA;


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
    PVOID SupportedContextsListHead;    //PALLOCATE_CONTEXT_HEADER
    PVOID SupportedContexts;            //PALLOCATE_CONTEXT_HEADER
    PVOID PreVolumeMount;
    PVOID PostVolumeMount;
    PFLT_GENERATE_FILE_NAME GenerateFileName;
    PFLT_NORMALIZE_NAME_COMPONENT NormalizeNameComponent;
    PFLT_NORMALIZE_CONTEXT_CLEANUP NormalizeContextCleanup;
    PFLT_OPERATION_REGISTRATION Operations;
    PFLT_FILTER_UNLOAD_CALLBACK OldDriverUnload;
    FLT_MUTEX_LIST_HEAD ActiveOpens;
    FLT_MUTEX_LIST_HEAD PortList;
    EX_PUSH_LOCK PortLock;

}  FLT_FILTER, *PFLT_FILTER;

typedef enum _FLT_INSTANCE_FLAGS
{
    INSFL_CAN_BE_DETACHED = 0x01,
    INSFL_DELETING = 0x02,
    INSFL_INITING = 0x04

} FLT_INSTANCE_FLAGS, *PFLT_INSTANCE_FLAGS;

typedef struct _FLT_INSTANCE   // size = 0x144
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
    PVOID Context;
    PVOID TrackCompletionNodes;
    PVOID CallbackNodes;

} FLT_INSTANCE, *PFLT_INSTANCE;





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

/////////// FIXME: put these into the correct header
NTSTATUS
FltpGetBaseDeviceObjectName(_In_ PDEVICE_OBJECT DeviceObject,
                            _Inout_ PUNICODE_STRING ObjectName);

NTSTATUS
FltpGetObjectName(_In_ PVOID Object,
                  _Inout_ PUNICODE_STRING ObjectName);

NTSTATUS
FltpReallocateUnicodeString(_In_ PUNICODE_STRING String,
                            _In_ SIZE_T NewLength,
                            _In_ BOOLEAN CopyExisting);

VOID
FltpFreeUnicodeString(_In_ PUNICODE_STRING String);
////////////////////////////////////////////////












//FM ? ? -fltmgr.sys - Unrecognized FltMgr tag(update pooltag.w)
//FMac - fltmgr.sys - ASCII String buffers
//FMas - fltmgr.sys - ASYNC_IO_COMPLETION_CONTEXT structure
//FMcb - fltmgr.sys - FLT_CCB structure
//FMcr - fltmgr.sys - Context registration structures
//FMct - fltmgr.sys - TRACK_COMPLETION_NODES structure
//FMdl - fltmgr.sys - Array of DEVICE_OBJECT pointers
//FMea - fltmgr.sys - EA buffer for create
//FMfc - fltmgr.sys - FLTMGR_FILE_OBJECT_CONTEXT structure
//FMfi - fltmgr.sys - Fast IO dispatch table
//FMfk - fltmgr.sys - Byte Range Lock structure
//FMfl - fltmgr.sys - FLT_FILTER structure
//FMfn - fltmgr.sys - NAME_CACHE_NODE structure
//FMfr - fltmgr.sys - FLT_FRAME structure
//FMfz - fltmgr.sys - FILE_LIST_CTRL structure
//FMib - fltmgr.sys - Irp SYSTEM buffers
//FMic - fltmgr.sys - IRP_CTRL structure
//FMin - fltmgr.sys - FLT_INSTANCE name
//FMil - fltmgr.sys - IRP_CTRL completion node stack
//FMis - fltmgr.sys - FLT_INSTANCE structure
//FMla - fltmgr.sys - Per - processor IRPCTRL lookaside lists
//FMnc - fltmgr.sys - NAME_CACHE_CREATE_CTRL structure
//FMng - fltmgr.sys - NAME_GENERATION_CONTEXT structure
//FMol - fltmgr.sys - OPLOCK_CONTEXT structure
//FMos - fltmgr.sys - Operation status ctrl structure
//FMpl - fltmgr.sys - Cache aware pushLock
//FMpr - fltmgr.sys - FLT_PRCB structure
//FMrl - fltmgr.sys - FLT_OBJECT rundown logs
//FMrp - fltmgr.sys - Reparse point data buffer
//FMrr - fltmgr.sys - Per - processor Cache - aware rundown ref structure
//FMsd - fltmgr.sys - Security descriptors
//FMsl - fltmgr.sys - STREAM_LIST_CTRL structure
//FMtn - fltmgr.sys - Temporary file names
//FMtr - fltmgr.sys - Temporary Registry information
//FMts - fltmgr.sys - Tree Stack
//FMus - fltmgr.sys - Unicode string
//FMvf - fltmgr.sys - FLT_VERIFIER_EXTENSION structure
//FMvj - fltmgr.sys - FLT_VERIFIER_OBJECT structure
//FMvo - fltmgr.sys - FLT_VOLUME structure
//FMwi - fltmgr.sys - Work item structures
//FMcn - fltmgr.sys - Non paged context extension structures.
//FMtp - fltmgr.sys - Non Paged tx vol context structures.
//FMlp - fltmgr.sys - Paged stream list control entry structures.




/*
FltAcquirePushLockExclusive
FltAcquirePushLockShared
FltAcquireResourceExclusive
FltAcquireResourceShared
FltAllocateCallbackData
FltAllocateContext
FltAllocateDeferredIoWorkItem
FltAllocateFileLock
FltAllocateGenericWorkItem
FltAllocatePoolAlignedWithTag
FltAttachVolume
FltAttachVolumeAtAltitude
FltBuildDefaultSecurityDescriptor
FltCancelFileOpen
FltCancelIo
FltCbdqDisable
FltCbdqEnable
FltCbdqInitialize
FltCbdqInsertIo
FltCbdqRemoveIo
FltCbdqRemoveNextIo
FltCheckAndGrowNameControl
FltCheckLockForReadAccess
FltCheckLockForWriteAccess
FltCheckOplock
FltClearCallbackDataDirty
FltClearCancelCompletion
FltClose
FltCloseClientPort
FltCloseCommunicationPort
FltCompareInstanceAltitudes
FltCompletePendedPostOperation
FltCompletePendedPreOperation
FltCreateCommunicationPort
FltCreateFile
FltCreateFileEx
FltCreateSystemVolumeInformationFolder
FltCurrentBatchOplock
FltDecodeParameters
FltDeleteContext
FltDeleteFileContext
FltDeleteInstanceContext
FltDeletePushLock
FltDeleteStreamContext
FltDeleteStreamHandleContext
FltDeleteVolumeContext
FltDetachVolume
FltDeviceIoControlFile
FltDoCompletionProcessingWhenSafe
FltEnumerateFilterInformation
FltEnumerateFilters
FltEnumerateInstanceInformationByFilter
FltEnumerateInstanceInformationByVolume
FltEnumerateInstances
FltEnumerateVolumeInformation
FltEnumerateVolumes
FltFlushBuffers
FltFreeCallbackData
FltFreeDeferredIoWorkItem
FltFreeFileLock
FltFreeGenericWorkItem
FltFreePoolAlignedWithTag
FltFreeSecurityDescriptor
FltFsControlFile
FltGetBottomInstance
FltGetContexts
FltGetDestinationFileNameInformation
FltGetDeviceObject
FltGetDiskDeviceObject
FltGetFileContext
FltGetFileNameInformation
FltGetFileNameInformationUnsafe
FltGetFilterFromInstance
FltGetFilterFromName
FltGetFilterInformation
FltGetInstanceContext
FltGetInstanceInformation
FltGetIrpName
FltGetLowerInstance
FltGetRequestorProcess
FltGetRequestorProcessId
FltGetRoutineAddress
FltGetStreamContext
FltGetStreamHandleContext
FltGetSwappedBufferMdlAddress
FltGetTopInstance
FltGetTunneledName
FltGetUpperInstance
FltGetVolumeContext
FltGetVolumeFromDeviceObject
FltGetVolumeFromFileObject
FltGetVolumeFromInstance
FltGetVolumeFromName
FltGetVolumeGuidName
FltGetVolumeInstanceFromName
FltGetVolumeName
FltGetVolumeProperties
FltInitializeFileLock
FltInitializeOplock
FltInitializePushLock
FltIs32bitProcess
FltIsCallbackDataDirty
FltIsDirectory
FltIsIoCanceled
FltIsOperationSynchronous
FltIsVolumeWritable
FltLoadFilter
FltLockUserBuffer
FltNotifyFilterChangeDirectory
FltObjectDereference
FltObjectReference
FltOpenVolume
FltOplockFsctrl
FltOplockIsFastIoPossible
FltParseFileName
FltParseFileNameInformation
FltPerformAsynchronousIo
FltPerformSynchronousIo
FltProcessFileLock
FltPurgeFileNameInformationCache
FltQueryEaFile
FltQueryInformationFile
FltQuerySecurityObject
FltQueryVolumeInformation
FltQueryVolumeInformationFile
FltQueueDeferredIoWorkItem
FltQueueGenericWorkItem
FltReadFile
FltReferenceContext
FltReferenceFileNameInformation
FltRegisterFilter
FltReissueSynchronousIo
FltReleaseContext
FltReleaseContexts
FltReleaseFileNameInformation
FltReleasePushLock
FltReleaseResource
FltRequestOperationStatusCallback
FltRetainSwappedBufferMdlAddress
FltReuseCallbackData
FltSendMessage
FltSetCallbackDataDirty
FltSetCancelCompletion
FltSetEaFile
FltSetFileContext
FltSetInformationFile
FltSetInstanceContext
FltSetSecurityObject
FltSetStreamContext
FltSetStreamHandleContext
FltSetVolumeContext
FltSetVolumeInformation
FltStartFiltering
FltSupportsFileContexts
FltSupportsStreamContexts
FltSupportsStreamHandleContexts
FltTagFile
FltUninitializeFileLock
FltUninitializeOplock
FltUnloadFilter
FltUnregisterFilter
FltUntagFile
FltWriteFile
*/




#endif /* _FLTMGR_H */
