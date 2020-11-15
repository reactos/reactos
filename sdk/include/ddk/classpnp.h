
#pragma once

#define _CLASS_

#include <ntdddisk.h>
#include <ntddcdrm.h>
#include <ntddtape.h>
#include <ntddscsi.h>
#include <ntddstor.h>

#include <stdio.h>

#include <scsi.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#define SRB_CLASS_FLAGS_LOW_PRIORITY      0x10000000
#define SRB_CLASS_FLAGS_PERSISTANT        0x20000000
#define SRB_CLASS_FLAGS_PAGING            0x40000000
#define SRB_CLASS_FLAGS_FREE_MDL          0x80000000

#define ASSERT_FDO(x) \
  ASSERT(((PCOMMON_DEVICE_EXTENSION) (x)->DeviceExtension)->IsFdo)

#define ASSERT_PDO(x) \
  ASSERT(!(((PCOMMON_DEVICE_EXTENSION) (x)->DeviceExtension)->IsFdo))

#define IS_CLEANUP_REQUEST(majorFunction)   \
  ((majorFunction == IRP_MJ_CLOSE) ||       \
   (majorFunction == IRP_MJ_CLEANUP) ||     \
   (majorFunction == IRP_MJ_SHUTDOWN))

#define DO_MCD(fdoExtension)                                 \
  (((fdoExtension)->MediaChangeDetectionInfo != NULL) &&     \
   ((fdoExtension)->MediaChangeDetectionInfo->MediaChangeDetectionDisableCount == 0))

#define IS_SCSIOP_READ(opCode)     \
  ((opCode == SCSIOP_READ6)   ||   \
   (opCode == SCSIOP_READ)    ||   \
   (opCode == SCSIOP_READ12)  ||   \
   (opCode == SCSIOP_READ16))

#define IS_SCSIOP_WRITE(opCode)     \
  ((opCode == SCSIOP_WRITE6)   ||   \
   (opCode == SCSIOP_WRITE)    ||   \
   (opCode == SCSIOP_WRITE12)  ||   \
   (opCode == SCSIOP_WRITE16))

#define IS_SCSIOP_READWRITE(opCode) (IS_SCSIOP_READ(opCode) || IS_SCSIOP_WRITE(opCode))

#define ADJUST_FUA_FLAG(fdoExt) {                                                       \
    if (TEST_FLAG(fdoExt->DeviceFlags, DEV_WRITE_CACHE) &&                              \
        !TEST_FLAG(fdoExt->DeviceFlags, DEV_POWER_PROTECTED) &&                         \
        !TEST_FLAG(fdoExt->ScanForSpecialFlags, CLASS_SPECIAL_FUA_NOT_SUPPORTED) ) {    \
        fdoExt->CdbForceUnitAccess = TRUE;                                              \
    } else {                                                                            \
        fdoExt->CdbForceUnitAccess = FALSE;                                             \
    }                                                                                   \
}

#define FREE_POOL(_PoolPtr)     \
    if (_PoolPtr != NULL) {     \
        ExFreePool(_PoolPtr);   \
        _PoolPtr = NULL;        \
    }

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'nUcS')
//#define ExAllocatePool(a,b) #assert(0)
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,'nUcS')
#endif

#define CLASS_TAG_AUTORUN_DISABLE           'ALcS'
#define CLASS_TAG_FILE_OBJECT_EXTENSION     'FLcS'
#define CLASS_TAG_MEDIA_CHANGE_DETECTION    'MLcS'
#define CLASS_TAG_MOUNT                     'mLcS'
#define CLASS_TAG_RELEASE_QUEUE             'qLcS'
#define CLASS_TAG_POWER                     'WLcS'
#define CLASS_TAG_WMI                       'wLcS'
#define CLASS_TAG_FAILURE_PREDICT           'fLcS'
#define CLASS_TAG_DEVICE_CONTROL            'OIcS'
#define CLASS_TAG_MODE_DATA                 'oLcS'
#define CLASS_TAG_MULTIPATH                 'mPcS'
#define CLASS_TAG_LOCK_TRACKING             'TLcS'
#define CLASS_TAG_LB_PROVISIONING           'PLcS'
#define CLASS_TAG_MANAGE_DATASET            'MDcS'

#define MAXIMUM_RETRIES 4

#define CLASS_DRIVER_EXTENSION_KEY ((PVOID) ClassInitialize)

#define NO_REMOVE                         0
#define REMOVE_PENDING                    1
#define REMOVE_COMPLETE                   2

#define ClassAcquireRemoveLock(devobj, tag) \
  ClassAcquireRemoveLockEx(devobj, tag, __FILE__, __LINE__)

#ifdef TRY
#undef TRY
#endif
#ifdef LEAVE
#undef LEAVE
#endif

#ifdef FINALLY
#undef FINALLY
#endif

#define TRY
#define LEAVE             goto __tryLabel;
#define FINALLY           __tryLabel:

#if defined DebugPrint
#undef DebugPrint
#endif

#if DBG
#define DebugPrint(x) ClassDebugPrint x
#else
#define DebugPrint(x)
#endif

#define DEBUG_BUFFER_LENGTH                        256

#define START_UNIT_TIMEOUT                         (60 * 4)

#define MEDIA_CHANGE_DEFAULT_TIME                  1
#define MEDIA_CHANGE_TIMEOUT_TIME                  300

#ifdef ALLOCATE_SRB_FROM_POOL

#define ClasspAllocateSrb(ext)                      \
  ExAllocatePoolWithTag(NonPagedPool,               \
                        sizeof(SCSI_REQUEST_BLOCK), \
                        'sBRS')

#define ClasspFreeSrb(ext, srb) ExFreePool((srb));

#else /* ALLOCATE_SRB_FROM_POOL */

#define ClasspAllocateSrb(ext)                      \
  ExAllocateFromNPagedLookasideList(                \
      &((ext)->CommonExtension.SrbLookasideList))

#define ClasspFreeSrb(ext, srb)                   \
  ExFreeToNPagedLookasideList(                    \
      &((ext)->CommonExtension.SrbLookasideList), \
      (srb))

#endif /* ALLOCATE_SRB_FROM_POOL */

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

#define CLASS_WORKING_SET_MAXIMUM                         2048

#define CLASS_INTERPRET_SENSE_INFO2_MAXIMUM_HISTORY_COUNT 30000

#define CLASS_SPECIAL_DISABLE_SPIN_DOWN                 0x00000001
#define CLASS_SPECIAL_DISABLE_SPIN_UP                   0x00000002
#define CLASS_SPECIAL_NO_QUEUE_LOCK                     0x00000008
#define CLASS_SPECIAL_DISABLE_WRITE_CACHE               0x00000010
#define CLASS_SPECIAL_CAUSE_NOT_REPORTABLE_HACK         0x00000020
#if ((NTDDI_VERSION == NTDDI_WIN2KSP3) || (OSVER(NTDDI_VERSION) == NTDDI_WINXP))
#define CLASS_SPECIAL_DISABLE_WRITE_CACHE_NOT_SUPPORTED 0x00000040
#endif
#define CLASS_SPECIAL_MODIFY_CACHE_UNSUCCESSFUL         0x00000040
#define CLASS_SPECIAL_FUA_NOT_SUPPORTED                 0x00000080
#define CLASS_SPECIAL_VALID_MASK                        0x000000FB
#define CLASS_SPECIAL_RESERVED         (~CLASS_SPECIAL_VALID_MASK)

#define DEV_WRITE_CACHE                                 0x00000001
#define DEV_USE_SCSI1                                   0x00000002
#define DEV_SAFE_START_UNIT                             0x00000004
#define DEV_NO_12BYTE_CDB                               0x00000008
#define DEV_POWER_PROTECTED                             0x00000010
#define DEV_USE_16BYTE_CDB                              0x00000020

#define GUID_CLASSPNP_QUERY_REGINFOEX {0x00e34b11, 0x2444, 0x4745, {0xa5, 0x3d, 0x62, 0x01, 0x00, 0xcd, 0x82, 0xf7}}
#define GUID_CLASSPNP_SENSEINFO2      {0x509a8c5f, 0x71d7, 0x48f6, {0x82, 0x1e, 0x17, 0x3c, 0x49, 0xbf, 0x2f, 0x18}}
#define GUID_CLASSPNP_WORKING_SET     {0x105701b0, 0x9e9b, 0x47cb, {0x97, 0x80, 0x81, 0x19, 0x8a, 0xf7, 0xb5, 0x24}}
#define GUID_CLASSPNP_SRB_SUPPORT     {0x0a483941, 0xbdfd, 0x4f7b, {0xbe, 0x95, 0xce, 0xe2, 0xa2, 0x16, 0x09, 0x0c}}

#define DEFAULT_FAILURE_PREDICTION_PERIOD 60 * 60 * 1

#define MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS (0x3b9aca00)

static inline ULONG CountOfSetBitsUChar(UCHAR _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
static inline ULONG CountOfSetBitsULong(ULONG _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
static inline ULONG CountOfSetBitsULong32(ULONG32 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
static inline ULONG CountOfSetBitsULong64(ULONG64 _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
static inline ULONG CountOfSetBitsUlongPtr(ULONG_PTR _X)
{ ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }

typedef enum _MEDIA_CHANGE_DETECTION_STATE {
  MediaUnknown,
  MediaPresent,
  MediaNotPresent,
  MediaUnavailable
} MEDIA_CHANGE_DETECTION_STATE, *PMEDIA_CHANGE_DETECTION_STATE;

typedef enum _CLASS_DEBUG_LEVEL {
  ClassDebugError = 0,
  ClassDebugWarning = 1,
  ClassDebugTrace = 2,
  ClassDebugInfo = 3,
  ClassDebugMediaLocks = 8,
  ClassDebugMCN = 9,
  ClassDebugDelayedRetry = 10,
  ClassDebugSenseInfo = 11,
  ClassDebugRemoveLock = 12,
  ClassDebugExternal4 = 13,
  ClassDebugExternal3 = 14,
  ClassDebugExternal2 = 15,
  ClassDebugExternal1 = 16
} CLASS_DEBUG_LEVEL, *PCLASS_DEBUG_LEVEL;

typedef enum {
  EventGeneration,
  DataBlockCollection
} CLASSENABLEDISABLEFUNCTION;

typedef enum {
  FailurePredictionNone = 0,
  FailurePredictionIoctl,
  FailurePredictionSmart,
  FailurePredictionSense
} FAILURE_PREDICTION_METHOD, *PFAILURE_PREDICTION_METHOD;

typedef enum {
  PowerDownDeviceInitial,
  PowerDownDeviceLocked,
  PowerDownDeviceStopped,
  PowerDownDeviceOff,
  PowerDownDeviceUnlocked
} CLASS_POWER_DOWN_STATE;

typedef enum {
  PowerDownDeviceInitial2,
  PowerDownDeviceLocked2,
  PowerDownDeviceFlushed2,
  PowerDownDeviceStopped2,
  PowerDownDeviceOff2,
  PowerDownDeviceUnlocked2
} CLASS_POWER_DOWN_STATE2;

typedef enum {
  PowerDownDeviceInitial3 = 0,
  PowerDownDeviceLocked3,
  PowerDownDeviceQuiesced3,
  PowerDownDeviceFlushed3,
  PowerDownDeviceStopped3,
  PowerDownDeviceOff3,
  PowerDownDeviceUnlocked3
} CLASS_POWER_DOWN_STATE3;

typedef enum {
  PowerUpDeviceInitial,
  PowerUpDeviceLocked,
  PowerUpDeviceOn,
  PowerUpDeviceStarted,
  PowerUpDeviceUnlocked
} CLASS_POWER_UP_STATE;

struct _CLASS_INIT_DATA;
typedef struct _CLASS_INIT_DATA CLASS_INIT_DATA, *PCLASS_INIT_DATA;

struct _CLASS_PRIVATE_FDO_DATA;
typedef struct _CLASS_PRIVATE_FDO_DATA CLASS_PRIVATE_FDO_DATA, *PCLASS_PRIVATE_FDO_DATA;

struct _CLASS_PRIVATE_PDO_DATA;
typedef struct _CLASS_PRIVATE_PDO_DATA CLASS_PRIVATE_PDO_DATA, *PCLASS_PRIVATE_PDO_DATA;

struct _CLASS_PRIVATE_COMMON_DATA;
typedef struct _CLASS_PRIVATE_COMMON_DATA CLASS_PRIVATE_COMMON_DATA, *PCLASS_PRIVATE_COMMON_DATA;

struct _MEDIA_CHANGE_DETECTION_INFO;
typedef struct _MEDIA_CHANGE_DETECTION_INFO MEDIA_CHANGE_DETECTION_INFO, *PMEDIA_CHANGE_DETECTION_INFO;

struct _DICTIONARY_HEADER;
typedef struct _DICTIONARY_HEADER DICTIONARY_HEADER, *PDICTIONARY_HEADER;

typedef struct _DICTIONARY {
  ULONGLONG Signature;
  struct _DICTIONARY_HEADER* List;
  KSPIN_LOCK SpinLock;
} DICTIONARY, *PDICTIONARY;

typedef struct _CLASSPNP_SCAN_FOR_SPECIAL_INFO {
  PCHAR VendorId;
  PCHAR ProductId;
  PCHAR ProductRevision;
  ULONG_PTR Data;
} CLASSPNP_SCAN_FOR_SPECIAL_INFO, *PCLASSPNP_SCAN_FOR_SPECIAL_INFO;

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef VOID
(NTAPI *PCLASS_ERROR)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PSCSI_REQUEST_BLOCK Srb,
  _Out_ NTSTATUS *Status,
  _Inout_ BOOLEAN *Retry);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_ADD_DEVICE)(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ PDEVICE_OBJECT Pdo);

typedef NTSTATUS
(NTAPI *PCLASS_POWER_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_START_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_STOP_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ UCHAR Type);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_INIT_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_ENUM_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_READ_WRITE)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_DEVICE_CONTROL)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_SHUTDOWN_FLUSH)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_CREATE_CLOSE)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_QUERY_ID)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ BUS_QUERY_ID_TYPE IdType,
  _In_ PUNICODE_STRING IdString);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_REMOVE_DEVICE)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ UCHAR Type);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef VOID
(NTAPI *PCLASS_UNLOAD)(
  _In_ PDRIVER_OBJECT DriverObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_QUERY_PNP_CAPABILITIES)(
  _In_ PDEVICE_OBJECT PhysicalDeviceObject,
  _In_ PDEVICE_CAPABILITIES Capabilities);

_IRQL_requires_(DISPATCH_LEVEL)
typedef VOID
(NTAPI *PCLASS_TICK)(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_QUERY_WMI_REGINFO_EX)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ ULONG *RegFlags,
  _Out_ PUNICODE_STRING Name,
  _Out_ PUNICODE_STRING MofResourceName);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_QUERY_WMI_REGINFO)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Out_ ULONG *RegFlags,
  _Out_ PUNICODE_STRING Name);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_QUERY_WMI_DATABLOCK)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _In_ ULONG BufferAvail,
  _Out_writes_bytes_(BufferAvail) PUCHAR Buffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_SET_WMI_DATABLOCK)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _In_ ULONG BufferSize,
  _In_reads_bytes_(BufferSize) PUCHAR Buffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_SET_WMI_DATAITEM)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _In_ ULONG DataItemId,
  _In_ ULONG BufferSize,
  _In_reads_bytes_(BufferSize) PUCHAR Buffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_EXECUTE_WMI_METHOD)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _In_ ULONG MethodId,
  _In_ ULONG InBufferSize,
  _In_ ULONG OutBufferSize,
  _In_reads_(_Inexpressible_(max(InBufferSize, OutBufferSize))) PUCHAR Buffer);

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef NTSTATUS
(NTAPI *PCLASS_WMI_FUNCTION_CONTROL)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG GuidIndex,
  _In_ CLASSENABLEDISABLEFUNCTION Function,
  _In_ BOOLEAN Enable);

typedef struct _SRB_HISTORY_ITEM {
  LARGE_INTEGER TickCountSent;
  LARGE_INTEGER TickCountCompleted;
  ULONG MillisecondsDelayOnRetry;
  SENSE_DATA NormalizedSenseData;
  UCHAR SrbStatus;
  UCHAR ClassDriverUse;
} SRB_HISTORY_ITEM, *PSRB_HISTORY_ITEM;

typedef struct _SRB_HISTORY {
  ULONG_PTR ClassDriverUse[4];
  _Field_range_(1,30000) ULONG TotalHistoryCount;
  _Field_range_(0,TotalHistoryCount) ULONG UsedHistoryCount;
  _Field_size_part_(TotalHistoryCount, UsedHistoryCount) SRB_HISTORY_ITEM History[1];
} SRB_HISTORY, *PSRB_HISTORY;

_IRQL_requires_max_(DISPATCH_LEVEL)
typedef BOOLEAN
(NTAPI *PCLASS_INTERPRET_SENSE_INFO)(
  _In_ PDEVICE_OBJECT Fdo,
  _In_opt_ PIRP OriginalRequest,
  _In_ PSCSI_REQUEST_BLOCK Srb,
  _In_ UCHAR MajorFunctionCode,
  _In_ ULONG IoDeviceCode,
  _In_ ULONG PreviousRetryCount,
  _In_opt_ SRB_HISTORY *RequestHistory,
  _Out_ NTSTATUS *Status,
  _Out_ _Deref_out_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
    LONGLONG *RetryIn100nsUnits);

_IRQL_requires_max_(DISPATCH_LEVEL)
_At_(RequestHistory->UsedHistoryCount, _Pre_equal_to_(RequestHistory->TotalHistoryCount)
   _Out_range_(0, RequestHistory->TotalHistoryCount - 1))
typedef VOID
(NTAPI *PCLASS_COMPRESS_RETRY_HISTORY_DATA)(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PSRB_HISTORY RequestHistory);

typedef struct {
  GUID Guid;
  ULONG InstanceCount;
  ULONG Flags;
} GUIDREGINFO, *PGUIDREGINFO;

typedef struct _CLASS_WMI_INFO {
  ULONG GuidCount;
  PGUIDREGINFO GuidRegInfo;
  PCLASS_QUERY_WMI_REGINFO ClassQueryWmiRegInfo;
  PCLASS_QUERY_WMI_DATABLOCK ClassQueryWmiDataBlock;
  PCLASS_SET_WMI_DATABLOCK ClassSetWmiDataBlock;
  PCLASS_SET_WMI_DATAITEM ClassSetWmiDataItem;
  PCLASS_EXECUTE_WMI_METHOD ClassExecuteWmiMethod;
  PCLASS_WMI_FUNCTION_CONTROL ClassWmiFunctionControl;
} CLASS_WMI_INFO, *PCLASS_WMI_INFO;

typedef struct _CLASS_DEV_INFO {
  ULONG DeviceExtensionSize;
  DEVICE_TYPE DeviceType;
  UCHAR StackSize;
  ULONG DeviceCharacteristics;
  PCLASS_ERROR ClassError;
  PCLASS_READ_WRITE ClassReadWriteVerification;
  PCLASS_DEVICE_CONTROL ClassDeviceControl;
  PCLASS_SHUTDOWN_FLUSH ClassShutdownFlush;
  PCLASS_CREATE_CLOSE ClassCreateClose;
  PCLASS_INIT_DEVICE ClassInitDevice;
  PCLASS_START_DEVICE ClassStartDevice;
  PCLASS_POWER_DEVICE ClassPowerDevice;
  PCLASS_STOP_DEVICE ClassStopDevice;
  PCLASS_REMOVE_DEVICE ClassRemoveDevice;
  PCLASS_QUERY_PNP_CAPABILITIES ClassQueryPnpCapabilities;
  CLASS_WMI_INFO ClassWmiInfo;
} CLASS_DEV_INFO, *PCLASS_DEV_INFO;

struct _CLASS_INIT_DATA {
  ULONG InitializationDataSize;
  CLASS_DEV_INFO FdoData;
  CLASS_DEV_INFO PdoData;
  PCLASS_ADD_DEVICE ClassAddDevice;
  PCLASS_ENUM_DEVICE ClassEnumerateDevice;
  PCLASS_QUERY_ID ClassQueryId;
  PDRIVER_STARTIO ClassStartIo;
  PCLASS_UNLOAD ClassUnload;
  PCLASS_TICK ClassTick;
};

typedef struct _FILE_OBJECT_EXTENSION {
  PFILE_OBJECT FileObject;
  PDEVICE_OBJECT DeviceObject;
  ULONG LockCount;
  ULONG McnDisableCount;
} FILE_OBJECT_EXTENSION, *PFILE_OBJECT_EXTENSION;

typedef struct _CLASS_WORKING_SET {
  _Field_range_(sizeof(CLASS_WORKING_SET),sizeof(CLASS_WORKING_SET)) ULONG Size;
  _Field_range_(0,2048) ULONG XferPacketsWorkingSetMaximum;
  _Field_range_(0,2048) ULONG XferPacketsWorkingSetMinimum;
} CLASS_WORKING_SET, *PCLASS_WORKING_SET;

typedef struct _CLASS_INTERPRET_SENSE_INFO2 {
  _Field_range_(sizeof(CLASS_INTERPRET_SENSE_INFO),sizeof(CLASS_INTERPRET_SENSE_INFO))
    ULONG Size;
  _Field_range_(1,30000) ULONG HistoryCount;
  __callback PCLASS_COMPRESS_RETRY_HISTORY_DATA Compress;
  __callback PCLASS_INTERPRET_SENSE_INFO Interpret;
} CLASS_INTERPRET_SENSE_INFO2, *PCLASS_INTERPRET_SENSE_INFO2;

C_ASSERT((MAXULONG - sizeof(SRB_HISTORY)) / 30000 >= sizeof(SRB_HISTORY_ITEM));

// for SrbSupport
#define CLASS_SRB_SCSI_REQUEST_BLOCK    0x1
#define CLASS_SRB_STORAGE_REQUEST_BLOCK 0x2

typedef struct _CLASS_DRIVER_EXTENSION {
  UNICODE_STRING RegistryPath;
  CLASS_INIT_DATA InitData;
  ULONG DeviceCount;
#if (NTDDI_VERSION >= NTDDI_WINXP)
  PCLASS_QUERY_WMI_REGINFO_EX ClassFdoQueryWmiRegInfoEx;
  PCLASS_QUERY_WMI_REGINFO_EX ClassPdoQueryWmiRegInfoEx;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
  REGHANDLE EtwHandle;
  PDRIVER_DISPATCH DeviceMajorFunctionTable[IRP_MJ_MAXIMUM_FUNCTION + 1];
  PDRIVER_DISPATCH MpDeviceMajorFunctionTable[IRP_MJ_MAXIMUM_FUNCTION + 1];
  PCLASS_INTERPRET_SENSE_INFO2 InterpretSenseInfo;
  PCLASS_WORKING_SET WorkingSet;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
  ULONG SrbSupport;
#endif
} CLASS_DRIVER_EXTENSION, *PCLASS_DRIVER_EXTENSION;

typedef struct _COMMON_DEVICE_EXTENSION {
  ULONG Version;
  PDEVICE_OBJECT DeviceObject;
  PDEVICE_OBJECT LowerDeviceObject;
  struct _FUNCTIONAL_DEVICE_EXTENSION *PartitionZeroExtension;
  PCLASS_DRIVER_EXTENSION DriverExtension;
  LONG RemoveLock;
  KEVENT RemoveEvent;
  KSPIN_LOCK RemoveTrackingSpinlock;
  PVOID RemoveTrackingList;
  LONG RemoveTrackingUntrackedCount;
  PVOID DriverData;
  _ANONYMOUS_STRUCT struct {
    BOOLEAN IsFdo:1;
    BOOLEAN IsInitialized:1;
    BOOLEAN IsSrbLookasideListInitialized:1;
  } DUMMYSTRUCTNAME;
  UCHAR PreviousState;
  UCHAR CurrentState;
  ULONG IsRemoved;
  UNICODE_STRING DeviceName;
  struct _PHYSICAL_DEVICE_EXTENSION *ChildList;
  ULONG PartitionNumber;
  LARGE_INTEGER PartitionLength;
  LARGE_INTEGER StartingOffset;
  PCLASS_DEV_INFO DevInfo;
  ULONG PagingPathCount;
  ULONG DumpPathCount;
  ULONG HibernationPathCount;
  KEVENT PathCountEvent;
#ifndef ALLOCATE_SRB_FROM_POOL
  NPAGED_LOOKASIDE_LIST SrbLookasideList;
#endif
  UNICODE_STRING MountedDeviceInterfaceName;
  ULONG GuidCount;
  PGUIDREGINFO GuidRegInfo;
  DICTIONARY FileObjectDictionary;
#if (NTDDI_VERSION >= NTDDI_WINXP)
  PCLASS_PRIVATE_COMMON_DATA PrivateCommonData;
#else
  ULONG_PTR Reserved1;
#endif
#if (NTDDI_VERSION >= NTDDI_VISTA)
  PDRIVER_DISPATCH *DispatchTable;
#else
  ULONG_PTR Reserved2;
#endif
  ULONG_PTR Reserved3;
  ULONG_PTR Reserved4;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _PHYSICAL_DEVICE_EXTENSION {
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      ULONG Version;
      PDEVICE_OBJECT DeviceObject;
    } DUMMYSTRUCTNAME;
    COMMON_DEVICE_EXTENSION CommonExtension;
  } DUMMYUNIONNAME;
  BOOLEAN IsMissing;
  BOOLEAN IsEnumerated;
#if (NTDDI_VERSION >= NTDDI_WINXP)
  PCLASS_PRIVATE_PDO_DATA PrivatePdoData;
#else
  ULONG_PTR Reserved1;
#endif
  ULONG_PTR Reserved2;
  ULONG_PTR Reserved3;
  ULONG_PTR Reserved4;
} PHYSICAL_DEVICE_EXTENSION, *PPHYSICAL_DEVICE_EXTENSION;

typedef struct _CLASS_POWER_OPTIONS {
  ULONG PowerDown:1;
  ULONG LockQueue:1;
  ULONG HandleSpinDown:1;
  ULONG HandleSpinUp:1;
  ULONG Reserved:27;
} CLASS_POWER_OPTIONS, *PCLASS_POWER_OPTIONS;

typedef struct _CLASS_POWER_CONTEXT {
  union {
    CLASS_POWER_DOWN_STATE PowerDown;
    CLASS_POWER_DOWN_STATE2 PowerDown2;
    CLASS_POWER_DOWN_STATE3 PowerDown3;
    CLASS_POWER_UP_STATE PowerUp;
  } PowerChangeState;
  CLASS_POWER_OPTIONS Options;
  BOOLEAN InUse;
  BOOLEAN QueueLocked;
  NTSTATUS FinalStatus;
  ULONG RetryCount;
  ULONG RetryInterval;
  PIO_COMPLETION_ROUTINE CompletionRoutine;
  PDEVICE_OBJECT DeviceObject;
  PIRP Irp;
  SCSI_REQUEST_BLOCK Srb;
} CLASS_POWER_CONTEXT, *PCLASS_POWER_CONTEXT;

#if (NTDDI_VERSION >= NTDDI_WIN8)

#define CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE (sizeof(STORAGE_REQUEST_BLOCK) + sizeof(STOR_ADDR_BTL8) + sizeof(SRBEX_DATA_SCSI_CDB16))
#define CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE (sizeof(STORAGE_REQUEST_BLOCK) + sizeof(STOR_ADDR_BTL8))

#endif

typedef struct _COMPLETION_CONTEXT {
  PDEVICE_OBJECT DeviceObject;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  union
  {
    SCSI_REQUEST_BLOCK Srb;
    STORAGE_REQUEST_BLOCK SrbEx;
    UCHAR SrbExBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE];
  } Srb;
#else
  SCSI_REQUEST_BLOCK Srb;
#endif
} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
SCSIPORT_API
ULONG
NTAPI
ClassInitialize(
  _In_ PVOID Argument1,
  _In_ PVOID Argument2,
  _In_ PCLASS_INIT_DATA InitializationData);

typedef struct _CLASS_QUERY_WMI_REGINFO_EX_LIST {
  ULONG Size;
  __callback PCLASS_QUERY_WMI_REGINFO_EX ClassFdoQueryWmiRegInfoEx;
  __callback PCLASS_QUERY_WMI_REGINFO_EX ClassPdoQueryWmiRegInfoEx;
} CLASS_QUERY_WMI_REGINFO_EX_LIST, *PCLASS_QUERY_WMI_REGINFO_EX_LIST;

typedef enum
{
  SupportUnknown = 0,
  Supported,
  NotSupported
} CLASS_FUNCTION_SUPPORT;

typedef struct _CLASS_VPD_B1_DATA
{
  NTSTATUS CommandStatus;
  USHORT MediumRotationRate;
  UCHAR NominalFormFactor;
  UCHAR Zoned;
  ULONG MediumProductType;
  ULONG DepopulationTime;
} CLASS_VPD_B1_DATA, *PCLASS_VPD_B1_DATA;

typedef struct _CLASS_VPD_B0_DATA
{
  NTSTATUS CommandStatus;
  ULONG MaxUnmapLbaCount;
  ULONG MaxUnmapBlockDescrCount;
  ULONG OptimalUnmapGranularity;
  ULONG UnmapGranularityAlignment;
  BOOLEAN UGAVALID;
  UCHAR Reserved0;
  USHORT OptimalTransferLengthGranularity;
  ULONG MaximumTransferLength;
  ULONG OptimalTransferLength;
} CLASS_VPD_B0_DATA, *PCLASS_VPD_B0_DATA;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4214)
#endif
typedef struct _CLASS_VPD_B2_DATA
{
  NTSTATUS CommandStatus;
  UCHAR ThresholdExponent;
  UCHAR DP:1;
  UCHAR ANC_SUP:1;
  UCHAR Reserved0:2;
  UCHAR LBPRZ:1;
  UCHAR LBPWS10:1;
  UCHAR LBPWS:1;
  UCHAR LBPU:1;
  UCHAR ProvisioningType:3;
  UCHAR Reserved1:5;
  ULONG SoftThresholdEventPending;
} CLASS_VPD_B2_DATA, *PCLASS_VPD_B2_DATA;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

typedef struct _CLASS_READ_CAPACITY16_DATA
{
  NTSTATUS CommandStatus;
  ULONG BytesPerLogicalSector;
  ULONG BytesPerPhysicalSector;
  ULONG BytesOffsetForSectorAlignment;
  BOOLEAN LBProvisioningEnabled;
  BOOLEAN LBProvisioningReadZeros;
  UCHAR Reserved0[2];
  ULONG Reserved1;
} CLASS_READ_CAPACITY16_DATA, *PCLASS_READ_CAPACITY16_DATA;

typedef struct _CLASS_VPD_ECOP_BLOCK_DEVICE_ROD_LIMITS
{
  NTSTATUS CommandStatus;
  USHORT MaximumRangeDescriptors;
  UCHAR Restricted;
  UCHAR Reserved;
  ULONG MaximumInactivityTimer;
  ULONG DefaultInactivityTimer;
  ULONGLONG MaximumTokenTransferSize;
  ULONGLONG OptimalTransferCount;
} CLASS_VPD_ECOP_BLOCK_DEVICE_ROD_LIMITS, *PCLASS_VPD_ECOP_BLOCK_DEVICE_ROD_LIMITS;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4214)
#endif
typedef struct _CLASS_FUNCTION_SUPPORT_INFO
{
  KSPIN_LOCK SyncLock;
  ULONG GenerationCount;
  volatile ULONG ChangeRequestCount;
  struct
  {
    ULONG BlockLimits:1;
    ULONG BlockDeviceCharacteristics:1;
    ULONG LBProvisioning:1;
    ULONG BlockDeviceRODLimits:1;
    ULONG ZonedBlockDeviceCharacteristics:1;
    ULONG Reserved:22;
    ULONG DeviceType:5;
  } ValidInquiryPages;
  struct
  {
    CLASS_FUNCTION_SUPPORT SeekPenaltyProperty;
    CLASS_FUNCTION_SUPPORT AccessAlignmentProperty;
    CLASS_FUNCTION_SUPPORT TrimProperty;
    CLASS_FUNCTION_SUPPORT TrimProcess;
  } LowerLayerSupport;
  BOOLEAN RegAccessAlignmentQueryNotSupported;
  BOOLEAN AsynchronousNotificationSupported;
#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)
  BOOLEAN UseModeSense10;
  UCHAR Reserved;
#else
  UCHAR Reserved[2];
#endif
  CLASS_VPD_B0_DATA BlockLimitsData;
  CLASS_VPD_B1_DATA DeviceCharacteristicsData;
  CLASS_VPD_B2_DATA LBProvisioningData;
  CLASS_READ_CAPACITY16_DATA ReadCapacity16Data;
  CLASS_VPD_ECOP_BLOCK_DEVICE_ROD_LIMITS BlockDeviceRODLimitsData;
  struct
  {
    ULONG D3ColdSupported:1;
    ULONG DeviceWakeable:1;
    ULONG IdlePowerEnabled:1;
    ULONG D3IdleTimeoutOverridden:1;
    ULONG NoVerifyDuringIdlePower:1;
    ULONG Reserved2:27;
    ULONG D3IdleTimeout;
  } IdlePower;

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
  CLASS_FUNCTION_SUPPORT HwFirmwareGetInfoSupport;
  PSTORAGE_HW_FIRMWARE_INFO HwFirmwareInfo;
#endif
} CLASS_FUNCTION_SUPPORT_INFO, *PCLASS_FUNCTION_SUPPORT_INFO;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

typedef struct _FUNCTIONAL_DEVICE_EXTENSION {
  _ANONYMOUS_UNION union {
    _ANONYMOUS_STRUCT struct {
      ULONG Version;
      PDEVICE_OBJECT DeviceObject;
    } DUMMYSTRUCTNAME;
    COMMON_DEVICE_EXTENSION CommonExtension;
  } DUMMYUNIONNAME;
  PDEVICE_OBJECT LowerPdo;
  PSTORAGE_DEVICE_DESCRIPTOR DeviceDescriptor;
  PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;
  DEVICE_POWER_STATE DevicePowerState;
  ULONG DMByteSkew;
  ULONG DMSkew;
  BOOLEAN DMActive;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  UCHAR SenseDataLength;
#else
  UCHAR Reserved;
#endif
  UCHAR Reserved0[2];
  DISK_GEOMETRY DiskGeometry;
  PSENSE_DATA SenseData;
  ULONG TimeOutValue;
  ULONG DeviceNumber;
  ULONG SrbFlags;
  ULONG ErrorCount;
  LONG LockCount;
  LONG ProtectedLockCount;
  LONG InternalLockCount;
  KEVENT EjectSynchronizationEvent;
  USHORT DeviceFlags;
  UCHAR SectorShift;
#if (NTDDI_VERSION >= NTDDI_VISTA)
  UCHAR CdbForceUnitAccess;
#else
  UCHAR ReservedByte;
#endif
  PMEDIA_CHANGE_DETECTION_INFO MediaChangeDetectionInfo;
  PKEVENT Unused1;
  HANDLE Unused2;
  FILE_OBJECT_EXTENSION KernelModeMcnContext;
  ULONG MediaChangeCount;
  HANDLE DeviceDirectory;
  KSPIN_LOCK ReleaseQueueSpinLock;
  PIRP ReleaseQueueIrp;
  SCSI_REQUEST_BLOCK ReleaseQueueSrb;
  BOOLEAN ReleaseQueueNeeded;
  BOOLEAN ReleaseQueueInProgress;
  BOOLEAN ReleaseQueueIrpFromPool;
  BOOLEAN FailurePredicted;
  ULONG FailureReason;
  struct _FAILURE_PREDICTION_INFO* FailurePredictionInfo;
  BOOLEAN PowerDownInProgress;
  ULONG EnumerationInterlock;
  KEVENT ChildLock;
  PKTHREAD ChildLockOwner;
  ULONG ChildLockAcquisitionCount;
  ULONG ScanForSpecialFlags;
  KDPC PowerRetryDpc;
  KTIMER PowerRetryTimer;
  CLASS_POWER_CONTEXT PowerContext;

#if (NTDDI_VERSION <= NTDDI_WIN2K)

#if (SPVER(NTDDI_VERSION) < 2))
  ULONG_PTR Reserved1;
  ULONG_PTR Reserved2;
  ULONG_PTR Reserved3;
  ULONG_PTR Reserved4;
#else
  ULONG CompletionSuccessCount;
  ULONG SavedSrbFlags;
  ULONG SavedErrorCount;
  ULONG_PTR Reserved1;
#endif /* (SPVER(NTDDI_VERSION) < 2) */

#else /* (NTDDI_VERSION <= NTDDI_WIN2K) */

  PCLASS_PRIVATE_FDO_DATA PrivateFdoData;
#if (NTDDI_VERSION >= NTDDI_WIN8)
  PCLASS_FUNCTION_SUPPORT_INFO FunctionSupportInfo;
  PSTORAGE_MINIPORT_DESCRIPTOR MiniportDescriptor;
#else
  ULONG_PTR Reserved2;
  ULONG_PTR Reserved3;
#endif

#if (NTDDI_VERSION >= NTDDI_WINTHRESHOLD)
  PADDITIONAL_FDO_DATA AdditionalFdoData;
#else
  ULONG_PTR Reserved4;
#endif
#endif /* (NTDDI_VERSION <= NTDDI_WIN2K) */

} FUNCTIONAL_DEVICE_EXTENSION, *PFUNCTIONAL_DEVICE_EXTENSION;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
SCSIPORT_API
ULONG
NTAPI
ClassInitializeEx(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_ LPGUID Guid,
  _In_ PVOID Data);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Post_satisfies_(return <= 0)
SCSIPORT_API
NTSTATUS
NTAPI
ClassCreateDeviceObject(
  _In_ PDRIVER_OBJECT DriverObject,
  _In_z_ PCCHAR ObjectNameBuffer,
  _In_ PDEVICE_OBJECT LowerDeviceObject,
  _In_ BOOLEAN IsFdo,
  _Outptr_result_nullonfailure_ _At_(*DeviceObject, __drv_allocatesMem(Mem) __drv_aliasesMem)
    PDEVICE_OBJECT *DeviceObject);

_Must_inspect_result_
SCSIPORT_API
NTSTATUS
NTAPI
ClassReadDriveCapacity(
  _In_ PDEVICE_OBJECT DeviceObject);

SCSIPORT_API
VOID
NTAPI
ClassReleaseQueue(
  _In_ PDEVICE_OBJECT DeviceObject);

SCSIPORT_API
VOID
NTAPI
ClassSplitRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ ULONG MaximumBytes);

SCSIPORT_API
NTSTATUS
NTAPI
ClassDeviceControl(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp);

SCSIPORT_API
NTSTATUS
NTAPI
ClassIoComplete(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context);

SCSIPORT_API
NTSTATUS
NTAPI
ClassIoCompleteAssociated(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context);

SCSIPORT_API
BOOLEAN
NTAPI
ClassInterpretSenseInfo(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PSCSI_REQUEST_BLOCK Srb,
  _In_ UCHAR MajorFunctionCode,
  _In_ ULONG IoDeviceCode,
  _In_ ULONG RetryCount,
  _Out_ NTSTATUS *Status,
  _Out_opt_ _Deref_out_range_(0,100) ULONG *RetryInterval);

VOID
NTAPI
ClassSendDeviceIoControlSynchronous(
  _In_ ULONG IoControlCode,
  _In_ PDEVICE_OBJECT TargetDeviceObject,
  _Inout_updates_opt_(_Inexpressible_(max(InputBufferLength, OutputBufferLength)))
    PVOID Buffer,
  _In_ ULONG InputBufferLength,
  _In_ ULONG OutputBufferLength,
  _In_ BOOLEAN InternalDeviceIoControl,
  _Out_ PIO_STATUS_BLOCK IoStatus);

SCSIPORT_API
NTSTATUS
NTAPI
ClassSendIrpSynchronous(
  _In_ PDEVICE_OBJECT TargetDeviceObject,
  _In_ PIRP Irp);

SCSIPORT_API
NTSTATUS
NTAPI
ClassForwardIrpSynchronous(
  _In_ PCOMMON_DEVICE_EXTENSION CommonExtension,
  _In_ PIRP Irp);

SCSIPORT_API
NTSTATUS
NTAPI
ClassSendSrbSynchronous(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PSCSI_REQUEST_BLOCK Srb,
  _In_reads_bytes_opt_(BufferLength) PVOID BufferAddress,
  _In_ ULONG BufferLength,
  _In_ BOOLEAN WriteToDevice);

SCSIPORT_API
NTSTATUS
NTAPI
ClassSendSrbAsynchronous(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PSCSI_REQUEST_BLOCK Srb,
  _In_ PIRP Irp,
  _In_reads_bytes_opt_(BufferLength) __drv_aliasesMem PVOID BufferAddress,
  _In_ ULONG BufferLength,
  _In_ BOOLEAN WriteToDevice);

SCSIPORT_API
NTSTATUS
NTAPI
ClassBuildRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

SCSIPORT_API
ULONG
NTAPI
ClassModeSense(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
  _In_ ULONG Length,
  _In_ UCHAR PageMode);

SCSIPORT_API
PVOID
NTAPI
ClassFindModePage(
  _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
  _In_ ULONG Length,
  _In_ UCHAR PageMode,
  _In_ BOOLEAN Use6Byte);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
NTSTATUS
NTAPI
ClassClaimDevice(
  _In_ PDEVICE_OBJECT LowerDeviceObject,
  _In_ BOOLEAN Release);

SCSIPORT_API
NTSTATUS
NTAPI
ClassInternalIoControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassInitializeSrbLookasideList(
  _Inout_ PCOMMON_DEVICE_EXTENSION CommonExtension,
  _In_ ULONG NumberElements);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassDeleteSrbLookasideList(
  _Inout_ PCOMMON_DEVICE_EXTENSION CommonExtension);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
ULONG
NTAPI
ClassQueryTimeOutRegistryValue(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
NTSTATUS
NTAPI
ClassGetDescriptor(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PSTORAGE_PROPERTY_ID PropertyId,
  _Outptr_ PVOID *Descriptor);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassInvalidateBusRelations(
  _In_ PDEVICE_OBJECT Fdo);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassMarkChildrenMissing(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION Fdo);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
BOOLEAN
NTAPI
ClassMarkChildMissing(
  _In_ PPHYSICAL_DEVICE_EXTENSION PdoExtension,
  _In_ BOOLEAN AcquireChildLock);

SCSIPORT_API
VOID
ClassDebugPrint(
  _In_ CLASS_DEBUG_LEVEL DebugPrintLevel,
  _In_z_ PCCHAR DebugMessage,
  ...);

__drv_aliasesMem
_IRQL_requires_max_(DISPATCH_LEVEL)
SCSIPORT_API
PCLASS_DRIVER_EXTENSION
NTAPI
ClassGetDriverExtension(
  _In_ PDRIVER_OBJECT DriverObject);

SCSIPORT_API
VOID
NTAPI
ClassCompleteRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp,
  _In_ CCHAR PriorityBoost);

SCSIPORT_API
VOID
NTAPI
ClassReleaseRemoveLock(
  _In_ PDEVICE_OBJECT DeviceObject,
  PIRP Tag);

SCSIPORT_API
ULONG
NTAPI
ClassAcquireRemoveLockEx(
  _In_ PDEVICE_OBJECT DeviceObject,
  PVOID Tag,
  _In_ PCSTR File,
  _In_ ULONG Line);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassUpdateInformationInRegistry(
  _In_ PDEVICE_OBJECT Fdo,
  _In_ PCHAR DeviceName,
  _In_ ULONG DeviceNumber,
  _In_reads_bytes_opt_(InquiryDataLength) PINQUIRYDATA InquiryData,
  _In_ ULONG InquiryDataLength);

SCSIPORT_API
NTSTATUS
NTAPI
ClassWmiCompleteRequest(
  _In_ PDEVICE_OBJECT DeviceObject,
  _Inout_ PIRP Irp,
  _In_ NTSTATUS Status,
  _In_ ULONG BufferUsed,
  _In_ CCHAR PriorityBoost);

_IRQL_requires_max_(DISPATCH_LEVEL)
SCSIPORT_API
NTSTATUS
NTAPI
ClassWmiFireEvent(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ LPGUID Guid,
  _In_ ULONG InstanceIndex,
  _In_ ULONG EventDataSize,
  _In_reads_bytes_(EventDataSize) PVOID EventData);

SCSIPORT_API
VOID
NTAPI
ClassResetMediaChangeTimer(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassInitializeMediaChangeDetection(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ PUCHAR EventPrefix);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
NTSTATUS
NTAPI
ClassInitializeTestUnitPolling(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ BOOLEAN AllowDriveToSleep);

SCSIPORT_API
PVPB
NTAPI
ClassGetVpb(
  _In_ PDEVICE_OBJECT DeviceObject);

SCSIPORT_API
NTSTATUS
NTAPI
ClassSpinDownPowerHandler(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

NTSTATUS
NTAPI
ClassStopUnitPowerHandler(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ PIRP Irp);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ClassSetFailurePredictionPoll(
  _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ FAILURE_PREDICTION_METHOD FailurePredictionMethod,
  _In_ ULONG PollingPeriod);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
ClassNotifyFailurePredicted(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_reads_bytes_(BufferSize) PUCHAR Buffer,
  _In_ ULONG BufferSize,
  _In_ BOOLEAN LogError,
  _In_ ULONG UniqueErrorValue,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassAcquireChildLock(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

SCSIPORT_API
VOID
NTAPI
ClassReleaseChildLock(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

IO_COMPLETION_ROUTINE ClassSignalCompletion;

VOID
NTAPI
ClassSendStartUnit(
  _In_ PDEVICE_OBJECT DeviceObject);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
NTSTATUS
NTAPI
ClassRemoveDevice(
  _In_ PDEVICE_OBJECT DeviceObject,
  _In_ UCHAR RemoveType);

SCSIPORT_API
NTSTATUS
NTAPI
ClassAsynchronousCompletion(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Event);

SCSIPORT_API
VOID
NTAPI
ClassCheckMediaState(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

SCSIPORT_API
NTSTATUS
NTAPI
ClassCheckVerifyComplete(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp,
  PVOID Context);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassSetMediaChangeState(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ MEDIA_CHANGE_DETECTION_STATE State,
  _In_ BOOLEAN Wait);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassEnableMediaChangeDetection(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassDisableMediaChangeDetection(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

_IRQL_requires_max_(PASSIVE_LEVEL)
SCSIPORT_API
VOID
NTAPI
ClassCleanupMediaChangeDetection(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
ClassGetDeviceParameter(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_opt_ PWSTR SubkeyName,
  _In_ PWSTR ParameterName,
  _Inout_ PULONG ParameterValue);

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
ClassSetDeviceParameter(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_opt_ PWSTR SubkeyName,
  _In_ PWSTR ParameterName,
  _In_ ULONG ParameterValue);

#if (NTDDI_VERSION >= NTDDI_VISTA)

_IRQL_requires_max_(PASSIVE_LEVEL)
PFILE_OBJECT_EXTENSION
NTAPI
ClassGetFsContext(
  _In_ PCOMMON_DEVICE_EXTENSION CommonExtension,
  _In_ PFILE_OBJECT FileObject);

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
ClassSendNotification(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ const GUID *Guid,
  _In_ ULONG ExtraDataSize,
  _In_reads_bytes_opt_(ExtraDataSize) PVOID ExtraData);

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

__inline
UCHAR
GET_FDO_EXTENSON_SENSE_DATA_LENGTH (
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension)
{
  UCHAR SenseDataLength = 0;

  if (FdoExtension->SenseData != NULL)
  {
#if (NTDDI_VERSION >= NTDDI_WIN8)
    if (FdoExtension->SenseDataLength > 0)
    {
        SenseDataLength = FdoExtension->SenseDataLength;
    }
    else
    {
        // For backward compatibility with Windows 7 and earlier
        SenseDataLength = SENSE_BUFFER_SIZE;
    }
#else
    SenseDataLength = SENSE_BUFFER_SIZE;
#endif
  }

  return SenseDataLength;
}

static __inline
BOOLEAN
PORT_ALLOCATED_SENSE(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ PSCSI_REQUEST_BLOCK Srb)
{
  return ((BOOLEAN)((TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE) &&
          TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER))                &&
          (Srb->SenseInfoBuffer != FdoExtension->SenseData)));
}

static __inline
VOID
FREE_PORT_ALLOCATED_SENSE_BUFFER(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ PSCSI_REQUEST_BLOCK Srb)
{
  ASSERT(TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
  ASSERT(TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
  ASSERT(Srb->SenseInfoBuffer != FdoExtension->SenseData);

  ExFreePool(Srb->SenseInfoBuffer);
  Srb->SenseInfoBuffer = FdoExtension->SenseData;
  Srb->SenseInfoBufferLength = SENSE_BUFFER_SIZE;
  CLEAR_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER);
  return;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
typedef VOID
(NTAPI *PCLASS_SCAN_FOR_SPECIAL_HANDLER)(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ ULONG_PTR Data);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
ClassScanForSpecial(
  _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
  _In_ CLASSPNP_SCAN_FOR_SPECIAL_INFO DeviceList[],
  _In_ PCLASS_SCAN_FOR_SPECIAL_HANDLER Function);
