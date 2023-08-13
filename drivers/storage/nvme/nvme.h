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
 * File: nvme.h
 */

#ifndef __NVME_H__
#define __NVME_H__

/* MEMORY STRUCTURES */


#define NUM_LBAF (16)

/* Section 4.2, Figure 6 */
typedef struct _NVMe_COMMAND_DWORD_0
{
    /* [Opcode] This field indicates the opcode of the command to be executed */
    UCHAR    OPC;

    /*
     * [Fused Operation] In a fused operation, a complex command is created by
     * "fusing� together two simpler commands. Refer to section 6.1. This field
     * indicates whether this command is part of a fused operation and if so,
     * which command it is in the sequence. Value 00b Normal Operation, Value
     * 01b == Fused operation, first command, Value 10b == Fused operation,
     * second command, Value 11b == Reserved.
     */
    UCHAR    FUSE           :2;
    UCHAR    Reserved       :6;

    /*
     * [Command Identifier] This field indicates a unique identifier for the
     * command when combined with the Submission Queue identifier.
     */
    USHORT   CID;
} NVMe_COMMAND_DWORD_0, *PNVMe_COMMAND_DWORD_0;

/*
 * Section 4.2, Figure 7
 */
typedef struct _NVMe_COMMAND
{
    /*
     * [Command Dword 0] This field is common to all commands and is defined
     * in Figure 6.
     */
    NVMe_COMMAND_DWORD_0    CDW0;

     /*
      * [Namespace Identifier] This field indicates the namespace that this
      * command applies to. If the namespace is not used for the command, then
      * this field shall be cleared to 0h. If a command shall be applied to all
      * namespaces on the device, then this value shall be set to FFFFFFFFh.
      */
    ULONG                   NSID;

    /* DWORD 2, 3 */
    ULONGLONG               Reserved;

    /*
     * [Metadata Pointer] This field contains the address of a contiguous
     * physical buffer of metadata. This field is only used if metadata is not
     * interleaved with the LBA data, as specified in the Format NVM command.
     * This field shall be Dword aligned.
     */
    ULONGLONG               MPTR;

    /* [PRP Entry 1] This field contains the first PRP entry for the command. */
    ULONGLONG               PRP1;

    /*
     * [PRP Entry 2] This field contains the second PRP entry for the command.
     * If the data transfer spans more than two memory pages, then this field is
     * a PRP List pointer.
     */
    ULONGLONG               PRP2;

    /* [Command Dword 10] This field is command specific Dword 10. */
    union {
        ULONG               CDW10;
        /*
         * Defined in Admin and NVM Vendor Specific Command format.
         * Number of DWORDs in PRP, data transfer (in Figure 8).
         */
        ULONG               NDP;
    };

    /* [Command Dword 11] This field is command specific Dword 11. */
    union {
        ULONG               CDW11;
        /*
         * Defined in Admin and NVM Vendor Specific Command format.
         * Number of DWORDs in MPTR, Metadata transfer (in Figure 8).
         */
        ULONG               NDM;
    };

    /* [Command Dword 12] This field is command specific Dword 12. */
    ULONG                   CDW12;

    /* [Command Dword 13] This field is command specific Dword 13. */
    ULONG                   CDW13;

    /* [Command Dword 14] This field is command specific Dword 14. */
    ULONG                   CDW14;

    /* [Command Dword 15] This field is command specific Dword 15. */
    ULONG                   CDW15;
} NVMe_COMMAND, *PNVMe_COMMAND;

/* Section 4.3, Figure 9 */
typedef struct _NVMe_PRP_ENTRY
{
    ULONGLONG   Reserved    :2;

    /*
     * [Page Base Address and Offset] This field indicates the 64-bit physical
     * memory page address. The lower bits (n:2) of this field indicate the
     * offset within the memory page. If the memory page size is 4KB, then bits
     * 11:02 form the Offset; if the memory page size is 8KB, then bits 12:02
     * form the Offset, etc. If this entry is not the first PRP entry in the
     * command or in the PRP List then the Offset portion of this field shall be
     * cleared to 0h.
     */
    ULONGLONG   PBAO        :62;
} NVMe_PRP_ENTRY, *PNVMe_PRP_ENTRY;

/* Section 4.5, Figure 12 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2
{
    /*
     * [SQ Head Pointer] Indicates the current Submission Queue Head pointer for
     * the Submission Queue indicated in the SQ Identifier field. This is used
     * to indicate to the host Submission Queue entries that have been consumed
     * and may be re-used for new entries. Note: The value returned is the value
     * of the SQ Head pointer when the completion entry was created. By the time
     * software consumes the completion entry, the controller may have an SQ
     * Head pointer that has advanced beyond the value indicated.
     */
    USHORT  SQHD;

    /*
     * [SQ Identifier] Indicates the Submission Queue that the associated
     * command was issued to. This field is used by software when more than one
     * Submission Queue shares a single Completion Queue to uniquely determine
     * the command completed in combination with the Command Identifier (CID).
     */
    USHORT  SQID;
} NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2, *PNVMe_COMPLETION_QUEUE_ENTRY_DWORD_2;

/* Section 4.5, Figure 13 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3
{
    /*
     * [Command Identifier] Indicates the identifier of the command that is
     * being completed. This identifier is assigned by host software when the
     * command is submitted to the Submission Queue. The combination of the SQ
     * Identifier and Command Identifier uniquely identifies the command that is
     * being completed. The maximum number of requests outstanding at one time
     * is 64K for an I/O Submission Queue and 4K for the Admin Submission Queue.
     */
    USHORT  CID;

    /*
     * [Status Field] Indicates status for the command that is being completed.
     * Refer to section 4.5.1.
     */
    struct
    {
        /*
         * [Phase Tag] Identifies whether a Completion Queue entry is new. The
         * Phase Tag values for all Completion Queue entries shall be
         * initialized to �0� by host software prior to setting CC.EN to �1�.
         * When the controller places an entry in the Completion Queue, it shall
         * invert the phase tag to enable host software to discriminate a new
         * entry. Specifically, for the first set of completion queue entries
         * after CC.EN is set to �1� all Phase Tags are set to �1� when they are
         * posted. For the second set of completion queue entries, when the
         * controller has wrapped around to the top of the Completion Queue, all
         * Phase Tags are cleared to �0� when they are posted. The value of the
         * Phase Tag is inverted each pass through the Completion Queue.
         */
        USHORT  P        :1;

        /* Section 4.5, Figure 14 */

        /*
         * [Status Code] Indicates a status code identifying any error or status
         * information for the command indicated.
         */
        USHORT  SC       :8;

        /*
         * [Status Code Type] Indicates teh status code type of the completion
         * entry. This indicates the type of status the controller is returning.
         */
        USHORT  SCT      :3;
        USHORT  Reserved :2;

        /*
         * [More] If set to �1�, there is more status information for this
         * command as part of the Error Information log that may be retrieved
         * with the Get Log Page command. If cleared to �0�, there is no
         * additional status information for this command. Refer to section
         * 5.10.1.1.
         */
        USHORT  M        :1;

        /*
         * [Do Not Retry] If set to �1�, indicates that if the same command is
         * re-issued it is expected to fail. If cleared to �0�, indicates that
         * the same command may succeed if retried. If a command is aborted due
         * to time limited error recovery (refer to section 5.12.1.5), this
         * field should be cleared to �0�.
         */
        USHORT  DNR      :1;
    } SF;
} NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3, *PNVMe_COMPLETION_QUEUE_ENTRY_DWORD_3;

/* Section 4.5, Figure 11 */
typedef struct _NVMe_COMPLETION_QUEUE_ENTRY
{
    ULONG                               DW0;
    ULONG                               Reserved;
    NVMe_COMPLETION_QUEUE_ENTRY_DWORD_2 DW2;
    NVMe_COMPLETION_QUEUE_ENTRY_DWORD_3 DW3;
} NVMe_COMPLETION_QUEUE_ENTRY, *PNVMe_COMPLETION_QUEUE_ENTRY;

/*Status Code Type (SCT), Section 4.5.1.1, Figure 15 */
#define GENERIC_COMMAND_STATUS                          0
#define COMMAND_SPECIFIC_ERRORS                         1
#define MEDIA_ERRORS                                    2

/*Status Code - Generic Command Status Values, Section 4.5.1.2.1, Figure 16 */

/* The command completed successfully. */
#define SUCCESSFUL_COMPLETION                           0x0

/* The associated command opcode field is not valid. */
#define INVALID_COMMAND_OPCODE                          0x1

/* An invalid field specified in the command parameters. */
#define INVALID_FIELD_IN_COMMAND                        0x2

/*
 * The command identifier is already in use. Note:  It is implementation
 * specific how many commands are searched for a conflict.
 */
#define COMMAND_ID_CONFLICT                             0x3

/* Transferring the data or metadata associated with a command had an error. */
#define DATA_TRANSFER_ERROR                             0x4

/* Indicates that the commands are aborted due to a power loss notification. */
#define COMMANDS_ABORTED_DUE_TO_POWER_LOSS_NOTIFICATION 0x5

/*
 * The command was not completed successfully due to an internal device error.
 * Details on the internal device error are returned as an asynchronous event.
 * Refer to section 5.2.
 */
#define INTERNAL_DEVICE_ERROR                           0x6

/*
 * The command was aborted due to a Command Abort command being received that
 * specified the Submission Queue ID and Command ID of this command.
 */
#define COMMAND_ABORT_REQUESTED                         0x7

/*
 * The command was aborted due to a Delete I/O Submission Queue request received
 * for the SQ that the command was issued to.
 */
#define COMMAND_ABORTED_DUE_TO_SQ_DELETION              0x8

/*
 * The command was aborted due to the other command in a fused operation
 * failing.
 */
#define COMMAND_ABORTED_DUE_TO_FAILED_FUSED_COMMAND     0x9

/*
 * The command was aborted due to the companion fused command not being found as
 * the subsequent SQ entry.
 */
#define COMMAND_ABORTED_DUE_TO_MISSING_FUSED_COMMAND    0xA

/* The namespace or the format of that namespace is invalid. */
#define INVALID_NAMESPACE_OR_FORMAT                     0xB

/*
 * Status Code - Generic Command Status Values, NVM Command Set
 *
 * Section 4.5.1.2.1, Figure 17
 */

/* The command references an LBA that exceeds the size of the namespace. */
#define LBA_OUT_OF_RANGE                                0x80

/*
 * Execution of the command has caused the capacity of the namespace to be
 * exceeded.
 */
#define CAPACITY_EXCEEDED                               0x81

/*
 * The namespace is not ready to be accessed. The Do Not Retry bit indicates
 * whether re-issuing the command at a later time may succeed.
 */
#define NAMESPACE_NOT_READY                             0x82

/*Status Code - Command Specific Error Values, Section 4.5.1.2.2, Figure 18 */

/* Create I/O Submission Queue */
#define COMPLETION_QUEUE_INVALID                        0x0

/*
 * Create I/O Submission Queue, Create I/O Completion Queue, Delete I/O
 * Completion Queue, Delete I/O Submission Queue
 */
#define INVALID_QUEUE_IDENTIFIER                        0x1

/* Create I/O Submission Queue, Create I/O Completion Queue */
#define MAXIMUM_QUEUE_SIZE_EXCEEDED                     0x2

/* Abort */
#define ABORT_COMMAND_LIMIT_EXCEEDED                    0x3

/* Abort */
#define REQUESTED_COMMAND_TO_ABORT_NOT_FOUND            0x4

/* Asynchronous Event Request */
#define ASYNCHRONOUS_EVENT_REQUEST_LIMIT_EXCEEDED       0x5

/* Firmware Activate */
#define INVALID_FIRMWARE_SLOT                           0x6

/* Firmware Activate */
#define INVALID_FIRMWARE_IMAGE                          0x7

/* Create I/O Submission Queue */
#define INVALID_INTERRUPT_VECTOR                        0x8

/* Get Log Page */
#define INVALID_LOG_PAGE                                0x9

/* Format NVM */
#define INVALID_FORMAT                                  0xA

/* Firmware Activate */
#define FIRMWARE_APP_REQUIRES_CONVENTIONAL_RESET        0xB // NVMe1.0E

/* Delete I/O Completion Queue */
#define INVALID_QUEUE_DELETION                          0xC // NVMe1.0E


/*
 * Status Code - Command Specific Error Values, NVM Command Set
 *
 * Section 4.5.1.2.2, Figure 19
 */

/* Dataset Management, Read, Write */
#define CONFLICTING_ATTRIBUTES                          0x80

/*
 * Status Code - Media Error Values, NVM Command Set
 *
 * Section 4.5.1.2.3, Figure 21
 */

/*
 * The write data could not be committed to the media. This may be due to the
 * lack of available spare locations that is reported as an asynchronous event.
 */
#define WRITE_FAULT                                     0x80

/* The read data could not be recovered from the media. */
#define UNRECOVERED_READ_ERROR                          0x81

/* The command was aborted due to an end-to-end guard check failure. */
#define END_TO_END_GUARD_CHECK_ERROR                    0x82

/* The command was aborted due to an end-to-end application tag check failure */
#define END_TO_END_APPLICATION_TAG_CHECK_ERROR          0x83

/* The command was aborted due to an end-to-end reference tag check failure */
#define END_TO_END_REFERENCE_TAG_CHECK_ERROR            0x84

/* The command failed due to a miscompare during a Compare command. */
#define COMPARE_FAILURE                                 0x85

/*
 * Access to the namespace and/or LBA range is denied due to lack of access
 * rights. Refer to TCG SIIS.
 */
#define ACCESS_DENIED                                   0x86

/* NVMe Admin Command Set */
typedef struct _NVM_OPCODE
{
    UCHAR DataTransfer   :2;
    UCHAR Function       :5;
    UCHAR GenericCommand :1;
} NVM_OPCODE, *PNVM_OPCODE;

/*Opcodes for Admin Commands, Section 5, Figure 24 */
#define ADMIN_DELETE_IO_SUBMISSION_QUEUE                0x00
#define ADMIN_CREATE_IO_SUBMISSION_QUEUE                0x01
#define ADMIN_GET_LOG_PAGE                              0x02

#define ADMIN_DELETE_IO_COMPLETION_QUEUE                0x04
#define ADMIN_CREATE_IO_COMPLETION_QUEUE                0x05
#define ADMIN_IDENTIFY                                  0x06

#define ADMIN_ABORT                                     0x08
#define ADMIN_SET_FEATURES                              0x09
#define ADMIN_GET_FEATURES                              0x0A

#define ADMIN_ASYNCHRONOUS_EVENT_REQUEST                0x0C
#define ADMIN_NAMESPACE_MANAGEMENT                      0x0D

#define ADMIN_FIRMWARE_ACTIVATE                         0x10
#define ADMIN_FIRMWARE_IMAGE_DOWNLOAD                   0x11
#define ADMIN_NAMESPACE_ATTACHMENT                      0x15

/*Opcodes for Admin Commands, NVM Command Set Specific, Section 5, Figure 25 */
#define ADMIN_FORMAT_NVM                                0x80
#define ADMIN_SECURITY_SEND                             0x81
#define ADMIN_SECURITY_RECEIVE                          0x82

#define ADMIN_VENDOR_SPECIFIC_START                     0xC0
#define ADMIN_VENDOR_SPECIFIC_END                       0xFF

/* Delete I/O Submission Queue Command, Section 5.6, Figure 42, Opcode 0x00 */
typedef struct _ADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier of the Submission
     * Queue to be deleted. The value of 0h (Admin Submission Queue) shall not
     * be specified.
     */
    USHORT  QID;
    USHORT  Reserved;
} ADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10,
  *PADMIN_DELETE_IO_SUBMISSION_QUEUE_DW10;

/* Create I/O Submission Queue Command, Section 5.4, Figure 37, Opcode 0x01 */
typedef struct _ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier to assign to the
     * Submission Queue to be created. This identifier corresponds to the
     * Submission Queue Tail Doorbell used for this command (i.e., the value y).
     * This value shall not exceed the value reported in the Number of Queues
     * feature for I/O Submission Queues.
     */
    USHORT  QID;

    /*
     * [Queue Size] This field indicates the size of the Submission Queue to be
     * created. Refer to section 4.1.3. This is a 0�s based value.
     */
    USHORT  QSIZE;
} ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10,
  *PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW10;

/* Create I/O Submission Queue Command, Section 5.4, Figure 38 */
typedef struct _ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11
{
    /*
     * [Physically Contiguous] If set to �1�, then the Submission Queue is
     * physically contiguous and PRP Entry 1 (PRP1) is the address of a
     * contiguous physical buffer. If cleared to �0�, then the Submission Queue
     * is not physically contiguous and PRP Entry 1 (PRP1) is a PRP List
     * pointer.
     */
    ULONG   PC      :1;

    /*
     * [Queue Priority] This field indicates the priority service class to use
     * for commands within this Submission Queue. This field is only used when
     * the weighted round robin with an urgent priority service class is the
     * arbitration mechanism is selected. Refer to section 4.7. Value 00b ==
     * Urgent, Value 01b == High, Value 10b == Medium, Value 11b == Low.
     */
    ULONG   QPRIO   :2;
    ULONG   Reserved:13;

    /*
     * [Completion Queue Identifier] This field indicates the identifier of the
     * Completion Queue to utilize for any command completions entries
     * associated with this Submission Queue. The value of 0h (Admin Completion
     * Queue) shall not be specified.
     */
    ULONG   CQID    :16;
} ADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11,
  *PADMIN_CREATE_IO_SUBMISSION_QUEUE_DW11;

/* Get Log Page Command, Section 5.10.1, Figure 56, Opcode 0x02 */
typedef struct _ADMIN_GET_LOG_PAGE_COMMAND_DW10
{
    /*
     * [Log Page Identifier] This field specifies the identifier of the log page
     * to retrieve.
     */
    ULONG   LID      :8;
    ULONG   Reserved0:8;
    /*
     * [Number of Dwords] This field specifies the number of Dwords to return.
     * If host software indicates a size larger than the log page requested, the
     * results are undefined.
     */
    ULONG   NUMD    :12;
    ULONG   Reserved:4;
} ADMIN_GET_LOG_PAGE_COMMAND_DW10, *PADMIN_GET_LOG_PAGE_COMMAND_DW10;

/* Get Log Page - Log Identifiers, Section 5.10.1, Figure 57 */
#define ERROR_INFORMATION           0x01
#define SMART_HEALTH_INFORMATION    0x02
#define FIRMWARE_SLOT_INFORMATION   0x03

/*
 * Get Log Page - Error Information Log Entry
 *
 * Section 5.10.1.1, Figure 58, (Log Identifier 0x01)
 */
typedef struct _ADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY
{
    /*
     * This is a 64-bit incrementing error count, indicating a unique identifier
     * for this error. The error count starts at 1h, is incremented for each
     * unique error log entry, and is retained across power off conditions. A
     * value of 0h indicates an invalid entry; this value may be used when there
     * are lost entries or when there a fewer errors than the maximum number of
     * entries the controller supports.
     */
    ULONGLONG   ErrorCount;

    /*
     * This field indicates the Submission Queue Identifier of the command that
     * the error information is associated with.
     */
    USHORT      SubmissionQueueID;

    /*
     * This field indicates the Command Identifier of the command that the error
     * is assocated with.
     */
    USHORT      CommandID;

    /* This field indicates the Status that the command completed with. */
    struct
    {
        /* Phase Tag posted for the command */
        USHORT  PhaseTag   :1;
        /* The reported status for the completed command. */
        USHORT  Status     :15;
    } StatusField;
    /*
     * This field indicates the byte and bit of the command parameter that the
     * error is associated with, if applicable. If the parameter spans multiple
     * bytes or bits, then the location indicates the first byte and bit of the
     * parameter.
     */
    struct
    {
        /* Byte in command that contained the error. Valid values are 0 to 63 */
        USHORT  ByteInCommand   :8;

        /* Bit in command that contained the error. Valid values are 0 to7. */
        USHORT  BitInCommand    :3;
        USHORT  Reserved        :5;
    } ParameterErrorLocation;

    ULONGLONG   LBA;
    ULONG       Namespace;
    UCHAR       VendorSpecificInformationAvailable;
    UCHAR       Reserved[35];
} ADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_ERROR_INFORMATION_LOG_ENTRY;


/* Remove default compiler padding for SMART log page */
#pragma pack(push, smart_log, 1)

/*
 * Get Log Page - SMART/Health Information Log
 *
 * Section 5.10.1.2, Figure 59, (Log Identifier 0x02)
 */
typedef struct _NVM_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY
{
    /*
     * This field indicates critical warnings for the state of the controller.
     * Each bit corresponds to a critical warning type; multiple bits may be
     * set. If a bit is cleared to �0�, then that critical warning does not
     * apply. Critical warnings may result in an asynchronous event notification
     * to the host.
     */
    struct
    {

        /*
         * If set to �1�, then the available spare space has fallen below the
         * threshold.
         */
        UCHAR   AvailableSpaceBelowThreshold            :1;

        /*
         * If set to �1�, then the temperature has exceeded a critical
         * threshold.
         */
        UCHAR   TemperatureExceededCriticalThreshold    :1;

        /*
         * If set to �1�, then the device reliability has been degraded due to
         * significant media related errors or any internal error that degrades
         * device reliability.
         */
        UCHAR   DeviceReliablityDegraded                :1;

        /* If set to �1�, then the media has been placed in read only mode. */
        UCHAR   MediaInReadOnlyMode                     :1;

        /*
         * If set to �1�, then the volatile memory backup device has failed.
         * This field is only valid if the controller has a volatile memory
         * backup solution.
         */
        UCHAR   VolatileMemoryBackupDeviceFailed        :1;
        UCHAR   Reserved                                :3;
    } CriticalWarning;

    /*
     * Contains the temperature of the overall device (controller and NVM
     * included) in units of Kelvin. If the temperature exceeds the temperature
     * threshold, refer to section 5.12.1.4, then an asynchronous event may be
     * issued to the host.
     */
    USHORT      Temperature;

    /*
     * Contains a normalized percentage (0 to 100%) of the remaining spare
     * capacity available.
     */
    UCHAR       AvailableSpare;

    /*
     * When the Available Spare falls below the threshold indicated in this
     * field, an asynchronous event may be issued to the host. The value is
     * indicated as a normalized percentage (0 to 100%).
     */
    UCHAR       AvailableSpareThreshold;

    /*
     * Contains a vendor specific estimate of the percentage of device life used
     * based on the actual device usage and the manufacturer�s prediction of
     * device life. A value of 100 indicates that the estimated endurance of the
     * device has been consumed, but may not indicate a device failure. The
     * value is allowed to exceed 100. Percentages greater than 254 shall be
     * represented as 255. This value shall be updated once per power-on hour
     * (when the controller is not in a sleep state). Refer to the JEDEC JESD218
     * standard for SSD device life and endurance measurement techniques.
     */
    UCHAR       PercentageUsed;
    UCHAR       Reserved1[26];

    /*
     * Contains the number of 512 byte data units the host has read from the
     * controller; this value does not include metadata. This value is reported
     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
     * read) and is rounded up. When the LBA size is a value other than 512
     * bytes, the controller shall convert the amount of data read to 512 byte
     * units. For the NVM command set, logical
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } DataUnitsRead;

    /*
     * Contains the number of 512 byte data units the host has written to the
     * controller; this value does not include metadata. This value is reported
     * in thousands (i.e., a value of 1 corresponds to 1000 units of 512 bytes
     * written) and is rounded up. When the LBA size is a value other than 512
     * bytes, the controller shall convert the amount of data written to 512
     * byte units. For the NVM command set, logical blocks written as part of
     * Write operations shall be included in this value. Write Uncorrectable
     * commands shall not impact this value.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } DataUnitsWritten;

    /*
     * Contains the number of read commands issued to the controller. For the
     * NVM command set, this is the number of Compare and Read commands.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } HostReadCommands;

    /*
     * Contains the number of write commands issued to the controller. For the
     * NVM command set, this is the number of Write commands.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } HostWriteCommands;

    /*
     * Contains the amount of time the controller is busy with I/O commands. The
     * controller is busy when there is a command outstanding to an I/O Queue
     * (specifically, a command was issued via an I/O Submission Queue Tail
     * doorbell write and the corresponding completion entry has not been posted
     * yet to the associated I/O Completion Queue). This value is reported in
     * minutes.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } ControllerBusyTime;

    /* Contains the number of power cycles. */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } PowerCycles;

    /*
     * Contains the number of power-on hours. This does not include time that
     * the controller was powered and in a low power state condition.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } PowerOnHours;

    /*
     * Contains the number of unsafe shutdowns. This count is incremented when a
     * shutdown notification (CC.SHN) is not received prior to loss of power.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } UnsafeShutdowns;

    /*
     * Contains the number of occurrences where the controller detected an
     * unrecovered data integrity error. Errors such as uncorrectable ECC, CRC
     * checksum failure, or LBA tag mismatch are included in this field.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } MediaErrors;

    /*
     * Contains the number of Error Information log entries over the life of the
     * controller.
     */
    struct
    {
        ULONGLONG Lower;
        ULONGLONG Upper;
    } NumberofErrorInformationLogEntries;

    UCHAR Reserved2[320];
} ADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_SMART_HEALTH_INFORMATION_LOG_ENTRY;
#pragma pack(pop, smart_log)

/*
 * Get Log Page - Firmware Slot Information
 *
 * Section 5.10.1.3, Figure 560, (Log Identifier 0x03)
 */
typedef struct _ADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY
{
    /*
     * [Active Firmware Info] Specifies information about the active firmware
     * revision.
     */
    struct
    {
        /*
         * Bits 2:0 indicates the firmware slot that is contains the actively
         * running firmware revision.
         */
        UCHAR FirmwareSlot  :2;
        UCHAR Reserved      :6;
    } AFI;

    UCHAR       Reserved1[7];

    /*
     * [Firmware Revision for Slot 1] Contains the revision of the firmware
     * downloaded to firmware slot 1. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS1;

    /*
     * [Firmware Revision for Slot 2] Contains the revision of the firmware
     * downloaded to firmware slot 2. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS2;

    /*
     * [Firmware Revision for Slot 3] Contains the revision of the firmware
     * downloaded to firmware slot 3. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS3;

    /*
     * [Firmware Revision for Slot 4] Contains the revision of the firmware
     * downloaded to firmware slot 4. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS4;

    /*
     * [Firmware Revision for Slot 5] Contains the revision of the firmware
     * downloaded to firmware slot 5. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS5;

    /*
     * [Firmware Revision for Slot 6] Contains the revision of the firmware
     * downloaded to firmware slot 6. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS6;

    /*
     * [Firmware Revision for Slot 7] Contains the revision of the firmware
     * downloaded to firmware slot 7. If no valid firmware revision is present
     * or if this slot is unsupported, all zeros shall be returned.
     */
    ULONGLONG   FRS7;
    UCHAR       Reserved2[448];
} ADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY,
  *PADMIN_GET_LOG_PAGE_FIRMWARE_SLOT_INFORMATION_LOG_ENTRY;

/* Delete I/O Completion Queue Command, Section 5.5, Figure 40, Opcode 0x04 */
typedef struct _ADMIN_DELETE_IO_COMPLETION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier of the Completion
     * Queue to be deleted. The value of 0h (Admin Completion Queue) shall not
     * be specified.
     */
    USHORT  QID;
    USHORT  Reserved;
} ADMIN_DELETE_IO_COMPLETION_QUEUE_DW10,
  *PADMIN_DELETE_IO_COMPLETION_QUEUE_DW10;

/* Create I/O Completion Queue Command, Section 5.3, Figure 33, Opcode 0x05 */
typedef struct _ADMIN_CREATE_IO_COMPLETION_QUEUE_DW10
{
    /*
     * [Queue Identifier] This field indicates the identifier to assign to the
     * Completion Queue to be created. This identifier corresponds to the
     * Completion Queue Head Doorbell used for this command (i.e., the value y).
     * This value shall not exceed the value reported in the Number of Queues
     * feature for I/O Completion Queues.
     */
    USHORT  QID;

    /*
     * [Queue Size] This field indicates the size of the Completion Queue to be
     * created. Refer to section 4.1.3. This is a 0�s based value.
     */
    USHORT  QSIZE;
} ADMIN_CREATE_IO_COMPLETION_QUEUE_DW10,
  *PADMIN_CREATE_IO_COMPLETION_QUEUE_DW10;

/* Create I/O Completion Queue Command, Section 5.3, Figure 34 */
typedef struct _ADMIN_CREATE_IO_COMPLETION_QUEUE_DW11
{
    /*
     * [Physically Contiguous] If set to �1�, then the Completion Queue is
     * physically contiguous and PRP Entry 1 (PRP1) is the address of a
     * contiguous physical buffer. If cleared to �0�, then the Completion Queue
     * is not physically contiguous and PRP Entry 1 (PRP1) is a PRP List
     * pointer.
     */
    ULONG   PC       :1;

    /*
     * [Interrupts Enabled] If set to �1�, then interrupts are enabled for this
     * Completion Queue. If cleared to �0�, then interrupts are disabled for
     * this Completion Queue.
     */
    ULONG   IEN      :1;
    ULONG   Reserved :14;

    /*
     * [Interrupt Vector] This field indicates interrupt vector to use for this
     * Completion Queue. This corresponds to the MSI-X or multiple message MSI
     * vector to use. If using single message MSI or pin-based interrupts, then
     * this field shall be cleared to 0h. In MSI-X, a maximum of 2K vectors are
     * used. This value shall not be set to a value greater than the number of
     * messages the controller supports (refer to MSICAP.MC.MME or
     * MSIXCAP.MXC.TS).
     */
    ULONG   IV       :16;
} ADMIN_CREATE_IO_COMPLETION_QUEUE_DW11,
  *PADMIN_CREATE_IO_COMPLETION_QUEUE_DW11;

/* Identify Command, Section 5.11, Figure 64, Opcode 0x06 */
typedef struct _ADMIN_IDENTIFY_COMMAND_DW10
{
    /*
     * [Controller or Namespace Structure] If set to �1�, then the Identify
     * Controller data structure is returned to the host. If cleared to �0�,
     * then the Identify Namespace data structure is returned to the host for
     * the namespace specified in the command header.
     *
     * NVMe Spec 1.2: This field specifies the information to be returned to
     * the host.  Refer to Figure 86.
     */
    ULONG   CNS      :8;
    ULONG   Reserved :8;
    /*
     * Controller Identifier (CNTID):  This field specifies the controller
     * identifier used as part of some Identify operations. If the field is
     * not used as part of the Identify operation, then host software shall
     * clear this field to 0h. Controllers that support Namespace Management
     * shall support this field.
     */
    ULONG	CNTID	 :16;
} ADMIN_IDENTIFY_COMMAND_DW10, *PADMIN_IDENTIFY_COMMAND_DW10;

/* Identify - Power State Descriptor Data Structure, Section 5.11, Figure 66 */
typedef struct _ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR
{
    /*
     * [Maximum Power] This field indicates the maximum power consumed by the
     * NVM subsystem in this power state. The power in Watts is equal to the
     * value in this field multiplied by 0.01.
     */
    USHORT  MP;
    USHORT  Reserved1;

    /*
     * [Entry Latency] This field indicates the maximum entry latency in
     * microseconds associated with entering this power state.
     */
    ULONG   ENLAT;

    /*
     * [Exit Latency] This field indicates the maximum exit latency in
     * microseconds associated with entering this power state.
     */
    ULONG   EXLAT;

    /*
     * [Relative Read Throughput] This field indicates the relative read
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher read throughput.
     */
    UCHAR   RRT       :5;
    UCHAR   Reserved2 :3;

    /*
     * [Relative Read Latency] This field indicates the relative read latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower read latency.
     */
    UCHAR   RRL       :5;
    UCHAR   Reserved3 :3;

    /*
     * [Relative Write Throughput] This field indicates the relative write
     * throughput associated with this power state. The value in this field
     * shall be less than the number of supported power states (e.g., if the
     * controller supports 16 power states, then valid values are 0 through 15).
     * A lower value means higher write throughput.
     */
    UCHAR   RWT       :5;
    UCHAR   Reserved4 :3;

    /*
     * Relative Write Latency] This field indicates the relative write latency
     * associated with this power state. The value in this field shall be less
     * than the number of supported power states (e.g., if the controller
     * supports 16 power states, then valid values are 0 through 15). A lower
     * value means lower write latency.
     */
    UCHAR   RWL       :5;
    UCHAR   Reserved5 :3;
    UCHAR   Reserved6[16];
} ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR,
  *PADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR;

/* Identify Controller Data Structure, Section 5.11, Figure 65 */
typedef struct _ADMIN_IDENTIFY_CONTROLLER
{
    /* Controller Capabiliites and Features */

    /*
     * [PCI Vendor ID] Contains the company vendor identifier that is assigned
     * by the PCI SIG. This is the same value as reported in the ID register in
     * section 2.1.1.
     */
    USHORT  VID;

    /*
     * [PCI Subsystem Vendor ID] Contains the company vendor identifier that is
     * assigned by the PCI SIG for the subsystem. This is the same value as
     * reported in the SS register in section 2.1.17.
     */
    USHORT  SSVID;

    /*
     * [Serial Number] Contains the serial number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements
     */
    UCHAR   SN[20];

    /*
     * [Model Number] Contains the model number for the NVM subsystem that is
     * assigned by the vendor as an ASCII string. Refer to section 7.7 for
     * unique identifier requirements.
     */
    UCHAR   MN[40];

    /*
     * [Firmware Revision] Contains the currently active firmware revision for
     * the NVM subsystem. This is the same revision information that may be
     * retrieved with the Get Log Page command, refer to section 5.10.1.3. See
     * section 1.8 for ASCII string requirements.
     */
    UCHAR   FR[8];

    /*
     * [Recommended Arbitration Burst] This is the recommended Arbitration Burst
     * size. Refer to section 4.7.
     */
    UCHAR   RAB;

    /*
     * IEEE OUI Identifier (IEEE): Contains the Organization Unique Identifier (OUI) for
     * the controller vendor. The OUI shall be a valid IEEE/RAC
     * ( assigned identifier that may be registered at
     * http://standards.ieee.org/develop/regauth/oui/public.html.
     * and Multi-Interface Capabilities
    */
    UCHAR IEEE[3];
    UCHAR MIC;
    /*
     *  Maximum Data Transfer Size(MDTS)
    */
    UCHAR MDTS;

    /*
    * Controller ID 
    */
    USHORT CNTLID;

    /*
    * Version
    */
    struct 
    {
        UCHAR Reserved;
        UCHAR MNR;
        USHORT MJR;
    } VER;

#define MAJOR_VER_1 0x01
#define MINOR_VER_0 0
#define MINOR_VER_1 0x01
#define MINOR_VER_2 0x02

    UCHAR Reserved1[172];

    /* Admin Command Set Attributes */

    /*
     * [Optional Admin Command Support] This field indicates the optional Admin
     * commands supported by the controller. Refer to section 5.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� then the controller supports the Security Send
         * and Security Receive commands. If cleared to �0� then the controller
         * does not support the Security Send and Security Receive commands.
         */
        USHORT  SupportsSecuritySendSecurityReceive     :1;

        /*
         * Bit 1 if set to �1� then the controller supports the Format NVM
         * command. If cleared to �0� then the controller does not support the
         * Format NVM command.
         */
        USHORT  SupportsFormatNVM                       :1;

        /*
         * Bit 2 if set to �1� then the controller supports the Firmware
         * Activate and Firmware Download commands. If cleared to �0� then the
         * controller does not support the Firmware Activate and Firmware
         * Download commands.
         */
        USHORT  SupportsFirmwareActivateFirmwareDownload:1;
        /*
         * Bit 3 if set to '1' then the controller supports the Namespace
         * Management and Namespace Attachment commands. If cleared to '0' then
         * then controller does not support the Namespace Management and
         * Namespace Attachment commands.
         */
        USHORT  SupportsNamespaceMgmtAndAttachment      :1;
        USHORT  Reserved                                :12;
    } OACS;

    /*
     * [Abort Command Limit] This field is used to convey the maximum number of
     * concurrently outstanding Abort commands supported by the controller (see
     * section 5.1). This is a 0�s based value. It is recommended that
     * implementations support a minimum of four Abort commands outstanding
     * simultaneously.
     */
    UCHAR ACL;

    /*
     * [Asynchronous Event Request Limit] This field is used to convey the
     * maximum number of concurrently outstanding Asynchronous Event Request
     * commands supported by the controller (see section 5.2). This is a 0�s
     * based value. It is recommended that implementations support a minimum of
     * four Asynchronous Event Request Limit commands oustanding simultaneously.
     */
    UCHAR   UAERL;

    /*
     * [Firmware Updates] This field indicates capabilities regarding firmware
     * updates. Refer to section 8.1 for more information on the firmware update
     * process.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� indicates that the first firmware slot (slot 1)
         * is read only. If cleared to �0� then the first firmware slot (slot 1)
         * is read/write. Implementations may choose to have a baseline read
         * only firmware image.
         */
        UCHAR   FirstFirmwareSlotReadOnly               :1;

        /*
         * Bits 3:1 indicate the number of firmware slots that the device
         * supports. This field shall specify a value between one and seven,
         * indicating that at least one firmware slot is supported and up to
         * seven maximum. This corresponds to firmware slots 1 through 7.
         */
        UCHAR   SupportedNumberOfFirmwareSlots          :3;
        UCHAR   Reserved                                :4;
    } FRMW;

    /*
     * [Log Page Attributes] This field indicates optional attributes for log
     * pages that are accessed via the Get Log Page command.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� then the controller supports the SMART / Health
         * information log page on a per namespace basis. If cleared to �0� then
         * the controller does not support the SMART / Health information log
         * page on a per namespace basis; the log page returned is global for
         * all namespaces.
         */
        UCHAR   SupportsSMART_HealthInformationLogPage  :1;
        UCHAR   Reserved                                :7;
    } LPA;

    /*
     * [Error Log Page Entries] This field indicates the number of Error
     * Information log entries that are stored by the controller. This field is
     * a 0�s based value.
     */
    UCHAR   ELPE;

    /*
     * [Number of Power States Support] This field indicates the number of
     * NVMHCI power states supported by the controller. This is a 0�s based
     * value. Refer to section 8.4. Power states are numbered sequentially
     * starting at power state 0. A controller shall support at least one power
     * state (i.e., power state 0) and may support up to 31 additional power
     * states (i.e., up to 32 total).
     */
    UCHAR NPSS;
    /*
     * Admin Vendor Specific Command Configuration (AVSCC): This field indicates
     * the configuration settings for admin vendor specific command handling.
     */
    UCHAR   AVSCC          :1;
    UCHAR   Reserved_AVSCC :7;
 
    /* 
    * Autonomous Power State Transition Attributes (APSTA): This field indicates the
    * attributes of the autonomous power state transition feature. 
    * Bits 7:1 are reserved. 
    * Bit 0 if set to ‘1’, then the controller supports autonomous power state transitions. If
    * cleared to ‘0’, then the controller does not support autonomous power state transitions
    */
    UCHAR   APSTA;

    /*
    * Warning Composite Temperature Threshold (WCTEMP): This field indicates the
    * minimum Composite Temperature field value (reported in the SMART / Health
    * Information log in Figure 194) that indicates an overheating condition during which
    * controller operation continues. Immediate remediation is recommended (e.g.,
    * additional cooling or workload reduction). The platform should strive to maintain a
    * composite temperature less than this value.
    * A value of 0h in this field indicates that no warning temperature threshold value is
    * reported by the controller. Implementations compliant to revision 1.2 or later of this
    * specification shall report a non-zero value in this field.
    * It is recommended that implementations report a value of 0157h in this field
    */
    USHORT  WCTEMP;

    /* 
    * Critical Composite Temperature Threshold (CCTEMP): This field indicates the 
    * minimum Composite Temperature field value (reported in the SMART / Health
    * Information log in Figure 194) that indicates a critical overheating condition (e.g., may
    * prevent continued normal operation, possibility of data loss, automatic device
    * shutdown, extreme performance throttling, or permanent damage).
    * A value of 0h in this field indicates that no critical temperature threshold value is
    * reported by the controller. Implementations compliant to revision 1.2 or later of this
    * specification shall report a non-zero value in this field.
    */
    USHORT CCTEMP;

    /*
    * Maximum Time for Firmware Activation (MTFA): Indicates the maximum time the
    * controller temporarily stops processing commands to activate the firmware image.
    * This field shall be valid if the controller supports firmware activation without a reset.
    * This field is specified in 100 millisecond units. A value of 0h indicates that the maximum
    * time is undefined
    */
    USHORT MTFA;

    /*
    * Host Memory Buffer Preferred Size (HMPRE): This field indicates the preferred size
    * that the host is requested to allocate for the Host Memory Buffer feature in 4 KiB units.
    * This value shall be greater than or equal to the Host Memory Buffer Minimum Size. If
    * this field is non-zero, then the Host Memory Buffer feature is supported. If this field is
    * cleared to 0h, then the Host Memory Buffer feature is not supported
    */
    UINT32 HMPRE;

    /*
    * Host Memory Buffer Minimum Size (HMMIN): This field indicates the minimum size
    * that the host is requested to allocate for the Host Memory Buffer feature in 4 KiB units.
    * If this field is cleared to 0h, then the host is requested to allocate any amount of host
    * memory possible up to the HMPRE value
    */
    UINT32 HMMIN;

    /*
    * Total NVM Capacity (TNVMCAP): This field indicates the total NVM capacity in the
    * NVM subsystem. The value is in bytes. This field shall be supported if the Namespace
    * Management capability (refer to section 8.12) is supported 
    */
    UINT64 TNVMCAP[2];

    /*
    * Unallocated NVM Capacity (UNVMCAP): This field indicates the unallocated NVM
    * capacity in the NVM subsystem. The value is in bytes. This field shall be supported if
    * the Namespace Management capability (refer to section 8.12) is supported
    */
    UINT64 UNVMCAP[2];

    UCHAR   Reserved2[200];
    /* NVM Command Set Attributes */

    /*
     * [Submission Queue Entry Size] This field defines the required and maximum
     * Submission Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Submission Queue Entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 6, corresponding to 64.
         */
        UCHAR   RequiredSubmissionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Submission Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required SQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumSubmissionQueueEntrySize         :4;
    } SQES;

    /*
     * [Completion Queue Entry Size] This field defines the required and maximum
     * Completion Queue entry size when using the NVM Command Set.
     */
    struct
    {
        /*
         * Bits 3:0 define the required Completion Queue entry size when using
         * the NVM Command Set. This is the minimum entry size that may be used.
         * The value is in bytes and is reported as a power of two (2^n). The
         * required value shall be 4, corresponding to 16.
         */
        UCHAR   RequiredCompletionQueueEntrySize        :4;

        /*
         * Bits 7:4 define the maximum Completion Queue entry size when using
         * the NVM Command Set. This value is larger than or equal to the
         * required CQ entry size. The value is in bytes and is reported as a
         * power of two (2^n).
         */
        UCHAR   MaximumCompletionQueueEntrySize         :4;
    } CQES;

    UCHAR   Reserved3[2];

    /*
     * [Number of Namespaces] This field defines the number of valid namespaces
     * present for the controller.  Namespaces shall be allocated in order
     * (starting with 1) and packed sequentially.
     */
    ULONG   NN;

    /*
     * [Optional NVM Command Support] This field indicates the optional NVM
     * commands supported by the controller. Refer to section 6.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� then the controller supports the Compare command.
         * If cleared to �0� then the controller does not support the Compare
         * command.
         */
        USHORT  SupportsCompare                         :1;

        /*
         * Bit 1 if set to �1� then the controller supports the Write
         * Uncorrectable command. If cleared to �0� then the controller does not
         * support the Write Uncorrectable command.
         */
        USHORT  SupportsWriteUncorrectable              :1;

        /*
         * Bit 2 if set to �1� then the controller supports the Dataset
         * Management command. If cleared to �0� then the controller does not
         * support the Dataset Management command.
         */
        USHORT  SupportsDataSetManagement               :1;

        /* 
        * Bit 3 if st to '1' then the controller support the Write Zeroes command.
        * If cleared to '0' then the controller does not support the Write Zeroes command.
        */
        USHORT SupportsWriteZeroes                      : 1;

        /* Bit 4 if set to '1' then the controller supports the Save field in the Set
        * Features command and the Select field in the Get Features command. If cleared 
        * to '0' then the controller does not support the Save field in the Set Features 
        * command and the Select field in the Get Features command 
        */
        USHORT SupportSetFeaturesSave                   : 1;

        /* Bit 5 if set to '1' then the controller supports reservations. If 
        * cleared to '0' then the controller does not support reservations. If the
        * controller supports reservations, then it shall support the following commands 
        * associated with reservations: Reservation Report, Reservation Register, 
        * Reservation Acquire, and Reservation Release. 
        */
        USHORT SupportsReservations                     : 1;

        USHORT  Reserved                                :10;
    } ONCS;

    /*
     * [Fused Operation Support] This field indicates the fused operations that
     * the controller supports. Refer to section 6.1.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� then the controller supports the Compare and
         * Write fused operation. If cleared to �0� then the controller does not
         * support the Compare and Write fused operation. Compare shall be the
         * first command in the sequence.
         */
        USHORT  SupportsCompare_Write                   :1;
        USHORT  Reserved                                :15;
    } FUSES;

    /*
     * [Format NVM Attributes] This field indicates attributes for the Format
     * NVM command.
     */
    struct
    {
        /*
         * Bit 0 indicates whether the format operation applies to all
         * namespaces or is specific to a particular namespace. If set to �1�,
         * then all namespaces shall be configured with the same attributes and
         * a format of any namespace results in a format of all namespaces. If
         * cleared to �0�, then the controller supports format on a per
         * namespace basis.
         */
        UCHAR   FormatAppliesToAllNamespaces            :1;

        /*
         * Bit 1 indicates whether secure erase functionality applies to all
         * namespaces or is specific to a particular namespace. If set to�1�,
         * then a secure erase of a particular namespace as part of a format
         * results in a secure erase of all namespaces. If cleared to �0�, then
         * a secure erase as part of a format is performed on a per namespace
         * basis.
         */
        UCHAR   SecureEraseAppliesToAllNamespaces       :1;

        /*
         * Bit 2 indicates whether cryptographic erase is supported as part of
         * the secure erase functionality. If set to �1�, then cryptographic
         * erase is supported. If cleared to �0�, then cryptographic erase is
         * not supported.
         */
        UCHAR   SupportsCryptographicErase              :1;
        UCHAR   Reserved                                :5;
    } FNA;

    /*
     * [Volatile Write Cache] This field indicates attributes related to the
     * presence of a volatile write cache in the implementation.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� indicates that a volatile write cache is present.
         * If cleared to �0�, a volatile write cache is not present. If a
         * volatile write cache is present, then the host may issue Flush
         * commands and control whether it is enabled with Set Features
         * specifying the Volatile Write Cache feature identifier. If a volatile
         * write cache is not present, the host shall not issue Flush commands
         * nor Set Features or Get Features with the Volatile Write Cache
         * identifier.
         */
        UCHAR   Present               :1;
        UCHAR   Reserved                                :7;
    } VWC;

    /*
     * [Atomic Write Unit Normal] This field indicates the atomic write size for
     * the controller during normal operation. This field is specified in
     * logical blocks and is a 0�s based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the
     * NVM with respect to other read or write operations. A value of FFh
     * indicates all commands are atomic as this is the largest command size. It
     * is recommended that implementations support a minimum of 128KB
     * (appropriately scaled based on LBA size).
     */
    USHORT  AWUN;

    /*
     * [Atomic Write Unit Power Fail] This field indicates the atomic write size
     * for the controller during a power fail condition. This field is specified
     * in logical blocks and is a 0�s based value. If a write is issued of this
     * size or less, the host is guaranteed that the write is atomic to the NVM
     * with respect to other read or write operations.
     */
    USHORT  AWUPF;
    /*
     * NVM Vendor Specific Command Configuration (NVSCC): This field indicates
     * the configuration settings for NVM vendor specific command handling.
     */
    UCHAR   NVSCC          :1;
    UCHAR   Reserved_NVSCC :7;
    UCHAR   Reserved4[173];
    /* I/O Command Set Attributes */
    UCHAR   Reserved5[1344];

    /* Power State Descriptors */

    /*
     * [Power State x Descriptor] This field indicates the characteristics of
     * power state x. The format of this field is defined in Figure 66.
     */
    ADMIN_IDENTIFY_POWER_STATE_DESCRIPTOR PSDx[32];

    /* Vendor Specific */

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR   VS[1024];
} ADMIN_IDENTIFY_CONTROLLER, *PADMIN_IDENTIFY_CONTROLLER;

/* CNS defines for NVMe 1.0 and 1.2 */
#define CNS_IDENTIFY_NAMESPACE		0x00
#define CNS_IDENTIFY_CNTLR			0x01
/* CNS defines for NVMe 1.2 */
#define CNS_LIST_ATTACHED_NAMESPACES 0x02
#define CNS_LIST_EXISTING_NAMESPACES 0x10

/*
 * Identify - LBA Format Data Structure, NVM Command Set Specific
 *
 * Section 5.11, Figure 68
 */
typedef struct _ADMIN_IDENTIFY_FORMAT_DATA
{
    /*
     * [Metadata Size] This field indicates the number of metadata bytes
     * provided per LBA based on the LBA Size indicated. The namespace may
     * support the metadata being transferred as part of an extended data LBA or
     * as part of a separate contiguous buffer. If end-to-end data protection is
     * enabled, then the first eight bytes or last eight bytes of the metadata
     * is the protection information.
     */
    USHORT  MS;

    /*
     * [LBA Data Size] This field indicates the LBA data size supported. The
     * value is reported in terms of a power of two (2^n). A value smaller than
     * 9 (i.e. 512 bytes) is not supported. If the value reported is 0h then the
     * LBA format is not supported / used.
     */
    UCHAR   LBADS;

    /*
     * [Relative Performance] This field indicates the relative performance of
     * the LBA format indicated relative to other LBA formats supported by the
     * controller. Depending on the size of the LBA and associated metadata,
     * there may be performance implications. The performance analysis is based
     * on better performance on a queue depth 32 with 4KB read workload. The
     * meanings of the values indicated are included in the following table.
     * Value 00b == Best performance. Value 01b == Better performance. Value 10b
     * == Good performance. Value 11b == Degraded performance.
     */
    UCHAR   RP       :2;
    UCHAR   Reserved :6;
} ADMIN_IDENTIFY_FORMAT_DATA, *PADMIN_IDENTIFY_FORMAT_DATA;

/* Identify Namespace Data Structure, Section 5.11, Figure 67 */
typedef struct _ADMIN_IDENTIFY_NAMESPACE
{
    /*
     * [Namespace Size] This field indicates the total size of the namespace in
     * logical blocks. A namespace of size n consists of LBA 0 through (n - 1).
     * The number of logical blocks is based on the formatted LBA size. This
     * field is undefined prior to the namespace being formatted. Note: The
     * creation of the namespace(s) and initial format operation are outside the
     * scope of this specification.
     */
    ULONGLONG                   NSZE;

    /*
     * [Namespace Capacity] This field indicates the maximum number of logical
     * blocks that may be allocated in the namespace at any point in time. The
     * number of logical blocks is based on the formatted LBA size. This field
     * is undefined prior to the namespace being formatted. This field is used
     * in the case of thin provisioning and reports a value that is smaller than
     * or equal to the Namespace Size. Spare LBAs are not reported as part of
     * this field. A value of 0h for the Namespace Capacity indicates that the
     * namespace is not available for use. A logical block is allocated when it
     * is written with a Write or Write Uncorrectable command. A logical block
     * may be deallocated using the Dataset Management command.
     */
    ULONGLONG                   NCAP;

    /*
     * [Namespace Utilization] This field indicates the current number of
     * logical blocks allocated in the namespace. This field is smaller than or
     * equal to the Namespace Capacity. The number of logical blocks is based on
     * the formatted LBA size. When using the NVM command set: A logical block
     * is allocated when it is written with a Write or Write Uncorrectable
     * command. A logical block may be deallocated using the Dataset Management
     * command.
     */
    ULONGLONG                   NUSE;

    /* [Namespace Features] This field defines features of the namespace. */
    struct
    {
        /*
         * Bit 0 if set to �1� indicates that the namespace supports thin
         * provisioning. Specifically, the Namespace Capacity reported may be
         * less than the Namespace Size. When this feature is supported and the
         * Dataset Management command is supported then deallocating LBAs shall
         * be reflected in the Namespace Utilization field. Bit 0 if cleared to
         * �0� indicates that thin provisioning is not supported and the
         * Namespace Size and Namespace Capacity fields report the same value.
         */
        UCHAR   SupportsThinProvisioning    :1;
        UCHAR   Reserved                    :7;
    } NSFEAT;

    /*
     * [Number of LBA Formats] This field defines the number of supported LBA
     * size and metadata size combinations supported by the namespace. LBA
     * formats shall be allocated in order (starting with 0) and packed
     * sequentially. This is a 0�s based value. The maximum number of LBA
     * formats that may be indicated as supported is 16. The supported LBA
     * formats are indicated in bytes 128 � 191 in this data structure. The
     * metadata may be either transferred as part of the LBA (creating an
     * extended LBA which is a larger LBA size that is exposed to the
     * application) or it may be transferred as a separate contiguous buffer of
     * data. The metadata shall not be split between the LBA and a separate
     * metadata buffer. It is recommended that software and controllers
     * transition to an LBA size that is 4KB or larger for ECC efficiency at the
     * controller. If providing metadata, it is recommended that at least 8
     * bytes are provided per logical block to enable use with end-to-end data
     * protection, refer to section 8.2.
     */
    UCHAR NLBAF;

    /*
     * [Formatted LBA Size] This field indicates the LBA size & metadata size
     * combination that the namespace has been formatted with.
     */
    struct
    {
        /*
         * Bits 3:0 indicates one of the 16 supported combinations indicated in
         * this data structure. This is a 0�s based value.
         */
        UCHAR   SupportedCombination        :4;

        /*
         * Bit 4 if set to �1� indicates that the metadata is transferred at the
         * end of the data LBA, creating an extended data LBA. Bit 4 if cleared
         * to 0� indicates that all of the metadata for a command is transferred
         * as a separate contiguous buffer of data.
         */
        UCHAR   SupportsMetadataAtEndOfLBA  :1;
        UCHAR   Reserved                    :3;
    } FLBAS;

    /*
     * [Metadata Capabilities] This field indicates the capabilities for
     * metadata.
     */
    struct
    {
        /*
         * Bit 0 if set to �1� indicates that the namespace supports the
         * metadata being transferred as part of an extended data LBA.
         * Specifically, the metadata is transferred as part of the data PRP
         * Lists. Bit 0 if cleared to �0� indicates that the namespace does not
         * support the metadata being transferred as part of an extended data
         * LBA.
         */
        UCHAR   SupportsMetadataAsPartOfLBA :1;

        /*
         * Bit 1 if set to �1� indicates the namespace supports the metadata
         * being transferred as part of a separate buffer that is specified in
         * the Metadata Pointer. Bit 1 if cleared to �0� indicates that the
         * controller does not support the metadata being transferred as part of
         * a separate buffer.
         */
        UCHAR   SupportsMetadataAsSeperate  :1;
        UCHAR   Reserved                    :6;
    } MC;

    /*
     * [End-to-end Data Protection Capabilities] This field indicates the
     * capabilities for the end-to-end data protection feature. Multiple bits
     * may be set in this field. Refer to section 8.3.
     */
    struct {
        /*
         * Bit 0 if set to �1� indicates that the namespace supports Protection
         * Information Type 1. Bit 0 if cleared to �0� indicates that the
         * namespace does not support Protection Information Type 1.
         */
        UCHAR   SupportsProtectionType1     :1;

        /*
         * Bit 1 if set to �1� indicates that the namespace supports Protection
         * Information Type 2. Bit 1 if cleared to �0� indicates that the
         * namespace does not support Protection Information Type 2.
         */
        UCHAR   SupportsProtectionType2     :1;

        /*
         * Bit 2 if set to �1� indicates that the namespace supports Protection
         * Information Type 3. Bit 2 if cleared to �0� indicates that the
         * namespace does not support Protection Information Type 3.
         */
        UCHAR   SupportsProtectionType3     :1;

        /*
         * Bit 3 if set to �1� indicates that the namespace supports protection
         * information transferred as the first eight bytes of metadata. Bit 3
         * if cleared to �0� indicates that the namespace does not support
         * protection information transferred as the first eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionFirst8    :1;

        /*
         * Bit 4 if set to �1� indicates that the namespace supports protection
         * information transferred as the last eight bytes of metadata. Bit 4 if
         * cleared to �0� indicates that the namespace does not support
         * protection information transferred as the last eight bytes of
         * metadata.
         */
        UCHAR   SupportsProtectionLast8     :1;
        UCHAR   Reserved                    :3;
    } DPC;

    /*
     * [End-to-end Data Protection Type Settings] This field indicates the Type
     * settings for the end-to-end data protection feature. Refer to section
     * 8.3.
     */
    struct
    {
        /*
         * Bits 2:0 indicate whether Protection Information is enabled and the
         * type of Protection Information enabled. The values for this field
         * have the following meanings: Value 000b == Protection information is
         * not enabled. Value 001b == Protection information is enabled, Type 1.
         * Value 010b == Protection information is enabled, Type 2. Value 011b
         * == Protection information is enabled, Type 3. Value 100b-111b ==
         * Reserved.
         */
        UCHAR   ProtectionEnabled           :3;

        /*
         * Bit 3 if set to �1� indicates that the protection information, if
         * enabled, is transferred as the first eight bytes of metadata. Bit 3
         * if cleared to 0� indicates that the protection information, if
         * enabled, is transferred as the last eight bytes of metadata.
         */
        UCHAR   ProtectionInFirst8          :1;
        UCHAR   Reserved                    :4;
    } DPS;

    /*
    * [Namespace Multi-path I/O and Namespace Sharing Capabilities (NMIC)]
    * This field specifies multi-path I/O and namespace sharing capabilities
    * of the namespace.
    */
    struct
    {
        /*
        * Bits 0, if set to 1, indicate whether the NVM namespace may be
        * accessible by two or more controllers in the NVM subsystem
        * (i.e., may be a shared namespace). If cleared to '0', then the
        * NVM namespace is a private namespace and may only be accessed by
        * the controller that returned this namespace data structure.
        */
        UCHAR   NamespaceSharing : 1;
        UCHAR   Reserved : 7;
    } NMIC;

    /*
    * [Reservation Capabilities (RESCAP)]
    * This field indicates the reservation capabilities of the namespace.
    * A value of 00h in this field indicates that reservations are not
    * supported by this namespace.
    */
    struct
    {
        /*
        * if set to �1� indicates that the namespace supports the Persist
        * Through Power Loss capability. If this bit is cleared to �0�,
        * then the namespace does not support the Persist Through Power
        * Loss Capability.
        */
        UCHAR   PTPLC : 1;
        /*
        * Bit 1 if set to �1� indicates that the namespace supports the
        * Write Exclusive reservation type. If this bit is cleared to �0�,
        * then the namespace does not support the reservation type.
        */
        UCHAR   WrEx : 1;
        /*
        * Bit 2 if set to �1� indicates that the namespace supports the
        * Exclusive Access reservation type. If this bit is cleared to �0�,
        * then the namespace does not support the reservation type.
        */
        UCHAR   ExAccess : 1;
        /*
        * Bit 3 if set to �1� indicates that the namespace supports the
        * Write Exclusive-Registrants Only reservation type. If this bit
        * is cleared to �0�, then the namespace does not support the
        * reservation type.
        */
        UCHAR   WrExReg : 1;
        /*
        * Bit 4 if set to �1� indicates that the namespace supports the
        * Exclusive Access-Registrants Only reservation type. If this bit
        * is cleared to �0�, then the namespace does not support the
        * reservation type.
        */
        UCHAR   ExAccessReg : 1;
        /*
        * Bit 5 if set to �1� indicates that the namespace supports the
        * Write Exclusive-All Registrants reservation type. If this bit
        * is cleared to �0�, then the namespace does not support the
        * reservation type.
        */
        UCHAR   WrExAllReg : 1;
        /*
        * Bit 6 if set to �1� indicates that the namespace supports the
        * Exclusive Access-All Registrants reservation type. If this bit
        * is cleared to �0�, then the namespace does not support the
        * reservation type.
        */
        UCHAR   ExAccessAllReg : 1;
        UCHAR   Reserved : 1;
    } RESCAP;

    UCHAR                       Reserved1[72];

    /* This field contains a 128-bit value that is globally unique and 
    *  assigned to the namespace when the namespace is created. This 
    *  field remains fixed throughout the life of the namespace and is 
    *  preserved across namespace and controller operations (e.g., controller 
    *  reset, namespace format, etc.).*/
    union
    {
        struct 
        {
            ULONGLONG VendorSpecExtId;
            UCHAR CompanyId[3];
            UCHAR ExtensionId[5];
        };
        struct
        {
            ULONGLONG UpperBytes;
            ULONGLONG LowerBytes;
        };
    } NGUID;
    

    /*
    * This field contains a 64-bit IEEE Extended Unique Identifier (EUI-64) 
    * that is globally unique and assigned to the namespace when the namespace
    * is created. This field remains fixed throughout the life of the namespace
    * and is preserved across namespace and controller operations 
    * (e.g., controller reset, namespace format, etc.).
	*/
	UCHAR                       EUI64[8];

    /*
     * [LBA Format x Support] This field indicates the LBA format x that is
     * supported by the controller. The LBA format field is defined in Figure
     * 68.
     */
    ADMIN_IDENTIFY_FORMAT_DATA  LBAFx[NUM_LBAF];
    UCHAR                       Reserved2[192];

    /*
     * [Vendor Specific] This range of bytes is allocated for vendor specific
     * usage.
     */
    UCHAR                       VS[3712];
} ADMIN_IDENTIFY_NAMESPACE, *PADMIN_IDENTIFY_NAMESPACE;

/* Abort Command, Section 5.1, Figure 26, Opcode 0x08 */
typedef struct _ADMIN_ABORT_COMMAND_DW10
{
    /*
     * [Command Identifier] This field specifies the command identifier of the
     * command to be aborted, that was specified in the CDW0.CID field within
     * the command itself.
     */
    USHORT  SQID;

    /*
     * [Submission Queue Identifier] This field specifies the identifier of the
     * Submission Queue that the command to be aborted is associated with.
     */
    USHORT CID;
} ADMIN_ABORT_COMMAND_DW10, *PADMIN_ABORT_COMMAND_DW10;

/* Get Features Command, Section 5.9, Figure 52, Opcode 0x09 */
typedef struct _ADMIN_GET_FEATURES_COMMAND_DW10
{
    /*
     * [Feature Identifier] This field indicates the identifier of the Feature
     * for which to provide data.
     */
    ULONG   FID      :8;
    ULONG   Reserved :24;
} ADMIN_GET_FEATURES_COMMAND_DW10, *PADMIN_GET_FEATURES_COMMAND_DW10;

/*
 * Get Features - Features Identifiers
 *
 * Section 5.9, Figure 53, Figure 54; Section 5.12.1, Figure 72, Figure 73
 */
#define FID_ARBITRATION                         0x01
#define FID_POWER_MANAGEMENT                    0x02
#define FID_LBA_RANGE_TYPE                      0x03
#define FID_TEMPERATURE_THRESHOLD               0x04
#define FID_ERROR_RECOVERY                      0x05
#define FID_VOLATILE_WRITE_CACHE                0x06
#define FID_NUMBER_OF_QUEUES                    0x07
#define FID_INTERRUPT_COALESCING                0x08
#define FID_INTERRUPT_VECTOR_CONFIGURATION      0x09
#define FID_WRITE_ATOMICITY                     0x0A
#define FID_ASYNCHRONOUS_EVENT_CONFIGURATION    0x0B
#define FID_SOFTWARE_PROGRESS_MARKER            0x80
#define FID_RESERVATION_PERSISTANCE             0x83

/* Set Features Command, Section 5.12, Figure 71, Opcode 0x0A */
typedef struct _ADMIN_SET_FEATURES_COMMAND_DW10
{
    /*
     * [Feature Identifier] his field indicates the identifier of the Feature
     * that attributes are being specified for.
     */
    ULONG   FID      :8;
    ULONG   Reserved :24;
} ADMIN_SET_FEATURES_COMMAND_DW10, *PADMIN_SET_FEATURES_COMMAND_DW10;

/*
 * Arbitration & Command Processing, Section 5.12.1.1, Figure 74, Feature
 * Identifier 00h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_ARBITRATION_DW11
{
    /*
     * [Arbitration Burst] Indicates the maximum number of commands that the
     * controller may launch at one time from a particular Submission Queue.
     * This value is specified as 2^n. A value of 111b indicates no limit. Thus,
     * the possible settings are 1, 2, 4, 8, 16, 32, 64, or no limit.
     */
    UCHAR   AB       :3;
    UCHAR   Reserved :5;

    /*
     * [Low Priority Weight] This field defines the number of commands that may
     * be executed from the low priority service class in each arbitration
     * round. This is a 0�s based value.
     */
    UCHAR   LPW;

    /*
     * [Medium Priority Weight] This field defines the number of commands that
     * may be executed from the medium priority service class in each
     * arbitration round. This is a 0�s based value.
     */
    UCHAR   MPW;

    /*
     * [High Priority Weight] This field defines the number of commands that may
     * be executed from the high priority service class in each arbitration
     * round. This is a 0�s based value.
     */
    UCHAR   HPW;
} ADMIN_SET_FEATURES_COMMAND_ARBITRATION_DW11,
  *PADMIN_SET_FEATURES_COMMAND_ARBITRATION_DW11;

/* Power Management, Section 5.12.1.2, Figure 75, Feature Identifier 01h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_POWER_MANAGEMENT_DW11
{
    /*
     * [Power State] This field indicates the new power state into which the
     * controller should transition. This power state shall be one supported by
     * the controller as indicated in the Number of Power States Supported
     * (NPSS) field in the Indentify Controller data structure. The behavior of
     * transitioning to a power state not supported by the controller is
     * undefined.
     */
    ULONG   PS      :5;
    ULONG   Reserved:27;
} ADMIN_SET_FEATURES_COMMAND_POWER_MANAGEMENT_DW11,
  *PADMIN_SET_FEATURES_COMMAND_POWER_MANAGEMENT_DW11;

/* LBA Range Type, Section 5.12.1.3, Figure 76, Feature Identifier 02h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11
{
    /*
     * [Number of LBA Ranges] This field indicates the number of LBA ranges
     * specified in this command. This is a 0�s based value.
     */
    ULONG   NUM      :6;
    ULONG   Reserved :26;
} ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11,
  *PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_DW11;

/*
 * LBA Range Type - Entry
 *
 * Section 5.12.1.3, Figure 77, Feature Identifier 03h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY
{
    /*
     * [Type] Identifies the Type of the LBA range. The Types are listed below.
     * Value 00h == Reserved.  Value 01h == Filesystem. Value 02h == RAID. Value
     * 03h == Cache. Value 04h == Page/swap file.  Value 05h-7Fh == Reserved.
     * Value 80h-FFh == Vendor Specific.
     */
    UCHAR       Type;

    /* Identifies attributes of the LBA range. Each bit defines an attribute. */
    struct
    {
        /*
         * If set to �1�, the LBA range may be overwritten. If cleared to �0�,
         * the area should not be overwritten.
         */
        UCHAR   Overwriteable   :1;

        /*
         * If set to �1�, the LBA range should be hidden from the OS / EFI /
         * BIOS. If cleared to �0�, the area should be visible to the OS / EFI
         * / BIOS.
         */
        UCHAR   Hidden          :1;
        UCHAR   Reserved        :6;
    } Attributes;

    UCHAR       Reserved1[14];

    /*
     * [Starting LBA] This field indicates the 64-bit address of the first LBA
     * that is part of this LBA range.
     */
    ULONGLONG   SLBA;

    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks that are part of this LBA range. This is a 0�s based value.
     */
    ULONGLONG   NLB;

    /*
     * [Unique Identifier] This field is a global unique identifier that
     * uniquely identifies the type of this LBA range. Well known Types may be
     * defined and are published on the NVMHCI website.
     */
    UCHAR       GUID[16];
    UCHAR       Reserved2[16];
} ADMIN_SET_FEATURES_LBA_COMMAND_RANGE_TYPE_ENTRY,
  *PADMIN_SET_FEATURES_COMMAND_LBA_RANGE_TYPE_ENTRY;

/* Temperature Threshold, Section 5.12.1.4, Figure 78, Feature Identifier 04h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_TEMPERATURE_THRESHOLD_DW11
{
    /*
     * [Temperature Threshold] Indicates the threshold for the temperature of
     * the overall device (controller and NVM included) in units of Kelvin.
     */
    ULONG   TMPTH    :16;
    ULONG   Reserved :16;
} ADMIN_SET_FEATURES_COMMAND_TEMPERATURE_THRESHOLD_DW11,
  *PADMIN_SET_FEATURES_COMMAND_TEMPERATURE_THRESHOLD_DW11;

/* Error Recovery, Section 5.12.1.5, Figure 79, Feature Identifier 05h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_ERROR_RECOVERY_DW11
{
    /*
     * [Time Limited Error Recovery] Indicates a limited retry timeout value in
     * 100 millisecond units. This applies to I/O (e.g. Read, Write, etc)
     * commands that indicate a time limit is required. A value of 0h indicates
     * that there is no timeout.
     */
    ULONG   TLER     :16;
    ULONG   Reserved :16;
} ADMIN_SET_FEATURES_ERROR_COMMAND_RECOVERY_DW11,
  *PADMIN_SET_FEATURES_COMMAND_ERROR_RECOVERY_DW11;

/* Volatile Write Cache, Section 5.12.1.6, Figure 80, Feature Identifier 06h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_VOLATILE_WRITE_CACHE_DW11
{
    /*
     * [Volatile Write Cache Enable] If set to �1�, then the volatile write
     * cache is enabled. If cleared to �0�, then the volatile write cache is
     * disabled.
     */
    ULONG   WCE      :1;
    ULONG   Reserved :31;
} ADMIN_SET_FEATURES_COMMAND_VOLATILE_WRITE_CACHE_DW11,
  *PADMIN_SET_FEATURES_COMMAND_VOLATILE_WRITE_CACHE_DW11;

/* Number of Queues, Section 5.12.1.7, Figure 81, Feature Identifier 07h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11
{
    /*
     * [Number of I/O Completion Queues Requested] Indicates the number of I/O
     * Completion Queues requested by software. This number does not include the
     * Admin Completion Queue. A minimum of one shall be requested, reflecting
     * that the minimum support is for one I/O Completion Queue. This is a 0�s
     * based value.
     */
    ULONG   NCQR     :16;

    /*
     * [Number of I/O Submission Queues Requested] Indicates the number of I/O
     * Submission Queues requested by software. This number does not include the
     * Admin Submission Queue. A minimum of one shall be requested, reflecting
     * that the minimum support is for one I/O Submission Queue. This is a 0�s
     * based value.
     */
    ULONG   NSQR     :16;
} ADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11,
  *PADMIN_SET_FEATURES_COMMAND_NUMBER_OF_QUEUES_DW11;

/* Number of Queues, Section 5.12.1.7, Figure 82 */
typedef struct _ADMIN_SET_FEATURES_COMPLETION_NUMBER_OF_QUEUES_DW0
{
    /*
     * [Number of I/O Completion Queues Allocated] Indicates the number of I/O
     * Completion Queues allocated by the controller. A minimum of one shall be
     * allocated, reflecting that the minimum support is for one I/O Completion
     * Queue. The value may be larger than the number requested by host
     * software. This is a 0�s based value.
     */
    ULONG   NCQA     :16;

    /*
     * [Number of I/O Submission Queues Allocated] Indicates the number of I/O
     * Submission Queues allocated by the controller. A minimum of one shall be
     * allocated, reflecting that the minimum support is for one I/O Submission
     * Queue. The value may be larger than the number requested by host
     * software. This is a 0�s based value.
     */
    ULONG   NSQA     :16;
} ADMIN_SET_FEATURES_COMPLETION_NUMBER_OF_QUEUES_DW0,
  *PADMIN_SET_FEATURES_COMPLETION_NUMBER_OF_QUEUES_DW0;

/* Interrupt Coalescing, Section 5.12.1.8, Figure 83, Feature Identifier 08h */
typedef struct _ADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11
{

    /*
     * [Aggregation Threshold] Specifies the desired minimum number of
     * completion queue entries to aggregate per interrupt vector before
     * signaling an interrupt to the host; the default value is 0h. This is a
     * 0�s based value.
     */
    ULONG   THR     :8;

    /*
     * [Aggregation Time] Specifies the recommended maximum time in 100
     * microsecond increments that a controller may delay an interrupt due to
     * interrupt coalescing. A value of 0h corresponds to no delay (i.e.,
     * disabling this capability). The controller may apply this time per
     * interrupt vector or across all interrupt vectors.
     */
    ULONG   TIME     :8;
    ULONG   Reserved :16;
} ADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11,
  *PADMIN_SET_FEATURES_COMMAND_INTERRUPT_COALESCING_DW11;

/*
 * Interrupt Vector Configuration
 *
 * Section 5.12.1.9, Figure 84, Feature Identifier 09h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_INTERRUPT_VECTOR_CONFIGURATION_DW11
{
    /*
     * [Interrupt Vector] This field indicates the interrupt vector for which
     * the configuration settings shall be applied.
     */
    ULONG   IV       :16;

    /*
     * [Coalescing Disable] If set to �1�, then any interrupt coalescing
     * settings shall not be applied for this interrupt vector. If cleared to
     * 0�, then interrupt coalescing settings apply for this interrupt vector.
     * By default, coalescing settings are enabled for each interrupt vector.
     * Interrupt coalescing is not supported for the Admin Completion Queue.
     */
    ULONG   CD       :1;
    ULONG   Reserved :15;
} ADMIN_SET_FEATURES_COMMAND_INTERRUPT_VECTOR_CONFIGURATION_DW11,
  *PADMIN_SET_FEATURES_COMMAND_INTERRUPT_VECTOR_CONFIGURATION_DW11;

/* Write Atomicity, Section 5.12.1.10, Figure 85, Feature Identifier 0Ah */
typedef struct _ADMIN_SET_FEATURES_COMMAND_WRITE_ATOMICITY_DW11
{
    /*
     * [Disable Normal] If set to �1�, then the host indicates that the atomic
     * write unit for normal operation is not required and that the controller
     * shall only honor the atomic write unit for power fail operations. If
     * cleared to 0�, the atomic write unit for normal operation shall be
     * honored by the controller.
     */
    ULONG   DN       :1;
    ULONG   Reserved :31;
} ADMIN_SET_FEATURES_COMMAND_WRITE_ATOMICITY_DW11,
  *PADMIN_SET_FEATURES_COMMAND_WRITE_ATOMICITY_DW11;

/*
 * Asynchronous Event Configuration
 *
 * Section 5.12.1.11, Figure 86, Feature Identifier 0Bh
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_ASYNCHRONOUS_EVENT_CONFIGURATION_DW11
{
    /*
     * [SMART / Health Critical Warnings:] This field determines whether an
     * asynchronous event notification is sent to the host for the
     * corresponding Critical Warning specified in the SMART / Health
     * Information Log (refer to Figure 59). If a bit is set to �1�, then an
     * asynchronous event notification is sent when the corresponding critical
     * warning bit is set to �1� in the SMART / Health Information Log. If a bit
     * is cleared to �0�, then an asynchronous event notification is not sent
     * when the corresponding critical warning bit is set to �1� in the SMART /
     * Health Information Log.
     */
    ULONG   SMART_HealthCriticalWarnings :8; // NVMe1.0E
    ULONG   Reserved                     :24; // NVMe1.0E
} ADMIN_SET_FEATURES_COMMAND_ASYNCHRONOUS_EVENT_CONFIGURATION_DW11,
  *PADMIN_SET_FEATURES_COMMAND_ASYNCHRONOUS_EVENT_CONFIGURATION_DW11;

/*
 * Software Progress Marker
 *
 * Section 5.12.1.12, Figure 87, Feature Identifier 80h
 */
typedef struct _ADMIN_SET_FEATURES_COMMAND_SOFTWARE_PROGRESS_MARKER_DW11
{
    /*
     * [Pre-boot Software Load Count] Indicates the load count of pre-boot
     * software. After successfully loading and initializing the controller,
     * pre-boot software should set this field to one more than the previous
     * value of the Pre-boot Software Load Count. If the previous value is 255
     * then the value should not be updated by pre-boot software (i.e., the
     * value does not wrap to 0). OS driver software should set this field to 0h
     * after the OS has successfully been initialized.
     */
    ULONG   PBSLC    :8;
    ULONG   Reserved :24;
} ADMIN_SET_FEATURES_COMMAND_SOFTWARE_PROGRESS_MARKER_DW11,
  *PADMIN_SET_FEATURES_COMMAND_SOFTWARE_PROGRESS_MARKER_DW11;

/*
* Reservation Persistence
*
* Feature Identifier 83h
*/
typedef struct _ADMIN_SET_FEATURES_COMMAND_RESERVATION_PERSISTENCE_DW11
{
    /*
    * [Persistence Through Power Loss] If set to '1', then
    * reservations and registrants persist across a power
    * loss. If cleared to '0', then reservations are released
    * and registrants are cleared on a power loss
    */
    ULONG   PTPL : 1;
    ULONG   Reserved : 31;
} ADMIN_SET_FEATURES_COMMAND_RESERVATION_PERSISTENCE_DW11,
*PADMIN_SET_FEATURES_COMMAND_RESERVATION_PERSISTENCE_DW11;

/* Asynchronous Event Request Command, Section 5.2.1, Figure 29, Opcode 0x0C */
typedef struct _ADMIN_ASYNCHRONOUS_EVENT_REQUEST_COMPLETION_DW0
{
    /*
     * Indicates the type of the asynchronous event. The Error status type
     * indicates an error condition for the controller. The SMART/Health status
     * type provides a controller or NVM health indication. More specific
     * information on the event is provided in the Asynchronous Event
     * Information field. Value 0h == Error status. Value 01h == SMART/Health
     * status. Value 02h-06h == Reserved. Value 07h == Vendor specific.
     */
    ULONG   AsynchronousEventType         :3;
    ULONG   Reserved1                     :5;

    /*
     * Refer to Figure 30 and Figure 31 for detailed information regarding the
     * asynchronous event.
     */
    ULONG   AsynchronousEventInformation  :8;

    /*
     * Indicates the log page associated with the asynchronous event. This log
     * page needs to be read by the host to clear the event.
     */
    ULONG   AssociatedLogPage             :8;
    ULONG   Reserved2                     :8;
} ADMIN_ASYNCHRONOUS_EVENT_REQUEST_COMPLETION_DW0,
  *PADMIN_ASYNCHRONOUS_EVENT_REQUEST_COMPLETION_DW0;

/* Firmware Activate Command, Section 5.7, Figure 44, Opcode 0x10 */
typedef struct _ADMIN_FIRMWARE_ACTIVATE_COMMAND_DW10
{
    /*
     * [Firmware Slot] Specifies the firmware slot that shall be used for the
     * Activate Action, if applicable. If the value specified is 0h, then the
     * controller shall choose the firmware slot (slot 1 � 7) to use for the
     * operation.
     */
    ULONG   FS       :3;

    /*
     * [Activate Action] This field indicates the action that is taken on the
     * image downloaded with the Firmware Image Download command or on a
     * previously downloaded and placed image. The actions are indicated in the
     * following table. Value 00b == Downloaded image replaces the image
     * indicated by the Firmware Slot field. This image is not activated. Value
     * 01b == Downloaded image replaces the image indicated by the Firmware Slot
     * field. This image is activated at the next reset. Value 10b == The image
     * indicated by the Firmware Slot field is activated at the next reset.
     * Value 11b == Reserved.
     */
    ULONG   AA       :2;
    ULONG   Reserved :27;
} ADMIN_FIRMWARE_ACTIVATE_COMMAND_DW10, *PADMIN_FIRMWARE_ACTIVATE_COMMAND_DW10;

/* Firmware Image Download Command, Section 5.8, Figure 48, Opcode 0x11 */
typedef struct _ADMIN_FIRMWARE_IMAGE_DOWNLOAD_COMMAND_DW10
{
    /*
     * [Number of Dwords] This field specifies the number of Dwords in the image
     */
    ULONG   NUMD;
} ADMIN_FIRMWARE_IMAGE_DOWNLOAD_COMMAND_DW10,
  *PADMIN_FIRMWARE_IMAGE_DOWNLOAD_COMMAND_DW10;

/* Format NVM Command, Section 5.13, Figure 88, Opcode 0x80 */
typedef struct _ADMIN_FORMAT_NVM_COMMAND_DW10
{
    /*
     * [LBA Format] This field specifies the LBA format to apply to the NVM
     * media. This corresponds to the LBA formats indicated in the Identify
     * command, refer to Figure 67 and Figure 68. Only supported LBA formats
     * shall be selected.
     */
    ULONG   LBAF     :4;

    /*
     * [Metadata Settings] This field is set to �1� if the metadata is
     * transferred as part of an extended data LBA. This field is cleared to 0�
     * if the metadata is transferred as part of a separate buffer. The metadata
     * may include protection information, based on the Protection Information
     * (PI) field.
     */
    ULONG   MS       :1;

    /*
     * [Protection Information] This field specifies whether end-to-end data
     * protection is enabled and the type of protection information. The values
     * for this field have the following meanings: Value 000b == Protection
     * information is not enabled. Value 001b == Protection information is
     * enabled, Type 1. Value 010b == Protection information is enabled, Type 2.
     * Value 011b == Protection information is enabled, Type 3. Value 100b-111b
     * == Reserved. When end-to-end data protected is enabled, the host shall
     * specify the appropriate protection information in the Read, Write, or
     * Compare commands.
     */
    ULONG   PI       :3;

    /*
     * [Protection Information Location] If set to �1� and protection
     * information is enabled, then protection information is transferred as the
     * first eight bytes of metadata. If cleared to �0� and protection
     * information is enabled, then protection information is transferred as the
     * last eight bytes of metadata.
     */
    ULONG   IPL      :1;

    /*
     * [Secure Erase Settings] This field specifies whether a secure erase
     * should be performed as part of the format and the type of the secure
     * erase operation. The erase applies to all user data, regardless of
     * location (e.g., within an exposed LBA, within a cache, within deallocated
     * LBAs, etc). Value 000b == No secure erase operation requested. Value 001b
     * == User Data Erase: All user data shall be erased, contents of the user
     * data after the erase is indeterminate (e.g., the user data may be zero
     * filled, one filled, etc). The controller may perform a cryptographic
     * erase when a User Data Erase is requested if all user data is encrypted.
     * Value 010b == Cryptographic Erase: All user data shall be erased
     * cryptographically. This is accomplished by deleting the encryption key.
     * Value 011b - 111b == Reserved.
     */
    ULONG   SES      :3;
    ULONG   Reserved :20;
} ADMIN_FORMAT_NVM_COMMAND_DW10, *PADMIN_FORMAT_NVM_COMMAND_DW10;

/* Security Send Command, Section 5.14, Figure 96, Opcode 0x81 */
typedef struct _ADMIN_SECURITY_SEND_COMMAND_DW10
{
    ULONG   Reserved    :8;

    /*
     * [SP Specific] The value of this field is specific to the Security
     * Protocol as defined in SPC-4.
     */
    ULONG   SPSP        :16;

    /*
     * [Security Protocol] This field indicates the security protocol as defined
     * in SPC-4. The controller shall fail the command with Invalid Parameter
     * indicated if an unsupported value of the Security Protocol is specified.
     */
    ULONG   SECP        :8;
} ADMIN_SECURITY_SEND_COMMAND_DW10, *PADMIN_SECURITY_SEND_COMMAND_DW10;

/* Security Send Command, Section 5.14, Figure 97 */
typedef struct _ADMIN_SECURITY_SEND_COMMAND_DW11
{
    /*
     * [Transfer Length] The value of this field is specific to the Security
     * Protocol as defined in SPC-4.
     */
    ULONG   AL;
} ADMIN_SECURITY_SEND_COMMAND_DW11, *PADMIN_SECURITY_SEND_COMMAND_DW11;

/* Security Receive Command, Section 5.14, Figure 92, Opcode 0x82 */
typedef struct _ADMIN_SECURITY_RECEIVE_COMMAND_DW10
{
    ULONG   Reserved    :8;

    /*
     * [SP Specific] The value of this field is specific to the Security
     * Protocol as defined in SPC-4.
     */
    ULONG   SPSP        :16;

    /*
     * [Security Protocol] This field indicates the security protocol as defined
     * in SPC-4. The controller shall fail the command with Invalid Parameter
     * indicated if an unsupported value of the Security Protocol is specified.
     */
    ULONG   SECP        :8;
} ADMIN_SECURITY_RECEIVE_COMMAND_DW10, *PADMIN_SECURITY_RECEIVE_COMMAND_DW10;

/* Security Receive Command, Section 5.14, Figure 93 */
typedef struct _ADMIN_SECURITY_RECEIVE_COMMAND_DW11
{
    /*
     * [Transfer Length] The value of this field is specific to the Security
     * Protocol as defined in SPC-4.
     */
    ULONG   AL;
} ADMIN_SECURITY_RECEIVE_COMMAND_DW11, *PADMIN_SECURITY_RECEIVE_COMMAND_DW11;

/*
 * Firmware Image Download Command, Section 5.8, Opcode 0x11
 *
 * no field tables needed
 */

/*
 * Asynchronous Event Information
 *
 * Error Status, Section 5.2.1, Figure 30
 */

/* Software wrote the doorbell of a queue that was not created. */
#define INVALID_SUBMISSION_QUEUE            0x00

/*
 * Software attempted to write an invalid doorbell value. Some possible causes
 * of this error are: � the value written was out of range of the corresponding
 * queue�s base address and size, � the value written is the same as the
 * previously written doorbell value, � software attempts to add a command to a
 * full Submission Queue, and � software attempts to remove a completion entry
 * from an empty Completion Queue.
 */
#define INVALID_DOORBELL_WRITE_VALUE        0x01

/* A diagnostic failure was detected. This may include a self test operation. */
#define DIAGNOSTIC_FAILURE                  0x02

/*
 * A failure occurred within the device that is persistent or the device is
 * unable to isolate to a specific set of commands.
 */
#define PERSISTENT_INTERNAL_DEVICE_ERROR    0x03

/*
 * A transient error occurred within the device that is specific to a particular
 * set of commands and operation may continue.
 */
#define TRANSIENT_INTERNAL_DEVICE_ERROR     0x04

/*
 * The firmware image could not be loaded. The controller reverted to the
 * previously active firmware image or a baseline read-only firmware image.
 */
#define FIRMWARE_IMAGE_LOAD_ERROR           0x05


/*
 * Asynchronous Event Information
 *
 * SMART/Health Status, Section 5.2.1, Figure 31
 */

/*
 * Device reliability has been compromised. This may be due to significant media
 * errors, an internal error, the media being placed in read only mode, or a
 * volatile memory backup device failing.
 */
#define DEVICE_RELIABILITY                  0x00

/* Temperature is above the temperature threshold. */
#define TEMPERATURE_ABOVE_THRESHOLD         0x01

/* Available spare space has fallen below the threshold. */
#define SPARE_BELOW_THRESHOLD               0x02

/*
 * NVMe NVM Command Set
 *
 * Opcodes for NVM Commands, Section 6, Figure 98
 *    and FUSE methods
 */
#define NVM_FLUSH                           0x00
#define NVM_WRITE                           0x01
#define NVM_READ                            0x02

#define NVM_WRITE_UNCORRECTABLE             0x04
#define NVM_COMPARE                         0x05
#define NVM_DATASET_MANAGEMENT              0x09
#define NVM_RESERVATION_REGISTER            0x0D
#define NVM_RESERVATION_REPORT              0x0E
#define NVM_RESERVATION_ACQUIRE             0x11
#define NVM_RESERVATION_RELEASE             0x15

#define NVM_VENDOR_SPECIFIC_START           0x80
#define NVM_VENDOR_SPECIFIC_END             0xFF

#define FUSE_NORMAL_OPERATION               0

/*
 * Flush Command, Section 6.7, Opcode 0x00
 *
 * no field tables needed
 */

/* Write Command, Section 6.9, Figure 129, Opcode 0x01 */
typedef struct _NVM_WRITE_COMMAND_DW12
{
    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks to be written.  This is a 0�s based value.
     */
    USHORT  NLB;
    USHORT  Reserved    :10;

    /*
     * [Protection Information Field] Specifies the protection information
     * action and check field, as defined in Figure 100.
     */
    USHORT  PRINFO      :4;

    /*
     * [Force Unit Access] This field indicates that the data written shall be
     * returned from non-volatile media.  There is no implied ordering with
     * other commands.
     */
    USHORT  FUA         :1;

    /*
     * [Limited Retry] If set to �1�, the controller should apply limited retry
     * efforts.  If cleared to �0�, the controller should apply all available
     * error recovery means to return the data to the host.
     */
    USHORT  LR          :1;
} NVM_WRITE_COMMAND_DW12, *PNVM_WRITE_COMMAND_DW12;

/* Write Command, Section 6.9, Figure 130 */
typedef struct _NVM_WRITE_COMMAND_DW13
{
    /*
     * [Dataset Management] This field indicates attributes for the dataset that
     * the LBA(s) being written are associated with.
     */
    struct
    {
        /*
         * Value 0000b == No frequency information provided. Value 0001b ==
         * Typical number of writes and writes expected for this LBA range.
         * Value 0010b == Infrequent writes and infrequent writes to the LBA
         * range indicated. Value 0011b == Infrequent writes and frequent writes
         * to the LBA range indicated. Value 0100b == Frequent writes and
         * infrequent writes to the LBA range indicated. Value 0101b == Frequent
         * writes and frequent writes to the LBA range indicated. Value 0110b ==
         * One time write. E.g. command is due to virus scan, backup, file copy,
         * or archive. Value 0111b == The LBA range is going to be overwritten
         * in the near future. Value 1001b � 1111b == Reserved.
         */
        UCHAR   AccessFrequency     :4;

        /*
         * Value 00b == None. No latency information provided. Value 01b ==
         * Idle. Longer latency acceptable. Value == 10b Normal. Typical
         * latency. Value 11b == Low. Smallest possible latency.
         */
        UCHAR   AccessLatency       :2;

        /*
         * If set to �1�, then this command is part of a sequential write that
         * includes multiple Write commands.  If cleared to �0�, then no
         * information on sequential access is provided.
         */
        UCHAR   SequentialRequest   :1;

        /*
         * If set to �1�, then data is not compressible for the logical blocks
         * indicated.  If cleared to �0�, then no information on compression is
         * provided.
         */
        UCHAR   Incompressible      :1;
    } DSM;

    UCHAR   Reserved[3];
} NVM_WRITE_COMMAND_DW13, *PNVM_WRITE_COMMAND_DW13;

/* Write Command, Section 6.9, Figure 131 */
typedef struct _NVM_WRITE_COMMAND_DW15
{
    /*
     * [Expected Logical Block Application Tag] This field indicates the
     * Application Tag expected value.  This field is only used if the namespace
     * is formatted to use end-to-end protection information.  Refer to
     * section 8.2.
     */
    USHORT  ELBAT;

    /*
     * [Expected Logical Block Application Tag Mask] This field indicates the
     * Application Tag Mask expected value.  This field is only used if the
     * namespace is formatted to use end-to-end protection information.  Refer
     * to section 8.2.
     */
    USHORT  ELBATM;
} NVM_WRITE_COMMAND_DW15, *PNVM_WRITE_COMMAND_DW15;

/* Read Command, Section 6.8, Figure 120, Opcode 0x02 */
typedef struct _NVM_READ_COMMAND_DW12
{
    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks to be read.  This is a 0�s based value.
     */
    USHORT  NLB;
    USHORT  Reserved    :10;

    /*
     * [Protection Information Field] Specifies the protection information
     * action and check field, as defined in Figure 100.
     */
    USHORT  PRINFO      :4;

    /*
     * [Force Unit Access] This field indicates that the data read shall be
     * returned from non-volatile media.  There is no implied ordering with
     * other commands.
     */
    USHORT  FUA         :1;

    /*
     * [Limited Retry] If set to �1�, the controller should apply limited retry
     * efforts.  If cleared to �0�, the controller should apply all available
     * error recovery means to return the data to the host.
     */
    USHORT  LR          :1;
} NVM_READ_COMMAND_DW12, *PNVM_READ_COMMAND_DW12;

/* Read Command, Section 6.8, Figure 121 */
typedef struct _NVM_READ_COMMAND_DW13
{
    /*
     * [Dataset Management] This field indicates attributes for the dataset that
     * the LBA(s) being read are associated with.
     */
    struct
    {
        /*
         * Value 0000b == No frequency information provided. Value 0001b ==
         * Typical number of reads and writes expected for this LBA range. Value
         * 0010b == Infrequent writes and infrequent reads to the LBA range
         * indicated. Value 0011b == Infrequent writes and frequent reads to the
         * LBA range indicated. Value 0100b == Frequent writes and infrequent
         * reads to the LBA range indicated. Value 0101b == Frequent writes and
         * frequent reads to the LBA range indicated. Value 0110b == One time
         * read. E.g. command is due to virus scan, backup, file copy, or
         * archive. Value 0111b == Speculative read.  The command is part of a
         * prefetch operation. Value 1000b == The LBA range is going to be
         * overwritten in the near future. Value 1001b � 1111b == Reserved.
         */
        UCHAR   AccessFrequency     :4;

        /*
         * Value 00b == None. No latency information provided. Value 01b ==
         * Idle. Longer latency acceptable. Value == 10b Normal. Typical
         * latency. Value 11b == Low. Smallest possible latency.
         */
        UCHAR   AccessLatency       :2;

        /*
         * If set to �1�, then this command is part of a sequential read that
         * includes multiple Read commands.  If cleared to �0�, then no
         * information on sequential access is provided.
         */
        UCHAR   SequentialRequest   :1;

        /*
         * If set to �1�, then data is not compressible for the logical blocks
         * indicated.  If cleared to �0�, then no information on compression is
         * provided.
         */
        UCHAR   Incompressible      :1;
    } DSM;

    UCHAR   Reserved[3];
} NVM_READ_COMMAND_DW13, *PNVM_READ_COMMAND_DW13;

/* Read Command, Section 6.8, Figure 123 */
typedef struct _NVM_READ_COMMAND_DW15
{
    /*
     * [Expected Logical Block Application Tag] This field indicates the
     * Application Tag expected value.  This field is only used if the namespace
     * is formatted to use end-to-end protection information.  Refer to section
     * 8.2.
     */
    USHORT  ELBAT;

    /*
     * [Expected Logical Block Application Tag Mask] This field indicates the
     * Application Tag Mask expected value.  This field is only used if the
     * namespace is formatted to use end-to-end protection information.  Refer
     * to section 8.2.
     */
    USHORT  ELBATM;
} NVM_READ_COMMAND_DW15, *PNVM_READ_COMMAND_DW15;

/* Write Uncorrectable Command, Section 6.10, Figure 135, Opcode 0x04 */
typedef struct _NVM_WRITE_UNCORRECTABLE_COMMAND_DW12
{
    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks to be marked as invalid.  This is a 0�s based value.
     */
    USHORT  NLB;
    USHORT  Reserved;
} NVM_WRITE_UNCORRECTABLE_COMMAND_DW12, *PNVM_WRITE_UNCORRECTABLE_COMMAND_DW12;


/* Compare Command, Section 6.5, Figure 105, Opcode 0x05 */
typedef struct _NVM_COMPARE_COMMAND_DW12
{
    /*
     * [Number of Logical Blocks] This field indicates the number of logical
     * blocks to be compared.  This is a 0�s based value.
     */
    USHORT  NLB;
    USHORT  Reserved    :10;

    /*
     * [Protection Information Field] Specifies the protection information
     * action and check field, as defined in Figure 100.
     */
    USHORT  PRINFO      :4;

    /*
     * [Force Unit Access] This field indicates that the data read shall be
     * returned from non-volatile media.  There is no implied ordering with
     * other commands.
     */
    USHORT  FUA         :1;

    /*
     * [Limited Retry] If set to 1�, the controller should apply limited retry
     * efforts.  If cleared to �0�, the controller should apply all available
     * error recovery means to retrieve the data for comparison.
     */
    USHORT  LR          :1;
} NVM_COMPARE_COMMAND_DW12, *PNVM_COMPARE_COMMAND_DW12;

/* Compare Command, Section 6.5, Figure 107 */
typedef struct _NVM_COMPARE_COMMAND_DW15
{
    /*
     * [Expected Logical Block Application Tag] This field indicates the
     * Application Tag expected value.  This field is only used if the namespace
     * is formatted to use end-to-end protection information.  Refer to section
     * 8.2.
     */
    USHORT  ELBAT;

    /*
     * [Expected Logical Block Application Tag Mask] This field indicates the
     * Application Tag Mask expected value.  This field is only used if the
     * namespace is formatted to use end-to-end protection information.  Refer
     * to section 8.2.
     */
    USHORT  ELBATM;
} NVM_COMPARE_COMMAND_DW15, *PNVM_COMPARE_COMMAND_DW15;

/* Dataset Management Command, Section 6.6, Figure 111 */
typedef struct _NVM_DATASET_MANAGEMENT_COMMAND_DW10
{
    /*
     * [Number of Ranges] Indicates the number of 16 byte range sets that are
     * specified in the command.  This is a 0�s based value.
     */
    ULONG   NR       :8;
    ULONG   Reserved :24;
} NVM_DATASET_MANAGEMENT_COMMAND_DW10, *PNVM_DATASET_MANAGEMENT_COMMAND_DW10;

/* Dataset Management Command, Section 6.6, Figure 112 */
typedef struct _NVM_DATASET_MANAGEMENT_COMMAND_DW11
{
    /*
     * [Attribute � Integral Dataset for Read] If set to �1� then the dataset
     * should be optimized for read access as an integral unit.  The host
     * expects to perform operations on all ranges provided as an integral unit
     * for reads, indicating that if a portion of the dataset is read it is
     * expected that all of the ranges in the dataset are going to be read.
     */
    ULONG   IDR      :1;

    /*
     * [Attribute � Integral Dataset for Write] If set to �1� then the dataset
     * should be optimized for write access as an integral unit.  The host
     * expects to perform operations on all ranges provided as an integral unit
     * for writes, indicating that if a portion of the dataset is written it is
     * expected that all of the ranges in the dataset are going to be written.
     */
    ULONG   IDW :1;

    /*
     * [Attribute � Deallocate] If set to �1� then the NVM subsystem may
     * deallocate all provided ranges.  If a read occurs to a deallocated range,
     * the NVM Express subsystem shall return all zeros, all ones, or the last
     * data written to the associated LBA.
     */
    ULONG   AD       :1;
    ULONG   Reserved :29;
} NVM_DATASET_MANAGEMENT_COMMAND_DW11, *PNVM_DATASET_MANAGEMENT_COMMAND_DW11;

/* Dataset Management - Context Attributesn, Section 6.6, Figure 114 */
typedef struct _NVM_DATASET_MANAGEMENT_CONTEXT_ATTRIBUTES
{
    /*
     * [Access Frequency] Value 0000b == No frequency information provided.
     * Value 0001b == Typical number of reads and writes expected for this LBA
     * range. Value 0010b == Infrequent writes and infrequent reads to the LBA
     * range indicated. Value 0011b == Infrequent writes and frequent reads to
     * the LBA range indicated. Value 0100b == Frequent writes and infrequent
     * reads to the LBA range indicated. Value 0101b == Frequent writes and
     * frequent reads to the LBA range indicated. Value 0110b � 1111b ==
     * Reserved.
     */
    ULONG   AF                :4;

    /*
     * [Access Latency] Value 00b == None. No latency information provided.
     * Value 01 == Idle. Longer latency acceptable. Value 10b == Normal. Typical
     * latency. Value 11b == Low. Smallest possible latency.
     */
    ULONG   AL                :2;
    ULONG   Reserved1         :2;

    /*
     * [Sequential Read Range] If set to �1� then the dataset should be
     * optimized for sequential read access. The host expects to perform
     * operations on the dataset as a single object for reads.
     */
    ULONG   SR                :1;

    /*
     * [Sequential Write Range] If set to �1� then the dataset should be
     * optimized for sequential write access.  The host expects to perform
     * operations on the dataset as a single object for writes.
     */
    ULONG   SW                :1;

    /*
     * [Write Prepare] If set to �1� then the provided range is expected to be
     * written in the near future.
     */
    ULONG   WP                :1;
    ULONG   Reserved2         :13;

    /*
     * Number of logical blocks expected to be transferred in a single Read or
     * Write command from this dataset. A value of 0h indicates no Command
     * Access Size is provided.
     */
    ULONG   CommandAccessSize :8;
} NVM_DATASET_MANAGEMENT_CONTEXT_ATTRIBUTES,
  *PNVM_DATASET_MANAGEMENT_CONTEXT_ATTRIBUTES;

/* Dataset Management - Range Definition, Section 6.6, Figure 113 */
typedef struct _NVM_DATASET_MANAGEMENT_RANGE
{
    NVM_DATASET_MANAGEMENT_CONTEXT_ATTRIBUTES   ContextAttributes;
    ULONG                                       LengthInLogicalBlocks;
    ULONGLONG                                   StartingLBA;
} NVM_DATASET_MANAGEMENT_RANGE, *PNVM_DATASET_MANAGEMENT_RANGE;


#pragma pack(1)
/* Reservation Report Command, NVMe 1.2, Section 6.13, Figure 189, Opcode 0x0E */
typedef struct _NVM_RESERVATION_REPORT_COMMAND_DW10
{
    /*
    * [Number of Dwords] This field specifies the number of Dwords of the
    * Reservation Status data structure to transfer. This is a 0's based 
    * value
    */
    ULONG   NUMD;
} NVM_RESERVATION_REPORT_COMMAND_DW10,
*PNVM_RESERVATION_REPORT_COMMAND_DW10;
#pragma pack()

typedef enum _NVME_RTYPE
{
    NVME_RTYPE_NO_RESERVATION       = 0,
    NVME_RTYPE_WRITE_EXC            = 1,
    NVME_RTYPE_EXCLUSIVE_ACC        = 2,
    NVME_RTYPE_WRITE_EXC_REG_ONLY   = 3,
    NVME_RTYPE_EXC_ACC_REG_ONLY     = 4,
    NVME_RTYPE_WRITE_EXC_ALL        = 5,
    NVME_RTYPE_EXC_ACC_ALL          = 6,
    NVME_RTYPE_MAX_VALUE            = 6,
    NVME_RTYPE_RESERVED             = 7
} NVME_RTYPE;

#define NVME_RCSTS_HOST_HOLDS_RES_MASK 0X01


#pragma pack(1)
/* Reservation Report Status Data Structure Header */
typedef struct _NVM_RES_REPORT_HDR
{
    ULONG  GEN;
    UCHAR  RTYPE;
    USHORT REGCTL;
    USHORT Reserved;
    UCHAR  PTPLS;
    UCHAR  Reserved2[14];
} NVM_RES_REPORT_HDR, *PNVM_RES_REPORT_HDR;
#pragma pack()


#pragma pack(1)
/* Registered Controller Data Structure */
typedef struct _NVM_REGISTERED_CTRL_DATASTRUCT
{
    USHORT CNTLID;
    UCHAR  RCSTS;
    UCHAR  Reserved[5];
    ULONGLONG HOSTID;
    ULONGLONG RKEY;
} NVM_REGISTERED_CTRL_DATASTRUCT, *PNVM_REGISTERED_CTRL_DATASTRUCT;
#pragma pack()


#pragma pack(1)
/* Reservation Report Command, NVMe 1.2, , Section 6.11, Figure 183, Opcode 0x0D */
typedef struct _NVM_RES_REGISTER_COMMAND_DW10
{
    ULONG RREGA     : 3;
    ULONG IEKEY     : 1;
    ULONG Reserved  : 26;
    ULONG CPTPL     : 2;
} NVM_RES_REGISTER_COMMAND_DW10,
*PNVM_RES_REGISTER_COMMAND_DW10;
#pragma pack()

typedef enum _NVM_RESERVATION {
    /* IEKEY settings */
    NVM_REG_CHECK_EXISTING_KEY   = 0,
    NVM_REG_IGNORE_EXISTING_KEY  = 1,
    /* Register actions */
    NVM_REG_ACTION_REG_KEY      = 0,
    NVM_REG_ACTION_UNREG_KEY    = 1,
    NVM_REG_ACTION_REPLACE_KEY  = 2,

    /* Release Actions */
    NVM_REL_ACTION_RELEASE      = 0,
    NVM_REL_ACTION_CLEAR        = 1,

    /* Register CPTPL commands */
    NVM_REG_CPTPL_NO_CHANGE     = 0,
    NVM_REG_CPTPL_RES_RELEASED  = 2,
    NVM_REG_CPTPL_RES_PERSIST   = 3,

    /* Acquire reservation types */
    NVM_ACQ_RESTYPE_WRITE_EX    = 1,
    NVM_ACQ_RESTYPE_EXC_ACC     = 2,
    NVM_ACQ_RESTYPE_WRITE_EX_RO = 3,
    NVM_ACQ_RESTYPE_EXC_ACC_RO  = 4,
    NVM_ACQ_RESTYPE_WRITE_EX_A  = 5,
    NVM_ACQ_RESTYPE_EXC_ACC_A   = 6,

    /* Reservation Acquire Action */
    NVM_RES_ACQ_ACTION_ACQUIRE  = 0,
    NVM_RES_ACQ_ACTION_PREEMPT  = 1,
    NVM_RES_ACQ_ACTION_PREEMPT_ABORT = 2

} NVM_RESERVATION;

#pragma pack(1)
/* Reservation Register data structure, NVMe 1.2, Section 6.11, Figure 184, Opcode 0x0D */
typedef struct _NVM_RES_REGISTER_DATASTRUCT
{
    ULONGLONG CRKEY;
    ULONGLONG NRKEY;
} NVM_RES_REGISTER_DATASTRUCT, *PNVM_RES_REGISTER_DATASTRUCT;

#pragma pack(1)
/* Reservation Acquire Command, NVMe 1.2, Section 6.10, Figure 179, Opcode 0x11 */
typedef struct _NVM_RES_ACQUIRE_COMMAND_DW10
{
    ULONG RACQA      : 3;
    ULONG IEKEY      : 1;
    ULONG Reserved1  : 4;
    ULONG RTYPE      : 8;
    ULONG Reserved2  : 16;
} NVM_RES_ACQUIRE_COMMAND_DW10,
*PNVM_RES_ACQUIRE_COMMAND_DW10;
#pragma pack()

#pragma pack(1)
/* Reservation Acquire data structure, NVMe 1.2, Section 6.10, Figure 180, Opcode 0x11 */
typedef struct _NVM_RES_ACQUIRE_DATASTRUCT
{
    ULONGLONG CRKEY;
    ULONGLONG PRKEY;
} NVM_RES_ACQUIRE_DATASTRUCT, *PNVM_RES_ACQUIRE_DATASTRUCT;

#pragma pack()

#pragma pack(1)
/* Reservation Release Command, NVMe 1.2, Section 6.12, Figure 186, Opcode 0x15 */
typedef struct _NVM_RES_RELEASE_COMMAND_DW10
{
    ULONG RRELA     : 3;
    ULONG IEKEY     : 1;
    ULONG Reserved1 : 4;
    ULONG RTYPE     : 8;
    ULONG Reserved2 : 16;
} NVM_RES_RELEASE_COMMAND_DW10,
*PNVM_RES_RELEASE_COMMAND_DW10;
#pragma pack()

#pragma pack(1)
/* Reservation Release data structure, NVMe 1.2, Section 6.12, Figure 187, Opcode 0x15 */
typedef struct _NVM_RES_RELEASE_DATASTRUCT
{
    ULONGLONG CRKEY;
} NVM_RES_RELEASE_DATASTRUCT, *PNVM_RES_RELEASE_DATASTRUCT;

#pragma pack()


/* NVME 1.2, Section 4.9, Figure 37 */
typedef struct _NVMe_CONTROLLER_LIST
{
    /*
     * This field contains the number of controller entries in the list. There
     * may be up to 2047 identifiers in the list. A value of 0 indicates there
     * are no controllers in the list.
     */
    USHORT     NumberOfIdentifiers;
    /*
     * This field contains the NVM subsystem unique controller identifer for
     * the controllers in the list or 0h if the list is empty or fewer than
     * N Entries.
     */
    USHORT     Identifers[1];
} NVMe_CONTROLLER_LIST, *PNVMe_CONTROLLER_LIST;

/* Namespace Management Command - NVME1.2, Section 5.12.2, Figure 101 */
typedef struct _ADMIN_NAMESPACE_MANAGEMENT_DW10
{
    /*
     * This field selects the type of the management operation to perform
     *           Value                   Description
     *            0h                     Create
     *            1h                     Delete
     *           2h-Fh                   Reserved
     */
    ULONG      SEL        :4;
    ULONG      Reserved   :28;
} ADMIN_NAMESPACE_MANAGEMENT_DW10, *PADMIN_NAMESPACE_MANAGEMENT_DW10;

/* Admin Namespace Management DW10 Select Field */
#define NAMESPACE_MANAGEMENT_CREATE                      0x0
#define NAMESPACE_MANAGEMENT_DELETE                      0x1

/* Namespace Attachment Command - NVME1.2, Section 5.12.1, Figure 96 */
typedef struct _ADMIN_NAMESPACE_ATTACHMENT_DW10
{
    /* 
     * This field selects the type of attachment to perform
     *           Value                   Description
     *            0h                     Controller attach
     *            1h                     Controller detach
     *           2h-Fh                   Reserved
     */
    ULONG     SEL          :4;
    ULONG     Reserved     :28;
} ADMIN_NAMESPACE_ATTACHMENT_DW10, *PADMIN_NAMESPACE_ATTACHMENT_DW10;

/* Admin Namespace Attachement DW10 Select Field */
#define NAMESPACE_ATTACH                      0x0
#define NAMESPACE_DETACH                      0x1

#endif /* __NVME_H__ */
