/*++

Copyright (C) Microsoft Corporation, 1991 - 2010

Module Name:

    classp.h

Abstract:

    Private header file for classpnp.sys modules.  This contains private
    structure and function declarations as well as constant values which do
    not need to be exported.

Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#define RTL_USE_AVL_TABLES 0

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>

#include <ntddk.h>

#ifdef __REACTOS__
#include <pseh/pseh2.h>
#endif

#include <scsi.h>

#include <wmidata.h>
#include <classpnp.h>
#include <storduid.h>

#if CLASS_INIT_GUID
#include <initguid.h>
#endif

#include <mountdev.h>
#include <ioevent.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>

#include <wdmguid.h>

#if (NTDDI_VERSION >= NTDDI_WIN8)

#include <ntpoapi.h>

#include <srbhelper.h>

#endif

#ifdef __REACTOS__
#undef MdlMappingNoExecute
#define MdlMappingNoExecute 0
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#undef POOL_NX_ALLOCATION
#define POOL_NX_ALLOCATION 0
#endif

//
// Set component ID for DbgPrintEx calls
//
#ifndef DEBUG_COMP_ID
#define DEBUG_COMP_ID   DPFLTR_CLASSPNP_ID
#endif

//
// Include header file and setup GUID for tracing
//
#include <storswtr.h>
#define WPP_GUID_CLASSPNP       (FA8DE7C4, ACDE, 4443, 9994, C4E2359A9EDB)
#ifndef WPP_CONTROL_GUIDS
#define WPP_CONTROL_GUIDS       WPP_CONTROL_GUIDS_NORMAL_FLAGS(WPP_GUID_CLASSPNP)
#endif

/*
 *  IA64 requires 8-byte alignment for pointers, but the IA64 NT kernel expects 16-byte alignment
 */
#ifdef _WIN64
    #define PTRALIGN                DECLSPEC_ALIGN(16)
#else
    #define PTRALIGN
#endif


extern CLASSPNP_SCAN_FOR_SPECIAL_INFO ClassBadItems[];

extern GUID ClassGuidQueryRegInfoEx;
extern GUID ClassGuidSenseInfo2;
extern GUID ClassGuidWorkingSet;
extern GUID ClassGuidSrbSupport;

extern ULONG ClassMaxInterleavePerCriticalIo;


#define Add2Ptr(P,I)                                ((PVOID)((PUCHAR)(P) + (I)))

#define CLASSP_REG_SUBKEY_NAME                      (L"Classpnp")

#define CLASSP_REG_HACK_VALUE_NAME                  (L"HackMask")
#define CLASSP_REG_MMC_DETECTION_VALUE_NAME         (L"MMCDetectionState")
#define CLASSP_REG_WRITE_CACHE_VALUE_NAME           (L"WriteCacheEnableOverride")
#define CLASSP_REG_PERF_RESTORE_VALUE_NAME          (L"RestorePerfAtCount")
#define CLASSP_REG_REMOVAL_POLICY_VALUE_NAME        (L"UserRemovalPolicy")
#define CLASSP_REG_IDLE_INTERVAL_NAME               (L"IdleInterval")
#define CLASSP_REG_IDLE_ACTIVE_MAX                  (L"IdleOutstandingIoMax")
#define CLASSP_REG_IDLE_PRIORITY_SUPPORTED          (L"IdlePrioritySupported")
#define CLASSP_REG_ACCESS_ALIGNMENT_NOT_SUPPORTED   (L"AccessAlignmentQueryNotSupported")
#define CLASSP_REG_DISBALE_IDLE_POWER_NAME          (L"DisableIdlePowerManagement")
#define CLASSP_REG_IDLE_TIMEOUT_IN_SECONDS          (L"IdleTimeoutInSeconds")
#define CLASSP_REG_DISABLE_D3COLD                   (L"DisableD3Cold")
#define CLASSP_REG_QERR_OVERRIDE_MODE               (L"QERROverrideMode")
#define CLASSP_REG_LEGACY_ERROR_HANDLING            (L"LegacyErrorHandling")
#define CLASSP_REG_COPY_OFFLOAD_MAX_TARGET_DURATION (L"CopyOffloadMaxTargetDuration")

#define CLASS_PERF_RESTORE_MINIMUM                  (0x10)
#define CLASS_ERROR_LEVEL_1                         (0x4)
#define CLASS_ERROR_LEVEL_2                         (0x8)
#define CLASS_MAX_INTERLEAVE_PER_CRITICAL_IO        (0x4)

#define FDO_HACK_CANNOT_LOCK_MEDIA                  (0x00000001)
#define FDO_HACK_GESN_IS_BAD                        (0x00000002)
#define FDO_HACK_NO_SYNC_CACHE                      (0x00000004)
#define FDO_HACK_NO_RESERVE6                        (0x00000008)
#define FDO_HACK_GESN_IGNORE_OPCHANGE               (0x00000010)

#define FDO_HACK_VALID_FLAGS                        (0x0000001F)
#define FDO_HACK_INVALID_FLAGS                      (~FDO_HACK_VALID_FLAGS)

/*
 *  Lots of retries of synchronized SCSI commands that devices may not
 *  even support really slows down the system (especially while booting).
 *  (Even GetDriveCapacity may be failed on purpose if an external disk is powered off).
 *  If a disk cannot return a small initialization buffer at startup
 *  in two attempts (with delay interval) then we cannot expect it to return
 *  data consistently with four retries.
 *  So don't set the retry counts as high here as for data SRBs.
 *
 *  If we find that these requests are failing consecutively,
 *  despite the retry interval, on otherwise reliable media,
 *  then we should either increase the retry interval for
 *  that failure or (by all means) increase these retry counts as appropriate.
 */
#define NUM_LOCKMEDIAREMOVAL_RETRIES    1
#define NUM_MODESENSE_RETRIES           1
#define NUM_MODESELECT_RETRIES          1
#define NUM_DRIVECAPACITY_RETRIES       1
#define NUM_THIN_PROVISIONING_RETRIES   32

#if (NTDDI_VERSION >= NTDDI_WINBLUE)

//
// New code should use the MAXIMUM_RETRIES value.
//
#define NUM_IO_RETRIES MAXIMUM_RETRIES
#define LEGACY_NUM_IO_RETRIES 8

#else

/*
 *  We retry failed I/O requests at 1-second intervals.
 *  In the case of a failure due to bus reset, we want to make sure that we retry after the allowable
 *  reset time.  For SCSI, the allowable reset time is 5 seconds.  ScsiPort queues requests during
 *  a bus reset, which should cause us to retry after the reset is over; but the requests queued in
 *  the miniport are failed all the way back to us immediately.  In any event, in order to make
 *  extra sure that our retries span the allowable reset time, we should retry more than 5 times.
 */
#define NUM_IO_RETRIES      8

#endif // NTDDI_VERSION >= NTDDI_WINBLUE

#define CLASS_FILE_OBJECT_EXTENSION_KEY             'eteP'
#define CLASSP_VOLUME_VERIFY_CHECKED                0x34

#define CLASS_TAG_PRIVATE_DATA                      'CPcS'
#define CLASS_TAG_SENSE2                            '2ScS'
#define CLASS_TAG_WORKING_SET                       'sWcS'
#define CLASSPNP_POOL_TAG_GENERIC                   'pCcS'
#define CLASSPNP_POOL_TAG_TOKEN_OPERATION           'oTcS'
#define CLASSPNP_POOL_TAG_SRB                       'rScS'
#define CLASSPNP_POOL_TAG_VPD                       'pVcS'
#define CLASSPNP_POOL_TAG_LOG_MESSAGE               'mlcS'
#define CLASSPNP_POOL_TAG_ADDITIONAL_DATA           'DAcS'
#define CLASSPNP_POOL_TAG_FIRMWARE                  'wFcS'

//
// Macros related to Token Operation commands
//
#define MAX_LIST_IDENTIFIER                                         MAXULONG
#define NUM_POPULATE_TOKEN_RETRIES                                  1
#define NUM_WRITE_USING_TOKEN_RETRIES                               2
#define NUM_RECEIVE_TOKEN_INFORMATION_RETRIES                       2
#define MAX_TOKEN_OPERATION_PARAMETER_DATA_LENGTH                   MAXUSHORT
#define MAX_RECEIVE_TOKEN_INFORMATION_PARAMETER_DATA_LENGTH         MAXULONG
#define MAX_TOKEN_TRANSFER_SIZE                                     MAXULONGLONG
#define MAX_NUMBER_BLOCKS_PER_BLOCK_DEVICE_RANGE_DESCRIPTOR         MAXULONG
#define DEFAULT_MAX_TARGET_DURATION                                 4 // 4sec
#define DEFAULT_MAX_NUMBER_BYTES_PER_SYNC_WRITE_USING_TOKEN         (64ULL * 1024 * 1024)  // 64MB
#define MAX_NUMBER_BYTES_PER_SYNC_WRITE_USING_TOKEN                 (256ULL * 1024 * 1024) // 256MB
#define MIN_TOKEN_LIST_IDENTIFIERS                                  256
#define MAX_TOKEN_LIST_IDENTIFIERS                                  MAXULONG
#define MAX_NUMBER_BLOCK_DEVICE_DESCRIPTORS                         64

#define REG_DISK_CLASS_CONTROL                                      L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\Control\\DISK"
#define REG_MAX_LIST_IDENTIFIER_VALUE                               L"MaximumListIdentifier"

#define VPD_PAGE_HEADER_SIZE                                        0x04


//
// Number of times to retry get LBA status in case of an error
// that can be caused by VPD data change
//

#define GET_LBA_STATUS_RETRY_COUNT_MAX                              (2)

extern ULONG MaxTokenOperationListIdentifier;
extern volatile ULONG TokenOperationListIdentifier;

extern LIST_ENTRY IdlePowerFDOList;
extern PVOID PowerSettingNotificationHandle;
extern PVOID ScreenStateNotificationHandle;
extern BOOLEAN ClasspScreenOff;
extern KGUARDED_MUTEX IdlePowerFDOListMutex;
extern ULONG DiskIdleTimeoutInMS;

//
// Definitions from ntos\rtl\time.c
//

extern CONST LARGE_INTEGER Magic10000;
#define SHIFT10000               13

//
// Constant to help wih various time conversions
//
#define CONST_MSECS_PER_SEC                 1000

#define Convert100nsToMilliseconds(LARGE_INTEGER)                       \
    (                                                                   \
    RtlExtendedMagicDivide((LARGE_INTEGER), Magic10000, SHIFT10000)     \
    )

#define ConvertMillisecondsTo100ns(MILLISECONDS) (                      \
    RtlExtendedIntegerMultiply ((MILLISECONDS), 10000)                  \
    )

typedef struct _MEDIA_CHANGE_DETECTION_INFO {

    //
    // Mutex to synchronize enable/disable requests and media state changes
    //

    KMUTEX MediaChangeMutex;

    //
    // The current state of the media (present, not present, unknown)
    // protected by MediaChangeSynchronizationEvent
    //

    MEDIA_CHANGE_DETECTION_STATE MediaChangeDetectionState;

    //
    // This is a count of how many time MCD has been disabled.  if it is
    // set to zero, then we'll poll the device for MCN events with the
    // then-current method (ie. TEST UNIT READY or GESN).  this is
    // protected by MediaChangeMutex
    //

    LONG MediaChangeDetectionDisableCount;


    //
    // The timer value to support media change events.  This is a countdown
    // value used to determine when to poll the device for a media change.
    // The max value for the timer is 255 seconds.  This is not protected
    // by an event -- simply InterlockedExchanged() as needed.
    //

    LONG MediaChangeCountDown;

    //
    // recent changes allowed instant retries of the MCN irp.  Since this
    // could cause an infinite loop, keep a count of how many times we've
    // retried immediately so that we can catch if the count exceeds an
    // arbitrary limit.
    //

    LONG MediaChangeRetryCount;

    //
    // use GESN if it's available
    //

    struct {
        BOOLEAN Supported;
        BOOLEAN HackEventMask;
        UCHAR   EventMask;
        UCHAR   NoChangeEventMask;
        PUCHAR  Buffer;
        PMDL    Mdl;
        ULONG   BufferSize;
    } Gesn;

    //
    // If this value is one, then the irp is currently in use.
    // If this value is zero, then the irp is available.
    // Use InterlockedCompareExchange() to set from "available" to "in use".
    // ASSERT that InterlockedCompareExchange() showed previous value of
    //    "in use" when changing back to "available" state.
    // This also implicitly protects the MediaChangeSrb and SenseBuffer
    //

    LONG MediaChangeIrpInUse;

    //
    // Pointer to the irp to be used for media change detection.
    // protected by Interlocked MediaChangeIrpInUse
    //

    PIRP MediaChangeIrp;

    //
    // The srb for the media change detection.
    // protected by Interlocked MediaChangeIrpInUse
    //

#if (NTDDI_VERSION >= NTDDI_WIN8)
    union {
        SCSI_REQUEST_BLOCK Srb;
        STORAGE_REQUEST_BLOCK SrbEx;
        UCHAR SrbExBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE];
    } MediaChangeSrb;
#else
    SCSI_REQUEST_BLOCK MediaChangeSrb;
#endif
    PUCHAR SenseBuffer;
    ULONG SrbFlags;

    //
    // Second timer to keep track of how long the media change IRP has been
    // in use.  If this value exceeds the timeout (#defined) then we should
    // print out a message to the user and set the MediaChangeIrpLost flag
    // protected by using Interlocked() operations in ClasspSendMediaStateIrp,
    // the only routine which should modify this value.
    //

    LONG MediaChangeIrpTimeInUse;

    //
    // Set by CdRomTickHandler when we determine that the media change irp has
    // been lost
    //

    BOOLEAN MediaChangeIrpLost;

    //
    // Buffer size of SenseBuffer
    //
    UCHAR SenseBufferLength;

} MEDIA_CHANGE_DETECTION_INFO, *PMEDIA_CHANGE_DETECTION_INFO;

typedef enum {
    SimpleMediaLock,
    SecureMediaLock,
    InternalMediaLock
} MEDIA_LOCK_TYPE, *PMEDIA_LOCK_TYPE;

typedef struct _FAILURE_PREDICTION_INFO {
    FAILURE_PREDICTION_METHOD Method;
    ULONG CountDown;                // Countdown timer
    ULONG Period;                   // Countdown period

    PIO_WORKITEM WorkQueueItem;

    KEVENT Event;

    //
    // Timestamp of last time the failure prediction info was queried.
    //
    LARGE_INTEGER LastFailurePredictionQueryTime;

} FAILURE_PREDICTION_INFO, *PFAILURE_PREDICTION_INFO;



//
// This struct must always fit within four PVOIDs of info,
// as it uses the irp's "PVOID DriverContext[4]" to store
// this info
//
typedef struct _CLASS_RETRY_INFO {
    struct _CLASS_RETRY_INFO *Next;
} CLASS_RETRY_INFO, *PCLASS_RETRY_INFO;

typedef struct _CSCAN_LIST {

    //
    // The current block which has an outstanding request.
    //

    ULONGLONG BlockNumber;

    //
    // The list of blocks past the CurrentBlock to which we're going to do
    // i/o.  This list is maintained in sorted order.
    //

    LIST_ENTRY CurrentSweep;

    //
    // The list of blocks behind the current block for which we'll have to
    // wait until the next scan across the disk.  This is kept as a stack,
    // the cost of sorting it is taken when it's moved over to be the
    // running list.
    //

    LIST_ENTRY NextSweep;

} CSCAN_LIST, *PCSCAN_LIST;

//
// add to the front of this structure to help prevent illegal
// snooping by other utilities.
//



typedef enum _CLASS_DETECTION_STATE {
    ClassDetectionUnknown = 0,
    ClassDetectionUnsupported = 1,
    ClassDetectionSupported = 2
} CLASS_DETECTION_STATE, *PCLASS_DETECTION_STATE;

#if _MSC_VER >= 1600
#pragma warning(push)
#pragma warning(disable:4214) // bit field types other than int
#endif
//
// CLASS_ERROR_LOG_DATA will still use SCSI_REQUEST_BLOCK even
// when using extended SRB as an extended SRB is too large to
// fit into. Should revisit this code once classpnp starts to
// use greater than 16 byte CDB.
//
typedef struct _CLASS_ERROR_LOG_DATA {
    LARGE_INTEGER TickCount;        // Offset 0x00
    ULONG PortNumber;               // Offset 0x08

    UCHAR ErrorPaging    : 1;       // Offset 0x0c
    UCHAR ErrorRetried   : 1;
    UCHAR ErrorUnhandled : 1;
    UCHAR ErrorReserved  : 5;

    UCHAR Reserved[3];

    SCSI_REQUEST_BLOCK Srb;     // Offset 0x10

    /*
     *  We define the SenseData as the default length.
     *  Since the sense data returned by the port driver may be longer,
     *  SenseData must be at the end of this structure.
     *  For our internal error log, we only log the default length.
     */
    SENSE_DATA SenseData;     // Offset 0x50 for x86 (or 0x68 for ia64) (ULONG32 Alignment required!)

} CLASS_ERROR_LOG_DATA, *PCLASS_ERROR_LOG_DATA;
#if _MSC_VER >= 1600
#pragma warning(pop)
#endif

#define NUM_ERROR_LOG_ENTRIES   16
#define DBG_NUM_PACKET_LOG_ENTRIES (64*2)   // 64 send&receive's

#if (NTDDI_VERSION >= NTDDI_WIN8)
typedef
VOID
(*PCONTINUATION_ROUTINE)(
    _In_ PVOID Context
    );
#endif

typedef struct _TRANSFER_PACKET {

        LIST_ENTRY AllPktsListEntry;    // entry in fdoData's static AllTransferPacketsList
        SLIST_ENTRY SlistEntry;         // for when in free list (use fast slist)

        PIRP Irp;
        PDEVICE_OBJECT Fdo;

        /*
         *  This is the client IRP that this TRANSFER_PACKET is currently
         *  servicing.
         */
        PIRP OriginalIrp;
        BOOLEAN CompleteOriginalIrpWhenLastPacketCompletes;

        /*
         *  Stuff for retrying the transfer.
         */
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
        UCHAR NumRetries; // Total number of retries remaining.
        UCHAR NumThinProvisioningRetries; //Number of retries carried out so far for a request failed with THIN_PROVISIONING_SOFT_THRESHOLD_ERROR
        UCHAR NumIoTimeoutRetries; // Number of retries remaining for a timed-out request.
        UCHAR TimedOut; // Indicates if this packet has timed-out.
#else
        ULONG NumRetries;
#endif
        KTIMER RetryTimer;
        KDPC RetryTimerDPC;

        _Field_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
        LONGLONG RetryIn100nsUnits;

        /*
         *  Event for synchronizing the transfer (optional).
         *  (Note that we can't have the event in the packet itself because
         *  by the time a thread waits on an event the packet may have
         *  been completed and re-issued.
         */
        PKEVENT SyncEventPtr;

        /*
         *  Stuff for retrying during extreme low-memory stress
         *  (when we retry 1 page at a time).
         *  NOTE: These fields are also used for StartIO-based
         *  class drivers, even when not in low memory conditions.
         */
        BOOLEAN DriverUsesStartIO; // if this is set, then the below low-mem flags are always used
        BOOLEAN InLowMemRetry;
        PUCHAR LowMemRetry_remainingBufPtr;
        ULONG LowMemRetry_remainingBufLen;
        LARGE_INTEGER LowMemRetry_nextChunkTargetLocation;

        /*
         *  Fields used for cancelling the packet.
         */
        // BOOLEAN Cancelled;
        // KEVENT CancelledEvent;

        /*
         *  We keep the buffer and length values here as well
         *  as in the SRB because some miniports return
         *  the transferred length in SRB.DataTransferLength,
         *  and if the SRB failed we need that value again for the retry.
         *  We don't trust the lower stack to preserve any of these values in the SRB.
         */
        PUCHAR BufPtrCopy;
        ULONG BufLenCopy;
        LARGE_INTEGER TargetLocationCopy;

        /*
         *  This is a standard SCSI structure that receives a detailed
         *  report about a SCSI error on the hardware.
         */
        SENSE_DATA_EX SrbErrorSenseData;

        /*
         *  This is the SRB block for this TRANSFER_PACKET.
         *  For IOCTLs, the SRB block includes two DWORDs for
         *  device object and ioctl code; so these must
         *  immediately follow the SRB block.
         */

#if (NTDDI_VERSION >= NTDDI_WIN8)
        PSTORAGE_REQUEST_BLOCK_HEADER Srb;
#else
        SCSI_REQUEST_BLOCK Srb;
#endif
        // ULONG SrbIoctlDevObj;        // not handling ioctls yet
        // ULONG SrbIoctlCode;

        #if DBG
            LARGE_INTEGER DbgTimeSent;
            LARGE_INTEGER DbgTimeReturned;
            ULONG DbgPktId;
            IRP DbgOriginalIrpCopy;
            MDL DbgMdlCopy;
        #endif

        BOOLEAN UsePartialMdl;
        PMDL PartialMdl;

        PSRB_HISTORY RetryHistory;

        // The time at which this request was sent to port driver.
        ULONGLONG RequestStartTime;

#if (NTDDI_VERSION >= NTDDI_WIN8)
        // ActivityId that is associated with the IRP that this transfer packet services.
        GUID ActivityId;

        // If non-NULL, called at packet completion with this context.
        PCONTINUATION_ROUTINE ContinuationRoutine;
        PVOID ContinuationContext;
        ULONGLONG TransferCount;
        ULONG AllocateNode;
#endif
} TRANSFER_PACKET, *PTRANSFER_PACKET;

/*
 *  MIN_INITIAL_TRANSFER_PACKETS is the minimum number of packets that
 *  we preallocate at startup for each device (we need at least one packet
 *  to guarantee forward progress during memory stress).
 *  MIN_WORKINGSET_TRANSFER_PACKETS is the number of TRANSFER_PACKETs
 *  we allow to build up and remain for each device;
 *  we _lazily_ work down to this number when they're not needed.
 *  MAX_WORKINGSET_TRANSFER_PACKETS is the number of TRANSFER_PACKETs
 *  that we _immediately_ reduce to when they are not needed.
 *
 *  The absolute maximum number of packets that we will allocate is
 *  whatever is required by the current activity, up to the memory limit;
 *  as soon as stress ends, we snap down to MAX_WORKINGSET_TRANSFER_PACKETS;
 *  we then lazily work down to MIN_WORKINGSET_TRANSFER_PACKETS.
 */
#define MIN_INITIAL_TRANSFER_PACKETS                        1
#define MIN_WORKINGSET_TRANSFER_PACKETS_Client              16
#define MAX_WORKINGSET_TRANSFER_PACKETS_Client              32
#define MIN_WORKINGSET_TRANSFER_PACKETS_Server_UpperBound   256
#define MIN_WORKINGSET_TRANSFER_PACKETS_Server_LowerBound   32
#define MAX_WORKINGSET_TRANSFER_PACKETS_Server              1024
#define MIN_WORKINGSET_TRANSFER_PACKETS_SPACES              512
#define MAX_WORKINGSET_TRANSFER_PACKETS_SPACES              2048
#define MAX_OUTSTANDING_IO_PER_LUN_DEFAULT                  16
#define MAX_CLEANUP_TRANSFER_PACKETS_AT_ONCE                8192



typedef struct _PNL_SLIST_HEADER {
    DECLSPEC_CACHEALIGN SLIST_HEADER SListHeader;
    DECLSPEC_CACHEALIGN ULONG NumFreeTransferPackets;
    ULONG NumTotalTransferPackets;
    ULONG DbgPeakNumTransferPackets;
} PNL_SLIST_HEADER, *PPNL_SLIST_HEADER;

//
// !!! WARNING !!!
// DO NOT use the following structure in code outside of classpnp
// as structure will not be guaranteed between OS versions.
//
// add to the front of this structure to help prevent illegal
// snooping by other utilities.
//
struct _CLASS_PRIVATE_FDO_DATA {

    //
    // The amount of time allowed for a target to complete a copy offload
    // operation, in seconds.  Default is 4s, but it can be modified via
    // registry key.
    //
    ULONG CopyOffloadMaxTargetDuration;


#if (NTDDI_VERSION >= NTDDI_WIN8)

    //
    // Periodic timer for polling for media change detection, failure prediction
    // and class tick function.
    //

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    PEX_TIMER TickTimer;
    LONGLONG CurrentNoWakeTolerance;
#else
    KTIMER TickTimer;
    KDPC TickTimerDpc;
#endif // (NTDDI_VERSION >= NTDDI_WINBLUE)

    //
    // Power related and release queue SRBs
    //
    union {
        STORAGE_REQUEST_BLOCK SrbEx;
        UCHAR PowerSrbBuffer[CLASS_SRBEX_SCSI_CDB16_BUFFER_SIZE];
    } PowerSrb;

    union {
        STORAGE_REQUEST_BLOCK SrbEx;
        UCHAR ReleaseQueueSrbBuffer[CLASS_SRBEX_NO_SRBEX_DATA_BUFFER_SIZE];
    } ReleaseQueueSrb;

#endif

    ULONG TrackingFlags;

    /*
     * Flag to detect recursion caused by devices
     * reporting different capacity per each request
     */
    ULONG UpdateDiskPropertiesWorkItemActive;

    //
    // Local equivalents of MinWorkingSetTransferPackets and MaxWorkingSetTransferPackets.
    // These values are initialized by the global equivalents but are then adjusted as
    // requested by the class driver.
    //
    ULONG LocalMinWorkingSetTransferPackets;
    ULONG LocalMaxWorkingSetTransferPackets;

#if DBG

    ULONG MaxOutstandingIOPerLUN;

#endif

    /*
     *  Entry in static list used by debug extension to quickly find all class FDOs.
     */
    LIST_ENTRY AllFdosListEntry;

    //
    // this private structure allows us to
    // dynamically re-enable the perf benefits
    // lost due to transient error conditions.
    // in w2k, a reboot was required. :(
    //
    struct {
        ULONG      OriginalSrbFlags;
        ULONG      SuccessfulIO;
        ULONG      ReEnableThreshhold; // 0 means never
    } Perf;

    ULONG_PTR HackFlags;

    STORAGE_HOTPLUG_INFO HotplugInfo;

    // Legacy.  Still used by obsolete legacy code.
    struct {
        LARGE_INTEGER     Delta;       // in ticks
        LARGE_INTEGER     Tick;        // when it should fire
        PCLASS_RETRY_INFO ListHead;    // singly-linked list
        ULONG             Granularity; // static
        KSPIN_LOCK        Lock;        // protective spin lock
        KDPC              Dpc;         // DPC routine object
        KTIMER            Timer;       // timer to fire DPC
    } Retry;

    BOOLEAN TimerInitialized;
    BOOLEAN LoggedTURFailureSinceLastIO;
    BOOLEAN LoggedSYNCFailure;

    //
    // privately allocated release queue irp
    // protected by fdoExtension->ReleaseQueueSpinLock
    //
    BOOLEAN ReleaseQueueIrpAllocated;
    PIRP ReleaseQueueIrp;

    /*
     *  Queues for TRANSFER_PACKETs that contextualize the IRPs and SRBs
     *  that we send down to the port driver.
     *  (The free list is an slist so that we can use fast
     *   interlocked operations on it; but the relatively-static
     *   AllTransferPacketsList list has to be
     *   a doubly-linked list since we have to dequeue from the middle).
     */
    LIST_ENTRY AllTransferPacketsList;
    PPNL_SLIST_HEADER FreeTransferPacketsLists;

    /*
     *  Queue for deferred client irps
     */
    LIST_ENTRY DeferredClientIrpList;

    /*
     *  Precomputed maximum transfer length for the hardware.
     */
    ULONG HwMaxXferLen;

    /*
     *  SCSI_REQUEST_BLOCK template preconfigured with the constant values.
     *  This is slapped into the SRB in the TRANSFER_PACKET for each transfer.
     */

#if (NTDDI_VERSION >= NTDDI_WIN8)
    PSTORAGE_REQUEST_BLOCK_HEADER SrbTemplate;
#else
    SCSI_REQUEST_BLOCK SrbTemplate;
#endif

    KSPIN_LOCK SpinLock;

    /*
     *  For non-removable media, we read the drive capacity at start time and cache it.
     *  This is so that ReadDriveCapacity failures at runtime (e.g. due to memory stress)
     *  don't cause I/O on the paging disk to start failing.
     */
    READ_CAPACITY_DATA_EX LastKnownDriveCapacityData;
    BOOLEAN IsCachedDriveCapDataValid;

    //
    // Idle priority support flag
    //
    BOOLEAN IdlePrioritySupported;

    //
    // Tick timer enabled
    //
    BOOLEAN TickTimerEnabled;

    BOOLEAN ReservedBoolean;

    /*
     *  Circular array of timestamped logs of errors that occurred on this device.
     */
    ULONG ErrorLogNextIndex;
    CLASS_ERROR_LOG_DATA ErrorLogs[NUM_ERROR_LOG_ENTRIES];

    //
    // Number of outstanding critical Io requests from Mm
    //
    ULONG NumHighPriorityPagingIo;

    //
    // Maximum number of normal Io requests that can be interleaved with the critical ones
    //
    ULONG MaxInterleavedNormalIo;

    //
    // The timestamp when entering throttle mode
    //
    LARGE_INTEGER ThrottleStartTime;

    //
    // The timestamp when exiting throttle mode
    //
    LARGE_INTEGER ThrottleStopTime;

    //
    // The longest time ever spent in throttle mode
    //
    LARGE_INTEGER LongestThrottlePeriod;

    #if DBG
        ULONG DbgMaxPktId;

        /*
         *  Logging fields for ForceUnitAccess and Flush
         */
        BOOLEAN DbgInitFlushLogging;         // must reset this to 1 for each logging session
        ULONG DbgNumIORequests;
        ULONG DbgNumFUAs;       // num I/O requests with ForceUnitAccess bit set
        ULONG DbgNumFlushes;    // num SRB_FUNCTION_FLUSH_QUEUE
        ULONG DbgIOsSinceFUA;
        ULONG DbgIOsSinceFlush;
        ULONG DbgAveIOsToFUA;      // average number of I/O requests between FUAs
        ULONG DbgAveIOsToFlush;   // ...
        ULONG DbgMaxIOsToFUA;
        ULONG DbgMaxIOsToFlush;
        ULONG DbgMinIOsToFUA;
        ULONG DbgMinIOsToFlush;

        /*
         *  Debug log of previously sent packets (including retries).
         */
        ULONG DbgPacketLogNextIndex;
        TRANSFER_PACKET DbgPacketLogs[DBG_NUM_PACKET_LOG_ENTRIES];
    #endif

    //
    // Spin lock for low priority I/O list
    //
    KSPIN_LOCK IdleListLock;

    //
    // Queue for low priority I/O
    //
    LIST_ENTRY IdleIrpList;

    //
    // Timer for low priority I/O
    //
    KTIMER IdleTimer;

    //
    // DPC for low priority I/O
    //
    KDPC IdleDpc;

#if (NTDDI_VERSION >= NTDDI_WIN8)

    //
    // Time (ms) since the completion of the last non-idle request before the
    // first idle request should be issued. Due to the coarseness of the idle
    // timer frequency, some variability in the idle interval will be tolerated
    // such that it is the desired idle interval on average.
    //
    USHORT IdleInterval;

    //
    // Max number of active idle requests.
    //
    USHORT IdleActiveIoMax;

#endif

    //
    // Idle duration required to process idle request
    // to avoid starvation
    //
    USHORT StarvationDuration;

    //
    // Idle I/O count
    //
    ULONG IdleIoCount;

    //
    // Flag to indicate timer status
    //
    LONG IdleTimerStarted;

    //
    // Time when the Idle timer was started
    //
    LARGE_INTEGER AntiStarvationStartTime;

    //
    // Normal priority I/O time
    //
    LARGE_INTEGER LastNonIdleIoTime;

    //
    // Time when the last IO of any priority completed.
    //
    LARGE_INTEGER LastIoCompletionTime;

    //
    // Count of active normal priority I/O
    //
    LONG ActiveIoCount;

    //
    // Count of active idle priority I/O
    //
    LONG ActiveIdleIoCount;

    //
    // Support for class drivers to extend
    // the interpret sense information routine
    // and retry history per-packet.  Copy of
    // values in driver extension.
    //
    PCLASS_INTERPRET_SENSE_INFO2 InterpretSenseInfo;

    //
    // power process parameters. they work closely with CLASS_POWER_CONTEXT structure.
    //
    ULONG MaxPowerOperationRetryCount;
    PIRP  PowerProcessIrp;

    //
    // Indicates legacy error handling should be used.
    // This means:
    //  - Max number of retries for an IO request is 8 (instead of 4).
    //
    BOOLEAN LegacyErrorHandling;

    //
    // Maximum number of retries allowed for IO requests for this device.
    //
    UCHAR MaxNumberOfIoRetries;

    //
    // Disable All Throttling in case of Error
    //
    BOOLEAN DisableThrottling;

};

//
// !!! WARNING !!!
// DO NOT use the following structure in code outside of classpnp
// as structure will not be guaranteed between OS versions.
//
// EX_RUNDOWN_REF_CACHE_AWARE is variable size and follows
// RemoveLockFailAcquire. EX_RUNDOWN_REF_CACHE_AWARE must be part
// of the device extension allocation to avoid issues with a device
// that has been PNP remove but still has outstanding references.
// In this case, the removed object may still receive incoming requests.
//
// There are code dependencies on the structure layout. To minimize
// code changes, new fields to _CLASS_PRIVATE_COMMON_DATA should be
// added based on the following guidance.
// - Fixed size: beginning of _CLASS_PRIVATE_COMMON_DATA
// - Variable size: at the end of _CLASS_PRIVATE_COMMON_DATA after the
//   last variable size field.
//

struct _CLASS_PRIVATE_COMMON_DATA {

    //
    // Cacheaware rundown lock reference
    //

    LONG RemoveLockFailAcquire;

    //
    // N.B. EX_RUNDOWN_REF_CACHE_AWARE begins with a pointer-sized item that is
    //      accessed interlocked, and must be aligned on ARM platforms. In order
    //      for this to work on ARM64, an additional 32-bit slot must be allocated.
    //

#if defined(_WIN64)
    LONG Align;
#endif

    // EX_RUNDOWN_REF_CACHE_AWARE (variable size) follows

};

//
// Verify that the size of _CLASS_PRIVATE_COMMON_DATA is pointer size aligned
// to ensure the EX_RUNDOWN_REF_CACHE_AWARE following it is properly aligned.
//

C_ASSERT((sizeof(struct _CLASS_PRIVATE_COMMON_DATA) % sizeof(PVOID)) == 0);

typedef struct _IDLE_POWER_FDO_LIST_ENTRY {
    LIST_ENTRY ListEntry;
    PDEVICE_OBJECT Fdo;
} IDLE_POWER_FDO_LIST_ENTRY, *PIDLE_POWER_FDO_LIST_ENTRY;

typedef struct _OFFLOAD_READ_CONTEXT {

    PDEVICE_OBJECT Fdo;

    //
    // Upper offload read DSM irp.
    //

    PIRP OffloadReadDsmIrp;

    //
    // A pseudo-irp is used despite the operation being async.  This is in
    // contrast to normal read and write, which let TransferPktComplete()
    // complete the upper IRP directly.  Offload requests are enough different
    // that it makes more sense to let them manage their own async steps with
    // minimal help from TransferPktComplete() (just a continuation function
    // call during TransferPktComplete()).
    //

    IRP PseudoIrp;

    //
    // The offload read context tracks one packet in flight at a time - it'll be
    // the POPULATE TOKEN packet first, then RECEIVE ROD TOKEN INFORMATION.
    //
    // This field exists only for debug purposes.
    //

    PTRANSFER_PACKET Pkt;

    PMDL PopulateTokenMdl;

    ULONG BufferLength;

    ULONG ListIdentifier;

    ULONG ReceiveTokenInformationBufferLength;

    //
    // Total sectors that the operation is attempting to process.
    //

    ULONGLONG TotalSectorsToProcess;

    //
    // Total sectors actually processed.
    //

    ULONGLONG TotalSectorsProcessed;

    //
    // Total upper request size in bytes.
    //

    ULONGLONG EntireXferLen;

    //
    // Just a cached copy of what was in the transfer packet.
    //

    SCSI_REQUEST_BLOCK Srb;

    //
    // Pointer into the token part of the SCSI buffer (the buffer immediately
    // after this struct), for easy reference.
    //

    PUCHAR Token;

    // The SCSI buffer (in/out buffer, not CDB) for the commands immediately
    // follows this struct, so no need to have a field redundantly pointing to
    // the buffer.
} OFFLOAD_READ_CONTEXT, *POFFLOAD_READ_CONTEXT;


typedef struct _OFFLOAD_WRITE_CONTEXT {

    PDEVICE_OBJECT Fdo;

    PIRP OffloadWriteDsmIrp;

    ULONGLONG EntireXferLen;
    ULONGLONG TotalRequestSizeSectors;

    ULONG DataSetRangesCount;

    PDEVICE_MANAGE_DATA_SET_ATTRIBUTES DsmAttributes;
    PDEVICE_DATA_SET_RANGE DataSetRanges;
    PDEVICE_DSM_OFFLOAD_WRITE_PARAMETERS OffloadWriteParameters;
    ULONGLONG LogicalBlockOffset;

    ULONG MaxBlockDescrCount;
    ULONGLONG MaxLbaCount;

    ULONG BufferLength;
    ULONG ReceiveTokenInformationBufferLength;

    IRP PseudoIrp;

    PMDL WriteUsingTokenMdl;

    ULONGLONG TotalSectorsProcessedSuccessfully;
    ULONG DataSetRangeIndex;
    ULONGLONG DataSetRangeByteOffset;

    PTRANSFER_PACKET Pkt;

    //
    // Per-WUT (WRITE USING TOKEN), not overall.
    //

    ULONGLONG TotalSectorsToProcess;
    ULONGLONG TotalSectorsProcessed;

    ULONG ListIdentifier;

    BOOLEAN TokenInvalidated;

    //
    // Just a cached copy of what was in the transfer packet.
    //

    SCSI_REQUEST_BLOCK Srb;

    ULONGLONG OperationStartTime;

} OFFLOAD_WRITE_CONTEXT, *POFFLOAD_WRITE_CONTEXT;


typedef struct _OPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER {
    PIO_WORKITEM WorkItem;
    PVOID SenseData;
    ULONG SenseDataSize;
    UCHAR SrbStatus;
    UCHAR ScsiStatus;
    UCHAR OpCode;
    UCHAR Reserved;
    ULONG ErrorCode;
} OPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER, *POPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER;

typedef struct _IO_RETRIED_LOG_MESSAGE_CONTEXT {
    OPCODE_SENSE_DATA_IO_LOG_MESSAGE_CONTEXT_HEADER ContextHeader;
    LARGE_INTEGER Lba;
    ULONG DeviceNumber;
} IO_RETRIED_LOG_MESSAGE_CONTEXT, *PIO_RETRIED_LOG_MESSAGE_CONTEXT;


#define QERR_SET_ZERO_ODX_OR_TP_ONLY 0
#define QERR_SET_ZERO_ALWAYS         1
#define QERR_SET_ZERO_NEVER          2


#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))


#define NOT_READY_RETRY_INTERVAL    10
#define MINIMUM_RETRY_UNITS         ((LONGLONG)32)
#define MODE_PAGE_DATA_SIZE         192

#define CLASS_IDLE_INTERVAL_MIN     12          // 12 milliseconds
#define CLASS_IDLE_INTERVAL         12          // 12 milliseconds
#define CLASS_STARVATION_INTERVAL   500         // 500 milliseconds

//
// Value of 50 milliseconds in 100 nanoseconds units
//
#define FIFTY_MS_IN_100NS_UNITS     50 * 100


/*
 *  Simple singly-linked-list queuing macros, with no synchronization.
 */
__inline VOID SimpleInitSlistHdr(SINGLE_LIST_ENTRY *SListHdr)
{
    SListHdr->Next = NULL;
}
__inline VOID SimplePushSlist(SINGLE_LIST_ENTRY *SListHdr, SINGLE_LIST_ENTRY *SListEntry)
{
    SListEntry->Next = SListHdr->Next;
    SListHdr->Next = SListEntry;
}
__inline SINGLE_LIST_ENTRY *SimplePopSlist(SINGLE_LIST_ENTRY *SListHdr)
{
    SINGLE_LIST_ENTRY *sListEntry = SListHdr->Next;
    if (sListEntry){
        SListHdr->Next = sListEntry->Next;
        sListEntry->Next = NULL;
    }
    return sListEntry;
}
__inline BOOLEAN SimpleIsSlistEmpty(SINGLE_LIST_ENTRY *SListHdr)
{
    return (SListHdr->Next == NULL);
}

__inline
BOOLEAN
ClasspIsIdleRequestSupported(
    PCLASS_PRIVATE_FDO_DATA FdoData,
    PIRP Irp
    )
{
#ifndef __REACTOS__
    IO_PRIORITY_HINT ioPriority = IoGetIoPriorityHint(Irp);
    return ((ioPriority <= IoPriorityLow) && (FdoData->IdlePrioritySupported == TRUE));
#else
    return (FdoData->IdlePrioritySupported == TRUE);
#endif
}

__inline
VOID
ClasspMarkIrpAsIdle(
    PIRP Irp,
    BOOLEAN Idle
    )
{
#ifndef __REACTOS__
// truncation is not an issue for this use case
// nonstandard extension used is not an issue for this use case
#pragma warning(suppress:4305; suppress:4213)
    ((BOOLEAN)Irp->Tail.Overlay.DriverContext[1]) = Idle;
#else
    ((PULONG_PTR)Irp->Tail.Overlay.DriverContext)[1] = Idle;
#endif
}

__inline
BOOLEAN
ClasspIsIdleRequest(
    PIRP Irp
    )
{
#ifdef _MSC_VER
#pragma warning(suppress:4305) // truncation is not an issue for this use case
#endif
    return ((BOOLEAN)Irp->Tail.Overlay.DriverContext[1]);
}

__inline
LARGE_INTEGER
ClasspGetCurrentTime(
    VOID
    )
{
    LARGE_INTEGER currentTime;

#ifndef __REACTOS__
    currentTime.QuadPart = KeQueryUnbiasedInterruptTimePrecise((ULONG64*)&currentTime.QuadPart);
#else
    currentTime = KeQueryPerformanceCounter(NULL);
#endif

    return currentTime;
}

__inline
ULONGLONG
ClasspTimeDiffToMs(
    ULONGLONG TimeDiff
    )
{
    TimeDiff /= (10 * 1000);

    return TimeDiff;
}

__inline
BOOLEAN
ClasspSupportsUnmap(
    _In_ PCLASS_FUNCTION_SUPPORT_INFO SupportInfo
    )
{
    return SupportInfo->LBProvisioningData.LBPU;
}

__inline
BOOLEAN
ClasspIsThinProvisioned(
    _In_ PCLASS_FUNCTION_SUPPORT_INFO SupportInfo
    )
{
    //
    // We only support thinly provisioned devices that also support UNMAP.
    //
    if (SupportInfo->LBProvisioningData.ProvisioningType == PROVISIONING_TYPE_THIN &&
        SupportInfo->LBProvisioningData.LBPU == TRUE)
    {
        return TRUE;
    }

    return FALSE;
}

__inline
BOOLEAN
ClasspIsObsoletePortDriver(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    )
{
    if ( (FdoExtension->MiniportDescriptor != NULL) &&
         (FdoExtension->MiniportDescriptor->Portdriver == StoragePortCodeSetSCSIport) ) {
        return TRUE;
    }

    return FALSE;
}


ULONG
ClasspCalculateLogicalSectorSize (
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG          BytesPerBlockInBigEndian
    );

DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD ClassUnload;

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
DRIVER_DISPATCH ClassCreateClose;

NTSTATUS
ClasspCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
ClasspCleanupProtectedLocks(
    IN PFILE_OBJECT_EXTENSION FsContext
    );

NTSTATUS
ClasspEjectionControl(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp,
    IN MEDIA_LOCK_TYPE LockType,
    IN BOOLEAN Lock
    );

_Dispatch_type_(IRP_MJ_READ)
_Dispatch_type_(IRP_MJ_WRITE)
DRIVER_DISPATCH ClassReadWrite;

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH ClassDeviceControlDispatch;

_Dispatch_type_(IRP_MJ_PNP)
DRIVER_DISPATCH ClassDispatchPnp;

NTSTATUS
ClassPnpStartDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

_Dispatch_type_(IRP_MJ_SHUTDOWN)
_Dispatch_type_(IRP_MJ_FLUSH_BUFFERS)
DRIVER_DISPATCH ClassShutdownFlush;

_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
DRIVER_DISPATCH ClassSystemControl;


//
// Class internal routines
//

DRIVER_ADD_DEVICE ClassAddDevice;

IO_COMPLETION_ROUTINE ClasspSendSynchronousCompletion;

VOID
RetryRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PSCSI_REQUEST_BLOCK Srb,
    BOOLEAN Associated,
    LONGLONG TimeDelta100ns
    );

NTSTATUS
ClassIoCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
ClassPnpQueryFdoRelations(
    IN PDEVICE_OBJECT Fdo,
    IN PIRP Irp
    );

NTSTATUS
ClassRetrieveDeviceRelations(
    IN PDEVICE_OBJECT Fdo,
    IN DEVICE_RELATION_TYPE RelationType,
    OUT PDEVICE_RELATIONS *DeviceRelations
    );

NTSTATUS
ClassGetPdoId(
    IN PDEVICE_OBJECT Pdo,
    IN BUS_QUERY_ID_TYPE IdType,
    IN PUNICODE_STRING IdString
    );

NTSTATUS
ClassQueryPnpCapabilities(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_CAPABILITIES Capabilities
    );

DRIVER_STARTIO ClasspStartIo;

NTSTATUS
ClasspPagingNotificationCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PDEVICE_OBJECT RealDeviceObject
    );

NTSTATUS
ClasspMediaChangeCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

NTSTATUS
ClasspMcnControl(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
ClasspRegisterMountedDeviceInterface(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
ClasspDisableTimer(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

VOID
ClasspEnableTimer(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspInitializeTimer(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

VOID
ClasspDeleteTimer(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

#if (NTDDI_VERSION >= NTDDI_WINBLUE)
BOOLEAN
ClasspUpdateTimerNoWakeTolerance(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);
#endif

NTSTATUS
ClasspDuidQueryProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
_Dispatch_type_(IRP_MJ_READ)
_Dispatch_type_(IRP_MJ_WRITE)
_Dispatch_type_(IRP_MJ_SCSI)
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Dispatch_type_(IRP_MJ_SHUTDOWN)
_Dispatch_type_(IRP_MJ_FLUSH_BUFFERS)
_Dispatch_type_(IRP_MJ_PNP)
_Dispatch_type_(IRP_MJ_POWER)
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
DRIVER_DISPATCH ClassGlobalDispatch;

VOID
ClassInitializeDispatchTables(
    PCLASS_DRIVER_EXTENSION DriverExtension
    );

NTSTATUS
ClasspPersistentReserve(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

//
// routines for dictionary list support
//

VOID
InitializeDictionary(
    IN PDICTIONARY Dictionary
    );

BOOLEAN
TestDictionarySignature(
    IN PDICTIONARY Dictionary
    );

NTSTATUS
AllocateDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key,
    IN ULONG Size,
    IN ULONG Tag,
    OUT PVOID *Entry
    );

PVOID
GetDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN ULONGLONG Key
    );

VOID
FreeDictionaryEntry(
    IN PDICTIONARY Dictionary,
    IN PVOID Entry
    );


NTSTATUS
ClasspAllocateReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    );

VOID
ClasspFreeReleaseRequest(
    IN PDEVICE_OBJECT Fdo
    );

IO_COMPLETION_ROUTINE ClassReleaseQueueCompletion;

VOID
ClasspReleaseQueue(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP ReleaseQueueIrp
    );

VOID
ClasspDisablePowerNotification(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
);

//
// class power routines
//

_Dispatch_type_(IRP_MJ_POWER)
DRIVER_DISPATCH ClassDispatchPower;

NTSTATUS
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClassMinimalPowerHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

_IRQL_requires_same_
NTSTATUS
ClasspEnableIdlePower(
    _In_ PDEVICE_OBJECT DeviceObject
    );

POWER_SETTING_CALLBACK ClasspPowerSettingCallback;

//
// Child list routines
//

VOID
ClassAddChild(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION Parent,
    _In_ PPHYSICAL_DEVICE_EXTENSION Child,
    _In_ BOOLEAN AcquireLock
    );

PPHYSICAL_DEVICE_EXTENSION
ClassRemoveChild(
    IN PFUNCTIONAL_DEVICE_EXTENSION Parent,
    IN PPHYSICAL_DEVICE_EXTENSION Child,
    IN BOOLEAN AcquireLock
    );

VOID
ClasspRetryDpcTimer(
    IN PCLASS_PRIVATE_FDO_DATA FdoData
    );

KDEFERRED_ROUTINE ClasspRetryRequestDpc;

VOID
ClassFreeOrReuseSrb(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN __drv_freesMem(mem) PSCSI_REQUEST_BLOCK Srb
    );

VOID
ClassRetryRequest(
    IN PDEVICE_OBJECT SelfDeviceObject,
    IN PIRP           Irp,
    _In_ _In_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS) // this is 100 seconds; already an assert in classpnp based on this
    IN LONGLONG       TimeDelta100ns // in 100ns units
    );

VOID
ClasspBuildRequestEx(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PIRP Irp,
    _In_ __drv_aliasesMem PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
ClasspAllocateReleaseQueueIrp(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspAllocatePowerProcessIrp(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspInitializeGesn(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PMEDIA_CHANGE_DETECTION_INFO Info
    );

VOID
ClassSendEjectionNotification(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

VOID
ClasspScanForSpecialInRegistry(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
ClasspScanForClassHacks(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN ULONG_PTR Data
    );

NTSTATUS
ClasspInitializeHotplugInfo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

VOID
ClasspPerfIncrementErrorCount(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );
VOID
ClasspPerfIncrementSuccessfulIo(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

IO_WORKITEM_ROUTINE ClasspUpdateDiskProperties;

__drv_allocatesMem(Mem)
PTRANSFER_PACKET NewTransferPacket(PDEVICE_OBJECT Fdo);
VOID DestroyTransferPacket(_In_ __drv_freesMem(mem) PTRANSFER_PACKET Pkt);
VOID EnqueueFreeTransferPacket(PDEVICE_OBJECT Fdo, __drv_aliasesMem PTRANSFER_PACKET Pkt);
PTRANSFER_PACKET DequeueFreeTransferPacket(PDEVICE_OBJECT Fdo, BOOLEAN AllocIfNeeded);
PTRANSFER_PACKET DequeueFreeTransferPacketEx(_In_ PDEVICE_OBJECT Fdo, _In_ BOOLEAN AllocIfNeeded, _In_ ULONG Node);
VOID SetupReadWriteTransferPacket(PTRANSFER_PACKET pkt, PVOID Buf, ULONG Len, LARGE_INTEGER DiskLocation, PIRP OriginalIrp);
NTSTATUS SubmitTransferPacket(PTRANSFER_PACKET Pkt);
IO_COMPLETION_ROUTINE TransferPktComplete;
NTSTATUS ServiceTransferRequest(PDEVICE_OBJECT Fdo, PIRP Irp, BOOLEAN PostToDpc);
VOID TransferPacketQueueRetryDpc(PTRANSFER_PACKET Pkt);
KDEFERRED_ROUTINE TransferPacketRetryTimerDpc;
BOOLEAN InterpretTransferPacketError(PTRANSFER_PACKET Pkt);
BOOLEAN RetryTransferPacket(PTRANSFER_PACKET Pkt);
VOID EnqueueDeferredClientIrp(PDEVICE_OBJECT Fdo, PIRP Irp);
PIRP DequeueDeferredClientIrp(PDEVICE_OBJECT Fdo);
VOID InitLowMemRetry(PTRANSFER_PACKET Pkt, PVOID BufPtr, ULONG Len, LARGE_INTEGER TargetLocation);
BOOLEAN StepLowMemRetry(PTRANSFER_PACKET Pkt);
VOID SetupEjectionTransferPacket(TRANSFER_PACKET *Pkt, BOOLEAN PreventMediaRemoval, PKEVENT SyncEventPtr, PIRP OriginalIrp);
VOID SetupModeSenseTransferPacket(TRANSFER_PACKET *Pkt, PKEVENT SyncEventPtr, PVOID ModeSenseBuffer, UCHAR ModeSenseBufferLen, UCHAR PageMode, UCHAR SubPage, PIRP OriginalIrp, UCHAR PageControl);
VOID SetupModeSelectTransferPacket(TRANSFER_PACKET *Pkt, PKEVENT SyncEventPtr, PVOID ModeSelectBuffer, UCHAR ModeSelectBufferLen, BOOLEAN SavePages, PIRP OriginalIrp);
VOID SetupDriveCapacityTransferPacket(TRANSFER_PACKET *Pkt, PVOID ReadCapacityBuffer, ULONG ReadCapacityBufferLen, PKEVENT SyncEventPtr, PIRP OriginalIrp, BOOLEAN Use16ByteCdb);
PMDL BuildDeviceInputMdl(PVOID Buffer, ULONG BufferLen);
PMDL ClasspBuildDeviceMdl(PVOID Buffer, ULONG BufferLen, BOOLEAN WriteToDevice);
VOID FreeDeviceInputMdl(PMDL Mdl);
VOID ClasspFreeDeviceMdl(PMDL Mdl);
NTSTATUS InitializeTransferPackets(PDEVICE_OBJECT Fdo);
VOID DestroyAllTransferPackets(PDEVICE_OBJECT Fdo);
VOID InterpretCapacityData(PDEVICE_OBJECT Fdo, PREAD_CAPACITY_DATA_EX ReadCapacityData);
IO_WORKITEM_ROUTINE_EX CleanupTransferPacketToWorkingSetSizeWorker;
VOID CleanupTransferPacketToWorkingSetSize(_In_ PDEVICE_OBJECT Fdo, _In_ BOOLEAN LimitNumPktToDelete, _In_ ULONG Node);

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupPopulateTokenTransferPacket(
    _In_ __drv_aliasesMem POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR PopulateTokenBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupReceivePopulateTokenInformationTransferPacket(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR ReceivePopulateTokenInformationBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupWriteUsingTokenTransferPacket(
    _In_ __drv_aliasesMem POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR WriteUsingTokenBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspSetupReceiveWriteUsingTokenInformationTransferPacket(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ PTRANSFER_PACKET Pkt,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PUCHAR ReceiveWriteUsingTokenInformationBuffer,
    _In_ PIRP OriginalIrp,
    _In_ ULONG ListIdentifier
    );

ULONG ClasspModeSense(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSenseBuffer,
    _In_ ULONG Length,
    _In_ UCHAR PageMode,
    _In_ UCHAR PageControl
    );

NTSTATUS
ClasspModeSelect(
    _In_ PDEVICE_OBJECT Fdo,
    _In_reads_bytes_(Length) PCHAR ModeSelectBuffer,
    _In_ ULONG Length,
    _In_ BOOLEAN SavePages
    );

NTSTATUS ClasspWriteCacheProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspAccessAlignmentProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceSeekPenaltyProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceGetLBProvisioningVPDPage(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_opt_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceGetBlockDeviceCharacteristicsVPDPage(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION fdoExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceGetBlockLimitsVPDPage(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _Inout_bytecount_(SrbSize) PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG SrbSize,
    _Out_ PCLASS_VPD_B0_DATA BlockLimitsData
    );

NTSTATUS ClasspDeviceTrimProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceLBProvisioningProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceTrimProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PGUID ActivityId,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceGetLBAStatus(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS ClasspDeviceGetLBAStatusWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PCLASS_VPD_B0_DATA BlockLimitsData,
    _In_ ULONGLONG StartingOffset,
    _In_ ULONGLONG LengthInBytes,
    _Out_ PDEVICE_MANAGE_DATA_SET_ATTRIBUTES_OUTPUT DsmOutput,
    _Inout_ PULONG DsmOutputLength,
    _Inout_ PSCSI_REQUEST_BLOCK Srb,
    _In_ BOOLEAN ConsolidateableBlocksOnly,
    _In_ ULONG OutputVersion,
    _Out_ PBOOLEAN BlockLimitsDataMayHaveChanged
    );

VOID ClassQueueThresholdEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID ClassQueueResourceExhaustionEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID ClassQueueCapacityChangedEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID ClassQueueProvisioningTypeChangedEventWorker(
    _In_ PDEVICE_OBJECT DeviceObject
    );

IO_WORKITEM_ROUTINE ClasspLogIOEventWithContext;

VOID
ClasspQueueLogIOEventWithContextWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG SenseBufferSize,
    _In_ PVOID SenseData,
    _In_ UCHAR SrbStatus,
    _In_ UCHAR ScsiStatus,
    _In_ ULONG ErrorCode,
    _In_ ULONG CdbLength,
    _In_opt_ PCDB Cdb,
    _In_opt_ PTRANSFER_PACKET Pkt
    );

VOID
ClasspZeroQERR(
    _In_ PDEVICE_OBJECT DeviceObject
    );


_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspGetMaximumTokenListIdentifier(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_z_ PWSTR RegistryPath,
    _Out_ PULONG MaximumListIdentifier
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspGetCopyOffloadMaxDuration(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_z_ PWSTR RegistryPath,
    _Out_ PULONG MaxDuration
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspDeviceCopyOffloadProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspValidateOffloadSupported(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspValidateOffloadInputParameters(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );

_IRQL_requires_same_
NTSTATUS
ClasspGetTokenOperationCommandBufferLength(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG ServiceAction,
    _Inout_ PULONG CommandBufferLength,
    _Out_opt_ PULONG TokenOperationBufferLength,
    _Out_opt_ PULONG ReceiveTokenInformationBufferLength
    );

_IRQL_requires_same_
NTSTATUS
ClasspGetTokenOperationDescriptorLimits(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ ULONG ServiceAction,
    _In_ ULONG MaxParameterBufferLength,
    _Out_ PULONG MaxBlockDescriptorsCount,
    _Out_ PULONGLONG MaxBlockDescriptorsLength
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
ClasspConvertDataSetRangeToBlockDescr(
    _In_    PDEVICE_OBJECT Fdo,
    _In_    PVOID BlockDescr,
    _Inout_ PULONG CurrentBlockDescrIndex,
    _In_    ULONG MaxBlockDescrCount,
    _Inout_ PULONG CurrentLbaCount,
    _In_    ULONGLONG MaxLbaCount,
    _Inout_ PDEVICE_DATA_SET_RANGE DataSetRange,
    _Inout_ PULONGLONG TotalSectorsProcessed
    );

NTSTATUS
ClasspDeviceMediaTypeProperty(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );


_IRQL_requires_same_
PUCHAR
ClasspBinaryToAscii(
    _In_reads_(Length) PUCHAR HexBuffer,
    _In_ ULONG Length,
    _Inout_ PULONG UpdateLength
    );

__inline
BOOLEAN
ClasspIsTokenOperationComplete(
    _In_ ULONG CurrentStatus
    )
{
    BOOLEAN operationCompleted = FALSE;

    switch (CurrentStatus) {
        case OPERATION_COMPLETED_WITH_SUCCESS:
        case OPERATION_COMPLETED_WITH_ERROR:
        case OPERATION_COMPLETED_WITH_RESIDUAL_DATA:
        case OPERATION_TERMINATED: {

            operationCompleted = TRUE;
        }
    }

    return operationCompleted;
}

__inline
BOOLEAN
ClasspIsTokenOperation(
    _In_ PCDB Cdb
    )
{
    BOOLEAN tokenOperation = FALSE;

    if (Cdb) {
        ULONG opCode = Cdb->AsByte[0];
        ULONG serviceAction = Cdb->AsByte[1];

        if ((opCode == SCSIOP_POPULATE_TOKEN && serviceAction == SERVICE_ACTION_POPULATE_TOKEN) ||
            (opCode == SCSIOP_WRITE_USING_TOKEN && serviceAction == SERVICE_ACTION_WRITE_USING_TOKEN)) {

            tokenOperation = TRUE;
        }
    }

    return tokenOperation;
}

__inline
BOOLEAN
ClasspIsReceiveTokenInformation(
    _In_ PCDB Cdb
    )
{
    BOOLEAN receiveTokenInformation = FALSE;

    if (Cdb) {
        ULONG opCode = Cdb->AsByte[0];
        ULONG serviceAction = Cdb->AsByte[1];

        if (opCode == SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION && serviceAction == SERVICE_ACTION_RECEIVE_TOKEN_INFORMATION) {

            receiveTokenInformation = TRUE;
        }
    }

    return receiveTokenInformation;
}

__inline
BOOLEAN
ClasspIsOffloadDataTransferCommand(
    _In_ PCDB Cdb
    )
{
    BOOLEAN offloadCommand = (ClasspIsTokenOperation(Cdb) || ClasspIsReceiveTokenInformation(Cdb)) ? TRUE : FALSE;

    return offloadCommand;
}

extern LIST_ENTRY AllFdosList;


VOID
ClasspInitializeIdleTimer(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspIsPortable(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION   FdoExtension,
    _Out_ PBOOLEAN                      IsPortable
    );

VOID
ClasspGetInquiryVpdSupportInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspGetLBProvisioningInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );


_IRQL_requires_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDetermineTokenOperationCommandSupport(
    _In_ PDEVICE_OBJECT DeviceObject
    );

_IRQL_requires_same_
NTSTATUS
ClasspGetBlockDeviceTokenLimitsInfo(
    _Inout_ PDEVICE_OBJECT DeviceObject
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDeviceProcessOffloadRead(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClassDeviceProcessOffloadWrite(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspServicePopulateTokenTransferRequest(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PIRP Irp
    );

_IRQL_requires_same_
VOID
ClasspReceivePopulateTokenInformation(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext
    );

_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS
ClasspServiceWriteUsingTokenTransferRequest(
    _In_ PDEVICE_OBJECT Fdo,
    _In_ PIRP Irp
    );

_IRQL_requires_same_
VOID
ClasspReceiveWriteUsingTokenInformation(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    );

VOID
ClasspCompleteOffloadRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ NTSTATUS CompletionStatus
    );

VOID
ClasspCleanupOffloadReadContext(
    _In_ __drv_freesMem(mem) POFFLOAD_READ_CONTEXT OffloadReadContext
    );

VOID
ClasspCompleteOffloadRead(
    _In_ POFFLOAD_READ_CONTEXT OffloadReadContext,
    _In_ NTSTATUS CompletionStatus
    );

// PCONTINUATION_ROUTINE
VOID
ClasspPopulateTokenTransferPacketDone(
    _In_ PVOID Context
    );

// PCONTINUATION_ROUTINE
VOID
ClasspReceivePopulateTokenInformationTransferPacketDone(
    _In_ PVOID Context
    );

VOID
ClasspContinueOffloadWrite(
    _In_ __drv_aliasesMem POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    );

VOID
ClasspCleanupOffloadWriteContext(
    _In_ __drv_freesMem(mem) POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    );

VOID
ClasspCompleteOffloadWrite(
    _In_ __drv_freesMem(Mem) POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ NTSTATUS CompletionCausingStatus
    );

VOID
ClasspReceiveWriteUsingTokenInformationDone(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext,
    _In_ NTSTATUS CompletionCausingStatus
    );

VOID
ClasspWriteUsingTokenTransferPacketDone(
    _In_ PVOID Context
    );

VOID
ClasspReceiveWriteUsingTokenInformationTransferPacketDone(
    _In_ POFFLOAD_WRITE_CONTEXT OffloadWriteContext
    );

NTSTATUS
ClasspRefreshFunctionSupportInfo(
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ BOOLEAN ForceQuery
    );

NTSTATUS
ClasspBlockLimitsDataSnapshot(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ BOOLEAN ForceQuery,
    _Out_ PCLASS_VPD_B0_DATA BlockLimitsData,
    _Out_ PULONG GenerationCount
    );

NTSTATUS
InterpretReadCapacity16Data (
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PREAD_CAPACITY16_DATA ReadCapacity16Data
    );

NTSTATUS
ClassReadCapacity16 (
    _Inout_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
ClassDeviceGetLBProvisioningResources(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

_IRQL_requires_same_
NTSTATUS
ClasspStorageEventNotification(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspPowerActivateDevice(
    _In_ PDEVICE_OBJECT DeviceObject
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
ClasspPowerIdleDevice(
    _In_ PDEVICE_OBJECT DeviceObject
    );

IO_WORKITEM_ROUTINE ClassLogThresholdEvent;

NTSTATUS
ClasspLogSystemEventWithDeviceNumber(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ NTSTATUS IoErrorCode
    );

IO_WORKITEM_ROUTINE ClassLogResourceExhaustionEvent;

NTSTATUS
ClasspEnqueueIdleRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
ClasspCompleteIdleRequest(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

NTSTATUS
ClasspPriorityHint(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

VOID
HistoryInitializeRetryLogs(
    _Out_ PSRB_HISTORY History,
    ULONG HistoryCount
    );
#define HISTORYINITIALIZERETRYLOGS(_packet)               \
    {                                                     \
        if (_packet->RetryHistory != NULL)                \
        {                                                 \
            HistoryInitializeRetryLogs(                   \
                _packet->RetryHistory,                    \
                _packet->RetryHistory->TotalHistoryCount  \
                );                                        \
        }                                                 \
    }

VOID
HistoryLogSendPacket(
    TRANSFER_PACKET *Pkt
    );
#define HISTORYLOGSENDPACKET(_packet)        \
    {                                        \
        if (_packet->RetryHistory != NULL) { \
            HistoryLogSendPacket(_packet);   \
        }                                    \
    }

VOID
HistoryLogReturnedPacket(
    TRANSFER_PACKET *Pkt
    );

#define HISTORYLOGRETURNEDPACKET(_packet)      \
    {                                          \
        if (_packet->RetryHistory != NULL) {   \
            HistoryLogReturnedPacket(_packet); \
        }                                      \
    }

BOOLEAN
InterpretSenseInfoWithoutHistory(
    _In_  PDEVICE_OBJECT Fdo,
    _In_opt_  PIRP OriginalRequest,
    _In_  PSCSI_REQUEST_BLOCK Srb,
          UCHAR MajorFunctionCode,
          ULONG IoDeviceCode,
          ULONG PreviousRetryCount,
    _Out_ NTSTATUS * Status,
    _Out_opt_ _Deref_out_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
          LONGLONG * RetryIn100nsUnits
    );

BOOLEAN
ClasspMyStringMatches(
    _In_opt_z_ PCHAR StringToMatch,
    _In_z_ PCHAR TargetString
    );



#define TRACKING_FORWARD_PROGRESS_PATH1                  (0x00000001)
#define TRACKING_FORWARD_PROGRESS_PATH2                  (0x00000002)
#define TRACKING_FORWARD_PROGRESS_PATH3                  (0x00000004)


VOID
ClasspInitializeRemoveTracking(
    _In_ PDEVICE_OBJECT DeviceObject
    );

VOID
ClasspUninitializeRemoveTracking(
    _In_ PDEVICE_OBJECT DeviceObject
    );

RTL_GENERIC_COMPARE_ROUTINE RemoveTrackingCompareRoutine;

RTL_GENERIC_ALLOCATE_ROUTINE RemoveTrackingAllocateRoutine;

RTL_GENERIC_FREE_ROUTINE RemoveTrackingFreeRoutine;

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef PVOID
(*PSRB_ALLOCATE_ROUTINE) (
    _In_ CLONG  ByteSize
    );

PVOID
DefaultStorageRequestBlockAllocateRoutine(
    _In_ CLONG ByteSize
    );


NTSTATUS
CreateStorageRequestBlock(
    _Inout_ PSTORAGE_REQUEST_BLOCK *Srb,
    _In_ USHORT AddressType,
    _In_opt_ PSRB_ALLOCATE_ROUTINE AllocateRoutine,
    _Inout_opt_ ULONG *ByteSize,
    _In_ ULONG NumSrbExData,
    ...
    );

NTSTATUS
InitializeStorageRequestBlock(
    _Inout_bytecount_(ByteSize) PSTORAGE_REQUEST_BLOCK Srb,
    _In_ USHORT AddressType,
    _In_ ULONG ByteSize,
    _In_ ULONG NumSrbExData,
    ...
    );

VOID
ClasspConvertToScsiRequestBlock(
    _Out_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PSTORAGE_REQUEST_BLOCK SrbEx
    );

__inline PCDB
ClasspTransferPacketGetCdb(
    _In_ PTRANSFER_PACKET Pkt
    )
{
    return SrbGetCdb(Pkt->Srb);
}

//
// This inline function calculates number of retries already happened till now for known operation codes
// and set the out parameter - TimesAlreadyRetried with the value,  returns True
//
// For unknown operation codes this function will return false and will set TimesAlreadyRetried with zero
//
__inline BOOLEAN
ClasspTransferPacketGetNumberOfRetriesDone(
    _In_ PTRANSFER_PACKET Pkt,
    _In_ PCDB Cdb,
    _Out_ PULONG TimesAlreadyRetried
    )
{
    PFUNCTIONAL_DEVICE_EXTENSION fdoExtension = Pkt->Fdo->DeviceExtension;
    PCLASS_PRIVATE_FDO_DATA fdoData = fdoExtension->PrivateFdoData;

    if (Cdb->MEDIA_REMOVAL.OperationCode == SCSIOP_MEDIUM_REMOVAL)
    {
        *TimesAlreadyRetried = NUM_LOCKMEDIAREMOVAL_RETRIES - Pkt->NumRetries;
    }
    else if ((Cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE) ||
               (Cdb->MODE_SENSE.OperationCode == SCSIOP_MODE_SENSE10))
    {
        *TimesAlreadyRetried = NUM_MODESENSE_RETRIES - Pkt->NumRetries;
    }
    else if ((Cdb->CDB10.OperationCode == SCSIOP_READ_CAPACITY) ||
               (Cdb->CDB16.OperationCode == SCSIOP_READ_CAPACITY16))
    {
        *TimesAlreadyRetried = NUM_DRIVECAPACITY_RETRIES - Pkt->NumRetries;
    }
    else if (IS_SCSIOP_READWRITE(Cdb->CDB10.OperationCode))
    {
        *TimesAlreadyRetried = fdoData->MaxNumberOfIoRetries - Pkt->NumRetries;
    }
    else if (Cdb->TOKEN_OPERATION.OperationCode == SCSIOP_POPULATE_TOKEN &&
               Cdb->TOKEN_OPERATION.ServiceAction == SERVICE_ACTION_POPULATE_TOKEN)
    {
        *TimesAlreadyRetried = NUM_POPULATE_TOKEN_RETRIES - Pkt->NumRetries;
    }
    else if (Cdb->TOKEN_OPERATION.OperationCode == SCSIOP_WRITE_USING_TOKEN &&
               Cdb->TOKEN_OPERATION.ServiceAction == SERVICE_ACTION_WRITE_USING_TOKEN)
    {
        *TimesAlreadyRetried = NUM_WRITE_USING_TOKEN_RETRIES - Pkt->NumRetries;
    }
    else if (ClasspIsReceiveTokenInformation(Cdb))
    {
        *TimesAlreadyRetried = NUM_RECEIVE_TOKEN_INFORMATION_RETRIES - Pkt->NumRetries;
    }

    else
    {
        *TimesAlreadyRetried = 0;
        return FALSE;
    }


    return TRUE;
}


__inline PVOID
ClasspTransferPacketGetSenseInfoBuffer(
    _In_ PTRANSFER_PACKET Pkt
    )
{
    return SrbGetSenseInfoBuffer(Pkt->Srb);
}

__inline UCHAR
ClasspTransferPacketGetSenseInfoBufferLength(
    _In_ PTRANSFER_PACKET Pkt
    )
{
    return SrbGetSenseInfoBufferLength(Pkt->Srb);
}


__inline VOID
ClasspSrbSetOriginalIrp(
    _In_ PSTORAGE_REQUEST_BLOCK_HEADER Srb,
    _In_ PIRP Irp
    )
{
    if (Srb->Function == SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    {
        ((PSTORAGE_REQUEST_BLOCK)Srb)->MiniportContext = (PVOID)Irp;
    }
    else
    {
        ((PSCSI_REQUEST_BLOCK)Srb)->SrbExtension = (PVOID)Irp;
    }
}

__inline
BOOLEAN
PORT_ALLOCATED_SENSE_EX(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PSTORAGE_REQUEST_BLOCK_HEADER Srb
    )
{
    return ((BOOLEAN)((TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_PORT_DRIVER_ALLOCSENSE) &&
             TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_FREE_SENSE_BUFFER)) &&
            (SrbGetSenseInfoBuffer(Srb) != FdoExtension->SenseData))
            );
}

__inline
VOID
FREE_PORT_ALLOCATED_SENSE_BUFFER_EX(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    _In_ PSTORAGE_REQUEST_BLOCK_HEADER Srb
    )
{
    NT_ASSERT(TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    NT_ASSERT(TEST_FLAG(SrbGetSrbFlags(Srb), SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(SrbGetSenseInfoBuffer(Srb) != FdoExtension->SenseData);

    ExFreePool(SrbGetSenseInfoBuffer(Srb));
    SrbSetSenseInfoBuffer(Srb, FdoExtension->SenseData);
    SrbSetSenseInfoBufferLength(Srb, GET_FDO_EXTENSON_SENSE_DATA_LENGTH(FdoExtension));
    SrbClearSrbFlags(Srb, SRB_FLAGS_FREE_SENSE_BUFFER);
    return;
}

#endif //NTDDI_WIN8

BOOLEAN
ClasspFailurePredictionPeriodMissed(
    _In_ PFUNCTIONAL_DEVICE_EXTENSION FdoExtension
    );

__inline
ULONG
ClasspGetMaxUsableBufferLengthFromOffset(
    _In_ PVOID BaseAddress,
    _In_ ULONG OffsetInBytes,
    _In_ ULONG BaseStructureSizeInBytes
    )
/*++

Routine Description:

    This routine returns the maximum size of a buffer that starts at a given offset,
    based on the size of the containing structure.

Arguments:

    BaseAddress - The base address of the structure. The offset is computed relative to this.

    OffsetInBytes - (BaseAddress + OffsetInBytes) points to the beginning of the buffer.

    BaseStructureSizeInBytes - The size of the structure which contains the buffer.

Return Value:

    max(BaseStructureSizeInBytes - OffsetInBytes, 0). If any operations wrap around,
    the return value is 0.

--*/

{

    ULONG_PTR offsetAddress = ((ULONG_PTR)BaseAddress + OffsetInBytes);

    if (offsetAddress < (ULONG_PTR)BaseAddress) {
        //
        // This means BaseAddress + OffsetInBytes > ULONG_PTR_MAX.
        //
        return 0;
    }

    if (OffsetInBytes > BaseStructureSizeInBytes) {
        return 0;
    }

    return BaseStructureSizeInBytes - OffsetInBytes;
}


BOOLEAN
ClasspIsThinProvisioningError (
    _In_ PSCSI_REQUEST_BLOCK _Srb
    );

__inline
BOOLEAN
ClasspLowerLayerNotSupport (
    _In_ NTSTATUS Status
    )
{
    return ((Status == STATUS_NOT_SUPPORTED) ||
            (Status == STATUS_NOT_IMPLEMENTED) ||
            (Status == STATUS_INVALID_DEVICE_REQUEST) ||
            (Status == STATUS_INVALID_PARAMETER_1));
}

#if defined(__REACTOS__) && (NTDDI_VERSION >= NTDDI_WINBLUE)
__inline
BOOLEAN
ClasspSrbTimeOutStatus (
    _In_ PSTORAGE_REQUEST_BLOCK_HEADER Srb
    )
{
    UCHAR srbStatus = SrbGetSrbStatus(Srb);
    return ((srbStatus == SRB_STATUS_BUS_RESET) ||
            (srbStatus == SRB_STATUS_TIMEOUT) ||
            (srbStatus == SRB_STATUS_COMMAND_TIMEOUT) ||
            (srbStatus == SRB_STATUS_ABORTED));
}
#endif

NTSTATUS
ClassDeviceHwFirmwareGetInfoProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    );

NTSTATUS
ClassDeviceHwFirmwareDownloadProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
ClassDeviceHwFirmwareActivateProcess(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _Inout_ PSCSI_REQUEST_BLOCK Srb
    );

