/*
 * storport.h
 *
 * StorPort interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
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

#ifndef _NTSTORPORT_
#define _NTSTORPORT_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_STORPORT_)
#define STORPORT_API
#else
#define STORPORT_API DECLSPEC_IMPORT
#endif

#define DIRECT_ACCESS_DEVICE                0x00
#define SEQUENTIAL_ACCESS_DEVICE            0x01
#define PRINTER_DEVICE                      0x02
#define PROCESSOR_DEVICE                    0x03
#define WRITE_ONCE_READ_MULTIPLE_DEVICE     0x04
#define READ_ONLY_DIRECT_ACCESS_DEVICE      0x05
#define SCANNER_DEVICE                      0x06
#define OPTICAL_DEVICE                      0x07
#define MEDIUM_CHANGER                      0x08
#define COMMUNICATION_DEVICE                0x09
#define ARRAY_CONTROLLER_DEVICE             0x0C
#define SCSI_ENCLOSURE_DEVICE               0x0D
#define REDUCED_BLOCK_DEVICE                0x0E
#define OPTICAL_CARD_READER_WRITER_DEVICE   0x0F
#define BRIDGE_CONTROLLER_DEVICE            0x10
#define OBJECT_BASED_STORAGE_DEVICE         0x11
#define LOGICAL_UNIT_NOT_PRESENT_DEVICE     0x7F

#define DEVICE_CONNECTED                    0x00

#define CDB6GENERIC_LENGTH                  6
#define CDB10GENERIC_LENGTH                 10
#define CDB12GENERIC_LENGTH                 12

#define INQUIRYDATABUFFERSIZE                36
#define SENSE_BUFFER_SIZE                    18
#define MAX_SENSE_BUFFER_SIZE               255

#define FILE_DEVICE_SCSI 0x0000001b
#define IOCTL_SCSI_EXECUTE_IN   ((FILE_DEVICE_SCSI << 16) + 0x0011)
#define IOCTL_SCSI_EXECUTE_OUT  ((FILE_DEVICE_SCSI << 16) + 0x0012)
#define IOCTL_SCSI_EXECUTE_NONE ((FILE_DEVICE_SCSI << 16) + 0x0013)

#define MODE_PAGE_VENDOR_SPECIFIC           0x00
#define MODE_PAGE_ERROR_RECOVERY            0x01
#define MODE_PAGE_DISCONNECT                0x02
#define MODE_PAGE_FORMAT_DEVICE             0x03
#define MODE_PAGE_MRW                       0x03
#define MODE_PAGE_RIGID_GEOMETRY            0x04
#define MODE_PAGE_FLEXIBILE                 0x05
#define MODE_PAGE_WRITE_PARAMETERS          0x05
#define MODE_PAGE_VERIFY_ERROR              0x07
#define MODE_PAGE_CACHING                   0x08
#define MODE_PAGE_PERIPHERAL                0x09
#define MODE_PAGE_CONTROL                   0x0A
#define MODE_PAGE_MEDIUM_TYPES              0x0B
#define MODE_PAGE_NOTCH_PARTITION           0x0C
#define MODE_PAGE_CD_AUDIO_CONTROL          0x0E
#define MODE_PAGE_DATA_COMPRESS             0x0F
#define MODE_PAGE_DEVICE_CONFIG             0x10
#define MODE_PAGE_XOR_CONTROL               0x10
#define MODE_PAGE_MEDIUM_PARTITION          0x11
#define MODE_PAGE_ENCLOSURE_SERVICES_MANAGEMENT 0x14
#define MODE_PAGE_EXTENDED                  0x15
#define MODE_PAGE_EXTENDED_DEVICE_SPECIFIC  0x16
#define MODE_PAGE_CDVD_FEATURE_SET          0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_LUN     0x18
#define MODE_PAGE_PROTOCOL_SPECIFIC_PORT    0x19
#define MODE_PAGE_POWER_CONDITION           0x1A
#define MODE_PAGE_LUN_MAPPING               0x1B
#define MODE_PAGE_FAULT_REPORTING           0x1C
#define MODE_PAGE_CDVD_INACTIVITY           0x1D
#define MODE_PAGE_ELEMENT_ADDRESS           0x1D
#define MODE_PAGE_TRANSPORT_GEOMETRY        0x1E
#define MODE_PAGE_DEVICE_CAPABILITIES       0x1F
#define MODE_PAGE_CAPABILITIES              0x2A
#define MODE_SENSE_RETURN_ALL               0x3F

#define MODE_SENSE_CURRENT_VALUES           0x00
#define MODE_SENSE_CHANGEABLE_VALUES        0x40
#define MODE_SENSE_DEFAULT_VAULES           0x80
#define MODE_SENSE_SAVED_VALUES             0xc0

#define SCSIOP_TEST_UNIT_READY              0x00
#define SCSIOP_REZERO_UNIT                  0x01
#define SCSIOP_REWIND                       0x01
#define SCSIOP_REQUEST_BLOCK_ADDR           0x02
#define SCSIOP_REQUEST_SENSE                0x03
#define SCSIOP_FORMAT_UNIT                  0x04
#define SCSIOP_READ_BLOCK_LIMITS            0x05
#define SCSIOP_REASSIGN_BLOCKS              0x07
#define SCSIOP_INIT_ELEMENT_STATUS          0x07
#define SCSIOP_READ6                        0x08
#define SCSIOP_RECEIVE                      0x08
#define SCSIOP_WRITE6                       0x0A
#define SCSIOP_PRINT                        0x0A
#define SCSIOP_SEND                         0x0A
#define SCSIOP_SEEK6                        0x0B
#define SCSIOP_TRACK_SELECT                 0x0B
#define SCSIOP_SLEW_PRINT                   0x0B
#define SCSIOP_SET_CAPACITY                 0x0B
#define SCSIOP_SEEK_BLOCK                   0x0C
#define SCSIOP_PARTITION                    0x0D
#define SCSIOP_READ_REVERSE                 0x0F
#define SCSIOP_WRITE_FILEMARKS              0x10
#define SCSIOP_FLUSH_BUFFER                 0x10
#define SCSIOP_SPACE                        0x11
#define SCSIOP_INQUIRY                      0x12
#define SCSIOP_VERIFY6                      0x13
#define SCSIOP_RECOVER_BUF_DATA             0x14
#define SCSIOP_MODE_SELECT                  0x15
#define SCSIOP_RESERVE_UNIT                 0x16
#define SCSIOP_RELEASE_UNIT                 0x17
#define SCSIOP_COPY                         0x18
#define SCSIOP_ERASE                        0x19
#define SCSIOP_MODE_SENSE                   0x1A
#define SCSIOP_START_STOP_UNIT              0x1B
#define SCSIOP_STOP_PRINT                   0x1B
#define SCSIOP_LOAD_UNLOAD                  0x1B
#define SCSIOP_RECEIVE_DIAGNOSTIC           0x1C
#define SCSIOP_SEND_DIAGNOSTIC              0x1D
#define SCSIOP_MEDIUM_REMOVAL               0x1E
#define SCSIOP_READ_FORMATTED_CAPACITY      0x23
#define SCSIOP_READ_CAPACITY                0x25
#define SCSIOP_READ                         0x28
#define SCSIOP_WRITE                        0x2A
#define SCSIOP_SEEK                         0x2B
#define SCSIOP_LOCATE                       0x2B
#define SCSIOP_POSITION_TO_ELEMENT          0x2B
#define SCSIOP_WRITE_VERIFY                 0x2E
#define SCSIOP_VERIFY                       0x2F
#define SCSIOP_SEARCH_DATA_HIGH             0x30
#define SCSIOP_SEARCH_DATA_EQUAL            0x31
#define SCSIOP_SEARCH_DATA_LOW              0x32
#define SCSIOP_SET_LIMITS                   0x33
#define SCSIOP_READ_POSITION                0x34
#define SCSIOP_SYNCHRONIZE_CACHE            0x35
#define SCSIOP_COMPARE                      0x39
#define SCSIOP_COPY_COMPARE                 0x3A
#define SCSIOP_WRITE_DATA_BUFF              0x3B
#define SCSIOP_READ_DATA_BUFF               0x3C
#define SCSIOP_WRITE_LONG                   0x3F
#define SCSIOP_CHANGE_DEFINITION            0x40
#define SCSIOP_WRITE_SAME                   0x41
#define SCSIOP_READ_SUB_CHANNEL             0x42
#define SCSIOP_READ_TOC                     0x43
#define SCSIOP_READ_HEADER                  0x44
#define SCSIOP_REPORT_DENSITY_SUPPORT       0x44
#define SCSIOP_PLAY_AUDIO                   0x45
#define SCSIOP_GET_CONFIGURATION            0x46
#define SCSIOP_PLAY_AUDIO_MSF               0x47
#define SCSIOP_PLAY_TRACK_INDEX             0x48
#define SCSIOP_PLAY_TRACK_RELATIVE          0x49
#define SCSIOP_GET_EVENT_STATUS             0x4A
#define SCSIOP_PAUSE_RESUME                 0x4B
#define SCSIOP_LOG_SELECT                   0x4C
#define SCSIOP_LOG_SENSE                    0x4D
#define SCSIOP_STOP_PLAY_SCAN               0x4E
#define SCSIOP_XDWRITE                      0x50
#define SCSIOP_XPWRITE                      0x51
#define SCSIOP_READ_DISK_INFORMATION        0x51
#define SCSIOP_READ_DISC_INFORMATION        0x51
#define SCSIOP_READ_TRACK_INFORMATION       0x52
#define SCSIOP_XDWRITE_READ                 0x53
#define SCSIOP_RESERVE_TRACK_RZONE          0x53
#define SCSIOP_SEND_OPC_INFORMATION         0x54
#define SCSIOP_MODE_SELECT10                0x55
#define SCSIOP_RESERVE_UNIT10               0x56
#define SCSIOP_RESERVE_ELEMENT              0x56
#define SCSIOP_RELEASE_UNIT10               0x57
#define SCSIOP_RELEASE_ELEMENT              0x57
#define SCSIOP_REPAIR_TRACK                 0x58
#define SCSIOP_MODE_SENSE10                 0x5A
#define SCSIOP_CLOSE_TRACK_SESSION          0x5B
#define SCSIOP_READ_BUFFER_CAPACITY         0x5C
#define SCSIOP_SEND_CUE_SHEET               0x5D
#define SCSIOP_PERSISTENT_RESERVE_IN        0x5E
#define SCSIOP_PERSISTENT_RESERVE_OUT       0x5F
#define SCSIOP_XDWRITE_EXTENDED16           0x80
#define SCSIOP_WRITE_FILEMARKS16            0x80
#define SCSIOP_REBUILD16                    0x81
#define SCSIOP_READ_REVERSE16               0x81
#define SCSIOP_REGENERATE16                 0x82
#define SCSIOP_EXTENDED_COPY                0x83
#define SCSIOP_RECEIVE_COPY_RESULTS         0x84
#define SCSIOP_ATA_PASSTHROUGH16            0x85
#define SCSIOP_ACCESS_CONTROL_IN            0x86
#define SCSIOP_ACCESS_CONTROL_OUT           0x87
#define SCSIOP_READ16                       0x88
#define SCSIOP_WRITE16                      0x8A
#define SCSIOP_READ_ATTRIBUTES              0x8C
#define SCSIOP_WRITE_ATTRIBUTES             0x8D
#define SCSIOP_WRITE_VERIFY16               0x8E
#define SCSIOP_VERIFY16                     0x8F
#define SCSIOP_PREFETCH16                   0x90
#define SCSIOP_SYNCHRONIZE_CACHE16          0x91
#define SCSIOP_SPACE16                      0x91
#define SCSIOP_LOCK_UNLOCK_CACHE16          0x92
#define SCSIOP_LOCATE16                     0x92
#define SCSIOP_WRITE_SAME16                 0x93
#define SCSIOP_ERASE16                      0x93
#define SCSIOP_READ_CAPACITY16              0x9E
#define SCSIOP_SERVICE_ACTION_IN16          0x9E
#define SCSIOP_SERVICE_ACTION_OUT16         0x9F
#define SCSIOP_REPORT_LUNS                  0xA0
#define SCSIOP_BLANK                        0xA1
#define SCSIOP_ATA_PASSTHROUGH12            0xA1
#define SCSIOP_SEND_EVENT                   0xA2
#define SCSIOP_SEND_KEY                     0xA3
#define SCSIOP_MAINTENANCE_IN               0xA3
#define SCSIOP_REPORT_KEY                   0xA4
#define SCSIOP_MAINTENANCE_OUT              0xA4
#define SCSIOP_MOVE_MEDIUM                  0xA5
#define SCSIOP_LOAD_UNLOAD_SLOT             0xA6
#define SCSIOP_EXCHANGE_MEDIUM              0xA6
#define SCSIOP_SET_READ_AHEAD               0xA7
#define SCSIOP_MOVE_MEDIUM_ATTACHED         0xA7
#define SCSIOP_READ12                       0xA8
#define SCSIOP_GET_MESSAGE                  0xA8
#define SCSIOP_SERVICE_ACTION_OUT12         0xA9
#define SCSIOP_WRITE12                      0xAA
#define SCSIOP_SEND_MESSAGE                 0xAB
#define SCSIOP_SERVICE_ACTION_IN12          0xAB
#define SCSIOP_GET_PERFORMANCE              0xAC
#define SCSIOP_READ_DVD_STRUCTURE           0xAD
#define SCSIOP_WRITE_VERIFY12               0xAE
#define SCSIOP_VERIFY12                     0xAF
#define SCSIOP_SEARCH_DATA_HIGH12           0xB0
#define SCSIOP_SEARCH_DATA_EQUAL12          0xB1
#define SCSIOP_SEARCH_DATA_LOW12            0xB2
#define SCSIOP_SET_LIMITS12                 0xB3
#define SCSIOP_READ_ELEMENT_STATUS_ATTACHED 0xB4
#define SCSIOP_REQUEST_VOL_ELEMENT          0xB5
#define SCSIOP_SEND_VOLUME_TAG              0xB6
#define SCSIOP_SET_STREAMING                0xB6
#define SCSIOP_READ_DEFECT_DATA             0xB7
#define SCSIOP_READ_ELEMENT_STATUS          0xB8
#define SCSIOP_READ_CD_MSF                  0xB9
#define SCSIOP_SCAN_CD                      0xBA
#define SCSIOP_REDUNDANCY_GROUP_IN          0xBA
#define SCSIOP_SET_CD_SPEED                 0xBB
#define SCSIOP_REDUNDANCY_GROUP_OUT         0xBB
#define SCSIOP_PLAY_CD                      0xBC
#define SCSIOP_SPARE_IN                     0xBC
#define SCSIOP_MECHANISM_STATUS             0xBD
#define SCSIOP_SPARE_OUT                    0xBD
#define SCSIOP_READ_CD                      0xBE
#define SCSIOP_VOLUME_SET_IN                0xBE
#define SCSIOP_SEND_DVD_STRUCTURE           0xBF
#define SCSIOP_VOLUME_SET_OUT               0xBF
#define SCSIOP_INIT_ELEMENT_RANGE           0xE7

#define SCSISTAT_GOOD                       0x00
#define SCSISTAT_CHECK_CONDITION            0x02
#define SCSISTAT_CONDITION_MET              0x04
#define SCSISTAT_BUSY                       0x08
#define SCSISTAT_INTERMEDIATE               0x10
#define SCSISTAT_INTERMEDIATE_COND_MET      0x14
#define SCSISTAT_RESERVATION_CONFLICT       0x18
#define SCSISTAT_COMMAND_TERMINATED         0x22
#define SCSISTAT_QUEUE_FULL                 0x28

#define SETBITON                            1
#define SETBITOFF                           0

#define SP_RETURN_NOT_FOUND                 0
#define SP_RETURN_FOUND                     1
#define SP_RETURN_ERROR                     2
#define SP_RETURN_BAD_CONFIG                3

#define SRB_FUNCTION_EXECUTE_SCSI           0x00
#define SRB_FUNCTION_CLAIM_DEVICE           0x01
#define SRB_FUNCTION_IO_CONTROL             0x02
#define SRB_FUNCTION_RECEIVE_EVENT          0x03
#define SRB_FUNCTION_RELEASE_QUEUE          0x04
#define SRB_FUNCTION_ATTACH_DEVICE          0x05
#define SRB_FUNCTION_RELEASE_DEVICE         0x06
#define SRB_FUNCTION_SHUTDOWN               0x07
#define SRB_FUNCTION_FLUSH                  0x08
#define SRB_FUNCTION_ABORT_COMMAND          0x10
#define SRB_FUNCTION_RELEASE_RECOVERY       0x11
#define SRB_FUNCTION_RESET_BUS              0x12
#define SRB_FUNCTION_RESET_DEVICE           0x13
#define SRB_FUNCTION_TERMINATE_IO           0x14
#define SRB_FUNCTION_FLUSH_QUEUE            0x15
#define SRB_FUNCTION_REMOVE_DEVICE          0x16
#define SRB_FUNCTION_WMI                    0x17
#define SRB_FUNCTION_LOCK_QUEUE             0x18
#define SRB_FUNCTION_UNLOCK_QUEUE           0x19
#define SRB_FUNCTION_RESET_LOGICAL_UNIT     0x20
#define SRB_FUNCTION_SET_LINK_TIMEOUT       0x21
#define SRB_FUNCTION_LINK_TIMEOUT_OCCURRED  0x22
#define SRB_FUNCTION_LINK_TIMEOUT_COMPLETE  0x23
#define SRB_FUNCTION_POWER                  0x24
#define SRB_FUNCTION_PNP                    0x25
#define SRB_FUNCTION_DUMP_POINTERS          0x26

#define SRB_STATUS_PENDING                  0x00
#define SRB_STATUS_SUCCESS                  0x01
#define SRB_STATUS_ABORTED                  0x02
#define SRB_STATUS_ABORT_FAILED             0x03
#define SRB_STATUS_ERROR                    0x04
#define SRB_STATUS_BUSY                     0x05
#define SRB_STATUS_INVALID_REQUEST          0x06
#define SRB_STATUS_INVALID_PATH_ID          0x07
#define SRB_STATUS_NO_DEVICE                0x08
#define SRB_STATUS_TIMEOUT                  0x09
#define SRB_STATUS_SELECTION_TIMEOUT        0x0A
#define SRB_STATUS_COMMAND_TIMEOUT          0x0B
#define SRB_STATUS_MESSAGE_REJECTED         0x0D
#define SRB_STATUS_BUS_RESET                0x0E
#define SRB_STATUS_PARITY_ERROR             0x0F
#define SRB_STATUS_REQUEST_SENSE_FAILED     0x10
#define SRB_STATUS_NO_HBA                   0x11
#define SRB_STATUS_DATA_OVERRUN             0x12
#define SRB_STATUS_UNEXPECTED_BUS_FREE      0x13
#define SRB_STATUS_PHASE_SEQUENCE_FAILURE   0x14
#define SRB_STATUS_BAD_SRB_BLOCK_LENGTH     0x15
#define SRB_STATUS_REQUEST_FLUSHED          0x16
#define SRB_STATUS_INVALID_LUN              0x20
#define SRB_STATUS_INVALID_TARGET_ID        0x21
#define SRB_STATUS_BAD_FUNCTION             0x22
#define SRB_STATUS_ERROR_RECOVERY           0x23
#define SRB_STATUS_NOT_POWERED              0x24
#define SRB_STATUS_LINK_DOWN                0x25
#define SRB_STATUS_INTERNAL_ERROR           0x30
#define SRB_STATUS_QUEUE_FROZEN             0x40
#define SRB_STATUS_AUTOSENSE_VALID          0x80
#define SRB_STATUS(Status)                  (Status & ~(SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN))

#define SRB_FLAGS_QUEUE_ACTION_ENABLE       0x00000002
#define SRB_FLAGS_DISABLE_DISCONNECT        0x00000004
#define SRB_FLAGS_DISABLE_SYNCH_TRANSFER    0x00000008

#define SRB_FLAGS_BYPASS_FROZEN_QUEUE       0x00000010
#define SRB_FLAGS_DISABLE_AUTOSENSE         0x00000020
#define SRB_FLAGS_DATA_IN                   0x00000040
#define SRB_FLAGS_DATA_OUT                  0x00000080
#define SRB_FLAGS_NO_DATA_TRANSFER          0x00000000
#define SRB_FLAGS_UNSPECIFIED_DIRECTION     (SRB_FLAGS_DATA_IN | SRB_FLAGS_DATA_OUT)

#define SRB_FLAGS_NO_QUEUE_FREEZE           0x00000100
#define SRB_FLAGS_ADAPTER_CACHE_ENABLE      0x00000200
#define SRB_FLAGS_FREE_SENSE_BUFFER         0x00000400

#define SRB_FLAGS_IS_ACTIVE                 0x00010000
#define SRB_FLAGS_ALLOCATED_FROM_ZONE       0x00020000
#define SRB_FLAGS_SGLIST_FROM_POOL          0x00040000
#define SRB_FLAGS_BYPASS_LOCKED_QUEUE       0x00080000

#define SRB_FLAGS_NO_KEEP_AWAKE             0x00100000
#define SRB_FLAGS_PORT_DRIVER_ALLOCSENSE    0x00200000

#define SRB_FLAGS_PORT_DRIVER_SENSEHASPORT  0x00400000
#define SRB_FLAGS_DONT_START_NEXT_PACKET    0x00800000

#define SRB_FLAGS_PORT_DRIVER_RESERVED      0x0F000000
#define SRB_FLAGS_CLASS_DRIVER_RESERVED     0xF0000000

#define SRB_SIMPLE_TAG_REQUEST              0x20
#define SRB_HEAD_OF_QUEUE_TAG_REQUEST       0x21
#define SRB_ORDERED_QUEUE_TAG_REQUEST       0x22

#define SRB_WMI_FLAGS_ADAPTER_REQUEST       0x01
#define SRB_POWER_FLAGS_ADAPTER_REQUEST     0x01
#define SRB_PNP_FLAGS_ADAPTER_REQUEST       0x01

#define STOR_MAP_NO_BUFFERS                 (0)
#define STOR_MAP_ALL_BUFFERS                (1)
#define STOR_MAP_NON_READ_WRITE_BUFFERS     (2)

#define VPD_SUPPORTED_PAGES                 0x00
#define VPD_SERIAL_NUMBER                   0x80
#define VPD_DEVICE_IDENTIFIERS              0x83
#define VPD_MEDIA_SERIAL_NUMBER             0x84
#define VPD_SOFTWARE_INTERFACE_IDENTIFIERS  0x84
#define VPD_NETWORK_MANAGEMENT_ADDRESSES    0x85
#define VPD_EXTENDED_INQUIRY_DATA           0x86
#define VPD_MODE_PAGE_POLICY                0x87
#define VPD_SCSI_PORTS                      0x88

#define SCSI_SENSE_NO_SENSE                 0x00
#define SCSI_SENSE_RECOVERED_ERROR          0x01
#define SCSI_SENSE_NOT_READY                0x02
#define SCSI_SENSE_MEDIUM_ERROR             0x03
#define SCSI_SENSE_HARDWARE_ERROR           0x04
#define SCSI_SENSE_ILLEGAL_REQUEST          0x05
#define SCSI_SENSE_UNIT_ATTENTION           0x06
#define SCSI_SENSE_DATA_PROTECT             0x07
#define SCSI_SENSE_BLANK_CHECK              0x08
#define SCSI_SENSE_UNIQUE                   0x09
#define SCSI_SENSE_COPY_ABORTED             0x0A
#define SCSI_SENSE_ABORTED_COMMAND          0x0B
#define SCSI_SENSE_EQUAL                    0x0C
#define SCSI_SENSE_VOL_OVERFLOW             0x0D
#define SCSI_SENSE_MISCOMPARE               0x0E
#define SCSI_SENSE_RESERVED                 0x0F

typedef enum _STOR_SYNCHRONIZATION_MODEL
{
    StorSynchronizeHalfDuplex,
    StorSynchronizeFullDuplex
} STOR_SYNCHRONIZATION_MODEL;

typedef enum _STOR_DMA_WIDTH
{
    DmaUnknown,
    Dma32Bit,
    Dma64BitScatterGather,
    Dma64Bit
} STOR_DMA_WIDTH;

typedef enum _STOR_SPINLOCK
{
    DpcLock = 1,
    StartIoLock,
    InterruptLock
} STOR_SPINLOCK;

typedef enum _SCSI_ADAPTER_CONTROL_TYPE
{
    ScsiQuerySupportedControlTypes = 0,
    ScsiStopAdapter,
    ScsiRestartAdapter,
    ScsiSetBootConfig,
    ScsiSetRunningConfig,
    ScsiAdapterControlMax,
    MakeAdapterControlTypeSizeOfUlong = 0xffffffff
} SCSI_ADAPTER_CONTROL_TYPE, *PSCSI_ADAPTER_CONTROL_TYPE;

typedef enum _SCSI_ADAPTER_CONTROL_STATUS
{
    ScsiAdapterControlSuccess = 0,
    ScsiAdapterControlUnsuccessful
} SCSI_ADAPTER_CONTROL_STATUS, *PSCSI_ADAPTER_CONTROL_STATUS;

typedef enum _SCSI_NOTIFICATION_TYPE
{
    RequestComplete,
    NextRequest,
    NextLuRequest,
    ResetDetected,
    _obsolete1,
    _obsolete2,
    RequestTimerCall,
    BusChangeDetected,
    WMIEvent,
    WMIReregister,
    LinkUp,
    LinkDown,
    QueryTickCount,
    BufferOverrunDetected,
    TraceNotification,
    GetExtendedFunctionTable,
    EnablePassiveInitialization = 0x1000,
    InitializeDpc,
    IssueDpc,
    AcquireSpinLock,
    ReleaseSpinLock
} SCSI_NOTIFICATION_TYPE, *PSCSI_NOTIFICATION_TYPE;

typedef enum _STOR_DEVICE_POWER_STATE
{
    StorPowerDeviceUnspecified = 0,
    StorPowerDeviceD0,
    StorPowerDeviceD1,
    StorPowerDeviceD2,
    StorPowerDeviceD3,
    StorPowerDeviceMaximum
} STOR_DEVICE_POWER_STATE, *PSTOR_DEVICE_POWER_STATE;

typedef enum _STOR_POWER_ACTION
{
    StorPowerActionNone = 0,
    StorPowerActionReserved,
    StorPowerActionSleep,
    StorPowerActionHibernate,
    StorPowerActionShutdown,
    StorPowerActionShutdownReset,
    StorPowerActionShutdownOff,
    StorPowerActionWarmEject
} STOR_POWER_ACTION, *PSTOR_POWER_ACTION;

typedef enum _STOR_PNP_ACTION
{
    StorStartDevice = 0x0,
    StorRemoveDevice = 0x2,
    StorStopDevice = 0x4,
    StorQueryCapabilities = 0x9,
    StorQueryResourceRequirements = 0xB,
    StorFilterResourceRequirements = 0xD,
    StorSurpriseRemoval = 0x17
} STOR_PNP_ACTION, *PSTOR_PNP_ACTION;

typedef enum _VPD_CODE_SET
{
    VpdCodeSetReserved = 0,
    VpdCodeSetBinary = 1,
    VpdCodeSetAscii = 2,
    VpdCodeSetUTF8 = 3
} VPD_CODE_SET, *PVPD_CODE_SET;

typedef enum _VPD_ASSOCIATION
{
    VpdAssocDevice = 0,
    VpdAssocPort = 1,
    VpdAssocTarget = 2,
    VpdAssocReserved1 = 3,
    VpdAssocReserved2 = 4
} VPD_ASSOCIATION, *PVPD_ASSOCIATION;

typedef enum _VPD_IDENTIFIER_TYPE
{
    VpdIdentifierTypeVendorSpecific = 0,
    VpdIdentifierTypeVendorId = 1,
    VpdIdentifierTypeEUI64 = 2,
    VpdIdentifierTypeFCPHName = 3,
    VpdIdentifierTypePortRelative = 4,
    VpdIdentifierTypeTargetPortGroup = 5,
    VpdIdentifierTypeLogicalUnitGroup = 6,
    VpdIdentifierTypeMD5LogicalUnitId = 7,
    VpdIdentifierTypeSCSINameString = 8
} VPD_IDENTIFIER_TYPE, *PVPD_IDENTIFIER_TYPE;

typedef enum _STORPORT_FUNCTION_CODE
{
    ExtFunctionAllocatePool,
    ExtFunctionFreePool,
    ExtFunctionAllocateMdl,
    ExtFunctionFreeMdl,
    ExtFunctionBuildMdlForNonPagedPool,
    ExtFunctionGetSystemAddress,
    ExtFunctionGetOriginalMdl,
    ExtFunctionCompleteServiceIrp,
    ExtFunctionGetDeviceObjects,
    ExtFunctionBuildScatterGatherList,
    ExtFunctionPutScatterGatherList,
    ExtFunctionAcquireMSISpinLock,
    ExtFunctionReleaseMSISpinLock,
    ExtFunctionGetMessageInterruptInformation,
    ExtFunctionInitializePerformanceOptimizations,
    ExtFunctionGetStartIoPerformanceParameters,
    ExtFunctionLogSystemEvent,
#if (NTDDI_VERSION >= NTDDI_WIN7)
    ExtFunctionGetCurrentProcessorNumber,
    ExtFunctionGetActiveGroupCount,
    ExtFunctionGetGroupAffinity,
    ExtFunctionGetActiveNodeCount,
    ExtFunctionGetNodeAffinity,
    ExtFunctionGetHighestNodeNumber,
    ExtFunctionGetLogicalProcessorRelationship,
    ExtFunctionAllocateContiguousMemorySpecifyCacheNode,
    ExtFunctionFreeContiguousMemorySpecifyCache
#endif
} STORPORT_FUNCTION_CODE, *PSTORPORT_FUNCTION_CODE;

typedef enum _STOR_EVENT_ASSOCIATION_ENUM
{
    StorEventAdapterAssociation = 0,
    StorEventLunAssociation,
    StorEventTargetAssociation,
    StorEventInvalidAssociation
} STOR_EVENT_ASSOCIATION_ENUM;

typedef enum _GETSGSTATUS
{
    SG_ALLOCATED = 0,
    SG_BUFFER_TOO_SMALL
} GETSGSTATUS, *PGETSGSTATUS;

typedef struct _SCSI_REQUEST_BLOCK
{
    USHORT Length;
    UCHAR Function;
    UCHAR SrbStatus;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR QueueTag;
    UCHAR QueueAction;
    UCHAR CdbLength;
    UCHAR SenseInfoBufferLength;
    ULONG SrbFlags;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    PVOID SenseInfoBuffer;
    struct _SCSI_REQUEST_BLOCK *NextSrb;
    PVOID OriginalRequest;
    PVOID SrbExtension;
    union
    {
        ULONG InternalStatus;
        ULONG QueueSortKey;
        ULONG LinkTimeoutValue;
    };
#if defined(_WIN64)
    ULONG Reserved;
#endif
    UCHAR Cdb[16];
} SCSI_REQUEST_BLOCK, *PSCSI_REQUEST_BLOCK;

typedef struct _SCSI_WMI_REQUEST_BLOCK
{
    USHORT Length;
    UCHAR Function;
    UCHAR SrbStatus;
    UCHAR WMISubFunction;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR Reserved1;
    UCHAR WMIFlags;
    UCHAR Reserved2[2];
    ULONG SrbFlags;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    PVOID DataPath;
    PVOID Reserved3;
    PVOID OriginalRequest;
    PVOID SrbExtension;
    ULONG Reserved4;
#if (NTDDI_VERSION >= NTDDI_WS03SP1) && defined(_WIN64)
    ULONG Reserved6;
#endif
    UCHAR Reserved5[16];
} SCSI_WMI_REQUEST_BLOCK, *PSCSI_WMI_REQUEST_BLOCK;

typedef struct _SCSI_POWER_REQUEST_BLOCK
{
    USHORT Length;
    UCHAR Function;
    UCHAR SrbStatus;
    UCHAR SrbPowerFlags;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    STOR_DEVICE_POWER_STATE DevicePowerState;
    ULONG SrbFlags;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    PVOID SenseInfoBuffer;
    struct _SCSI_REQUEST_BLOCK *NextSrb;
    PVOID OriginalRequest;
    PVOID SrbExtension;
    STOR_POWER_ACTION PowerAction;
#if defined(_WIN64)
    ULONG Reserved;
#endif
    UCHAR Reserved5[16];
} SCSI_POWER_REQUEST_BLOCK, *PSCSI_POWER_REQUEST_BLOCK;

typedef struct _STOR_DEVICE_CAPABILITIES
{
    USHORT Version;
    ULONG DeviceD1:1;
    ULONG DeviceD2:1;
    ULONG LockSupported:1;
    ULONG EjectSupported:1;
    ULONG Removable:1;
    ULONG DockDevice:1;
    ULONG UniqueID:1;
    ULONG SilentInstall:1;
    ULONG SurpriseRemovalOK:1;
    ULONG NoDisplayInUI:1;
} STOR_DEVICE_CAPABILITIES, *PSTOR_DEVICE_CAPABILITIES;

typedef struct _SCSI_PNP_REQUEST_BLOCK
{
    USHORT Length;
    UCHAR Function;
    UCHAR SrbStatus;
    UCHAR PnPSubFunction;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    STOR_PNP_ACTION PnPAction;
    ULONG SrbFlags;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    PVOID SenseInfoBuffer;
    struct _SCSI_REQUEST_BLOCK *NextSrb;
    PVOID OriginalRequest;
    PVOID SrbExtension;
    ULONG SrbPnPFlags;
#if defined(_WIN64)
    ULONG Reserved;
#endif
    UCHAR Reserved4[16];
} SCSI_PNP_REQUEST_BLOCK, *PSCSI_PNP_REQUEST_BLOCK;

#include <pshpack1.h>
typedef union _CDB
{
    struct _CDB6GENERIC
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR CommandUniqueBits:4;
        UCHAR LogicalUnitNumber:3;
        UCHAR CommandUniqueBytes[3];
        UCHAR Link:1;
        UCHAR Flag:1;
        UCHAR Reserved:4;
        UCHAR VendorUnique:2;
    } CDB6GENERIC, *PCDB6GENERIC;
    struct _CDB6READWRITE
    {
        UCHAR OperationCode;
        UCHAR LogicalBlockMsb1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlockMsb0;
        UCHAR LogicalBlockLsb;
        UCHAR TransferBlocks;
        UCHAR Control;
    } CDB6READWRITE, *PCDB6READWRITE;
    struct _CDB6INQUIRY
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR PageCode;
        UCHAR IReserved;
        UCHAR AllocationLength;
        UCHAR Control;
    } CDB6INQUIRY, *PCDB6INQUIRY;
    struct _CDB6INQUIRY3
    {
        UCHAR OperationCode;
        UCHAR EnableVitalProductData:1;
        UCHAR CommandSupportData:1;
        UCHAR Reserved1:6;
        UCHAR PageCode;
        UCHAR Reserved2;
        UCHAR AllocationLength;
        UCHAR Control;
    } CDB6INQUIRY3, *PCDB6INQUIRY3;
    struct _CDB6VERIFY
    {
        UCHAR OperationCode;
        UCHAR Fixed:1;
        UCHAR ByteCompare:1;
        UCHAR Immediate:1;
        UCHAR Reserved:2;
        UCHAR LogicalUnitNumber:3;
        UCHAR VerificationLength[3];
        UCHAR Control;
    } CDB6VERIFY, *PCDB6VERIFY;
    struct _CDB6FORMAT
    {
        UCHAR OperationCode;
        UCHAR FormatControl:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR FReserved1;
        UCHAR InterleaveMsb;
        UCHAR InterleaveLsb;
        UCHAR FReserved2;
    } CDB6FORMAT, *PCDB6FORMAT;
    struct _CDB10
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:2;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlockByte0;
        UCHAR LogicalBlockByte1;
        UCHAR LogicalBlockByte2;
        UCHAR LogicalBlockByte3;
        UCHAR Reserved2;
        UCHAR TransferBlocksMsb;
        UCHAR TransferBlocksLsb;
        UCHAR Control;
    } CDB10, *PCDB10;
    struct _CDB12
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:2;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlock[4];
        UCHAR TransferLength[4];
        UCHAR Reserved2;
        UCHAR Control;
    } CDB12, *PCDB12;
    struct _CDB16
    {
        UCHAR OperationCode;
        UCHAR Reserved1:3;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR Protection:3;
        UCHAR LogicalBlock[8];
        UCHAR TransferLength[4];
        UCHAR Reserved2;
        UCHAR Control;
    } CDB16, *PCDB16;
    struct _PAUSE_RESUME
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[6];
        UCHAR Action;
        UCHAR Control;
    } PAUSE_RESUME, *PPAUSE_RESUME;
    struct _READ_TOC
    {
        UCHAR OperationCode;
        UCHAR Reserved0:1;
        UCHAR Msf:1;
        UCHAR Reserved1:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR Format2:4;
        UCHAR Reserved2:4;
        UCHAR Reserved3[3];
        UCHAR StartingTrack;
        UCHAR AllocationLength[2];
        UCHAR Control:6;
        UCHAR Format:2;
    } READ_TOC, *PREAD_TOC;
    struct _READ_DISK_INFORMATION
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR Reserved2[5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_DISK_INFORMATION, *PREAD_DISK_INFORMATION;
    struct _READ_TRACK_INFORMATION
    {
        UCHAR OperationCode;
        UCHAR Track:1;
        UCHAR Reserved1:3;
        UCHAR Reserved2:1;
        UCHAR Lun:3;
        UCHAR BlockAddress[4];
        UCHAR Reserved3;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_TRACK_INFORMATION, *PREAD_TRACK_INFORMATION;
    struct _RESERVE_TRACK_RZONE
    {
        UCHAR OperationCode;
        UCHAR Reserved1[4];
        UCHAR ReservationSize[4];
        UCHAR Control;
    } RESERVE_TRACK_RZONE, *PRESERVE_TRACK_RZONE;
    struct _SEND_OPC_INFORMATION
    {
        UCHAR OperationCode;
        UCHAR DoOpc:1;
        UCHAR Reserved1:7;
        UCHAR Exclude0:1;
        UCHAR Exclude1:1;
        UCHAR Reserved2:6;
        UCHAR Reserved3[4];
        UCHAR ParameterListLength[2];
        UCHAR Reserved4;
    } SEND_OPC_INFORMATION, *PSEND_OPC_INFORMATION;
    struct _REPAIR_TRACK
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved1:7;
        UCHAR Reserved2[2];
        UCHAR TrackNumber[2];
        UCHAR Reserved3[3];
        UCHAR Control;
    } REPAIR_TRACK, *PREPAIR_TRACK;
    struct _CLOSE_TRACK
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved1:7;
        UCHAR Track:1;
        UCHAR Session:1;
        UCHAR Reserved2:6;
        UCHAR Reserved3;
        UCHAR TrackNumber[2];
        UCHAR Reserved4[3];
        UCHAR Control;
    } CLOSE_TRACK, *PCLOSE_TRACK;
    struct _READ_BUFFER_CAPACITY
    {
        UCHAR OperationCode;
        UCHAR BlockInfo:1;
        UCHAR Reserved1:7;
        UCHAR Reserved2[5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_BUFFER_CAPACITY, *PREAD_BUFFER_CAPACITY;
    struct _SEND_CUE_SHEET
    {
        UCHAR OperationCode;
        UCHAR Reserved[5];
        UCHAR CueSheetSize[3];
        UCHAR Control;
    } SEND_CUE_SHEET, *PSEND_CUE_SHEET;
    struct _READ_HEADER
    {
        UCHAR OperationCode;
        UCHAR Reserved1:1;
        UCHAR Msf:1;
        UCHAR Reserved2:3;
        UCHAR Lun:3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved3;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_HEADER, *PREAD_HEADER;
    struct _PLAY_AUDIO
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR StartingBlockAddress[4];
        UCHAR Reserved2;
        UCHAR PlayLength[2];
        UCHAR Control;
    } PLAY_AUDIO, *PPLAY_AUDIO;
    struct _PLAY_AUDIO_MSF
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2;
        UCHAR StartingM;
        UCHAR StartingS;
        UCHAR StartingF;
        UCHAR EndingM;
        UCHAR EndingS;
        UCHAR EndingF;
        UCHAR Control;
    } PLAY_AUDIO_MSF, *PPLAY_AUDIO_MSF;
    struct _BLANK_MEDIA
    {
        UCHAR OperationCode;
        UCHAR BlankType:3;
        UCHAR Reserved1:1;
        UCHAR Immediate:1;
        UCHAR Reserved2:3;
        UCHAR AddressOrTrack[4];
        UCHAR Reserved3[5];
        UCHAR Control;
    } BLANK_MEDIA, *PBLANK_MEDIA;
    struct _PLAY_CD
    {
        UCHAR OperationCode;
        UCHAR Reserved1:1;
        UCHAR CMSF:1;
        UCHAR ExpectedSectorType:3;
        UCHAR Lun:3;
        _ANONYMOUS_UNION union
        {
            struct _LBA
            {
                UCHAR StartingBlockAddress[4];
                UCHAR PlayLength[4];
            } LBA;
            struct _MSF
            {
                UCHAR Reserved1;
                UCHAR StartingM;
                UCHAR StartingS;
                UCHAR StartingF;
                UCHAR EndingM;
                UCHAR EndingS;
                UCHAR EndingF;
                UCHAR Reserved2;
            } MSF;
        } DUMMYUNIONNAME;
        UCHAR Audio:1;
        UCHAR Composite:1;
        UCHAR Port1:1;
        UCHAR Port2:1;
        UCHAR Reserved2:3;
        UCHAR Speed:1;
        UCHAR Control;
    } PLAY_CD, *PPLAY_CD;
    struct _SCAN_CD
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:3;
        UCHAR Direct:1;
        UCHAR Lun:3;
        UCHAR StartingAddress[4];
        UCHAR Reserved2[3];
        UCHAR Reserved3:6;
        UCHAR Type:2;
        UCHAR Reserved4;
        UCHAR Control;
    } SCAN_CD, *PSCAN_CD;
    struct _STOP_PLAY_SCAN
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR Reserved2[7];
        UCHAR Control;
    } STOP_PLAY_SCAN, *PSTOP_PLAY_SCAN;
    struct _SUBCHANNEL
    {
        UCHAR OperationCode;
        UCHAR Reserved0:1;
        UCHAR Msf:1;
        UCHAR Reserved1:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2:6;
        UCHAR SubQ:1;
        UCHAR Reserved3:1;
        UCHAR Format;
        UCHAR Reserved4[2];
        UCHAR TrackNumber;
        UCHAR AllocationLength[2];
        UCHAR Control;
    } SUBCHANNEL, *PSUBCHANNEL;
    struct _READ_CD
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved0:1;
        UCHAR ExpectedSectorType:3;
        UCHAR Lun:3;
        UCHAR StartingLBA[4];
        UCHAR TransferBlocks[3];
        UCHAR Reserved2:1;
        UCHAR ErrorFlags:2;
        UCHAR IncludeEDC:1;
        UCHAR IncludeUserData:1;
        UCHAR HeaderCode:2;
        UCHAR IncludeSyncData:1;
        UCHAR SubChannelSelection:3;
        UCHAR Reserved3:5;
        UCHAR Control;
    } READ_CD, *PREAD_CD;
    struct _READ_CD_MSF
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:1;
        UCHAR ExpectedSectorType:3;
        UCHAR Lun:3;
        UCHAR Reserved2;
        UCHAR StartingM;
        UCHAR StartingS;
        UCHAR StartingF;
        UCHAR EndingM;
        UCHAR EndingS;
        UCHAR EndingF;
        UCHAR Reserved3;
        UCHAR Reserved4:1;
        UCHAR ErrorFlags:2;
        UCHAR IncludeEDC:1;
        UCHAR IncludeUserData:1;
        UCHAR HeaderCode:2;
        UCHAR IncludeSyncData:1;
        UCHAR SubChannelSelection:3;
        UCHAR Reserved5:5;
        UCHAR Control;
    } READ_CD_MSF, *PREAD_CD_MSF;
    struct _PLXTR_READ_CDDA
    {
        UCHAR OperationCode;
        UCHAR Reserved0:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlockByte0;
        UCHAR LogicalBlockByte1;
        UCHAR LogicalBlockByte2;
        UCHAR LogicalBlockByte3;
        UCHAR TransferBlockByte0;
        UCHAR TransferBlockByte1;
        UCHAR TransferBlockByte2;
        UCHAR TransferBlockByte3;
        UCHAR SubCode;
        UCHAR Control;
    } PLXTR_READ_CDDA, *PPLXTR_READ_CDDA;
    struct _NEC_READ_CDDA
    {
        UCHAR OperationCode;
        UCHAR Reserved0;
        UCHAR LogicalBlockByte0;
        UCHAR LogicalBlockByte1;
        UCHAR LogicalBlockByte2;
        UCHAR LogicalBlockByte3;
        UCHAR Reserved1;
        UCHAR TransferBlockByte0;
        UCHAR TransferBlockByte1;
        UCHAR Control;
    } NEC_READ_CDDA, *PNEC_READ_CDDA;
    struct _MODE_SENSE
    {
        UCHAR OperationCode;
        UCHAR Reserved1:3;
        UCHAR Dbd:1;
        UCHAR Reserved2:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR PageCode:6;
        UCHAR Pc:2;
        UCHAR Reserved3;
        UCHAR AllocationLength;
        UCHAR Control;
    } MODE_SENSE, *PMODE_SENSE;
    struct _MODE_SENSE10
    {
        UCHAR OperationCode;
        UCHAR Reserved1:3;
        UCHAR Dbd:1;
        UCHAR Reserved2:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR PageCode:6;
        UCHAR Pc:2;
        UCHAR Reserved3[4];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } MODE_SENSE10, *PMODE_SENSE10;
    struct _MODE_SELECT
    {
        UCHAR OperationCode;
        UCHAR SPBit:1;
        UCHAR Reserved1:3;
        UCHAR PFBit:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[2];
        UCHAR ParameterListLength;
        UCHAR Control;
    } MODE_SELECT, *PMODE_SELECT;
    struct _MODE_SELECT10
    {
        UCHAR OperationCode;
        UCHAR SPBit:1;
        UCHAR Reserved1:3;
        UCHAR PFBit:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[5];
        UCHAR ParameterListLength[2];
        UCHAR Control;
    } MODE_SELECT10, *PMODE_SELECT10;
    struct _LOCATE
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR CPBit:1;
        UCHAR BTBit:1;
        UCHAR Reserved1:2;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved4;
        UCHAR Partition;
        UCHAR Control;
    } LOCATE, *PLOCATE;
    struct _LOGSENSE
    {
        UCHAR OperationCode;
        UCHAR SPBit:1;
        UCHAR PPCBit:1;
        UCHAR Reserved1:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR PageCode:6;
        UCHAR PCBit:2;
        UCHAR Reserved2;
        UCHAR Reserved3;
        UCHAR ParameterPointer[2];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } LOGSENSE, *PLOGSENSE;
    struct _LOGSELECT
    {
        UCHAR OperationCode;
        UCHAR SPBit:1;
        UCHAR PCRBit:1;
        UCHAR Reserved1:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved:6;
        UCHAR PCBit:2;
        UCHAR Reserved2[4];
        UCHAR ParameterListLength[2];
        UCHAR Control;
    } LOGSELECT, *PLOGSELECT;
    struct _PRINT
    {
        UCHAR OperationCode;
        UCHAR Reserved:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR TransferLength[3];
        UCHAR Control;
    } PRINT, *PPRINT;
    struct _SEEK
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved2[3];
        UCHAR Control;
    } SEEK, *PSEEK;
    struct _ERASE
    {
        UCHAR OperationCode;
        UCHAR Long:1;
        UCHAR Immediate:1;
        UCHAR Reserved1:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[3];
        UCHAR Control;
    } ERASE, *PERASE;
    struct _START_STOP
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved1:4;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[2];
        UCHAR Start:1;
        UCHAR LoadEject:1;
        UCHAR Reserved3:6;
        UCHAR Control;
    } START_STOP, *PSTART_STOP;
    struct _MEDIA_REMOVAL
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR Reserved2[2];
        UCHAR Prevent:1;
        UCHAR Persistant:1;
        UCHAR Reserved3:6;
        UCHAR Control;
    } MEDIA_REMOVAL, *PMEDIA_REMOVAL;
    struct _SEEK_BLOCK
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved1:7;
        UCHAR BlockAddress[3];
        UCHAR Link:1;
        UCHAR Flag:1;
        UCHAR Reserved2:4;
        UCHAR VendorUnique:2;
    } SEEK_BLOCK, *PSEEK_BLOCK;
    struct _REQUEST_BLOCK_ADDRESS
    {
        UCHAR OperationCode;
        UCHAR Reserved1[3];
        UCHAR AllocationLength;
        UCHAR Link:1;
        UCHAR Flag:1;
        UCHAR Reserved2:4;
        UCHAR VendorUnique:2;
    } REQUEST_BLOCK_ADDRESS, *PREQUEST_BLOCK_ADDRESS;
    struct _PARTITION
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Sel:1;
        UCHAR PartitionSelect:6;
        UCHAR Reserved1[3];
        UCHAR Control;
    } PARTITION, *PPARTITION;
    struct _WRITE_TAPE_MARKS
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR WriteSetMarks:1;
        UCHAR Reserved:3;
        UCHAR LogicalUnitNumber:3;
        UCHAR TransferLength[3];
        UCHAR Control;
    } WRITE_TAPE_MARKS, *PWRITE_TAPE_MARKS;
    struct _SPACE_TAPE_MARKS
    {
        UCHAR OperationCode;
        UCHAR Code:3;
        UCHAR Reserved:2;
        UCHAR LogicalUnitNumber:3;
        UCHAR NumMarksMSB;
        UCHAR NumMarks;
        UCHAR NumMarksLSB;
        union
        {
            UCHAR value;
            struct
            {
                UCHAR Link:1;
                UCHAR Flag:1;
                UCHAR Reserved:4;
                UCHAR VendorUnique:2;
            } Fields;
        } Byte6;
    } SPACE_TAPE_MARKS, *PSPACE_TAPE_MARKS;
    struct _READ_POSITION
    {
        UCHAR Operation;
        UCHAR BlockType:1;
        UCHAR Reserved1:4;
        UCHAR Lun:3;
        UCHAR Reserved2[7];
        UCHAR Control;
    } READ_POSITION, *PREAD_POSITION;
    struct _CDB6READWRITETAPE
    {
        UCHAR OperationCode;
        UCHAR VendorSpecific:5;
        UCHAR Reserved:3;
        UCHAR TransferLenMSB;
        UCHAR TransferLen;
        UCHAR TransferLenLSB;
        UCHAR Link:1;
        UCHAR Flag:1;
        UCHAR Reserved1:4;
        UCHAR VendorUnique:2;
    } CDB6READWRITETAPE, *PCDB6READWRITETAPE;
    struct _INIT_ELEMENT_STATUS
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNubmer:3;
        UCHAR Reserved2[3];
        UCHAR Reserved3:7;
        UCHAR NoBarCode:1;
    } INIT_ELEMENT_STATUS, *PINIT_ELEMENT_STATUS;
    struct _INITIALIZE_ELEMENT_RANGE
    {
        UCHAR OperationCode;
        UCHAR Range:1;
        UCHAR Reserved1:4;
        UCHAR LogicalUnitNubmer:3;
        UCHAR FirstElementAddress[2];
        UCHAR Reserved2[2];
        UCHAR NumberOfElements[2];
        UCHAR Reserved3;
        UCHAR Reserved4:7;
        UCHAR NoBarCode:1;
    } INITIALIZE_ELEMENT_RANGE, *PINITIALIZE_ELEMENT_RANGE;
    struct _POSITION_TO_ELEMENT
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR TransportElementAddress[2];
        UCHAR DestinationElementAddress[2];
        UCHAR Reserved2[2];
        UCHAR Flip:1;
        UCHAR Reserved3:7;
        UCHAR Control;
    } POSITION_TO_ELEMENT, *PPOSITION_TO_ELEMENT;
    struct _MOVE_MEDIUM
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR TransportElementAddress[2];
        UCHAR SourceElementAddress[2];
        UCHAR DestinationElementAddress[2];
        UCHAR Reserved2[2];
        UCHAR Flip:1;
        UCHAR Reserved3:7;
        UCHAR Control;
    } MOVE_MEDIUM, *PMOVE_MEDIUM;
    struct _EXCHANGE_MEDIUM
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR LogicalUnitNumber:3;
        UCHAR TransportElementAddress[2];
        UCHAR SourceElementAddress[2];
        UCHAR Destination1ElementAddress[2];
        UCHAR Destination2ElementAddress[2];
        UCHAR Flip1:1;
        UCHAR Flip2:1;
        UCHAR Reserved3:6;
        UCHAR Control;
    } EXCHANGE_MEDIUM, *PEXCHANGE_MEDIUM;
    struct _READ_ELEMENT_STATUS
    {
        UCHAR OperationCode;
        UCHAR ElementType:4;
        UCHAR VolTag:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR StartingElementAddress[2];
        UCHAR NumberOfElements[2];
        UCHAR Reserved1;
        UCHAR AllocationLength[3];
        UCHAR Reserved2;
        UCHAR Control;
    } READ_ELEMENT_STATUS, *PREAD_ELEMENT_STATUS;
    struct _SEND_VOLUME_TAG
    {
        UCHAR OperationCode;
        UCHAR ElementType:4;
        UCHAR Reserved1:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR StartingElementAddress[2];
        UCHAR Reserved2;
        UCHAR ActionCode:5;
        UCHAR Reserved3:3;
        UCHAR Reserved4[2];
        UCHAR ParameterListLength[2];
        UCHAR Reserved5;
        UCHAR Control;
    } SEND_VOLUME_TAG, *PSEND_VOLUME_TAG;
    struct _REQUEST_VOLUME_ELEMENT_ADDRESS
    {
        UCHAR OperationCode;
        UCHAR ElementType:4;
        UCHAR VolTag:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR StartingElementAddress[2];
        UCHAR NumberElements[2];
        UCHAR Reserved1;
        UCHAR AllocationLength[3];
        UCHAR Reserved2;
        UCHAR Control;
    } REQUEST_VOLUME_ELEMENT_ADDRESS, *PREQUEST_VOLUME_ELEMENT_ADDRESS;
    struct _LOAD_UNLOAD
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved1:4;
        UCHAR Lun:3;
        UCHAR Reserved2[2];
        UCHAR Start:1;
        UCHAR LoadEject:1;
        UCHAR Reserved3:6;
        UCHAR Reserved4[3];
        UCHAR Slot;
        UCHAR Reserved5[3];
    } LOAD_UNLOAD, *PLOAD_UNLOAD;
    struct _MECH_STATUS
    {
        UCHAR OperationCode;
        UCHAR Reserved:5;
        UCHAR Lun:3;
        UCHAR Reserved1[6];
        UCHAR AllocationLength[2];
        UCHAR Reserved2[1];
        UCHAR Control;
    } MECH_STATUS, *PMECH_STATUS;
    struct _SYNCHRONIZE_CACHE10
    {
        UCHAR OperationCode;
        UCHAR RelAddr:1;
        UCHAR Immediate:1;
        UCHAR Reserved:3;
        UCHAR Lun:3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved2;
        UCHAR BlockCount[2];
        UCHAR Control;
    } SYNCHRONIZE_CACHE10, *PSYNCHRONIZE_CACHE10;
    struct _GET_EVENT_STATUS_NOTIFICATION
    {
        UCHAR OperationCode;
        UCHAR Immediate:1;
        UCHAR Reserved:4;
        UCHAR Lun:3;
        UCHAR Reserved2[2];
        UCHAR NotificationClassRequest;
        UCHAR Reserved3[2];
        UCHAR EventListLength[2];
        UCHAR Control;
    } GET_EVENT_STATUS_NOTIFICATION, *PGET_EVENT_STATUS_NOTIFICATION;
    struct _GET_PERFORMANCE
    {
        UCHAR OperationCode;
        UCHAR Except:2;
        UCHAR Write:1;
        UCHAR Tolerance:2;
        UCHAR Reserved0:3;
        UCHAR StartingLBA[4];
        UCHAR Reserved1[2];
        UCHAR MaximumNumberOfDescriptors[2];
        UCHAR Type;
        UCHAR Control;
    } GET_PERFORMANCE;
    struct _READ_DVD_STRUCTURE
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR RMDBlockNumber[4];
        UCHAR LayerNumber;
        UCHAR Format;
        UCHAR AllocationLength[2];
        UCHAR Reserved3:6;
        UCHAR AGID:2;
        UCHAR Control;
    } READ_DVD_STRUCTURE, *PREAD_DVD_STRUCTURE;
    struct _SET_STREAMING
    {
        UCHAR OperationCode;
        UCHAR Reserved[8];
        UCHAR ParameterListLength[2];
        UCHAR Control;
    } SET_STREAMING;
    struct _SEND_DVD_STRUCTURE
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR Reserved2[5];
        UCHAR Format;
        UCHAR ParameterListLength[2];
        UCHAR Reserved3;
        UCHAR Control;
    } SEND_DVD_STRUCTURE, *PSEND_DVD_STRUCTURE;
    struct _SEND_KEY
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR Reserved2[6];
        UCHAR ParameterListLength[2];
        UCHAR KeyFormat:6;
        UCHAR AGID:2;
        UCHAR Control;
    } SEND_KEY, *PSEND_KEY;
    struct _REPORT_KEY
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR LogicalBlockAddress[4];
        UCHAR Reserved2[2];
        UCHAR AllocationLength[2];
        UCHAR KeyFormat:6;
        UCHAR AGID:2;
        UCHAR Control;
    } REPORT_KEY, *PREPORT_KEY;
    struct _SET_READ_AHEAD
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR TriggerLBA[4];
        UCHAR ReadAheadLBA[4];
        UCHAR Reserved2;
        UCHAR Control;
    } SET_READ_AHEAD, *PSET_READ_AHEAD;
    struct _READ_FORMATTED_CAPACITIES
    {
        UCHAR OperationCode;
        UCHAR Reserved1:5;
        UCHAR Lun:3;
        UCHAR Reserved2[5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } READ_FORMATTED_CAPACITIES, *PREAD_FORMATTED_CAPACITIES;
    struct _REPORT_LUNS
    {
        UCHAR OperationCode;
        UCHAR Reserved1[5];
        UCHAR AllocationLength[4];
        UCHAR Reserved2[1];
        UCHAR Control;
    } REPORT_LUNS, *PREPORT_LUNS;
    struct _PERSISTENT_RESERVE_IN
    {
        UCHAR OperationCode;
        UCHAR ServiceAction:5;
        UCHAR Reserved1:3;
        UCHAR Reserved2[5];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } PERSISTENT_RESERVE_IN, *PPERSISTENT_RESERVE_IN;
    struct _PERSISTENT_RESERVE_OUT
    {
        UCHAR OperationCode;
        UCHAR ServiceAction:5;
        UCHAR Reserved1:3;
        UCHAR Type:4;
        UCHAR Scope:4;
        UCHAR Reserved2[4];
        UCHAR ParameterListLength[2];
        UCHAR Control;
    } PERSISTENT_RESERVE_OUT, *PPERSISTENT_RESERVE_OUT;
    struct _GET_CONFIGURATION
    {
        UCHAR OperationCode;
        UCHAR RequestType:1;
        UCHAR Reserved1:7;
        UCHAR StartingFeature[2];
        UCHAR Reserved2[3];
        UCHAR AllocationLength[2];
        UCHAR Control;
    } GET_CONFIGURATION, *PGET_CONFIGURATION;
    struct _SET_CD_SPEED
    {
        UCHAR OperationCode;
        _ANONYMOUS_UNION union
        {
            UCHAR Reserved1;
            _ANONYMOUS_STRUCT struct
            {
                UCHAR RotationControl:2;
                UCHAR Reserved3:6;
            } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;
        UCHAR ReadSpeed[2];
        UCHAR WriteSpeed[2];
        UCHAR Reserved2[5];
        UCHAR Control;
    } SET_CD_SPEED, *PSET_CD_SPEED;
    struct _READ12
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:2;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlock[4];
        UCHAR TransferLength[4];
        UCHAR Reserved2:7;
        UCHAR Streaming:1;
        UCHAR Control;
    } READ12;
    struct _WRITE12
    {
        UCHAR OperationCode;
        UCHAR RelativeAddress:1;
        UCHAR Reserved1:1;
        UCHAR EBP:1;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR LogicalUnitNumber:3;
        UCHAR LogicalBlock[4];
        UCHAR TransferLength[4];
        UCHAR Reserved2:7;
        UCHAR Streaming:1;
        UCHAR Control;
    } WRITE12;
    struct _READ16
    {
        UCHAR OperationCode;
        UCHAR Reserved1:3;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR ReadProtect:3;
        UCHAR LogicalBlock[8];
        UCHAR TransferLength[4];
        UCHAR Reserved2:7;
        UCHAR Streaming:1;
        UCHAR Control;
    } READ16;
    struct _WRITE16
    {
        UCHAR OperationCode;
        UCHAR Reserved1:3;
        UCHAR ForceUnitAccess:1;
        UCHAR DisablePageOut:1;
        UCHAR WriteProtect:3;
        UCHAR LogicalBlock[8];
        UCHAR TransferLength[4];
        UCHAR Reserved2:7;
        UCHAR Streaming:1;
        UCHAR Control;
    } WRITE16;
    struct _VERIFY16
    {
        UCHAR OperationCode;
        UCHAR Reserved1:1;
        UCHAR ByteCheck:1;
        UCHAR BlockVerify:1;
        UCHAR Reserved2: 1;
        UCHAR DisablePageOut:1;
        UCHAR VerifyProtect:3;
        UCHAR LogicalBlock[8];
        UCHAR VerificationLength[4];
        UCHAR Reserved3:7;
        UCHAR Streaming:1;
        UCHAR Control;
    } VERIFY16;
    struct _SYNCHRONIZE_CACHE16
    {
        UCHAR OperationCode;
        UCHAR Reserved1:1;
        UCHAR Immediate:1;
        UCHAR Reserved2:6;
        UCHAR LogicalBlock[8];
        UCHAR BlockCount[4];
        UCHAR Reserved3;
        UCHAR Control;
    } SYNCHRONIZE_CACHE16;
    struct _READ_CAPACITY16
    {
        UCHAR OperationCode;
        UCHAR ServiceAction:5;
        UCHAR Reserved1:3;
        UCHAR LogicalBlock[8];
        UCHAR BlockCount[4];
        UCHAR PMI:1;
        UCHAR Reserved2:7;
        UCHAR Control;
    } READ_CAPACITY16;
    ULONG AsUlong[4];
    UCHAR AsByte[16];
} CDB, *PCDB;

typedef union _EIGHT_BYTE
{
    struct
    {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
        UCHAR Byte4;
        UCHAR Byte5;
        UCHAR Byte6;
        UCHAR Byte7;
    };
    ULONGLONG AsULongLong;
} EIGHT_BYTE, *PEIGHT_BYTE;

typedef union _FOUR_BYTE
{
    struct
    {
        UCHAR Byte0;
        UCHAR Byte1;
        UCHAR Byte2;
        UCHAR Byte3;
    };
    ULONG AsULong;
} FOUR_BYTE, *PFOUR_BYTE;

typedef union _TWO_BYTE
{
    struct
    {
        UCHAR Byte0;
        UCHAR Byte1;
    };
    USHORT AsUShort;
} TWO_BYTE, *PTWO_BYTE;
#include <poppack.h>

#if (NTDDI_VERSION < NTDDI_WINXP)
typedef struct _INQUIRYDATA
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR DeviceTypeModifier:7;
    UCHAR RemovableMedia:1;
    UCHAR Versions;
    UCHAR ResponseDataFormat:4;
    UCHAR HiSupport:1;
    UCHAR NormACA:1;
    UCHAR ReservedBit:1;
    UCHAR AERC:1;
    UCHAR AdditionalLength;
    UCHAR Reserved[2];
    UCHAR SoftReset:1;
    UCHAR CommandQueue:1;
    UCHAR Reserved2:1;
    UCHAR LinkedCommands:1;
    UCHAR Synchronous:1;
    UCHAR Wide16Bit:1;
    UCHAR Wide32Bit:1;
    UCHAR RelativeAddressing:1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;
#else
#include <pshpack1.h>
typedef struct _INQUIRYDATA
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR DeviceTypeModifier:7;
    UCHAR RemovableMedia:1;
    union
    {
        UCHAR Versions;
        struct
        {
            UCHAR ANSIVersion:3;
            UCHAR ECMAVersion:3;
            UCHAR ISOVersion:2;
        };
    };
    UCHAR ResponseDataFormat:4;
    UCHAR HiSupport:1;
    UCHAR NormACA:1;
    UCHAR TerminateTask:1;
    UCHAR AERC:1;
    UCHAR AdditionalLength;
    UCHAR Reserved;
    UCHAR Addr16:1;
    UCHAR Addr32:1;
    UCHAR AckReqQ:1;
    UCHAR MediumChanger:1;
    UCHAR MultiPort:1;
    UCHAR ReservedBit2:1;
    UCHAR EnclosureServices:1;
    UCHAR ReservedBit3:1;
    UCHAR SoftReset:1;
    UCHAR CommandQueue:1;
    UCHAR TransferDisable:1;
    UCHAR LinkedCommands:1;
    UCHAR Synchronous:1;
    UCHAR Wide16Bit:1;
    UCHAR Wide32Bit:1;
    UCHAR RelativeAddressing:1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;
#include <poppack.h>
#endif

typedef struct _VPD_MEDIA_SERIAL_NUMBER_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[0];
} VPD_MEDIA_SERIAL_NUMBER_PAGE, *PVPD_MEDIA_SERIAL_NUMBER_PAGE;

typedef struct _VPD_SERIAL_NUMBER_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SerialNumber[0];
} VPD_SERIAL_NUMBER_PAGE, *PVPD_SERIAL_NUMBER_PAGE;

typedef struct _VPD_IDENTIFICATION_DESCRIPTOR
{
    UCHAR CodeSet:4;
    UCHAR Reserved:4;
    UCHAR IdentifierType:4;
    UCHAR Association:2;
    UCHAR Reserved2:2;
    UCHAR Reserved3;
    UCHAR IdentifierLength;
    UCHAR Identifier[0];
} VPD_IDENTIFICATION_DESCRIPTOR, *PVPD_IDENTIFICATION_DESCRIPTOR;

typedef struct _VPD_IDENTIFICATION_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR Descriptors[0];
} VPD_IDENTIFICATION_PAGE, *PVPD_IDENTIFICATION_PAGE;

typedef struct _VPD_SUPPORTED_PAGES_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR Reserved;
    UCHAR PageLength;
    UCHAR SupportedPageList[0];
} VPD_SUPPORTED_PAGES_PAGE, *PVPD_SUPPORTED_PAGES_PAGE;

#include <pshpack1.h>
typedef struct _READ_CAPACITY_DATA
{
    ULONG LogicalBlockAddress;
    ULONG BytesPerBlock;
} READ_CAPACITY_DATA, *PREAD_CAPACITY_DATA;

typedef struct _READ_CAPACITY_DATA_EX
{
    LARGE_INTEGER LogicalBlockAddress;
    ULONG BytesPerBlock;
} READ_CAPACITY_DATA_EX, *PREAD_CAPACITY_DATA_EX;

typedef struct _MODE_PARAMETER_HEADER
{
    UCHAR ModeDataLength;
    UCHAR MediumType;
    UCHAR DeviceSpecificParameter;
    UCHAR BlockDescriptorLength;
}MODE_PARAMETER_HEADER, *PMODE_PARAMETER_HEADER;

typedef struct _MODE_PARAMETER_HEADER10
{
    UCHAR ModeDataLength[2];
    UCHAR MediumType;
    UCHAR DeviceSpecificParameter;
    UCHAR Reserved[2];
    UCHAR BlockDescriptorLength[2];
}MODE_PARAMETER_HEADER10, *PMODE_PARAMETER_HEADER10;

typedef struct _MODE_PARAMETER_BLOCK
{
    UCHAR DensityCode;
    UCHAR NumberOfBlocks[3];
    UCHAR Reserved;
    UCHAR BlockLength[3];
}MODE_PARAMETER_BLOCK, *PMODE_PARAMETER_BLOCK;

typedef struct _LUN_LIST
{
    UCHAR LunListLength[4];
    UCHAR Reserved[4];
#if !defined(__midl)
    UCHAR Lun[0][8];
#endif
} LUN_LIST, *PLUN_LIST;

typedef struct _SENSE_DATA
{
    UCHAR ErrorCode:7;
    UCHAR Valid:1;
    UCHAR SegmentNumber;
    UCHAR SenseKey:4;
    UCHAR Reserved:1;
    UCHAR IncorrectLength:1;
    UCHAR EndOfMedia:1;
    UCHAR FileMark:1;
    UCHAR Information[4];
    UCHAR AdditionalSenseLength;
    UCHAR CommandSpecificInformation[4];
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR FieldReplaceableUnitCode;
    UCHAR SenseKeySpecific[3];
} SENSE_DATA, *PSENSE_DATA;

#include <poppack.h>

typedef PHYSICAL_ADDRESS STOR_PHYSICAL_ADDRESS;

typedef struct _ACCESS_RANGE
{
    STOR_PHYSICAL_ADDRESS RangeStart;
    ULONG RangeLength;
    BOOLEAN RangeInMemory;
} ACCESS_RANGE, *PACCESS_RANGE;

typedef struct _MEMORY_REGION
{
    PUCHAR VirtualBase;
    PHYSICAL_ADDRESS PhysicalBase;
    ULONG Length;
} MEMORY_REGION, *PMEMORY_REGION;

typedef struct _PORT_CONFIGURATION_INFORMATION
{
    ULONG Length;
    ULONG SystemIoBusNumber;
    INTERFACE_TYPE  AdapterInterfaceType;
    ULONG BusInterruptLevel;
    ULONG BusInterruptVector;
    KINTERRUPT_MODE InterruptMode;
    ULONG MaximumTransferLength;
    ULONG NumberOfPhysicalBreaks;
    ULONG DmaChannel;
    ULONG DmaPort;
    DMA_WIDTH DmaWidth;
    DMA_SPEED DmaSpeed;
    ULONG AlignmentMask;
    ULONG NumberOfAccessRanges;
    ACCESS_RANGE (*AccessRanges)[];
    PVOID Reserved;
    UCHAR NumberOfBuses;
    CCHAR InitiatorBusId[8];
    BOOLEAN ScatterGather;
    BOOLEAN Master;
    BOOLEAN CachesData;
    BOOLEAN AdapterScansDown;
    BOOLEAN AtdiskPrimaryClaimed;
    BOOLEAN AtdiskSecondaryClaimed;
    BOOLEAN Dma32BitAddresses;
    BOOLEAN DemandMode;
    UCHAR MapBuffers;
    BOOLEAN NeedPhysicalAddresses;
    BOOLEAN TaggedQueuing;
    BOOLEAN AutoRequestSense;
    BOOLEAN MultipleRequestPerLu;
    BOOLEAN ReceiveEvent;
    BOOLEAN RealModeInitialized;
    BOOLEAN BufferAccessScsiPortControlled;
    UCHAR MaximumNumberOfTargets;
    UCHAR ReservedUchars[2];
    ULONG SlotNumber;
    ULONG BusInterruptLevel2;
    ULONG BusInterruptVector2;
    KINTERRUPT_MODE InterruptMode2;
    ULONG DmaChannel2;
    ULONG DmaPort2;
    DMA_WIDTH DmaWidth2;
    DMA_SPEED DmaSpeed2;
    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;
    UCHAR  Dma64BitAddresses;
    BOOLEAN ResetTargetSupported;
    UCHAR MaximumNumberOfLogicalUnits;
    BOOLEAN WmiDataProvider;
    STOR_SYNCHRONIZATION_MODEL SynchronizationModel;
} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

typedef struct _STOR_SCATTER_GATHER_ELEMENT
{
    STOR_PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Length;
    ULONG_PTR Reserved;
} STOR_SCATTER_GATHER_ELEMENT, *PSTOR_SCATTER_GATHER_ELEMENT;

typedef struct _STOR_SCATTER_GATHER_LIST
{
    ULONG NumberOfElements;
    ULONG_PTR Reserved;
    STOR_SCATTER_GATHER_ELEMENT List[];
} STOR_SCATTER_GATHER_LIST, *PSTOR_SCATTER_GATHER_LIST;

typedef struct _DPC_BUFFER
{
    CSHORT Type;
    UCHAR Number;
    UCHAR Importance;
    struct
    {
        PVOID F;
        PVOID B;
    };
    PVOID DeferredRoutine;
    PVOID DeferredContext;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PVOID DpcData;
} DPC_BUFFER;

typedef struct _STOR_DPC
{
    DPC_BUFFER Dpc;
    ULONG_PTR Lock;
} STOR_DPC, *PSTOR_DPC;

typedef struct _STOR_LOCK_HANDLE
{
    STOR_SPINLOCK Lock;
    struct
    {
        struct
        {
            PVOID Next;
            PVOID Lock;
        } LockQueue;
        KIRQL OldIrql;
    } Context;
} STOR_LOCK_HANDLE, *PSTOR_LOCK_HANDLE;

typedef struct _STOR_LOG_EVENT_DETAILS
{
    ULONG InterfaceRevision;
    ULONG Size;
    ULONG Flags;
    STOR_EVENT_ASSOCIATION_ENUM EventAssociation;
    ULONG PathId;
    ULONG TargetId;
    ULONG LunId;
    BOOLEAN StorportSpecificErrorCode;
    ULONG ErrorCode;
    ULONG UniqueId;
    ULONG DumpDataSize;
    PVOID DumpData;
    ULONG StringCount;
    PWSTR *StringList;
} STOR_LOG_EVENT_DETAILS, *PSTOR_LOG_EVENT_DETAILS;

typedef struct _PERF_CONFIGURATION_DATA
{
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG ConcurrentChannels;
    ULONG FirstRedirectionMessageNumber, LastRedirectionMessageNumber;
    ULONG DeviceNode;
    ULONG Reserved;
    PGROUP_AFFINITY MessageTargets;
} PERF_CONFIGURATION_DATA, *PPERF_CONFIGURATION_DATA;

typedef struct _STARTIO_PERFORMANCE_PARAMETERS
{
    ULONG Version;
    ULONG Size;
    ULONG MessageNumber;
    ULONG ChannelNumber;
} STARTIO_PERFORMANCE_PARAMETERS, *PSTARTIO_PERFORMANCE_PARAMETERS;

typedef struct _MESSAGE_INTERRUPT_INFORMATION
{
    ULONG MessageId;
    ULONG MessageData;
    STOR_PHYSICAL_ADDRESS MessageAddress;
    ULONG InterruptVector;
    ULONG InterruptLevel;
    KINTERRUPT_MODE InterruptMode;
} MESSAGE_INTERRUPT_INFORMATION, *PMESSAGE_INTERRUPT_INFORMATION;

typedef
BOOLEAN
(NTAPI *PHW_INITIALIZE)(
    _In_ PVOID DeviceExtension);

typedef
BOOLEAN
(NTAPI *PHW_BUILDIO)(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

typedef
BOOLEAN
(NTAPI *PHW_STARTIO)(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

typedef
BOOLEAN
(NTAPI *PHW_INTERRUPT)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(NTAPI *PHW_TIMER)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(NTAPI *PHW_DMA_STARTED)(
    _In_ PVOID DeviceExtension);

typedef
ULONG
(NTAPI *PHW_FIND_ADAPTER)(
    IN PVOID DeviceExtension,
    IN PVOID HwContext,
    IN PVOID BusInformation,
    IN PCHAR ArgumentString,
    IN OUT PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    OUT PBOOLEAN Again);

typedef
BOOLEAN
(NTAPI *PHW_RESET_BUS)(
    IN PVOID DeviceExtension,
    IN ULONG PathId);

typedef
BOOLEAN
(NTAPI *PHW_ADAPTER_STATE)(
    IN PVOID DeviceExtension,
    IN PVOID Context,
    IN BOOLEAN SaveState);

typedef
SCSI_ADAPTER_CONTROL_STATUS
(NTAPI *PHW_ADAPTER_CONTROL)(
    IN PVOID DeviceExtension,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters);

typedef
BOOLEAN
(*PHW_PASSIVE_INITIALIZE_ROUTINE)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(*PHW_DPC_ROUTINE)(
    _In_ PSTOR_DPC Dpc,
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

typedef
BOOLEAN
(NTAPI STOR_SYNCHRONIZED_ACCESS)(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Context);

typedef STOR_SYNCHRONIZED_ACCESS *PSTOR_SYNCHRONIZED_ACCESS;

typedef
VOID
(NTAPI *PpostScaterGatherExecute)(
    _In_ PVOID *DeviceObject,
    _In_ PVOID *Irp,
    _In_ PSTOR_SCATTER_GATHER_LIST ScatterGather,
    _In_ PVOID Context);

typedef
BOOLEAN
(NTAPI *PStorPortGetMessageInterruptInformation)(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId,
    _Out_ PMESSAGE_INTERRUPT_INFORMATION InterruptInfo);

typedef
VOID
(NTAPI *PStorPortPutScatterGatherList)(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_SCATTER_GATHER_LIST ScatterGatherList,
    _In_ BOOLEAN WriteToDevice);

typedef
GETSGSTATUS
(NTAPI *PStorPortBuildScatterGatherList)(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Mdl,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ PpostScaterGatherExecute ExecutionRoutine,
    _In_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _Inout_ PVOID ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength);

typedef
VOID
(NTAPI *PStorPortFreePool)(
    _In_ PVOID PMemory,
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID PMdl);

typedef
PVOID
(NTAPI *PStorPortAllocatePool)(
    _In_ ULONG NumberOfBytes,
    _In_ ULONG Tag,
    _In_ PVOID HwDeviceExtension,
    _Out_ PVOID *PMdl);

typedef
PVOID
(NTAPI *PStorPortGetSystemAddress)(
    _In_ PSCSI_REQUEST_BLOCK Srb);

typedef struct _STORPORT_EXTENDED_FUNCTIONS
{
    ULONG Version;
    PStorPortGetMessageInterruptInformation GetMessageInterruptInformation;
    PStorPortPutScatterGatherList PutScatterGatherList;
    PStorPortBuildScatterGatherList BuildScatterGatherList;
    PStorPortFreePool FreePool;
    PStorPortAllocatePool AllocatePool;
    PStorPortGetSystemAddress GetSystemAddress;
} STORPORT_EXTENDED_FUNCTIONS, *PSTORPORT_EXTENDED_FUNCTIONS;

typedef struct _HW_INITIALIZATION_DATA
{
    ULONG HwInitializationDataSize;
    INTERFACE_TYPE  AdapterInterfaceType;
    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_FIND_ADAPTER HwFindAdapter;
    PHW_RESET_BUS HwResetBus;
    PHW_DMA_STARTED HwDmaStarted;
    PHW_ADAPTER_STATE HwAdapterState;
    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;
    ULONG NumberOfAccessRanges;
    PVOID Reserved;
    UCHAR MapBuffers;
    BOOLEAN NeedPhysicalAddresses;
    BOOLEAN TaggedQueuing;
    BOOLEAN AutoRequestSense;
    BOOLEAN MultipleRequestPerLu;
    BOOLEAN ReceiveEvent;
    USHORT VendorIdLength;
    PVOID VendorId;
    USHORT ReservedUshort;
    USHORT DeviceIdLength;
    PVOID DeviceId;
    PHW_ADAPTER_CONTROL HwAdapterControl;
    PHW_BUILDIO HwBuildIo;
} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;



#define REVERSE_BYTES_QUAD(Destination, Source) { \
    PEIGHT_BYTE d = (PEIGHT_BYTE)(Destination); \
    PEIGHT_BYTE s = (PEIGHT_BYTE)(Source); \
    d->Byte7 = s->Byte0; \
    d->Byte6 = s->Byte1; \
    d->Byte5 = s->Byte2; \
    d->Byte4 = s->Byte3; \
    d->Byte3 = s->Byte4; \
    d->Byte2 = s->Byte5; \
    d->Byte1 = s->Byte6; \
    d->Byte0 = s->Byte7; \
}

#define REVERSE_BYTES(Destination, Source) { \
    PFOUR_BYTE d = (PFOUR_BYTE)(Destination); \
    PFOUR_BYTE s = (PFOUR_BYTE)(Source); \
    d->Byte3 = s->Byte0; \
    d->Byte2 = s->Byte1; \
    d->Byte1 = s->Byte2; \
    d->Byte0 = s->Byte3; \
}

#define REVERSE_BYTES_SHORT(Destination, Source) { \
    PTWO_BYTE d = (PTWO_BYTE)(Destination); \
    PTWO_BYTE s = (PTWO_BYTE)(Source); \
    d->Byte1 = s->Byte0; \
    d->Byte0 = s->Byte1; \
}

#define StorPortCopyMemory(Destination,Source,Length) memcpy((Destination),(Source),(Length))

STORPORT_API
PUCHAR
NTAPI
StorPortAllocateRegistryBuffer(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Length);

STORPORT_API
BOOLEAN
NTAPI
StorPortBusy(
  _In_ PVOID HwDeviceExtension,
  _In_ ULONG RequestsToComplete);

STORPORT_API
VOID
NTAPI
StorPortCompleteRequest(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ UCHAR SrbStatus);

STORPORT_API
ULONG64
NTAPI
StorPortConvertPhysicalAddressToUlong64(
    _In_ STOR_PHYSICAL_ADDRESS Address);

STORPORT_API
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortConvertUlong64ToPhysicalAddress(
    _In_ ULONG64 UlongAddress);

STORPORT_API
VOID
__cdecl
StorPortDebugPrint(
    _In_ ULONG DebugPrintLevel,
    _In_ PCCHAR DebugMessage,
    ...);

STORPORT_API
BOOLEAN
NTAPI
StorPortDeviceBusy(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG RequestsToComplete);

STORPORT_API
BOOLEAN
NTAPI
StorPortDeviceReady(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun);

STORPORT_API
VOID
NTAPI
StorPortFreeDeviceBase(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID MappedAddress);

STORPORT_API
VOID
NTAPI
StorPortFreeRegistryBuffer(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Buffer);

STORPORT_API
ULONG
NTAPI
StorPortGetBusData(
    _In_ PVOID DeviceExtension,
    _In_ ULONG BusDataType,
    _In_ ULONG SystemIoBusNumber,
    _In_ ULONG SlotNumber,
    _Out_ _When_(Length != 0, _Out_writes_bytes_(Length)) PVOID Buffer,
    _In_ ULONG Length);

STORPORT_API
PVOID
NTAPI
StorPortGetDeviceBase(
    _In_ PVOID HwDeviceExtension,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG SystemIoBusNumber,
    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
    _In_ ULONG NumberOfBytes,
    _In_ BOOLEAN InIoSpace);

STORPORT_API
PVOID
NTAPI
StorPortGetLogicalUnit(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun);

STORPORT_API
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortGetPhysicalAddress(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PVOID VirtualAddress,
    _Out_ ULONG *Length);

STORPORT_API
PSTOR_SCATTER_GATHER_LIST
NTAPI
StorPortGetScatterGatherList(
    _In_ PVOID DeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb);

STORPORT_API
PSCSI_REQUEST_BLOCK
NTAPI
StorPortGetSrb(
    _In_ PVOID DeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ LONG QueueTag);

STORPORT_API
PVOID
NTAPI
StorPortGetUncachedExtension(
    _In_ PVOID HwDeviceExtension,
    _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ ULONG NumberOfBytes);

STORPORT_API
PVOID
NTAPI
StorPortGetVirtualAddress(
    _In_ PVOID HwDeviceExtension,
    _In_ STOR_PHYSICAL_ADDRESS PhysicalAddress);

STORPORT_API
ULONG
NTAPI
StorPortInitialize(
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PHW_INITIALIZATION_DATA HwInitializationData,
    _In_opt_ PVOID Unused);

STORPORT_API
VOID
NTAPI
StorPortLogError(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG ErrorCode,
    _In_ ULONG UniqueId);

STORPORT_API
VOID
NTAPI
StorPortMoveMemory(
    _Out_writes_bytes_(Length) PVOID WriteBuffer,
    _In_reads_bytes_(Length) PVOID ReadBuffer,
    _In_ ULONG Length);

STORPORT_API
VOID
__cdecl
StorPortNotification(
    _In_ SCSI_NOTIFICATION_TYPE NotificationType,
    _In_ PVOID HwDeviceExtension,
    ...);

STORPORT_API
VOID
NTAPI
StorPortQuerySystemTime(
    _Out_ PLARGE_INTEGER CurrentTime);

STORPORT_API
BOOLEAN
NTAPI
StorPortPause(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG TimeOut);

STORPORT_API
BOOLEAN
NTAPI
StorPortPauseDevice(
    _In_ PVOID HwDeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun,
    _In_ ULONG TimeOut);

STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortReadPortBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count);

STORPORT_API
UCHAR
NTAPI
StorPortReadPortUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port);

STORPORT_API
ULONG
NTAPI
StorPortReadPortUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port);

STORPORT_API
USHORT
NTAPI
StorPortReadPortUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port);

STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ PULONG Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortReadRegisterBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count);

STORPORT_API
UCHAR
NTAPI
StorPortReadRegisterUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register);

STORPORT_API
ULONG
NTAPI
StorPortReadRegisterUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register);

STORPORT_API
USHORT
NTAPI
StorPortReadRegisterUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register);

STORPORT_API
BOOLEAN
NTAPI
StorPortReady(
  _In_ PVOID HwDeviceExtension);

STORPORT_API
BOOLEAN
NTAPI
StorPortRegistryRead(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR ValueName,
    _In_ ULONG Global,
    _In_ ULONG Type,
    _In_ PUCHAR Buffer,
    _In_ PULONG BufferLength);

STORPORT_API
BOOLEAN
NTAPI
StorPortRegistryWrite(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR ValueName,
    _In_ ULONG Global,
    _In_ ULONG Type,
    _In_ PUCHAR Buffer,
    _In_ ULONG BufferLength);

STORPORT_API
BOOLEAN
NTAPI
StorPortResume(
  _In_ PVOID HwDeviceExtension);

STORPORT_API
BOOLEAN
NTAPI
StorPortResumeDevice(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun);

STORPORT_API
ULONG
NTAPI
StorPortSetBusDataByOffset(
    _In_ PVOID DeviceExtension,
    _In_ ULONG BusDataType,
    _In_ ULONG SystemIoBusNumber,
    _In_ ULONG SlotNumber,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length);

STORPORT_API
BOOLEAN
NTAPI
StorPortSetDeviceQueueDepth(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ ULONG Depth);

STORPORT_API
VOID
NTAPI
StorPortStallExecution(
    _In_ ULONG Delay);

STORPORT_API
VOID
NTAPI
StorPortSynchronizeAccess(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
    _In_opt_ PVOID Context);

STORPORT_API
BOOLEAN
NTAPI
StorPortValidateRange(
    _In_ PVOID HwDeviceExtension,
    _In_ INTERFACE_TYPE BusType,
    _In_ ULONG SystemIoBusNumber,
    _In_ STOR_PHYSICAL_ADDRESS IoAddress,
    _In_ ULONG NumberOfBytes,
    _In_ BOOLEAN InIoSpace);

STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ PULONG Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWritePortBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWritePortUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Port,
    _In_ UCHAR Value);

STORPORT_API
VOID
NTAPI
StorPortWritePortUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Port,
    _In_ ULONG Value);

STORPORT_API
VOID
NTAPI
StorPortWritePortUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Port,
    _In_ USHORT Value);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ PUCHAR Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ PULONG Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterBufferUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ PUSHORT Buffer,
    _In_ ULONG Count);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUchar(
    _In_ PVOID HwDeviceExtension,
    _In_ PUCHAR Register,
    _In_ UCHAR Value);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUlong(
    _In_ PVOID HwDeviceExtension,
    _In_ PULONG Register,
    _In_ ULONG Value);

STORPORT_API
VOID
NTAPI
StorPortWriteRegisterUshort(
    _In_ PVOID HwDeviceExtension,
    _In_ PUSHORT Register,
    _In_ USHORT Value);


FORCEINLINE
BOOLEAN
StorPortEnablePassiveInitialization(
    _In_ PVOID DeviceExtension,
    _In_ PHW_PASSIVE_INITIALIZE_ROUTINE HwPassiveInitializeRoutine)
{
    LONG Succ;
    Succ = FALSE;
    StorPortNotification(EnablePassiveInitialization,
                         DeviceExtension,
                         HwPassiveInitializeRoutine,
                         &Succ);
    return (BOOLEAN)Succ;
}

FORCEINLINE
VOID
StorPortInitializeDpc(
    _In_ PVOID DeviceExtension,
    _Out_ PSTOR_DPC Dpc,
    _In_ PHW_DPC_ROUTINE HwDpcRoutine)
{
    StorPortNotification(InitializeDpc,
                         DeviceExtension,
                         Dpc,
                         HwDpcRoutine);
}

FORCEINLINE
BOOLEAN
StorPortIssueDpc(
    _In_ PVOID DeviceExtension,
    _In_ PSTOR_DPC Dpc,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    LONG Succ;
    Succ = FALSE;
    StorPortNotification(IssueDpc,
                         DeviceExtension,
                         Dpc,
                         SystemArgument1,
                         SystemArgument2,
                         &Succ);
    return (BOOLEAN)Succ;
}

FORCEINLINE
VOID
StorPortAcquireSpinLock(
    _In_ PVOID DeviceExtension,
    _In_ STOR_SPINLOCK SpinLock,
    _In_ PVOID LockContext,
    _Inout_ PSTOR_LOCK_HANDLE LockHandle)
{
    StorPortNotification(AcquireSpinLock,
                         DeviceExtension,
                         SpinLock,
                         LockContext,
                         LockHandle);
}

FORCEINLINE
VOID
StorPortReleaseSpinLock(
    _In_ PVOID DeviceExtension,
    _Inout_ PSTOR_LOCK_HANDLE LockHandle)
{
    StorPortNotification(ReleaseSpinLock,
                         DeviceExtension,
                         LockHandle);
}

STORPORT_API
ULONG
StorPortExtendedFunction(
    _In_ STORPORT_FUNCTION_CODE FunctionCode,
    _In_ PVOID HwDeviceExtension,
    ...);

FORCEINLINE
ULONG
StorPortAllocatePool(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG NumberOfBytes,
    _In_ ULONG Tag,
    _Out_ PVOID *BufferPointer
    )
{
    return StorPortExtendedFunction(ExtFunctionAllocatePool,
                                    HwDeviceExtension,
                                    NumberOfBytes,
                                    Tag,
                                    BufferPointer);
}

FORCEINLINE
ULONG
StorPortFreePool(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID BufferPointer)
{
    return StorPortExtendedFunction(ExtFunctionFreePool,
                                    HwDeviceExtension,
                                    BufferPointer);
}

FORCEINLINE
ULONG
StorPortAllocateMdl(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID BufferPointer,
    _In_ ULONG NumberOfBytes,
    _Out_ PVOID *Mdl)
{
    return StorPortExtendedFunction(ExtFunctionAllocateMdl,
                                    HwDeviceExtension,
                                    BufferPointer,
                                    NumberOfBytes,
                                    Mdl);
}

FORCEINLINE
ULONG
StorPortFreeMdl(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Mdl)
{
    return StorPortExtendedFunction(ExtFunctionFreeMdl,
                                    HwDeviceExtension,
                                    Mdl);
}

FORCEINLINE
ULONG
StorPortBuildMdlForNonPagedPool(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PVOID Mdl)
{
    return StorPortExtendedFunction(ExtFunctionBuildMdlForNonPagedPool,
                                    HwDeviceExtension,
                                    Mdl);
}

FORCEINLINE
ULONG
StorPortGetSystemAddress(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PVOID *SystemAddress)
{
    return StorPortExtendedFunction(ExtFunctionGetSystemAddress,
                                    HwDeviceExtension,
                                    Srb,
                                    SystemAddress);
}

FORCEINLINE
ULONG
StorPortGetOriginalMdl(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PVOID *Mdl)
{
    return StorPortExtendedFunction(ExtFunctionGetOriginalMdl,
                                    HwDeviceExtension,
                                    Srb,
                                    Mdl);
}

FORCEINLINE
ULONG
StorPortCompleteServiceIrp(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Irp)
{
    return StorPortExtendedFunction(ExtFunctionCompleteServiceIrp,
                                    HwDeviceExtension,
                                    Irp);
}

FORCEINLINE
ULONG
StorPortGetDeviceObjects(
    _In_ PVOID HwDeviceExtension,
    _Out_ PVOID *AdapterDeviceObject,
    _Out_ PVOID *PhysicalDeviceObject,
    _Out_ PVOID *LowerDeviceObject)
{
    return StorPortExtendedFunction(ExtFunctionGetDeviceObjects,
                                    HwDeviceExtension,
                                    AdapterDeviceObject,
                                    PhysicalDeviceObject,
                                    LowerDeviceObject);
}

FORCEINLINE
ULONG
StorPortBuildScatterGatherList(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Mdl,
    _In_ PVOID CurrentVa,
    _In_ ULONG Length,
    _In_ PpostScaterGatherExecute ExecutionRoutine,
    _In_ PVOID Context,
    _In_ BOOLEAN WriteToDevice,
    _Inout_ PVOID ScatterGatherBuffer,
    _In_ ULONG ScatterGatherBufferLength)
{
    return StorPortExtendedFunction(ExtFunctionBuildScatterGatherList,
                                    HwDeviceExtension,
                                    Mdl,
                                    CurrentVa,
                                    Length,
                                    ExecutionRoutine,
                                    Context,
                                    WriteToDevice,
                                    ScatterGatherBuffer,
                                    ScatterGatherBufferLength);
}

FORCEINLINE
ULONG
StorPortPutScatterGatherList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_SCATTER_GATHER_LIST ScatterGatherList,
    _In_ BOOLEAN WriteToDevice)
{
    return StorPortExtendedFunction(ExtFunctionPutScatterGatherList,
                                    HwDeviceExtension,
                                    ScatterGatherList,
                                    WriteToDevice);
}

FORCEINLINE
ULONG
StorPortAcquireMSISpinLock(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId,
    _In_ PULONG OldIrql)
{
    return StorPortExtendedFunction(ExtFunctionAcquireMSISpinLock,
                                    HwDeviceExtension,
                                    MessageId,
                                    OldIrql);
}

FORCEINLINE
ULONG
StorPortReleaseMSISpinLock(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId,
    _In_ ULONG OldIrql)
{
    return StorPortExtendedFunction(ExtFunctionReleaseMSISpinLock,
                                    HwDeviceExtension,
                                    MessageId,
                                    OldIrql);
}

FORCEINLINE
ULONG
StorPortGetMSIInfo(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId,
    _Out_ PMESSAGE_INTERRUPT_INFORMATION InterruptInfo)
{
    return StorPortExtendedFunction(ExtFunctionGetMessageInterruptInformation,
                                    HwDeviceExtension,
                                    MessageId,
                                    InterruptInfo);
}

FORCEINLINE
ULONG
StorPortInitializePerfOpts(
    _In_ PVOID HwDeviceExtension,
    _In_ BOOLEAN Query,
    _Inout_ PPERF_CONFIGURATION_DATA PerfConfigData)
{
    return StorPortExtendedFunction(ExtFunctionInitializePerformanceOptimizations,
                                    HwDeviceExtension,
                                    Query,
                                    PerfConfigData);
}

FORCEINLINE
ULONG
StorPortGetStartIoPerfParams(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Inout_ PSTARTIO_PERFORMANCE_PARAMETERS StartIoPerfParams)
{
    return StorPortExtendedFunction(ExtFunctionGetStartIoPerformanceParameters,
                                    HwDeviceExtension,
                                    Srb,
                                    StartIoPerfParams);
}

FORCEINLINE
ULONG
StorPortLogSystemEvent(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PSTOR_LOG_EVENT_DETAILS LogDetails,
    _Inout_ PULONG MaximumSize)
{
    return StorPortExtendedFunction(ExtFunctionLogSystemEvent,
                                    HwDeviceExtension,
                                    LogDetails,
                                    MaximumSize);
}

#if DBG
#define DebugPrint(x) StorPortDebugPrint x
#else
#define DebugPrint(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NTSTORPORT_ */
