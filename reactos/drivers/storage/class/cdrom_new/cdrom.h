/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    cdromp.h

Abstract:

    Private header file for cdrom.sys.  This contains private
    structure and function declarations as well as constant
    values which do not need to be exported.

Author:

Environment:

    kernel mode only

Notes:


Revision History:

--*/

#ifndef __CDROMP_H__
#define __CDROMP_H__

#include <ntddk.h>
#include <ntddcdvd.h>
#include <classpnp.h>
#include <ntddmmc.h>

#include "trace.h"

extern CLASSPNP_SCAN_FOR_SPECIAL_INFO CdromHackItems[];

typedef enum {
    CdromDebugError     = 0,  // always printed
    CdromDebugWarning   = 1,  // set bit 0x00000001 in nt!kd_cdrom_mask
    CdromDebugTrace     = 2,  // set bit 0x00000002 in nt!kd_cdrom_mask
    CdromDebugInfo      = 3,  // set bit 0x00000004 in nt!kd_cdrom_mask
#if 0
    CdromDebug          = z,  // set bit 0x00000000 in nt!kd_cdrom_mask
    CdromDebug          = z,  // set bit 0x00000000 in nt!kd_cdrom_mask
    CdromDebug          = z,  // set bit 0x00000000 in nt!kd_cdrom_mask
    CdromDebug          = z,  // set bit 0x00000000 in nt!kd_cdrom_mask
#endif
    CdromDebugFeatures  = 32  // set bit 0x80000000 in nt!kd_cdrom_mask
}CdromError;

#define CDROM_GET_CONFIGURATION_TIMEOUT    (0x4)

#define CDROM_HACK_DEC_RRD                 (0x00000001)
#define CDROM_HACK_FUJITSU_FMCD_10x        (0x00000002)
#define CDROM_HACK_HITACHI_1750            (0x00000004)
#define CDROM_HACK_HITACHI_GD_2000         (0x00000008)
#define CDROM_HACK_TOSHIBA_SD_W1101        (0x00000010)
#define CDROM_HACK_TOSHIBA_XM_3xx          (0x00000020)
#define CDROM_HACK_NEC_CDDA                (0x00000040)
#define CDROM_HACK_PLEXTOR_CDDA            (0x00000080)
#define CDROM_HACK_BAD_GET_CONFIG_SUPPORT  (0x00000100)
#define CDROM_HACK_FORCE_READ_CD_DETECTION (0x00000200)
#define CDROM_HACK_READ_CD_SUPPORTED       (0x00000400)
#define CDROM_HACK_LOCKED_PAGES            (0x80000000) // not a valid flag to save
                                     
#define CDROM_HACK_VALID_FLAGS             (0x000007ff)
#define CDROM_HACK_INVALID_FLAGS           (~CDROM_HACK_VALID_FLAGS)


typedef struct _XA_CONTEXT {

    //
    // Pointer to the device object.
    //

    PDEVICE_OBJECT DeviceObject;

    //
    // Pointer to the original request when
    // a mode select must be sent.
    //

    PIRP OriginalRequest;

    //
    // Pointer to the mode select srb.
    //

    PSCSI_REQUEST_BLOCK Srb;
} XA_CONTEXT, *PXA_CONTEXT;

typedef struct _ERROR_RECOVERY_DATA {
    MODE_PARAMETER_HEADER   Header;
    MODE_PARAMETER_BLOCK BlockDescriptor;
    MODE_READ_RECOVERY_PAGE ReadRecoveryPage;
} ERROR_RECOVERY_DATA, *PERROR_RECOVERY_DATA;

typedef struct _ERROR_RECOVERY_DATA10 {
    MODE_PARAMETER_HEADER10 Header10;
    MODE_PARAMETER_BLOCK BlockDescriptor10;
    MODE_READ_RECOVERY_PAGE ReadRecoveryPage10;
} ERROR_RECOVERY_DATA10, *PERROR_RECOVERY_DATA10;

//
// CdRom specific addition to device extension.
//

typedef struct _CDROM_DRIVER_EXTENSION {
    ULONG InterlockedCdRomCounter;
    PVOID Reserved[3];
} CDROM_DRIVER_EXTENSION, *PCDROM_DRIVER_EXTENSION;

#define CdromMmcUpdateComplete 0
#define CdromMmcUpdateRequired 1
#define CdromMmcUpdateStarted  2

typedef struct _CDROM_MMC_EXTENSION {

    ULONG        IsMmc;        // allow quick checks
    ULONG        WriteAllowed;
           
    LONG         UpdateState;
    
    SLIST_HEADER DelayedIrps;  // irps delayed due to 
    KSPIN_LOCK   DelayedLock;  // lock for delayed irps
    
    PIO_WORKITEM              CapabilitiesWorkItem;
    PIRP                      CapabilitiesIrp;
    PMDL                      CapabilitiesMdl;
    PGET_CONFIGURATION_HEADER CapabilitiesBuffer;
    ULONG                     CapabilitiesBufferSize;
    KEVENT                    CapabilitiesEvent;
    SCSI_REQUEST_BLOCK        CapabilitiesSrb;

} CDROM_MMC_EXTENSION, *PCDROM_MMC_EXTENSION;


#define CDROM_DRIVER_EXTENSION_ID CdRomAddDevice

typedef struct _CDROM_DATA {

    //
    // Pointer to the cdrom driver extension
    //

    PCDROM_DRIVER_EXTENSION DriverExtension;


    //
    // These bits allow detection of when to requery the
    // drive's capabilities.
    //

    CDROM_MMC_EXTENSION Mmc;

    //
    // hack flags for ScanForSpecial routines
    //

    ULONG_PTR HackFlags;

    //
    // the error handling routines need to be per-device,
    // not per-driver....
    //

    PCLASS_ERROR ErrorHandler;

    //
    // Indicates whether an audio play operation
    // is currently being performed.
    // Only thing this does is prevent reads and
    // toc requests while playing audio.
    //

    BOOLEAN PlayActive;

    //
    // Indicates whether the blocksize used for user data
    // is 2048 or 2352.
    //

    BOOLEAN RawAccess;

    //
    // Indicates that this is a DEC RRD cdrom.
    // This drive requires software to fix responses
    // from the faulty firmware
    //

    BOOLEAN IsDecRrd;

    //
    // This points to an irp which needs to be delayed for a bit before a
    // retry can be attempted.  The interval counter is set by the deferring
    // routine and will be decremented to zero in the tick handler.  Once
    // the counter goes to zero the irp will be issued again.
    // DelayedRetryResend controls whether the irp is resent to the lower
    // driver (TRUE) or reissued into the startio routine (FALSE)
    //

    BOOLEAN DelayedRetryResend;

    PIRP DelayedRetryIrp;

    ULONG DelayedRetryInterval;

    KSPIN_LOCK DelayedRetrySpinLock;

    //
    // indicate we need to pick a default dvd region
    // for the user if we can
    //

    ULONG PickDvdRegion;

    //
    // The interface strings registered for this device.
    //

    UNICODE_STRING CdromInterfaceString;
    UNICODE_STRING VolumeInterfaceString;

    //
    // The well known name link for this device.
    //

    UNICODE_STRING WellKnownName;

    //
    // Indicates whether 6 or 10 bytes mode sense/select
    // should be used
    //

    ULONG XAFlags;

    //
    // keep track of what type of DVD device we are
    //

    BOOLEAN DvdRpc0Device;
    BOOLEAN DvdRpc0LicenseFailure;
    UCHAR   Rpc0SystemRegion;           // bitmask, one means prevent play
    UCHAR   Rpc0SystemRegionResetCount;

    ULONG   Rpc0RetryRegistryCallback;   // one until initial region chosen

    KMUTEX  Rpc0RegionMutex;

    //
    // Storage for the error recovery page. This is used
    // as an easy method to switch block sizes.
    //
    // NOTE - doubly unnamed structs just aren't very clean looking code - this
    // should get cleaned up at some point in the future.
    //

    union {
        ERROR_RECOVERY_DATA;
        ERROR_RECOVERY_DATA10;
    };

} CDROM_DATA, *PCDROM_DATA;

#define DEVICE_EXTENSION_SIZE sizeof(FUNCTIONAL_DEVICE_EXTENSION) + sizeof(CDROM_DATA)
#define SCSI_CDROM_TIMEOUT          10
#define SCSI_CHANGER_BONUS_TIMEOUT  10
#define HITACHI_MODE_DATA_SIZE      12
#define MODE_DATA_SIZE              64
#define RAW_SECTOR_SIZE           2352
#define COOKED_SECTOR_SIZE        2048
#define CDROM_SRB_LIST_SIZE          4

#define PLAY_ACTIVE(x) (((PCDROM_DATA)(x->CommonExtension.DriverData))->PlayActive)

#define MSF_TO_LBA(Minutes,Seconds,Frames) \
                (ULONG)((60 * 75 * (Minutes)) + (75 * (Seconds)) + ((Frames) - 150))

#define LBA_TO_MSF(Lba,Minutes,Seconds,Frames)               \
{                                                            \
    (Minutes) = (UCHAR)(Lba  / (60 * 75));                   \
    (Seconds) = (UCHAR)((Lba % (60 * 75)) / 75);             \
    (Frames)  = (UCHAR)((Lba % (60 * 75)) % 75);             \
}

#define DEC_TO_BCD(x) (((x / 10) << 4) + (x % 10))

//
// Define flags for XA, CDDA, and Mode Select/Sense
//

#define XA_USE_6_BYTE             0x01
#define XA_USE_10_BYTE            0x02

#define XA_NOT_SUPPORTED          0x10
#define XA_USE_READ_CD            0x20
#define XA_PLEXTOR_CDDA           0x40
#define XA_NEC_CDDA               0x80

//
// Sector types for READ_CD
//

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

#define CDROM_TAG_GET_CONFIG    'cCcS'  // "ScCc" - ioctl GET_CONFIGURATION
#define CDROM_TAG_DC_EVENT      'ECcS'  // "ScCE" - device control synch event
#define CDROM_TAG_FEATURE       'FCcS'  // "ScCF" - allocated by CdRomGetConfiguration(), free'd by caller
#define CDROM_TAG_DISK_GEOM     'GCcS'  // "ScCG" - disk geometry buffer
#define CDROM_TAG_HITACHI_ERROR 'HCcS'  // "ScCH" - hitachi error buffer
#define CDROM_TAG_SENSE_INFO    'ICcS'  // "ScCI" - sense info buffers
#define CDROM_TAG_POWER_IRP     'iCcS'  // "ScCi" - irp for power request
#define CDROM_TAG_SRB           'SCcS'  // "ScCS" - srb allocation
#define CDROM_TAG_STRINGS       'sCcS'  // "ScCs" - assorted string data
#define CDROM_TAG_MODE_DATA     'MCcS'  // "ScCM" - mode data buffer
#define CDROM_TAG_READ_CAP      'PCcS'  // "ScCP" - read capacity buffer
#define CDROM_TAG_PLAY_ACTIVE   'pCcS'  // "ScCp" - play active checks
#define CDROM_TAG_SUB_Q         'QCcS'  // "ScCQ" - read sub q buffer
#define CDROM_TAG_RAW           'RCcS'  // "ScCR" - raw mode read buffer
#define CDROM_TAG_TOC           'TCcS'  // "ScCT" - read toc buffer
#define CDROM_TAG_TOSHIBA_ERROR 'tCcS'  // "ScCt" - toshiba error buffer
#define CDROM_TAG_DEC_ERROR     'dCcS'  // "ScCt" - DEC error buffer
#define CDROM_TAG_UPDATE_CAP    'UCcS'  // "ScCU" - update capacity path
#define CDROM_TAG_VOLUME        'VCcS'  // "ScCV" - volume control buffer
#define CDROM_TAG_VOLUME_INT    'vCcS'  // "ScCv" - volume control buffer

#define DVD_TAG_READ_STRUCTURE  'SVcS'  // "ScVS" - used for dvd structure reads
#define DVD_TAG_READ_KEY        'kVcS'  // "ScVk" - read buffer for dvd key
#define DVD_TAG_SEND_KEY        'KVcS'  // "ScVK" - write buffer for dvd key
#define DVD_TAG_RPC2_CHECK      'sVcS'  // "ScVs" - read buffer for dvd/rpc2 check
#define DVD_TAG_DVD_REGION      'tVcS'  // "ScVt" - read buffer for rpc2 check
#define DVD_TAG_SECURITY        'XVcS' // "ScVX" - security descriptor


#define CDROM_SUBKEY_NAME        (L"CdRom")  // store new settings here
#define CDROM_READ_CD_NAME       (L"ReadCD") // READ_CD support previously detected
#define CDROM_NON_MMC_DRIVE_NAME (L"NonMmc") // MMC commands hang
//
// DVD Registry Value Names for RPC0 Device
//
#define DVD_DEFAULT_REGION       (L"DefaultDvdRegion")    // this is init. by the dvd class installer
#define DVD_CURRENT_REGION       (L"DvdR")
#define DVD_REGION_RESET_COUNT   (L"DvdRCnt")
#define DVD_MAX_REGION_RESET_COUNT  2
#define DVD_MAX_REGION              8

#define BAIL_OUT(Irp) \
    DebugPrint((2, "Cdrom: [%p] Bailing with status " \
                " %lx at line %x file %s\n",          \
                (Irp), (Irp)->IoStatus.Status,        \
                __LINE__, __FILE__))


/*++

Routine Description:

    This routine grabs an extra remove lock using a local variable
    for a unique tag.  It then completes the irp in question, and
    the just-acquired removelock guarantees that it is still safe
    to call IoStartNextPacket().  When that finishes, we release
    the newly acquired RemoveLock and return.

Arguments:

    DeviceObject - the device object for the StartIo queue
    Irp - the request we are completing

Return Value:

    None

Notes:

    This is implemented as an inline function to allow the compiler
    to optimize this as either a function call or as actual inline code.
    
    This routine will not work with IoXxxRemoveLock() calls, as the
    behavior is different.  ClassXxxRemoveLock() calls succeed until
    the remove has completed, while IoXxxRemoveLock() calls fail as
    soon as the call to IoReleaseRemoveLockAndWait() has been called.
    The Class version allows this routine to work in a safe manner.
    
    replaces the following two lines:
        IoStartNextPacket(DeviceObject, FALSE);
        ClassReleaseRemoveLock(DeviceObject, Irp);
    and raises irql as needed to call IoStartNextPacket()

--*/
static inline
VOID
CdRomCompleteIrpAndStartNextPacketSafely(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    UCHAR uniqueAddress;
    KIRQL oldIrql = KeGetCurrentIrql();
    
    ClassAcquireRemoveLock(DeviceObject, (PIRP)&uniqueAddress);
    ClassReleaseRemoveLock(DeviceObject, Irp);
    ClassCompleteRequest(DeviceObject, Irp, IO_CD_ROM_INCREMENT);
    
    if (oldIrql > DISPATCH_LEVEL) {
        ASSERT(!"Cannot call IoStartNextPacket at raised IRQL!");
    } else if (oldIrql < DISPATCH_LEVEL) {
        KeRaiseIrqlToDpcLevel();
    } else { //  (oldIrql == DISPATCH_LEVEL)
        NOTHING;
    }

    IoStartNextPacket(DeviceObject, FALSE);
    
    if (oldIrql > DISPATCH_LEVEL) {
        ASSERT(!"Cannot call IoStartNextPacket at raised IRQL!");
    } else if (oldIrql < DISPATCH_LEVEL) {
        KeLowerIrql(oldIrql);
    } else { //  (oldIrql == DISPATCH_LEVEL)
        NOTHING;
    }

    ClassReleaseRemoveLock(DeviceObject, (PIRP)&uniqueAddress);


    return;
}

VOID
NTAPI
CdRomDeviceControlDvdReadStructure(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
NTAPI
CdRomDeviceControlDvdEndSession(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
NTAPI
CdRomDeviceControlDvdStartSessionReadKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

VOID
NTAPI
CdRomDeviceControlDvdSendKey(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP OriginalIrp,
    IN PIRP NewIrp,
    IN PSCSI_REQUEST_BLOCK Srb
    );

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NTAPI
CdRomUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
NTAPI
CdRomAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
NTAPI
CdRomOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NTAPI
CdRomReadWriteVerification(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NTAPI
CdRomSwitchMode(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG SectorSize,
    IN PIRP  OriginalRequest
    );

NTSTATUS
NTAPI
CdRomDeviceControlDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NTAPI
CdRomDeviceControlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NTAPI
CdRomSetVolumeIntermediateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NTAPI
CdRomSwitchModeCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NTAPI
CdRomXACompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NTAPI
CdRomClassIoctlCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
NTAPI
CdRomStartIo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NTAPI
CdRomTickHandler(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NTAPI
CdRomUpdateCapacity(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN PIRP IrpToComplete,
    IN OPTIONAL PKEVENT IoctlEvent
    );

NTSTATUS
NTAPI
CdRomCreateDeviceObject(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT Pdo
    );

VOID
NTAPI
ScanForSpecialHandler(
    PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    ULONG_PTR HackFlags
    );

VOID
NTAPI
ScanForSpecial(
    PDEVICE_OBJECT DeviceObject
    );

BOOLEAN
NTAPI
CdRomIsPlayActive(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
NTAPI
CdRomErrorHandler(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

VOID
NTAPI
HitachiProcessErrorGD2000(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

VOID
NTAPI
HitachiProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

VOID
NTAPI
ToshibaProcessError(
    PDEVICE_OBJECT DeviceObject,
    PSCSI_REQUEST_BLOCK Srb,
    NTSTATUS *Status,
    BOOLEAN *Retry
    );

NTSTATUS
NTAPI
ToshibaProcessErrorCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    );

VOID
NTAPI
CdRomCreateNamedEvent(
    IN PFUNCTIONAL_DEVICE_EXTENSION DeviceExtension,
    IN ULONG DeviceNumber
    );

NTSTATUS
NTAPI
CdRomInitDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
NTAPI
CdRomStartDevice(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
NTAPI
CdRomStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
NTAPI
CdRomRemoveDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR Type
    );

NTSTATUS
NTAPI
CdRomDvdEndAllSessionsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
NTAPI
CdRomDvdReadDiskKeyCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

DEVICE_TYPE
NTAPI
CdRomGetDeviceType(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NTAPI
CdRomCreateWellKnownName(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
NTAPI
CdRomDeleteWellKnownName(
    IN PDEVICE_OBJECT DeviceObject
    );

NTSTATUS
NTAPI
CdRomGetDeviceParameter (
    IN     PDEVICE_OBJECT      DeviceObject,
    IN     PWSTR               ParameterName,
    IN OUT PULONG              ParameterValue
    );

NTSTATUS
NTAPI
CdRomSetDeviceParameter (
    IN PDEVICE_OBJECT DeviceObject,
    IN PWSTR          ParameterName,
    IN ULONG          ParameterValue
    );

VOID
NTAPI
CdRomPickDvdRegion (
    IN PDEVICE_OBJECT Fdo
);

NTSTATUS
NTAPI
CdRomRetryRequest(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN PIRP Irp,
    IN ULONG Delay,
    IN BOOLEAN ResendIrp
    );

NTSTATUS
NTAPI
CdRomRerunRequest(
    IN PFUNCTIONAL_DEVICE_EXTENSION FdoExtension,
    IN OPTIONAL PIRP Irp,
    IN BOOLEAN ResendIrp
    );

NTSTATUS
NTAPI
CdRomGetRpc0Settings(
    IN PDEVICE_OBJECT Fdo
    );

NTSTATUS
NTAPI
CdRomSetRpc0Settings(
    IN PDEVICE_OBJECT Fdo,
    IN UCHAR NewRegion
    );

NTSTATUS
NTAPI
CdRomShutdownFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

////////////////////////////////////////////////////////////////////////////////

VOID
NTAPI
CdRomIsDeviceMmcDevice(
    IN PDEVICE_OBJECT Fdo,
    OUT PBOOLEAN IsMmc
    );

VOID
NTAPI
CdRomMmcErrorHandler(
    IN PDEVICE_OBJECT Fdo,
    IN PSCSI_REQUEST_BLOCK Srb,
    OUT PNTSTATUS Status,
    OUT PBOOLEAN Retry
    );

PVOID
NTAPI
CdRomFindFeaturePage(
    IN PGET_CONFIGURATION_HEADER FeatureBuffer,
    IN ULONG Length,
    IN FEATURE_NUMBER Feature
    );

NTSTATUS
NTAPI
CdRomGetConfiguration(
    IN PDEVICE_OBJECT Fdo,
    OUT PGET_CONFIGURATION_HEADER *Buffer,
    OUT PULONG BytesReturned,
    IN FEATURE_NUMBER StartingFeature,
    IN ULONG RequestedType
    );

VOID
NTAPI
CdRomUpdateMmcDriveCapabilities(
    IN PDEVICE_OBJECT Fdo,
    IN PVOID Context // RESERVED == NULL
    );

VOID
NTAPI
CdRomFindProfileInProfiles(
    IN PFEATURE_DATA_PROFILE_LIST ProfileHeader,
    IN FEATURE_PROFILE_TYPE ProfileToFind,
    OUT PBOOLEAN Exists
    );

NTSTATUS
NTAPI
CdRomAllocateMmcResources(
    IN PDEVICE_OBJECT Fdo
    );

VOID
NTAPI
CdRomDeAllocateMmcResources(
    IN PDEVICE_OBJECT Fdo
    );

VOID
NTAPI
CdromFakePartitionInfo(
    IN PCOMMON_DEVICE_EXTENSION CommonExtension,
    IN PIRP Irp
    );

VOID
NTAPI
CdRomInterpretReadCapacity(
    IN PDEVICE_OBJECT Fdo,
    IN PREAD_CAPACITY_DATA ReadCapacityBuffer
    );

NTSTATUS
NTAPI
CdRomShutdownFlushCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIRP Context
    );

VOID
NTAPI
CdRompFlushDelayedList(
    IN PDEVICE_OBJECT Fdo,
    IN PCDROM_MMC_EXTENSION MmcData,
    IN NTSTATUS Status,
    IN BOOLEAN CalledFromWorkItem
    );

#endif /* __CDROMP_H__ */
