/*
 * fltkernel.h
 *
 * This file is part of the ReactOS DDK package.
 *
 * Contributors:
 *   Amine Khaldi (amine.khaldi@reactos.org)
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#pragma once

#ifndef __FLTKERNEL__
#define __FLTKERNEL__

#ifdef __cplusplus
extern "C" {
#endif

#define FLT_MGR_BASELINE (((OSVER(NTDDI_VERSION) == NTDDI_WIN2K) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WIN2KSP4))) || \
                          ((OSVER(NTDDI_VERSION) == NTDDI_WINXP) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WINXPSP2))) || \
                          ((OSVER(NTDDI_VERSION) == NTDDI_WS03)  && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WS03SP1)))  || \
                          (NTDDI_VERSION >= NTDDI_VISTA))

#define FLT_MGR_AFTER_XPSP2 (((OSVER(NTDDI_VERSION) == NTDDI_WIN2K) && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WIN2KSP4))) || \
                             ((OSVER(NTDDI_VERSION) == NTDDI_WINXP) && (SPVER(NTDDI_VERSION) >  SPVER(NTDDI_WINXPSP2))) || \
                             ((OSVER(NTDDI_VERSION) == NTDDI_WS03)  && (SPVER(NTDDI_VERSION) >= SPVER(NTDDI_WS03SP1)))  || \
                             (NTDDI_VERSION >= NTDDI_VISTA))

#define FLT_MGR_LONGHORN (NTDDI_VERSION >= NTDDI_VISTA)
#define FLT_MGR_WIN7 (NTDDI_VERSION >= NTDDI_WIN7)

#include <ntifs.h>
#include <fltuserstructures.h>
#include <initguid.h>

#if FLT_MGR_BASELINE

#if FLT_MGR_LONGHORN
#define FLT_ASSERT(_e) NT_ASSERT(_e)
#define FLT_ASSERTMSG(_m, _e) NT_ASSERTMSG(_m, _e)
#else
#define FLT_ASSERT(_e) ASSERT(_e)
#define FLT_ASSERTMSG(_m, _e) ASSERTMSG(_m, _e)
#endif /* FLT_MGR_LONGHORN */

#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#define PtrOffset(B,O) ((ULONG)((ULONG_PTR)(O) - (ULONG_PTR)(B)))

#define ROUND_TO_SIZE(_length, _alignment) \
  ((((ULONG_PTR)(_length)) + ((_alignment)-1)) & ~(ULONG_PTR) ((_alignment) - 1))

#define IS_ALIGNED(_pointer, _alignment) \
  ((((ULONG_PTR) (_pointer)) & ((_alignment) - 1)) == 0)

#define IRP_MJ_ACQUIRE_FOR_SECTION_SYNCHRONIZATION ((UCHAR)-1)
#define IRP_MJ_RELEASE_FOR_SECTION_SYNCHRONIZATION ((UCHAR)-2)
#define IRP_MJ_ACQUIRE_FOR_MOD_WRITE               ((UCHAR)-3)
#define IRP_MJ_RELEASE_FOR_MOD_WRITE               ((UCHAR)-4)
#define IRP_MJ_ACQUIRE_FOR_CC_FLUSH                ((UCHAR)-5)
#define IRP_MJ_RELEASE_FOR_CC_FLUSH                ((UCHAR)-6)
#define IRP_MJ_FAST_IO_CHECK_IF_POSSIBLE           ((UCHAR)-13)
#define IRP_MJ_NETWORK_QUERY_OPEN                  ((UCHAR)-14)
#define IRP_MJ_MDL_READ                            ((UCHAR)-15)
#define IRP_MJ_MDL_READ_COMPLETE                   ((UCHAR)-16)
#define IRP_MJ_PREPARE_MDL_WRITE                   ((UCHAR)-17)
#define IRP_MJ_MDL_WRITE_COMPLETE                  ((UCHAR)-18)
#define IRP_MJ_VOLUME_MOUNT                        ((UCHAR)-19)
#define IRP_MJ_VOLUME_DISMOUNT                     ((UCHAR)-20)
#define IRP_MJ_OPERATION_END                       ((UCHAR)0x80)
#define FLT_INTERNAL_OPERATION_COUNT               22

#define NULL_CONTEXT ((PFLT_CONTEXT)NULL)

typedef struct _FLT_FILTER *PFLT_FILTER;
typedef struct _FLT_VOLUME *PFLT_VOLUME;
typedef struct _FLT_INSTANCE *PFLT_INSTANCE;
typedef struct _FLT_PORT *PFLT_PORT;

typedef PVOID PFLT_CONTEXT;

#if !FLT_MGR_LONGHORN
typedef struct _KTRANSACTION *PKTRANSACTION;
#endif

#if !defined(_AMD64_) && !defined(_IA64_) && !defined(_ARM_)
#include "pshpack4.h"
#endif

typedef union _FLT_PARAMETERS {
  struct {
    PIO_SECURITY_CONTEXT SecurityContext;
    ULONG Options;
    USHORT POINTER_ALIGNMENT FileAttributes;
    USHORT ShareAccess;
    ULONG POINTER_ALIGNMENT EaLength;
    PVOID EaBuffer;
    LARGE_INTEGER AllocationSize;
  } Create;
  struct {
    PIO_SECURITY_CONTEXT SecurityContext;
    ULONG Options;
    USHORT POINTER_ALIGNMENT Reserved;
    USHORT ShareAccess;
    PVOID Parameters;
  } CreatePipe;
  struct {
    PIO_SECURITY_CONTEXT SecurityContext;
    ULONG Options;
    USHORT POINTER_ALIGNMENT Reserved;
    USHORT ShareAccess;
    PVOID Parameters;
  } CreateMailslot;
  struct {
    ULONG Length;
    ULONG POINTER_ALIGNMENT Key;
    LARGE_INTEGER ByteOffset;
    PVOID ReadBuffer;
    PMDL MdlAddress;
  } Read;
  struct {
    ULONG Length;
    ULONG POINTER_ALIGNMENT Key;
    LARGE_INTEGER ByteOffset;
    PVOID WriteBuffer;
    PMDL MdlAddress;
  } Write;
  struct {
    ULONG Length;
    FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
    PVOID InfoBuffer;
  } QueryFileInformation;
  struct {
    ULONG Length;
    FILE_INFORMATION_CLASS POINTER_ALIGNMENT FileInformationClass;
    PFILE_OBJECT ParentOfTarget;
    _ANONYMOUS_UNION union {
      _ANONYMOUS_STRUCT struct {
        BOOLEAN ReplaceIfExists;
        BOOLEAN AdvanceOnly;
      } DUMMYSTRUCTNAME;
      ULONG ClusterCount;
      HANDLE DeleteHandle;
    } DUMMYUNIONNAME;
    PVOID InfoBuffer;
  } SetFileInformation;
  struct {
    ULONG Length;
    PVOID EaList;
    ULONG EaListLength;
    ULONG POINTER_ALIGNMENT EaIndex;
    PVOID EaBuffer;
    PMDL MdlAddress;
  } QueryEa;
  struct {
    ULONG Length;
    PVOID EaBuffer;
    PMDL MdlAddress;
  } SetEa;
  struct {
    ULONG Length;
    FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
    PVOID VolumeBuffer;
  } QueryVolumeInformation;
  struct {
    ULONG Length;
    FS_INFORMATION_CLASS POINTER_ALIGNMENT FsInformationClass;
    PVOID VolumeBuffer;
  } SetVolumeInformation;
  union {
    struct {
      ULONG Length;
      PUNICODE_STRING FileName;
      FILE_INFORMATION_CLASS FileInformationClass;
      ULONG POINTER_ALIGNMENT FileIndex;
      PVOID DirectoryBuffer;
      PMDL MdlAddress;
    } QueryDirectory;
    struct {
      ULONG Length;
      ULONG POINTER_ALIGNMENT CompletionFilter;
      ULONG POINTER_ALIGNMENT Spare1;
      ULONG POINTER_ALIGNMENT Spare2;
      PVOID DirectoryBuffer;
      PMDL MdlAddress;
    } NotifyDirectory;
  } DirectoryControl;
  union {
    struct {
      PVPB Vpb;
      PDEVICE_OBJECT DeviceObject;
    } VerifyVolume;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT FsControlCode;
    } Common;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT FsControlCode;
      PVOID InputBuffer;
      PVOID OutputBuffer;
      PMDL OutputMdlAddress;
    } Neither;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT FsControlCode;
      PVOID SystemBuffer;
    } Buffered;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT FsControlCode;
      PVOID InputSystemBuffer;
      PVOID OutputBuffer;
      PMDL OutputMdlAddress;
    } Direct;
  } FileSystemControl;
  union {
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
    } Common;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID InputBuffer;
      PVOID OutputBuffer;
      PMDL OutputMdlAddress;
    } Neither;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID SystemBuffer;
    } Buffered;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID InputSystemBuffer;
      PVOID OutputBuffer;
      PMDL OutputMdlAddress;
    } Direct;
    struct {
      ULONG OutputBufferLength;
      ULONG POINTER_ALIGNMENT InputBufferLength;
      ULONG POINTER_ALIGNMENT IoControlCode;
      PVOID InputBuffer;
      PVOID OutputBuffer;
    } FastIo;
  } DeviceIoControl;
  struct {
    PLARGE_INTEGER Length;
    ULONG POINTER_ALIGNMENT Key;
    LARGE_INTEGER ByteOffset;
    PEPROCESS ProcessId;
    BOOLEAN FailImmediately;
    BOOLEAN ExclusiveLock;
  } LockControl;
  struct {
    SECURITY_INFORMATION SecurityInformation;
    ULONG POINTER_ALIGNMENT Length;
    PVOID SecurityBuffer;
    PMDL MdlAddress;
  } QuerySecurity;
  struct {
    SECURITY_INFORMATION SecurityInformation;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
  } SetSecurity;
  struct {
    ULONG_PTR ProviderId;
    PVOID DataPath;
    ULONG BufferSize;
    PVOID Buffer;
  } WMI;
  struct {
    ULONG Length;
    PSID StartSid;
    PFILE_GET_QUOTA_INFORMATION SidList;
    ULONG SidListLength;
    PVOID QuotaBuffer;
    PMDL MdlAddress;
  } QueryQuota;
  struct {
    ULONG Length;
    PVOID QuotaBuffer;
    PMDL MdlAddress;
  } SetQuota;
  union {
    struct {
      PCM_RESOURCE_LIST AllocatedResources;
      PCM_RESOURCE_LIST AllocatedResourcesTranslated;
    } StartDevice;
    struct {
      DEVICE_RELATION_TYPE Type;
    } QueryDeviceRelations;
    struct {
      CONST GUID *InterfaceType;
      USHORT Size;
      USHORT Version;
      PINTERFACE Interface;
      PVOID InterfaceSpecificData;
    } QueryInterface;
    struct {
      PDEVICE_CAPABILITIES Capabilities;
    } DeviceCapabilities;
    struct {
      PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementList;
    } FilterResourceRequirements;
    struct {
      ULONG WhichSpace;
      PVOID Buffer;
      ULONG Offset;
      ULONG POINTER_ALIGNMENT Length;
    } ReadWriteConfig;
    struct {
      BOOLEAN Lock;
    } SetLock;
    struct {
      BUS_QUERY_ID_TYPE IdType;
    } QueryId;
    struct {
      DEVICE_TEXT_TYPE DeviceTextType;
      LCID POINTER_ALIGNMENT LocaleId;
    } QueryDeviceText;
    struct {
      BOOLEAN InPath;
      BOOLEAN Reserved[3];
      DEVICE_USAGE_NOTIFICATION_TYPE POINTER_ALIGNMENT Type;
    } UsageNotification;
  } Pnp;
  struct {
    FS_FILTER_SECTION_SYNC_TYPE SyncType;
    ULONG PageProtection;
  } AcquireForSectionSynchronization;
  struct {
    PLARGE_INTEGER EndingOffset;
    PERESOURCE *ResourceToRelease;
  } AcquireForModifiedPageWriter;
  struct {
    PERESOURCE ResourceToRelease;
  } ReleaseForModifiedPageWriter;
  struct {
    LARGE_INTEGER FileOffset;
    ULONG Length;
    ULONG POINTER_ALIGNMENT LockKey;
    BOOLEAN POINTER_ALIGNMENT CheckForReadOperation;
  } FastIoCheckIfPossible;
  struct {
    PIRP Irp;
    PFILE_NETWORK_OPEN_INFORMATION NetworkInformation;
  } NetworkQueryOpen;
  struct {
    LARGE_INTEGER FileOffset;
    ULONG POINTER_ALIGNMENT Length;
    ULONG POINTER_ALIGNMENT Key;
    PMDL *MdlChain;
  } MdlRead;
  struct {
    PMDL MdlChain;
  } MdlReadComplete;
  struct {
    LARGE_INTEGER FileOffset;
    ULONG POINTER_ALIGNMENT Length;
    ULONG POINTER_ALIGNMENT Key;
    PMDL *MdlChain;
  } PrepareMdlWrite;
  struct {
    LARGE_INTEGER FileOffset;
    PMDL MdlChain;
  } MdlWriteComplete;
  struct {
    ULONG DeviceType;
  } MountVolume;
  struct {
    PVOID Argument1;
    PVOID Argument2;
    PVOID Argument3;
    PVOID Argument4;
    PVOID Argument5;
    LARGE_INTEGER Argument6;
  } Others;
} FLT_PARAMETERS, *PFLT_PARAMETERS;

#if !defined(_AMD64_) && !defined(_IA64_) && !defined(_ARM_)
#include "poppack.h"
#endif

typedef struct _FLT_IO_PARAMETER_BLOCK {
  ULONG IrpFlags;
  UCHAR MajorFunction;
  UCHAR MinorFunction;
  UCHAR OperationFlags;
  UCHAR Reserved;
  PFILE_OBJECT TargetFileObject;
  PFLT_INSTANCE TargetInstance;
  FLT_PARAMETERS Parameters;
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;

#define FLTFL_CALLBACK_DATA_REISSUE_MASK        0x0000FFFF
#define FLTFL_CALLBACK_DATA_IRP_OPERATION       0x00000001
#define FLTFL_CALLBACK_DATA_FAST_IO_OPERATION   0x00000002
#define FLTFL_CALLBACK_DATA_FS_FILTER_OPERATION 0x00000004
#define FLTFL_CALLBACK_DATA_SYSTEM_BUFFER       0x00000008
#define FLTFL_CALLBACK_DATA_GENERATED_IO        0x00010000
#define FLTFL_CALLBACK_DATA_REISSUED_IO         0x00020000
#define FLTFL_CALLBACK_DATA_DRAINING_IO         0x00040000
#define FLTFL_CALLBACK_DATA_POST_OPERATION      0x00080000
#define FLTFL_CALLBACK_DATA_NEW_SYSTEM_BUFFER   0x00100000
#define FLTFL_CALLBACK_DATA_DIRTY               0x80000000

#define FLT_SET_CALLBACK_DATA_DIRTY(Data)   FltSetCallbackDataDirty(Data)
#define FLT_CLEAR_CALLBACK_DATA_DIRTY(Data) FltClearCallbackDataDirty(Data)
#define FLT_IS_CALLBACK_DATA_DIRTY(Data)    FltIsCallbackDataDirty(Data)

#define FLT_IS_IRP_OPERATION(Data)       (FlagOn((Data)->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION))
#define FLT_IS_FASTIO_OPERATION(Data)    (FlagOn((Data)->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
#define FLT_IS_FS_FILTER_OPERATION(Data) (FlagOn((Data)->Flags, FLTFL_CALLBACK_DATA_FS_FILTER_OPERATION))
#define FLT_IS_REISSUED_IO(Data)         (FlagOn((Data)->Flags, FLTFL_CALLBACK_DATA_REISSUED_IO))
#define FLT_IS_SYSTEM_BUFFER(Data)       (FlagOn((Data)->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER))

typedef USHORT FLT_CONTEXT_TYPE;

#define FLT_VOLUME_CONTEXT       0x0001
#define FLT_INSTANCE_CONTEXT     0x0002
#define FLT_FILE_CONTEXT         0x0004
#define FLT_STREAM_CONTEXT       0x0008
#define FLT_STREAMHANDLE_CONTEXT 0x0010
#define FLT_TRANSACTION_CONTEXT  0x0020
#define FLT_CONTEXT_END          0xffff

#define FLT_ALL_CONTEXTS (FLT_VOLUME_CONTEXT | FLT_INSTANCE_CONTEXT | \
                          FLT_FILE_CONTEXT | FLT_STREAM_CONTEXT |     \
                          FLT_STREAMHANDLE_CONTEXT | FLT_TRANSACTION_CONTEXT)

typedef ULONG FLT_CALLBACK_DATA_FLAGS;

#if FLT_MGR_WIN7
typedef ULONG FLT_ALLOCATE_CALLBACK_DATA_FLAGS;
#define FLT_ALLOCATE_CALLBACK_DATA_PREALLOCATE_ALL_MEMORY 0x00000001
#endif /* FLT_MGR_WIN7 */

typedef struct _FLT_CALLBACK_DATA {
  FLT_CALLBACK_DATA_FLAGS Flags;
  PETHREAD CONST Thread;
  PFLT_IO_PARAMETER_BLOCK CONST Iopb;
  IO_STATUS_BLOCK IoStatus;
  struct _FLT_TAG_DATA_BUFFER *TagData;
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      LIST_ENTRY QueueLinks;
      PVOID QueueContext[2];
    } DUMMYSTRUCTNAME;
    PVOID FilterContext[4];
  } DUMMYUNIONNAME;
  KPROCESSOR_MODE RequestorMode;
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

typedef struct _FLT_RELATED_OBJECTS {
  USHORT CONST Size;
  USHORT CONST TransactionContext;
  PFLT_FILTER CONST Filter;
  PFLT_VOLUME CONST Volume;
  PFLT_INSTANCE CONST Instance;
  PFILE_OBJECT CONST FileObject;
  PKTRANSACTION CONST Transaction;
} FLT_RELATED_OBJECTS, *PFLT_RELATED_OBJECTS;
typedef const struct _FLT_RELATED_OBJECTS *PCFLT_RELATED_OBJECTS;

typedef struct _FLT_RELATED_CONTEXTS {
  PFLT_CONTEXT VolumeContext;
  PFLT_CONTEXT InstanceContext;
  PFLT_CONTEXT FileContext;
  PFLT_CONTEXT StreamContext;
  PFLT_CONTEXT StreamHandleContext;
  PFLT_CONTEXT TransactionContext;
} FLT_RELATED_CONTEXTS, *PFLT_RELATED_CONTEXTS;

typedef VOID
(FLTAPI *PFLT_CONTEXT_CLEANUP_CALLBACK)(
    _In_ PFLT_CONTEXT Context,
    _In_ FLT_CONTEXT_TYPE ContextType);

typedef PVOID
(FLTAPI *PFLT_CONTEXT_ALLOCATE_CALLBACK)(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T Size,
    _In_ FLT_CONTEXT_TYPE ContextType);

typedef VOID
(FLTAPI *PFLT_CONTEXT_FREE_CALLBACK)(
    _In_ PVOID Pool,
    _In_ FLT_CONTEXT_TYPE ContextType);

typedef USHORT FLT_CONTEXT_REGISTRATION_FLAGS;

#define FLTFL_CONTEXT_REGISTRATION_NO_EXACT_SIZE_MATCH 0x0001

#define FLT_VARIABLE_SIZED_CONTEXTS ((SIZE_T)-1)

typedef struct _FLT_CONTEXT_REGISTRATION {
    FLT_CONTEXT_TYPE ContextType;
    FLT_CONTEXT_REGISTRATION_FLAGS Flags;
    PFLT_CONTEXT_CLEANUP_CALLBACK ContextCleanupCallback;
    SIZE_T Size;
    ULONG PoolTag;
    PFLT_CONTEXT_ALLOCATE_CALLBACK ContextAllocateCallback;
    PFLT_CONTEXT_FREE_CALLBACK ContextFreeCallback;
    PVOID Reserved1;
} FLT_CONTEXT_REGISTRATION, *PFLT_CONTEXT_REGISTRATION;
typedef const struct _FLT_CONTEXT_REGISTRATION *PCFLT_CONTEXT_REGISTRATION;

typedef ULONG FLT_INSTANCE_SETUP_FLAGS;

#define FLTFL_INSTANCE_SETUP_AUTOMATIC_ATTACHMENT 0x00000001
#define FLTFL_INSTANCE_SETUP_MANUAL_ATTACHMENT    0x00000002
#define FLTFL_INSTANCE_SETUP_NEWLY_MOUNTED_VOLUME 0x00000004

#if FLT_MGR_LONGHORN

#define FLTFL_INSTANCE_SETUP_DETACHED_VOLUME        0x00000008

#define FLT_MAX_TRANSACTION_NOTIFICATIONS (TRANSACTION_NOTIFY_PREPREPARE | \
                                           TRANSACTION_NOTIFY_PREPARE |    \
                                           TRANSACTION_NOTIFY_COMMIT |     \
                                           TRANSACTION_NOTIFY_ROLLBACK |   \
                                           TRANSACTION_NOTIFY_COMMIT_FINALIZE)

#endif /* FLT_MGR_LONGHORN */

typedef NTSTATUS
(FLTAPI *PFLT_INSTANCE_SETUP_CALLBACK)(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);

typedef ULONG FLT_INSTANCE_QUERY_TEARDOWN_FLAGS;

typedef NTSTATUS
(FLTAPI *PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK)(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);

typedef ULONG FLT_INSTANCE_TEARDOWN_FLAGS;

#define FLTFL_INSTANCE_TEARDOWN_MANUAL                  0x00000001
#define FLTFL_INSTANCE_TEARDOWN_FILTER_UNLOAD           0x00000002
#define FLTFL_INSTANCE_TEARDOWN_MANDATORY_FILTER_UNLOAD 0x00000004
#define FLTFL_INSTANCE_TEARDOWN_VOLUME_DISMOUNT         0x00000008
#define FLTFL_INSTANCE_TEARDOWN_INTERNAL_ERROR          0x00000010

typedef VOID
(FLTAPI *PFLT_INSTANCE_TEARDOWN_CALLBACK)(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Reason);

typedef enum _FLT_PREOP_CALLBACK_STATUS {
  FLT_PREOP_SUCCESS_WITH_CALLBACK,
  FLT_PREOP_SUCCESS_NO_CALLBACK,
  FLT_PREOP_PENDING,
  FLT_PREOP_DISALLOW_FASTIO,
  FLT_PREOP_COMPLETE,
  FLT_PREOP_SYNCHRONIZE
} FLT_PREOP_CALLBACK_STATUS, *PFLT_PREOP_CALLBACK_STATUS;

typedef FLT_PREOP_CALLBACK_STATUS
(FLTAPI *PFLT_PRE_OPERATION_CALLBACK)(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Outptr_result_maybenull_ PVOID *CompletionContext);

typedef enum _FLT_POSTOP_CALLBACK_STATUS {
  FLT_POSTOP_FINISHED_PROCESSING,
  FLT_POSTOP_MORE_PROCESSING_REQUIRED
} FLT_POSTOP_CALLBACK_STATUS, *PFLT_POSTOP_CALLBACK_STATUS;

typedef ULONG FLT_POST_OPERATION_FLAGS;

#define FLTFL_POST_OPERATION_DRAINING 0x00000001

typedef FLT_POSTOP_CALLBACK_STATUS
(FLTAPI *PFLT_POST_OPERATION_CALLBACK)(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags);

typedef ULONG FLT_OPERATION_REGISTRATION_FLAGS;

#define FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO   0x00000001
#define FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO   0x00000002
#define FLTFL_OPERATION_REGISTRATION_SKIP_NON_DASD_IO 0x00000004

typedef struct _FLT_OPERATION_REGISTRATION {
  UCHAR MajorFunction;
  FLT_OPERATION_REGISTRATION_FLAGS Flags;
  PFLT_PRE_OPERATION_CALLBACK PreOperation;
  PFLT_POST_OPERATION_CALLBACK PostOperation;
  PVOID Reserved1;
} FLT_OPERATION_REGISTRATION, *PFLT_OPERATION_REGISTRATION;

typedef struct _FLT_TAG_DATA_BUFFER {
  ULONG FileTag;
  USHORT TagDataLength;
  USHORT UnparsedNameLength;
  _ANONYMOUS_UNION union {
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      ULONG Flags;
      WCHAR PathBuffer[1];
    } SymbolicLinkReparseBuffer;
    struct {
      USHORT SubstituteNameOffset;
      USHORT SubstituteNameLength;
      USHORT PrintNameOffset;
      USHORT PrintNameLength;
      WCHAR PathBuffer[1];
    } MountPointReparseBuffer;
    struct {
      UCHAR DataBuffer[1];
    } GenericReparseBuffer;
    struct {
      GUID TagGuid;
      UCHAR DataBuffer[1];
    } GenericGUIDReparseBuffer;
  } DUMMYUNIONNAME;
} FLT_TAG_DATA_BUFFER, *PFLT_TAG_DATA_BUFFER;

#define FLT_TAG_DATA_BUFFER_HEADER_SIZE FIELD_OFFSET(FLT_TAG_DATA_BUFFER, GenericReparseBuffer)

typedef ULONG FLT_FILTER_UNLOAD_FLAGS;

#define FLTFL_FILTER_UNLOAD_MANDATORY               0x00000001

typedef NTSTATUS
(FLTAPI *PFLT_FILTER_UNLOAD_CALLBACK)(
    FLT_FILTER_UNLOAD_FLAGS Flags);

typedef struct _FLT_NAME_CONTROL {
  UNICODE_STRING Name;
} FLT_NAME_CONTROL, *PFLT_NAME_CONTROL;

typedef ULONG FLT_FILE_NAME_OPTIONS;

typedef NTSTATUS
(FLTAPI *PFLT_GENERATE_FILE_NAME)(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_CALLBACK_DATA CallbackData,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Out_ PBOOLEAN CacheFileNameInformation,
    _Out_ PFLT_NAME_CONTROL FileName);

typedef ULONG FLT_NORMALIZE_NAME_FLAGS;

#define FLTFL_NORMALIZE_NAME_CASE_SENSITIVE        0x01
#define FLTFL_NORMALIZE_NAME_DESTINATION_FILE_NAME 0x02

typedef NTSTATUS
(FLTAPI *PFLT_NORMALIZE_NAME_COMPONENT)(
    _In_ PFLT_INSTANCE Instance,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT VolumeNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext);

typedef NTSTATUS
(FLTAPI *PFLT_NORMALIZE_NAME_COMPONENT_EX)(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ PCUNICODE_STRING ParentDirectory,
    _In_ USHORT VolumeNameLength,
    _In_ PCUNICODE_STRING Component,
    _Out_writes_bytes_(ExpandComponentNameLength) PFILE_NAMES_INFORMATION ExpandComponentName,
    _In_ ULONG ExpandComponentNameLength,
    _In_ FLT_NORMALIZE_NAME_FLAGS Flags,
    _Inout_ PVOID *NormalizationContext);

typedef VOID
(FLTAPI *PFLT_NORMALIZE_CONTEXT_CLEANUP)(
    _In_opt_ PVOID *NormalizationContext);

#if FLT_MGR_LONGHORN
typedef NTSTATUS
(FLTAPI *PFLT_TRANSACTION_NOTIFICATION_CALLBACK)(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_CONTEXT TransactionContext,
    _In_ ULONG NotificationMask);
#endif /* FLT_MGR_LONGHORN */

#define FLT_REGISTRATION_VERSION_0200  0x0200
#define FLT_REGISTRATION_VERSION_0201  0x0201
#define FLT_REGISTRATION_VERSION_0202  0x0202
#define FLT_REGISTRATION_VERSION_0203  0x0203

#if FLT_MGR_LONGHORN
#define FLT_REGISTRATION_VERSION FLT_REGISTRATION_VERSION_0202
#else
#define FLT_REGISTRATION_VERSION FLT_REGISTRATION_VERSION_0200
#endif

typedef ULONG FLT_REGISTRATION_FLAGS;

#define FLTFL_REGISTRATION_DO_NOT_SUPPORT_SERVICE_STOP 0x00000001
#define FLTFL_REGISTRATION_SUPPORT_NPFS_MSFS           0x00000002

typedef struct _FLT_REGISTRATION {
  USHORT Size;
  USHORT Version;
  FLT_REGISTRATION_FLAGS Flags;
  CONST FLT_CONTEXT_REGISTRATION *ContextRegistration;
  CONST FLT_OPERATION_REGISTRATION *OperationRegistration;
  PFLT_FILTER_UNLOAD_CALLBACK FilterUnloadCallback;
  PFLT_INSTANCE_SETUP_CALLBACK InstanceSetupCallback;
  PFLT_INSTANCE_QUERY_TEARDOWN_CALLBACK InstanceQueryTeardownCallback;
  PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownStartCallback;
  PFLT_INSTANCE_TEARDOWN_CALLBACK InstanceTeardownCompleteCallback;
  PFLT_GENERATE_FILE_NAME GenerateFileNameCallback;
  PFLT_NORMALIZE_NAME_COMPONENT NormalizeNameComponentCallback;
  PFLT_NORMALIZE_CONTEXT_CLEANUP NormalizeContextCleanupCallback;
#if FLT_MGR_LONGHORN
  PFLT_TRANSACTION_NOTIFICATION_CALLBACK TransactionNotificationCallback;
  PFLT_NORMALIZE_NAME_COMPONENT_EX NormalizeNameComponentExCallback;
#endif /* FLT_MGR_LONGHORN */
} FLT_REGISTRATION, *PFLT_REGISTRATION;

typedef VOID
(FLTAPI *PFLT_COMPLETED_ASYNC_IO_CALLBACK)(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ PFLT_CONTEXT Context);

typedef ULONG FLT_IO_OPERATION_FLAGS;

#define FLTFL_IO_OPERATION_NON_CACHED                   0x00000001
#define FLTFL_IO_OPERATION_PAGING                       0x00000002
#define FLTFL_IO_OPERATION_DO_NOT_UPDATE_BYTE_OFFSET    0x00000004

#if FLT_MGR_LONGHORN
#define FLTFL_IO_OPERATION_SYNCHRONOUS_PAGING           0x00000008
#endif

typedef VOID
(FLTAPI *PFLT_GET_OPERATION_STATUS_CALLBACK)(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK IopbSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_opt_ PVOID RequesterContext);

typedef ULONG FLT_FILE_NAME_OPTIONS;

#define FLT_VALID_FILE_NAME_FORMATS 0x000000ff

#define FLT_FILE_NAME_NORMALIZED    0x01
#define FLT_FILE_NAME_OPENED        0x02
#define FLT_FILE_NAME_SHORT         0x03

#define FltGetFileNameFormat( _NameOptions ) ((_NameOptions) & FLT_VALID_FILE_NAME_FORMATS)

#define FLT_VALID_FILE_NAME_QUERY_METHODS               0x0000ff00

#define FLT_FILE_NAME_QUERY_DEFAULT                   0x0100
#define FLT_FILE_NAME_QUERY_CACHE_ONLY                0x0200
#define FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY           0x0300
#define FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP 0x0400

#define FltGetFileNameQueryMethod( _NameOptions ) ((_NameOptions) & FLT_VALID_FILE_NAME_QUERY_METHODS)

#define FLT_VALID_FILE_NAME_FLAGS                     0xff000000

#define FLT_FILE_NAME_REQUEST_FROM_CURRENT_PROVIDER   0x01000000
#define FLT_FILE_NAME_DO_NOT_CACHE                    0x02000000

#if FLT_MGR_AFTER_XPSP2
#define FLT_FILE_NAME_ALLOW_QUERY_ON_REPARSE          0x04000000
#endif

typedef USHORT FLT_FILE_NAME_PARSED_FLAGS;

#define FLTFL_FILE_NAME_PARSED_FINAL_COMPONENT      0x0001
#define FLTFL_FILE_NAME_PARSED_EXTENSION            0x0002
#define FLTFL_FILE_NAME_PARSED_STREAM               0x0004
#define FLTFL_FILE_NAME_PARSED_PARENT_DIR           0x0008

typedef struct _FLT_FILE_NAME_INFORMATION {
  USHORT Size;
  FLT_FILE_NAME_PARSED_FLAGS NamesParsed;
  FLT_FILE_NAME_OPTIONS Format;
  UNICODE_STRING Name;
  UNICODE_STRING Volume;
  UNICODE_STRING Share;
  UNICODE_STRING Extension;
  UNICODE_STRING Stream;
  UNICODE_STRING FinalComponent;
  UNICODE_STRING ParentDir;
} FLT_FILE_NAME_INFORMATION, *PFLT_FILE_NAME_INFORMATION;

typedef enum _FLT_SET_CONTEXT_OPERATION {
  FLT_SET_CONTEXT_REPLACE_IF_EXISTS,
  FLT_SET_CONTEXT_KEEP_IF_EXISTS
} FLT_SET_CONTEXT_OPERATION, *PFLT_SET_CONTEXT_OPERATION;

typedef struct _FLT_VOLUME_PROPERTIES {
  DEVICE_TYPE DeviceType;
  ULONG DeviceCharacteristics;
  ULONG DeviceObjectFlags;
  ULONG AlignmentRequirement;
  USHORT SectorSize;
  USHORT Reserved0;
  UNICODE_STRING FileSystemDriverName;
  UNICODE_STRING FileSystemDeviceName;
  UNICODE_STRING RealDeviceName;
} FLT_VOLUME_PROPERTIES, *PFLT_VOLUME_PROPERTIES;

#define FLT_PORT_CONNECT        0x0001
#define FLT_PORT_ALL_ACCESS     (FLT_PORT_CONNECT | STANDARD_RIGHTS_ALL)

typedef NTSTATUS
(FLTAPI *PFLT_MESSAGE_NOTIFY)(
    _In_opt_ PVOID PortCookie,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength,*ReturnOutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength);

typedef NTSTATUS
(FLTAPI *PFLT_CONNECT_NOTIFY)(
    _In_ PFLT_PORT ClientPort,
    _In_opt_ PVOID ServerPortCookie,
    _In_reads_bytes_opt_(SizeOfContext) PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Outptr_result_maybenull_ PVOID *ConnectionPortCookie);

typedef VOID
(FLTAPI *PFLT_DISCONNECT_NOTIFY)(
    _In_opt_ PVOID ConnectionCookie);

typedef VOID
(FLTAPI *PFLT_COMPLETE_CANCELED_CALLBACK)(
    _In_ PFLT_CALLBACK_DATA CallbackData);

typedef struct _FLT_DEFERRED_IO_WORKITEM *PFLT_DEFERRED_IO_WORKITEM;
typedef struct _FLT_GENERIC_WORKITEM *PFLT_GENERIC_WORKITEM;

typedef VOID
(FLTAPI *PFLT_DEFERRED_IO_WORKITEM_ROUTINE)(
    _In_ PFLT_DEFERRED_IO_WORKITEM FltWorkItem,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context);

typedef VOID
(FLTAPI *PFLT_GENERIC_WORKITEM_ROUTINE)(
    _In_ PFLT_GENERIC_WORKITEM FltWorkItem,
    _In_ PVOID FltObject,
    _In_opt_ PVOID Context);

typedef IO_CSQ_IRP_CONTEXT FLT_CALLBACK_DATA_QUEUE_IO_CONTEXT, *PFLT_CALLBACK_DATA_QUEUE_IO_CONTEXT;

typedef struct _FLT_CALLBACK_DATA_QUEUE FLT_CALLBACK_DATA_QUEUE, *PFLT_CALLBACK_DATA_QUEUE;

typedef NTSTATUS
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_INSERT_IO)(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_opt_ PVOID InsertContext);

typedef VOID
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_REMOVE_IO)(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_ PFLT_CALLBACK_DATA Cbd);

typedef PFLT_CALLBACK_DATA
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_PEEK_NEXT_IO)(
    _In_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_opt_ PFLT_CALLBACK_DATA Cbd,
    _In_opt_ PVOID PeekContext);

typedef VOID
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_ACQUIRE)(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _Out_opt_ PKIRQL Irql);

typedef VOID
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_RELEASE)(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_opt_ KIRQL Irql);

typedef VOID
(FLTAPI *PFLT_CALLBACK_DATA_QUEUE_COMPLETE_CANCELED_IO)(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _Inout_ PFLT_CALLBACK_DATA Cbd);

typedef enum _FLT_CALLBACK_DATA_QUEUE_FLAGS FLT_CALLBACK_DATA_QUEUE_FLAGS;

typedef struct _FLT_CALLBACK_DATA_QUEUE {
  IO_CSQ Csq;
  FLT_CALLBACK_DATA_QUEUE_FLAGS Flags;
  PFLT_INSTANCE Instance;
  PFLT_CALLBACK_DATA_QUEUE_INSERT_IO InsertIo;
  PFLT_CALLBACK_DATA_QUEUE_REMOVE_IO RemoveIo;
  PFLT_CALLBACK_DATA_QUEUE_PEEK_NEXT_IO PeekNextIo;
  PFLT_CALLBACK_DATA_QUEUE_ACQUIRE Acquire;
  PFLT_CALLBACK_DATA_QUEUE_RELEASE Release;
  PFLT_CALLBACK_DATA_QUEUE_COMPLETE_CANCELED_IO CompleteCanceledIo;
} FLT_CALLBACK_DATA_QUEUE, *PFLT_CALLBACK_DATA_QUEUE;

typedef NTSTATUS
(*PFLT_COMPLETE_LOCK_CALLBACK_DATA_ROUTINE)(
    _In_opt_ PVOID Context,
    _In_ PFLT_CALLBACK_DATA CallbackData);

typedef VOID
(FLTAPI *PFLTOPLOCK_WAIT_COMPLETE_ROUTINE)(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context);

typedef VOID
(FLTAPI *PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE)(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context);

VOID
FLTAPI
FltSetCallbackDataDirty(
    _Inout_ PFLT_CALLBACK_DATA Data);

VOID
FLTAPI
FltClearCallbackDataDirty(
    _Inout_ PFLT_CALLBACK_DATA Data);

BOOLEAN
FLTAPI
FltIsCallbackDataDirty(
    _In_ PFLT_CALLBACK_DATA Data);

_Must_inspect_result_
BOOLEAN
FLTAPI
FltDoCompletionProcessingWhenSafe(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags,
    _In_ PFLT_POST_OPERATION_CALLBACK SafePostCallback,
    _Out_ PFLT_POSTOP_CALLBACK_STATUS RetPostOperationStatus);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltCheckAndGrowNameControl(
    _Inout_ PFLT_NAME_CONTROL NameCtrl,
    _In_ USHORT NewSize);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltPurgeFileNameInformationCache(
    _In_ PFLT_INSTANCE Instance,
    _In_opt_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltRegisterFilter(
    _In_ PDRIVER_OBJECT Driver,
    _In_ CONST FLT_REGISTRATION *Registration,
    _Outptr_ PFLT_FILTER *RetFilter);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltUnregisterFilter(
    _In_ PFLT_FILTER Filter);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltStartFiltering(
    _In_ PFLT_FILTER Filter);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
PVOID
FLTAPI
FltGetRoutineAddress(
    _In_ PCSTR FltMgrRoutineName);

_When_(CallbackStatus==FLT_PREOP_COMPLETE, _IRQL_requires_max_(DISPATCH_LEVEL))
_When_(CallbackStatus!=FLT_PREOP_COMPLETE, _IRQL_requires_max_(APC_LEVEL))
VOID
FLTAPI
FltCompletePendedPreOperation(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ FLT_PREOP_CALLBACK_STATUS CallbackStatus,
    _In_opt_ PVOID Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltCompletePendedPostOperation(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltRequestOperationStatusCallback(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PFLT_GET_OPERATION_STATUS_CALLBACK CallbackRoutine,
    _In_opt_ PVOID RequesterContext);

_When_((PoolType==NonPagedPoolNx), _IRQL_requires_max_(DISPATCH_LEVEL))
_When_((PoolType!=NonPagedPoolNx), _IRQL_requires_max_(APC_LEVEL))
PVOID
FLTAPI
FltAllocatePoolAlignedWithTag(
    _In_ PFLT_INSTANCE Instance,
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltFreePoolAlignedWithTag(
    _In_ PFLT_INSTANCE Instance,
    _In_ PVOID Buffer,
    _In_ ULONG Tag);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFileNameInformation(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFileNameInformationUnsafe(
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_INSTANCE Instance,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *FileNameInformation);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReleaseFileNameInformation(
    _In_ PFLT_FILE_NAME_INFORMATION FileNameInformation);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReferenceFileNameInformation(
    _In_ PFLT_FILE_NAME_INFORMATION FileNameInformation);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltParseFileName(
    _In_ PCUNICODE_STRING FileName,
    _Inout_opt_ PUNICODE_STRING Extension,
    _Inout_opt_ PUNICODE_STRING Stream,
    _Inout_opt_ PUNICODE_STRING FinalComponent);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltParseFileNameInformation(
    _Inout_ PFLT_FILE_NAME_INFORMATION FileNameInformation);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetTunneledName(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ PFLT_FILE_NAME_INFORMATION FileNameInformation,
    _Outptr_result_maybenull_ PFLT_FILE_NAME_INFORMATION *RetTunneledFileNameInformation);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeName(
    _In_ PFLT_VOLUME Volume,
    _Inout_opt_ PUNICODE_STRING VolumeName,
    _Out_opt_ PULONG BufferSizeNeeded);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetDestinationFileNameInformation(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ HANDLE RootDirectory,
    _In_reads_bytes_(FileNameLength) PWSTR FileName,
    _In_ ULONG FileNameLength,
    _In_ FLT_FILE_NAME_OPTIONS NameOptions,
    _Outptr_ PFLT_FILE_NAME_INFORMATION *RetFileNameInformation);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltIsDirectory(
    _In_ PFILE_OBJECT FileObject,
    _In_ PFLT_INSTANCE Instance,
    _Out_ PBOOLEAN IsDirectory);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltLoadFilter(
    _In_ PCUNICODE_STRING FilterName);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltUnloadFilter(
    _In_ PCUNICODE_STRING FilterName);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAttachVolume(
    _Inout_ PFLT_FILTER Filter,
    _Inout_ PFLT_VOLUME Volume,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _Outptr_opt_result_maybenull_ PFLT_INSTANCE *RetInstance);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAttachVolumeAtAltitude(
    _Inout_ PFLT_FILTER Filter,
    _Inout_ PFLT_VOLUME Volume,
    _In_ PCUNICODE_STRING Altitude,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _Outptr_opt_result_maybenull_ PFLT_INSTANCE *RetInstance);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDetachVolume(
    _Inout_ PFLT_FILTER Filter,
    _Inout_ PFLT_VOLUME Volume,
    _In_opt_ PCUNICODE_STRING InstanceName);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateCallbackData(
    _In_ PFLT_INSTANCE Instance,
    _In_opt_ PFILE_OBJECT FileObject,
    _Outptr_ PFLT_CALLBACK_DATA *RetNewCallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltFreeCallbackData(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReuseCallbackData(
    _Inout_ PFLT_CALLBACK_DATA CallbackData);

_When_(FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(APC_LEVEL))
_When_(!FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(PASSIVE_LEVEL))
VOID
FLTAPI
FltPerformSynchronousIo(
    _Inout_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_When_( FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(APC_LEVEL))
_When_( !FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(PASSIVE_LEVEL))
NTSTATUS
FLTAPI
FltPerformAsynchronousIo(
    _Inout_ PFLT_CALLBACK_DATA CallbackData,
    _In_ PFLT_COMPLETED_ASYNC_IO_CALLBACK CallbackRoutine,
    _In_ PVOID CallbackContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateFile(
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength)PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_((Flags|FLTFL_IO_OPERATION_PAGING|FLTFL_IO_OPERATION_SYNCHRONOUS_PAGING),_IRQL_requires_max_(APC_LEVEL))
NTSTATUS
FLTAPI
FltReadFile(
    _In_ PFLT_INSTANCE InitiatingInstance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_ ULONG Length,
    _Out_writes_bytes_to_(Length,*BytesRead) PVOID Buffer,
    _In_ FLT_IO_OPERATION_FLAGS Flags,
    _Out_opt_ PULONG BytesRead,
    _In_opt_ PFLT_COMPLETED_ASYNC_IO_CALLBACK CallbackRoutine,
    _In_opt_ PVOID CallbackContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltTagFile(
    _In_ PFLT_INSTANCE InitiatingInstance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG FileTag,
    _In_opt_ GUID *Guid,
    _In_reads_bytes_(DataBufferLength) PVOID DataBuffer,
    _In_ USHORT DataBufferLength);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltUntagFile(
    _In_ PFLT_INSTANCE InitiatingInstance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG FileTag,
    _In_opt_ GUID *Guid);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
_When_((Flags|FLTFL_IO_OPERATION_PAGING|FLTFL_IO_OPERATION_SYNCHRONOUS_PAGING),_IRQL_requires_max_(APC_LEVEL))
NTSTATUS
FLTAPI
FltWriteFile(
    _In_ PFLT_INSTANCE InitiatingInstance,
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PLARGE_INTEGER ByteOffset,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ FLT_IO_OPERATION_FLAGS Flags,
    _Out_opt_ PULONG BytesWritten,
    _In_opt_ PFLT_COMPLETED_ASYNC_IO_CALLBACK CallbackRoutine,
    _In_opt_ PVOID CallbackContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQueryInformationFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_to_(Length,*LengthReturned) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _Out_opt_ PULONG LengthReturned);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltSetInformationFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQueryVolumeInformationFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_to_(Length,*LengthReturned) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass,
    _Out_opt_ PULONG LengthReturned);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQuerySecurityObject(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _Inout_updates_bytes_opt_(Length) PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Length,
    _Out_opt_ PULONG LengthNeeded);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltSetSecurityObject(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltFlushBuffers(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltFsControlFile(
    _In_ PFLT_INSTANCE Instance,
    _In_  PFILE_OBJECT FileObject,
    _In_ ULONG FsControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength,*LengthReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG LengthReturned);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltDeviceIoControlFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength,*LengthReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG LengthReturned);

_Must_inspect_result_
_When_(FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(APC_LEVEL))
_When_(!FlagOn(CallbackData->Iopb->IrpFlags, IRP_PAGING_IO), _IRQL_requires_max_(PASSIVE_LEVEL))
VOID
FLTAPI
FltReissueSynchronousIo(
   _In_ PFLT_INSTANCE InitiatingInstance,
   _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltClose(
   _In_ HANDLE FileHandle);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FLTAPI
FltCancelFileOpen(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateSystemVolumeInformationFolder(
    _In_ PFLT_INSTANCE Instance);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltSupportsFileContexts(
    _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltSupportsStreamContexts(
    _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltSupportsStreamHandleContexts(
    _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateContext(
    _In_ PFLT_FILTER Filter,
    _In_ FLT_CONTEXT_TYPE ContextType,
    _In_ SIZE_T ContextSize,
    _In_ POOL_TYPE PoolType,
    _Outptr_result_bytebuffer_(ContextSize) PFLT_CONTEXT *ReturnedContext);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltGetContexts(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_CONTEXT_TYPE DesiredContexts,
    _Out_ PFLT_RELATED_CONTEXTS Contexts);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReleaseContexts(
    _In_ PFLT_RELATED_CONTEXTS Contexts);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetVolumeContext(
    _In_ PFLT_VOLUME Volume,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetInstanceContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetFileContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetStreamContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetStreamHandleContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltDeleteContext(
    _In_ PFLT_CONTEXT Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteVolumeContext(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_VOLUME Volume,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteInstanceContext(
    _In_ PFLT_INSTANCE Instance,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteFileContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteStreamContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteStreamHandleContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_opt_result_maybenull_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeContext(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetInstanceContext(
    _In_ PFLT_INSTANCE Instance,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFileContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetStreamContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetStreamHandleContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltReferenceContext(
    _In_ PFLT_CONTEXT Context);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltReleaseContext(
    _In_ PFLT_CONTEXT Context);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFilterFromName(
    _In_ PCUNICODE_STRING FilterName,
    _Outptr_ PFLT_FILTER *RetFilter);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeFromName(
    _In_ PFLT_FILTER Filter,
    _In_ PCUNICODE_STRING VolumeName,
    _Outptr_ PFLT_VOLUME *RetVolume);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeInstanceFromName(
    _In_opt_ PFLT_FILTER Filter,
    _In_ PFLT_VOLUME Volume,
    _In_opt_ PCUNICODE_STRING InstanceName,
    _Outptr_ PFLT_INSTANCE *RetInstance);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeFromInstance(
    _In_ PFLT_INSTANCE Instance,
    _Outptr_ PFLT_VOLUME *RetVolume);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFilterFromInstance(
    _In_ PFLT_INSTANCE Instance,
    _Outptr_ PFLT_FILTER *RetFilter);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeFromFileObject(
    _In_ PFLT_FILTER Filter,
    _In_ PFILE_OBJECT FileObject,
    _Outptr_ PFLT_VOLUME *RetVolume);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeFromDeviceObject(
    _In_ PFLT_FILTER Filter,
    _In_ PDEVICE_OBJECT DeviceObject,
    _Outptr_ PFLT_VOLUME *RetVolume);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltGetDeviceObject(
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PDEVICE_OBJECT *DeviceObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltGetDiskDeviceObject(
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PDEVICE_OBJECT *DiskDeviceObject);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetLowerInstance(
    _In_ PFLT_INSTANCE CurrentInstance,
    _Outptr_ PFLT_INSTANCE *LowerInstance);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetUpperInstance(
    _In_ PFLT_INSTANCE CurrentInstance,
    _Outptr_ PFLT_INSTANCE *UpperInstance);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetTopInstance(
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PFLT_INSTANCE *Instance);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetBottomInstance(
    _In_ PFLT_VOLUME Volume,
    _Outptr_ PFLT_INSTANCE *Instance);

LONG
FLTAPI
FltCompareInstanceAltitudes(
    _In_ PFLT_INSTANCE Instance1,
    _In_ PFLT_INSTANCE Instance2);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFilterInformation(
    _In_ PFLT_FILTER Filter,
    _In_ FILTER_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize, *BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetInstanceInformation(
    _In_ PFLT_INSTANCE Instance,
    _In_ INSTANCE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeProperties(
    _In_ PFLT_VOLUME Volume,
    _Out_writes_bytes_to_opt_(VolumePropertiesLength,*LengthReturned) PFLT_VOLUME_PROPERTIES VolumeProperties,
    _In_ ULONG VolumePropertiesLength,
    _Out_ PULONG LengthReturned);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltIsVolumeWritable(
    _In_ PVOID FltObject,
    _Out_ PBOOLEAN IsWritable);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeGuidName(
    _In_ PFLT_VOLUME Volume,
    _Out_ PUNICODE_STRING VolumeGuidName,
    _Out_opt_ PULONG BufferSizeNeeded);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQueryVolumeInformation(
    _In_ PFLT_INSTANCE Instance,
    _Out_ PIO_STATUS_BLOCK Iosb,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltSetVolumeInformation(
    _In_ PFLT_INSTANCE Instance,
    _Out_ PIO_STATUS_BLOCK Iosb,
    _Out_writes_bytes_(Length) PVOID FsInformation,
    _In_ ULONG Length,
    _In_ FS_INFORMATION_CLASS FsInformationClass);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateFilters(
    _Out_writes_to_opt_(FilterListSize,*NumberFiltersReturned) PFLT_FILTER *FilterList,
    _In_ ULONG FilterListSize,
    _Out_ PULONG NumberFiltersReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateVolumes(
    _In_ PFLT_FILTER Filter,
    _Out_writes_to_opt_(VolumeListSize,*NumberVolumesReturned) PFLT_VOLUME *VolumeList,
    _In_ ULONG VolumeListSize,
    _Out_ PULONG NumberVolumesReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateInstances(
    _In_opt_ PFLT_VOLUME Volume,
    _In_opt_ PFLT_FILTER Filter,
    _Out_writes_to_opt_(InstanceListSize,*NumberInstancesReturned) PFLT_INSTANCE *InstanceList,
    _In_ ULONG InstanceListSize,
    _Out_ PULONG NumberInstancesReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateFilterInformation(
    _In_ ULONG Index,
    _In_ FILTER_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateInstanceInformationByFilter(
    _In_ PFLT_FILTER Filter,
    _In_ ULONG Index,
    _In_ INSTANCE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateInstanceInformationByVolume(
    _In_ PFLT_VOLUME Volume,
    _In_ ULONG Index,
    _In_ INSTANCE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnumerateVolumeInformation(
    _In_ PFLT_FILTER Filter,
    _In_ ULONG Index,
    _In_ FILTER_VOLUME_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltObjectReference(
    _Inout_ PVOID FltObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltObjectDereference(
    _Inout_ PVOID FltObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateCommunicationPort(
    _In_ PFLT_FILTER Filter,
    _Outptr_ PFLT_PORT *ServerPort,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PVOID ServerPortCookie,
    _In_ PFLT_CONNECT_NOTIFY ConnectNotifyCallback,
    _In_ PFLT_DISCONNECT_NOTIFY DisconnectNotifyCallback,
    _In_opt_ PFLT_MESSAGE_NOTIFY MessageNotifyCallback,
    _In_ LONG MaxConnections);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FLTAPI
FltCloseCommunicationPort(
    _In_ PFLT_PORT ServerPort);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FLTAPI
FltCloseClientPort(
    _In_ PFLT_FILTER Filter,
    _Outptr_ PFLT_PORT *ClientPort);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSendMessage(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_PORT *ClientPort,
    _In_reads_bytes_(SenderBufferLength) PVOID SenderBuffer,
    _In_ ULONG SenderBufferLength,
    _Out_writes_bytes_opt_(*ReplyLength) PVOID ReplyBuffer,
    _Inout_opt_ PULONG ReplyLength,
    _In_opt_ PLARGE_INTEGER Timeout);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltBuildDefaultSecurityDescriptor(
    _Outptr_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ ACCESS_MASK DesiredAccess);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltFreeSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
FLTAPI
FltCancelIo(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltSetCancelCompletion(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ PFLT_COMPLETE_CANCELED_CALLBACK CanceledCallback);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltClearCancelCompletion(
    _In_ PFLT_CALLBACK_DATA CallbackData);

BOOLEAN
FLTAPI
FltIsIoCanceled(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
PFLT_DEFERRED_IO_WORKITEM
FLTAPI
FltAllocateDeferredIoWorkItem(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltFreeDeferredIoWorkItem(
    _In_ PFLT_DEFERRED_IO_WORKITEM FltWorkItem);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
PFLT_GENERIC_WORKITEM
FLTAPI
FltAllocateGenericWorkItem(VOID);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltFreeGenericWorkItem(
    _In_ PFLT_GENERIC_WORKITEM FltWorkItem);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltQueueDeferredIoWorkItem(
    _In_ PFLT_DEFERRED_IO_WORKITEM FltWorkItem,
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PFLT_DEFERRED_IO_WORKITEM_ROUTINE WorkerRoutine,
    _In_ WORK_QUEUE_TYPE QueueType,
    _In_ PVOID Context);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltQueueGenericWorkItem(
    _In_ PFLT_GENERIC_WORKITEM FltWorkItem,
    _In_ PVOID FltObject,
    _In_ PFLT_GENERIC_WORKITEM_ROUTINE WorkerRoutine,
    _In_ WORK_QUEUE_TYPE QueueType,
    _In_opt_ PVOID Context);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltLockUserBuffer(
    _In_ PFLT_CALLBACK_DATA CallbackData);

NTSTATUS
FLTAPI
FltDecodeParameters(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _Outptr_opt_ PMDL **MdlAddressPointer,
    _Outptr_opt_result_bytebuffer_(**Length) PVOID **Buffer,
    _Outptr_opt_ PULONG *Length,
    _Out_opt_ LOCK_OPERATION *DesiredAccess);

PMDL
FASTCALL
FltGetSwappedBufferMdlAddress(
    _In_ PFLT_CALLBACK_DATA CallbackData);

VOID
FASTCALL
FltRetainSwappedBufferMdlAddress(
    _In_ PFLT_CALLBACK_DATA CallbackData);

NTSTATUS
FLTAPI
FltCbdqInitialize(
    _In_ PFLT_INSTANCE Instance,
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_ PFLT_CALLBACK_DATA_QUEUE_INSERT_IO CbdqInsertIo,
    _In_ PFLT_CALLBACK_DATA_QUEUE_REMOVE_IO CbdqRemoveIo,
    _In_ PFLT_CALLBACK_DATA_QUEUE_PEEK_NEXT_IO CbdqPeekNextIo,
    _In_ PFLT_CALLBACK_DATA_QUEUE_ACQUIRE CbdqAcquire,
    _In_ PFLT_CALLBACK_DATA_QUEUE_RELEASE CbdqRelease,
    _In_ PFLT_CALLBACK_DATA_QUEUE_COMPLETE_CANCELED_IO CbdqCompleteCanceledIo);

VOID
FLTAPI
FltCbdqEnable(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq);

VOID
FLTAPI
FltCbdqDisable(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq);

_Must_inspect_result_
NTSTATUS
FLTAPI
FltCbdqInsertIo(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_ PFLT_CALLBACK_DATA Cbd,
    _In_opt_ PFLT_CALLBACK_DATA_QUEUE_IO_CONTEXT Context,
    _In_opt_ PVOID InsertContext);

_Must_inspect_result_
PFLT_CALLBACK_DATA
FLTAPI
FltCbdqRemoveIo(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_ PFLT_CALLBACK_DATA_QUEUE_IO_CONTEXT Context);

_Must_inspect_result_
PFLT_CALLBACK_DATA
FLTAPI
FltCbdqRemoveNextIo(
    _Inout_ PFLT_CALLBACK_DATA_QUEUE Cbdq,
    _In_opt_ PVOID PeekContext);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltInitializeOplock(
    _Out_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltUninitializeOplock(
    _In_ POPLOCK Oplock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltOplockFsctrl(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ ULONG OpenCount);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltCheckOplock(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context,
    _In_opt_ PFLTOPLOCK_WAIT_COMPLETE_ROUTINE WaitCompletionRoutine,
    _In_opt_ PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE PrePostCallbackDataRoutine);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltOplockIsFastIoPossible(
    _In_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltCurrentBatchOplock(
    _In_ POPLOCK Oplock);

VOID
FLTAPI
FltInitializeFileLock(
    _Out_ PFILE_LOCK FileLock);

VOID
FLTAPI
FltUninitializeFileLock(
    _In_ PFILE_LOCK FileLock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
PFILE_LOCK
FLTAPI
FltAllocateFileLock(
    _In_opt_ PFLT_COMPLETE_LOCK_CALLBACK_DATA_ROUTINE CompleteLockCallbackDataRoutine,
    _In_opt_ PUNLOCK_ROUTINE UnlockRoutine);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltFreeFileLock(
    _In_ PFILE_LOCK FileLock);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltProcessFileLock(
    _In_ PFILE_LOCK FileLock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltCheckLockForReadAccess(
    _In_ PFILE_LOCK FileLock,
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltCheckLockForWriteAccess(
    _In_ PFILE_LOCK FileLock,
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquireResourceExclusive(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PERESOURCE Resource);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquireResourceShared(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PERESOURCE Resource);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
FLTAPI
FltReleaseResource(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PERESOURCE Resource);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltInitializePushLock(
    _Out_ PEX_PUSH_LOCK PushLock);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltDeletePushLock(
    _In_ PEX_PUSH_LOCK PushLock);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquirePushLockExclusive(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PEX_PUSH_LOCK PushLock);

_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcquirePushLockShared(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PEX_PUSH_LOCK PushLock);

_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltReleasePushLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PEX_PUSH_LOCK PushLock);

BOOLEAN
FLTAPI
FltIsOperationSynchronous(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
FLTAPI
FltIs32bitProcess(
    _In_opt_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
PEPROCESS
FLTAPI
FltGetRequestorProcess(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG
FLTAPI
FltGetRequestorProcessId(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltNotifyFilterChangeDirectory(
    _Inout_ PNOTIFY_SYNC NotifySync,
    _Inout_ PLIST_ENTRY NotifyList,
    _In_ PVOID FsContext,
    _In_ PSTRING FullDirectoryName,
    _In_ BOOLEAN WatchTree,
    _In_ BOOLEAN IgnoreBuffer,
    _In_ ULONG CompletionFilter,
    _In_ PFLT_CALLBACK_DATA NotifyCallbackData,
    _In_opt_ PCHECK_FOR_TRAVERSE_ACCESS TraverseCallback,
    _In_opt_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_opt_ PFILTER_REPORT_CHANGE FilterCallback);

PCHAR
FLTAPI
FltGetIrpName(
    _In_ UCHAR IrpMajorCode);

#if FLT_MGR_AFTER_XPSP2

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateFileEx(
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltOpenVolume(
    _In_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE VolumeHandle,
    _Outptr_opt_ PFILE_OBJECT *VolumeFileObject);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQueryEaFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_to_(Length,*LengthReturned) PVOID ReturnedEaData,
    _In_ ULONG Length,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_reads_bytes_opt_(EaListLength) PVOID EaList,
    _In_ ULONG EaListLength,
    _In_opt_ PULONG EaIndex,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltSetEaFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _In_reads_bytes_(Length) PVOID EaBuffer,
    _In_ ULONG Length);

#endif /* FLT_MGR_AFTER_XPSP2 */

#if FLT_MGR_LONGHORN

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateFileEx2(
    _In_ PFLT_FILTER Filter,
    _In_opt_ PFLT_INSTANCE Instance,
    _Out_ PHANDLE FileHandle,
    _Outptr_opt_ PFILE_OBJECT *FileObject,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_opt_ PLARGE_INTEGER AllocationSize,
    _In_ ULONG FileAttributes,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_reads_bytes_opt_(EaLength) PVOID EaBuffer,
    _In_ ULONG EaLength,
    _In_ ULONG Flags,
    _In_opt_ PIO_DRIVER_CREATE_CONTEXT DriverContext);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltQueryDirectoryFile(
    _In_ PFLT_INSTANCE Instance,
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(Length) PVOID FileInformation,
    _In_ ULONG Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass,
    _In_ BOOLEAN ReturnSingleEntry,
    _In_opt_ PUNICODE_STRING FileName,
    _In_ BOOLEAN RestartScan,
    _Out_opt_ PULONG LengthReturned);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltSupportsFileContextsEx(
    _In_ PFILE_OBJECT FileObject,
    _In_opt_ PFLT_INSTANCE Instance);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetTransactionContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_ FLT_SET_CONTEXT_OPERATION Operation,
    _In_ PFLT_CONTEXT NewContext,
    _Outptr_opt_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltDeleteTransactionContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _Outptr_opt_ PFLT_CONTEXT *OldContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetTransactionContext(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _Outptr_ PFLT_CONTEXT *Context);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltIsFltMgrVolumeDeviceObject(
    _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetVolumeInformation(
    _In_ PFLT_VOLUME Volume,
    _In_ FILTER_VOLUME_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_to_opt_(BufferSize,*BytesReturned) PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_ PULONG BytesReturned);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetFileSystemType(
    _In_ PVOID FltObject,
    _Out_ PFLT_FILESYSTEM_TYPE FileSystemType);

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltIsVolumeSnapshot(
    _In_ PVOID FltObject,
    _Out_ PBOOLEAN IsSnapshotVolume);

_Must_inspect_result_
_When_(((CallbackData!=NULL) && FLT_IS_IRP_OPERATION(CallbackData)), _IRQL_requires_max_(PASSIVE_LEVEL))
_When_((!((CallbackData!=NULL) && FLT_IS_IRP_OPERATION(CallbackData))), _IRQL_requires_max_(APC_LEVEL))
NTSTATUS
FLTAPI
FltCancellableWaitForSingleObject(
    _In_ PVOID Object,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_opt_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_When_(((CallbackData!=NULL) && FLT_IS_IRP_OPERATION(CallbackData)), _IRQL_requires_max_(PASSIVE_LEVEL))
_When_((!((CallbackData!=NULL) && FLT_IS_IRP_OPERATION(CallbackData))), _IRQL_requires_max_(APC_LEVEL))
NTSTATUS
FLTAPI
FltCancellableWaitForMultipleObjects(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID ObjectArray[],
    _In_ WAIT_TYPE WaitType,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_opt_ PKWAIT_BLOCK WaitBlockArray,
    _In_ PFLT_CALLBACK_DATA CallbackData);

_IRQL_requires_max_(DISPATCH_LEVEL)
HANDLE
FLTAPI
FltGetRequestorProcessIdEx(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltEnlistInTransaction(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_ PFLT_CONTEXT TransactionContext,
    _In_ NOTIFICATION_MASK NotificationMask);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltRollbackEnlistment(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltPrePrepareComplete(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltPrepareComplete(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCommitComplete(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltCommitFinalizeComplete(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltRollbackComplete(
    _In_ PFLT_INSTANCE Instance,
    _In_ PKTRANSACTION Transaction,
    _In_opt_ PFLT_CONTEXT TransactionContext);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateExtraCreateParameterList(
    _In_ PFLT_FILTER Filter,
    _In_ FSRTL_ALLOCATE_ECPLIST_FLAGS Flags,
    _Outptr_ PECP_LIST *EcpList);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _In_ LPCGUID EcpType,
     ULONG SizeOfContext,
    _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
    _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
    _In_ ULONG PoolTag,
    _Outptr_ PVOID *EcpContext);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltInitExtraCreateParameterLookasideList(
    _In_ PFLT_FILTER Filter,
    _Inout_ PVOID Lookaside,
    _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags,
    _In_ SIZE_T Size,
    _In_ ULONG Tag);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltDeleteExtraCreateParameterLookasideList(
    _In_ PFLT_FILTER Filter,
    _Inout_ PVOID Lookaside,
    _In_ FSRTL_ECP_LOOKASIDE_FLAGS Flags);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateExtraCreateParameterFromLookasideList(
    _In_ PFLT_FILTER Filter,
    _In_ LPCGUID EcpType,
    _In_ ULONG SizeOfContext,
    _In_ FSRTL_ALLOCATE_ECP_FLAGS Flags,
    _In_opt_ PFSRTL_EXTRA_CREATE_PARAMETER_CLEANUP_CALLBACK CleanupCallback,
    _Inout_ PVOID LookasideList,
    _Outptr_ PVOID *EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltInsertExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _Inout_ PECP_LIST EcpList,
    _Inout_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltFindExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _In_ PECP_LIST EcpList,
    _In_ LPCGUID EcpType,
    _Outptr_opt_ PVOID *EcpContext,
    _Out_opt_ ULONG *EcpContextSize);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltRemoveExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _Inout_ PECP_LIST EcpList,
    _In_ LPCGUID EcpType,
    _Outptr_ PVOID *EcpContext,
    _Out_opt_ ULONG *EcpContextSize);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltFreeExtraCreateParameterList(
    _In_ PFLT_FILTER Filter,
    _In_ PECP_LIST EcpList);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltFreeExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetEcpListFromCallbackData(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _Outptr_result_maybenull_ PECP_LIST *EcpList);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSetEcpListIntoCallbackData(
    _In_ PFLT_FILTER Filter,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ PECP_LIST EcpList);

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetNextExtraCreateParameter(
    _In_ PFLT_FILTER Filter,
    _In_ PECP_LIST EcpList,
    _In_opt_ PVOID CurrentEcpContext,
    _Out_opt_ LPGUID NextEcpType,
    _Outptr_opt_ PVOID *NextEcpContext,
    _Out_opt_ ULONG *NextEcpContextSize);

_IRQL_requires_max_(APC_LEVEL)
VOID
FLTAPI
FltAcknowledgeEcp(
    _In_ PFLT_FILTER Filter,
    _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltIsEcpAcknowledged(
    _In_ PFLT_FILTER Filter,
    _In_ PVOID EcpContext);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltIsEcpFromUserMode(
    _In_ PFLT_FILTER Filter,
    _In_ PVOID EcpContext);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltRetrieveIoPriorityInfo(
    _In_opt_ PFLT_CALLBACK_DATA Data,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_opt_ PETHREAD Thread,
    _Inout_ PIO_PRIORITY_INFO PriorityInfo);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltApplyPriorityInfoThread(
    _In_ PIO_PRIORITY_INFO InputPriorityInfo,
    _Out_opt_ PIO_PRIORITY_INFO OutputPriorityInfo,
    _In_ PETHREAD Thread);

_IRQL_requires_max_(DISPATCH_LEVEL)
IO_PRIORITY_HINT
FLTAPI
FltGetIoPriorityHint(
    _In_ PFLT_CALLBACK_DATA Data);

_IRQL_requires_max_(DISPATCH_LEVEL)
IO_PRIORITY_HINT
FLTAPI
FltGetIoPriorityHintFromCallbackData(
    _In_ PFLT_CALLBACK_DATA Data);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltSetIoPriorityHintIntoCallbackData(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ IO_PRIORITY_HINT PriorityHint);

_IRQL_requires_max_(DISPATCH_LEVEL)
IO_PRIORITY_HINT
FLTAPI
FltGetIoPriorityHintFromFileObject(
    _In_ PFILE_OBJECT FileObject);

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltSetIoPriorityHintIntoFileObject(
    _In_ PFILE_OBJECT FileObject,
    _In_ IO_PRIORITY_HINT PriorityHint);

_IRQL_requires_max_(DISPATCH_LEVEL)
IO_PRIORITY_HINT
FLTAPI
FltGetIoPriorityHintFromThread(
    _In_ PETHREAD Thread);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltSetIoPriorityHintIntoThread(
    _In_ PETHREAD Thread,
    _In_ IO_PRIORITY_HINT PriorityHint);

#endif /* FLT_MGR_LONGHORN */

#if FLT_MGR_WIN7

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltAllocateCallbackDataEx(
    _In_ PFLT_INSTANCE Instance,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ FLT_ALLOCATE_CALLBACK_DATA_FLAGS Flags,
    _Outptr_ PFLT_CALLBACK_DATA *RetNewCallbackData);

_Must_inspect_result_
_IRQL_requires_max_(DPC_LEVEL)
PVOID
FLTAPI
FltGetNewSystemBufferAddress(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltCheckOplockEx(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_opt_ PFLTOPLOCK_WAIT_COMPLETE_ROUTINE WaitCompletionRoutine,
    _In_opt_ PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE PrePostCallbackDataRoutine);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltCurrentOplock(
    _In_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltCurrentOplockH(
    _In_ POPLOCK Oplock);

_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltOplockBreakH(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_opt_ PFLTOPLOCK_WAIT_COMPLETE_ROUTINE WaitCompletionRoutine,
    _In_opt_ PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE PrePostCallbackDataRoutine);

_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltOplockBreakToNone(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_opt_ PVOID Context,
    _In_opt_ PFLTOPLOCK_WAIT_COMPLETE_ROUTINE WaitCompletionRoutine,
    _In_opt_ PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE PrePostCallbackDataRoutine);

_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltOplockBreakToNoneEx(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context,
    _In_opt_ PFLTOPLOCK_WAIT_COMPLETE_ROUTINE WaitCompletionRoutine,
    _In_opt_ PFLTOPLOCK_PREPOST_CALLBACKDATA_ROUTINE PrePostCallbackDataRoutine);

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
FLTAPI
FltOplockIsSharedRequest(
    _In_ PFLT_CALLBACK_DATA CallbackData);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
FLT_PREOP_CALLBACK_STATUS
FLTAPI
FltOplockFsctrlEx(
    _In_ POPLOCK Oplock,
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _In_ ULONG OpenCount,
    _In_ ULONG Flags);

BOOLEAN
FLTAPI
FltOplockKeysEqual(
    _In_opt_ PFILE_OBJECT Fo1,
    _In_opt_ PFILE_OBJECT Fo2);

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltGetRequestorSessionId(
    _In_ PFLT_CALLBACK_DATA CallbackData,
    _Out_ PULONG SessionId);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltAdjustDeviceStackSizeForIoRedirection(
    _In_ PFLT_INSTANCE SourceInstance,
    _In_ PFLT_INSTANCE TargetInstance,
    _Out_opt_ PBOOLEAN SourceDeviceStackSizeModified);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltIsIoRedirectionAllowed(
    _In_ PFLT_INSTANCE SourceInstance,
    _In_ PFLT_INSTANCE TargetInstance,
    _Out_ PBOOLEAN RedirectionAllowed);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
FLTAPI
FltIsIoRedirectionAllowedForOperation(
    _In_ PFLT_CALLBACK_DATA Data,
    _In_ PFLT_INSTANCE TargetInstance,
    _Out_ PBOOLEAN RedirectionAllowedThisIo,
    _Out_opt_ PBOOLEAN RedirectionAllowedAllIo);

#endif /* FLT_MGR_WIN7 */

#endif /* FLT_MGR_BASELINE */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __FLTKERNEL__ */
