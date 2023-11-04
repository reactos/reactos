/*++

Copyright (C) Microsoft Corporation. All rights reserved.

Module Name:

    cdrom.h

Abstract:

    Main header file for cdrom.sys.
    This contains structure and function declarations as well as constant values.

Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#ifndef __CDROM_H__
#define __CDROM_H__

#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int
#pragma warning(disable:4152) // nonstandard extension, function/data pointer conversion in expression

#include "wdf.h"
#include "ntddmmc.h"
#include "ntddcdvd.h"
#include "ntddcdrm.h"
#include "ntdddisk.h"
#include "ntddtape.h"
#include "ntddscsi.h"
#include "ntddvol.h"
#include "specstrings.h"
#include "cdromp.h"

// Set component ID for DbgPrintEx calls
#ifndef DEBUG_COMP_ID
    #define DEBUG_COMP_ID   DPFLTR_CDROM_ID
#endif

// Include initguid.h so GUID_CONSOLE_DISPLAY_STATE is declared
#include <initguid.h>

// Include header file and setup GUID for tracing
#include <storswtr.h>
#define WPP_GUID_CDROM      (A4196372, C3C4, 42d5, 87BF, 7EDB2E9BCC27)
#ifndef WPP_CONTROL_GUIDS
    #define WPP_CONTROL_GUIDS   WPP_CONTROL_GUIDS_NORMAL_FLAGS(WPP_GUID_CDROM)
#endif

#ifdef __REACTOS__
#include <pseh/pseh2.h>
#endif

#ifdef __REACTOS__
#undef MdlMappingNoExecute
#define MdlMappingNoExecute 0
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#undef POOL_NX_ALLOCATION
#define POOL_NX_ALLOCATION 0
#endif

// This prototype is needed because, although NTIFS.H is now shipping with
// the WDK, can't include both it and the other headers we already use.
_IRQL_requires_max_(DISPATCH_LEVEL)
NTKERNELAPI
BOOLEAN
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
PsIsThreadTerminating(
    _In_ PETHREAD Thread
    );

//
//
extern CDROM_SCAN_FOR_SPECIAL_INFO CdromHackItems[];

#define CDROM_HACK_DEC_RRD                 (0x00000001)
#define CDROM_HACK_FUJITSU_FMCD_10x        (0x00000002)
//#define CDROM_HACK_HITACHI_1750            (0x00000004) -- obsolete
#define CDROM_HACK_HITACHI_GD_2000         (0x00000008)
#define CDROM_HACK_TOSHIBA_SD_W1101        (0x00000010)
//#define CDROM_HACK_TOSHIBA_XM_3xx          (0x00000020) -- obsolete
//#define CDROM_HACK_NEC_CDDA                (0x00000040) -- obsolete
//#define CDROM_HACK_PLEXTOR_CDDA            (0x00000080) -- obsolete
#define CDROM_HACK_BAD_GET_CONFIG_SUPPORT  (0x00000100)
//#define CDROM_HACK_FORCE_READ_CD_DETECTION (0x00000200) -- obsolete
//#define CDROM_HACK_READ_CD_SUPPORTED       (0x00000400) -- obsolete
#define CDROM_HACK_BAD_TYPE_ONE_GET_CONFIG (0x00000800)
#define CDROM_HACK_BAD_VENDOR_PROFILES     (0x00001000)
#define CDROM_HACK_MSFT_VIRTUAL_ODD        (0x00002000)
#define CDROM_HACK_LOCKED_PAGES            (0x80000000) // not a valid flag to save

#define CDROM_HACK_VALID_FLAGS             (0x00003fff)
#define CDROM_HACK_INVALID_FLAGS           (~CDROM_HACK_VALID_FLAGS)


// A 64k buffer to be written takes the following amount of time:
//   1x    CD ==     75 sectors/sec == 0.4266667 seconds == 4266667 100ns units
//   4x    CD ==    300 sectors/sec == 0.1066667 seconds == 1066667 100ns units
//  10x    CD ==    300 sectors/sec == 0.0426667 seconds ==  426667 100ns units
//   1x   DVD ==    676 sectors/sec == 0.0473373 seconds ==  473373 100ns units
//  16x   DVD == 10,816 sectors/sec == 0.0029586 seconds ==   29586 100ns units
//   1x HDDVD ==  2,230 sectors/sec == 0.0143498 seconds ==  143498 100ns units
#define WRITE_RETRY_DELAY_CD_1x    ((LONGLONG)4266667)
#define WRITE_RETRY_DELAY_CD_4x    ((LONGLONG)1066667)
#define WRITE_RETRY_DELAY_CD_10x   ((LONGLONG) 426667)
#define WRITE_RETRY_DELAY_DVD_1x   ((LONGLONG) 473373)
#define WRITE_RETRY_DELAY_DVD_4x   ((LONGLONG) 118343)
#define WRITE_RETRY_DELAY_DVD_16x  ((LONGLONG)  29586)
#define WRITE_RETRY_DELAY_HDDVD_1x ((LONGLONG) 143498)

//
#define MAXIMUM_RETRIES 4

#define CDROM_GET_CONFIGURATION_TIMEOUT     (0x4)
#define CDROM_READ_DISC_INFORMATION_TIMEOUT (0x4)
#define CDROM_TEST_UNIT_READY_TIMEOUT       (0x14)
#define CDROM_GET_PERFORMANCE_TIMEOUT       (0x14)
#define CDROM_READ_CAPACITY_TIMEOUT         (0x14)

#define START_UNIT_TIMEOUT  (60 * 4)

// Used to detect the loss of the autorun irp.
#define MEDIA_CHANGE_TIMEOUT_TIME  300

// Indicates whether is is safe to send StartUnit commands
// to this device. It will only be off for some removeable devices.
#define DEV_SAFE_START_UNIT 0x00000004

// Indicates that the device is connected to a backup power supply
// and hence write-through and synch cache requests may be ignored
#define DEV_POWER_PROTECTED 0x00000010

// The following CDROM_SPECIAL_ flags are set in ScanForSpecialFlags
// in the Device Extension

// Never Spin Up/Down the drive (may not handle properly)
#define CDROM_SPECIAL_DISABLE_SPIN_DOWN                 0x00000001
//#define CDROM_SPECIAL_DISABLE_SPIN_UP                   0x00000002

// Don't bother to lock the queue when powering down
// (used mostly to send a quick stop to a cdrom to abort audio playback)
//#define CDROM_SPECIAL_NO_QUEUE_LOCK                     0x00000008

// Disable write cache due to known bugs
#define CDROM_SPECIAL_DISABLE_WRITE_CACHE               0x00000010

// Used to indicate that this request shouldn't invoke any power type operations
// like spinning up the drive.

#define SRB_CLASS_FLAGS_LOW_PRIORITY      0x10000000

// Used to indicate that an SRB is the result of a paging operation.
#define SRB_CLASS_FLAGS_PAGING            0x40000000

typedef struct _ERROR_RECOVERY_DATA {
    MODE_PARAMETER_HEADER   Header;
    MODE_PARAMETER_BLOCK    BlockDescriptor;
    MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA, *PERROR_RECOVERY_DATA;

// A compile-time check of the 30,000 limit not overflowing ULONG size...
// Note that it is not expected that a release (FRE) driver will normally
// have such a large history, instead using the compression function.
#define CDROM_INTERPRET_SENSE_INFO2_MAXIMUM_HISTORY_COUNT   30000
C_ASSERT( (MAXULONG - sizeof(SRB_HISTORY)) / 30000 >= sizeof(SRB_HISTORY_ITEM) );

// Intended to reuse a defined IOCTL code that not seen in Optical stack and does not require input parameter.
// This fake IOCTL is used used for MCN process sync-ed with serial queue.
#define IOCTL_MCN_SYNC_FAKE_IOCTL    IOCTL_DISK_UPDATE_DRIVE_SIZE

/*++////////////////////////////////////////////////////////////////////////////

PCDROM_ERROR_HANDLER()

Routine Description:

    This routine is a callback into the driver to handle errors.  The queue
    shall not be unfrozen when this error handler is called, even though the
    SRB flags may mark the queue as having been frozen due to this SRB.

Irql:

    This routine will be called at KIRQL <= DISPATCH_LEVEL

Arguments:

    DeviceObject is the device object the error occurred on.

    Srb is the Srb that was being processed when the error occurred.

    Status may be overwritten by the routine if it decides that the error
        was benign, or otherwise wishes to change the returned status code
        for this command

    Retry may be overwritten to specify that this command should or should
        not be retried (if the callee supports retrying commands)

Return Value:

    status

--*/
struct _CDROM_DEVICE_EXTENSION;     // *PCDROM_DEVICE_EXTENSION;
typedef struct _CDROM_DEVICE_EXTENSION
                CDROM_DEVICE_EXTENSION,
                *PCDROM_DEVICE_EXTENSION;

typedef
VOID
(*PCDROM_ERROR_HANDLER) (
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb,
    _Inout_ PNTSTATUS             Status,
    _Inout_ PBOOLEAN              Retry
    );

// CdRom driver extension
typedef struct _CDROM_DRIVER_EXTENSION {
    ULONG               Version;
    PDRIVER_OBJECT      DriverObject;
    ULONG               Flags;

} CDROM_DRIVER_EXTENSION, *PCDROM_DRIVER_EXTENSION;

#define CDROM_FLAG_WINPE_MODE   0x00000001

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CDROM_DRIVER_EXTENSION, DriverGetExtension)

#define CdromMmcUpdateComplete  0
#define CdromMmcUpdateRequired  1
#define CdromMmcUpdateStarted   2

typedef struct _CDROM_MMC_EXTENSION {

    BOOLEAN         IsMmc;          // mmc device
    BOOLEAN         IsAACS;         // aacs compatible device
    BOOLEAN         IsWriter;       // the drive is a writer or not
    BOOLEAN         WriteAllowed;   // currently allow write request or not

    BOOLEAN         IsCssDvd;       // A CSS protected DVD or CPPM-protected DVDAudio media in Drive.
    BOOLEAN         StreamingReadSupported;   // the drive supports streaming for reads
    BOOLEAN         StreamingWriteSupported;  // the drive supports streaming for writes

    LONG            UpdateState;

    // The feature number defines the level and form of
    // validation that needs to be performed on Io requests
    FEATURE_NUMBER  ValidationSchema;
    ULONG           Blocking;

    SCSI_REQUEST_BLOCK          CapabilitiesSrb;
    PIRP                        CapabilitiesIrp;
    WDFREQUEST                  CapabilitiesRequest;
    SENSE_DATA                  CapabilitiesSenseData;
    _Field_size_bytes_(CapabilitiesBufferSize)
    PGET_CONFIGURATION_HEADER   CapabilitiesBuffer;
    ULONG                       CapabilitiesBufferSize;
    PMDL                        CapabilitiesMdl;

    BOOLEAN                     ReadCdC2Pointers;
    BOOLEAN                     ReadCdSubCode;

} CDROM_MMC_EXTENSION, *PCDROM_MMC_EXTENSION;

typedef struct _CDROM_SCRATCH_READ_WRITE_CONTEXT {

    // Information about the data that we need to read/write
    ULONG           PacketsCount;
    ULONG           TransferedBytes;
    ULONG           EntireXferLen;
    ULONG           MaxLength;
    PUCHAR          DataBuffer;
    LARGE_INTEGER   StartingOffset;
    BOOLEAN         IsRead;

    // A pointer to the SRB history item to be filled upon completion
    PSRB_HISTORY_ITEM   SrbHistoryItem;

} CDROM_SCRATCH_READ_WRITE_CONTEXT, *PCDROM_SCRATCH_READ_WRITE_CONTEXT;

// Many commands get double-buffered.  Since the max
// transfer size is typically 64k, most of these requests
// can be handled with a single pre-allocated buffer.
typedef struct _CDROM_SCRATCH_CONTEXT {

    _Field_range_(4*1024, 64*1024)       ULONG   ScratchBufferSize; // 0x1000..0x10000 (4k..64k)
    _Field_size_bytes_(ScratchBufferSize)    PVOID   ScratchBuffer;     // used to get data for clients

    PMDL                         ScratchBufferMdl;  // used to get data for clients

    WDFREQUEST                   ScratchRequest;
    PSCSI_REQUEST_BLOCK          ScratchSrb;
    PSENSE_DATA                  ScratchSense;
    PSRB_HISTORY                 ScratchHistory;

    // This MDL is used to performed the request whose required transfer size is bigger than adaptor's max.
    PMDL                         PartialMdl;
    BOOLEAN                      PartialMdlIsBuilt;

    // For debugging, set/clear this field when using the scratch buffer/request
    PVOID                        ScratchInUse;
    PCSTR                        ScratchInUseFileName;
    ULONG                        ScratchInUseLineNumber;

    // Stuff for asynchronous retrying of the transfer.
    ULONG                        NumRetries;

    // Read Write context
    CDROM_SCRATCH_READ_WRITE_CONTEXT ScratchReadWriteContext;

} CDROM_SCRATCH_CONTEXT, *PCDROM_SCRATCH_CONTEXT;

// Context structure for the IOCTL work item
typedef struct _CDROM_IOCTL_CONTEXT {

    WDFREQUEST OriginalRequest;

} CDROM_IOCTL_CONTEXT, *PCDROM_IOCTL_CONTEXT;

// Context structure for the read/write work item
typedef struct _CDROM_READ_WRITE_CONTEXT {

    WDFREQUEST OriginalRequest;

} CDROM_READ_WRITE_CONTEXT, *PCDROM_READ_WRITE_CONTEXT;

typedef struct _CDROM_DATA {

    CDROM_MMC_EXTENSION     Mmc;

    // hack flags for ScanForSpecial routines
    ULONG_PTR               HackFlags;

    // the error handling routines need to be per-device, not per-driver....
    PCDROM_ERROR_HANDLER    ErrorHandler;

    // Indicates whether an audio play operation is currently being performed.
    // Only thing this does is prevent reads and toc requests while playing audio.
    BOOLEAN                 PlayActive;

    // indicate we need to pick a default dvd region for the user if we can
    ULONG                   PickDvdRegion;

    // The well known name link for this device.
    UNICODE_STRING          WellKnownName;

    // We need to distinguish between the two...
    ULONG                   MaxPageAlignedTransferBytes;
    ULONG                   MaxUnalignedTransferBytes;

    // Indicates that this is a DEC RRD cdrom.
    // This drive requires software to fix responses from the faulty firmware
    BOOLEAN                 IsDecRrd;

    // Storage for the error recovery page. This is used as the method
    // to switch block sizes for some drive-specific error recovery routines.
    // ERROR_RECOVERY_DATA     recoveryData;    //obsolete along with error process for TOSHIBA_XM_3xx

    // Indicates that the device is in exclusive mode and only
    // the requests from the exclusive owner will be processed.
    WDFFILEOBJECT           ExclusiveOwner;

    // Caller name of the owner, if the device is in exclusive mode.
    UCHAR                   CallerName[CDROM_EXCLUSIVE_CALLER_LENGTH];

    // Indicates that the device speed should be set to
    // default value on the next media change.
    BOOLEAN                 RestoreDefaults;

    // How long to wait between retries if a READ/WRITE irp
    // gets a LWIP (2/4/7, 2/4/8)?
    LONGLONG                ReadWriteRetryDelay100nsUnits;

    // Cached Device Type information. Maybe FILE_DEVICE_CD_ROM or FILE_DEVICE_DVD
    // CommonExtension.DevInfo->DeviceType maybe FILE_DEVICE_CD_ROM when this field is FILE_DEVICE_DVD
    DEVICE_TYPE             DriveDeviceType;

    _Field_size_bytes_(CachedInquiryDataByteCount)
        PINQUIRYDATA        CachedInquiryData;
    ULONG                   CachedInquiryDataByteCount;

} CDROM_DATA, *PCDROM_DATA;


typedef struct _CDROM_POWER_OPTIONS {
    ULONG   PowerDown              :  1;
    ULONG   LockQueue              :  1;
    ULONG   HandleSpinDown         :  1;
    ULONG   HandleSpinUp           :  1;
    ULONG   Reserved               : 27;
} CDROM_POWER_OPTIONS, *PCDROM_POWER_OPTIONS;

// this is a private enum, but must be kept here
// to properly compile size of CDROM_DEVICE_EXTENSION
typedef enum {
    PowerDownDeviceInitial,
    PowerDownDeviceLocked,
    PowerDownDeviceQuiesced,
    PowerDownDeviceFlushed,
    PowerDownDeviceStopped,
    PowerDownDeviceOff,
    PowerDownDeviceUnlocked
} CDROM_POWER_DOWN_STATE;

// this is a private enum, but must be kept here
// to properly compile size of CDROM_DEVICE_EXTENSION
typedef enum {
    PowerUpDeviceInitial,
    PowerUpDeviceLocked,
    PowerUpDeviceOn,
    PowerUpDeviceStarted,
    PowerUpDeviceUnlocked
} CDROM_POWER_UP_STATE;

// this is a private structure, but must be kept here
// to properly compile size of CDROM_DEVICE_EXTENSION
typedef struct _CDROM_POWER_CONTEXT {

    BOOLEAN                 InUse;

    LARGE_INTEGER           StartTime;
    LARGE_INTEGER           Step1CompleteTime;  // for SYNC CACHE in power down case
    LARGE_INTEGER           CompleteTime;

    union {
        CDROM_POWER_DOWN_STATE  PowerDown;
        CDROM_POWER_UP_STATE    PowerUp;    // currently not used.
    } PowerChangeState;

    CDROM_POWER_OPTIONS     Options;

    WDFREQUEST              PowerRequest;

    SCSI_REQUEST_BLOCK      Srb;
    SENSE_DATA              SenseData;

    ULONG                   RetryCount;
    LONGLONG                RetryIntervalIn100ns;

} CDROM_POWER_CONTEXT, *PCDROM_POWER_CONTEXT;

// device extension structure
typedef struct _CDROM_DEVICE_EXTENSION {

    // Version control field
    ULONG           Version;

    // structure size
    ULONG           Size;

    // the structure is fully ready to use
    BOOLEAN         IsInitialized;

    // the device is active
    BOOLEAN         IsActive;

    // the device is surprise removed.
    BOOLEAN         SurpriseRemoved;

    // Back pointer to device object
    WDFDEVICE       Device;

    // Save IoTarget here, do not retrieve anytime.
    WDFIOTARGET     IoTarget;

    // Additional WDF queue for serial I/O processing
    WDFQUEUE        SerialIOQueue;

    // A separate queue for all the create file requests in sync with device
    // removal
    WDFQUEUE        CreateQueue;

    //Main timer of driver, will do Mcn work. (once per second)
    WDFTIMER        MainTimer;

    // Pointer to the initialization data for this driver.  This is more
    // efficient than constantly getting the driver extension.
    PCDROM_DRIVER_EXTENSION DriverExtension;

    // WDM device information
    PDEVICE_OBJECT  DeviceObject;

    // Pointer to the physical device object we attached to
    PDEVICE_OBJECT  LowerPdo;

    // FILE_DEVICE_CD_ROM -- 2
    DEVICE_TYPE     DeviceType;

    // The name of the object
    UNICODE_STRING  DeviceName;

    // System device number
    ULONG           DeviceNumber;

    // Values for the flags are below.
    USHORT          DeviceFlags;

    // Flags for special behaviour required by different hardware,
    // such as never spinning down or disabling advanced features such as write cache
    ULONG           ScanForSpecialFlags;

    // Add default Srb Flags.
    ULONG           SrbFlags;

    // Request timeout in seconds;
    ULONG           TimeOutValue;

    //The SCSI address of the device.
    SCSI_ADDRESS    ScsiAddress;

    // Buffer for drive parameters returned in IO device control.
    DISK_GEOMETRY   DiskGeometry;

    // Log2 of sector size
    UCHAR           SectorShift;

    // Length of partition in bytes
    LARGE_INTEGER   PartitionLength;

    // Number of bytes before start of partition
    LARGE_INTEGER   StartingOffset;

    // Interface name string returned by IoRegisterDeviceInterface.
    UNICODE_STRING  MountedDeviceInterfaceName;

    // Device capabilities
    PSTORAGE_DEVICE_DESCRIPTOR  DeviceDescriptor;

    // SCSI port driver capabilities
    PSTORAGE_ADAPTER_DESCRIPTOR AdapterDescriptor;

    // Device power properties
    PDEVICE_POWER_DESCRIPTOR PowerDescriptor;

    // Request Sense Buffer
    PSENSE_DATA     SenseData;

    // Total number of SCSI protocol errors on the device.
    ULONG           ErrorCount;

    // Lock count for removable media.
    LONG            LockCount;
    LONG            ProtectedLockCount;
    LONG            InternalLockCount;

    KEVENT          EjectSynchronizationEvent;
    WDFWAITLOCK     EjectSynchronizationLock;

    // Indicates that the necessary data structures for media change
    // detection have been initialized.
    PMEDIA_CHANGE_DETECTION_INFO MediaChangeDetectionInfo;

    // Contains necessary data structures for ZPODD support.
    PZERO_POWER_ODD_INFO         ZeroPowerODDInfo;

    // File system context. Used for kernel-mode requests to disable autorun.
    FILE_OBJECT_CONTEXT   KernelModeMcnContext;

    // Count of media changes.  This field is only valid for the root partition
    // (ie. if PhysicalDevice == NULL).
    ULONG               MediaChangeCount;

    // Storage for a release queue request.
    WDFSPINLOCK         ReleaseQueueSpinLock;
    WDFREQUEST          ReleaseQueueRequest;
    SCSI_REQUEST_BLOCK  ReleaseQueueSrb;
    WDFMEMORY           ReleaseQueueInputMemory;    //This is a wrapper of ReleaseQueueSrb
    BOOLEAN             ReleaseQueueNeeded;
    BOOLEAN             ReleaseQueueInProgress;

    // Context structure for power operations.  Since we can only have
    // one D irp at any time in the stack we don't need to worry about
    // allocating multiple of these structures.
    CDROM_POWER_CONTEXT     PowerContext;
    BOOLEAN                 PowerDownInProgress;

#if (NTDDI_VERSION >= NTDDI_WIN8)
    BOOLEAN                 IsVolumeOnlinePending;
    WDFQUEUE                ManualVolumeReadyQueue;
#endif

    // Lock for Shutdown/Flush operations that need to stop/start the queue
    WDFWAITLOCK             ShutdownFlushWaitLock;

    // device specific data area
    CDROM_DATA              DeviceAdditionalData;

    // scratch buffer related fields.
    CDROM_SCRATCH_CONTEXT   ScratchContext;

    // Hold new private data that only classpnp should modify
    // in this structure.
    PCDROM_PRIVATE_FDO_DATA PrivateFdoData;

    // Work item for async reads and writes and its context
    WDFWORKITEM                 ReadWriteWorkItem;
    CDROM_READ_WRITE_CONTEXT    ReadWriteWorkItemContext;

    // Auxiliary WDF object for processing ioctl requests that need to go down
    // to the port driver.
    WDFWORKITEM             IoctlWorkItem;
    CDROM_IOCTL_CONTEXT     IoctlWorkItemContext;

} CDROM_DEVICE_EXTENSION, *PCDROM_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CDROM_DEVICE_EXTENSION, DeviceGetExtension)

// a type definition for functions to be called after synchronization
typedef
NTSTATUS
SYNC_HANDLER (
    _In_ WDFDEVICE  Device,
    _In_ WDFREQUEST Request
    );

typedef SYNC_HANDLER *PSYNC_HANDLER ;

typedef struct _CDROM_REQUEST_CONTEXT {
    PCDROM_DEVICE_EXTENSION     DeviceExtension;

    WDFREQUEST      OriginalRequest;

    LARGE_INTEGER   TimeReceived;
    LARGE_INTEGER   TimeSentDownFirstTime;
    LARGE_INTEGER   TimeSentDownLasttTime;


    ULONG           RetriedCount;

    // Used to send down an incoming IOCTL in the original context
    BOOLEAN         SyncRequired;
    PKEVENT         SyncEvent;
    PSYNC_HANDLER   SyncCallback;

    //
    // Used for READ/WRITE requests.
    // The reason why kernel primitives are used for the spinlock and
    // the timer instead of WDF object is there is a race condition
    // between the cancel callback and the timer routine.
    // Because of this, the timer and the spin lock need to be associated
    // per request rather than using shared memory in the device extension
    // and it is possible for the WDF object initialization to fail whereas
    // the kernel primitives initialize provided memory.  Initializing the
    // kernel primitives will never fail.  Since READ/WRITE is a critical
    // code path, it is not desired for READs or WRITEs to fail due to
    // an allocation failure that can be avoided and because it is not
    // uncommon to see a failure during a READ or WRITE that may not
    // occur upon a retry.
    //
    KSPIN_LOCK      ReadWriteCancelSpinLock;
    KTIMER          ReadWriteTimer;
    KDPC            ReadWriteDpc;
    BOOLEAN         ReadWriteIsCompleted;
    BOOLEAN         ReadWriteRetryInitialized;

} CDROM_REQUEST_CONTEXT, *PCDROM_REQUEST_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CDROM_REQUEST_CONTEXT, RequestGetContext)

// Define context structure for asynchronous completions.
typedef struct _COMPLETION_CONTEXT {
    WDFDEVICE           Device;
    SCSI_REQUEST_BLOCK  Srb;
} COMPLETION_CONTEXT, *PCOMPLETION_CONTEXT;


//
#define SCSI_CDROM_TIMEOUT          10
#define SCSI_CHANGER_BONUS_TIMEOUT  10

//
// This value is used as the upper limit for all commands CDROM sends down
// to device, unless the default timeout value is overriden by registry value
// "TimeOutValue" and it is larger than this value.
//
#define SCSI_CDROM_OPC_TIMEOUT      260

#define HITACHI_MODE_DATA_SIZE      12
#define MODE_DATA_SIZE              64

#define RAW_SECTOR_SIZE           2352
#define COOKED_SECTOR_SIZE        2048

#define CDROM_SRB_LIST_SIZE          4

#define PLAY_ACTIVE(x) (x->DeviceAdditionalData.PlayActive)

#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))

// Sector types for READ_CD

#define ANY_SECTOR                0
#define CD_DA_SECTOR              1
#define YELLOW_MODE1_SECTOR       2
#define YELLOW_MODE2_SECTOR       3
#define FORM2_MODE1_SECTOR        4
#define FORM2_MODE2_SECTOR        5

#define MAX_COPY_PROTECT_AGID     4

#ifdef ExAllocatePool
    #undef ExAllocatePool
    #define ExAllocatePool #assert(FALSE)
#endif

// memory allocation tags
// Sc?? - Mass storage driver tags
// ScC<number> - Class driver misc allocations
// ScC? - CdRom

#define CDROM_TAG_AUTORUN_DISABLE        'ACcS' // "ScCA" - Autorun disable functionality
#define CDROM_TAG_MEDIA_CHANGE_DETECTION 'aCcS' // "ScCa" - Media change detection

#define CDROM_TAG_SCRATCH               'BCcS'  // "ScSB" - Scratch buffer (usually 64k)
#define CDROM_TAG_GET_CONFIG            'CCcS'  // "ScCC" - Ioctl GET_CONFIGURATION
#define CDROM_TAG_COMPLETION_CONTEXT    'cCcS'  // "ScCc" - Context of completion routine
#define CDROM_TAG_DESCRIPTOR            'DCcS'  // "ScCD" - Adaptor & Device descriptor buffer
#define CDROM_TAG_DISC_INFO             'dCcS'  // "ScCd" - Disc information
#define CDROM_TAG_SYNC_EVENT            'eCcS'  // "ScCe" - Request sync event
#define CDROM_TAG_FEATURE               'FCcS'  // "ScCF" - Feature descriptor
#define CDROM_TAG_GESN                  'GCcS'  // "ScCG" - GESN buffer
#define CDROM_TAG_SENSE_INFO            'ICcS'  // "ScCI" - Sense info buffers
#define CDROM_TAG_INQUIRY               'iCcS'  // "ScCi" - Cached inquiry buffer
#define CDROM_TAG_MODE_DATA             'MCcS'  // "ScCM" - Mode data buffer
#define CDROM_TAG_STREAM                'OCCS'  // "SCCO" - Set stream buffer
#define CDROM_TAG_NOTIFICATION          'oCcS'  // "ScCo" - Device Notification buffer
#define CDROM_TAG_PLAY_ACTIVE           'pCcS'  // "ScCp" - Play active checks
#define CDROM_TAG_REGISTRY              'rCcS'  // "ScCr" - Registry string
#define CDROM_TAG_SRB                   'SCcS'  // "ScCS" - Srb allocation
#define CDROM_TAG_STRINGS               'sCcS'  // "ScCs" - Assorted string data
#define CDROM_TAG_UPDATE_CAP            'UCcS'  // "ScCU" - Update capacity path
#define CDROM_TAG_ZERO_POWER_ODD        'ZCcS'  // "ScCZ" - Zero Power ODD

#define DVD_TAG_READ_KEY                'uCcS'  // "ScCu" - Read buffer for dvd key
#define DVD_TAG_RPC2_CHECK              'VCcS'  // "ScCV" - Read buffer for dvd/rpc2 check
#define DVD_TAG_DVD_REGION              'vCcS'  // "ScCv" - Read buffer for rpc2 check
#define DVD_TAG_SECURITY                'XCcS'  // "ScCX" - Security descriptor


// registry keys and data entry names.
#define CDROM_SUBKEY_NAME                       (L"CdRom")  // store new settings here
#define CDROM_READ_CD_NAME                      (L"ReadCD") // READ_CD support previously detected
#define CDROM_NON_MMC_DRIVE_NAME                (L"NonMmc") // MMC commands hang
#define CDROM_TYPE_ONE_GET_CONFIG_NAME          (L"NoTypeOneGetConfig") // Type One Get Config commands not supported
#define CDROM_NON_MMC_VENDOR_SPECIFIC_PROFILE   (L"NonMmcVendorSpecificProfile") // GET_CONFIG returns vendor specific header
                                                                                 // profiles that are not per spec (length divisible by 4)
#define DVD_DEFAULT_REGION          (L"DefaultDvdRegion")   // this is init. by the dvd class installer
#define DVD_MAX_REGION              8

// AACS defines
#define AACS_MKB_PACK_SIZE  0x8000 // does not include header

//enumeration of device interfaces need to be registered.
typedef enum {
    CdRomDeviceInterface = 0,     // CdRomClassGuid
    MountedDeviceInterface        // MOUNTDEV_MOUNTED_DEVICE_GUID
} CDROM_DEVICE_INTERFACES, *PCDROM_DEVICE_INTERFACES;


typedef
VOID
(*PCDROM_SCAN_FOR_SPECIAL_HANDLER) (
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ ULONG_PTR                Data
    );

// Rountines Definition

#define FREE_POOL(_PoolPtr)     \
    if (_PoolPtr != NULL) {     \
        ExFreePool(_PoolPtr);   \
        _PoolPtr = NULL;        \
    }

#define EXCLUSIVE_MODE(_CdData)                 (_CdData->ExclusiveOwner != NULL)
#define EXCLUSIVE_OWNER(_CdData, _FileObject)   (_CdData->ExclusiveOwner == _FileObject)

#define IS_SCSIOP_READ(opCode)         \
      ((opCode == SCSIOP_READ6)   ||   \
       (opCode == SCSIOP_READ)    ||   \
       (opCode == SCSIOP_READ12)  ||   \
       (opCode == SCSIOP_READ16))

#define IS_SCSIOP_WRITE(opCode)         \
      ((opCode == SCSIOP_WRITE6)   ||   \
       (opCode == SCSIOP_WRITE)    ||   \
       (opCode == SCSIOP_WRITE12)  ||   \
       (opCode == SCSIOP_WRITE16))

#define IS_SCSIOP_READWRITE(opCode)  (IS_SCSIOP_READ(opCode) || IS_SCSIOP_WRITE(opCode))

// Bit Flag Macros
#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

FORCEINLINE // __REACTOS__
BOOLEAN
ValidChar(UCHAR Ch)
{
    if (((Ch >= '0') && (Ch <= '9')) ||
        (((Ch|0x20) >= 'a') && ((Ch|0x20) <= 'z')) ||
        (strchr(" .,:;_-", Ch) != NULL))
    {
        return TRUE;
    }
    return FALSE;
}

// could be #define, but this allows typechecking
FORCEINLINE // __REACTOS__
BOOLEAN
PORT_ALLOCATED_SENSE(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb
    )
{
    UNREFERENCED_PARAMETER(DeviceExtension);
    return (BOOLEAN)((TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE) &&
             TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER))
            );
}

FORCEINLINE // __REACTOS__
VOID
FREE_PORT_ALLOCATED_SENSE_BUFFER(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb
    )
{
#ifndef DEBUG
    UNREFERENCED_PARAMETER(DeviceExtension);
#endif
    NT_ASSERT(TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_PORT_DRIVER_ALLOCSENSE));
    NT_ASSERT(TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER));
    NT_ASSERT(Srb->SenseInfoBuffer != DeviceExtension->SenseData);

    ExFreePool(Srb->SenseInfoBuffer);
    Srb->SenseInfoBuffer = NULL;
    Srb->SenseInfoBufferLength = 0;
    CLEAR_FLAG(Srb->SrbFlags, SRB_FLAGS_FREE_SENSE_BUFFER);
    return;
}

// Standard driver entry function
DRIVER_INITIALIZE DriverEntry;

// Driver Event callbacks
EVT_WDF_DRIVER_DEVICE_ADD DriverEvtDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP DriverEvtCleanup;

// Device Event callbacks

EVT_WDF_OBJECT_CONTEXT_CLEANUP DeviceEvtCleanup;

EVT_WDF_FILE_CLOSE DeviceEvtFileClose;

EVT_WDF_IO_IN_CALLER_CONTEXT DeviceEvtIoInCallerContext;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT DeviceEvtSelfManagedIoInit;

EVT_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP DeviceEvtSelfManagedIoCleanup;

EVT_WDF_DEVICE_D0_ENTRY DeviceEvtD0Entry;

EVT_WDF_DEVICE_D0_EXIT DeviceEvtD0Exit;

EVT_WDF_DEVICE_SURPRISE_REMOVAL DeviceEvtSurpriseRemoval;

// Create Queue Event callbacks

EVT_WDF_IO_QUEUE_IO_DEFAULT CreateQueueEvtIoDefault;

// Sequential Queue Event callbacks

// We do not use KMDF annotation for the following function, because it handles
// both read and write requests and there is no single annotation for that.
VOID
NTAPI /* ReactOS Change: GCC Does not support STDCALL by default */
SequentialQueueEvtIoReadWrite(
    _In_ WDFQUEUE     Queue,
    _In_ WDFREQUEST   Request,
    _In_ size_t       Length
    );

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL SequentialQueueEvtIoDeviceControl;

EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE SequentialQueueEvtCanceledOnQueue;

// Miscellaneous request callbacks

EVT_WDF_OBJECT_CONTEXT_CLEANUP RequestEvtCleanup;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS RequestProcessShutdownFlush;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS RequestProcessSetPower;


// helper functions

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceClaimRelease(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN      Release
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceReleaseMcnResources(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRetrieveDescriptor(
    _In_ WDFDEVICE                              Device,
    _In_ PSTORAGE_PROPERTY_ID                   PropertyId,
    _Outptr_ PSTORAGE_DESCRIPTOR_HEADER*        Descriptor
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceRetrieveHackFlagsFromRegistry(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceHackFlagsScan(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ ULONG_PTR                Data
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
StringsAreMatched(
    _In_opt_z_ PCHAR StringToMatch,
    _In_z_     PCHAR TargetString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceGetParameter(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_opt_ PWSTR                  SubkeyName,
    _In_ PWSTR                      ParameterName,
    _Inout_ PULONG                  ParameterValue  // also default value
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSetParameter(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_opt_z_ PWSTR                SubkeyName,
    _In_ PWSTR                      ParameterName,
    _In_ ULONG                      ParameterValue
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
DeviceGetTimeOutValueFromRegistry();

_IRQL_requires_max_(APC_LEVEL)
VOID
ScanForSpecialHandler(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ ULONG_PTR               HackFlags
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceSendSrbSynchronously(
    _In_ WDFDEVICE           Device,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_opt_ PVOID           BufferAddress,
    _In_ ULONG               BufferLength,
    _In_ BOOLEAN             WriteToDevice,
    _In_opt_ WDFREQUEST      OriginalRequest
    );

BOOLEAN
RequestSenseInfoInterpret(
    _In_      PCDROM_DEVICE_EXTENSION   DeviceExtension,
    _In_      WDFREQUEST                Request, // get IRP MJ code, IoControlCode from it (or attached original request)
    _In_      PSCSI_REQUEST_BLOCK       Srb,
    _In_      ULONG                     RetriedCount,
    _Out_     NTSTATUS*                 Status,
    _Out_opt_ _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
              LONGLONG*                 RetryIntervalIn100ns
    );

VOID
RequestSetReceivedTime(
    _In_ WDFREQUEST Request
    );

VOID
RequestSetSentTime(
    _In_ WDFREQUEST Request
    );

VOID
RequestClearSendTime(
    _In_ WDFREQUEST Request
    );

BOOLEAN
RequestSenseInfoInterpretForScratchBuffer(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_    ULONG                   RetriedCount,
    _Out_   NTSTATUS*               Status,
    _Out_ _Deref_out_range_(0, MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
            LONGLONG*               RetryIntervalIn100ns
    );


VOID
DeviceSendNotification(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ const GUID*              Guid,
    _In_ ULONG                    ExtraDataSize,
    _In_opt_ PVOID                ExtraData
    );

VOID
DeviceSendStartUnit(
    _In_ WDFDEVICE Device
    );

EVT_WDF_REQUEST_COMPLETION_ROUTINE DeviceAsynchronousCompletion;

VOID
DeviceReleaseQueue(
    _In_ WDFDEVICE    Device
    );

EVT_WDF_REQUEST_COMPLETION_ROUTINE DeviceReleaseQueueCompletion;

VOID
DevicePerfIncrementErrorCount(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceCacheDeviceInquiryData(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceCacheGetConfigurationData(
    _In_  WDFDEVICE     Device
    );

_IRQL_requires_max_(APC_LEVEL)
PVOID
DeviceFindFeaturePage(
    _In_reads_bytes_(Length) PGET_CONFIGURATION_HEADER   FeatureBuffer,
    _In_ ULONG const                                Length,
    _In_ FEATURE_NUMBER const                       Feature
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DevicePrintAllFeaturePages(
    _In_reads_bytes_(Usable) PGET_CONFIGURATION_HEADER   Buffer,
    _In_ ULONG const                                Usable
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceSetRawReadInfo(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
MediaReadCapacity(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
MediaReadCapacityDataInterpret(
    _In_ WDFDEVICE            Device,
    _In_ PREAD_CAPACITY_DATA  ReadCapacityBuffer
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitReleaseQueueContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitPowerContext(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceCreateWellKnownName(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceInitializeDvd(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DevicePickDvdRegion(
    _In_ WDFDEVICE Device
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceRegisterInterface(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ CDROM_DEVICE_INTERFACES InterfaceType
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
DeviceSendPowerDownProcessRequest(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_opt_ PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_opt_ WDFCONTEXT Context
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceScanForSpecial(
    _In_ PCDROM_DEVICE_EXTENSION          DeviceExtension,
    _In_ CDROM_SCAN_FOR_SPECIAL_INFO      DeviceList[],
    _In_ PCDROM_SCAN_FOR_SPECIAL_HANDLER  Function
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitializeHotplugInfo(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
DeviceErrorHandlerForMmc(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb,
    _Inout_ PNTSTATUS             Status,
    _Inout_ PBOOLEAN              Retry
    );

NTSTATUS
DeviceErrorHandlerForHitachiGD2000(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK      Srb,
    _Inout_ PNTSTATUS             Status,
    _Inout_ PBOOLEAN              Retry
    );

EVT_WDF_WORKITEM DeviceRestoreDefaultSpeed;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitializeMediaChangeDetection(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension
    );

VOID
DeviceSetMediaChangeStateEx(
    _In_        PCDROM_DEVICE_EXTENSION       DeviceExtension,
    _In_        MEDIA_CHANGE_DETECTION_STATE  NewState,
    _Inout_opt_ PMEDIA_CHANGE_DETECTION_STATE OldState
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceSendDelayedMediaChangeNotifications(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
RequestSynchronizeProcessWithSerialQueue(
    _In_ WDFDEVICE Device,
    _In_ WDFREQUEST Request
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceCleanupProtectedLocks(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PFILE_OBJECT_CONTEXT     FileObjectContext
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceCleanupDisableMcn(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ PFILE_OBJECT_CONTEXT     FileObjectContext
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceUnlockExclusive(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFFILEOBJECT            FileObject,
    _In_ BOOLEAN                  IgnorePreviousMediaChanges
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
RequestProcessSerializedIoctl(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFREQUEST               Request
    );

NTSTATUS
RequestIsIoctlBlockedByExclusiveAccess(
    _In_  WDFREQUEST  Request,
    _Out_ PBOOLEAN    IsBlocked
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceSendRequestSynchronously(
    _In_ WDFDEVICE    Device,
    _In_ WDFREQUEST   Request,
    _In_ BOOLEAN      RequestFormated
    );

VOID
DeviceSendIoctlAsynchronously(
    _In_ PCDROM_DEVICE_EXTENSION    DeviceExtension,
    _In_ ULONG                      IoControlCode,
    _In_ PDEVICE_OBJECT             TargetDeviceObject
    );

IO_COMPLETION_ROUTINE RequestAsynchronousIrpCompletion;

//  MMC Update functions

BOOLEAN
DeviceIsMmcUpdateRequired(
    _In_ WDFDEVICE    Device
    );

// Helper functions

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceEnableMediaChangeDetection(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PFILE_OBJECT_CONTEXT    FileObjectContext,
    _In_    BOOLEAN                 IgnorePreviousMediaChanges
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceDisableMediaChangeDetection(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PFILE_OBJECT_CONTEXT    FileObjectContext
    );

NTSTATUS
RequestSetContextFields(
    _In_ WDFREQUEST    Request,
    _In_ PSYNC_HANDLER Handler
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
DeviceIsPlayActive(
    _In_ WDFDEVICE Device
    );

NTSTATUS
RequestDuidGetDeviceIdProperty(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ WDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_ size_t *               DataLength
    );

NTSTATUS
RequestDuidGetDeviceProperty(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ WDF_REQUEST_PARAMETERS  RequestParameters,
    _Out_ size_t *               DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
ULONG
DeviceRetrieveModeSenseUsingScratch(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_reads_bytes_(Length) PCHAR     ModeSenseBuffer,
    _In_ ULONG                    Length,
    _In_ UCHAR                    PageCode,
    _In_ UCHAR                    PageControl
    );

_IRQL_requires_max_(APC_LEVEL)
PVOID
ModeSenseFindSpecificPage(
    _In_reads_bytes_(Length) PCHAR   ModeSenseBuffer,
    _In_ size_t                 Length,
    _In_ UCHAR                  PageMode,
    _In_ BOOLEAN                Use6Byte
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
PerformEjectionControl(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ WDFREQUEST               Request,
    _In_ MEDIA_LOCK_TYPE          LockType,
    _In_ BOOLEAN                  Lock
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceDisableMainTimer(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DeviceEnableMainTimer(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

EVT_WDF_TIMER DeviceMainTimerTickHandler;

VOID
RequestSetupMcnSyncIrp(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
RequestSetupMcnRequest(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_ BOOLEAN                  UseGesn
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
RequestSendMcnRequest(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
BOOLEAN
RequestPostWorkMcnRequest(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextReuseRequest(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextBeginUse(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
PowerContextEndUse(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

POWER_SETTING_CALLBACK DevicePowerSettingCallback;

//  Zero Power ODD functions

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
DeviceInitializeZPODD(
    _In_ PCDROM_DEVICE_EXTENSION  DeviceExtension
    );

_IRQL_requires_max_(APC_LEVEL)
VOID
DeviceReleaseZPODDResources(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

NTSTATUS
DeviceZPODDGetPowerupReason(
    _In_    PCDROM_DEVICE_EXTENSION         DeviceExtension,
    _Out_   PSTORAGE_IDLE_POWERUP_REASON    PowerupReason
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
DeviceZPODDIsInHomePosition(
    _In_    PCDROM_DEVICE_EXTENSION DeviceExtension
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
DeviceMarkActive(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ BOOLEAN                 IsActive,
    _In_ BOOLEAN                 SetIdleTimeout
    );

// common routines for specific IOCTL process

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DvdStartSessionReadKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_  ULONG                    IoControlCode,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_opt_  PVOID                InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ReadDvdStructure(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
ReadQChannel(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _In_  PVOID                    OutputBuffer,
    _In_  size_t                   OutputBufferLength,
    _Out_ size_t *                 DataLength
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
DvdSendKey(
    _In_  PCDROM_DEVICE_EXTENSION  DeviceExtension,
    _In_opt_  WDFREQUEST           OriginalRequest,
    _In_  PVOID                    InputBuffer,
    _In_  size_t                   InputBufferLength,
    _Out_ size_t *                 DataLength
    );


#ifndef SIZEOF_ARRAY
    #define SIZEOF_ARRAY(ar)        (sizeof(ar)/sizeof((ar)[0]))
#endif // !defined(SIZEOF_ARRAY)


// 100us to seconds
#define UNIT_100NS_PER_SECOND (10*1000*1000)
#define SECONDS_TO_100NS_UNITS(x) (((LONGLONG)x) * UNIT_100NS_PER_SECOND)

//
// Bit Flag Macros
//
#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

//
// neat little hacks to count number of bits set efficiently
//
FORCEINLINE ULONG CountOfSetBitsUChar(UCHAR _X)
                    { ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
FORCEINLINE ULONG CountOfSetBitsULong(ULONG _X)
                    { ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
FORCEINLINE ULONG CountOfSetBitsULong32(ULONG32 _X)
                    { ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
FORCEINLINE ULONG CountOfSetBitsULong64(ULONG64 _X)
                    { ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }
FORCEINLINE ULONG CountOfSetBitsUlongPtr(ULONG_PTR _X)
                    { ULONG i = 0; while (_X) { _X &= _X - 1; i++; } return i; }


FORCEINLINE // __REACTOS__
BOOLEAN
IsVolumeMounted(
    _In_ PDEVICE_OBJECT DeviceObject
    )
{
#pragma prefast(push)
#pragma prefast(disable: 28175, "there is no other way to check if there is volume mounted")
    return (DeviceObject->Vpb != NULL) &&
           ((DeviceObject->Vpb->Flags & VPB_MOUNTED) != 0);
#pragma prefast(pop)
}


FORCEINLINE _Ret_range_(0,MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS)
LONGLONG
ConvertSectorsPerSecondTo100nsUnitsFor64kWrite(
    _In_range_(1,0xFFFFFFFF) ULONG SectorsPerSecond // zero would cause divide-by-zero
    )
{
    //     64k write
    // ---------------- == time per 64k write
    //  sectors/second
    //
    //  (32 / N) == seconds
    //  (32 / N) * (1,000,000,000) == nanoseconds
    //  (32 / N) * (1,000,000,000) / 100 == 100ns increments
    //  (32 * 10,000,000) / N == 100ns increments
    //
    //  320,000,000 / N == 100ns increments
    //  And this is safe to run in kernel-mode (no floats)
    //

    // this assert ensures that we _never_ can return a value
    // larger than the maximum allowed.
    C_ASSERT(320000000 < MAXIMUM_RETRY_FOR_SINGLE_IO_IN_100NS_UNITS);

    return 320000000 / SectorsPerSecond;
}

FORCEINLINE // __REACTOS__
UCHAR
RequestGetCurrentStackLocationFlags(
    _In_ WDFREQUEST Request
    )
{
    PIRP                irp = NULL;
    PIO_STACK_LOCATION  currentStack = NULL;

    irp = WdfRequestWdmGetIrp(Request);
    currentStack = IoGetCurrentIrpStackLocation(irp);

    return currentStack->Flags;
}

FORCEINLINE // __REACTOS__
ULONG
TimeOutValueGetCapValue(
    _In_ ULONG  TimeOutValue,
    _In_ ULONG  Times
    )
{
    ULONG   value = 0;

    if (TimeOutValue > SCSI_CDROM_OPC_TIMEOUT)
    {
        // if time out value is specified by user in registry, and is
        // bigger than OPC time out, it should be big enough
        value = TimeOutValue;
    }
    else
    {
        // otherwise, OPC time out value should be the upper limit
        value = min(TimeOutValue * Times, SCSI_CDROM_OPC_TIMEOUT);
    }

    return value;
}

VOID
RequestCompletion(
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_ WDFREQUEST              Request,
    _In_ NTSTATUS                Status,
    _In_ ULONG_PTR               Information
    );

NTSTATUS
RequestSend(
    _In_        PCDROM_DEVICE_EXTENSION DeviceExtension,
    _In_        WDFREQUEST              Request,
    _In_        WDFIOTARGET             IoTarget,
    _In_        ULONG                   Flags,
    _Out_opt_   PBOOLEAN                RequestSent
    );

EVT_WDF_REQUEST_COMPLETION_ROUTINE RequestDummyCompletionRoutine;

VOID
RequestProcessInternalDeviceControl(
    _In_ WDFREQUEST              Request,
    _In_ PCDROM_DEVICE_EXTENSION DeviceExtension
    );

EVT_WDF_WORKITEM IoctlWorkItemRoutine;

EVT_WDF_WORKITEM ReadWriteWorkItemRoutine;

#pragma warning(pop) // un-sets any local warning changes

#endif // __CDROMP_H__


