/**
 *******************************************************************************
 ** Copyright (c) 2011-2012                                                   **
 **                                                                           **
 **   Integrated Device Technology, Inc.                                      **
 **   Intel Corporation                                                       **
 **   LSI Corporation                                                         **
 **                                                                           **
 ** All rights reserved.                                                      **
 **                                                                           **
 *******************************************************************************
 **                                                                           **
 ** Redistribution and use in source and binary forms, with or without        **
 ** modification, are permitted provided that the following conditions are    **
 ** met:                                                                      **
 **                                                                           **
 **   1. Redistributions of source code must retain the above copyright       **
 **      notice, this list of conditions and the following disclaimer.        **
 **                                                                           **
 **   2. Redistributions in binary form must reproduce the above copyright    **
 **      notice, this list of conditions and the following disclaimer in the  **
 **      documentation and/or other materials provided with the distribution. **
 **                                                                           **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS   **
 ** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, **
 ** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR    **
 ** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR         **
 ** CONTRIBUTORS BE LIABLE FOR ANY DIRECT,INDIRECT, INCIDENTAL, SPECIAL,      **
 ** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,       **
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR        **
 ** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
 ** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
 ** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
 ** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
 **                                                                           **
 ** The views and conclusions contained in the software and documentation     **
 ** are those of the authors and should not be interpreted as representing    **
 ** official policies, either expressed or implied, of Intel Corporation,     **
 ** Integrated Device Technology Inc., or Sandforce Corporation.              **
 **                                                                           **
 *******************************************************************************
**/

/*
 * File: nvmeSntiTypes.h
 */

#ifndef __SNTI_TYPES_H__
#define __SNTI_TYPES_H__

/* SCSI SPC/SBC opcodes not defined by Storport.h (WDK) */
#define SCSIOP_COMPARE_AND_WRITE                   0x89
#define SCSIOP_SECURITY_PROTOCOL_IN                0xA2
#define SCSIOP_SECURITY_PROTOCOL_OUT               0xB5
#define SCSIOP_UNMAP                               0x42
#define SCSIOP_WRITE_LONG16                        0x9F

/* CDB offsets */
#define CDB_6_CONTROL_OFFSET                          5
#define CDB_10_CONTROL_OFFSET                         9
#define CDB_12_CONTROL_OFFSET                        11
#define CDB_16_CONTROL_OFFSET                        15
#define CDB_VAR_CONTROL_OFFSET                        1
#define CONTROL_BYTE_NACA_MASK                      0x4
#define READ_6_LBA_OFFSET                             1
#define READ_10_LBA_OFFSET                            2
#define READ_12_LBA_OFFSET                            2
#define READ_16_LBA_OFFSET_MSW                        2
#define READ_16_LBA_OFFSET_LSW                        6
#define READ_6_LENGTH_OFFSET                          4
#define READ_10_LENGTH_OFFSET                         7
#define READ_12_LENGTH_OFFSET                         6
#define READ_16_LENGTH_OFFSET                        10
#define WRITE_6_LBA_OFFSET                            1
#define WRITE_10_LBA_OFFSET                           2
#define WRITE_12_LBA_OFFSET                           2
#define WRITE_16_LBA_OFFSET                           2
#define INQ_EVPD_BYTE_OFFSET                          1
#define INQ_PAGE_CODE_BYTE_OFFSET                     2
#define INQ_EVPD_BIT_MASK                             1
#define INQ_CDB_ALLOCATION_LENGTH_OFFSET              3
#define READ_CAP_10_DISK_CAPACITY_LSB                 4
#define READ_CAP_10_SECTOR_SIZE_LSB                   8
#define READ_CAP_10_SECTOR_SIZE_OFFSET                4
#define READ_CAP_10_PARM_DATA_SIZE                    8
#define READ_CAP_16_PARM_DATA_SIZE                   32
#define REPORT_LUNS_CDB_ALLOC_LENGTH_OFFSET           6
#define REPORT_LUNS_SELECT_REPORT_OFFSET              2
#define READ_CAP_16_CDB_ALLOC_LENGTH_OFFSET          10
#define READ_CAP_16_SERVICE_ACTION_OFFSET             1
#define READ_CAP_16_SERVICE_ACTION_IN              0x10
#define REQUEST_SENSE_CDB_ALLOC_LENGTH_OFFSET         4
#define REQUEST_SENSE_DESCRIPTOR_FORMAT_OFFSET        1
#define REQUEST_SENSE_DESCRIPTOR_FORMAT_MASK       0x01
#define DESCRIPTOR_FORMAT_SENSE_DATA_TYPE             1
#define INQUIRY_EVPD_BYTE_OFFSET                      1
#define INQUIRY_PAGE_CODE_BYTE_OFFSET                 2
#define INQUIRY_EVPD_BIT_MASK                         1
#define INQUIRY_CDB_ALLOCATION_LENGTH_OFFSET          3
#define READ_CAP_10_DISK_CAPACITY_LSB                 4
#define READ_CAP_10_SECTOR_SIZE_LSB                   8
#define READ_CAP_10_SECTOR_SIZE_OFFSET                4
#define RETURNED_LBA_OFFSET                           0
#define LBA_LENGTH_OFFSET                             4
#define START_STOP_UNIT_CDB_IMMED_OFFSET              1
#define START_STOP_UNIT_CDB_IMMED_MASK              0x1
#define START_STOP_UNIT_CDB_POWER_COND_MOD_OFFSET     3
#define START_STOP_UNIT_CDB_POWER_COND_MOD_MASK     0xF
#define START_STOP_UNIT_CDB_POWER_COND_OFFSET         4
#define START_STOP_UNIT_CDB_POWER_COND_MASK        0xF0
#define START_STOP_UNIT_CDB_NO_FLUSH_OFFSET           4
#define START_STOP_UNIT_CDB_NO_FLUSH_MASK           0x4
#define START_STOP_UNIT_CDB_LOAD_EJECT_OFFSET         4
#define START_STOP_UNIT_CDB_LOAD_EJECT_MASK         0x2
#define START_STOP_UNIT_CDB_START_OFFSET              4
#define START_STOP_UNIT_CDB_START_MASK              0x1
#define WRITE_BUFFER_CDB_MODE_OFFSET                  1
#define WRITE_BUFFER_CDB_MODE_MASK                 0x1F
#define WRITE_BUFFER_CDB_BUFFER_ID_OFFSET             2
#define WRITE_BUFFER_CDB_BUFFER_OFFSET_OFFSET         3
#define WRITE_BUFFER_CDB_PARAM_LIST_LENGTH_OFFSET     6
#define FORMAT_UNIT_CDB_FORMAT_PROT_INFO_OFFSET       1
#define FORMAT_UNIT_CDB_FORMAT_PROT_INFO_MASK      0xC0
#define FORMAT_UNIT_CDB_FORMAT_PROT_INFO_SHIFT        6
#define FORMAT_UNIT_CDB_LONG_LIST_OFFSET              1
#define FORMAT_UNIT_CDB_LONG_LIST_MASK             0x20
#define FORMAT_UNIT_CDB_FORMAT_DATA_OFFSET            1
#define FORMAT_UNIT_CDB_FORMAT_DATA_MASK           0x10
#define FORMAT_UNIT_CDB_COMPLETE_LIST_OFFSET          1
#define FORMAT_UNIT_CDB_COMPLETE_LIST_MASK          0x8
#define FORMAT_UNIT_CDB_DEFECT_LIST_FORMAT_OFFSET     1
#define FORMAT_UNIT_CDB_DEFECT_LIST_FORMAT_MASK     0x7
#define PROT_TYPE_0                                   0
#define PROT_TYPE_1                                   1
#define PROT_TYPE_2                                   2
#define PROT_TYPE_3                                   3
#define FORMAT_NVM_PROTECTION_INFO_SHIFT_MASK         5

/* Misc. defines */
#define NVME_MAX_LEN                                256
#define SECTOR_SIZE                                 512
#define STANDARD_INQUIRY_PAGE_SIZE                   (INQUIRYDATABUFFERSIZE)
#define UNIT_SERIAL_NUMBER_PAGE_SIZE                 24
#define DEVICE_IDENTIFICATION_PAGE_SIZE              24
#define EXTENDED_INQUIRY_DATA_PAGE_SIZE              64
#define SUPPORTED_VPD_PAGES_PAGE_SIZE                 7
#define REPORT_LUNS_SIZE                             16
#define DWORD_SHIFT_MASK                             32
#define DWORD_MASK_BYTE_3                    0xFF000000
#define DWORD_MASK_BYTE_2                    0x00FF0000
#define DWORD_MASK_BYTE_1                    0x0000FF00
#define DWORD_MASK_BYTE_0                    0x000000FF
#define DWORD_MASK_LOW_WORD                  0x0000FFFF
#define DWORD_MASK_HIGH_WORD                 0xFFFF0000
#define NUM_BYTES_IN_DWORD                           4
#define BYTE_SHIFT_3                                 24
#define BYTE_SHIFT_2                                 16
#define BYTE_SHIFT_1                                  8
#define NIBBLE_SHIFT                                  4
#define ONE_OR_MORE_PHYSICAL_BLOCKS                   0
#define UNSPECIFIED                                   0
#define SPECIFIED                                     1
#define LBA_0                                         0
#define PROTECTION_DISABLED                           0
#define PROTECTION_ENABLED                            1
#define FIXED_SENSE_DATA                           0x70
#define DESC_FORMAT_SENSE_DATA                     0x72
#define FIXED_SENSE_DATA_ADD_LENGTH                  10
#define RESERVED_FIELDS                             0x0
#define LUN_ENTRY_SIZE                                8
#define LUN_DATA_HEADER_SIZE                          8
#define LOGICAL_AND_SUBS_LUNS_RET                  0x12
#define ADMIN_AND_LOGICAL_LUNS_RET                 0x11
#define ADMIN_LUNS_RETURNED                        0x10
#define ALL_LUNS_RETURNED                          0x02
#define ALL_WELL_KNOWN_LUNS_RETURNED               0x01
#define RESTRICTED_LUNS_RETURNED                   0x00
#define NVME_PAGE_SIZE                             4096
#define PRP_ENTRY_OFFSET_SHIFT_MASK                  12
#define PRP_ENTRY_BASE_ADDR_MASK     0xFFFFFFFFFFFFF000
#define WORD_HIGH_BYTE_MASK                      0xFF00
#define WORD_LOW_BYTE_MASK                         0xFF
#define SINGLE_BYTE_SHIFT                           0x8
#define NVME_WRITE_PBAOFFSET_SHIFT                    2
#define NVME_WRITE_PBAOFFSET_MASK            0xFFFFFFFC
#define IN_COMMAND_PRP_ENTRY_LIMIT                    2
#define PRP_ENTRY_LOW_2_BITS_MASK                   0x3
#define DWORD_BIT_MASK                       0xFFFFFFFF
#define NVME_POWER_STATE_START_VALID               0x00
#define NVME_POWER_STATE_ACTIVE                    0x01
#define NVME_POWER_STATE_IDLE                      0x02
#define NVME_POWER_STATE_STANDBY                   0x03
#define NVME_POWER_STATE_LU_CONTROL                0x07
#define POWER_STATE_0                                 0
#define POWER_STATE_1                                 1
#define POWER_STATE_2                                 2
#define POWER_STATE_3                                 3
#define POWER_STATE_4                                 4
#define POWER_STATE_5                                 5
#define POWER_STATE_6                                 6
#define DOWNLOAD_SAVE_ACTIVATE                     0x05
#define DOWNLOAD_SAVE_DEFER_ACTIVATE               0x0E
#define ACTIVATE_DEFERRED_MICROCODE                0x0F
#define PROTECTION_FIELD_USAGE_MASK                 0x7
#define FORMAT_UNIT_IMMED_MASK                      0x2
#define PROTECTION_INTERVAL_EXPONENT_MASK           0xF
#define SCSISTAT_TASK_ABORTED                      0x40
#define SCSI_ADSENSE_PERIPHERAL_DEV_WRITE_FAULT    0x03
#define SCSI_ADSENSE_LOG_BLOCK_GUARD_CHECK_FAILED  0x10
#define SCSI_ADSENSE_LOG_BLOCK_APPTAG_CHECK_FAILED 0x10
#define SCSI_ADSENSE_LOG_BLOCK_REFTAG_CHECK_FAILED 0x10
#define SCSI_ADSENSE_UNRECOVERED_READ_ERROR        0x11
#define SCSI_ADSENSE_MISCOMPARE_DURING_VERIFY      0x1D
#define SCSI_ADSENSE_ACCESS_DENIED_INVALID_LUN_ID  0x20
#define SCSI_ADSENSE_INTERNAL_TARGET_FAILURE       0x44
#define SCSI_ADSENSE_FORMAT_COMMAND_FAILED         0x31
#define SCSI_SENSEQ_INVALID_LUN_ID                 0x09
#define SCSI_SENSEQ_FORMAT_COMMAND_FAILED          0x01
#define SCSI_SENSEQ_LOG_BLOCK_GUARD_CHECK_FAILED   0x01
#define SCSI_SENSEQ_LOG_BLOCK_APPTAG_CHECK_FAILED  0x02
#define SCSI_SENSEQ_LOG_BLOCK_REFTAG_CHECK_FAILED  0x03
#define SCSI_SENSEQ_ACCESS_DENIED_INVALID_LUN_ID   0x09
#define NVM_CMD_SET_STATUS                         0x80
#define NVM_CMD_SET_GENERIC_STATUS_OFFSET          0x74
#define NVM_CMD_SET_SPECIFIC_STATUS_OFFSET         0x75
#define NVM_MEDIA_ERROR_STATUS_OFFSET              0x80
#define KELVIN_TEMP_FACTOR                          273
#define UPPER_DWORD_BIT_MASK         0xFFFFFFFF00000000
#define PRP_ENTRY_1                                   1
#define PRP_ENTRY_2                                   2
#define PRP_ENTRY_3                                   3
#define PAGE_MASK                       (PAGE_SIZE - 1)
#define NUM_SUPPORTED_INQ_PAGES                       3
#define BYTE_0                                        0
#define BYTE_1                                        1
#define BYTE_2                                        2
#define BYTE_3                                        3
#define BYTE_4                                        4
#define BYTE_5                                        5
#define BYTE_6                                        6
#define BYTE_7                                        7
/* 
  Set the following to 1 if your NVMe controller returns zeros when LBAs 
  that have been previously UNMAPED (via DSM dealloc) are read
*/
#define ZEROS_RETURNED_INDICATOR                      1

/* Diagnostig page codes */
#define DIAG_SUPPORTED_LOG                         0x00
#define DIAG_BUFFER_RUN                            0x01
#define DIAG_ERROR_WRITE                           0x02
#define DIAG_ERROR_READ                            0x03
#define DIAG_ERROR_VERIFY                          0x05
#define DIAG_NO_MEDIUM_ERRORS                      0x06

/* SCSI/NVMe defines and bit masks */
#define READ_6_LBA_MASK                        0x1FFFFF
#define LBA_MASK_LOWER_32_BITS               0xFFFFFFFF
#define LSB_BYTE_MASK                               0xF
#define NAMESPACE_IDENTIFY_DATA_DPS_ENABLED_MASK    0x8
#define NAMESPACE_IDENTIFY_DATA_DPS_MASK            0x7
#define LIMITED_RETRY_ENABLED                0x80000000
#define LIMITED_RETRY_DISABLED               0x00000000
#define FUA_ENABLED                          0x40000000
#define FUA_DISABLED                         0x00000000
#define PROTECTION_INFO                      0x00000000
#define NVME_WRITE_PRINFO_BIT_OFFSET                 26
#define NVME_READ_PRINFO_BIT_OFFSET                  26
#define INQ_STANDARD_INQUIRY_PAGE                  0x00
#define INQ_SUPPORTED_VPD_PAGES_PAGE               0x00
#define INQ_UNIT_SERIAL_NUMBER_PAGE                0x80
#define INQ_DEVICE_IDENTIFICATION_PAGE             0x83
#define INQ_EXTENDED_INQUIRY_DATA_PAGE             0x86
#define INQ_SN_FROM_EUI64_LENGTH                   0x14
#define INQ_SN_FROM_NGUID_LENGTH                   0x28
#define INQ_V10_SN_LENGTH                          0x1E

#define INQ_SN_SUB_LENGTH                             4
/* For Win 8 UNMAP support, there are additional VPD pages
   we provide such as logical block provisioning
*/ 
#if (NTDDI_VERSION > NTDDI_WIN7)
#define INQ_NUM_SUPPORTED_VPD_PAGES                   6
#else 
#define INQ_NUM_SUPPORTED_VPD_PAGES                   3
#endif
#define INQ_RESERVED                                  0
#define BLOCK_LIMITS_PAGE_LENGTH                   0x3C
#define BLOCK_DEVICE_CHAR_PAGE_LENGTH              0x3C
#define LOGICAL_BLOCK_PROVISIONING_PAGE_LENGTH     0x04
#define MAX_UNMAP_BLOCK_DESCRIPTOR_COUNT            256
/* Rotation rate of 1 indicates non-rotating (SSD) */
#define MEDIUM_ROTATIONAL_RATE                   0x0001
#define FORM_FACTOR_NOT_REPORTED                      0
#define NO_THIN_PROVISIONING_THRESHHOLD               0
#define WR_SAME_16_TO_UNMAP_NOT_SUPPORTED             0
#define WR_SAME_10_TO_UNMAP_NOT_SUPPORTED             0
#define ANC_NOT_SUPPORTED                             0
#define UNMAP_ANCHAR_BIT                              1
#define NO_PROVISIONING_GROUP_DESCRIPTOR              0
#define UNREMOVABLE_MEDIA                             0
#define VERSION_SPC_4                              0x06
#define ACA_UNSUPPORTED                               0
#define HIERARCHAL_ADDR_UNSUPPORTED                   0
#define RESPONSE_DATA_FORMAT_SPC_4                    2
#define STANDARD_INQUIRY_LENGTH                      36
#define ADDITIONAL_STD_INQ_LENGTH                    31
#define EXTENDED_INQUIRY_DATA_PAGE_LENGTH          0x3C
#define VENDOR_ID_LENGTH                            0x8
#define EMBEDDED_ENCLOSURE_SERVICES_UNSUPPORTED       0
#define MEDIUM_CHANGER_UNSUPPORTED                    0
#define COMMAND_MANAGEMENT_MODEL                      1
#define WIDE_16_BIT_XFERS_UNSUPPORTED                 0
#define WIDE_16_BIT_ADDRESES_UNSUPPORTED              0
#define SYNCHRONOUS_DATA_XFERS_UNSUPPORTED            0
#define RESERVED_FIELD                                0
#define CLOCKING_UNSUPPORTED                        0x0
#define QAS_UNSUPPORTED                               0
#define IUS_UNSUPPORTED                               0
#define SBC_3_VERSION_DESCRIPTOR                 0x04C0
#define SPC_4_VERSION_DESCRIPTOR                 0x0300
#define VERSION_DESC_1                                2
#define VERSION_DESC_2                                4
#define EUI64_ID                     0x84EB0AFFFF171500
#define NAA_IEEE_EX                                   6

// Device Identification defined in NVMe-SCSI Translation Spec 1.5, Section 6.1.4
#define VPD_ID_DESCRIPTOR_HDR_LENGTH (sizeof(VPD_IDENTIFICATION_DESCRIPTOR))
#define EUI64_DATA_SIZE                              8
#define EUI64_ASCII_SIZE                             EUI64_DATA_SIZE*2
#define NGUID_DATA_SIZE                              16
#define NGUID_ASCII_SIZE                             NGUID_DATA_SIZE*2
#define PRODUCT_ID_ASCII_SIZE                        16
#define VENDOR_ID_DATA_SIZE                          2
#define VENDOR_ID_ASCII_SIZE                         VENDOR_ID_DATA_SIZE*2
#define VENDOR_SPEC_V10_SN_ASCII_SIZE                13
#define VENDOR_SPEC_V10_NSID_ASCII_SIZE              8


// T10 Vendor Specific identifier defined in NVMe-SCSI Translation Spec 1.5
#define T10_VID_DES_VID_FIELD_SIZE                    sizeof(SNTI_T10_VID_DESCRIPTOR) - 1


// SCSI Name String defined in NVMe-SCSI Translation Spec 1.5
/* SCSI name string defines for V1.0 */
#define ASCII_SPACE_CHAR_VALUE                     0x20
#define SCSI_NAME_PCI_VENDOR_ID_SIZE	              4
#define SCSI_NAME_NAMESPACE_ID_SIZE                   4	
#define SCSI_NAME_MODEL_NUM_SIZE                     40
#define SCSI_NAME_SERIAL_NUM_SIZE                    20

/* SCSI name string defines for V1.1 */
#define EUI_ASCII_SIZE                                4
#define NGUID_ID_SIZE                                 4


#define INQ_DEV_ID_DESCRIPTOR_RESERVED                0
#define INQ_DEV_ID_DESCRIPTOR_OFFSET                0x8
#define DATA_PROTECTION_CAPABILITIES_MASK           0xF
#define DATA_PROTECTION_SETTINGS_MASK               0x7
#define ACTIVATE_AFTER_HARD_RESET                   0x2
#define SENSE_KEY_SPECIFIC_DATA                       1
#define GROUPING_FUNCTION_UNSUPPORTED                 0
#define COMMAND_PRIORITY_UNSUPPORTED                  0
#define HEAD_OF_QUEUE_TASK_ATTR_UNSUPPORTED           0
#define ORDERED_TASK_ATTR_UNSUPPORTED                 0
#define SIMPLE_TASK_ATTR_UNSUPPORTED                  0
#define WRITE_UNCORRECTABLE_UNSUPPORTED               0
#define CORRECTION_DISABLE_UNSUPPORTED                0
#define NON_VOLATILE_CACHE_UNSUPPORTED                0
#define PROTECTION_INFO_INTERNALS_UNSUPPORTED         0
#define LUN_UNIT_ATTENTIONS_CLEARED                   1
#define REFERRALS_UNSUPPORTED                         0
#define CAPABILITY_BASED_SECURITY_UNSUPPORTED         0
#define MICROCODE_DOWNLOAD_VENDOR_SPECIFIC            0

/* SCSI READ/WRITE Defines */
#define WRITE_CDB_WP_OFFSET                           1
#define WRITE_CDB_WP_MASK                          0xE0
#define WRITE_CDB_WP_SHIFT                            5
#define WRITE_CDB_FUA_OFFSET                          1
#define WRITE_CDB_FUA_MASK                          0x8
#define WRITE_6_CDB_LBA_OFFSET                        1
#define WRITE_6_CDB_LBA_MASK                 0x001FFFFF
#define WRITE_6_CDB_TX_LEN_OFFSET                     4
#define WRITE_10_CDB_LBA_OFFSET                       2
#define WRITE_10_CDB_TX_LEN_OFFSET                    7
#define WRITE_10_CDB_WP_OFFSET                        1
#define WRITE_10_CDB_WP_MASK                       0xE0
#define WRITE_10_CDB_DPO_OFFSET                       1
#define WRITE_10_CDB_DPO_MASK                      0x10
#define WRITE_10_CDB_FUA_OFFSET                       1
#define WRITE_10_CDB_FUA_MASK                       0x8
#define WRITE_10_CDB_FUA_NV_OFFSET                    1
#define WRITE_10_CDB_FUA_NV_MASK                    0x2
#define WRITE_10_CDB_GROUP_NUM_OFFSET                 6
#define WRITE_10_CDB_GROUP_NUM_MASK                0x1F
#define WRITE_12_CDB_LBA_OFFSET                       2
#define WRITE_12_CDB_TX_LEN_OFFSET                    6
#define WRITE_12_CDB_WP_OFFSET                        1
#define WRITE_12_CDB_WP_MASK                       0xE0
#define WRITE_12_CDB_FUA_OFFSET                       1
#define WRITE_12_CDB_FUA_MASK                       0x8
#define WRITE_16_CDB_LBA_OFFSET                       2
#define WRITE_16_CDB_TX_LEN_OFFSET                   10
#define WRITE_16_CDB_FUA_OFFSET                       1
#define WRITE_PROTECTION_CODE_0                       0
#define WRITE_PROTECTION_CODE_1                       1
#define WRITE_PROTECTION_CODE_2                       2
#define WRITE_PROTECTION_CODE_3                       3
#define WRITE_PROTECTION_CODE_4                       4
#define WRITE_PROTECTION_CODE_5                       5
#define READ_CDB_RP_OFFSET                            1
#define READ_CDB_RP_MASK                           0xE0
#define READ_CDB_RP_SHIFT                             5
#define READ_CDB_FUA_OFFSET                           1
#define READ_CDB_FUA_MASK                           0x8
#define READ_6_CDB_LBA_OFFSET                         1
#define READ_6_CDB_TX_LEN_OFFSET                      4
#define READ_6_CDB_LBA_MASK                  0x001FFFFF
#define READ_10_CDB_LBA_OFFSET                        2
#define READ_10_CDB_TX_LEN_OFFSET                     7
#define READ_10_CDB_FUA_OFFSET                        1
#define READ_12_CDB_LBA_OFFSET                        2
#define READ_12_CDB_TX_LEN_OFFSET                     6
#define READ_16_CDB_LBA_OFFSET                        2
#define READ_16_CDB_TX_LEN_OFFSET                    10
#define READ_16_CDB_FUA_OFFSET                        1
#define READ_PROTECTION_CODE_0                        0
#define READ_PROTECTION_CODE_1                        1
#define READ_PROTECTION_CODE_2                        2
#define READ_PROTECTION_CODE_3                        3
#define READ_PROTECTION_CODE_4                        4
#define READ_PROTECTION_CODE_5                        5
#define READ_WRITE_6_MAX_LBA                        256

/* Security Protocol In/Out Defines */
#define SECURITY_PROTOCOL_CDB_SEC_PROT_OFFSET         1
#define SECURITY_PROTOCOL_CDB_SEC_PROT_SP_OFFSET      2
#define SECURITY_PROTOCOL_CDB_INC_512_OFFSET          4
#define SECURITY_PROTOCOL_CDB_INC_512_MASK         0x80
#define SECURITY_PROTOCOL_CDB_INC_512_SHIFT           7
#define SECURITY_PROTOCOL_CDB_LENGTH_OFFSET           6
#define SECURITY_RECEIVE_SEND_DWORD_10_SECP_SHIFT    24
#define SECURITY_RECEIVE_SEND_DWORD_10_SPSP_SHIFT     8
#define SECURITY_RECEIVE_SEND_DWORD_11_TL_SHIFT      16

/* Persistent Reservation In/Out defines */
#define PERSISTENT_RES_CDB_SER_ACTION_OFFSET          1
#define PERSISTENT_RES_CDB_RTYPE_OFFSET               2
#define PERSISTENT_RES_CDB_RTYPE_MASK               0xF
#define PERSISTENT_RES_CDB_SCOPE_OFFSET               2
#define PERSISTENT_RES_CDB_SCOPE_MASK              0XF0
#define PERSISTENT_RESIN_CDB_ALLOC_LEN_OFFSET         7
#define PERSISTENT_RESOUT_CDB_ALLOC_LEN_OFFSET        5
#define PERSISTENT_RESOUT_LU_SCOPE                    0

/* Persistent Reservation Actions*/
#define RESERVATION_ACTION_REPORT_CAPABILITIES        2
#define RESERVATION_ACTION_READ_FULL_STATUS           3
#define RESERVATION_ACTION_REGISTER_AND_MOVE          7

/* Persistent Reservation Types */
#define RESERVATION_TYPE_NONE                         0
#define RESERVATION_TYPE_WRITE_EXCLUSIVE_ALL          7
#define RESERVATION_TYPE_EXCLUSIVE_ACCESS_ALL         8
#define RESERVATION_TYPE_MAX_VALUE                    8

/* Mode Sense/Select defines */
#define MODE_PAGE_READ_WRITE_ERROR_RECOVERY        0x01
#define MODE_PAGE_DISCONNECT_RECONNECT             0x02
#define MODE_PAGE_PROTOCOL_SPECIFIC_PORT           0x19
#define MODE_PAGE_INFORMATIONAL_EXCEPTIONS_CONTROL 0x1C
#define MODE_PAGE_RETURN_ALL                       0x3F
#define MODE_SENSE_6_PAGE_08_SIZE                    32
#define MODE_SENSE_6_PAGE_08_SIZE_DBD                24
#define MODE_SENSE_6_PAGE_1C_SIZE                    24
#define MODE_SENSE_6_PAGE_1C_SIZE_DBD                16
#define MODE_SENSE_6_PAGE_3F_SIZE                    44
#define MODE_SENSE_6_PAGE_3F_SIZE_DBD                36
#define MODE_SENSE_6_CDB_ALLOC_LENGTH_OFFSET          4
#define MODE_SENSE_CDB_PAGE_CONTROL_OFFSET            2
#define MODE_SENSE_CDB_PAGE_CONTROL_MASK           0xC0
#define MODE_SENSE_CDB_PAGE_CONTROL_SHIFT             6
#define MODE_SENSE_CDB_PAGE_CODE_OFFSET               2
#define MODE_SENSE_CDB_PAGE_CODE_MASK              0x3F
#define MODE_SENSE_CDB_SUBPAGE_CODE_OFFSET            3
#define MODE_SENSE_CDB_LLBAA_OFFSET                   1
#define MODE_SENSE_CDB_LLBAA_MASK                  0x10
#define MODE_SENSE_CDB_LLBAA_SHIFT                    4
#define MODE_SENSE_CDB_DBD_OFFSET                     1
#define MODE_SENSE_CDB_DBD_MASK                       8
#define MODE_SENSE_CDB_DBD_SHIFT                      3

#define MODE_SENSE_10_PAGE_08_SIZE                   36
#define MODE_SENSE_10_PAGE_08_SIZE_DBD               28
#define MODE_SENSE_10_PAGE_1C_SIZE                   28
#define MODE_SENSE_10_PAGE_1C_SIZE_DBD               20
#define MODE_SENSE_10_PAGE_3F_SIZE                   48
#define MODE_SENSE_10_PAGE_3F_SIZE_DBD               40
#define MODE_SENSE_10_CDB_ALLOC_LENGTH_OFFSET         7
#define MODE_SENSE_10_CDB_PAGE_CODE_OFFSET            2
#define MODE_SENSE_10_CDB_PAGE_CODE_MASK           0x3F
#define MODE_SENSE_10_CDB_DBD_OFFSET                  1
#define MODE_SENSE_10_CDB_DBD_MASK                    8

#define MODE_SELECT_CDB_PAGE_FORMAT_OFFSET            1
#define MODE_SELECT_CDB_SAVE_PAGES_OFFSET             1
#define MODE_SELECT_6_CDB_PARAM_LIST_LENGTH_OFFSET    4
#define MODE_SELECT_10_CDB_PARAM_LIST_LENGTH_OFFSET   7
#define MODE_SELECT_CDB_PAGE_FORMAT_MASK           0x10
#define MODE_SELECT_CDB_SAVE_PAGES_MASK             0x1
#define MODE_SELECT_PAGE_FORMAT_STANDARD              1
#define MODE_SELECT_SAVE_PAGES_ENABLED                1

#define MODE_SENSE_CDB_BLOCK_DESC_ENABLED             0
#define CACHE_MODE_PAGE_PARAM_SAVEABLE_ENABLED        1
#define MODE_PAGE_PARAM_SAVEABLE_ENABLED              1
#define MODE_PAGE_PARAM_SAVEABLE_DISABLED             0
#define MODE_SELECT_PAGE_CODE_MASK                 0x3F
#define ONE_TASK_SET                                0x0
#define ACA_UNSUPPORTED                               0
#define PROT_INFO_DISABLED_RDPROTECT_0                1
#define SENSE_DATA_DESC_FORMAT                        1
#define LOG_PARMS_NOT_IMPLICITLY_SAVED_PER_LUN        1
#define LOG_EXCP_COND_NOT_REPORTED                    0
#define CMD_REORDERING_SUPPORTED                      1
#define UA_NOT_SUPPORTED_ON_PR_RELEASE                1
#define BUSY_RETURNS_ENABLED                          0
#define UA_CLEARED_AT_CC_STATUS                     0x0
#define SW_WRITE_PROTECT_UNSUPPORTED                  0
#define LBAT_LBRT_MODIFIABLE                          0
#define LBAT_LBRT_NOT_MODIFIABLE                      1
#define TASK_ABORTED_STATUS_FOR_ABORTED_CMDS          1
#define MEDIUM_LOADED_FULL_ACCESS                   0x0
#define UNLIMITED_BUSY_TIMEOUT_HIGH                0xFF
#define UNLIMITED_BUSY_TIMEOUT_LOW                 0xFF
#define SMART_SELF_TEST_UNSUPPORTED_HIGH           0x00
#define SMART_SELF_TEST_UNSUPPORTED_LOW            0x00
#define LONG_LBA_MASK                               0x1
#define SHORT_DESC_BLOCK                              8
#define LONG_DESC_BLOCK                              16
#define POWER_COND_MODE_PAGE_LENGTH                0x26
#define INFO_EXCP_MODE_PAGE_LENGTH                 0x0A
#define CACHING_MODE_PAGE_LENGTH                   0x12
#define SUB_PAGE_CODE_NONE                         0x00
#define CONTROL_MODE_PAGE_SIZE                     0x0A
#define BLOCK_DESCRIPTORS_ENABLED                     0
#define MODE_SENSE_PC_CURRENT_VALUES                  0
#define MODE_SENSE_PC_CHANGEABLE_VALUES               1
#define MODE_SENSE_PC_DEFAULT_VALUES                  2
#define MODE_SENSE_ALL_PAGES_LENGTH                0x64

/* Log Sense defines */
#define LOG_PAGE_SUPPORTED_LOG_PAGES_PAGE          0x00
#define LOG_PAGE_INFORMATIONAL_EXCEPTIONS_PAGE     0x2F
#define LOG_PAGE_TEMPERATURE_PAGE                  0x0D
#define LOG_SENSE_CDB_SP_OFFSET                       1
#define LOG_SENSE_CDB_SP_MASK                      0x01
#define LOG_SENSE_CDB_SP_NOT_ENABLED                  0
#define LOG_SENSE_CDB_PC_OFFSET                       2
#define LOG_SENSE_CDB_PC_MASK                      0xC0
#define LOG_SENSE_CDB_PC_SHIFT                        6
#define LOG_SENSE_CDB_PC_CUMULATIVE_VALUES            1
#define LOG_SENSE_CDB_PAGE_CODE_OFFSET                2
#define LOG_SENSE_CDB_PAGE_CODE_MASK               0x3F
#define LOG_SENSE_CDB_SUB_PAGE_CODE_OFFSET            3
#define LOG_SENSE_CDB_PARM_PTR_OFFSET                 5
#define LOG_SENSE_CDB_ALLOC_LENGTH_OFFSET             7
#define PC_CUMULATIVE_VALUES                          1
#define SUB_PAGE_FORMAT_UNSUPPORTED                   0
#define DISABLE_SAVE_UNSUPPORTED                      0
#define SUB_PAGE_CODE_UNSUPPORTED                  0x00
#define REMAINING_INFO_EXCP_PAGE_LENGTH             0x8
#define REMAINING_TEMP_PAGE_LENGTH                  0xC
#define GENERAL_PARAMETER_DATA                   0x0000
#define TEMPERATURE_PARM_CODE                      0x00
#define REFERENCE_TEMPERATURE_PARM_CODE            0x01
#define BINARY_FORMAT                               0x1
#define BINARY_FORMAT_LIST                          0x3
#define TMC_UNSUPPORTED                               0
#define ETC_UNSUPPORTED                               0
#define TSD_UNSUPPORTED                               0
#define LOG_PARAMETER_DISABLED                        1
#define DU_UNSUPPORTED                                0
#define INFO_EXCP_PARM_LENGTH                         4
#define INFO_EXCP_ASC_NONE                          0x0
#define INFO_EXCP_ASCQ_NONE                         0x0
#define TEMP_PARM_LENGTH                              2
#define REF_TEMP_PARM_LENGTH                          2
#define SUPPORTED_LOG_PAGES_PAGE_LENGTH             0x3

/* NVMe Namespace and Command Defines */
#define MAX_NON_HIDDEN_NAMESPACE_SCSI_TARGETS        16
#define MAX_NUMBER_OF_NAMESPACES                   1024
#define NAMESPACE_NAME_MAX_SIZE                     100
#define NAMESPACE_INVALID_MAP_INDEX                0xFF
#define DEFAULT_SECTOR_SIZE                         512
#define PRODUCT_ID_SIZE                              16
#define VENDOR_ID_SIZE                                8
#define PRODUCT_REVISION_LEVEL_SIZE                   4
#define FUSE_NORMAL_OPERATION                         0
#define NVME_FLUSH                                 0x00
#define NVME_WRITE                                 0x01
#define NVME_READ                                  0x02
#define NVME_FORMAT_NVM                            0x80
#define NVME_SECURITY_SEND                         0x81
#define NVME_SECURITY_RECEIVE                      0x82
#define NVME_GET_LOG_PAGE                          0x02
#define NVME_GET_FEATURES                          0x0A
#define VOLATILE_WRITE_CACHE_FEATURE                  0
#define VOLATILE_WRITE_CACHE_MASK                     1

/* Report LUNs defines */
#define REPORT_LUNS_FIRST_LUN_OFFSET                0x8

/**
 * DEVICE_SPECIFIC_PARAMETER in mode parametr header (see sbc2r16) to
 * enable DPOFUA support type 0x10 value.
 */
#define DEVICE_SPECIFIC_PARAMETER                     0
#define NIBBLES_PER_LONGLONG                         16
#define NIBBLES_PER_LONG                              8
#define LOWER_NIBBLE_MASK                           0xF
#define UPPER_NIBBLE_MASK                          0xF0

#endif /* __SNTI_TYPES_H__ */
