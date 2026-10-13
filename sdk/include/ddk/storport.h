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

#ifdef _NTSCSI_
  #error STORPORT.H must be included instead of SCSI.H
#endif

#ifdef _NTSRB_
  #error STORPORT.H must be included instead of SRB.H
#endif

#ifndef _NTSTORPORT_
#define _NTSTORPORT_

#ifdef __cplusplus
extern "C" {
#endif

//
// Common definitions with SRB.H
//

/* NOTE: the current SCSI_MAXIMUM_TARGETS_PER_BUS is applicable
 * only on scsiport miniports. For storport miniports, the max
 * target supported is 255. */
#if (NTDDI_VERSION >= NTDDI_WIN8)
#define SCSI_MAXIMUM_BUSES_PER_ADAPTER    255
#endif
#define SCSI_MAXIMUM_TARGETS_PER_BUS      128
#define SCSI_MAXIMUM_LUNS_PER_TARGET      255
#define SCSI_MINIMUM_PHYSICAL_BREAKS      16
#define SCSI_MAXIMUM_PHYSICAL_BREAKS      255

/* These constants are for backward compatibility.
 * They used to be the maximum supported. */
#define SCSI_MAXIMUM_BUSES                8
#define SCSI_MAXIMUM_TARGETS              8
#define SCSI_MAXIMUM_LOGICAL_UNITS        8

/* PORT_CONFIGURATION_INFORMATION.Dma64BitAddresses constants */
#define SCSI_DMA64_MINIPORT_SUPPORTED            0x01
#define SCSI_DMA64_SYSTEM_SUPPORTED              0x80
#if (NTDDI_VERSION > NTDDI_WS03SP1)
#define SCSI_DMA64_MINIPORT_FULL64BIT_SUPPORTED  0x02
#endif

#define SP_UNINITIALIZED_VALUE            ((ULONG) ~0)
#define SP_UNTAGGED                       ((UCHAR) ~0)

// End of common definitions with SRB.H


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

#define SCSIOP_UNMAP                    0x42

#define SCSIOP_SANITIZE                 0x48

#define SCSIOP_OPERATION32              0x7F

#define SCSIOP_POPULATE_TOKEN           0x83
#define SCSIOP_WRITE_USING_TOKEN        0x83

#define SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION 0x84

#define SCSIOP_COMPARE_AND_WRITE        0x89

#define SCSIOP_ZBC_OUT                  0x94
#define SCSIOP_ZBC_IN                   0x95
#define SCSIOP_READ_DATA_BUFF16         0x9B

#define SCSIOP_GET_LBA_STATUS           0x9E
#define SCSIOP_GET_PHYSICAL_ELEMENT_STATUS 0x9E
#define SCSIOP_REMOVE_ELEMENT_AND_TRUNCATE 0x9E

#define SCSIOP_SECURITY_PROTOCOL_IN     0xA2

#define SCSIOP_SECURITY_PROTOCOL_OUT    0xB5

#define SERVICE_ACTION_OVERWRITE                                                0x01
#define SERVICE_ACTION_BLOCK_ERASE                                              0x02
#define SERVICE_ACTION_CRYPTO_ERASE                                             0x03
#define SERVICE_ACTION_EXIT_FAILURE                                             0x1f

// SCSIOP_OPERATION32 (0x7F)

#define SERVICE_ACTION_XDWRITE                                                  0x0004
#define SERVICE_ACTION_XPWRITE                                                  0x0006
#define SERVICE_ACTION_XDWRITEREAD                                              0x0007
#define SERVICE_ACTION_WRITE                                                    0x000B
#define SERVICE_ACTION_WRITE_VERIFY                                             0x000C
#define SERVICE_ACTION_WRITE_SAME                                               0x000D
#define SERVICE_ACTION_ORWRITE                                                  0x000E

// SCSIOP_POPULATE_TOKEN, SCSIOP_WRITE_USING_TOKEN (0x83)

#define SERVICE_ACTION_POPULATE_TOKEN                                           0x10
#define SERVICE_ACTION_WRITE_USING_TOKEN                                        0x11

// SCSIOP_RECEIVE_ROD_TOKEN_INFORMATION (0x84)

#define SERVICE_ACTION_RECEIVE_TOKEN_INFORMATION                                0x07

// SCSIOP_ZBC_OUT (0x94)

#define SERVICE_ACTION_CLOSE_ZONE                                               0x01
#define SERVICE_ACTION_FINISH_ZONE                                              0x02
#define SERVICE_ACTION_OPEN_ZONE                                                0x03
#define SERVICE_ACTION_RESET_WRITE_POINTER                                      0x04

// SCSIOP_ZBC_IN (0x95)

#define SERVICE_ACTION_REPORT_ZONES                                             0x00

#define REPORT_ZONES_OPTION_LIST_ALL_ZONES                                      0x00
#define REPORT_ZONES_OPTION_LIST_EMPTY_ZONES                                    0x01
#define REPORT_ZONES_OPTION_LIST_IMPLICITLY_OPENED_ZONES                        0x02
#define REPORT_ZONES_OPTION_LIST_EXPLICITLY_OPENED_ZONES                        0x03
#define REPORT_ZONES_OPTION_LIST_CLOSED_ZONES                                   0x04
#define REPORT_ZONES_OPTION_LIST_FULL_ZONES                                     0x05
#define REPORT_ZONES_OPTION_LIST_READ_ONLY_ZONES                                0x06
#define REPORT_ZONES_OPTION_LIST_OFFLINE_ZONES                                  0x07
#define REPORT_ZONES_OPTION_LIST_RWP_ZONES                                      0x10
#define REPORT_ZONES_OPTION_LIST_NON_SEQUENTIAL_WRITE_RESOURCES_ACTIVE_ZONES    0x11
#define REPORT_ZONES_OPTION_LIST_NOT_WRITE_POINTER_ZONES                        0x3F

// SCSIOP_SERVICE_ACTION_IN16 (0x9E)

#define SERVICE_ACTION_READ_CAPACITY16                                          0x10
#define SERVICE_ACTION_GET_LBA_STATUS                                           0x12
#define SERVICE_ACTION_GET_PHYSICAL_ELEMENT_STATUS                              0x17
#define SERVICE_ACTION_REMOVE_ELEMENT_AND_TRUNCATE                              0x18

// SCSIOP_MAINTENANCE_IN (0xA3)

#define SERVICE_ACTION_REPORT_TIMESTAMP                                         0x0F

// SCSIOP_MAINTENANCE_OUT (0xA4)

#define SERVICE_ACTION_SET_TIMESTAMP                                            0x0F

#define CDB_RETURN_ON_COMPLETION   0
#define CDB_RETURN_IMMEDIATE       1

#define CDB_FORCE_MEDIA_ACCESS 0x08

#define SCSIOP_DENON_EJECT_DISC    0xE6
#define SCSIOP_DENON_STOP_AUDIO    0xE7
#define SCSIOP_DENON_PLAY_AUDIO    0xE8
#define SCSIOP_DENON_READ_TOC      0xE9
#define SCSIOP_DENON_READ_SUBCODE  0xEB

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

/* SCSI_REQUEST_BLOCK.Function constants */
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
#define SRB_FUNCTION_FREE_DUMP_POINTERS     0x27

#if (NTDDI_VERSION >= NTDDI_WIN8)
#define SRB_FUNCTION_PROTOCOL_COMMAND       0x09
#define SRB_FUNCTION_QUIESCE_DEVICE         0x1A
#define SRB_FUNCTION_STORAGE_REQUEST_BLOCK  0x28
#define SRB_FUNCTION_GET_DUMP_INFO          0x2A
#define SRB_FUNCTION_FREE_DUMP_INFO         0x2B
#endif

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)
#define SRB_FUNCTION_EXECUTE_NVME           0x0A
#define SRB_FUNCTION_NVMEOF_OPERATION       0x2C
#define SRB_FUNCTION_MINIPORT_PASSTHROUGH_REQUEST 0x2D
#endif

/* SCSI_REQUEST_BLOCK.SrbStatus constants */
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
#define SRB_STATUS_ACCESS_DENIED            0x17
#define SRB_STATUS_OPERATION_IN_PROGRESS    0x18
#define SRB_STATUS_INVALID_LUN              0x20
#define SRB_STATUS_INVALID_TARGET_ID        0x21
#define SRB_STATUS_BAD_FUNCTION             0x22
#define SRB_STATUS_ERROR_RECOVERY           0x23
#define SRB_STATUS_NOT_POWERED              0x24
#define SRB_STATUS_LINK_DOWN                0x25
#define SRB_STATUS_INSUFFICIENT_RESOURCES   0x26
#define SRB_STATUS_THROTTLED_REQUEST        0x27
#define SRB_STATUS_INVALID_PARAMETER        0x28
#define SRB_STATUS_INTERNAL_ERROR           0x30
#define SRB_STATUS_QUEUE_FROZEN             0x40
#define SRB_STATUS_AUTOSENSE_VALID          0x80
#define SRB_STATUS(Status)                  (Status & ~(SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_QUEUE_FROZEN))

/* SCSI_REQUEST_BLOCK.SrbFlags constants */
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
#define SRB_FLAGS_D3_PROCESSING             0x00000800
#define SRB_FLAGS_SEQUENTIAL_REQUIRED       0x00001000

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
#define SRB_IOCTL_FLAGS_ADAPTER_REQUEST     0x01
#define SRB_PROTOCOL_FLAGS_ADAPTER_REQUEST  0x01

#define STOR_MAP_NO_BUFFERS                 (0)
#define STOR_MAP_ALL_BUFFERS                (1)
#define STOR_MAP_NON_READ_WRITE_BUFFERS     (2)
#define STOR_MAP_ALL_BUFFERS_INCLUDING_READ_WRITE (3)

/* StorPortExtendedFunction and the inline wrappers around it return these */
#define STOR_STATUS_SUCCESS                 (0x00000000L)
#define STOR_STATUS_UNSUCCESSFUL            (0xC1000001L)
#define STOR_STATUS_NOT_IMPLEMENTED         (0xC1000002L)
#define STOR_STATUS_INSUFFICIENT_RESOURCES  (0xC1000003L)
#define STOR_STATUS_BUFFER_TOO_SMALL        (0xC1000004L)
#define STOR_STATUS_ACCESS_DENIED           (0xC1000005L)
#define STOR_STATUS_INVALID_PARAMETER       (0xC1000006L)
#define STOR_STATUS_INVALID_DEVICE_REQUEST  (0xC1000007L)
#define STOR_STATUS_INVALID_IRQL            (0xC1000008L)
#define STOR_STATUS_INVALID_DEVICE_STATE    (0xC1000009L)
#define STOR_STATUS_INVALID_BUFFER_SIZE     (0xC100000AL)
#define STOR_STATUS_UNSUPPORTED_VERSION     (0xC100000BL)
#define STOR_STATUS_BUSY                    (0xC100000CL)
#define STOR_STATUS_THROTTLED_REQUEST       (0xC100000DL)
#define STOR_STATUS_TIMEOUT                 (0xC100000EL)
#define STOR_STATUS_INVALID_DATA            (0xC100000FL)
#define STOR_STATUS_RESET_REQUIRED          (0xC1000010L)
#define STOR_STATUS_NOT_SUPPORTED           (0xC1000011L)

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

#define NOTIFICATION_OPERATIONAL_CHANGE_CLASS_MASK    0x02
#define NOTIFICATION_POWER_MANAGEMENT_CLASS_MASK      0x04
#define NOTIFICATION_EXTERNAL_REQUEST_CLASS_MASK      0x08
#define NOTIFICATION_MEDIA_STATUS_CLASS_MASK          0x10
#define NOTIFICATION_MULTI_HOST_CLASS_MASK            0x20
#define NOTIFICATION_DEVICE_BUSY_CLASS_MASK           0x40


#define NOTIFICATION_NO_CLASS_EVENTS                  0x0
#define NOTIFICATION_OPERATIONAL_CHANGE_CLASS_EVENTS  0x1
#define NOTIFICATION_POWER_MANAGEMENT_CLASS_EVENTS    0x2
#define NOTIFICATION_EXTERNAL_REQUEST_CLASS_EVENTS    0x3
#define NOTIFICATION_MEDIA_STATUS_CLASS_EVENTS        0x4
#define NOTIFICATION_MULTI_HOST_CLASS_EVENTS          0x5
#define NOTIFICATION_DEVICE_BUSY_CLASS_EVENTS         0x6

#define NOTIFICATION_OPERATIONAL_EVENT_NO_CHANGE         0x0
#define NOTIFICATION_OPERATIONAL_EVENT_CHANGE_REQUESTED  0x1
#define NOTIFICATION_OPERATIONAL_EVENT_CHANGE_OCCURRED   0x2

#define NOTIFICATION_OPERATIONAL_STATUS_AVAILABLE        0x0
#define NOTIFICATION_OPERATIONAL_STATUS_TEMPORARY_BUSY   0x1
#define NOTIFICATION_OPERATIONAL_STATUS_EXTENDED_BUSY    0x2

#define NOTIFICATION_OPERATIONAL_OPCODE_NONE             0x0
#define NOTIFICATION_OPERATIONAL_OPCODE_FEATURE_CHANGE   0x1
#define NOTIFICATION_OPERATIONAL_OPCODE_FEATURE_ADDED    0x2
#define NOTIFICATION_OPERATIONAL_OPCODE_UNIT_RESET       0x3
#define NOTIFICATION_OPERATIONAL_OPCODE_FIRMWARE_CHANGED 0x4
#define NOTIFICATION_OPERATIONAL_OPCODE_INQUIRY_CHANGED  0x5

#define NOTIFICATION_POWER_EVENT_NO_CHANGE          0x0
#define NOTIFICATION_POWER_EVENT_CHANGE_SUCCEEDED   0x1
#define NOTIFICATION_POWER_EVENT_CHANGE_FAILED      0x2

#define NOTIFICATION_POWER_STATUS_ACTIVE            0x1
#define NOTIFICATION_POWER_STATUS_IDLE              0x2
#define NOTIFICATION_POWER_STATUS_STANDBY           0x3
#define NOTIFICATION_POWER_STATUS_SLEEP             0x4

#define NOTIFICATION_MEDIA_EVENT_NO_EVENT           0x0
#define NOTIFICATION_EXTERNAL_EVENT_NO_CHANGE       0x0
#define NOTIFICATION_EXTERNAL_EVENT_BUTTON_DOWN     0x1
#define NOTIFICATION_EXTERNAL_EVENT_BUTTON_UP       0x2
#define NOTIFICATION_EXTERNAL_EVENT_EXTERNAL        0x3

#define NOTIFICATION_EXTERNAL_STATUS_READY          0x0
#define NOTIFICATION_EXTERNAL_STATUS_PREVENT        0x1

#define NOTIFICATION_EXTERNAL_REQUEST_NONE          0x0000
#define NOTIFICATION_EXTERNAL_REQUEST_QUEUE_OVERRUN 0x0001
#define NOTIFICATION_EXTERNAL_REQUEST_PLAY          0x0101
#define NOTIFICATION_EXTERNAL_REQUEST_REWIND_BACK   0x0102
#define NOTIFICATION_EXTERNAL_REQUEST_FAST_FORWARD  0x0103
#define NOTIFICATION_EXTERNAL_REQUEST_PAUSE         0x0104
#define NOTIFICATION_EXTERNAL_REQUEST_STOP          0x0106
#define NOTIFICATION_EXTERNAL_REQUEST_ASCII_LOW     0x0200
#define NOTIFICATION_EXTERNAL_REQUEST_ASCII_HIGH    0x02ff

#define NOTIFICATION_MEDIA_EVENT_NO_CHANGE          0x0
#define NOTIFICATION_MEDIA_EVENT_EJECT_REQUEST      0x1
#define NOTIFICATION_MEDIA_EVENT_NEW_MEDIA          0x2
#define NOTIFICATION_MEDIA_EVENT_MEDIA_REMOVAL      0x3
#define NOTIFICATION_MEDIA_EVENT_MEDIA_CHANGE       0x4

#define NOTIFICATION_BUSY_EVENT_NO_EVENT               0x0
#define NOTIFICATION_MULTI_HOST_EVENT_NO_CHANGE        0x0
#define NOTIFICATION_MULTI_HOST_EVENT_CONTROL_REQUEST  0x1
#define NOTIFICATION_MULTI_HOST_EVENT_CONTROL_GRANT    0x2
#define NOTIFICATION_MULTI_HOST_EVENT_CONTROL_RELEASE  0x3

#define NOTIFICATION_MULTI_HOST_STATUS_READY           0x0
#define NOTIFICATION_MULTI_HOST_STATUS_PREVENT         0x1

#define NOTIFICATION_MULTI_HOST_PRIORITY_NO_REQUESTS   0x0
#define NOTIFICATION_MULTI_HOST_PRIORITY_LOW           0x1
#define NOTIFICATION_MULTI_HOST_PRIORITY_MEDIUM        0x2
#define NOTIFICATION_MULTI_HOST_PRIORITY_HIGH          0x3

#define NOTIFICATION_BUSY_EVENT_NO_EVENT            0x0
#define NOTIFICATION_BUSY_EVENT_NO_CHANGE           0x0
#define NOTIFICATION_BUSY_EVENT_BUSY                0x1
#define NOTIFICATION_BUSY_EVENT_LO_CHANGE           0x2

#define NOTIFICATION_BUSY_STATUS_NO_EVENT           0x0
#define NOTIFICATION_BUSY_STATUS_POWER              0x1
#define NOTIFICATION_BUSY_STATUS_IMMEDIATE          0x2
#define NOTIFICATION_BUSY_STATUS_DEFERRED           0x3

#define DVD_FORMAT_LEAD_IN          0x00
#define DVD_FORMAT_COPYRIGHT        0x01
#define DVD_FORMAT_DISK_KEY         0x02
#define DVD_FORMAT_BCA              0x03
#define DVD_FORMAT_MANUFACTURING    0x04

#define DVD_REPORT_AGID            0x00
#define DVD_CHALLENGE_KEY          0x01
#define DVD_KEY_1                  0x02
#define DVD_KEY_2                  0x03
#define DVD_TITLE_KEY              0x04
#define DVD_REPORT_ASF             0x05
#define DVD_INVALIDATE_AGID        0x3F

#define BLANK_FULL              0x0
#define BLANK_MINIMAL           0x1
#define BLANK_TRACK             0x2
#define BLANK_UNRESERVE_TRACK   0x3
#define BLANK_TAIL              0x4
#define BLANK_UNCLOSE_SESSION   0x5
#define BLANK_SESSION           0x6

#define CD_EXPECTED_SECTOR_ANY          0x0
#define CD_EXPECTED_SECTOR_CDDA         0x1
#define CD_EXPECTED_SECTOR_MODE1        0x2
#define CD_EXPECTED_SECTOR_MODE2        0x3
#define CD_EXPECTED_SECTOR_MODE2_FORM1  0x4
#define CD_EXPECTED_SECTOR_MODE2_FORM2  0x5

#define DISK_STATUS_EMPTY       0x00
#define DISK_STATUS_INCOMPLETE  0x01
#define DISK_STATUS_COMPLETE    0x02
#define DISK_STATUS_OTHERS      0x03

#define LAST_SESSION_EMPTY              0x00
#define LAST_SESSION_INCOMPLETE         0x01
#define LAST_SESSION_RESERVED_DAMAGED   0x02
#define LAST_SESSION_COMPLETE           0x03

#define DISK_TYPE_CDDA          0x00
#define DISK_TYPE_CDI           0x10
#define DISK_TYPE_XA            0x20
#define DISK_TYPE_UNDEFINED     0xFF

#define DISC_BGFORMAT_STATE_NONE        0x0
#define DISC_BGFORMAT_STATE_INCOMPLETE  0x1
#define DISC_BGFORMAT_STATE_RUNNING     0x2
#define DISC_BGFORMAT_STATE_COMPLETE    0x3

#define DATA_BLOCK_MODE0    0x0
#define DATA_BLOCK_MODE1    0x1
#define DATA_BLOCK_MODE2    0x2

/* READ_TOC formats */
#define READ_TOC_FORMAT_TOC         0x00
#define READ_TOC_FORMAT_SESSION     0x01
#define READ_TOC_FORMAT_FULL_TOC    0x02
#define READ_TOC_FORMAT_PMA         0x03
#define READ_TOC_FORMAT_ATIP        0x04

#define SCSIMESS_ABORT                0x06
#define SCSIMESS_ABORT_WITH_TAG       0x0D
#define SCSIMESS_BUS_DEVICE_RESET     0X0C
#define SCSIMESS_CLEAR_QUEUE          0X0E
#define SCSIMESS_COMMAND_COMPLETE     0X00
#define SCSIMESS_DISCONNECT           0X04
#define SCSIMESS_EXTENDED_MESSAGE     0X01
#define SCSIMESS_IDENTIFY             0X80
#define SCSIMESS_IDENTIFY_WITH_DISCON 0XC0
#define SCSIMESS_IGNORE_WIDE_RESIDUE  0X23
#define SCSIMESS_INITIATE_RECOVERY    0X0F
#define SCSIMESS_INIT_DETECTED_ERROR  0X05
#define SCSIMESS_LINK_CMD_COMP        0X0A
#define SCSIMESS_LINK_CMD_COMP_W_FLAG 0X0B
#define SCSIMESS_MESS_PARITY_ERROR    0X09
#define SCSIMESS_MESSAGE_REJECT       0X07
#define SCSIMESS_NO_OPERATION         0X08
#define SCSIMESS_HEAD_OF_QUEUE_TAG    0X21
#define SCSIMESS_ORDERED_QUEUE_TAG    0X22
#define SCSIMESS_SIMPLE_QUEUE_TAG     0X20
#define SCSIMESS_RELEASE_RECOVERY     0X10
#define SCSIMESS_RESTORE_POINTERS     0X03
#define SCSIMESS_SAVE_DATA_POINTER    0X02
#define SCSIMESS_TERMINATE_IO_PROCESS 0X11

#define SCSIMESS_MODIFY_DATA_POINTER  0X00
#define SCSIMESS_SYNCHRONOUS_DATA_REQ 0X01
#define SCSIMESS_WIDE_DATA_REQUEST    0X03

#define SCSIMESS_MODIFY_DATA_LENGTH   5
#define SCSIMESS_SYNCH_DATA_LENGTH    3
#define SCSIMESS_WIDE_DATA_LENGTH     2

#define CDB_INQUIRY_EVPD           0x01

#define LUN0_FORMAT_SAVING_DEFECT_LIST 0
#define USE_DEFAULTMSB 0
#define USE_DEFAULTLSB 0

#define START_UNIT_CODE 0x01
#define STOP_UNIT_CODE 0x00

#define DEVICE_QUALIFIER_ACTIVE           0x00
#define DEVICE_QUALIFIER_NOT_ACTIVE       0x01
#define DEVICE_QUALIFIER_NOT_SUPPORTED    0x03

#define RESERVATION_ACTION_READ_KEYS                    0x00
#define RESERVATION_ACTION_READ_RESERVATIONS            0x01

#define RESERVATION_ACTION_REGISTER                     0x00
#define RESERVATION_ACTION_RESERVE                      0x01
#define RESERVATION_ACTION_RELEASE                      0x02
#define RESERVATION_ACTION_CLEAR                        0x03
#define RESERVATION_ACTION_PREEMPT                      0x04
#define RESERVATION_ACTION_PREEMPT_ABORT                0x05
#define RESERVATION_ACTION_REGISTER_IGNORE_EXISTING     0x06

#define RESERVATION_SCOPE_LU                            0x00
#define RESERVATION_SCOPE_ELEMENT                       0x02

#define RESERVATION_TYPE_WRITE_EXCLUSIVE                0x01
#define RESERVATION_TYPE_EXCLUSIVE                      0x03
#define RESERVATION_TYPE_WRITE_EXCLUSIVE_REGISTRANTS    0x05
#define RESERVATION_TYPE_EXCLUSIVE_REGISTRANTS          0x06

#define SENSE_BUFFER_SIZE_EX sizeof(SENSE_DATA_EX)

#define MAX_ADDITIONAL_SENSE_BYTES (MAX_SENSE_BUFFER_SIZE - SENSE_BUFFER_SIZE)
#define MAX_ADDITIONAL_SENSE_BYTES_EX (MAX_SENSE_BUFFER_SIZE - SENSE_BUFFER_SIZE_EX)

#define SCSI_SENSE_ERRORCODE_FIXED_CURRENT        0x70
#define SCSI_SENSE_ERRORCODE_FIXED_DEFERRED       0x71
#define SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT   0x72
#define SCSI_SENSE_ERRORCODE_DESCRIPTOR_DEFERRED  0x73

// Sense Descriptor Types

#define SCSI_SENSE_DESCRIPTOR_TYPE_INFORMATION                  0x00
#define SCSI_SENSE_DESCRIPTOR_TYPE_COMMAND_SPECIFIC             0x01
#define SCSI_SENSE_DESCRIPTOR_TYPE_SENSE_KEY_SPECIFIC           0x02
#define SCSI_SENSE_DESCRIPTOR_TYPE_FIELD_REPLACEABLE_UNIT       0x03
#define SCSI_SENSE_DESCRIPTOR_TYPE_STREAM_COMMAND               0x04
#define SCSI_SENSE_DESCRIPTOR_TYPE_BLOCK_COMMAND                0x05
#define SCSI_SENSE_DESCRIPTOR_TYPE_OSD_OBJECT_IDENTIFICATION    0x06
#define SCSI_SENSE_DESCRIPTOR_TYPE_OSD_RESPONSE_INTEGRITY_CHECK 0x07
#define SCSI_SENSE_DESCRIPTOR_TYPE_OSD_ATTRIBUTE_IDENTIFICATION 0x08
#define SCSI_SENSE_DESCRIPTOR_TYPE_ATA_STATUS_RETURN            0x09
#define SCSI_SENSE_DESCRIPTOR_TYPE_PROGRESS_INDICATION          0x0A
#define SCSI_SENSE_DESCRIPTOR_TYPE_USER_DATA_SEGMENT_REFERRAL   0x0B

/* Additional tape bit */
#define SCSI_ILLEGAL_LENGTH               0x20
#define SCSI_EOM                          0x40
#define SCSI_FILE_MARK                    0x80

/* Additional Sense codes */
#define SCSI_ADSENSE_NO_SENSE                              0x00
#define SCSI_ADSENSE_NO_SEEK_COMPLETE                      0x02
#define SCSI_ADSENSE_WRITE                                 0x03
#define SCSI_ADSENSE_LUN_NOT_READY                         0x04
#define SCSI_ADSENSE_LUN_COMMUNICATION                     0x08
#define SCSI_ADSENSE_SERVO_ERROR                           0x09
#define SCSI_ADSENSE_WARNING                               0x0B
#define SCSI_ADSENSE_WRITE_ERROR                           0x0C
#define SCSI_ADSENSE_COPY_TARGET_DEVICE_ERROR              0x0D
#define SCSI_ADSENSE_UNRECOVERED_ERROR                     0x11
#define SCSI_ADSENSE_TRACK_ERROR                           0x14
#define SCSI_ADSENSE_SEEK_ERROR                            0x15
#define SCSI_ADSENSE_REC_DATA_NOECC                        0x17
#define SCSI_ADSENSE_REC_DATA_ECC                          0x18
#define SCSI_ADSENSE_DEFECT_LIST_ERROR                     0x19
#define SCSI_ADSENSE_PARAMETER_LIST_LENGTH                 0x1A
#define SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY_OPERATION    0x1D
#define SCSI_ADSENSE_ILLEGAL_COMMAND                       0x20
#define SCSI_ADSENSE_ACCESS_DENIED                         0x20
#define SCSI_ADSENSE_ILLEGAL_BLOCK                         0x21
#define SCSI_ADSENSE_INVALID_TOKEN                         0x23
#define SCSI_ADSENSE_INVALID_CDB                           0x24
#define SCSI_ADSENSE_INVALID_LUN                           0x25
#define SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST          0x26
#define SCSI_ADSENSE_WRITE_PROTECT                         0x27
#define SCSI_ADSENSE_MEDIUM_CHANGED                        0x28
#define SCSI_ADSENSE_BUS_RESET                             0x29
#define SCSI_ADSENSE_PARAMETERS_CHANGED                    0x2A
#define SCSI_ADSENSE_INSUFFICIENT_TIME_FOR_OPERATION       0x2E
#define SCSI_ADSENSE_INVALID_MEDIA                         0x30
#define SCSI_ADSENSE_DEFECT_LIST                           0x32
#define SCSI_ADSENSE_LB_PROVISIONING                       0x38
#define SCSI_ADSENSE_NO_MEDIA_IN_DEVICE                    0x3a
#define SCSI_ADSENSE_POSITION_ERROR                        0x3b
#define SCSI_ADSENSE_LOGICAL_UNIT_ERROR                    0x3e
#define SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED          0x3f
#define SCSI_ADSENSE_DATA_PATH_FAILURE                     0x41
#define SCSI_ADSENSE_POWER_ON_SELF_TEST_FAILURE            0x42
#define SCSI_ADSENSE_INTERNAL_TARGET_FAILURE               0x44
#define SCSI_ADSENSE_DATA_TRANSFER_ERROR                   0x4b
#define SCSI_ADSENSE_LUN_FAILED_SELF_CONFIGURATION         0x4c
#define SCSI_ADSENSE_RESOURCE_FAILURE                      0x55
#define SCSI_ADSENSE_OPERATOR_REQUEST                      0x5a
#define SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED 0x5d
#define SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK           0x64
#define SCSI_ADSENSE_COPY_PROTECTION_FAILURE               0x6f
#define SCSI_ADSENSE_POWER_CALIBRATION_ERROR               0x73
#define SCSI_ADSENSE_VENDOR_UNIQUE                         0x80
#define SCSI_ADSENSE_MUSIC_AREA                            0xA0
#define SCSI_ADSENSE_DATA_AREA                             0xA1
#define SCSI_ADSENSE_VOLUME_OVERFLOW                       0xA7

#define SCSI_ADWRITE_PROTECT                        SCSI_ADSENSE_WRITE_PROTECT
#define SCSI_FAILURE_PREDICTION_THRESHOLD_EXCEEDED  SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED

// ADSENSE additional qualifiers

// SCSI_ADSENSE_NO_SENSE (0x00)

#define SCSI_SENSEQ_FILEMARK_DETECTED            0x01
#define SCSI_SENSEQ_END_OF_MEDIA_DETECTED        0x02
#define SCSI_SENSEQ_SETMARK_DETECTED             0x03
#define SCSI_SENSEQ_BEGINNING_OF_MEDIA_DETECTED  0x04
#define SCSI_SENSEQ_OPERATION_IS_IN_PROGRESS     0x16

// SCSI_ADSENSE_WRITE (0x03)

#define SCSI_SENSEQ_PERIPHERAL_DEVICE_WRITE_FAULT   0x00
#define SCSI_SENSEQ_NO_WRITE_CURRENT                0x01
#define SCSI_SENSEQ_EXCESSIVE_WRITE_ERRORS          0x02

// SCSI_ADSENSE_LUN_NOT_READY (0x04)

#define SCSI_SENSEQ_CAUSE_NOT_REPORTABLE         0x00
#define SCSI_SENSEQ_BECOMING_READY               0x01
#define SCSI_SENSEQ_INIT_COMMAND_REQUIRED        0x02
#define SCSI_SENSEQ_MANUAL_INTERVENTION_REQUIRED 0x03
#define SCSI_SENSEQ_FORMAT_IN_PROGRESS           0x04
#define SCSI_SENSEQ_REBUILD_IN_PROGRESS          0x05
#define SCSI_SENSEQ_RECALCULATION_IN_PROGRESS    0x06
#define SCSI_SENSEQ_OPERATION_IN_PROGRESS        0x07
#define SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS       0x08
#define SCSI_SENSEQ_SPACE_ALLOC_IN_PROGRESS      0x14

// SCSI_ADSENSE_LUN_COMMUNICATION (0x08)

#define SCSI_SENSEQ_COMM_FAILURE                 0x00
#define SCSI_SENSEQ_COMM_TIMEOUT                 0x01
#define SCSI_SENSEQ_COMM_PARITY_ERROR            0x02
#define SCSI_SESNEQ_COMM_CRC_ERROR               0x03
#define SCSI_SENSEQ_UNREACHABLE_TARGET           0x04

// SCSI_ADSENSE_SERVO_ERROR (0x09)

#define SCSI_SENSEQ_TRACK_FOLLOWING_ERROR   0x00
#define SCSI_SENSEQ_TRACKING_SERVO_FAILURE  0x01
#define SCSI_SENSEQ_FOCUS_SERVO_FAILURE     0x02
#define SCSI_SENSEQ_SPINDLE_SERVO_FAILURE   0x03
#define SCSI_SENSEQ_HEAD_SELECT_FAULT       0x04

// SCSI_ADSENSE_WARNING (0x0B)

#define SCSI_SENSEQ_POWER_LOSS_EXPECTED          0x08

// SCSI_ADSENSE_WRITE_ERROR (0x0C)

#define SCSI_SENSEQ_LOSS_OF_STREAMING            0x09
#define SCSI_SENSEQ_PADDING_BLOCKS_ADDED         0x0A

// SCSI_ADSENSE_COPY_TARGET_DEVICE_ERROR (0x0D)

#define SCSI_SENSEQ_NOT_REACHABLE                0x02
#define SCSI_SENSEQ_DATA_UNDERRUN                0x04

// SCSI_ADSENSE_UNRECOVERED_ERROR (0x11)

#define SCSI_SENSEQ_UNRECOVERED_READ_ERROR       0x00

// SCSI_ADSENSE_SEEK_ERROR (0x15)

#define SCSI_SENSEQ_RANDOM_POSITIONING_ERROR                        0x00
#define SCSI_SENSEQ_MECHANICAL_POSITIONING_ERROR                    0x01
#define SCSI_SENSEQ_POSITIONING_ERROR_DETECTED_BY_READ_OF_MEDIUM    0x02

// SCSI_ADSENSE_DEFECT_LIST_ERROR (0x19)

#define SCSI_SENSEQ_DEFECT_LIST_ERROR                  0x00
#define SCSI_SENSEQ_DEFECT_LIST_NOT_AVAILABLE          0x01
#define SCSI_SENSEQ_DEFECT_LIST_ERROR_IN_PRIMARY_LIST  0x02
#define SCSI_SENSEQ_DEFECT_LIST_ERROR_IN_GROWN_LIST    0x03

// SCSI_ADSENSE_ACCESS_DENIED (0x20)

#define SCSI_SENSEQ_NO_ACCESS_RIGHTS             0x02

// SCSI_ADSENSE_ILLEGAL_BLOCK (0x21)

#define SCSI_SENSEQ_LOGICAL_ADDRESS_OUT_OF_RANGE 0x00
#define SCSI_SENSEQ_ILLEGAL_ELEMENT_ADDR         0x01
#define SCSI_SENSEQ_INVALID_WRITE_ADDRESS        0x02
#define SCSI_SENSEQ_INVALID_WRITE_CROSSING_LAYER_JUMP 0x03
#define SCSI_SENSEQ_UNALIGNED_WRITE              0x04
#define SCSI_SENSEQ_WRITE_BOUNDARY_VIOLATION     0x05
#define SCSI_SENSEQ_READ_INVALID_DATA            0x06
#define SCSI_SENSEQ_READ_BOUNDARY_VIOLATION      0x07
#define SCSI_SENSEQ_MISALIGNED_WRITE             0x08

// SCSI_ADSENSE_INVALID_FIELD_PARAMETER_LIST (0x26)

#define SCSI_SENSEQ_INVALID_RELEASE_OF_PERSISTENT_RESERVATION 0x04
#define SCSI_SENSEQ_TOO_MANY_SEGMENT_DESCRIPTORS 0x08

// SCSI_ADSENSE_WRITE_PROTECT (0x27)

#define SCSI_SENSEQ_SPACE_ALLOC_FAILED_WRITE_PROTECT 0x07

// SCSI_ADSENSE_PARAMETERS_CHANGED (0x2A)

#define SCSI_SENSEQ_CAPACITY_DATA_CHANGED        0x09

// SCSI_ADSENSE_POSITION_ERROR (0x3b)

#define SCSI_SENSEQ_DESTINATION_FULL             0x0d
#define SCSI_SENSEQ_SOURCE_EMPTY                 0x0e

// SCSI_ADSENSE_INVALID_MEDIA (0x30)

#define SCSI_SENSEQ_INCOMPATIBLE_MEDIA_INSTALLED 0x00
#define SCSI_SENSEQ_UNKNOWN_FORMAT 0x01
#define SCSI_SENSEQ_INCOMPATIBLE_FORMAT 0x02
#define SCSI_SENSEQ_CLEANING_CARTRIDGE_INSTALLED 0x03

// SCSI_ADSENSE_DEFECT_LIST (0x32)

#define SCSI_SENSEQ_NO_DEFECT_SPARE_LOCATION_AVAILABLE  0x00
#define SCSI_SENSEQ_DEFECT_LIST_UPDATE_FAILURE          0x01

// SCSI_ADSENSE_LB_PROVISIONING (0x38)

#define SCSI_SENSEQ_SOFT_THRESHOLD_REACHED 0x07

// SCSI_ADSENSE_LOGICAL_UNIT_ERROR (0x3e)

#define SCSI_SENSEQ_LOGICAL_UNIT_HAS_NOT_SELF_CONFIGURED_YET    0x00
#define SCSI_SENSEQ_LOGICAL_UNIT_FAILURE                        0x01
#define SCSI_SENSEQ_TIMEOUT_ON_LOGICAL_UNIT                     0x02
#define SCSI_SENSEQ_LOGICAL_UNIT_FAILED_SELF_TEST               0x03
#define SCSI_SENSEQ_LOGICAL_UNIT_FAILED_TO_UPDATE_SELF_TEST_LOG 0x04

// SCSI_ADSENSE_OPERATING_CONDITIONS_CHANGED (0x3f)

#define SCSI_SENSEQ_TARGET_OPERATING_CONDITIONS_CHANGED 0x00
#define SCSI_SENSEQ_MICROCODE_CHANGED                   0x01
#define SCSI_SENSEQ_OPERATING_DEFINITION_CHANGED        0x02
#define SCSI_SENSEQ_INQUIRY_DATA_CHANGED                0x03
#define SCSI_SENSEQ_COMPONENT_DEVICE_ATTACHED           0x04
#define SCSI_SENSEQ_DEVICE_IDENTIFIER_CHANGED           0x05
#define SCSI_SENSEQ_REDUNDANCY_GROUP_MODIFIED           0x06
#define SCSI_SENSEQ_REDUNDANCY_GROUP_DELETED            0x07
#define SCSI_SENSEQ_SPARE_MODIFIED                      0x08
#define SCSI_SENSEQ_SPARE_DELETED                       0x09
#define SCSI_SENSEQ_VOLUME_SET_MODIFIED                 0x0A
#define SCSI_SENSEQ_VOLUME_SET_DELETED                  0x0B
#define SCSI_SENSEQ_VOLUME_SET_DEASSIGNED               0x0C
#define SCSI_SENSEQ_VOLUME_SET_REASSIGNED               0x0D
#define SCSI_SENSEQ_REPORTED_LUNS_DATA_CHANGED          0x0E
#define SCSI_SENSEQ_ECHO_BUFFER_OVERWRITTEN             0x0F
#define SCSI_SENSEQ_MEDIUM_LOADABLE                     0x10
#define SCSI_SENSEQ_MEDIUM_AUXILIARY_MEMORY_ACCESSIBLE  0x11

// SCSI_ADSENSE_INTERNAL_TARGET_FAILURE (0x44)

#define SCSI_SENSEQ_INTERNAL_TARGET_FAILURE                 0x00
#define SCSI_SENSEQ_PRESISTENT_RESERVATION_INFORMATION_LOST 0x01
#define SCSI_SENSEQ_ATA_DEVICE_FAILED_SET_FEATURES          0x71

// SCSI_ADSENSE_DATA_TRANSFER_ERROR (0x4b)

#define SCSI_SENSEQ_INITIATOR_RESPONSE_TIMEOUT          0x06

// SCSI_ADSENSE_RESOURCE_FAILURE (0x55)

#define SCSI_SENSEQ_SYSTEM_RESOURCE_FAILURE             0x00
#define SCSI_SENSEQ_SYSTEM_BUFFER_FULL                  0x01
#define SCSI_SENSEQ_INSUFFICIENT_RESERVATION_RESOURCES  0x02
#define SCSI_SENSEQ_INSUFFICIENT_RESOURCES              0x03

// SCSI_ADSENSE_OPERATOR_REQUEST (0x5a)

#define SCSI_SENSEQ_STATE_CHANGE_INPUT                  0x00
#define SCSI_SENSEQ_MEDIUM_REMOVAL                      0x01
#define SCSI_SENSEQ_WRITE_PROTECT_ENABLE                0x02
#define SCSI_SENSEQ_WRITE_PROTECT_DISABLE               0x03

// SCSI_ADSENSE_FAILURE_PREDICTION_THRESHOLD_EXCEEDED (0x5d)

#define SCSI_SENSEQ_FAILURE_PREDICTION_THRESHOLD_EXCEEDED               0x00
#define SCSI_SENSEQ_MEDIA_FAILURE_PREDICTION_THRESHOLD_EXCEEDED         0x01
#define SCSI_SENSEQ_LUN_FAILURE_PREDICTION_THRESHOLD_EXCEEDED           0x02
#define SCSI_SENSEQ_SPARE_AREA_EXHAUSTION_PREDICTION_THRESHOLD_EXCEEDED 0x03
#define SCSI_SENSEQ_GENERAL_HARD_DRIVE_FAILURE                          0x10
#define SCSI_SENSEQ_DRIVE_ERROR_RATE_TOO_HIGH                           0x11
#define SCSI_SENSEQ_DATA_ERROR_RATE_TOO_HIGH                            0x12
#define SCSI_SENSEQ_SEEK_ERROR_RATE_TOO_HIGH                            0x13
#define SCSI_SENSEQ_TOO_MANY_BLOCK_REASSIGNS                            0x14
#define SCSI_SENSEQ_ACCESS_TIMES_TOO_HIGH                               0x15
#define SCSI_SENSEQ_START_UNIT_TIMES_TOO_HIGH                           0x16
#define SCSI_SENSEQ_CHANNEL_PARAMETRICS                                 0x17
#define SCSI_SENSEQ_CONTROLLER_DETECTED                                 0x18
#define SCSI_SENSEQ_THROUGHPUT_PERFORMANCE                              0x19
#define SCSI_SENSEQ_SEEK_TIME_PERFORMANCE                               0x1A
#define SCSI_SENSEQ_SPIN_UP_RETRY_COUNT                                 0x1B
#define SCSI_SENSEQ_DRIVE_CALIBRATION_RETRY_COUNT                       0x1C
#define SCSI_SENSEQ_DATA_CHANNEL_DATA_ERROR_RATE_TOO_HIGH               0x32
#define SCSI_SENSEQ_SERVO_DATA_ERROR_RATE_TOO_HIGH                      0x42
#define SCSI_SENSEQ_SERVER_SEEK_ERROR_RATE_TOO_HIGH                     0x43
#define SCSI_SENSEQ_FAILURE_PREDICTION_THRESHOLD_EXCEEDED_FALSE         0xFF

// SCSI_ADSENSE_COPY_PROTECTION_FAILURE (0x6f)

#define SCSI_SENSEQ_AUTHENTICATION_FAILURE                          0x00
#define SCSI_SENSEQ_KEY_NOT_PRESENT                                 0x01
#define SCSI_SENSEQ_KEY_NOT_ESTABLISHED                             0x02
#define SCSI_SENSEQ_READ_OF_SCRAMBLED_SECTOR_WITHOUT_AUTHENTICATION 0x03
#define SCSI_SENSEQ_MEDIA_CODE_MISMATCHED_TO_LOGICAL_UNIT           0x04
#define SCSI_SENSEQ_LOGICAL_UNIT_RESET_COUNT_ERROR                  0x05

// SCSI_ADSENSE_POWER_CALIBRATION_ERROR (0x73)

#define SCSI_SENSEQ_POWER_CALIBRATION_AREA_ALMOST_FULL  0x01
#define SCSI_SENSEQ_POWER_CALIBRATION_AREA_FULL         0x02
#define SCSI_SENSEQ_POWER_CALIBRATION_AREA_ERROR        0x03
#define SCSI_SENSEQ_PMA_RMA_UPDATE_FAILURE              0x04
#define SCSI_SENSEQ_PMA_RMA_IS_FULL                     0x05
#define SCSI_SENSEQ_PMA_RMA_ALMOST_FULL                 0x06

/* SMART support in ATAPI */
#define IOCTL_SCSI_MINIPORT_SMART_VERSION               ((FILE_DEVICE_SCSI << 16) + 0x0500)
#define IOCTL_SCSI_MINIPORT_IDENTIFY                    ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define IOCTL_SCSI_MINIPORT_READ_SMART_ATTRIBS          ((FILE_DEVICE_SCSI << 16) + 0x0502)
#define IOCTL_SCSI_MINIPORT_READ_SMART_THRESHOLDS       ((FILE_DEVICE_SCSI << 16) + 0x0503)
#define IOCTL_SCSI_MINIPORT_ENABLE_SMART                ((FILE_DEVICE_SCSI << 16) + 0x0504)
#define IOCTL_SCSI_MINIPORT_DISABLE_SMART               ((FILE_DEVICE_SCSI << 16) + 0x0505)
#define IOCTL_SCSI_MINIPORT_RETURN_STATUS               ((FILE_DEVICE_SCSI << 16) + 0x0506)
#define IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTOSAVE     ((FILE_DEVICE_SCSI << 16) + 0x0507)
#define IOCTL_SCSI_MINIPORT_SAVE_ATTRIBUTE_VALUES       ((FILE_DEVICE_SCSI << 16) + 0x0508)
#define IOCTL_SCSI_MINIPORT_EXECUTE_OFFLINE_DIAGS       ((FILE_DEVICE_SCSI << 16) + 0x0509)
#define IOCTL_SCSI_MINIPORT_ENABLE_DISABLE_AUTO_OFFLINE ((FILE_DEVICE_SCSI << 16) + 0x050a)
#define IOCTL_SCSI_MINIPORT_READ_SMART_LOG              ((FILE_DEVICE_SCSI << 16) + 0x050b)
#define IOCTL_SCSI_MINIPORT_WRITE_SMART_LOG             ((FILE_DEVICE_SCSI << 16) + 0x050c)

/* CLUSTER support */
#define IOCTL_SCSI_MINIPORT_NOT_QUORUM_CAPABLE ((FILE_DEVICE_SCSI << 16) + 0x0520)
#define IOCTL_SCSI_MINIPORT_NOT_CLUSTER_CAPABLE ((FILE_DEVICE_SCSI << 16) + 0x0521)

#define MODE_FD_SINGLE_SIDE               0x01
#define MODE_FD_DOUBLE_SIDE               0x02
#define MODE_FD_MAXIMUM_TYPE              0x1E
#define MODE_DSP_FUA_SUPPORTED            0x10
#define MODE_DSP_WRITE_PROTECT            0x80

#define CDDA_CHANNEL_MUTED      0x0
#define CDDA_CHANNEL_ZERO       0x1
#define CDDA_CHANNEL_ONE        0x2
#define CDDA_CHANNEL_TWO        0x4
#define CDDA_CHANNEL_THREE      0x8

#define CDVD_LMT_CADDY              0
#define CDVD_LMT_TRAY               1
#define CDVD_LMT_POPUP              2
#define CDVD_LMT_RESERVED1          3
#define CDVD_LMT_CHANGER_INDIVIDUAL 4
#define CDVD_LMT_CHANGER_CARTRIDGE  5
#define CDVD_LMT_RESERVED2          6
#define CDVD_LMT_RESERVED3          7

#define LOADING_MECHANISM_CADDY                 0x00
#define LOADING_MECHANISM_TRAY                  0x01
#define LOADING_MECHANISM_POPUP                 0x02
#define LOADING_MECHANISM_INDIVIDUAL_CHANGER    0x04
#define LOADING_MECHANISM_CARTRIDGE_CHANGER     0x05

#define MODE_BLOCK_DESC_LENGTH        8
#define MODE_HEADER_LENGTH            4
#define MODE_HEADER_LENGTH10          8

/* CDROM audio control */
#define CDB_AUDIO_PAUSE                   0x00
#define CDB_AUDIO_RESUME                  0x01
#define CDB_DEVICE_START                  0x11
#define CDB_DEVICE_STOP                   0x10
#define CDB_EJECT_MEDIA                   0x10
#define CDB_LOAD_MEDIA                    0x01
#define CDB_SUBCHANNEL_HEADER             0x00
#define CDB_SUBCHANNEL_BLOCK              0x01

#define CDROM_AUDIO_CONTROL_PAGE          0x0E
#define MODE_SELECT_IMMEDIATE             0x04
#define MODE_SELECT_PFBIT                 0x10

#define CDB_USE_MSF                       0x01

/* Multisession CDROMs */
#define GET_LAST_SESSION 0x01
#define GET_SESSION_DATA 0x02

#define VPD_MAX_BUFFER_SIZE                0xff

#define VPD_ATA_INFORMATION                0x89

#define VPD_THIRD_PARTY_COPY               0x8F
#define VPD_BLOCK_LIMITS                   0xB0
#define VPD_BLOCK_DEVICE_CHARACTERISTICS   0xB1
#define VPD_LOGICAL_BLOCK_PROVISIONING     0xB2
#define VPD_ZONED_BLOCK_DEVICE_CHARACTERISTICS  0xB6

#define BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR_TYPE_WINDOWS       0x00

#define ZONED_CAPABILITIES_NOT_REPORTED       0x0
#define ZONED_CAPABILITIES_HOST_AWARE         0x1
#define ZONED_CAPABILITIES_DEVICE_MANAGED     0x2

#define PROVISIONING_TYPE_UNKNOWN       0x0
#define PROVISIONING_TYPE_RESOURCE      0x1
#define PROVISIONING_TYPE_THIN          0x2

#define RC_BASIS_LAST_LBA_NOT_SEQUENTIAL_WRITE_REQUIRED_ZONES       0x0
#define RC_BASIS_LAST_LBA_ON_LOGICAL_UNIT                           0x1

#define LBA_STATUS_MAPPED      0x0
#define LBA_STATUS_DEALLOCATED 0x1
#define LBA_STATUS_ANCHORED    0x2

#define LOG_PAGE_CODE_SUPPORTED_LOG_PAGES           0x00
#define LOG_PAGE_CODE_WRITE_ERROR_COUNTERS          0x02
#define LOG_PAGE_CODE_READ_ERROR_COUNTERS           0x03
#define LOG_PAGE_CODE_LOGICAL_BLOCK_PROVISIONING    0x0C
#define LOG_PAGE_CODE_TEMPERATURE                   0x0D
#define LOG_PAGE_CODE_STARTSTOP_CYCLE_COUNTERS      0x0E
#define LOG_PAGE_CODE_SELFTEST_RESULTS              0x10
#define LOG_PAGE_CODE_SOLID_STATE_MEDIA             0x11
#define LOG_PAGE_CODE_BACKGROUND_SCAN_RESULTS       0x15
#define LOG_PAGE_CODE_INFORMATIONAL_EXCEPTIONS      0x2F

#define LOG_PAGE_LBP_PARAMETER_CODE_AVAILABLE   0x1
#define LOG_PAGE_LBP_PARAMETER_CODE_USED        0x2

#define LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_REPORTED            0x0
#define LOG_PAGE_LBP_RESOURCE_SCOPE_DEDICATED_TO_LUN        0x1
#define LOG_PAGE_LBP_RESOURCE_SCOPE_NOT_DEDICATED_TO_LUN    0x2

#define REVERSE_SHORT(Short) { \
  UCHAR _val; \
  PTWO_BYTE _val2 = (PTWO_BYTE)(Short); \
  _val = _val2->Byte0; \
  _val2->Byte0 = _val2->Byte1; \
  _val2->Byte1 = _val; \
}

#define REVERSE_LONG(Long) { \
  UCHAR _val; \
  PFOUR_BYTE _val2 = (PFOUR_BYTE)(Long); \
  _val = _val2->Byte3; \
  _val2->Byte3 = _val2->Byte0; \
  _val2->Byte0 = _val; \
  _val = _val2->Byte2; \
  _val2->Byte2 = _val2->Byte1; \
  _val2->Byte1 = _val; \
}

#define WHICH_BIT(Data, Bit) { \
  UCHAR _val; \
  for (_val = 0; _val < 32; _val++) { \
    if (((Data) >> _val) == 1) { \
      break; \
    } \
  } \
  ASSERT(_val != 32); \
  (Bit) = _val; \
}

#define BLOCK_DEVICE_TOKEN_SIZE         512

#define ScsiGetSenseErrorCode(SenseInfoBuffer) (((PUCHAR)(SenseInfoBuffer))[0] & 0x7f)

#define ScsiGetSenseDescriptorLength(DescriptorBuffer) \
            (sizeof(SCSI_SENSE_DESCRIPTOR_HEADER) + ((PSCSI_SENSE_DESCRIPTOR_HEADER)(DescriptorBuffer))->AdditionalLength)

#define IsFixedSenseDataFormat(SenseInfoBuffer) \
            ((ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_FIXED_CURRENT || \
             (ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_FIXED_DEFERRED)

#define IsDescriptorSenseDataFormat(SenseInfoBuffer) \
            ((ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT || \
             (ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_DESCRIPTOR_DEFERRED)

#define IsSenseDataCurrentError(SenseInfoBuffer) \
            ((ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_FIXED_CURRENT || \
             (ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_DESCRIPTOR_CURRENT)

#define IsSenseDataDeferredError(SenseInfoBuffer) \
            ((ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_FIXED_DEFERRED || \
             (ScsiGetSenseErrorCode(SenseInfoBuffer)) == SCSI_SENSE_ERRORCODE_DESCRIPTOR_DEFERRED)

#define IsSenseDataFormatValueValid(SenseInfoBuffer) \
            (IsFixedSenseDataFormat(SenseInfoBuffer) || IsDescriptorSenseDataFormat(SenseInfoBuffer))

#define SCSI_SENSE_OPTIONS_NONE ((SCSI_SENSE_OPTIONS)0x00000000)
#define SCSI_SENSE_OPTIONS_FIXED_FORMAT_IF_UNKNOWN_FORMAT_INDICATED ((SCSI_SENSE_OPTIONS)0x00000001)

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
    InvalidLock = 0,
    DpcLock = 1,
    StartIoLock,
    InterruptLock,
    ThreadedDpcLock,
    DpcLevelLock
} STOR_SPINLOCK;

typedef enum _INTERRUPT_SYNCHRONIZATION_MODE
{
    InterruptSupportNone,
    InterruptSynchronizeAll,
    InterruptSynchronizePerMessage
} INTERRUPT_SYNCHRONIZATION_MODE;

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)

/*
 * An NVMe miniport gets these instead of the SCSI adapter and unit control
 * codes. The adapter list deliberately mirrors SCSI_ADAPTER_CONTROL_TYPE.
 */
typedef enum _NVME_ADAPTER_CONTROL_TYPE
{
    NvmeQuerySupportedControlTypes = 0,
    NvmeStopAdapter,
    NvmeRestartAdapter,
    NvmeSetBootConfig,
    NvmeSetRunningConfig,
    NvmePowerSettingNotification,
    NvmeAdapterPower,
    NvmeAdapterPoFxPowerRequired,
    NvmeAdapterPoFxPowerActive,
    NvmeAdapterPoFxPowerSetFState,
    NvmeAdapterPoFxPowerControl,
    NvmeAdapterPrepareForBusReScan,
    NvmeAdapterSystemPowerHints,
    NvmeAdapterFilterResourceRequirements,
    NvmeAdapterPoFxMaxOperationalPower,
    NvmeAdapterPoFxSetPerfState,
    NvmeAdapterSurpriseRemoval,
    NvmeAdapterSerialNumber,
    NvmeAdapterCryptoOperation,
    NvmeAdapterQueryFruId,
    NvmeAdapterSetEventLogging,
    NvmeAdapterReportInternalData,
    NvmeAdapterResetBusSynchronous,
    NvmeAdapterPostHwInitialize,
    NvmeAdapterPrepareEarlyDumpData,
    NvmeAdapterRestoreEarlyDumpData,
    NvmeAdapterKsrPowerDown,
    NvmeAdapterPreparePLDR,
    NvmeNvmeofAdapterOperation,
    NvmeAdapterControlMax,
    MakeNvmeAdapterControlTypeSizeOfUlong = 0xffffffff
} NVME_ADAPTER_CONTROL_TYPE, *PNVME_ADAPTER_CONTROL_TYPE;

typedef enum _NVME_ADAPTER_CONTROL_STATUS
{
    NvmeAdapterControlSuccess = 0,
    NvmeAdapterControlUnsuccessful,
    NvmeAdapterControlRetryNeeded,
    NvmeAdapterControlBufferTooSmall
} NVME_ADAPTER_CONTROL_STATUS, *PNVME_ADAPTER_CONTROL_STATUS;

typedef enum _NVME_NAMESPACE_CONTROL_TYPE
{
    NvmeQuerySupportedNamespaceControlTypes = 0,
    NvmeNamespaceStart,
    NvmeNamespacePower,
    NvmeNamespacePoFxPowerInfo,
    NvmeNamespacePoFxPowerRequired,
    NvmeNamespacePoFxPowerActive,
    NvmeNamespacePoFxPowerSetFState,
    NvmeNamespacePoFxPowerControl,
    NvmeNamespaceRemove,
    NvmeNamespaceSurpriseRemoval,
    NvmeNamespaceControlMax,
    MakeNamespaceControlTypeSizeOfUlong = 0xffffffff
} NVME_NAMESPACE_CONTROL_TYPE, *PNVME_NAMESPACE_CONTROL_TYPE;

typedef enum _NVME_NAMESPACE_CONTROL_STATUS
{
    NvmeNamespaceControlSuccess = 0,
    NvmeNamespaceControlUnsuccessful,
    NvmeNamespaceControlNotSupported
} NVME_NAMESPACE_CONTROL_STATUS, *PNVME_NAMESPACE_CONTROL_STATUS;

#endif /* (NTDDI_VERSION >= NTDDI_WIN11_GE) */

typedef enum _SCSI_ADAPTER_CONTROL_TYPE
{
    ScsiQuerySupportedControlTypes = 0,
    ScsiStopAdapter,
    ScsiRestartAdapter,
    ScsiSetBootConfig,
    ScsiSetRunningConfig,
    ScsiPowerSettingNotification,
    ScsiAdapterPower,
    ScsiAdapterPoFxPowerRequired,
    ScsiAdapterPoFxPowerActive,
    ScsiAdapterPoFxPowerSetFState,
    ScsiAdapterPoFxPowerControl,
    ScsiAdapterPrepareForBusReScan,
    ScsiAdapterSystemPowerHints,
    ScsiAdapterFilterResourceRequirements,
    ScsiAdapterPoFxMaxOperationalPower,
    ScsiAdapterPoFxSetPerfState,
    ScsiAdapterSurpriseRemoval,
    ScsiAdapterSerialNumber,
    ScsiAdapterCryptoOperation,
    ScsiAdapterQueryFruId,
    ScsiAdapterSetEventLogging,
    ScsiAdapterReportInternalData,
    ScsiAdapterResetBusSynchronous,
    ScsiAdapterPostHwInitialize,
    ScsiAdapterPrepareEarlyDumpData,
    ScsiAdapterRestoreEarlyDumpData,
    ScsiAdapterKsrPowerDown,
    ScsiAdapterPreparePLDR,
    ScsiAdapterReserved0,
    ScsiAdapterReserved1,
    ScsiAdapterControlMax,
    MakeAdapterControlTypeSizeOfUlong = 0xffffffff
} SCSI_ADAPTER_CONTROL_TYPE, *PSCSI_ADAPTER_CONTROL_TYPE;

typedef enum _SCSI_ADAPTER_CONTROL_STATUS
{
    ScsiAdapterControlSuccess = 0,
    ScsiAdapterControlUnsuccessful,
    ScsiAdapterControlRetryNeeded,
    ScsiAdapterControlBufferTooSmall
} SCSI_ADAPTER_CONTROL_STATUS, *PSCSI_ADAPTER_CONTROL_STATUS;

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef enum _SCSI_UNIT_CONTROL_TYPE
{
    ScsiQuerySupportedUnitControlTypes = 0,
    ScsiUnitUsage,
    ScsiUnitStart,
    ScsiUnitPower,
    ScsiUnitPoFxPowerInfo,
    ScsiUnitPoFxPowerRequired,
    ScsiUnitPoFxPowerActive,
    ScsiUnitPoFxPowerSetFState,
    ScsiUnitPoFxPowerControl,
    ScsiUnitRemove,
    ScsiUnitSurpriseRemoval,
    ScsiUnitRichDescription,
    ScsiUnitQueryBusType,
    ScsiUnitQueryFruId,
    ScsiUnitReportInternalData,
    ScsiUnitKsrPowerDown,
    ScsiUnitNvmeIceInformation,
    ScsiUnitControlMax,
    MakeUnitControlTypeSizeOfUlong = 0xffffffff
} SCSI_UNIT_CONTROL_TYPE, *PSCSI_UNIT_CONTROL_TYPE;

typedef enum _SCSI_UNIT_CONTROL_STATUS
{
    ScsiUnitControlSuccess = 0,
    ScsiUnitControlUnsuccessful,
    ScsiUnitControlNotSupported
} SCSI_UNIT_CONTROL_STATUS, *PSCSI_UNIT_CONTROL_STATUS;

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

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
    ReleaseSpinLock,
    StateChangeDetectedCall,
    IoTargetRequestServiceTime,
    AsyncNotificationDetected,
    RequestDirectComplete,
    InitializeDpcWithContext,
    InitializeThreadedDpc,
    SetTargetProcessorDpc,
    MarkDeviceFailed,
    MarkDeviceFailedEx,
    TerminateSystemThread
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
    ExtFunctionFreeContiguousMemorySpecifyCache,
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ExtFunctionSetPowerSettingNotificationGuids,
    ExtFunctionInvokeAcpiMethod,
    ExtFunctionGetRequestInfo,
    ExtFunctionInitializeWorker,
    ExtFunctionQueueWorkItem,
    ExtFunctionFreeWorker,
    ExtFunctionInitializeTimer,
    ExtFunctionRequestTimer,
    ExtFunctionFreeTimer,
    ExtFunctionInitializeSListHead,
    ExtFunctionInterlockedFlushSList,
    ExtFunctionInterlockedPopEntrySList,
    ExtFunctionInterlockedPushEntrySList,
    ExtFunctionQueryDepthSList,
    ExtFunctionGetActivityId,
    ExtFunctionGetSystemPortNumber,
    ExtFunctionGetDataInBufferMdl,
    ExtFunctionGetDataInBufferSystemAddress,
    ExtFunctionGetDataInBufferScatterGatherList,
    ExtFunctionMarkDumpMemory,
    ExtFunctionSetUnitAttributes,
    ExtFunctionQueryPerformanceCounter,
    ExtFunctionInitializePoFxPower,
    ExtFunctionPoFxActivateComponent,
    ExtFunctionPoFxIdleComponent,
    ExtFunctionPoFxSetComponentLatency,
    ExtFunctionPoFxSetComponentResidency,
    ExtFunctionPoFxPowerControl,
    ExtFunctionFlushDataBufferMdl,
    ExtFunctionDeviceOperationAllowed,
    ExtFunctionGetProcessorIndexFromNumber,
    ExtFunctionPoFxSetIdleTimeout,
    ExtFunctionMiniportEtwEvent2,
    ExtFunctionMiniportEtwEvent4,
    ExtFunctionMiniportEtwEvent8,
    ExtFunctionCurrentOsInstallationUpgrade,
    ExtFunctionRegistryReadAdapterKey,
    ExtFunctionRegistryWriteAdapterKey,
    ExtFunctionSetAdapterBusType,
    ExtFunctionPoFxRegisterPerfStates,
    ExtFunctionPoFxSetPerfState,
    ExtFunctionGetD3ColdSupport,
    ExtFunctionInitializeRpmb,
    ExtFunctionAllocateHmb,
    ExtFunctionFreeHmb,
    ExtFunctionPropagateIrpExtension,
    ExtFunctionInterlockedInsertHeadList,
    ExtFunctionInterlockedInsertTailList,
    ExtFunctionInterlockedRemoveHeadList,
    ExtFunctionInitializeSpinlock,
    ExtFunctionGetPfns,
    ExtFunctionInitializeCryptoEngine,
    ExtFunctionGetRequestCryptoInfo,
    ExtFunctionMiniportTelemetry,
    ExtFunctionUpdateAdapterMaxIO,
    ExtFunctionDelayExecution,
    ExtFunctionAllocateDmaMemory,
    ExtFunctionFreeDmaMemory,
    ExtFunctionUpdateAdapterMaxIOInfo,
    ExtFunctionMiniportChannelEtwEvent2,
    ExtFunctionMiniportChannelEtwEvent4,
    ExtFunctionMiniportChannelEtwEvent8,
    ExtFunctionInitializeHighResolutionTimer,
    ExtFunctionRequestHighResolutionTimer,
    ExtFunctionCancelHighResolutionTimer,
    ExtFunctionFreeHighResolutionTimer,
    ExtFunctionGetCurrentProcessorIndex,
    ExtFunctionAcquireSpinLock,
    ExtFunctionGetProcessorCount,
    ExtFunctionCancelDpc,
    ExtFunctionMiniportTelemetryEx,
    ExtFunctionQueryConfiguration,
    ExtFunctionLogHardwareError,
    ExtFunctionInitializeEvent,
    ExtFunctionWaitForEvent,
    ExtFunctionSetEvent,
    ExtFunctionDeviceReset,
    ExtFunctionSetFeatureList,
    ExtFunctionCaptureLiveDump,
    ExtFunctionMiniportLogByteStream,
    ExtFunctionQueryDpcWatchdogInformation,
    ExtFunctionQueryTimerMinInterval,
    ExtFunctionMaskPciMsixEntry,
    ExtFunctionGetCurrentIrql,
    ExtFunctionCreateSystemThread,
    ExtFunctionSetPriorityThread,
    ExtFunctionSetSystemGroupAffinityThread,
    ExtFunctionRevertToUserGroupAffinityThread,
    ExtFunctionDeviceResetEx,
    ExtFunctionMiniportReportInternalData
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

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef struct _STOR_DEVICE_CAPABILITIES_EX
{
    USHORT Version;
    USHORT Size;
    ULONG DeviceD1:1;
    ULONG DeviceD2:1;
    ULONG LockSupported:1;
    ULONG EjectSupported:1;
    ULONG Removable:1;
    ULONG DockDevice:1;
    ULONG UniqueID:1;
    ULONG SilentInstall:1;
    ULONG RawDeviceOK:1;
    ULONG SurpriseRemovalOK:1;
    ULONG NoDisplayInUI:1;
    ULONG DefaultWriteCacheEnabled:1;
    ULONG Reserved0:20;
    ULONG Address;
    ULONG UINumber;
    ULONG Reserved1[2];
} STOR_DEVICE_CAPABILITIES_EX, *PSTOR_DEVICE_CAPABILITIES_EX;

/*
 * The variable length members of an extended SRB carry pointers, so on 64 bit
 * targets everything ahead of them has to keep those pointers naturally aligned.
 */
#if defined(_WIN64)
#define SRB_ALIGN                           DECLSPEC_ALIGN(8)
#define STOR_ADDRESS_ALIGN                  DECLSPEC_ALIGN(8)
#define POINTER_ALIGN                       DECLSPEC_ALIGN(8)
#else
#define SRB_ALIGN
#define STOR_ADDRESS_ALIGN
#define POINTER_ALIGN
#endif

#define STOR_ADDRESS_TYPE_UNKNOWN           0x0
#define STOR_ADDRESS_TYPE_BTL8              0x1
#define STOR_ADDRESS_TYPE_MAX               0xffff

typedef struct STOR_ADDRESS_ALIGN _STOR_ADDRESS
{
    USHORT Type;
    USHORT Port;
    ULONG AddressLength;
    _Field_size_bytes_(AddressLength) UCHAR AddressData[ANYSIZE_ARRAY];
} STOR_ADDRESS, *PSTOR_ADDRESS;

#define STOR_ADDR_BTL8_ADDRESS_LENGTH       4

typedef struct STOR_ADDRESS_ALIGN _STOR_ADDR_BTL8
{
    _Field_range_(STOR_ADDRESS_TYPE_BTL8, STOR_ADDRESS_TYPE_BTL8)
    USHORT Type;
    USHORT Port;
    _Field_range_(STOR_ADDR_BTL8_ADDRESS_LENGTH, STOR_ADDR_BTL8_ADDRESS_LENGTH)
    ULONG AddressLength;
    UCHAR Path;
    UCHAR Target;
    UCHAR Lun;
    UCHAR Reserved;
} STOR_ADDR_BTL8, *PSTOR_ADDR_BTL8;

typedef enum _SRBEXDATATYPE
{
    SrbExDataTypeUnknown = 0,
    SrbExDataTypeBidirectional,
    SrbExDataTypeScsiCdb16 = 0x40,
    SrbExDataTypeScsiCdb32,
    SrbExDataTypeScsiCdbVar,
    SrbExDataTypeNvmeCommand,
    SrbExDataTypeNvmeofOperation,
    SrbExDataTypeMiniportPassthrough,
    SrbExDataTypeWmi = 0x60,
    SrbExDataTypePower,
    SrbExDataTypePnP,
    SrbExDataTypeIoInfo = 0x80,
    SrbExDataTypePassthroughDirect = 0xa0,
    SrbExDataTypeMSReservedStart = 0xf0000000,
    SrbExDataTypeReserved = 0xffffffff
} SRBEXDATATYPE, *PSRBEXDATATYPE;

/* Every extended data block opens with these two members */
typedef struct SRB_ALIGN _SRBEX_DATA
{
    SRBEXDATATYPE Type;
    ULONG Length;
    _Field_size_bytes_(Length) UCHAR Data[ANYSIZE_ARRAY];
} SRBEX_DATA, *PSRBEX_DATA;

#define SRBEX_DATA_BIDIRECTIONAL_LENGTH     ((2 * sizeof(ULONG)) + sizeof(PVOID))

typedef struct SRB_ALIGN _SRBEX_DATA_BIDIRECTIONAL
{
    _Field_range_(SrbExDataTypeBidirectional, SrbExDataTypeBidirectional)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_BIDIRECTIONAL_LENGTH, SRBEX_DATA_BIDIRECTIONAL_LENGTH)
    ULONG Length;
    ULONG DataInTransferLength;
    ULONG Reserved1;
    _Field_size_bytes_full_(DataInTransferLength)
    PVOID POINTER_ALIGN DataInBuffer;
} SRBEX_DATA_BIDIRECTIONAL, *PSRBEX_DATA_BIDIRECTIONAL;

#define SRBEX_DATA_SCSI_CDB16_LENGTH        ((20 * sizeof(UCHAR)) + sizeof(ULONG) + sizeof(PVOID))

typedef struct SRB_ALIGN _SRBEX_DATA_SCSI_CDB16
{
    _Field_range_(SrbExDataTypeScsiCdb16, SrbExDataTypeScsiCdb16)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_SCSI_CDB16_LENGTH, SRBEX_DATA_SCSI_CDB16_LENGTH)
    ULONG Length;
    UCHAR ScsiStatus;
    UCHAR SenseInfoBufferLength;
    UCHAR CdbLength;
    UCHAR Reserved;
    ULONG Reserved1;
    _Field_size_bytes_full_(SenseInfoBufferLength)
    PVOID POINTER_ALIGN SenseInfoBuffer;
    UCHAR POINTER_ALIGN Cdb[16];
} SRBEX_DATA_SCSI_CDB16, *PSRBEX_DATA_SCSI_CDB16;

#define SRBEX_DATA_SCSI_CDB32_LENGTH        ((36 * sizeof(UCHAR)) + sizeof(ULONG) + sizeof(PVOID))

typedef struct SRB_ALIGN _SRBEX_DATA_SCSI_CDB32
{
    _Field_range_(SrbExDataTypeScsiCdb32, SrbExDataTypeScsiCdb32)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_SCSI_CDB32_LENGTH, SRBEX_DATA_SCSI_CDB32_LENGTH)
    ULONG Length;
    UCHAR ScsiStatus;
    UCHAR SenseInfoBufferLength;
    UCHAR CdbLength;
    UCHAR Reserved;
    ULONG Reserved1;
    _Field_size_bytes_full_(SenseInfoBufferLength)
    PVOID POINTER_ALIGN SenseInfoBuffer;
    UCHAR POINTER_ALIGN Cdb[32];
} SRBEX_DATA_SCSI_CDB32, *PSRBEX_DATA_SCSI_CDB32;

#define SRBEX_DATA_SCSI_CDB_VAR_LENGTH_MIN  ((4 * sizeof(UCHAR)) + (3 * sizeof(ULONG)) + sizeof(PVOID))
#define SRBEX_DATA_SCSI_CDB_VAR_LENGTH_MAX  0xffffffffUL

typedef struct SRB_ALIGN _SRBEX_DATA_SCSI_CDB_VAR
{
    _Field_range_(SrbExDataTypeScsiCdbVar, SrbExDataTypeScsiCdbVar)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_SCSI_CDB_VAR_LENGTH_MIN, SRBEX_DATA_SCSI_CDB_VAR_LENGTH_MAX)
    ULONG Length;
    UCHAR ScsiStatus;
    UCHAR SenseInfoBufferLength;
    UCHAR Reserved[2];
    ULONG CdbLength;
    ULONG Reserved1[2];
    _Field_size_bytes_full_(SenseInfoBufferLength)
    PVOID POINTER_ALIGN SenseInfoBuffer;
    _Field_size_bytes_full_(CdbLength)
    UCHAR POINTER_ALIGN Cdb[ANYSIZE_ARRAY];
} SRBEX_DATA_SCSI_CDB_VAR, *PSRBEX_DATA_SCSI_CDB_VAR;

#define SRBEX_DATA_WMI_LENGTH               ((4 * sizeof(UCHAR)) + sizeof(ULONG) + sizeof(PVOID))

typedef struct SRB_ALIGN _SRBEX_DATA_WMI
{
    _Field_range_(SrbExDataTypeWmi, SrbExDataTypeWmi)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_WMI_LENGTH, SRBEX_DATA_WMI_LENGTH)
    ULONG Length;
    UCHAR WMISubFunction;
    UCHAR WMIFlags;
    UCHAR Reserved[2];
    ULONG Reserved1;
    PVOID POINTER_ALIGN DataPath;
} SRBEX_DATA_WMI, *PSRBEX_DATA_WMI;

#define SRBEX_DATA_POWER_LENGTH             ((4 * sizeof(UCHAR)) + sizeof(STOR_DEVICE_POWER_STATE) + sizeof(STOR_POWER_ACTION))

typedef struct SRB_ALIGN _SRBEX_DATA_POWER
{
    _Field_range_(SrbExDataTypePower, SrbExDataTypePower)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_POWER_LENGTH, SRBEX_DATA_POWER_LENGTH)
    ULONG Length;
    UCHAR SrbPowerFlags;
    UCHAR Reserved[3];
    STOR_DEVICE_POWER_STATE DevicePowerState;
    STOR_POWER_ACTION PowerAction;
} SRBEX_DATA_POWER, *PSRBEX_DATA_POWER;

#define SRBEX_DATA_PNP_LENGTH               ((4 * sizeof(UCHAR)) + sizeof(STOR_PNP_ACTION) + (2 * sizeof(ULONG)))

typedef struct SRB_ALIGN _SRBEX_DATA_PNP
{
    _Field_range_(SrbExDataTypePnP, SrbExDataTypePnP)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_PNP_LENGTH, SRBEX_DATA_PNP_LENGTH)
    ULONG Length;
    UCHAR PnPSubFunction;
    UCHAR Reserved[3];
    STOR_PNP_ACTION PnPAction;
    ULONG SrbPnPFlags;
    ULONG Reserved1;
} SRBEX_DATA_PNP, *PSRBEX_DATA_PNP;

#define SRBEX_DATA_IO_INFO_LENGTH           ((5 * sizeof(ULONG)) + (4 * sizeof(UCHAR)))

/* SRBEX_DATA_IO_INFO.Flags */
#define REQUEST_INFO_NO_CACHE_FLAG          0x00000001
#define REQUEST_INFO_PAGING_IO_FLAG         0x00000002
#define REQUEST_INFO_SEQUENTIAL_IO_FLAG     0x00000004
#define REQUEST_INFO_TEMPORARY_FLAG         0x00000008
#define REQUEST_INFO_WRITE_THROUGH_FLAG     0x00000010
#define REQUEST_INFO_HYBRID_WRITE_THROUGH_FLAG 0x00000020
#define REQUEST_INFO_NO_FILE_OBJECT_FLAG    0x00000040
#define REQUEST_INFO_VOLSNAP_IO_FLAG        0x00000080
#define REQUEST_INFO_STREAM_FLAG            0x00000100
#define REQUEST_INFO_CRYPTO_FLAG            0x00000200
#define REQUEST_INFO_VALID_CACHEPRIORITY_FLAG 0x80000000

typedef struct SRB_ALIGN _SRBEX_DATA_IO_INFO
{
    _Field_range_(SrbExDataTypeIoInfo, SrbExDataTypeIoInfo)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_IO_INFO_LENGTH, SRBEX_DATA_IO_INFO_LENGTH)
    ULONG Length;
    ULONG Flags;
    ULONG Key;
    ULONG RWLength;
    BOOLEAN IsWriteRequest;
    UCHAR CachePriority;
#if (NTDDI_VERSION >= NTDDI_WIN11_GE)
    UCHAR IoPriorityLevel;
    UCHAR Reserved;
#else
    UCHAR Reserved[2];
#endif
    ULONG Reserved1[2];
} SRBEX_DATA_IO_INFO, *PSRBEX_DATA_IO_INFO;

/* ASCII "SRBX" */
#define SRB_SIGNATURE                       0x53524258
#define STORAGE_REQUEST_BLOCK_VERSION_1     0x1

/*
 * The leading four bytes mean the same thing in every SRB format, so a
 * receiver can recognize an extended SRB before committing to a layout.
 */
typedef struct SRB_ALIGN _STORAGE_REQUEST_BLOCK_HEADER
{
    USHORT Length;
    _Field_range_(SRB_FUNCTION_STORAGE_REQUEST_BLOCK, SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    UCHAR Function;
    UCHAR SrbStatus;
} STORAGE_REQUEST_BLOCK_HEADER, *PSTORAGE_REQUEST_BLOCK_HEADER;

typedef _Struct_size_bytes_(SrbLength) struct SRB_ALIGN _STORAGE_REQUEST_BLOCK
{
    USHORT Length;
    _Field_range_(SRB_FUNCTION_STORAGE_REQUEST_BLOCK, SRB_FUNCTION_STORAGE_REQUEST_BLOCK)
    UCHAR Function;
    UCHAR SrbStatus;
    UCHAR ReservedUchar[4];
    _Field_range_(SRB_SIGNATURE, SRB_SIGNATURE)
    ULONG Signature;
    _Field_range_(STORAGE_REQUEST_BLOCK_VERSION_1, STORAGE_REQUEST_BLOCK_VERSION_1)
    ULONG Version;
    ULONG SrbLength;
    ULONG SrbFunction;
    ULONG SrbFlags;
    ULONG ReservedUlong;
    ULONG RequestTag;
    USHORT RequestPriority;
    USHORT RequestAttribute;
    ULONG TimeOutValue;
#if (NTDDI_VERSION >= NTDDI_WIN10_CU)
    union
    {
        ULONG SystemStatus;
        ULONG RequestTagHigh4Bytes;
    } DUMMYUNIONNAME;
#else
    ULONG SystemStatus;
#endif
    ULONG ZeroGuard1;
    _Field_range_(sizeof(STORAGE_REQUEST_BLOCK), SrbLength - sizeof(STOR_ADDRESS))
    ULONG AddressOffset;
    ULONG NumSrbExData;
    ULONG DataTransferLength;
    _Field_size_bytes_full_(DataTransferLength)
    PVOID POINTER_ALIGN DataBuffer;
    PVOID POINTER_ALIGN ZeroGuard2;
    PVOID POINTER_ALIGN OriginalRequest;
    PVOID POINTER_ALIGN ClassContext;
    PVOID POINTER_ALIGN PortContext;
    PVOID POINTER_ALIGN MiniportContext;
    struct _STORAGE_REQUEST_BLOCK POINTER_ALIGN *NextSrb;
    _At_buffer_(SrbExDataOffset, _Iter_, NumSrbExData, _Field_range_(0, SrbLength - sizeof(SRBEX_DATA)))
    _Field_size_(NumSrbExData)
    ULONG SrbExDataOffset[ANYSIZE_ARRAY];
} STORAGE_REQUEST_BLOCK, *PSTORAGE_REQUEST_BLOCK;

/* PORT_CONFIGURATION_INFORMATION.SrbType */
#define SRB_TYPE_SCSI_REQUEST_BLOCK         0
#define SRB_TYPE_STORAGE_REQUEST_BLOCK      1
#define SRB_TYPE_NVME_REQUEST_BLOCK         2

/* PORT_CONFIGURATION_INFORMATION.AddressType */
#define STORAGE_ADDRESS_TYPE_BTL8           0

/* HW_INITIALIZATION_DATA.SrbTypeFlags */
#define SRB_TYPE_FLAG_SCSI_REQUEST_BLOCK    0x1
#define SRB_TYPE_FLAG_STORAGE_REQUEST_BLOCK 0x2

/* HW_INITIALIZATION_DATA.AddressTypeFlags */
#define ADDRESS_TYPE_FLAG_BTL8              0x1

/* HW_INITIALIZATION_DATA.FeatureSupport */
#define STOR_FEATURE_VIRTUAL_MINIPORT                    0x00000001
#define STOR_FEATURE_ATA_PASS_THROUGH                    0x00000002
#define STOR_FEATURE_FULL_PNP_DEVICE_CAPABILITIES        0x00000004
#define STOR_FEATURE_DUMP_POINTERS                       0x00000008
#define STOR_FEATURE_DEVICE_NAME_NO_SUFFIX               0x00000010
#define STOR_FEATURE_DUMP_RESUME_CAPABLE                 0x00000020
#define STOR_FEATURE_DEVICE_DESCRIPTOR_FROM_ATA_INFO_VPD 0x00000040
#define STOR_FEATURE_EXTRA_IO_INFORMATION                0x00000080
#define STOR_FEATURE_ADAPTER_CONTROL_PRE_FINDADAPTER     0x00000100
#define STOR_FEATURE_ADAPTER_NOT_REQUIRE_IO_PORT         0x00000200
#define STOR_FEATURE_DUMP_16_BYTE_ALIGNMENT              0x00000400
#define STOR_FEATURE_SET_ADAPTER_INTERFACE_TYPE          0x00000800
#define STOR_FEATURE_DUMP_INFO                           0x00001000
#define STOR_FEATURE_DMA_ALLOCATION_NO_BOUNDARY          0x00002000
#define STOR_FEATURE_REPORT_INTERNAL_DATA                0x00008000
#define STOR_FEATURE_EARLY_DUMP                          0x00010000

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)

/*
 * StorMQ addresses a controller directly rather than by bus, target and LUN.
 * Controller is the controller extension the miniport already knows.
 */
#define STOR_ADDRESS_TYPE_NVME              0x2
#define STOR_ADDR_NVME_ADDRESS_LENGTH       16

typedef struct STOR_ADDRESS_ALIGN _STOR_ADDR_NVME
{
    _Field_range_(STOR_ADDRESS_TYPE_NVME, STOR_ADDRESS_TYPE_NVME)
    USHORT Type;
    USHORT Port;
    _Field_range_(STOR_ADDR_NVME_ADDRESS_LENGTH, STOR_ADDR_NVME_ADDRESS_LENGTH)
    ULONG AddressLength;
    PVOID Controller;
    ULONG NamespaceId;
    ULONG Reserved;
} STOR_ADDR_NVME, *PSTOR_ADDR_NVME;

#define SRBEX_DATA_MINIPORT_PASSTHROUGH_LENGTH (4 * sizeof(ULONG))

/* SRB_FUNCTION_MINIPORT_PASSTHROUGH_REQUEST */
typedef struct SRB_ALIGN _SRBEX_DATA_MINIPORT_PASSTHROUGH
{
    _Field_range_(SrbExDataTypeMiniportPassthrough, SrbExDataTypeMiniportPassthrough)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_MINIPORT_PASSTHROUGH_LENGTH, SRBEX_DATA_MINIPORT_PASSTHROUGH_LENGTH)
    ULONG Length;
    ULONG InputBufferLength;
    ULONG OutputBufferLength;
    ULONG OutputBufferWritten;
    ULONG Reserved;
} SRBEX_DATA_MINIPORT_PASSTHROUGH, *PSRBEX_DATA_MINIPORT_PASSTHROUGH;

#define SRBEX_DATA_NVME_COMMAND_LENGTH \
    ((4 * sizeof(ULONGLONG)) + (14 * sizeof(ULONG)) + (5 * sizeof(USHORT)) + (2 * sizeof(UCHAR)))

/* SRBEX_DATA_NVME_COMMAND.CommandType */
typedef enum
{
    SRBEX_DATA_NVME_COMMAND_TYPE_NVM = 0,
    SRBEX_DATA_NVME_COMMAND_TYPE_ADMIN,
    SRBEX_DATA_NVME_COMMAND_TYPE_FABRICS
} SRBEX_DATA_NVME_COMMAND_TYPE, *PSRBEX_DATA_NVME_COMMAND_TYPE;

/* SRBEX_DATA_NVME_COMMAND.CommandFlags */
typedef enum
{
    SRBEX_DATA_NVME_COMMAND_FLAG_REQUIRE_DATA_TRANSFER_IN = 0x1,
    SRBEX_DATA_NVME_COMMAND_FLAG_REQUIRE_DATA_TRANSFER_OUT = 0x2,
    SRBEX_DATA_NVME_COMMAND_FLAG_PRP_SET_ALREADY = 0x4,
    SRBEX_DATA_NVME_COMMAND_FLAG_SIGNATURE_ENABLED = 0x8,
    SRBEX_DATA_NVME_COMMAND_FLAG_NO_POLLING = 0x10
} SRBEX_DATA_NVME_COMMAND_FLAG, *PSRBEX_DATA_NVME_COMMAND_FLAG;

/* SRBEX_DATA_NVME_COMMAND.ResponseFlags */
typedef enum
{
    SRBEX_DATA_NVME_RESPONSE_FLAG_SQHD_VALID = 0x1
} SRBEX_DATA_NVME_RESPONSE_FLAG, *PSRBEX_DATA_NVME_RESPONSE_FLAG;

/*
 * SRB_FUNCTION_EXECUTE_NVME. The command members mirror the NVMe common
 * command format, so the three views of the same 64 bytes let a caller fill
 * in an NVM, admin or fabrics command without reshaping the block.
 */
typedef struct SRB_ALIGN _SRBEX_DATA_NVME_COMMAND
{
    _Field_range_(SrbExDataTypeNvmeCommand, SrbExDataTypeNvmeCommand)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_NVME_COMMAND_LENGTH, SRBEX_DATA_NVME_COMMAND_LENGTH)
    ULONG Length;
    union
    {
        PVOID ControllerHandle;
        ULONGLONG Reserved0;
    };
    union
    {
        struct
        {
            ULONG CommandDWORD0;
            ULONG CommandNSID;
            ULONG Reserved1[2];
            ULONGLONG CommandMPTR;
            union
            {
                struct
                {
                    ULONGLONG CommandPRP1;
                    ULONGLONG CommandPRP2;
                };
                ULONGLONG CommandSGL1[2];
            };
            ULONG CommandCDW10;
            ULONG CommandCDW11;
            ULONG CommandCDW12;
            ULONG CommandCDW13;
            ULONG CommandCDW14;
            ULONG CommandCDW15;
        };
        struct
        {
            UCHAR OPC;
            UCHAR PSDT;
            USHORT CID;
            UCHAR FCTYPE;
            UCHAR Reserved[35];
            UCHAR Specific[24];
        } FabricsCommand;
        struct
        {
            ULONG OPC:8;
            ULONG FUSE:2;
            ULONG Reserved:4;
            ULONG PSDT:2;
            ULONG CID:16;
            UCHAR TypeSpecific[60];
        } Command;
    };
    UCHAR CommandType;
    UCHAR Reserved2;
    USHORT CommandFlags;
    USHORT ResponseFlags;
    union
    {
        struct
        {
            USHORT P:1;
            USHORT SC:8;
            USHORT SCT:3;
            USHORT CRD:2;
            USHORT M:1;
            USHORT DNR:1;
        } DUMMYSTRUCTNAME;
        USHORT AsUshort;
    } CommandStatus;
    /* 0xFFFFFFFF leaves the queue choice to the miniport */
    ULONG QID;
    ULONG CommandTag;
    union
    {
        struct
        {
            ULONG CQEntryDW0;
            ULONG CQEntryDW1;
        };
        UCHAR Specific[8];
    };
    USHORT SQHD;
    USHORT SQID;
} SRBEX_DATA_NVME_COMMAND, *PSRBEX_DATA_NVME_COMMAND;

#define STOR_NVMEOF_OPERATION_V1            0x0001
#define SRBEX_DATA_NVMEOF_OPERATION_LENGTH  ((2 * sizeof(USHORT)) + (2 * sizeof(ULONG)))

/* SRB_FUNCTION_NVMEOF_OPERATION. Any payload travels in the data buffer. */
typedef struct SRB_ALIGN _SRBEX_DATA_NVMEOF_OPERATION
{
    _Field_range_(SrbExDataTypeNvmeofOperation, SrbExDataTypeNvmeofOperation)
    SRBEXDATATYPE Type;
    _Field_range_(SRBEX_DATA_NVMEOF_OPERATION_LENGTH, SRBEX_DATA_NVMEOF_OPERATION_LENGTH)
    ULONG Length;
    _Field_range_(STOR_NVMEOF_OPERATION_V1, STOR_NVMEOF_OPERATION_V1)
    USHORT Version;
    USHORT Reserved1;
    ULONG Flags;
    ULONG FunctionType;
} SRBEX_DATA_NVMEOF_OPERATION, *PSRBEX_DATA_NVMEOF_OPERATION;

#endif /* (NTDDI_VERSION >= NTDDI_WIN11_GE) */

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

typedef struct _SENSE_DATA FIXED_SENSE_DATA, *PFIXED_SENSE_DATA;

typedef ULONG SCSI_SENSE_OPTIONS;

typedef struct _NOTIFICATION_EVENT_STATUS_HEADER
{
    UCHAR EventDataLength[2];
    UCHAR NotificationClass:3;
    UCHAR Reserved:4;
    UCHAR NEA:1;
    UCHAR SupportedEventClasses;
    UCHAR ClassEventData[0];
} NOTIFICATION_EVENT_STATUS_HEADER, *PNOTIFICATION_EVENT_STATUS_HEADER;

typedef struct _NOTIFICATION_OPERATIONAL_STATUS
{
    UCHAR OperationalEvent:4;
    UCHAR Reserved1:4;
    UCHAR OperationalStatus:4;
    UCHAR Reserved2:3;
    UCHAR PersistentPrevented:1;
    UCHAR Operation[2];
} NOTIFICATION_OPERATIONAL_STATUS, *PNOTIFICATION_OPERATIONAL_STATUS;

typedef struct _NOTIFICATION_POWER_STATUS
{
    UCHAR PowerEvent:4;
    UCHAR Reserved:4;
    UCHAR PowerStatus;
    UCHAR Reserved2[2];
} NOTIFICATION_POWER_STATUS, *PNOTIFICATION_POWER_STATUS;

typedef struct _NOTIFICATION_EXTERNAL_STATUS
{
    UCHAR ExternalEvent:4;
    UCHAR Reserved1:4;
    UCHAR ExternalStatus:4;
    UCHAR Reserved2:3;
    UCHAR PersistentPrevented:1;
    UCHAR Request[2];
} NOTIFICATION_EXTERNAL_STATUS, *PNOTIFICATION_EXTERNAL_STATUS;

typedef struct _NOTIFICATION_MEDIA_STATUS
{
    UCHAR MediaEvent:4;
    UCHAR Reserved:4;
    _ANONYMOUS_UNION union
    {
        UCHAR PowerStatus;
        UCHAR MediaStatus;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR DoorTrayOpen:1;
            UCHAR MediaPresent:1;
            UCHAR ReservedX:6;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    UCHAR StartSlot;
    UCHAR EndSlot;
} NOTIFICATION_MEDIA_STATUS, *PNOTIFICATION_MEDIA_STATUS;

typedef struct _NOTIFICATION_MULTI_HOST_STATUS
{
    UCHAR MultiHostEvent:4;
    UCHAR Reserved1:4;
    UCHAR MultiHostStatus:4;
    UCHAR Reserved2:3;
    UCHAR PersistentPrevented:1;
    UCHAR Priority[2];
} NOTIFICATION_MULTI_HOST_STATUS, *PNOTIFICATION_MULTI_HOST_STATUS;

typedef struct _NOTIFICATION_BUSY_STATUS
{
    UCHAR DeviceBusyEvent:4;
    UCHAR Reserved:4;
    UCHAR DeviceBusyStatus;
    UCHAR Time[2];
} NOTIFICATION_BUSY_STATUS, *PNOTIFICATION_BUSY_STATUS;

typedef struct _READ_DVD_STRUCTURES_HEADER
{
    UCHAR Length[2];
    UCHAR Reserved[2];
    UCHAR Data[0];
} READ_DVD_STRUCTURES_HEADER, *PREAD_DVD_STRUCTURES_HEADER;

typedef struct _CDVD_KEY_HEADER
{
    UCHAR DataLength[2];
    UCHAR Reserved[2];
    UCHAR Data[0];
} CDVD_KEY_HEADER, *PCDVD_KEY_HEADER;

typedef struct _CDVD_REPORT_AGID_DATA
{
    UCHAR Reserved1[3];
    UCHAR Reserved2:6;
    UCHAR AGID:2;
} CDVD_REPORT_AGID_DATA, *PCDVD_REPORT_AGID_DATA;

typedef struct _CDVD_CHALLENGE_KEY_DATA
{
    UCHAR ChallengeKeyValue[10];
    UCHAR Reserved[2];
} CDVD_CHALLENGE_KEY_DATA, *PCDVD_CHALLENGE_KEY_DATA;

typedef struct _CDVD_KEY_DATA
{
    UCHAR Key[5];
    UCHAR Reserved[3];
} CDVD_KEY_DATA, *PCDVD_KEY_DATA;

typedef struct _CDVD_REPORT_ASF_DATA
{
    UCHAR Reserved1[3];
    UCHAR Success:1;
    UCHAR Reserved2:7;
} CDVD_REPORT_ASF_DATA, *PCDVD_REPORT_ASF_DATA;

typedef struct _CDVD_TITLE_KEY_HEADER
{
    UCHAR DataLength[2];
    UCHAR Reserved1[1];
    UCHAR Reserved2:3;
    UCHAR CGMS:2;
    UCHAR CP_SEC:1;
    UCHAR CPM:1;
    UCHAR Zero:1;
    CDVD_KEY_DATA TitleKey;
} CDVD_TITLE_KEY_HEADER, *PCDVD_TITLE_KEY_HEADER;

typedef struct _FORMAT_DESCRIPTOR
{
    UCHAR NumberOfBlocks[4];
    UCHAR FormatSubType:2;
    UCHAR FormatType:6;
    UCHAR BlockLength[3];
} FORMAT_DESCRIPTOR, *PFORMAT_DESCRIPTOR;

typedef struct _FORMAT_LIST_HEADER
{
    UCHAR Reserved;
    UCHAR VendorSpecific:1;
    UCHAR Immediate:1;
    UCHAR TryOut:1;
    UCHAR IP:1;
    UCHAR STPF:1;
    UCHAR DCRT:1;
    UCHAR DPRY:1;
    UCHAR FOV:1;
    UCHAR FormatDescriptorLength[2];
    FORMAT_DESCRIPTOR Descriptors[0];
} FORMAT_LIST_HEADER, *PFORMAT_LIST_HEADER;

typedef struct _FORMATTED_CAPACITY_DESCRIPTOR
{
    UCHAR NumberOfBlocks[4];
    UCHAR Maximum:1;
    UCHAR Valid:1;
    UCHAR FormatType:6;
    UCHAR BlockLength[3];
} FORMATTED_CAPACITY_DESCRIPTOR, *PFORMATTED_CAPACITY_DESCRIPTOR;

typedef struct _FORMATTED_CAPACITY_LIST
{
    UCHAR Reserved[3];
    UCHAR CapacityListLength;
    FORMATTED_CAPACITY_DESCRIPTOR Descriptors[0];
} FORMATTED_CAPACITY_LIST, *PFORMATTED_CAPACITY_LIST;

typedef struct _OPC_TABLE_ENTRY
{
    UCHAR Speed[2];
    UCHAR OPCValue[6];
} OPC_TABLE_ENTRY, *POPC_TABLE_ENTRY;

typedef struct _DISC_INFORMATION
{
    UCHAR Length[2];
    UCHAR DiscStatus:2;
    UCHAR LastSessionStatus:2;
    UCHAR Erasable:1;
    UCHAR Reserved1:3;
    UCHAR FirstTrackNumber;
    UCHAR NumberOfSessionsLsb;
    UCHAR LastSessionFirstTrackLsb;
    UCHAR LastSessionLastTrackLsb;
    UCHAR MrwStatus:2;
    UCHAR MrwDirtyBit:1;
    UCHAR Reserved2:2;
    UCHAR URU:1;
    UCHAR DBC_V:1;
    UCHAR DID_V:1;
    UCHAR DiscType;
    UCHAR NumberOfSessionsMsb;
    UCHAR LastSessionFirstTrackMsb;
    UCHAR LastSessionLastTrackMsb;
    UCHAR DiskIdentification[4];
    UCHAR LastSessionLeadIn[4];
    UCHAR LastPossibleLeadOutStartTime[4];
    UCHAR DiskBarCode[8];
    UCHAR Reserved4;
    UCHAR NumberOPCEntries;
    OPC_TABLE_ENTRY OPCTable[1];
} DISC_INFORMATION, *PDISC_INFORMATION;

typedef struct _DISK_INFORMATION
{
    UCHAR Length[2];
    UCHAR DiskStatus:2;
    UCHAR LastSessionStatus:2;
    UCHAR Erasable:1;
    UCHAR Reserved1:3;
    UCHAR FirstTrackNumber;
    UCHAR NumberOfSessions;
    UCHAR LastSessionFirstTrack;
    UCHAR LastSessionLastTrack;
    UCHAR Reserved2:5;
    UCHAR GEN:1;
    UCHAR DBC_V:1;
    UCHAR DID_V:1;
    UCHAR DiskType;
    UCHAR Reserved3[3];
    UCHAR DiskIdentification[4];
    UCHAR LastSessionLeadIn[4];
    UCHAR LastPossibleStartTime[4];
    UCHAR DiskBarCode[8];
    UCHAR Reserved4;
    UCHAR NumberOPCEntries;
    OPC_TABLE_ENTRY OPCTable[0];
} DISK_INFORMATION, *PDISK_INFORMATION;

typedef struct _DATA_BLOCK_HEADER
{
    UCHAR DataMode;
    UCHAR Reserved[4];
    _ANONYMOUS_UNION union
    {
        UCHAR LogicalBlockAddress[4];
        struct
        {
            UCHAR Reserved;
            UCHAR M;
            UCHAR S;
            UCHAR F;
        } MSF;
    } DUMMYUNIONNAME;
} DATA_BLOCK_HEADER, *PDATA_BLOCK_HEADER;

typedef struct _TRACK_INFORMATION
{
    UCHAR Length[2];
    UCHAR TrackNumber;
    UCHAR SessionNumber;
    UCHAR Reserved1;
    UCHAR TrackMode:4;
    UCHAR Copy:1;
    UCHAR Damage:1;
    UCHAR Reserved2:2;
    UCHAR DataMode:4;
    UCHAR FP:1;
    UCHAR Packet:1;
    UCHAR Blank:1;
    UCHAR RT:1;
    UCHAR NWA_V:1;
    UCHAR Reserved3:7;
    UCHAR TrackStartAddress[4];
    UCHAR NextWritableAddress[4];
    UCHAR FreeBlocks[4];
    UCHAR FixedPacketSize[4];
} TRACK_INFORMATION, *PTRACK_INFORMATION;

typedef struct _TRACK_INFORMATION2
{
    UCHAR Length[2];
    UCHAR TrackNumberLsb;
    UCHAR SessionNumberLsb;
    UCHAR Reserved4;
    UCHAR TrackMode:4;
    UCHAR Copy:1;
    UCHAR Damage:1;
    UCHAR Reserved5:2;
    UCHAR DataMode:4;
    UCHAR FixedPacket:1;
    UCHAR Packet:1;
    UCHAR Blank:1;
    UCHAR ReservedTrack:1;
    UCHAR NWA_V:1;
    UCHAR LRA_V:1;
    UCHAR Reserved6:6;
    UCHAR TrackStartAddress[4];
    UCHAR NextWritableAddress[4];
    UCHAR FreeBlocks[4];
    UCHAR FixedPacketSize[4];
    UCHAR TrackSize[4];
    UCHAR LastRecordedAddress[4];
    UCHAR TrackNumberMsb;
    UCHAR SessionNumberMsb;
    UCHAR Reserved7[2];
} TRACK_INFORMATION2, *PTRACK_INFORMATION2;

typedef struct _TRACK_INFORMATION3
{
    UCHAR Length[2];
    UCHAR TrackNumberLsb;
    UCHAR SessionNumberLsb;
    UCHAR Reserved4;
    UCHAR TrackMode:4;
    UCHAR Copy:1;
    UCHAR Damage:1;
    UCHAR Reserved5:2;
    UCHAR DataMode:4;
    UCHAR FixedPacket:1;
    UCHAR Packet:1;
    UCHAR Blank:1;
    UCHAR ReservedTrack:1;
    UCHAR NWA_V:1;
    UCHAR LRA_V:1;
    UCHAR Reserved6:6;
    UCHAR TrackStartAddress[4];
    UCHAR NextWritableAddress[4];
    UCHAR FreeBlocks[4];
    UCHAR FixedPacketSize[4];
    UCHAR TrackSize[4];
    UCHAR LastRecordedAddress[4];
    UCHAR TrackNumberMsb;
    UCHAR SessionNumberMsb;
    UCHAR Reserved7[2];
    UCHAR ReadCompatibilityLba[4];
} TRACK_INFORMATION3, *PTRACK_INFORMATION3;

typedef struct _PERFORMANCE_DESCRIPTOR
{
    UCHAR RandomAccess:1;
    UCHAR Exact:1;
    UCHAR RestoreDefaults:1;
    UCHAR WriteRotationControl:2;
    UCHAR Reserved1:3;
    UCHAR Reserved[3];
    UCHAR StartLba[4];
    UCHAR EndLba[4];
    UCHAR ReadSize[4];
    UCHAR ReadTime[4];
    UCHAR WriteSize[4];
    UCHAR WriteTime[4];
} PERFORMANCE_DESCRIPTOR, *PPERFORMANCE_DESCRIPTOR;

typedef struct _SCSI_EXTENDED_MESSAGE
{
    UCHAR InitialMessageCode;
    UCHAR MessageLength;
    UCHAR MessageType;
    union _EXTENDED_ARGUMENTS
    {
        struct
        {
            UCHAR Modifier[4];
        } Modify;
        struct
        {
            UCHAR TransferPeriod;
            UCHAR ReqAckOffset;
        } Synchronous;
        struct{
            UCHAR Width;
        } Wide;
    } ExtendedArguments;
} SCSI_EXTENDED_MESSAGE, *PSCSI_EXTENDED_MESSAGE;

typedef struct _VPD_ATA_INFORMATION_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR PageLength[2];
    UCHAR Reserved0[4];
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR DeviceSignature[20];
    UCHAR CommandCode;
    UCHAR Reserved1[3];
    UCHAR IdentifyDeviceData[512];
} VPD_ATA_INFORMATION_PAGE, *PVPD_ATA_INFORMATION_PAGE;

typedef struct _VPD_THIRD_PARTY_COPY_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR PageLength[2];
#if !defined(__midl)
    UCHAR ThirdPartyCopyDescriptors[ANYSIZE_ARRAY];
#endif
} VPD_THIRD_PARTY_COPY_PAGE, *PVPD_THIRD_PARTY_COPY_PAGE;

typedef struct _WINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR
{
    UCHAR DescriptorType[2];
    UCHAR DescriptorLength[2];
    UCHAR VendorSpecific[6];
    UCHAR MaximumRangeDescriptors[2];
    UCHAR MaximumInactivityTimer[4];
    UCHAR DefaultInactivityTimer[4];
    UCHAR MaximumTokenTransferSize[8];
    UCHAR OptimalTransferCount[8];
} WINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR, *PWINDOWS_BLOCK_DEVICE_TOKEN_LIMITS_DESCRIPTOR;

typedef struct _VPD_BLOCK_LIMITS_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR PageLength[2];
    union
    {
        struct
        {
            UCHAR Reserved0;
            UCHAR MaximumCompareAndWriteLength;
            UCHAR OptimalTransferLengthGranularity[2];
            UCHAR MaximumTransferLength[4];
            UCHAR OptimalTransferLength[4];
            UCHAR MaxPrefetchXDReadXDWriteTransferLength[4];
            UCHAR MaximumUnmapLBACount[4];
            UCHAR MaximumUnmapBlockDescriptorCount[4];
            UCHAR OptimalUnmapGranularity[4];
            union
            {
                struct
                {
                    UCHAR UnmapGranularityAlignmentByte3:7;
                    UCHAR UGAValid:1;
                    UCHAR UnmapGranularityAlignmentByte2;
                    UCHAR UnmapGranularityAlignmentByte1;
                    UCHAR UnmapGranularityAlignmentByte0;
                };
                UCHAR UnmapGranularityAlignment[4];
            };
            UCHAR Reserved1[28];
        };
#if !defined(__midl)
    UCHAR Descriptors[0];
#endif
    };
} VPD_BLOCK_LIMITS_PAGE, *PVPD_BLOCK_LIMITS_PAGE;

typedef struct _VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR Reserved0;
    UCHAR PageLength;
    UCHAR MediumRotationRateMsb;
    UCHAR MediumRotationRateLsb;
    UCHAR MediumProductType;
    UCHAR NominalFormFactor:4;
    UCHAR WACEREQ:2;
    UCHAR WABEREQ:2;
    UCHAR VBULS:1;
    UCHAR FUAB:1;
    UCHAR BOCS:1;
    UCHAR Reserved1:1;
    UCHAR ZONED:2;
    UCHAR Reserved2:2;
    UCHAR Reserved3[3];
    UCHAR DepopulationTime[4];
    UCHAR Reserved4[48];
} VPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE, *PVPD_BLOCK_DEVICE_CHARACTERISTICS_PAGE;

typedef struct _VPD_LOGICAL_BLOCK_PROVISIONING_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR PageLength[2];
    UCHAR ThresholdExponent;
    UCHAR DP:1;
    UCHAR ANC_SUP:1;
    UCHAR LBPRZ:1;
    UCHAR Reserved0:2;
    UCHAR LBPWS10:1;
    UCHAR LBPWS:1;
    UCHAR LBPU:1;
    UCHAR ProvisioningType:3;
    UCHAR Reserved1:5;
    UCHAR Reserved2;
#if !defined(__midl)
    UCHAR ProvisioningGroupDescr[0];
#endif
} VPD_LOGICAL_BLOCK_PROVISIONING_PAGE, *PVPD_LOGICAL_BLOCK_PROVISIONING_PAGE;

typedef struct _VPD_ZONED_BLOCK_DEVICE_CHARACTERISTICS_PAGE
{
    UCHAR DeviceType:5;
    UCHAR DeviceTypeQualifier:3;
    UCHAR PageCode;
    UCHAR PageLength[2];
    UCHAR URSWRZ:1;
    UCHAR Reserved1:7;
    UCHAR Reserved2[3];
    UCHAR OptimalNumberOfOpenSequentialWritePreferredZone[4];
    UCHAR OptimalNumberOfNonSequentiallyWrittenSequentialWritePreferredZone[4];
    UCHAR MaxNumberOfOpenSequentialWriteRequiredZone[4];
    UCHAR Reserved3[44];
} VPD_ZONED_BLOCK_DEVICE_CHARACTERISTICS_PAGE, *PVPD_ZONED_BLOCK_DEVICE_CHARACTERISTICS_PAGE;

typedef struct _PRI_REGISTRATION_LIST
{
    UCHAR Generation[4];
    UCHAR AdditionalLength[4];
    UCHAR ReservationKeyList[0][8];
} PRI_REGISTRATION_LIST, *PPRI_REGISTRATION_LIST;

typedef struct _PRI_RESERVATION_DESCRIPTOR
{
    UCHAR ReservationKey[8];
    UCHAR ScopeSpecificAddress[4];
    UCHAR Reserved;
    UCHAR Type:4;
    UCHAR Scope:4;
    UCHAR Obsolete[2];
} PRI_RESERVATION_DESCRIPTOR, *PPRI_RESERVATION_DESCRIPTOR;

typedef struct _PRI_RESERVATION_LIST
{
    UCHAR Generation[4];
    UCHAR AdditionalLength[4];
    PRI_RESERVATION_DESCRIPTOR Reservations[0];
} PRI_RESERVATION_LIST, *PPRI_RESERVATION_LIST;

typedef struct _PRO_PARAMETER_LIST
{
    UCHAR ReservationKey[8];
    UCHAR ServiceActionReservationKey[8];
    UCHAR ScopeSpecificAddress[4];
    UCHAR ActivatePersistThroughPowerLoss:1;
    UCHAR Reserved1:7;
    UCHAR Reserved2;
    UCHAR Obsolete[2];
} PRO_PARAMETER_LIST, *PPRO_PARAMETER_LIST;

typedef struct _SCSI_SENSE_DESCRIPTOR_HEADER
{
    UCHAR DescriptorType;
    UCHAR AdditionalLength;
} SCSI_SENSE_DESCRIPTOR_HEADER, *PSCSI_SENSE_DESCRIPTOR_HEADER;

typedef struct _SCSI_SENSE_DESCRIPTOR_INFORMATION
{
    SCSI_SENSE_DESCRIPTOR_HEADER Header;
    UCHAR Valid:1;
    UCHAR Reserved1:7;
    UCHAR Reserved2;
    UCHAR Information[8];
} SCSI_SENSE_DESCRIPTOR_INFORMATION, *PSCSI_SENSE_DESCRIPTOR_INFORMATION;

typedef struct _SCSI_SENSE_DESCRIPTOR_BLOCK_COMMAND
{
    SCSI_SENSE_DESCRIPTOR_HEADER Header;
    UCHAR Reserved1;
    UCHAR Reserved2:5;
    UCHAR IncorrectLength:1;
    UCHAR Reserved3:2;
} SCSI_SENSE_DESCRIPTOR_BLOCK_COMMAND, *PSCSI_SENSE_DESCRIPTOR_BLOCK_COMMAND;

typedef struct _SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN
{
    SCSI_SENSE_DESCRIPTOR_HEADER Header;
    UCHAR Extend:1;
    UCHAR Reserved1:7;
    UCHAR Error;
    UCHAR SectorCount15_8;
    UCHAR SectorCount7_0;
    UCHAR LbaLow15_8;
    UCHAR LbaLow7_0;
    UCHAR LbaMid15_8;
    UCHAR LbaMid7_0;
    UCHAR LbaHigh15_8;
    UCHAR LbaHigh7_0;
    UCHAR Device;
    UCHAR Status;
} SCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN, *PSCSI_SENSE_DESCRIPTOR_ATA_STATUS_RETURN;

typedef struct _SENSE_DATA FIXED_SENSE_DATA, *PFIXED_SENSE_DATA;

typedef struct _DESCRIPTOR_SENSE_DATA
{
    UCHAR ErrorCode:7;
    UCHAR Reserved1:1;
    UCHAR SenseKey:4;
    UCHAR Reserved2:4;
    UCHAR AdditionalSenseCode;
    UCHAR AdditionalSenseCodeQualifier;
    UCHAR Reserved3[3];
    UCHAR AdditionalSenseLength;
    UCHAR DescriptorBuffer[ANYSIZE_ARRAY];
} DESCRIPTOR_SENSE_DATA, *PDESCRIPTOR_SENSE_DATA;

typedef union _SENSE_DATA_EX
{
    FIXED_SENSE_DATA FixedData;
    DESCRIPTOR_SENSE_DATA DescriptorData;
} SENSE_DATA_EX, *PSENSE_DATA_EX;

typedef struct _READ_CAPACITY16_DATA
{
    LARGE_INTEGER LogicalBlockAddress;
    ULONG BytesPerBlock;
    UCHAR ProtectionEnable:1;
    UCHAR ProtectionType:3;
    UCHAR RcBasis:2;
    UCHAR Reserved:2;
    UCHAR LogicalPerPhysicalExponent:4;
    UCHAR ProtectionInfoExponent:4;
    UCHAR LowestAlignedBlock_MSB:6;
    UCHAR LBPRZ:1;
    UCHAR LBPME:1;
    UCHAR LowestAlignedBlock_LSB;
    UCHAR Reserved3[16];
} READ_CAPACITY16_DATA, *PREAD_CAPACITY16_DATA;

typedef struct _LBA_STATUS_DESCRIPTOR
{
    ULONGLONG StartingLBA;
    ULONG LogicalBlockCount;
    UCHAR ProvisioningStatus:4;
    UCHAR Reserved1:4;
    UCHAR Reserved2[3];
} LBA_STATUS_DESCRIPTOR, *PLBA_STATUS_DESCRIPTOR;

typedef struct _LBA_STATUS_LIST_HEADER
{
    ULONG ParameterLength;
    ULONG Reserved;
    LBA_STATUS_DESCRIPTOR Descriptors[0];
} LBA_STATUS_LIST_HEADER, *PLBA_STATUS_LIST_HEADER;

typedef struct _READ_BLOCK_LIMITS
{
    UCHAR Reserved;
    UCHAR BlockMaximumSize[3];
    UCHAR BlockMinimumSize[2];
} READ_BLOCK_LIMITS_DATA, *PREAD_BLOCK_LIMITS_DATA;

typedef struct _READ_BUFFER_CAPACITY_DATA
{
    UCHAR DataLength[2];
    UCHAR Reserved1;
    UCHAR BlockDataReturned:1;
    UCHAR Reserved4:7;
    UCHAR TotalBufferSize[4];
    UCHAR AvailableBufferSize[4];
} READ_BUFFER_CAPACITY_DATA, *PREAD_BUFFER_CAPACITY_DATA;

typedef struct _MODE_DISCONNECT_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR BufferFullRatio;
    UCHAR BufferEmptyRatio;
    UCHAR BusInactivityLimit[2];
    UCHAR BusDisconnectTime[2];
    UCHAR BusConnectTime[2];
    UCHAR MaximumBurstSize[2];
    UCHAR DataTransferDisconnect:2;
    UCHAR Reserved2[3];
} MODE_DISCONNECT_PAGE, *PMODE_DISCONNECT_PAGE;

typedef struct _MODE_CACHING_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR ReadDisableCache:1;
    UCHAR MultiplicationFactor:1;
    UCHAR WriteCacheEnable:1;
    UCHAR Reserved2:5;
    UCHAR WriteRetensionPriority:4;
    UCHAR ReadRetensionPriority:4;
    UCHAR DisablePrefetchTransfer[2];
    UCHAR MinimumPrefetch[2];
    UCHAR MaximumPrefetch[2];
    UCHAR MaximumPrefetchCeiling[2];
} MODE_CACHING_PAGE, *PMODE_CACHING_PAGE;

typedef struct _MODE_CDROM_WRITE_PARAMETERS_PAGE2
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR WriteType:4;
    UCHAR TestWrite:1;
    UCHAR LinkSizeValid:1;
    UCHAR BufferUnderrunFreeEnabled:1;
    UCHAR Reserved2:1;
    UCHAR TrackMode:4;
    UCHAR Copy:1;
    UCHAR FixedPacket:1;
    UCHAR MultiSession:2;
    UCHAR DataBlockType:4;
    UCHAR Reserved3:4;
    UCHAR LinkSize;
    UCHAR Reserved4;
    UCHAR HostApplicationCode:6;
    UCHAR Reserved5:2;
    UCHAR SessionFormat;
    UCHAR Reserved6;
    UCHAR PacketSize[4];
    UCHAR AudioPauseLength[2];
    UCHAR MediaCatalogNumber[16];
    UCHAR ISRC[16];
    UCHAR SubHeaderData[4];
} MODE_CDROM_WRITE_PARAMETERS_PAGE2, *PMODE_CDROM_WRITE_PARAMETERS_PAGE2;

typedef struct _MODE_MRW_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR Reserved1;
    UCHAR LbaSpace:1;
    UCHAR Reserved2:7;
    UCHAR Reserved3[4];
} MODE_MRW_PAGE, *PMODE_MRW_PAGE;

typedef struct _MODE_FLEXIBLE_DISK_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR TransferRate[2];
    UCHAR NumberOfHeads;
    UCHAR SectorsPerTrack;
    UCHAR BytesPerSector[2];
    UCHAR NumberOfCylinders[2];
    UCHAR StartWritePrecom[2];
    UCHAR StartReducedCurrent[2];
    UCHAR StepRate[2];
    UCHAR StepPluseWidth;
    UCHAR HeadSettleDelay[2];
    UCHAR MotorOnDelay;
    UCHAR MotorOffDelay;
    UCHAR Reserved2:5;
    UCHAR MotorOnAsserted:1;
    UCHAR StartSectorNumber:1;
    UCHAR TrueReadySignal:1;
    UCHAR StepPlusePerCyclynder:4;
    UCHAR Reserved3:4;
    UCHAR WriteCompenstation;
    UCHAR HeadLoadDelay;
    UCHAR HeadUnloadDelay;
    UCHAR Pin2Usage:4;
    UCHAR Pin34Usage:4;
    UCHAR Pin1Usage:4;
    UCHAR Pin4Usage:4;
    UCHAR MediumRotationRate[2];
    UCHAR Reserved4[2];
} MODE_FLEXIBLE_DISK_PAGE, *PMODE_FLEXIBLE_DISK_PAGE;

typedef struct _MODE_FORMAT_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR TracksPerZone[2];
    UCHAR AlternateSectorsPerZone[2];
    UCHAR AlternateTracksPerZone[2];
    UCHAR AlternateTracksPerLogicalUnit[2];
    UCHAR SectorsPerTrack[2];
    UCHAR BytesPerPhysicalSector[2];
    UCHAR Interleave[2];
    UCHAR TrackSkewFactor[2];
    UCHAR CylinderSkewFactor[2];
    UCHAR Reserved2:4;
    UCHAR SurfaceFirst:1;
    UCHAR RemovableMedia:1;
    UCHAR HardSectorFormating:1;
    UCHAR SoftSectorFormating:1;
    UCHAR Reserved3[3];
} MODE_FORMAT_PAGE, *PMODE_FORMAT_PAGE;

typedef struct _MODE_RIGID_GEOMETRY_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR NumberOfCylinders[3];
    UCHAR NumberOfHeads;
    UCHAR StartWritePrecom[3];
    UCHAR StartReducedCurrent[3];
    UCHAR DriveStepRate[2];
    UCHAR LandZoneCyclinder[3];
    UCHAR RotationalPositionLock:2;
    UCHAR Reserved2:6;
    UCHAR RotationOffset;
    UCHAR Reserved3;
    UCHAR RoataionRate[2];
    UCHAR Reserved4[2];
} MODE_RIGID_GEOMETRY_PAGE, *PMODE_RIGID_GEOMETRY_PAGE;

typedef struct _MODE_READ_WRITE_RECOVERY_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved1:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR DCRBit:1;
    UCHAR DTEBit:1;
    UCHAR PERBit:1;
    UCHAR EERBit:1;
    UCHAR RCBit:1;
    UCHAR TBBit:1;
    UCHAR ARRE:1;
    UCHAR AWRE:1;
    UCHAR ReadRetryCount;
    UCHAR Reserved4[4];
    UCHAR WriteRetryCount;
    UCHAR Reserved5[3];
} MODE_READ_WRITE_RECOVERY_PAGE, *PMODE_READ_WRITE_RECOVERY_PAGE;

typedef struct _MODE_READ_RECOVERY_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved1:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR DCRBit:1;
    UCHAR DTEBit:1;
    UCHAR PERBit:1;
    UCHAR Reserved2:1;
    UCHAR RCBit:1;
    UCHAR TBBit:1;
    UCHAR Reserved3:2;
    UCHAR ReadRetryCount;
    UCHAR Reserved4[4];
} MODE_READ_RECOVERY_PAGE, *PMODE_READ_RECOVERY_PAGE;

typedef struct _MODE_INFO_EXCEPTIONS
{
    UCHAR PageCode:6;
    UCHAR Reserved1:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    _ANONYMOUS_UNION union
    {
        UCHAR Flags;
        _ANONYMOUS_STRUCT struct
        {
            UCHAR LogErr:1;
            UCHAR Reserved2:1;
            UCHAR Test:1;
            UCHAR Dexcpt:1;
            UCHAR Reserved3:3;
            UCHAR Perf:1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    UCHAR ReportMethod:4;
    UCHAR Reserved4:4;
    UCHAR IntervalTimer[4];
    UCHAR ReportCount[4];
} MODE_INFO_EXCEPTIONS, *PMODE_INFO_EXCEPTIONS;

typedef struct _POWER_CONDITION_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR Reserved2;
    UCHAR Standby:1;
    UCHAR Idle:1;
    UCHAR Reserved3:6;
    UCHAR IdleTimer[4];
    UCHAR StandbyTimer[4];
} POWER_CONDITION_PAGE, *PPOWER_CONDITION_PAGE;

typedef struct _CDDA_OUTPUT_PORT
{
    UCHAR ChannelSelection:4;
    UCHAR Reserved:4;
    UCHAR Volume;
} CDDA_OUTPUT_PORT, *PCDDA_OUTPUT_PORT;

typedef struct _CDAUDIO_CONTROL_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR Reserved2:1;
    UCHAR StopOnTrackCrossing:1;
    UCHAR Immediate:1;
    UCHAR Reserved3:5;
    UCHAR Reserved4[3];
    UCHAR Obsolete[2];
    CDDA_OUTPUT_PORT CDDAOutputPorts[4];
} CDAUDIO_CONTROL_PAGE, *PCDAUDIO_CONTROL_PAGE;

typedef struct _CDVD_FEATURE_SET_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR CDAudio[2];
    UCHAR EmbeddedChanger[2];
    UCHAR PacketSMART[2];
    UCHAR PersistantPrevent[2];
    UCHAR EventStatusNotification[2];
    UCHAR DigitalOutput[2];
    UCHAR CDSequentialRecordable[2];
    UCHAR DVDSequentialRecordable[2];
    UCHAR RandomRecordable[2];
    UCHAR KeyExchange[2];
    UCHAR Reserved2[2];
} CDVD_FEATURE_SET_PAGE, *PCDVD_FEATURE_SET_PAGE;

typedef struct _CDVD_INACTIVITY_TIMEOUT_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR Reserved2[2];
    UCHAR SWPP:1;
    UCHAR DISP:1;
    UCHAR Reserved3:6;
    UCHAR Reserved4;
    UCHAR GroupOneMinimumTimeout[2];
    UCHAR GroupTwoMinimumTimeout[2];
} CDVD_INACTIVITY_TIMEOUT_PAGE, *PCDVD_INACTIVITY_TIMEOUT_PAGE;

typedef struct _CDVD_CAPABILITIES_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved:1;
    UCHAR PSBit:1;
    UCHAR PageLength;
    UCHAR CDRRead:1;
    UCHAR CDERead:1;
    UCHAR Method2:1;
    UCHAR DVDROMRead:1;
    UCHAR DVDRRead:1;
    UCHAR DVDRAMRead:1;
    UCHAR Reserved2:2;
    UCHAR CDRWrite:1;
    UCHAR CDEWrite:1;
    UCHAR TestWrite:1;
    UCHAR Reserved3:1;
    UCHAR DVDRWrite:1;
    UCHAR DVDRAMWrite:1;
    UCHAR Reserved4:2;
    UCHAR AudioPlay:1;
    UCHAR Composite:1;
    UCHAR DigitalPortOne:1;
    UCHAR DigitalPortTwo:1;
    UCHAR Mode2Form1:1;
    UCHAR Mode2Form2:1;
    UCHAR MultiSession:1;
    UCHAR BufferUnderrunFree:1;
    UCHAR CDDA:1;
    UCHAR CDDAAccurate:1;
    UCHAR RWSupported:1;
    UCHAR RWDeinterleaved:1;
    UCHAR C2Pointers:1;
    UCHAR ISRC:1;
    UCHAR UPC:1;
    UCHAR ReadBarCodeCapable:1;
    UCHAR Lock:1;
    UCHAR LockState:1;
    UCHAR PreventJumper:1;
    UCHAR Eject:1;
    UCHAR Reserved6:1;
    UCHAR LoadingMechanismType:3;
    UCHAR SeparateVolume:1;
    UCHAR SeperateChannelMute:1;
    UCHAR SupportsDiskPresent:1;
    UCHAR SWSlotSelection:1;
    UCHAR SideChangeCapable:1;
    UCHAR RWInLeadInReadable:1;
    UCHAR Reserved7:2;
    _ANONYMOUS_UNION union
    {
        UCHAR ReadSpeedMaximum[2];
        UCHAR ObsoleteReserved[2];
    } DUMMYUNIONNAME;
    UCHAR NumberVolumeLevels[2];
    UCHAR BufferSize[2];
    _ANONYMOUS_UNION union
    {
        UCHAR ReadSpeedCurrent[2];
        UCHAR ObsoleteReserved2[2];
    } DUMMYUNIONNAME2;
    UCHAR ObsoleteReserved3;
    UCHAR Reserved8:1;
    UCHAR BCK:1;
    UCHAR RCK:1;
    UCHAR LSBF:1;
    UCHAR Length:2;
    UCHAR Reserved9:2;
    _ANONYMOUS_UNION union
    {
        UCHAR WriteSpeedMaximum[2];
        UCHAR ObsoleteReserved4[2];
    } DUMMYUNIONNAME3;
    _ANONYMOUS_UNION union
    {
        UCHAR WriteSpeedCurrent[2];
        UCHAR ObsoleteReserved11[2];
    } DUMMYUNIONNAME4;
    _ANONYMOUS_UNION union
    {
        UCHAR CopyManagementRevision[2];
        UCHAR Reserved10[2];
    } DUMMYUNIONNAME5;
} CDVD_CAPABILITIES_PAGE, *PCDVD_CAPABILITIES_PAGE;

typedef struct _MODE_PARM_READ_WRITE
{
    MODE_PARAMETER_HEADER ParameterListHeader;
    MODE_PARAMETER_BLOCK ParameterListBlock;
} MODE_PARM_READ_WRITE_DATA, *PMODE_PARM_READ_WRITE_DATA;

typedef struct _PORT_OUTPUT
{
    UCHAR ChannelSelection;
    UCHAR Volume;
} PORT_OUTPUT, *PPORT_OUTPUT;

typedef struct _AUDIO_OUTPUT
{
    UCHAR CodePage;
    UCHAR ParameterLength;
    UCHAR Immediate;
    UCHAR Reserved[2];
    UCHAR LbaFormat;
    UCHAR LogicalBlocksPerSecond[2];
    PORT_OUTPUT PortOutput[4];
} AUDIO_OUTPUT, *PAUDIO_OUTPUT;

typedef struct _MECHANICAL_STATUS_INFORMATION_HEADER
{
    UCHAR CurrentSlot:5;
    UCHAR ChangerState:2;
    UCHAR Fault:1;
    UCHAR Reserved:5;
    UCHAR MechanismState:3;
    UCHAR CurrentLogicalBlockAddress[3];
    UCHAR NumberAvailableSlots;
    UCHAR SlotTableLength[2];
} MECHANICAL_STATUS_INFORMATION_HEADER, *PMECHANICAL_STATUS_INFORMATION_HEADER;

typedef struct _SLOT_TABLE_INFORMATION
{
    UCHAR DiscChanged:1;
    UCHAR Reserved:6;
    UCHAR DiscPresent:1;
    UCHAR Reserved2[3];
} SLOT_TABLE_INFORMATION, *PSLOT_TABLE_INFORMATION;

typedef struct _MECHANICAL_STATUS
{
    MECHANICAL_STATUS_INFORMATION_HEADER MechanicalStatusHeader;
    SLOT_TABLE_INFORMATION SlotTableInfo[1];
} MECHANICAL_STATUS, *PMECHANICAL_STATUS;

typedef struct _TAPE_POSITION_DATA
{
    UCHAR Reserved1:2;
    UCHAR BlockPositionUnsupported:1;
    UCHAR Reserved2:3;
    UCHAR EndOfPartition:1;
    UCHAR BeginningOfPartition:1;
    UCHAR PartitionNumber;
    USHORT Reserved3;
    UCHAR FirstBlock[4];
    UCHAR LastBlock[4];
    UCHAR Reserved4;
    UCHAR NumberOfBlocks[3];
    UCHAR NumberOfBytes[4];
} TAPE_POSITION_DATA, *PTAPE_POSITION_DATA;


/* ZONE_DESCRIPTIOR.ZoneType */
#define ZONE_TYPE_CONVENTIONAL              0x1
#define ZONE_TYPE_SEQUENTIAL_WRITE_REQUIRED 0x2
#define ZONE_TYPE_SEQUENTIAL_WRITE_PREFERRED 0x3

/* ZONE_DESCRIPTIOR.ZoneCondition */
#define ZONE_CONDITION_NOT_WRITE_POINTER    0x0
#define ZONE_CONDITION_EMPTY                0x1
#define ZONE_CONDITION_IMPLICITLY_OPENED    0x2
#define ZONE_CONDITION_EXPLICITLY_OPENED    0x3
#define ZONE_CONDITION_CLOSED               0x4
#define ZONE_CONDITION_READ_ONLY            0xD
#define ZONE_CONDITION_FULL                 0xE
#define ZONE_CONDITION_OFFLINE              0xF

/* PHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR.PhysicalElementType */
#define PHYSICAL_ELEMENT_TYPE_STORAGE_ELEMENT 0x01

/* PHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR.PhysicalElementHealth */
#define PHYSICAL_ELEMENT_HEALTH_NOT_REPORTED 0x00
#define PHYSICAL_ELEMENT_HEALTH_MANUFACTURER_SPECIFICATION_LIMIT 0x64
#define PHYSICAL_ELEMENT_HEALTH_RESERVED_LOWER_BOUNDARY 0xD0
#define PHYSICAL_ELEMENT_HEALTH_RESERVED_UPPER_BOUNDARY 0xFC
#define PHYSICAL_ELEMENT_HEALTH_DEPOPULATION_COMPLETED_WITH_ERROR 0xFD
#define PHYSICAL_ELEMENT_HEALTH_DEPOPULATION_IN_PROGRESS 0xFE
#define PHYSICAL_ELEMENT_HEALTH_DEPOPULATION_COMPLETED_SUCCESS 0xFF

/* READ BUFFER buffer ids that address the error history */
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY 0x0
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_CREATE_NEW_ERROR_HISTORY_SNAPSHOT 0x1
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_ESTABLISH_NEW_NEXUS 0x2
#define BUFFER_ID_RETURN_ERROR_HISTORY_DIRECTORY_ESTABLISH_NEW_NEXUS_AND_SNAPSHOT 0x3
#define BUFFER_ID_RETURN_ERROR_HISTORY_MINIMUM_THRESHOLD 0x10
#define BUFFER_ID_RETURN_ERROR_HISTORY_MAXIMUM_THRESHOLD 0xEF
#define BUFFER_ID_CLEAR_ERROR_HISTORY_NEXUS 0xFE
#define BUFFER_ID_CLEAR_ERROR_HISTORY_AND_RELEASE_ANY_SNAPSHOT 0xFF

/* ERROR_HISTORY_DIRECTORY.ErrorHistorySource */
#define ERROR_HISTORY_SOURCE_CREATED_BY_DEVICE_SERVER 0x0
#define ERROR_HISTORY_SOURCE_CREATED_DUE_TO_CURRENT_READ_BUFFER_COMMAND 0x1
#define ERROR_HISTORY_SOURCE_CREATED_DUE_TO_PREVIOUS_READ_BUFFER_COMMAND 0x2
#define ERROR_HISTORY_SOURCE_INDICATED_IN_BUFFER_SOURCE_FIELD 0x3

/* ERROR_HISTORY_DIRECTORY.ErrorHistoryRetrieved */
#define ERROR_HISTORY_RETRIEVED_NO_INFORMATION 0x0
#define ERROR_HISTORY_RETRIEVED_BUFFER_ID_FE_OR_FF 0x1
#define ERROR_HISTORY_RETRIEVED_NOT_BUFFER_ID_FE_OR_FF 0x2
#define ERROR_HISTORY_RETRIEVED_RESERVED    0x3
typedef struct _ZONE_DESCRIPTIOR
{
    UCHAR ZoneType:4;
    UCHAR Reserved1:4;
    UCHAR Reset:1;
    UCHAR Non_Seq:1;
    UCHAR Reserved2:2;
    UCHAR ZoneCondition:4;
    UCHAR Reserved3[6];
    UCHAR ZoneLength[8];
    UCHAR ZoneStartLBA[8];
    UCHAR WritePointerLBA[8];
    UCHAR Reserved4[32];
} ZONE_DESCRIPTIOR, *PZONE_DESCRIPTIOR;

typedef struct _REPORT_ZONES_DATA
{
    UCHAR ZoneListLength[4];
    UCHAR Same:4;
    UCHAR Reserved1:4;
    UCHAR Reserved2[3];
    UCHAR MaxLBA[8];
    UCHAR Reserved3[48];
#if !defined(__midl)
    ZONE_DESCRIPTIOR ZoneDescriptors[ANYSIZE_ARRAY];
#endif
} REPORT_ZONES_DATA, *PREPORT_ZONES_DATA;

typedef struct _PHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR
{
    UCHAR Reserved1[4];
    UCHAR ElementIdentifier[4];
    UCHAR Reserved2[6];
    UCHAR PhysicalElementType;
    UCHAR PhysicalElementHealth;
    UCHAR AssociatedCapacity[8];
    UCHAR Reserved3[8];
} PHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR, *PPHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR;

typedef struct _PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA
{
    UCHAR DescriptorCount[4];
    UCHAR ReturnedDescriptorCount[4];
    UCHAR ElementIdentifierBeingDepoped[4];
    UCHAR Reserved[20];
    PHYSICAL_ELEMENT_STATUS_DATA_DESCRIPTOR Descriptors[ANYSIZE_ARRAY];
} PHYSICAL_ELEMENT_STATUS_PARAMETER_DATA, *PPHYSICAL_ELEMENT_STATUS_PARAMETER_DATA;

typedef struct _ERROR_HISTORY_DIRECTORY_ENTRY
{
    UCHAR SupportedBufferId;
    UCHAR BufferFormat;
    UCHAR BufferSource:4;
    UCHAR Reserved0:4;
    UCHAR Reserved1;
    UCHAR MaxAvailableLength[4];
} ERROR_HISTORY_DIRECTORY_ENTRY, *PERROR_HISTORY_DIRECTORY_ENTRY;

typedef struct _ERROR_HISTORY_DIRECTORY
{
    UCHAR T10VendorId[8];
    UCHAR ErrorHistoryVersion;
    UCHAR ClearSupport:1;
    UCHAR ErrorHistorySource:2;
    UCHAR ErrorHistoryRetrieved:2;
    UCHAR Reserved0:3;
    UCHAR Reserved1[20];
    UCHAR DirectoryLength[2];
    ERROR_HISTORY_DIRECTORY_ENTRY ErrorHistoryDirectoryList[ANYSIZE_ARRAY];
} ERROR_HISTORY_DIRECTORY, *PERROR_HISTORY_DIRECTORY;

typedef struct _CURRENT_INTERNAL_STATUS_PARAMETER_DATA
{
    UCHAR Reserved0[4];
    UCHAR IEEECompanyId[4];
    UCHAR CurrentInternalStatusDataSetOneLength[2];
    UCHAR CurrentInternalStatusDataSetTwoLength[2];
    UCHAR CurrentInternalStatusDataSetThreeLength[2];
    UCHAR CurrentInternalStatusDataSetFourLength[4];
    UCHAR Reserved1[364];
    UCHAR NewSavedDataAvailable;
    UCHAR SavedDataGenerationNumber;
    UCHAR CurrentReasonIdentifier[128];
    UCHAR CurrentInternalStatusData[ANYSIZE_ARRAY];
} CURRENT_INTERNAL_STATUS_PARAMETER_DATA, *PCURRENT_INTERNAL_STATUS_PARAMETER_DATA;

typedef struct _SAVED_INTERNAL_STATUS_PARAMETER_DATA
{
    UCHAR Reserved0[4];
    UCHAR IEEECompanyId[4];
    UCHAR SavedInternalStatusDataSetOneLength[2];
    UCHAR SavedInternalStatusDataSetTwoLength[2];
    UCHAR SavedInternalStatusDataSetThreeLength[2];
    UCHAR SavedInternalStatusDataSetFourLength[4];
    UCHAR Reserved1[364];
    UCHAR NewSavedDataAvailable;
    UCHAR SavedDataGenerationNumber;
    UCHAR SavedReasonIdentifier[128];
    UCHAR SavedInternalStatusData[ANYSIZE_ARRAY];
} SAVED_INTERNAL_STATUS_PARAMETER_DATA, *PSAVED_INTERNAL_STATUS_PARAMETER_DATA;

typedef struct _SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA
{
    UCHAR Reserved1[6];
    UCHAR SupportedSecurityListLength[2];
    UCHAR SupportedSecurityProtocol[0];
} SUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA, *PSUPPORTED_SECURITY_PROTOCOLS_PARAMETER_DATA;

typedef struct _OVERWRITE_PARAMETER_LIST
{
    UCHAR OverWriteCount:5;
    UCHAR Test:2;
    UCHAR Invert:1;
    UCHAR Reserved1;
    UCHAR InitializationPatternLength[2];
#if !defined(__midl)
    UCHAR InitializationPattern[ANYSIZE_ARRAY];
#endif
} OVERWRITE_PARAMETER_LIST, *POVERWRITE_PARAMETER_LIST;

typedef struct
{
    UCHAR ParameterDataLength[2];
    UCHAR Origin:3;
    UCHAR Reserved1:5;
    UCHAR Reserved2;
    UCHAR Timestamp[6];
    UCHAR Reserved3[2];
} RT_PARAMETER_DATA, *PRT_PARAMETER_DATA;

typedef struct
{
    UCHAR Reserved1[4];
    UCHAR Timestamp[6];
    UCHAR Reserved2[2];
} ST_PARAMETER_DATA, *PST_PARAMETER_DATA;

#include <pshpack1.h>

typedef struct _UNMAP_BLOCK_DESCRIPTOR
{
    UCHAR StartingLba[8];
    UCHAR LbaCount[4];
    UCHAR Reserved[4];
} UNMAP_BLOCK_DESCRIPTOR, *PUNMAP_BLOCK_DESCRIPTOR;

typedef struct _UNMAP_LIST_HEADER
{
    UCHAR DataLength[2];
    UCHAR BlockDescrDataLength[2];
    UCHAR Reserved[4];
#if !defined(__midl)
    UNMAP_BLOCK_DESCRIPTOR Descriptors[0];
#endif
} UNMAP_LIST_HEADER, *PUNMAP_LIST_HEADER;

typedef struct _LOG_PARAMETER_HEADER
{
    UCHAR ParameterCode[2];
    union
    {
        UCHAR ControlByte;
        struct
        {
            UCHAR FormatAndLinking:2;
            UCHAR TMC:2;
            UCHAR ETC:1;
            UCHAR TSD:1;
            UCHAR Obsolete:1;
            UCHAR DU:1;
        };
    };
    UCHAR ParameterLength;
} LOG_PARAMETER_HEADER, *PLOG_PARAMETER_HEADER;

typedef struct _LOG_PARAMETER
{
    LOG_PARAMETER_HEADER Header;
    union
    {
#if !defined(__midl)
        UCHAR AsByte[0];
#endif
        struct _THRESHOLD_RESOURCE_COUNT
        {
            UCHAR ResourceCount[4];
            UCHAR Scope : 2;
            UCHAR Reserved1 : 6;
            UCHAR Reserved2[3];
        } THRESHOLD_RESOURCE_COUNT;
        struct _TEMPERATURE
        {
            UCHAR Reserved;
            UCHAR Temperature;
        } TEMPERATURE;
        struct _DATE_OF_MANUFACTURE
        {
            UCHAR Year[4];
            UCHAR Week[2];
        } DATE_OF_MANUFACTURE;
        struct _SELF_TEST_RESULTS
        {
            UCHAR SelfTestResults : 4;
            UCHAR Reserved1 : 1;
            UCHAR SelfTestCode : 3;
            UCHAR SelfTestNumber;
            UCHAR PowerOnHours[2];
            UCHAR AddressOfFirstFailure[8];
            UCHAR SenseKey : 4;
            UCHAR Reserved2 : 4;
            UCHAR AdditionalSenseCode;
            UCHAR AdditionalSenseCodeQualifier;
            UCHAR VendorSpecific;
        } SELF_TEST_RESULTS;

        struct _SOLID_STATE_MEDIA
        {
            UCHAR Reserved[3];
            UCHAR PercentageUsed;
        } SOLID_STATE_MEDIA;

        struct _BACKGROUND_SCAN_STATUS
        {
            UCHAR PowerOnMinutes[4];
            UCHAR Reserved;
            UCHAR ScanStatus;
            UCHAR ScansPerformed[2];
            UCHAR ScanProgress[2];
            UCHAR MediumScansPerformed[2];
        } BACKGROUND_SCAN_STATUS;

        struct _INFORMATIONAL_EXCEPTIONS
        {
            UCHAR ASC;
            UCHAR ASCQ;
            UCHAR MostRecentTemperature;
            UCHAR VendorSpecific[ANYSIZE_ARRAY];
        } INFORMATIONAL_EXCEPTIONS;
    };
} LOG_PARAMETER, *PLOG_PARAMETER;

typedef struct _LOG_PAGE
{
    UCHAR PageCode:6;
    UCHAR SPF:1;
    UCHAR DS:1;
    UCHAR SubPageCode;
    UCHAR PageLength[2];
#if !defined(__midl)
    LOG_PARAMETER Parameters[0];
#endif

} LOG_PAGE, *PLOG_PAGE;

#include <poppack.h>

typedef struct _LOG_PARAMETER_THRESHOLD_RESOURCE_COUNT
{
    LOG_PARAMETER_HEADER Header;
    UCHAR ResourceCount[4];
    UCHAR Scope:2;
    UCHAR Reserved0:6;
    UCHAR Reserved1[3];
} LOG_PARAMETER_THRESHOLD_RESOURCE_COUNT, *PLOG_PARAMETER_THRESHOLD_RESOURCE_COUNT;

typedef struct _LOG_PAGE_LOGICAL_BLOCK_PROVISIONING
{
    UCHAR PageCode:6;
    UCHAR SPF:1;
    UCHAR DS:1;
    UCHAR SubPageCode;
    UCHAR PageLength[2];
#if !defined(__midl)
    LOG_PARAMETER_HEADER Parameters[0];
#endif
} LOG_PAGE_LOGICAL_BLOCK_PROVISIONING, *PLOG_PAGE_LOGICAL_BLOCK_PROVISIONING;

typedef struct _MODE_CONTROL_PAGE
{
    UCHAR PageCode:6;
    UCHAR Reserved1:1;
    UCHAR PageSavable:1;
    UCHAR PageLength;
    UCHAR RLEC:1;
    UCHAR GLTSD:1;
    UCHAR D_SENSE:1;
    UCHAR DPICZ:1;
    UCHAR TMF_ONLY:1;
    UCHAR TST:3;
    UCHAR Obsolete1:1;
    UCHAR QERR:2;
    UCHAR NUAR:1;
    UCHAR QueueAlgorithmModifier:4;
    UCHAR Obsolete2:3;
    UCHAR SWP:1;
    UCHAR UA_INTLCK_CTRL:2;
    UCHAR RAC:1;
    UCHAR VS:1;
    UCHAR AutoloadMode:3;
    UCHAR Reserved2:1;
    UCHAR RWWP:1;
    UCHAR ATMPE:1;
    UCHAR TAS:1;
    UCHAR ATO:1;
    UCHAR Obsolete3[2];
    UCHAR BusyTimeoutPeriod[2];
    UCHAR ExtendeSelfTestCompletionTime[2];
} MODE_CONTROL_PAGE, *PMODE_CONTROL_PAGE;

typedef struct _MODE_CDROM_WRITE_PARAMETERS_PAGE
{
    UCHAR PageLength;
    UCHAR WriteType:4;
    UCHAR TestWrite:1;
    UCHAR LinkSizeValid:1;
    UCHAR BufferUnderrunFreeEnabled:1;
    UCHAR Reserved2:1;
    UCHAR TrackMode:4;
    UCHAR Copy:1;
    UCHAR FixedPacket:1;
    UCHAR MultiSession:2;
    UCHAR DataBlockType:4;
    UCHAR Reserved3:4;
    UCHAR LinkSize;
    UCHAR Reserved4;
    UCHAR HostApplicationCode:6;
    UCHAR Reserved5:2;
    UCHAR SessionFormat;
    UCHAR Reserved6;
    UCHAR PacketSize[4];
    UCHAR AudioPauseLength[2];
    UCHAR Reserved7:7;
    UCHAR MediaCatalogNumberValid:1;
    UCHAR MediaCatalogNumber[13];
    UCHAR MediaCatalogNumberZero;
    UCHAR MediaCatalogNumberAFrame;
    UCHAR Reserved8:7;
    UCHAR ISRCValid:1;
    UCHAR ISRCCountry[2];
    UCHAR ISRCOwner[3];
    UCHAR ISRCRecordingYear[2];
    UCHAR ISRCSerialNumber[5];
    UCHAR ISRCZero;
    UCHAR ISRCAFrame;
    UCHAR ISRCReserved;
    UCHAR SubHeaderData[4];
} MODE_CDROM_WRITE_PARAMETERS_PAGE, *PMODE_CDROM_WRITE_PARAMETERS_PAGE;

#include <pshpack1.h>

typedef struct
{
    UCHAR LogicalBlockAddress[8];
    UCHAR TransferLength[4];
    UCHAR Reserved[4];
} BLOCK_DEVICE_RANGE_DESCRIPTOR, *PBLOCK_DEVICE_RANGE_DESCRIPTOR;

typedef struct
{
    UCHAR PopulateTokenDataLength[2];
    UCHAR Immediate:1;
    UCHAR Reserved1:7;
    UCHAR Reserved2;
    UCHAR InactivityTimeout[4];
    UCHAR Reserved3[6];
    UCHAR BlockDeviceRangeDescriptorListLength[2];
#if !defined(__midl)
    UCHAR BlockDeviceRangeDescriptor[ANYSIZE_ARRAY];
#endif
} POPULATE_TOKEN_HEADER, *PPOPULATE_TOKEN_HEADER;

typedef struct
{
    UCHAR WriteUsingTokenDataLength[2];
    UCHAR Immediate:1;
    UCHAR Reserved1:7;
    UCHAR Reserved2[5];
    UCHAR BlockOffsetIntoToken[8];
    UCHAR Token[BLOCK_DEVICE_TOKEN_SIZE];
    UCHAR Reserved3[6];
    UCHAR BlockDeviceRangeDescriptorListLength[2];
#if !defined(__midl)
    UCHAR BlockDeviceRangeDescriptor[ANYSIZE_ARRAY];
#endif
} WRITE_USING_TOKEN_HEADER, *PWRITE_USING_TOKEN_HEADER;

typedef struct
{
    UCHAR AvailableData[4];
    UCHAR ResponseToServiceAction:5;
    UCHAR Reserved1:3;
    UCHAR OperationStatus:7;
    UCHAR Reserved2:1;
    UCHAR OperationCounter[2];
    UCHAR EstimatedStatusUpdateDelay[4];
    UCHAR CompletionStatus;
    UCHAR SenseDataFieldLength;
    UCHAR SenseDataLength;
    UCHAR TransferCountUnits;
    UCHAR TransferCount[8];
    UCHAR SegmentsProcessed[2];
    UCHAR Reserved3[6];
#if !defined(__midl)
    UCHAR SenseData[ANYSIZE_ARRAY];
#endif
} RECEIVE_TOKEN_INFORMATION_HEADER, *PRECEIVE_TOKEN_INFORMATION_HEADER;

typedef struct
{
    UCHAR TokenDescriptorsLength[4];
#if !defined(__midl)
    UCHAR TokenDescriptor[ANYSIZE_ARRAY];
#endif
} RECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER, *PRECEIVE_TOKEN_INFORMATION_RESPONSE_HEADER;

typedef struct
{
    UCHAR TokenIdentifier[2];
    UCHAR Token[BLOCK_DEVICE_TOKEN_SIZE];
} BLOCK_DEVICE_TOKEN_DESCRIPTOR, *PBLOCK_DEVICE_TOKEN_DESCRIPTOR;

typedef enum _OPERATION_STATUS
{
    OPERATION_COMPLETED_WITH_SUCCESS = 0x01,
    OPERATION_COMPLETED_WITH_ERROR = 0x02,
    OPERATION_COMPLETED_WITH_RESIDUAL_DATA = 0x03,
    OPERATION_IN_PROGRESS_IN_FOREGROUND = 0x11,
    OPERATION_IN_PROGRESS_IN_BACKGROUND = 0x12,
    OPERATION_TERMINATED = 0x60
} OPERATION_STATUS, *POPERATION_STATUS;

typedef enum _TRANSFER_COUNT_UNITS
{
    TRANSFER_COUNT_UNITS_BYTES = 0,
    TRANSFER_COUNT_UNITS_KIBIBYTES = 1,
    TRANSFER_COUNT_UNITS_MEBIBYTES = 2,
    TRANSFER_COUNT_UNITS_GIBIBYTES = 3,
    TRANSFER_COUNT_UNITS_TEBIBYTES = 4,
    TRANSFER_COUNT_UNITS_PEBIBYTES = 5,
    TRANSFER_COUNT_UNITS_EXBIBYTES = 6,
    TRANSFER_COUNT_UNITS_NUMBER_BLOCKS = 0xF1
} TRANSFER_COUNT_UNITS, *PTRANSFER_COUNT_UNITS;

#include <poppack.h>

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

typedef
BOOLEAN
(NTAPI *PHW_MESSAGE_SIGNALED_INTERRUPT_ROUTINE)(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId);

typedef struct _PORT_CONFIGURATION_INFORMATION
{
    ULONG Length;
    ULONG SystemIoBusNumber;
    INTERFACE_TYPE AdapterInterfaceType;
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
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PVOID MiniportDumpData;
#else
    PVOID Reserved;
#endif
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
#if (NTDDI_VERSION >= NTDDI_WIN8)
    UCHAR SrbType;
    UCHAR AddressType;
#else
    UCHAR ReservedUchars[2];
#endif
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
    UCHAR Dma64BitAddresses;
    BOOLEAN ResetTargetSupported;
    UCHAR MaximumNumberOfLogicalUnits;
    BOOLEAN WmiDataProvider;
    STOR_SYNCHRONIZATION_MODEL SynchronizationModel;
    PHW_MESSAGE_SIGNALED_INTERRUPT_ROUTINE HwMSInterruptRoutine;
    INTERRUPT_SYNCHRONIZATION_MODE InterruptSynchronizationMode;
    MEMORY_REGION DumpRegion;
    ULONG RequestedDumpBufferSize;
    BOOLEAN VirtualDevice;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    UCHAR DumpMode;
#if (NTDDI_VERSION >= NTDDI_WIN10_VB)
    UCHAR DmaAddressWidth;
#endif
    /* One trailing pad byte is unused */
#endif
    ULONG ExtendedFlags1;
    ULONG MaxNumberOfIO;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONG MaxIOsPerLun;
    ULONG InitialLunQueueDepth;
    /* Microseconds */
    ULONG BusResetHoldTime;
    ULONG FeatureSupport;
#endif
} PORT_CONFIGURATION_INFORMATION, *PPORT_CONFIGURATION_INFORMATION;

/* PORT_CONFIGURATION_INFORMATION.ExtendedFlags1 */
#define STOR_ADAPTER_FEATURE_DEVICE_TELEMETRY               0x00000001
#define STOR_ADAPTER_FEATURE_STOP_UNIT_DURING_POWER_DOWN    0x00000002
#define STOR_ADAPTER_UNCACHED_EXTENSION_NUMA_NODE_PREFERRED 0x00000004
#define STOR_ADAPTER_DMA_V3_PREFERRED                       0x00000008
#define STOR_ADAPTER_FEATURE_ABORT_COMMAND                  0x00000010
#define STOR_ADAPTER_FEATURE_RICH_TEMPERATURE_THRESHOLD     0x00000020
#define STOR_ADAPTER_DMA_ADDRESS_WIDTH_SPECIFIED            0x00000040

/* PORT_CONFIGURATION_INFORMATION.DumpMode */
#define DUMP_MODE_CRASH                     0x01
#define DUMP_MODE_HIBER                     0x02
#define DUMP_MODE_MARK_MEMORY               0x03
#define DUMP_MODE_RESUME                    0x04

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

/* PERF_CONFIGURATION_DATA.Flags */
#define STOR_PERF_DPC_REDIRECTION           0x00000001
#define STOR_PERF_CONCURRENT_CHANNELS       0x00000002
#define STOR_PERF_INTERRUPT_MESSAGE_RANGES  0x00000004
#define STOR_PERF_ADV_CONFIG_LOCALITY       0x00000008
#define STOR_PERF_OPTIMIZE_FOR_COMPLETION_DURING_STARTIO 0x00000010
#define STOR_PERF_DPC_REDIRECTION_CURRENT_CPU 0x00000020
#define STOR_PERF_NO_SGL                    0x00000040
#define STOR_PERF_SOFT_NUMA                 0x00000080
#define STOR_PERF_HETEROGENEOUS_CPU         0x00000100

#define STOR_PERF_VERSION_2                 0x00000002
#define STOR_PERF_VERSION_3                 0x00000003
#define STOR_PERF_VERSION_4                 0x00000004
#define STOR_PERF_VERSION_5                 0x00000005
#define STOR_PERF_VERSION_6                 0x00000006
#define STOR_PERF_VERSION_7                 0x00000007
#define STOR_PERF_VERSION_8                 0x00000008

#define STOR_PERF_VERSION                   STOR_PERF_VERSION_8

typedef struct _PERF_CONFIGURATION_DATA
{
    ULONG Version;
    ULONG Size;
    ULONG Flags;
    ULONG ConcurrentChannels;
    ULONG FirstRedirectionMessageNumber;
    ULONG LastRedirectionMessageNumber;
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

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef struct _STOR_UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    _Field_size_bytes_part_(MaximumLength, Length) PWCH Buffer;
} STOR_UNICODE_STRING, *PSTOR_UNICODE_STRING;

typedef enum _STOR_IO_PRIORITY_HINT
{
    StorIoPriorityVeryLow = 0,
    StorIoPriorityLow,
    StorIoPriorityNormal,
    StorIoPriorityHigh,
    StorIoPriorityCritical,
    StorIoMaxPriorityTypes,
    StorIoMaxPriorityValue = 0xffff
} STOR_IO_PRIORITY_HINT, *PSTOR_IO_PRIORITY_HINT;

#define STOR_REQUEST_INFO_VER_1             1
#define STOR_REQUEST_INFO_VER_2             2
#define STOR_REQUEST_INFO_VER               STOR_REQUEST_INFO_VER_2

typedef struct _STOR_REQUEST_INFO_V1
{
    USHORT Version;
    USHORT Size;
    STOR_IO_PRIORITY_HINT PriorityHint;
    ULONG Flags;
    ULONG Key;
    ULONG Length;
    BOOLEAN IsWriteRequest;
    UCHAR Reserved[3];
} STOR_REQUEST_INFO_V1, *PSTOR_REQUEST_INFO_V1;

/*
 * FileName is only valid below DISPATCH_LEVEL and only for as long as the
 * request lives. ProcessId reads back as 0xFFFFFFFF when the query was made
 * above DISPATCH_LEVEL.
 */
typedef struct _STOR_REQUEST_INFO_V2
{
    USHORT Version;
    USHORT Size;
    STOR_IO_PRIORITY_HINT PriorityHint;
    ULONG Flags;
    ULONG Key;
    ULONG Length;
    BOOLEAN IsWriteRequest;
    UCHAR Reserved[3];
    PSTOR_UNICODE_STRING FileName;
    ULONG ProcessId;
} STOR_REQUEST_INFO_V2, *PSTOR_REQUEST_INFO_V2;

typedef STOR_REQUEST_INFO_V2 STOR_REQUEST_INFO, *PSTOR_REQUEST_INFO;

typedef struct _STOR_UNIT_ATTRIBUTES
{
    ULONG DeviceAttentionSupported:1;
    ULONG AsyncNotificationSupported:1;
    ULONG D3ColdNotSupported:1;
    ULONG BypassIoSupported:1;
    ULONG Reserved:28;
} STOR_UNIT_ATTRIBUTES, *PSTOR_UNIT_ATTRIBUTES;

typedef enum _STORPORT_ETW_LEVEL
{
    StorportEtwLevelLogAlways = 0,
    StorportEtwLevelCritical = 1,
    StorportEtwLevelError = 2,
    StorportEtwLevelWarning = 3,
    StorportEtwLevelInformational = 4,
    StorportEtwLevelVerbose = 5,
    StorportEtwLevelMax = StorportEtwLevelVerbose
} STORPORT_ETW_LEVEL, *PSTORPORT_ETW_LEVEL;

#define STORPORT_ETW_EVENT_KEYWORD_IO                   0x0000000000000001
#define STORPORT_ETW_EVENT_KEYWORD_PERFORMANCE          0x0000000000000002
#define STORPORT_ETW_EVENT_KEYWORD_POWER                0x0000000000000004
#define STORPORT_ETW_EVENT_KEYWORD_ENUMERATION          0x0000000000000008

typedef enum _STORPORT_ETW_EVENT_OPCODE
{
    StorportEtwEventOpcodeInfo = 0,
    StorportEtwEventOpcodeStart = 1,
    StorportEtwEventOpcodeStop = 2,
    StorportEtwEventOpcodeDC_Start = 3,
    StorportEtwEventOpcodeDC_Stop = 4,
    StorportEtwEventOpcodeExtension = 5,
    StorportEtwEventOpcodeReply = 6,
    StorportEtwEventOpcodeResume = 7,
    StorportEtwEventOpcodeSuspend = 8,
    StorportEtwEventOpcodeSend = 9,
    StorportEtwEventOpcodeReceive = 240
} STORPORT_ETW_EVENT_OPCODE, *PSTORPORT_ETW_EVENT_OPCODE;

typedef enum _STORPORT_ETW_EVENT_CHANNEL
{
    StorportEtwEventDiagnostic = 0,
    StorportEtwEventOperational = 1,
    StorportEtwEventHealth = 2,
    StorportEtwEventIoPerformance = 3
} STORPORT_ETW_EVENT_CHANNEL, *PSTORPORT_ETW_EVENT_CHANNEL;

#define STORPORT_ETW_MAX_DESCRIPTION_LENGTH 32
#define STORPORT_ETW_MAX_PARAM_NAME_LENGTH  16

/* Prefix shared by every adapter and unit control power parameter block */
typedef struct _STOR_POWER_CONTROL_HEADER
{
    ULONG Version;
    ULONG Size;
    PSTOR_ADDRESS Address;
} STOR_POWER_CONTROL_HEADER, *PSTOR_POWER_CONTROL_HEADER;

typedef struct _STOR_ADAPTER_CONTROL_POWER
{
    STOR_POWER_CONTROL_HEADER Header;
    STOR_POWER_ACTION PowerAction;
    STOR_DEVICE_POWER_STATE PowerState;
} STOR_ADAPTER_CONTROL_POWER, *PSTOR_ADAPTER_CONTROL_POWER;

typedef struct _STOR_UNIT_CONTROL_POWER
{
    PSTOR_ADDRESS Address;
    STOR_POWER_ACTION PowerAction;
    STOR_DEVICE_POWER_STATE PowerState;
} STOR_UNIT_CONTROL_POWER, *PSTOR_UNIT_CONTROL_POWER;

/* ScsiUnitPoFxPowerInfo */
typedef struct _STOR_POFX_UNIT_POWER_INFO
{
    STOR_POWER_CONTROL_HEADER Header;
    BOOLEAN IdlePowerEnabled;
} STOR_POFX_UNIT_POWER_INFO, *PSTOR_POFX_UNIT_POWER_INFO;

/* Scsi(Adapter|Unit)PoFxPowerRequired */
typedef struct _STOR_POFX_POWER_REQUIRED_CONTEXT
{
    STOR_POWER_CONTROL_HEADER Header;
    BOOLEAN PowerRequired;
} STOR_POFX_POWER_REQUIRED_CONTEXT, *PSTOR_POFX_POWER_REQUIRED_CONTEXT;

/* Scsi(Adapter|Unit)PoFxPowerActive */
typedef struct _STOR_POFX_ACTIVE_CONTEXT
{
    STOR_POWER_CONTROL_HEADER Header;
    ULONG ComponentIndex;
    BOOLEAN Active;
} STOR_POFX_ACTIVE_CONTEXT, *PSTOR_POFX_ACTIVE_CONTEXT;

/* Scsi(Adapter|Unit)PoFxPowerSetFState */
typedef struct _STOR_POFX_FSTATE_CONTEXT
{
    STOR_POWER_CONTROL_HEADER Header;
    ULONG ComponentIndex;
    ULONG FState;
} STOR_POFX_FSTATE_CONTEXT, *PSTOR_POFX_FSTATE_CONTEXT;

/* Scsi(Adapter|Unit)PoFxPowerControl */
typedef struct _STOR_POFX_POWER_CONTROL
{
    STOR_POWER_CONTROL_HEADER Header;
    LPCGUID PowerControlCode;
    SIZE_T InBufferSize;
    SIZE_T OutBufferSize;
    PVOID InBuffer;
    PVOID OutBuffer;
    PSIZE_T BytesReturned;
} STOR_POFX_POWER_CONTROL, *PSTOR_POFX_POWER_CONTROL;

/* ScsiQuerySupportedControlTypes and ScsiQuerySupportedUnitControlTypes */
typedef struct _SCSI_SUPPORTED_CONTROL_TYPE_LIST
{
    ULONG MaxControlType;
    /*
     * One entry per control type, sized by ScsiAdapterControlMax or
     * ScsiUnitControlMax. The miniport must not write past MaxControlType.
     */
    BOOLEAN SupportedTypeList[0];
} SCSI_SUPPORTED_CONTROL_TYPE_LIST, *PSCSI_SUPPORTED_CONTROL_TYPE_LIST;

/* ScsiPowerSettingNotification */
typedef struct _STOR_POWER_SETTING_INFO
{
    GUID PowerSettingGuid;
    _Field_size_bytes_(ValueLength) PVOID Value;
    ULONG ValueLength;
} STOR_POWER_SETTING_INFO, *PSTOR_POWER_SETTING_INFO;

typedef enum _RAID_SYSTEM_POWER
{
    RaidSystemPowerUnknown = 0,
    RaidSystemPowerLowest,
    RaidSystemPowerLow,
    RaidSystemPowerMedium,
    RaidSystemPowerHigh
} RAID_SYSTEM_POWER, *PRAID_SYSTEM_POWER;

#define STOR_SYSTEM_POWER_HINTS_V1          0x1

/* ScsiAdapterSystemPowerHints, only sent on always-on always-connected systems */
typedef struct _STOR_SYSTEM_POWER_HINTS
{
    ULONG Version;
    ULONG Size;
    RAID_SYSTEM_POWER SystemPower;
    /* Milliseconds of resume latency this hint tolerates */
    ULONG ResumeLatencyMSec;
} STOR_SYSTEM_POWER_HINTS, *PSTOR_SYSTEM_POWER_HINTS;

#define STOR_FILTER_RESOURCE_REQUIREMENTS_V1 0x1

/*
 * ScsiAdapterFilterResourceRequirements. A miniport may drop or narrow a
 * requirement but never add one; to drop one it moves the rest of the buffer
 * up and lowers the descriptor count.
 */
typedef struct _STOR_FILTER_RESOURCE_REQUIREMENTS
{
    ULONG Version;
    ULONG Size;
    PIO_RESOURCE_REQUIREMENTS_LIST IoResourceRequirementsList;
} STOR_FILTER_RESOURCE_REQUIREMENTS, *PSTOR_FILTER_RESOURCE_REQUIREMENTS;

/* ScsiAdapterPoFxMaxOperationalPower */
typedef struct _STOR_MAX_OPERATIONAL_POWER
{
    ULONG Version;
    ULONG Size;
    ULONGLONG Value;
} STOR_MAX_OPERATIONAL_POWER, *PSTOR_MAX_OPERATIONAL_POWER;

/*
 * ScsiAdapterPoFxSetPerfState reports the outcome of a transition the miniport
 * asked for with StorPortPoFxSetPerfState. Context is whatever it passed there.
 */
typedef struct _STOR_POFX_PERF_STATE_CONTEXT
{
    ULONG Version;
    ULONG Size;
    ULONG ComponentIndex;
    BOOLEAN Succeeded;
    PVOID Context;
} STOR_POFX_PERF_STATE_CONTEXT, *PSTOR_POFX_PERF_STATE_CONTEXT;

#define STOR_SERIAL_NUMBER_MAX_SIZE         (128 * sizeof(WCHAR))

/* ScsiAdapterSerialNumber. At most 128 characters including the terminator. */
typedef struct _STOR_SERIAL_NUMBER
{
    ULONG Version;
    ULONG Size;
    BOOLEAN Unicode;
    UCHAR SerialNumber[STOR_SERIAL_NUMBER_MAX_SIZE];
} STOR_SERIAL_NUMBER, *PSTOR_SERIAL_NUMBER;

/* Matches DEVICE_USAGE_NOTIFICATION_TYPE */
typedef enum _SCSI_UC_DEVICE_USAGE_TYPE
{
    ScsiDeviceUsageTypeUndefined,
    ScsiDeviceUsageTypePaging,
    ScsiDeviceUsageTypeHibernation,
    ScsiDeviceUsageTypeDumpFile,
    ScsiDeviceUsageTypeBoot,
    ScsiDeviceUsageTypeInlineCryptoEngine = 7
} SCSI_UC_DEVICE_USAGE_TYPE;

/* ScsiUnitUsage */
typedef struct _STOR_UC_DEVICE_USAGE
{
    PSTOR_ADDRESS Address;
    SCSI_UC_DEVICE_USAGE_TYPE UsageType;
    BOOLEAN InUse;
} STOR_UC_DEVICE_USAGE, *PSTOR_UC_DEVICE_USAGE;

#define STOR_VENDOR_ID_LENGTH               16
#define STOR_MODEL_NUMBER_LENGTH            64
#define STOR_FIRMWARE_REVISION_LENGTH       16

#define STOR_RICH_DEVICE_DESCRIPTION_STRUCTURE_VERSION 0x1

/* ScsiUnitRichDescription, for devices whose strings outgrow the SCSI fields */
typedef struct _STOR_RICH_DEVICE_DESCRIPTION
{
    ULONG Version;
    ULONG Size;
    CHAR VendorId[STOR_VENDOR_ID_LENGTH + 1];
    CHAR ModelNumber[STOR_MODEL_NUMBER_LENGTH + 1];
    CHAR FirmwareRevision[STOR_FIRMWARE_REVISION_LENGTH + 1];
} STOR_RICH_DEVICE_DESCRIPTION, *PSTOR_RICH_DEVICE_DESCRIPTION;

#define STOR_RICH_DEVICE_DESCRIPTION_STRUCTURE_VERSION_V2 0x2

typedef struct _STOR_RICH_DEVICE_DESCRIPTION_V2
{
    ULONG Version;
    ULONG Size;
    CHAR VendorId[STOR_VENDOR_ID_LENGTH + 1];
    CHAR ModelNumber[STOR_MODEL_NUMBER_LENGTH + 1];
    CHAR FirmwareRevision[STOR_FIRMWARE_REVISION_LENGTH + 1];
    PSTOR_ADDRESS Address;
} STOR_RICH_DEVICE_DESCRIPTION_V2, *PSTOR_RICH_DEVICE_DESCRIPTION_V2;

/* ScsiUnitQueryBusType */
typedef struct _STOR_UNIT_CONTROL_QUERY_BUS_TYPE
{
    PSTOR_ADDRESS Address;
    ULONG BusType;
} STOR_UNIT_CONTROL_QUERY_BUS_TYPE, *PSTOR_UNIT_CONTROL_QUERY_BUS_TYPE;

#define STOR_FRU_ID_MAX_LENGTH              128
#define STOR_FRU_ID_DESCRIPTION_STRUCTURE_VERSION_1 0x1

/* Scsi(Adapter|Unit)QueryFruId */
typedef struct _STOR_FRU_ID_DESCRIPTION
{
    ULONG Version;
    ULONG Size;
    PSTOR_ADDRESS Address;
    UCHAR FruId[STOR_FRU_ID_MAX_LENGTH + 1];
} STOR_FRU_ID_DESCRIPTION, *PSTOR_FRU_ID_DESCRIPTION;

typedef enum _INTERNAL_DATA_SCOPE
{
    InternalDataScopeUndefined = 0,
    InternalDataScopeInMemoryState,
    InternalDataScopeMax
} INTERNAL_DATA_SCOPE, *PINTERNAL_DATA_SCOPE;

/*
 * Scsi(Adapter|Unit)ReportInternalData. CallbackContext is passed straight
 * back to StorPortMiniportReportInternalData.
 */
typedef struct _STOR_REPORT_INTERNAL_DATA
{
    ULONG Version;
    ULONG Size;
    PSTOR_ADDRESS Address;
    INTERNAL_DATA_SCOPE Scope;
    PVOID CallbackContext;
} STOR_REPORT_INTERNAL_DATA, *PSTOR_REPORT_INTERNAL_DATA;

/*
 * ScsiAdapterResetBusSynchronous. Delivered at PASSIVE_LEVEL so the miniport
 * can reset the bus and wait for it.
 */
typedef struct _STOR_RESET_BUS_SYNCHRONOUS_PARAMETER
{
    ULONG Version;
    ULONG Size;
    ULONG PathId;
    ULONG Reserved;
} STOR_RESET_BUS_SYNCHRONOUS_PARAMETER, *PSTOR_RESET_BUS_SYNCHRONOUS_PARAMETER;

typedef enum _STOR_CRYPTO_OPERATION_TYPE
{
    StorCryptoOperationInsertKey = 1,
    StorCryptoOperationMax
} STOR_CRYPTO_OPERATION_TYPE, *PSTOR_CRYPTO_OPERATION_TYPE;

/* Scsi(Adapter|Unit)CryptoOperation */
typedef struct _STOR_CRYPTO_OPERATION
{
    STOR_CRYPTO_OPERATION_TYPE OperationType;
    ULONG OperationBufferLength;
    PVOID OperationBuffer;
} STOR_CRYPTO_OPERATION, *PSTOR_CRYPTO_OPERATION;

#define STOR_CRYPTO_OPERATION_INSERT_KEY_VERSION_1 1

typedef struct _STOR_CRYPTO_OPERATION_INSERT_KEY
{
    USHORT Version;
    USHORT Size;
    ULONG KeyIndex;
    ULONG CryptoCapabilityIndex;
    ULONG DataUnitSizeBitmask;
    ULONG KeySize;
    ULONG Reserved;
    PVOID KeyVirtualAddress;
    PHYSICAL_ADDRESS KeyPhysicalAddress;
} STOR_CRYPTO_OPERATION_INSERT_KEY, *PSTOR_CRYPTO_OPERATION_INSERT_KEY;

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)

/* NvmeQuerySupportedControlTypes and NvmeQuerySupportedNamespaceControlTypes */
typedef struct _NVME_SUPPORTED_CONTROL_TYPE_LIST
{
    ULONG MaxControlType;
    BOOLEAN SupportedTypeList[0];
} NVME_SUPPORTED_CONTROL_TYPE_LIST, *PNVME_SUPPORTED_CONTROL_TYPE_LIST;

typedef struct _NVME_ICE_ENTRY *PNVME_ICE_ENTRY;

/*
 * ScsiUnitNvmeIceInformation, asked during enumeration to find out whether a
 * unit can take part in NVMe inline crypto.
 */
typedef struct _STOR_UNIT_NVME_ICE_INFORMATION
{
    ULONG Version;
    PSTOR_ADDRESS Address;
    PNVME_ICE_ENTRY NvmeIceEntry;
} STOR_UNIT_NVME_ICE_INFORMATION, *PSTOR_UNIT_NVME_ICE_INFORMATION;

#endif /* (NTDDI_VERSION >= NTDDI_WIN11_GE) */

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef enum _STOR_IMPORTANCE
{
    StorLowImportance,
    StorMediumImportance,
    StorHighImportance,
    StorMediumHighImportance
} STOR_IMPORTANCE;

/* StorPortSetExtendedDpcParameters */
typedef struct _STOR_EXT_SET_PARAMETERS
{
    ULONG Version;
    ULONG Reserved;
    LONGLONG NoWakeTolerance;
} STOR_EXT_SET_PARAMETERS, *PSTOR_EXT_SET_PARAMETERS;

typedef
VOID
(NTAPI *PSTOR_EXT_DELETE_CALLBACK)(
    _In_opt_ PVOID Context);

typedef struct _STOR_EXT_DELETE_PARAMETERS
{
    ULONG Version;
    ULONG Reserved;
    PSTOR_EXT_DELETE_CALLBACK DeleteCallback;
    PVOID DeleteContext;
} STOR_EXT_DELETE_PARAMETERS, *PSTOR_EXT_DELETE_PARAMETERS;

typedef struct _STARTIO_PERFORMANCE_PARAMETERS_V2
{
    ULONG Version;
    ULONG Size;
    ULONG MessageNumber;
    ULONG ChannelNumber;
    PROCESSOR_NUMBER InitiatingProcessor;
} STARTIO_PERFORMANCE_PARAMETERS_V2, *PSTARTIO_PERFORMANCE_PARAMETERS_V2;

/*
 * Storport owns these list primitives because a miniport in dump mode cannot
 * call the kernel ones. The layout follows SLIST_ENTRY and SLIST_HEADER.
 */
#if defined(_WIN64)

typedef struct DECLSPEC_ALIGN(16) _STOR_SLIST_ENTRY *PSTOR_SLIST_ENTRY;

typedef struct DECLSPEC_ALIGN(16) _STOR_SLIST_ENTRY
{
    PSTOR_SLIST_ENTRY Next;
} STOR_SLIST_ENTRY;

typedef union DECLSPEC_ALIGN(16) _STOR_SLIST_HEADER
{
    struct
    {
        ULONGLONG Alignment;
        ULONGLONG Region;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:9;
        ULONGLONG NextEntry:39;
        /* 0 for the 8 byte header, 1 for the 16 byte one */
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:59;
        ULONGLONG Region:3;
    } Header8;
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:48;
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:2;
        /* The low four bits are always clear */
        ULONGLONG NextEntry:60;
    } Header16;
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:48;
        ULONGLONG HeaderType:1;
        ULONGLONG Reserved:3;
        ULONGLONG NextEntry:60;
    } HeaderX64;
} STOR_SLIST_HEADER, *PSTOR_SLIST_HEADER;

#else

typedef struct _STOR_SLIST_ENTRY
{
    struct _STOR_SLIST_ENTRY *Next;
} STOR_SLIST_ENTRY, *PSTOR_SLIST_ENTRY;

typedef union _STOR_SLIST_HEADER
{
    ULONGLONG Alignment;
    struct
    {
        STOR_SLIST_ENTRY Next;
        USHORT Depth;
        USHORT Sequence;
    } DUMMYSTRUCTNAME;
} STOR_SLIST_HEADER, *PSTOR_SLIST_HEADER;

#endif /* defined(_WIN64) */

typedef struct _STOR_LIST_ENTRY
{
    struct _STOR_LIST_ENTRY *Flink;
    struct _STOR_LIST_ENTRY *Blink;
} STOR_LIST_ENTRY, *PSTOR_LIST_ENTRY;

typedef ULONG_PTR STOR_KSPIN_LOCK, *PSTOR_KSPIN_LOCK;

typedef ULONG_PTR STOR_AFFINITY;

typedef struct _STOR_GROUP_AFFINITY
{
    STOR_AFFINITY Mask;
    USHORT Group;
    USHORT Reserved[3];
} STOR_GROUP_AFFINITY, *PSTOR_GROUP_AFFINITY;

/* StorPortMarkDumpMemory */
#define MARK_DUMP_MEMORY_FLAG_PHYSICAL_ADDRESS 0x1

typedef struct _STOR_DISPATCHER_HEADER
{
    union
    {
        struct
        {
            UCHAR Type;
            UCHAR Flags;
            UCHAR Size;
            union
            {
                UCHAR Inserted;
                BOOLEAN DebugActive;
            };
        } Data;
        volatile LONG Lock;
    };
    LONG SignalState;
    STOR_LIST_ENTRY WaitListHead;
} STOR_DISPATCHER_HEADER, *PSTOR_DISPATCHER_HEADER;

typedef struct _STOR_EVENT
{
    STOR_DISPATCHER_HEADER Header;
} STOR_EVENT, *PSTOR_EVENT, *PRSTOR_EVENT;

typedef enum _STOR_EVENT_TYPE
{
    StorNotificationEvent = 0,
    StorSynchronizationEvent = 1
} STOR_EVENT_TYPE, *PSTOR_EVENT_TYPE;

typedef enum _STOR_THREAD_PRIORITY
{
    StorThreadPriorityBackground = 7,
    StorThreadPriorityNormal = 8,
    StorThreadPriorityDelayed = 12,
    StorThreadPriorityCritical = 13,
    StorThreadPrioritySuperCritical = 14,
    StorThreadPriorityHyperCritical = 15,
    StorThreadPriorityRealTime = 18
} STOR_THREAD_PRIORITY, *PSTOR_THREAD_PRIORITY;

typedef enum _STOR_DEVICE_RESET_TYPE
{
    StorFunctionLevelReset,
    StorPlatformLevelReset,
    StorBusSpecificReset
} STOR_DEVICE_RESET_TYPE;

/* All times are in timer ticks */
typedef struct _STOR_DPC_WATCHDOG_INFORMATION
{
    ULONG DpcTimeLimit;
    ULONG DpcTimeCount;
    ULONG DpcWatchdogLimit;
    ULONG DpcWatchdogCount;
    ULONG Reserved;
} STOR_DPC_WATCHDOG_INFORMATION, *PSTOR_DPC_WATCHDOG_INFORMATION;

typedef enum _STORPORT_QUERY_CONFIGURATION_TYPE
{
    StorportQueryConfigurationD3 = 0,
    StorportQueryConfigurationNvmeIce,
    StorportQueryConfigurationMax
} STORPORT_QUERY_CONFIGURATION_TYPE, *PSTORPORT_QUERY_CONFIGURATION_TYPE;

/*
 * StorPortSetFeatureList. A miniport has to claim every adapter and unit
 * control code added after Windows 8 before Storport will send it.
 */
typedef enum _STORPORT_FEATURE_TYPE
{
    StorportFeatureBusTypeUnitControl = 0,
    StorportFeatureFruIdUnitControl,
    StorportFeatureFruIdAdapterControl,
    StorportFeatureSetEventLoggingAdapterControl,
    StorportFeatureReportInternalDataUnitControl,
    StorportFeatureReportInternalDataAdapterControl,
    StorportFeatureResetBusSynchronous,
    StorportFeaturePostHwInitialize,
    StorportFeaturePrepareEarlyDumpData,
    StorportFeatureRestoreEarlyDumpData,
    StorportFeatureKsrAdapterPowerDownOptimization,
    StorportFeatureKsrUnitPowerDownOptimization,
    StorportFeaturePreparePLDR,
    StorportFeatureNvmeofAdapterOperation,
    StorportFeatureReserved1,
    StorportFeatureQueryStorMQInterface,
    StorportFeatureMax
} STORPORT_FEATURE_TYPE;

/* ScsiAdapterSetEventLogging */
typedef struct _STOR_SET_EVENT_LOGGING
{
    STORPORT_ETW_EVENT_CHANNEL Channel;
    BOOLEAN Enabled;
} STOR_SET_EVENT_LOGGING, *PSTOR_SET_EVENT_LOGGING;

typedef enum _STOR_TELEMETRY_CATEGORY
{
    StorTelemetryCategory,
    StorMeasuresCategory
} STOR_TELEMETRY_CATEGORY, *PSTOR_TELEMETRY_CATEGORY;

#define EVENT_BUFFER_MAX_LENGTH             4096
#define EVENT_NAME_MAX_LENGTH               32
#define EVENT_MAX_PARAM_NAME_LEN            32

typedef struct _STORPORT_TELEMETRY_EVENT
{
    ULONG DriverVersion;
    ULONG EventId;
    UCHAR EventName[EVENT_NAME_MAX_LENGTH];
    ULONG EventVersion;
    ULONG Flags;
    _Field_range_(0, EVENT_BUFFER_MAX_LENGTH)
    ULONG EventBufferLength;
    _Field_size_bytes_(EventBufferLength)
    PUCHAR EventBuffer;
    UCHAR ParameterName0[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue0;
    UCHAR ParameterName1[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue1;
    UCHAR ParameterName2[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue2;
    UCHAR ParameterName3[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue3;
    UCHAR ParameterName4[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue4;
    UCHAR ParameterName5[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue5;
    UCHAR ParameterName6[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue6;
    UCHAR ParameterName7[EVENT_MAX_PARAM_NAME_LEN];
    ULONGLONG ParameterValue7;
} STORPORT_TELEMETRY_EVENT, *PSTORPORT_TELEMETRY_EVENT;

/* StorPortMiniportReportInternalData payload description */
typedef enum _DATA_TYPE
{
    DataTypeGuid,
    DataTypeBoolean,
    DataTypeUnicodeString,
    DataTypeAnsiString,
    DataTypeInt8,
    DataTypeInt16,
    DataTypeInt32,
    DataTypeInt64,
    DataTypeUInt8,
    DataTypeUInt16,
    DataTypeUInt32,
    DataTypeUInt64,
    DataTypeHex8,
    DataTypeHex16,
    DataTypeHex32,
    DataTypeHex64,
    DataTypeByteArray
} DATA_TYPE;

typedef struct _MINIPORT_DATA
{
    PSTR Name;
    DATA_TYPE Type;
    /* Only meaningful for DataTypeByteArray */
    USHORT ArrayLength;
    PVOID Data;
} MINIPORT_DATA, *PMINIPORT_DATA;

typedef enum _STORPORT_LIVEDUMP_ISSUE_TYPE
{
    StorportLivedumpIssueTypeUndefined = 0,
    StorportLivedumpIssueTypeDevicePanic
} STORPORT_LIVEDUMP_ISSUE_TYPE, *PSTORPORT_LIVEDUMP_ISSUE_TYPE;

#define STORPORT_LIVEDUMP_BUCKET_ID_LENGTH  128

typedef enum _STORPORT_CAPTURE_LIVEDUMP_TYPE
{
    StorportCaptureLiveDumpTypeUndefined = 0,
    StorportCaptureLiveDumpTypeMini,
    StorportCaptureLiveDumpTypeFull
} STORPORT_CAPTURE_LIVEDUMP_TYPE, *PSTORPORT_CAPTURE_LIVEDUMP_TYPE;

typedef enum _STORPORT_LIVEDUMP_DATA_TYPE
{
    StorportLivedumpDataTypeUndefined = 0,
    StorportLivedumpDataTypeDeviceTelemetryLog
} STORPORT_LIVEDUMP_DATA_TYPE, *PSTORPORT_LIVEDUMP_DATA_TYPE;

typedef struct _STORPORT_LIVEDUMP_DEVICE_TELEMETRY_LOG
{
    ULONG Version;
    ULONG Size;
    /* Identifies the device state at the time of the issue */
    UCHAR BucketId[STORPORT_LIVEDUMP_BUCKET_ID_LENGTH];
    UCHAR OrganizationID[3];
    UCHAR Reserved;
    ULONG TelemetryLength;
    PVOID DeviceTelemetry;
} STORPORT_LIVEDUMP_DEVICE_TELEMETRY_LOG, *PSTORPORT_LIVEDUMP_DEVICE_TELEMETRY_LOG;

typedef struct _STORPORT_CAPTURE_LIVEDUMP
{
    ULONG Version;
    ULONG Size;
    STORPORT_CAPTURE_LIVEDUMP_TYPE LiveDumpType;
    STORPORT_LIVEDUMP_ISSUE_TYPE IssueType;
    PWSTR ComponentName;
    STORPORT_LIVEDUMP_DATA_TYPE DataType;
    PVOID Data;
} STORPORT_CAPTURE_LIVEDUMP, *PSTORPORT_CAPTURE_LIVEDUMP;

#define STOR_RPMB_VERSION_1                 0x00000001

typedef enum _STOR_RPMB_FRAME_TYPE
{
    StorRpmbFrameTypeUnknown = 0,
    StorRpmbFrameTypeStandard,
    StorRpmbFrameTypeMax
} STOR_RPMB_FRAME_TYPE, *PSTOR_RPMB_FRAME_TYPE;

typedef struct _STOR_RPMB_CAPABILITIES_DATA
{
    ULONG Version;
    ULONG Size;
    ULONG RpmbSize;
    ULONG MaxReliableRpmbWriteSize;
    UCHAR SecurityProtocol;
    USHORT SecurityProtocolSpecifier;
    STOR_ADDRESS RpmbTarget;
    STOR_RPMB_FRAME_TYPE FrameFormat;
} STOR_RPMB_CAPABILITIES_DATA, *PSTOR_RPMB_CAPABILITIES_DATA;

/* ASCII "MPDI" */
#define MINIPORT_DUMP_INFO_SIGNATURE        0x4D504449

/* Returned by SRB_FUNCTION_GET_DUMP_INFO */
typedef struct _MINIPORT_DUMP_INFO
{
    ULONG Version;
    ULONG Size;
    _Field_range_(MINIPORT_DUMP_INFO_SIGNATURE, MINIPORT_DUMP_INFO_SIGNATURE)
    ULONG Signature;
    UCHAR Reserved0[4];
    PVOID Context;
} MINIPORT_DUMP_INFO, *PMINIPORT_DUMP_INFO;

/* ASCII "GMDE" */
#define GET_DUMP_INFO_SIGNATURE_EXT         0x474D4445
#define GET_DUMP_INFO_VERSION_1             sizeof(GET_MINIPORT_DUMP_INFO_V1)

/* Input buffer of SRB_FUNCTION_GET_DUMP_INFO */
typedef struct _GET_MINIPORT_DUMP_INFO_V1
{
    _Field_range_(GET_DUMP_INFO_VERSION_1, GET_DUMP_INFO_VERSION_1)
    ULONG Version;
    ULONG Size;
    _Field_range_(GET_DUMP_INFO_SIGNATURE_EXT, GET_DUMP_INFO_SIGNATURE_EXT)
    ULONG Signature;
    USHORT DiskCount;
    /* A STOR_ADDRESS_TYPE_* value, never STOR_ADDRESS_TYPE_UNKNOWN */
    USHORT AddressType;
    ULONG AddressSize;
    UCHAR Reserved0[4];
    _Field_size_bytes_(DiskCount * AddressSize)
    UCHAR Addresses[ANYSIZE_ARRAY];
} GET_MINIPORT_DUMP_INFO_V1, *PGET_MINIPORT_DUMP_INFO_V1;

/* ASCII "FMDI" */
#define FREE_DUMP_INFO_SIGNATURE            0x464D4449

/* Input buffer of SRB_FUNCTION_FREE_DUMP_INFO */
typedef struct _FREE_MINIPORT_DUMP_INFO
{
    ULONG Version;
    ULONG Size;
    _Field_range_(FREE_DUMP_INFO_SIGNATURE, FREE_DUMP_INFO_SIGNATURE)
    ULONG Signature;
    UCHAR Reserved0[4];
    PVOID Context;
} FREE_MINIPORT_DUMP_INFO, *PFREE_MINIPORT_DUMP_INFO;

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#define STOR_POFX_UNKNOWN_POWER             0xFFFFFFFF
#define STOR_POFX_UNKNOWN_TIME              0xFFFFFFFFFFFFFFFF

/* One F-state of a component. Times are in 100ns units. */
typedef struct _STOR_POFX_COMPONENT_IDLE_STATE
{
    ULONG Version;
    ULONG Size;
    ULONGLONG TransitionLatency;
    ULONGLONG ResidencyRequirement;
    ULONG NominalPower;
} STOR_POFX_COMPONENT_IDLE_STATE, *PSTOR_POFX_COMPONENT_IDLE_STATE;

#define STOR_POFX_COMPONENT_IDLE_STATE_SIZE (sizeof(STOR_POFX_COMPONENT_IDLE_STATE))
#define STOR_POFX_COMPONENT_IDLE_STATE_VERSION_V1 1

typedef struct _STOR_POFX_COMPONENT
{
    ULONG Version;
    ULONG Size;
    ULONG FStateCount;
    ULONG DeepestWakeableFState;
    GUID Id;
    _Field_size_full_(FStateCount) STOR_POFX_COMPONENT_IDLE_STATE FStates[ANYSIZE_ARRAY];
} STOR_POFX_COMPONENT, *PSTOR_POFX_COMPONENT;

#define STOR_POFX_COMPONENT_SIZE            ((ULONG)FIELD_OFFSET(STOR_POFX_COMPONENT, FStates))
#define STOR_POFX_COMPONENT_VERSION_V1      1

/*
 * DeepestAdapterPowerRequiredFState only applies to components identified by
 * STORPORT_POFX_LUN_GUID.
 */
typedef struct _STOR_POFX_COMPONENT_V2
{
    ULONG Version;
    ULONG Size;
    ULONG FStateCount;
    ULONG DeepestWakeableFState;
    GUID Id;
    ULONG DeepestAdapterPowerRequiredFState;
    ULONG DeepestCrashDumpReadyFState;
    _Field_size_full_(FStateCount) STOR_POFX_COMPONENT_IDLE_STATE FStates[ANYSIZE_ARRAY];
} STOR_POFX_COMPONENT_V2, *PSTOR_POFX_COMPONENT_V2;

#define STOR_POFX_COMPONENT_VERSION_V2      2

#define STOR_POFX_UNIT_MIN_IDLE_STATES      1
#define STOR_POFX_UNIT_MAX_IDLE_STATES      2
#define STOR_POFX_ADAPTER_MIN_IDLE_STATES   1
#define STOR_POFX_ADAPTER_MAX_IDLE_STATES   8

typedef struct _STOR_POFX_DEVICE
{
    ULONG Version;
    ULONG Size;
    ULONG ComponentCount;
    ULONG Flags;
    _Field_size_full_(ComponentCount) STOR_POFX_COMPONENT Components[ANYSIZE_ARRAY];
} STOR_POFX_DEVICE, *PSTOR_POFX_DEVICE;

#define STOR_POFX_DEVICE_SIZE               ((ULONG)FIELD_OFFSET(STOR_POFX_DEVICE, Components))
#define STOR_POFX_DEVICE_VERSION_V1         1

/*
 * The idle timeout members are only read when
 * STOR_POFX_DEVICE_FLAG_IDLE_TIMEOUT is set.
 */
typedef struct _STOR_POFX_DEVICE_V2
{
    ULONG Version;
    ULONG Size;
    ULONG ComponentCount;
    ULONG Flags;
    union
    {
        ULONG UnitMinIdleTimeoutInMS;
        ULONG AdapterIdleTimeoutInMS;
    };
    _Field_size_full_(ComponentCount) STOR_POFX_COMPONENT Components[ANYSIZE_ARRAY];
} STOR_POFX_DEVICE_V2, *PSTOR_POFX_DEVICE_V2;

#define STOR_POFX_DEVICE_VERSION_V2         2

typedef enum _STOR_POFX_PERF_STATE_UNIT
{
    StorPoFxPerfStateUnitOther,
    StorPoFxPerfStateUnitFrequency,
    StorPoFxPerfStateUnitBandwidth,
    StorPoFxPerfStateUnitMilliwatts,
    StorPoFxPerfStateUnitMaximum
} STOR_POFX_PERF_STATE_UNIT, *PSTOR_POFX_PERF_STATE_UNIT;

typedef enum _STOR_POFX_PERF_STATE_TYPE
{
    StorPoFxPerfStateTypeDiscrete,
    StorPoFxPerfStateTypeRange,
    StorPoFxPerfStateTypeMaximum
} STOR_POFX_PERF_STATE_TYPE, *PSTOR_POFX_PERF_STATE_TYPE;

/* One discrete P-state. Context carries whatever Value cannot express. */
typedef struct _STOR_POFX_PERF_STATE
{
    ULONG Version;
    ULONG Size;
    ULONGLONG Value;
    PVOID Context;
} STOR_POFX_PERF_STATE, *PSTOR_POFX_PERF_STATE;

#define STOR_POFX_PERF_STATE_SIZE           (sizeof(STOR_POFX_PERF_STATE))
#define STOR_POFX_PERF_STATE_VERSION_V1     1

typedef struct _STOR_POFX_COMPONENT_PERF_SET
{
    ULONG Version;
    ULONG Size;
    ULONGLONG Flags;
    STOR_POFX_PERF_STATE_UNIT PStateUnit;
    STOR_POFX_PERF_STATE_TYPE PStateType;
    union
    {
        struct
        {
            ULONG Count;
            /* Byte offset from this structure to the STOR_POFX_PERF_STATE array */
            ULONG Offset;
        } Discrete;
        struct
        {
            ULONGLONG Minimum;
            ULONGLONG Maximum;
        } Range;
    } PStates;
} STOR_POFX_COMPONENT_PERF_SET, *PSTOR_POFX_COMPONENT_PERF_SET;

#define STOR_POFX_COMPONENT_PERF_SET_SIZE   (sizeof(STOR_POFX_COMPONENT_PERF_SET))
#define STOR_POFX_COMPONENT_PERF_SET_VERSION_V1 1

/* At most one perf set per non-other unit type */
#define STOR_POFX_COMPONENT_MIN_PERF_SETS   0
#define STOR_POFX_COMPONENT_MAX_PERF_SETS   3
#define STOR_POFX_MIN_DISCRETE_PERF_STATES  1
#define STOR_POFX_MAX_DISCRETE_PERF_STATES  8

/*
 * MinimumPowerCyclePeriodInMS is only read for units and only when
 * STOR_POFX_DEVICE_FLAG_ADAPTIVE_D3_IDLE_TIMEOUT is set.
 */
typedef struct _STOR_POFX_DEVICE_V3
{
    ULONG Version;
    ULONG Size;
    ULONG ComponentCount;
    ULONG Flags;
    union
    {
        ULONG UnitMinIdleTimeoutInMS;
        ULONG AdapterIdleTimeoutInMS;
    };
    ULONG MinimumPowerCyclePeriodInMS;
    _Field_size_full_(ComponentCount) STOR_POFX_COMPONENT Components[ANYSIZE_ARRAY];
} STOR_POFX_DEVICE_V3, *PSTOR_POFX_DEVICE_V3;

#define STOR_POFX_DEVICE_V3_SIZE            ((ULONG)FIELD_OFFSET(STOR_POFX_DEVICE_V3, Components))
#define STOR_POFX_DEVICE_VERSION_V3         3

/* A unit and an adapter each register exactly one component */
#define STOR_POFX_UNIT_MIN_COMPONENTS       1
#define STOR_POFX_UNIT_MAX_COMPONENTS       1
#define STOR_POFX_ADAPTER_MIN_COMPONENTS    1
#define STOR_POFX_ADAPTER_MAX_COMPONENTS    1

#define STOR_POFX_DEVICE_FLAG_GET_PERF_STATE_FROM_PEP 0x1000

#ifndef STORAGE_CRYPTO_ALGORITHMS_DEFINED
#define STORAGE_CRYPTO_ALGORITHMS_DEFINED

typedef enum _STORAGE_CRYPTO_ALGORITHM_ID
{
    StorageCryptoAlgorithmUnknown = 0,
    StorageCryptoAlgorithmXTSAES = 1,
    StorageCryptoAlgorithmBitlockerAESCBC,
    StorageCryptoAlgorithmAESECB,
    StorageCryptoAlgorithmESSIVAESCBC,
    StorageCryptoAlgorithmMax,
    /* Kept for drivers written against the original names */
    StorCryptoAlgorithmUnknown = StorageCryptoAlgorithmUnknown,
    StorCryptoAlgorithmXTSAES = StorageCryptoAlgorithmXTSAES,
    StorCryptoAlgorithmBitlockerAESCBC = StorageCryptoAlgorithmBitlockerAESCBC,
    StorCryptoAlgorithmAESECB = StorageCryptoAlgorithmAESECB,
    StorCryptoAlgorithmESSIVAESCBC = StorageCryptoAlgorithmESSIVAESCBC
} STORAGE_CRYPTO_ALGORITHM_ID, *PSTORAGE_CRYPTO_ALGORITHM_ID;

typedef enum _STORAGE_CRYPTO_KEY_SIZE
{
    StorageCryptoKeySizeUnknown = 0,
    StorageCryptoKeySize128Bits = 1,
    StorageCryptoKeySize192Bits,
    StorageCryptoKeySize256Bits,
    StorageCryptoKeySize512Bits,
    StorageCryptoKeySizeMax,
    StorCryptoKeySizeUnknown = StorageCryptoKeySizeUnknown,
    StorCryptoKeySize128Bits = StorageCryptoKeySize128Bits,
    StorCryptoKeySize192Bits = StorageCryptoKeySize192Bits,
    StorCryptoKeySize256Bits = StorageCryptoKeySize256Bits,
    StorCryptoKeySize512Bits = StorageCryptoKeySize512Bits
} STORAGE_CRYPTO_KEY_SIZE, *PSTORAGE_CRYPTO_KEY_SIZE;

#endif /* STORAGE_CRYPTO_ALGORITHMS_DEFINED */

#ifndef STORAGE_SECURITY_COMPLIANCE_BITMASK_DEFINED
#define STORAGE_SECURITY_COMPLIANCE_BITMASK_DEFINED

typedef union _STORAGE_SECURITY_COMPLIANCE_BITMASK
{
    struct
    {
        UCHAR FIPS:1;
        UCHAR Reserved:7;
    };
    UCHAR AsUchar;
} STORAGE_SECURITY_COMPLIANCE_BITMASK;

#endif /* STORAGE_SECURITY_COMPLIANCE_BITMASK_DEFINED */

#ifndef STORAGE_CRYPTO_KEY_TYPE_DEFINED
#define STORAGE_CRYPTO_KEY_TYPE_DEFINED

typedef union _STORAGE_CRYPTO_KEY_TYPE
{
    struct
    {
        UCHAR DirectKey:1;
        UCHAR PlatformWrappedKey:1;
        UCHAR PlutonWrappedKey:1;
        UCHAR Reserved:5;
    };
    UCHAR AsUchar;
} STORAGE_CRYPTO_KEY_TYPE;

#endif /* STORAGE_CRYPTO_KEY_TYPE_DEFINED */

typedef STORAGE_CRYPTO_ALGORITHM_ID STOR_CRYPTO_ALGORITHM_ID, *PSTOR_CRYPTO_ALGORITHM_ID;
typedef STORAGE_CRYPTO_KEY_SIZE STOR_CRYPTO_KEY_SIZE, *PSTOR_CRYPTO_KEY_SIZE;

#define STOR_CRYPTO_ALGORITHM_ID_OFFSET     StorCryptoAlgorithmXTSAES

#define STOR_CRYPTO_CAPABILITY_VERSION_1    1
#define STOR_CRYPTO_CAPABILITY_VERSION_2    2

typedef struct _STOR_CRYPTO_CAPABILITY
{
    ULONG Version;
    ULONG Size;
    USHORT CryptoCapabilityIndex;
    USHORT DataUnitSizeBitmask;
    STORAGE_CRYPTO_ALGORITHM_ID AlgorithmId;
    STORAGE_CRYPTO_KEY_SIZE KeySize;
#if (NTDDI_VERSION >= NTDDI_WIN11_GA)
    /* Zero where the algorithm has no initialization vector */
    USHORT MaxIVBitSize;
    USHORT Reserved;
    STORAGE_SECURITY_COMPLIANCE_BITMASK SecurityComplianceBitmask;
#endif
} STOR_CRYPTO_CAPABILITY, *PSTOR_CRYPTO_CAPABILITY;

#define STOR_CRYPTO_CAPABILITIES_DATA_VERSION_1 1

typedef struct _STOR_CRYPTO_CAPABILITIES_DATA
{
    ULONG Version;
    ULONG Size;
    USHORT NumKeysSupported;
    USHORT NumCryptoCapabilities;
    _Field_size_(NumCryptoCapabilities) STOR_CRYPTO_CAPABILITY CryptoCapabilities[ANYSIZE_ARRAY];
} STOR_CRYPTO_CAPABILITIES_DATA, *PSTOR_CRYPTO_CAPABILITIES_DATA;

#define STOR_CRYPTO_KEY_INFO_VERSION_1      1

typedef struct _STOR_CRYPTO_KEY_INFO
{
    ULONG Version;
    ULONG Size;
    ULONG KeyIndex;
    ULONGLONG Tweak;
} STOR_CRYPTO_KEY_INFO, *PSTOR_CRYPTO_KEY_INFO;

/* Smallest tolerable delay the timer calls accept */
#define RAID_MIN_TIMER_DELAY_IN_MSEC        32

/* StorPortStateChangeDetected ChangedEntity. The highest bit set wins. */
#define STATE_CHANGE_LUN                    0x1
#define STATE_CHANGE_TARGET                 0x2
#define STATE_CHANGE_BUS                    0x4

/* StorPortStateChangeDetected Attributes, applied to newly found entities */
#define ATTRIBUTE_VM_PASSTHROUGH_LUN        0x1

/* StorPortAsyncNotificationDetected Flags */
#define RAID_ASYNC_NOTIFY_FLAG_MEDIA_STATUS     0x1
#define RAID_ASYNC_NOTIFY_FLAG_DEVICE_STATUS    0x2
#define RAID_ASYNC_NOTIFY_FLAG_DEVICE_OPERATION 0x4

#define RAID_ASYNC_NOTIFY_SUPPORTED_FLAGS   (RAID_ASYNC_NOTIFY_FLAG_MEDIA_STATUS | \
                                             RAID_ASYNC_NOTIFY_FLAG_DEVICE_STATUS | \
                                             RAID_ASYNC_NOTIFY_FLAG_DEVICE_OPERATION)

#define DUMP_MINIPORT_VERSION_1             0x0100
#define DUMP_MINIPORT_VERSION               0x0200
#define DUMP_MINIPORT_NAME_LENGTH           15

typedef struct _MINIPORT_MAPPINGS
{
    USHORT Version;
    PVOID IBFTable;
    PVOID Nic0Map;
    PVOID Nic1Map;
    ULONG NumberOfAdditionalNic;
    PVOID NicMap[];
} MINIPORT_MAPPINGS, *PMINIPORT_MAPPINGS;

/* Returned by SRB_FUNCTION_DUMP_POINTERS */
typedef struct _MINIPORT_DUMP_POINTERS
{
    USHORT Version;
    USHORT Size;
    WCHAR DriverName[DUMP_MINIPORT_NAME_LENGTH];
    struct _ADAPTER_OBJECT *AdapterObject;
    PVOID MappedRegisterBase;
    /* Must not exceed 64KB */
    ULONG CommonBufferSize;
    PVOID MiniportPrivateDumpData;
    /* The rest mirrors PORT_CONFIGURATION_INFORMATION */
    ULONG SystemIoBusNumber;
    INTERFACE_TYPE AdapterInterfaceType;
    ULONG MaximumTransferLength;
    ULONG NumberOfPhysicalBreaks;
    ULONG AlignmentMask;
    ULONG NumberOfAccessRanges;
    ACCESS_RANGE (*AccessRanges)[];
    UCHAR NumberOfBuses;
    BOOLEAN Master;
    BOOLEAN MapBuffers;
    UCHAR MaximumNumberOfTargets;
} MINIPORT_DUMP_POINTERS, *PMINIPORT_DUMP_POINTERS;

/* STOR_POFX_DEVICE.Flags */
#define STOR_POFX_DEVICE_FLAG_NO_D0                     0x01
#define STOR_POFX_DEVICE_FLAG_NO_D3                     0x02
#define STOR_POFX_DEVICE_FLAG_ENABLE_D3_COLD            0x04
#define STOR_POFX_DEVICE_FLAG_NO_DUMP_ACTIVE            0x08
#define STOR_POFX_DEVICE_FLAG_IDLE_TIMEOUT              0x10
#define STOR_POFX_DEVICE_FLAG_ADAPTIVE_D3_IDLE_TIMEOUT  0x20
#define STOR_POFX_DEVICE_FLAG_NO_UNIT_REGISTRATION      0x40
#define STOR_POFX_DEVICE_FLAG_PERF_STATE_PEP_OPTIONAL   0x80
#define STOR_POFX_DEVICE_FLAG_NO_IDLE_DEBOUNCE          0x100
#define STOR_POFX_DEVICE_FLAG_DUMP_ALWAYS_POWER_ON      0x200
#define STOR_POFX_DEVICE_FLAG_DISABLE_INTERRUPTS_ON_D3  0x400
#define STOR_POFX_DEVICE_FLAG_ADAPTER_D3_WAKE           0x800

/* A component is identified as either the adapter or one of its units */
static const GUID STORPORT_POFX_ADAPTER_GUID =
    {0xdcaf9c10, 0x895f, 0x481f, {0xa4, 0x92, 0xd4, 0xce, 0xd2, 0xf5, 0x56, 0x33}};
static const GUID STORPORT_POFX_LUN_GUID =
    {0x585d326b, 0x0b3a, 0x4088, {0x89, 0x39, 0x88, 0xb0, 0x0f, 0x69, 0x58, 0xbe}};

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
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
(NTAPI *PHW_PASSIVE_INITIALIZE_ROUTINE)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(NTAPI *PHW_DPC_ROUTINE)(
    _In_ PSTOR_DPC Dpc,
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

typedef
VOID
(NTAPI *PHW_TIMER_EX)(
    _In_ PVOID DeviceExtension,
    _In_opt_ PVOID Context);

#if (NTDDI_VERSION >= NTDDI_WIN8)

typedef
SCSI_UNIT_CONTROL_STATUS
(NTAPI *PHW_UNIT_CONTROL)(
    _In_ PVOID DeviceExtension,
    _In_ SCSI_UNIT_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters);

#if (NTDDI_VERSION >= NTDDI_WIN11_GE)

typedef
NVME_ADAPTER_CONTROL_STATUS
(NTAPI *PHW_NVME_ADAPTER_CONTROL)(
    _In_ PVOID DeviceExtension,
    _In_ NVME_ADAPTER_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters);

typedef
NVME_NAMESPACE_CONTROL_STATUS
(NTAPI *PHW_NAMESPACE_CONTROL)(
    _In_ PVOID DeviceExtension,
    _In_ NVME_NAMESPACE_CONTROL_TYPE ControlType,
    _In_ PVOID Parameters);

#endif /* (NTDDI_VERSION >= NTDDI_WIN11_GE) */

typedef
VOID
(NTAPI *PHW_WORKITEM)(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID Context,
    _In_ PVOID Worker);

typedef
VOID
(NTAPI *PHW_STATE_CHANGE)(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PVOID Context,
    _In_ SHORT AddressType,
    _In_ PVOID Address,
    _In_ ULONG Status);

typedef
VOID
(NTAPI *PHW_TRACING_ENABLED)(
    _In_ PVOID HwDeviceExtension,
    _In_ BOOLEAN Enabled);

typedef
VOID
(NTAPI *PHW_REGISTRY_NOTIFICATION_ROUTINE)(
    _In_ PVOID HwDeviceExtension);

/* Virtual miniport callbacks. A physical miniport leaves all of these NULL. */

typedef
ULONG
(NTAPI *PVIRTUAL_HW_FIND_ADAPTER)(
    _In_ PVOID DeviceExtension,
    _In_ PVOID HwContext,
    _In_ PVOID BusInformation,
    _In_ PVOID LowerDevice,
    _In_ PCHAR ArgumentString,
    _Inout_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
    _In_ PBOOLEAN Again);

typedef
VOID
(NTAPI *PHW_FREE_ADAPTER_RESOURCES)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(NTAPI *PHW_PROCESS_SERVICE_REQUEST)(
    _In_ PVOID DeviceExtension,
    _In_ PVOID Irp);

typedef
VOID
(NTAPI *PHW_COMPLETE_SERVICE_IRP)(
    _In_ PVOID DeviceExtension);

typedef
VOID
(NTAPI *PHW_INITIALIZE_TRACING)(
    _In_ PVOID Arg1,
    _In_ PVOID Arg2);

typedef
VOID
(NTAPI *PHW_CLEANUP_TRACING)(
    _In_ PVOID Arg1);

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
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
    INTERFACE_TYPE AdapterInterfaceType;
    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    /*
     * PHW_FIND_ADAPTER for a physical miniport, PVIRTUAL_HW_FIND_ADAPTER for a
     * virtual one.
     */
    PVOID HwFindAdapter;
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
    union
    {
        USHORT ReservedUshort;
        USHORT PortVersionFlags;
    };
    USHORT DeviceIdLength;
    PVOID DeviceId;
#if (NTDDI_VERSION >= NTDDI_WIN11_GE)
    union
    {
        PHW_ADAPTER_CONTROL HwAdapterControl;
        PHW_NVME_ADAPTER_CONTROL HwNvmeAdapterControl;
    };
#else
    PHW_ADAPTER_CONTROL HwAdapterControl;
#endif
    PHW_BUILDIO HwBuildIo;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    PHW_FREE_ADAPTER_RESOURCES HwFreeAdapterResources;
    PHW_PROCESS_SERVICE_REQUEST HwProcessServiceRequest;
    PHW_COMPLETE_SERVICE_IRP HwCompleteServiceIrp;
    PHW_INITIALIZE_TRACING HwInitializeTracing;
    PHW_CLEANUP_TRACING HwCleanupTracing;
    PHW_TRACING_ENABLED HwTracingEnabled;
    ULONG FeatureSupport;
    ULONG SrbTypeFlags;
    ULONG AddressTypeFlags;
    ULONG Reserved1;
#if (NTDDI_VERSION >= NTDDI_WIN11_GE)
    union
    {
        PHW_UNIT_CONTROL HwUnitControl;
        PHW_NAMESPACE_CONTROL HwNamespaceControl;
    };
#else
    PHW_UNIT_CONTROL HwUnitControl;
#endif
#endif
} HW_INITIALIZATION_DATA, *PHW_INITIALIZATION_DATA;

#if (NTDDI_VERSION >= NTDDI_WIN8)

/*
 * A virtual miniport registers with this instead. The layout up to HwBuildIo
 * matches HW_INITIALIZATION_DATA so both can share the registration path.
 */
typedef struct _VIRTUAL_HW_INITIALIZATION_DATA
{
    ULONG HwInitializationDataSize;
    INTERFACE_TYPE AdapterInterfaceType;
    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PVOID Reserved1;
    PVIRTUAL_HW_FIND_ADAPTER HwFindAdapter;
    PHW_RESET_BUS HwResetBus;
    PVOID Reserved2;
    PVOID Reserved3;
    ULONG DeviceExtensionSize;
    ULONG SpecificLuExtensionSize;
    ULONG SrbExtensionSize;
    ULONG Reserved4;
    PVOID Reserved5;
    UCHAR MapBuffers;
    BOOLEAN Reserved6;
    BOOLEAN TaggedQueuing;
    BOOLEAN AutoRequestSense;
    BOOLEAN MultipleRequestPerLu;
    BOOLEAN ReceiveEvent;
    USHORT VendorIdLength;
    PVOID VendorId;
    union
    {
        USHORT ReservedUshort;
        USHORT PortVersionFlags;
    };
    USHORT DeviceIdLength;
    PVOID DeviceId;
    PHW_ADAPTER_CONTROL HwAdapterControl;
    PHW_BUILDIO HwBuildIo;
    PHW_FREE_ADAPTER_RESOURCES HwFreeAdapterResources;
    PHW_PROCESS_SERVICE_REQUEST HwProcessServiceRequest;
    PHW_COMPLETE_SERVICE_IRP HwCompleteServiceIrp;
    PHW_INITIALIZE_TRACING HwInitializeTracing;
    PHW_CLEANUP_TRACING HwCleanupTracing;
    PHW_TRACING_ENABLED HwTracingEnabled;
    ULONG FeatureSupport;
    ULONG SrbTypeFlags;
    ULONG AddressTypeFlags;
    ULONG Reserved7;
    PHW_UNIT_CONTROL HwUnitControl;
} VIRTUAL_HW_INITIALIZATION_DATA, *PVIRTUAL_HW_INITIALIZATION_DATA;

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */



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


#if (NTDDI_VERSION >= NTDDI_WIN7)

FORCEINLINE
ULONG
StorPortGetCurrentProcessorNumber(
    _In_ PVOID HwDeviceExtension,
    _Out_ PPROCESSOR_NUMBER ProcNumber)
{
    return StorPortExtendedFunction(ExtFunctionGetCurrentProcessorNumber,
                                    HwDeviceExtension,
                                    ProcNumber);
}

FORCEINLINE
ULONG
StorPortGetActiveGroupCount(
    _In_ PVOID HwDeviceExtension,
    _Out_ PUSHORT NumberGroups)
{
    return StorPortExtendedFunction(ExtFunctionGetActiveGroupCount,
                                    HwDeviceExtension,
                                    NumberGroups);
}

FORCEINLINE
ULONG
StorPortGetGroupAffinity(
    _In_ PVOID HwDeviceExtension,
    _In_ USHORT GroupNumber,
    _Out_ PKAFFINITY GroupAffinityMask)
{
    return StorPortExtendedFunction(ExtFunctionGetGroupAffinity,
                                    HwDeviceExtension,
                                    GroupNumber,
                                    GroupAffinityMask);
}

FORCEINLINE
ULONG
StorPortGetActiveNodeCount(
    _In_ PVOID HwDeviceExtension,
    _Out_ PULONG NumberNodes)
{
    return StorPortExtendedFunction(ExtFunctionGetActiveNodeCount,
                                    HwDeviceExtension,
                                    NumberNodes);
}

FORCEINLINE
ULONG
StorPortGetNodeAffinity(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG NodeNumber,
    _Out_ PGROUP_AFFINITY NodeAffinityMask)
{
    return StorPortExtendedFunction(ExtFunctionGetNodeAffinity,
                                    HwDeviceExtension,
                                    NodeNumber,
                                    NodeAffinityMask);
}

FORCEINLINE
ULONG
StorPortGetHighestNodeNumber(
    _In_ PVOID HwDeviceExtension,
    _Out_ PULONG HighestNode)
{
    return StorPortExtendedFunction(ExtFunctionGetHighestNodeNumber,
                                    HwDeviceExtension,
                                    HighestNode);
}

FORCEINLINE
ULONG
StorPortAllocateContiguousMemorySpecifyCacheNode(
    _In_ PVOID HwDeviceExtension,
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ NODE_REQUIREMENT PreferredNode,
    _Out_ PVOID *BufferPointer)
{
    return StorPortExtendedFunction(ExtFunctionAllocateContiguousMemorySpecifyCacheNode,
                                    HwDeviceExtension,
                                    NumberOfBytes,
                                    LowestAcceptableAddress,
                                    HighestAcceptableAddress,
                                    BoundaryAddressMultiple,
                                    CacheType,
                                    PreferredNode,
                                    BufferPointer);
}

FORCEINLINE
ULONG
StorPortFreeContiguousMemorySpecifyCache(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ MEMORY_CACHING_TYPE CacheType)
{
    return StorPortExtendedFunction(ExtFunctionFreeContiguousMemorySpecifyCache,
                                    HwDeviceExtension,
                                    BaseAddress,
                                    NumberOfBytes,
                                    CacheType);
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN7) */

#if (NTDDI_VERSION >= NTDDI_WIN8)

FORCEINLINE
ULONG
StorPortInitializeWorker(
    _In_ PVOID HwDeviceExtension,
    _Out_ PVOID *Worker)
{
    return StorPortExtendedFunction(ExtFunctionInitializeWorker,
                                    HwDeviceExtension,
                                    Worker);
}

FORCEINLINE
ULONG
StorPortQueueWorkItem(
    _In_ PVOID HwDeviceExtension,
    _In_ PHW_WORKITEM WorkItemCallback,
    _In_ PVOID Worker,
    _In_opt_ PVOID Context)
{
    return StorPortExtendedFunction(ExtFunctionQueueWorkItem,
                                    HwDeviceExtension,
                                    WorkItemCallback,
                                    Worker,
                                    Context);
}

FORCEINLINE
ULONG
StorPortFreeWorker(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Worker)
{
    return StorPortExtendedFunction(ExtFunctionFreeWorker,
                                    HwDeviceExtension,
                                    Worker);
}

FORCEINLINE
ULONG
StorPortInitializeTimer(
    _In_ PVOID HwDeviceExtension,
    _Out_ PVOID *TimerHandle)
{
    return StorPortExtendedFunction(ExtFunctionInitializeTimer,
                                    HwDeviceExtension,
                                    TimerHandle);
}

/* TimerValue and TolerableDelay are in microseconds. Zero stops the timer. */
FORCEINLINE
ULONG
StorPortRequestTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID TimerHandle,
    _In_ PHW_TIMER_EX TimerCallback,
    _In_opt_ PVOID CallbackContext,
    _In_ ULONGLONG TimerValue,
    _In_ ULONGLONG TolerableDelay)
{
    return StorPortExtendedFunction(ExtFunctionRequestTimer,
                                    HwDeviceExtension,
                                    TimerHandle,
                                    TimerCallback,
                                    CallbackContext,
                                    TimerValue,
                                    TolerableDelay);
}

FORCEINLINE
ULONG
StorPortFreeTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID TimerHandle)
{
    return StorPortExtendedFunction(ExtFunctionFreeTimer,
                                    HwDeviceExtension,
                                    TimerHandle);
}

FORCEINLINE
ULONG
StorPortInitializeSListHead(
    _In_ PVOID HwDeviceExtension,
    _In_ PSLIST_HEADER SListHead)
{
    return StorPortExtendedFunction(ExtFunctionInitializeSListHead,
                                    HwDeviceExtension,
                                    SListHead);
}

FORCEINLINE
ULONG
StorPortInterlockedFlushSList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSLIST_HEADER SListHead,
    _Out_ PSLIST_ENTRY *Result)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedFlushSList,
                                    HwDeviceExtension,
                                    SListHead,
                                    Result);
}

FORCEINLINE
ULONG
StorPortInterlockedPopEntrySList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSLIST_HEADER SListHead,
    _Out_ PSLIST_ENTRY *Result)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedPopEntrySList,
                                    HwDeviceExtension,
                                    SListHead,
                                    Result);
}

FORCEINLINE
ULONG
StorPortInterlockedPushEntrySList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSLIST_HEADER SListHead,
    _In_ PSLIST_ENTRY SListEntry,
    _Out_ PSLIST_ENTRY *Result)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedPushEntrySList,
                                    HwDeviceExtension,
                                    SListHead,
                                    SListEntry,
                                    Result);
}

FORCEINLINE
ULONG
StorPortQueryDepthSList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSLIST_HEADER SListHead,
    _Out_ PUSHORT Depth)
{
    return StorPortExtendedFunction(ExtFunctionQueryDepthSList,
                                    HwDeviceExtension,
                                    SListHead,
                                    Depth);
}

FORCEINLINE
ULONG
StorPortGetSystemPortNumber(
    _In_ PVOID HwDeviceExtension,
    _Inout_ PSTOR_ADDRESS Address)
{
    return StorPortExtendedFunction(ExtFunctionGetSystemPortNumber,
                                    HwDeviceExtension,
                                    Address);
}

FORCEINLINE
ULONG
StorPortSetUnitAttributes(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ STOR_UNIT_ATTRIBUTES Attributes)
{
    return StorPortExtendedFunction(ExtFunctionSetUnitAttributes,
                                    HwDeviceExtension,
                                    Address,
                                    Attributes);
}

FORCEINLINE
ULONG
StorPortGetRequestInfo(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PSTOR_REQUEST_INFO RequestInfo)
{
    return StorPortExtendedFunction(ExtFunctionGetRequestInfo,
                                    HwDeviceExtension,
                                    Srb,
                                    RequestInfo);
}

FORCEINLINE
ULONG
StorPortGetActivityIdSrb(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ LPGUID ActivityId)
{
    return StorPortExtendedFunction(ExtFunctionGetActivityId,
                                    HwDeviceExtension,
                                    Srb,
                                    ActivityId);
}

FORCEINLINE
ULONG
StorPortGetDataInBufferMdl(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PVOID *Mdl)
{
    return StorPortExtendedFunction(ExtFunctionGetDataInBufferMdl,
                                    HwDeviceExtension,
                                    Srb,
                                    Mdl);
}

FORCEINLINE
ULONG
StorPortGetDataInBufferSystemAddress(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PVOID *SystemAddress)
{
    return StorPortExtendedFunction(ExtFunctionGetDataInBufferSystemAddress,
                                    HwDeviceExtension,
                                    Srb,
                                    SystemAddress);
}

FORCEINLINE
ULONG
StorPortGetDataInBufferScatterGatherList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PSTOR_SCATTER_GATHER_LIST *SgList)
{
    return StorPortExtendedFunction(ExtFunctionGetDataInBufferScatterGatherList,
                                    HwDeviceExtension,
                                    Srb,
                                    SgList);
}

FORCEINLINE
ULONG
StorPortMarkDumpMemory(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Address,
    _In_ ULONG_PTR Length,
    _In_ ULONG Flags)
{
    return StorPortExtendedFunction(ExtFunctionMarkDumpMemory,
                                    HwDeviceExtension,
                                    Address,
                                    Length,
                                    Flags);
}

FORCEINLINE
ULONG
StorPortQueryPerformanceCounter(
    _In_ PVOID HwDeviceExtension,
    _Out_opt_ PLARGE_INTEGER PerformanceFrequency,
    _Out_ PLARGE_INTEGER PerformanceCounter)
{
    return StorPortExtendedFunction(ExtFunctionQueryPerformanceCounter,
                                    HwDeviceExtension,
                                    PerformanceFrequency,
                                    PerformanceCounter);
}

FORCEINLINE
ULONG
StorPortInvokeAcpiMethod(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG MethodName,
    _In_opt_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PULONG BytesReturned)
{
    return StorPortExtendedFunction(ExtFunctionInvokeAcpiMethod,
                                    HwDeviceExtension,
                                    Address,
                                    MethodName,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength,
                                    BytesReturned);
}

FORCEINLINE
ULONG
StorPortInitializePoFxPower(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ PSTOR_POFX_DEVICE Device,
    _Inout_ PBOOLEAN D3ColdEnabled)
{
    return StorPortExtendedFunction(ExtFunctionInitializePoFxPower,
                                    HwDeviceExtension,
                                    Address,
                                    Device,
                                    D3ColdEnabled);
}

FORCEINLINE
ULONG
StorPortPoFxActivateComponent(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    return StorPortExtendedFunction(ExtFunctionPoFxActivateComponent,
                                    HwDeviceExtension,
                                    Address,
                                    Srb,
                                    Component,
                                    Flags);
}

FORCEINLINE
ULONG
StorPortPoFxIdleComponent(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG Component,
    _In_ ULONG Flags)
{
    return StorPortExtendedFunction(ExtFunctionPoFxIdleComponent,
                                    HwDeviceExtension,
                                    Address,
                                    Srb,
                                    Component,
                                    Flags);
}

FORCEINLINE
ULONG
StorPortPoFxSetComponentLatency(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG Component,
    _In_ ULONGLONG Latency)
{
    return StorPortExtendedFunction(ExtFunctionPoFxSetComponentLatency,
                                    HwDeviceExtension,
                                    Address,
                                    Component,
                                    Latency);
}

FORCEINLINE
ULONG
StorPortPoFxSetComponentResidency(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG Component,
    _In_ ULONGLONG Residency)
{
    return StorPortExtendedFunction(ExtFunctionPoFxSetComponentResidency,
                                    HwDeviceExtension,
                                    Address,
                                    Component,
                                    Residency);
}

FORCEINLINE
ULONG
StorPortPoFxPowerControl(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ LPCGUID PowerControlCode,
    _In_opt_ PVOID InBuffer,
    _In_ SIZE_T InBufferSize,
    _Out_opt_ PVOID OutBuffer,
    _In_ SIZE_T OutBufferSize,
    _Out_opt_ PSIZE_T BytesReturned)
{
    return StorPortExtendedFunction(ExtFunctionPoFxPowerControl,
                                    HwDeviceExtension,
                                    Address,
                                    PowerControlCode,
                                    InBuffer,
                                    InBufferSize,
                                    OutBuffer,
                                    OutBufferSize,
                                    BytesReturned);
}

FORCEINLINE
ULONG
StorPortPoFxSetIdleTimeout(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG IdleTimeout)
{
    return StorPortExtendedFunction(ExtFunctionPoFxSetIdleTimeout,
                                    HwDeviceExtension,
                                    Address,
                                    IdleTimeout);
}

FORCEINLINE
ULONG
StorPortFlushDataBufferMdl(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    return StorPortExtendedFunction(ExtFunctionFlushDataBufferMdl,
                                    HwDeviceExtension,
                                    Srb);
}

FORCEINLINE
ULONG
StorPortDeviceOperationAllowed(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_ADDRESS Address,
    _In_ LPCGUID DeviceOperation,
    _Inout_ PULONG AllowedFlag)
{
    return StorPortExtendedFunction(ExtFunctionDeviceOperationAllowed,
                                    HwDeviceExtension,
                                    Address,
                                    DeviceOperation,
                                    AllowedFlag);
}

FORCEINLINE
ULONG
StorPortGetProcessorIndexFromNumber(
    _In_ PVOID HwDeviceExtension,
    _In_ PPROCESSOR_NUMBER ProcNumber,
    _Out_ PULONG ProcIndex)
{
    return StorPortExtendedFunction(ExtFunctionGetProcessorIndexFromNumber,
                                    HwDeviceExtension,
                                    ProcNumber,
                                    ProcIndex);
}

FORCEINLINE
ULONG
StorPortRegistryReadAdapterKey(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PCWSTR SubKeyName,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _Inout_ PVOID *ValueData,
    _Inout_ PULONG ValueDataLength)
{
    return StorPortExtendedFunction(ExtFunctionRegistryReadAdapterKey,
                                    HwDeviceExtension,
                                    SubKeyName,
                                    ValueName,
                                    ValueType,
                                    ValueData,
                                    ValueDataLength);
}

FORCEINLINE
ULONG
StorPortRegistryWriteAdapterKey(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PCWSTR SubKeyName,
    _In_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueDataLength)
{
    return StorPortExtendedFunction(ExtFunctionRegistryWriteAdapterKey,
                                    HwDeviceExtension,
                                    SubKeyName,
                                    ValueName,
                                    ValueType,
                                    ValueData,
                                    ValueDataLength);
}

FORCEINLINE
ULONG
StorPortUpdateAdapterMaxIO(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MaxIoCount)
{
    return StorPortExtendedFunction(ExtFunctionUpdateAdapterMaxIO,
                                    HwDeviceExtension,
                                    MaxIoCount);
}

/* Microseconds. Only legal below DISPATCH_LEVEL. */
FORCEINLINE
ULONG
StorPortDelayExecution(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_ LONGLONG Delay)
{
    return StorPortExtendedFunction(ExtFunctionDelayExecution,
                                    HwDeviceExtension,
                                    Srb,
                                    Delay);
}

FORCEINLINE
ULONG
StorPortAllocateDmaMemory(
    _In_ PVOID HwDeviceExtension,
    _In_ SIZE_T NumberOfBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ NODE_REQUIREMENT PreferredNode,
    _Out_ PVOID *BufferPointer,
    _Out_ PPHYSICAL_ADDRESS PhysicalAddress)
{
    return StorPortExtendedFunction(ExtFunctionAllocateDmaMemory,
                                    HwDeviceExtension,
                                    NumberOfBytes,
                                    LowestAcceptableAddress,
                                    HighestAcceptableAddress,
                                    BoundaryAddressMultiple,
                                    CacheType,
                                    PreferredNode,
                                    BufferPointer,
                                    PhysicalAddress);
}

FORCEINLINE
ULONG
StorPortFreeDmaMemory(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ MEMORY_CACHING_TYPE CacheType,
    _In_ PHYSICAL_ADDRESS PhysicalAddress)
{
    return StorPortExtendedFunction(ExtFunctionFreeDmaMemory,
                                    HwDeviceExtension,
                                    BaseAddress,
                                    NumberOfBytes,
                                    CacheType,
                                    PhysicalAddress);
}

/*
 * Host Memory Buffer. The device owns the returned ranges outright, so the
 * miniport has to be able to survive losing them at any moment.
 */
FORCEINLINE
ULONG
StorPortAllocateHostMemoryBuffer(
    _In_ PVOID HwDeviceExtension,
    _In_ SIZE_T MinimumBytes,
    _In_ SIZE_T PreferredBytes,
    _In_ ULONGLONG UtilizationBytes,
    _In_ ULONG AlignmentBytes,
    _In_ PHYSICAL_ADDRESS LowestAcceptableAddress,
    _In_ PHYSICAL_ADDRESS HighestAcceptableAddress,
    _In_opt_ PHYSICAL_ADDRESS BoundaryAddressMultiple,
    _Out_ PACCESS_RANGE PhysicalAddressRanges,
    _Inout_ PULONG PhysicalAddressRangeCount)
{
    return StorPortExtendedFunction(ExtFunctionAllocateHmb,
                                    HwDeviceExtension,
                                    MinimumBytes,
                                    PreferredBytes,
                                    UtilizationBytes,
                                    AlignmentBytes,
                                    LowestAcceptableAddress,
                                    HighestAcceptableAddress,
                                    BoundaryAddressMultiple,
                                    PhysicalAddressRanges,
                                    PhysicalAddressRangeCount);
}

FORCEINLINE
ULONG
StorPortFreeHostMemoryBuffer(
    _In_ PVOID HwDeviceExtension)
{
    return StorPortExtendedFunction(ExtFunctionFreeHmb, HwDeviceExtension);
}

FORCEINLINE
ULONG
StorPortEtwEvent2(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_ PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_opt_ PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_opt_ PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value)
{
    return StorPortExtendedFunction(ExtFunctionMiniportEtwEvent2,
                                    HwDeviceExtension,
                                    Address,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value);
}

FORCEINLINE
ULONG
StorPortEtwEvent4(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG EventId,
    _In_ PWSTR EventDescription,
    _In_ ULONGLONG EventKeywords,
    _In_ STORPORT_ETW_LEVEL EventLevel,
    _In_ STORPORT_ETW_EVENT_OPCODE EventOpcode,
    _In_opt_ PSCSI_REQUEST_BLOCK Srb,
    _In_opt_ PWSTR Parameter1Name,
    _In_ ULONGLONG Parameter1Value,
    _In_opt_ PWSTR Parameter2Name,
    _In_ ULONGLONG Parameter2Value,
    _In_opt_ PWSTR Parameter3Name,
    _In_ ULONGLONG Parameter3Value,
    _In_opt_ PWSTR Parameter4Name,
    _In_ ULONGLONG Parameter4Value)
{
    return StorPortExtendedFunction(ExtFunctionMiniportEtwEvent4,
                                    HwDeviceExtension,
                                    Address,
                                    EventId,
                                    EventDescription,
                                    EventKeywords,
                                    EventLevel,
                                    EventOpcode,
                                    Srb,
                                    Parameter1Name,
                                    Parameter1Value,
                                    Parameter2Name,
                                    Parameter2Value,
                                    Parameter3Name,
                                    Parameter3Value,
                                    Parameter4Name,
                                    Parameter4Value);
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#if (NTDDI_VERSION >= NTDDI_WIN8)

FORCEINLINE
ULONG
StorPortInitializeSpinLock(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_KSPIN_LOCK SpinLock)
{
    return StorPortExtendedFunction(ExtFunctionInitializeSpinlock,
                                    HwDeviceExtension,
                                    SpinLock);
}

FORCEINLINE
ULONG
StorPortInterlockedInsertHeadList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_LIST_ENTRY ListHead,
    _In_ PSTOR_LIST_ENTRY ListEntry,
    _In_ PSTOR_KSPIN_LOCK Lock)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedInsertHeadList,
                                    HwDeviceExtension,
                                    ListHead,
                                    ListEntry,
                                    Lock);
}

FORCEINLINE
ULONG
StorPortInterlockedInsertTailList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_LIST_ENTRY ListHead,
    _In_ PSTOR_LIST_ENTRY ListEntry,
    _In_ PSTOR_KSPIN_LOCK Lock)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedInsertTailList,
                                    HwDeviceExtension,
                                    ListHead,
                                    ListEntry,
                                    Lock);
}

FORCEINLINE
ULONG
StorPortInterlockedRemoveHeadList(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_LIST_ENTRY ListHead,
    _In_ PSTOR_KSPIN_LOCK Lock,
    _Out_ PSTOR_LIST_ENTRY *Result)
{
    return StorPortExtendedFunction(ExtFunctionInterlockedRemoveHeadList,
                                    HwDeviceExtension,
                                    ListHead,
                                    Lock,
                                    Result);
}

FORCEINLINE
ULONG
StorPortGetPfns(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID Mdl,
    _Out_ PVOID Pfns,
    _Inout_ PULONG PfnCount)
{
    return StorPortExtendedFunction(ExtFunctionGetPfns,
                                    HwDeviceExtension,
                                    Mdl,
                                    Pfns,
                                    PfnCount);
}

FORCEINLINE
ULONG
StorPortSetFeatureList(
    _In_ PVOID HwDeviceExtension,
    _In_range_(1, StorportFeatureMax) ULONG FeatureCount,
    _In_reads_(FeatureCount) PBOOLEAN FeatureList)
{
    return StorPortExtendedFunction(ExtFunctionSetFeatureList,
                                    HwDeviceExtension,
                                    FeatureCount,
                                    FeatureList);
}

FORCEINLINE
ULONG
StorPortQueryConfiguration(
    _In_ PVOID HwDeviceExtension,
    _In_ STORPORT_QUERY_CONFIGURATION_TYPE Type,
    _Out_ PVOID Buffer,
    _In_ ULONG BufferLength)
{
    return StorPortExtendedFunction(ExtFunctionQueryConfiguration,
                                    HwDeviceExtension,
                                    Type,
                                    Buffer,
                                    BufferLength);
}

FORCEINLINE
ULONG
StorPortInitializeEvent(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_EVENT Event,
    _In_ STOR_EVENT_TYPE Type,
    _In_ BOOLEAN State)
{
    return StorPortExtendedFunction(ExtFunctionInitializeEvent,
                                    HwDeviceExtension,
                                    Event,
                                    Type,
                                    State);
}

/* Timeout is in microseconds; a negative value means wait forever */
FORCEINLINE
ULONG
StorPortWaitForEvent(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_EVENT Event,
    _In_ LONGLONG Timeout)
{
    return StorPortExtendedFunction(ExtFunctionWaitForEvent,
                                    HwDeviceExtension,
                                    Event,
                                    Timeout);
}

FORCEINLINE
ULONG
StorPortSetEvent(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_EVENT Event,
    _Out_opt_ PLONG PreviousState)
{
    return StorPortExtendedFunction(ExtFunctionSetEvent,
                                    HwDeviceExtension,
                                    Event,
                                    PreviousState);
}

FORCEINLINE
ULONG
StorPortHardwareReset(
    _In_ PVOID HwDeviceExtension,
    _In_ STOR_DEVICE_RESET_TYPE Type)
{
    return StorPortExtendedFunction(ExtFunctionDeviceReset,
                                    HwDeviceExtension,
                                    Type);
}

FORCEINLINE
ULONG
StorPortQueryDpcWatchdogInformation(
    _In_ PVOID HwDeviceExtension,
    _Out_ PSTOR_DPC_WATCHDOG_INFORMATION WatchdogInformation)
{
    return StorPortExtendedFunction(ExtFunctionQueryDpcWatchdogInformation,
                                    HwDeviceExtension,
                                    WatchdogInformation);
}

FORCEINLINE
ULONG
StorPortQueryTimerMinInterval(
    _In_ PVOID HwDeviceExtension,
    _Out_ PULONG MinInterval)
{
    return StorPortExtendedFunction(ExtFunctionQueryTimerMinInterval,
                                    HwDeviceExtension,
                                    MinInterval);
}

FORCEINLINE
ULONG
StorPortGetCurrentIrql(
    _In_ PVOID HwDeviceExtension,
    _Out_ PKIRQL Irql)
{
    return StorPortExtendedFunction(ExtFunctionGetCurrentIrql,
                                    HwDeviceExtension,
                                    Irql);
}

FORCEINLINE
ULONG
StorPortGetProcessorCount(
    _In_ PVOID HwDeviceExtension,
    _Out_ PULONG ProcessorCount)
{
    return StorPortExtendedFunction(ExtFunctionGetProcessorCount,
                                    HwDeviceExtension,
                                    ProcessorCount);
}

FORCEINLINE
ULONG
StorPortGetCurrentProcessorIndex(
    _In_ PVOID HwDeviceExtension,
    _Out_ PULONG ProcessorIndex)
{
    return StorPortExtendedFunction(ExtFunctionGetCurrentProcessorIndex,
                                    HwDeviceExtension,
                                    ProcessorIndex);
}

FORCEINLINE
ULONG
StorPortCreateSystemThread(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID ThreadStartRoutine,
    _In_opt_ PVOID StartContext,
    _Out_ PVOID *ThreadHandle)
{
    return StorPortExtendedFunction(ExtFunctionCreateSystemThread,
                                    HwDeviceExtension,
                                    ThreadStartRoutine,
                                    StartContext,
                                    ThreadHandle);
}

FORCEINLINE
ULONG
StorPortSetPriorityThread(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID ThreadHandle,
    _In_ STOR_THREAD_PRIORITY Priority,
    _Out_opt_ PSTOR_THREAD_PRIORITY OldPriority)
{
    return StorPortExtendedFunction(ExtFunctionSetPriorityThread,
                                    HwDeviceExtension,
                                    ThreadHandle,
                                    Priority,
                                    OldPriority);
}

FORCEINLINE
ULONG
StorPortSetSystemGroupAffinityThread(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_GROUP_AFFINITY Affinity,
    _Out_opt_ PSTOR_GROUP_AFFINITY PreviousAffinity)
{
    return StorPortExtendedFunction(ExtFunctionSetSystemGroupAffinityThread,
                                    HwDeviceExtension,
                                    Affinity,
                                    PreviousAffinity);
}

FORCEINLINE
ULONG
StorPortRevertToUserGroupAffinityThread(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_GROUP_AFFINITY PreviousAffinity)
{
    return StorPortExtendedFunction(ExtFunctionRevertToUserGroupAffinityThread,
                                    HwDeviceExtension,
                                    PreviousAffinity);
}

FORCEINLINE
ULONG
StorPortTerminateSystemThread(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG ExitStatus)
{
    StorPortNotification(TerminateSystemThread, HwDeviceExtension, ExitStatus);
    return STOR_STATUS_SUCCESS;
}

FORCEINLINE
ULONG
StorPortLogTelemetry(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ PSTORPORT_TELEMETRY_EVENT Event)
{
    return StorPortExtendedFunction(ExtFunctionMiniportTelemetry,
                                    HwDeviceExtension,
                                    Address,
                                    Event);
}

FORCEINLINE
ULONG
StorPortLogTelemetryEx(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ PSTORPORT_TELEMETRY_EVENT Event,
    _In_ STOR_TELEMETRY_CATEGORY Category)
{
    return StorPortExtendedFunction(ExtFunctionMiniportTelemetryEx,
                                    HwDeviceExtension,
                                    Address,
                                    Event,
                                    Category);
}

FORCEINLINE
ULONG
StorPortMiniportReportInternalData(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID CallbackContext,
    _In_ ULONG DataCount,
    _In_reads_(DataCount) PMINIPORT_DATA Data)
{
    return StorPortExtendedFunction(ExtFunctionMiniportReportInternalData,
                                    HwDeviceExtension,
                                    CallbackContext,
                                    DataCount,
                                    Data);
}

FORCEINLINE
ULONG
StorPortCaptureLiveDump(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ PSTORPORT_CAPTURE_LIVEDUMP CaptureLivedump)
{
    return StorPortExtendedFunction(ExtFunctionCaptureLiveDump,
                                    HwDeviceExtension,
                                    Address,
                                    CaptureLivedump);
}

FORCEINLINE
ULONG
StorPortInitializeHighResolutionTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PHW_TIMER_EX Callback,
    _In_opt_ PVOID Context,
    _Out_ PVOID *TimerHandle)
{
    return StorPortExtendedFunction(ExtFunctionInitializeHighResolutionTimer,
                                    HwDeviceExtension,
                                    Callback,
                                    Context,
                                    TimerHandle);
}

/* TimerValue is in microseconds */
FORCEINLINE
ULONG
StorPortRequestHighResolutionTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID TimerHandle,
    _In_ ULONGLONG TimerValue)
{
    return StorPortExtendedFunction(ExtFunctionRequestHighResolutionTimer,
                                    HwDeviceExtension,
                                    TimerHandle,
                                    TimerValue);
}

FORCEINLINE
ULONG
StorPortCancelHighResolutionTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID TimerHandle,
    _In_ BOOLEAN Wait)
{
    return StorPortExtendedFunction(ExtFunctionCancelHighResolutionTimer,
                                    HwDeviceExtension,
                                    TimerHandle,
                                    Wait);
}

FORCEINLINE
ULONG
StorPortFreeHighResolutionTimer(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID TimerHandle)
{
    return StorPortExtendedFunction(ExtFunctionFreeHighResolutionTimer,
                                    HwDeviceExtension,
                                    TimerHandle);
}

FORCEINLINE
ULONG
StorPortCancelDpc(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_DPC Dpc,
    _Out_ PBOOLEAN Cancelled)
{
    return StorPortExtendedFunction(ExtFunctionCancelDpc,
                                    HwDeviceExtension,
                                    Dpc,
                                    Cancelled);
}

FORCEINLINE
ULONG
StorPortMaskPciMsixEntry(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG MessageId,
    _In_ BOOLEAN Mask)
{
    return StorPortExtendedFunction(ExtFunctionMaskPciMsixEntry,
                                    HwDeviceExtension,
                                    MessageId,
                                    Mask);
}

FORCEINLINE
ULONG
StorPortCurrentOsInstallationUpgrade(
    _In_ PVOID HwDeviceExtension,
    _Out_ PBOOLEAN OsInstallationUpgrade)
{
    return StorPortExtendedFunction(ExtFunctionCurrentOsInstallationUpgrade,
                                    HwDeviceExtension,
                                    OsInstallationUpgrade);
}

FORCEINLINE
ULONG
StorPortGetD3ColdSupport(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _Out_ PBOOLEAN D3ColdSupported)
{
    return StorPortExtendedFunction(ExtFunctionGetD3ColdSupport,
                                    HwDeviceExtension,
                                    Address,
                                    D3ColdSupported);
}

FORCEINLINE
ULONG
StorPortSetAdapterBusType(
    _In_ PVOID HwDeviceExtension,
    _In_ ULONG BusType)
{
    return StorPortExtendedFunction(ExtFunctionSetAdapterBusType,
                                    HwDeviceExtension,
                                    BusType);
}

FORCEINLINE
ULONG
StorPortPropagateIrpExtension(
    _In_ PVOID HwDeviceExtension,
    _In_ PVOID SourceSrb,
    _Inout_ PVOID DestinationSrb)
{
    return StorPortExtendedFunction(ExtFunctionPropagateIrpExtension,
                                    HwDeviceExtension,
                                    SourceSrb,
                                    DestinationSrb);
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */

#if (NTDDI_VERSION >= NTDDI_WIN8)

FORCEINLINE
ULONG
StorPortPoFxRegisterPerfStates(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG ComponentIndex,
    _In_ ULONG PerfSetCount,
    _In_reads_(PerfSetCount) PSTOR_POFX_COMPONENT_PERF_SET *PerfSets)
{
    return StorPortExtendedFunction(ExtFunctionPoFxRegisterPerfStates,
                                    HwDeviceExtension,
                                    Address,
                                    ComponentIndex,
                                    PerfSetCount,
                                    PerfSets);
}

FORCEINLINE
ULONG
StorPortPoFxSetPerfState(
    _In_ PVOID HwDeviceExtension,
    _In_opt_ PSTOR_ADDRESS Address,
    _In_ ULONG ComponentIndex,
    _In_ ULONG PerfSetIndex,
    _In_ ULONGLONG Value,
    _In_opt_ PVOID Context)
{
    return StorPortExtendedFunction(ExtFunctionPoFxSetPerfState,
                                    HwDeviceExtension,
                                    Address,
                                    ComponentIndex,
                                    PerfSetIndex,
                                    Value,
                                    Context);
}

FORCEINLINE
ULONG
StorPortInitializeCryptoEngine(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_CRYPTO_CAPABILITIES_DATA CryptoCapabilities)
{
    return StorPortExtendedFunction(ExtFunctionInitializeCryptoEngine,
                                    HwDeviceExtension,
                                    CryptoCapabilities);
}

FORCEINLINE
ULONG
StorPortGetRequestCryptoInfo(
    _In_ PVOID HwDeviceExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _Out_ PSTOR_CRYPTO_KEY_INFO KeyInfo)
{
    return StorPortExtendedFunction(ExtFunctionGetRequestCryptoInfo,
                                    HwDeviceExtension,
                                    Srb,
                                    KeyInfo);
}

FORCEINLINE
ULONG
StorPortInitializeRpmb(
    _In_ PVOID HwDeviceExtension,
    _In_ PSTOR_RPMB_CAPABILITIES_DATA RpmbCapabilities)
{
    return StorPortExtendedFunction(ExtFunctionInitializeRpmb,
                                    HwDeviceExtension,
                                    RpmbCapabilities);
}

#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
#if DBG
#define DebugPrint(x) StorPortDebugPrint x
#else
#define DebugPrint(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NTSTORPORT_ */
