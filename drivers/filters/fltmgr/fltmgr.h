#ifndef _FLTMGR_H
#define _FLTMGR_H

#include <ntifs.h>
#include <ndk/obfuncs.h>
#include <ndk/exfuncs.h>
#include <fltkernel.h>
#include <pseh/pseh2.h>

#define DRIVER_NAME     L"FltMgr"

#define FLT_MAJOR_VERSION   0x0200
#define FLT_MINOR_VERSION   0x0000 //win2k3

#define FM_TAG_DISPATCH_TABLE   'ifMF'
#define FM_TAG_REGISTRY_DATA    'rtMF'
#define FM_TAG_DEV_OBJ_PTRS     'ldMF'
#define FM_TAG_UNICODE_STRING   'suMF'
#define FM_TAG_FILTER           'lfMF'
#define FM_TAG_CONTEXT_REGISTA  'rcMF'
#define FM_TAG_CCB              'bcMF'
#define FM_TAG_TEMP_REGISTRY    'rtMF'

#define MAX_DEVNAME_LENGTH  64


typedef struct _DRIVER_DATA
{
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING ServiceKey;

    PDEVICE_OBJECT CommsDeviceObject;

    PFAST_IO_DISPATCH FastIoDispatch;

    FAST_MUTEX FilterAttachLock;

} DRIVER_DATA, *PDRIVER_DATA;

typedef struct _FLTMGR_DEVICE_EXTENSION
{
    /* The file system we're attached to */
    PDEVICE_OBJECT AttachedToDeviceObject;

    /* The storage stack(disk) associated with the file system device object we're attached to */
    PDEVICE_OBJECT StorageStackDeviceObject;

    /* Either physical drive for volume device objects otherwise
    * it's the name of the control device we're attached to */
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[MAX_DEVNAME_LENGTH];

} FLTMGR_DEVICE_EXTENSION, *PFLTMGR_DEVICE_EXTENSION;


NTSTATUS
FltpRegisterContexts(
    _In_ PFLT_FILTER Filter,
    _In_ const FLT_CONTEXT_REGISTRATION *Context
);

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
FltLoadFilter
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
