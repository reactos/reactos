/******************************************************************************
 *
 * Name: actbl1.h - Additional ACPI table definitions
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2011, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#ifndef __ACTBL1_H__
#define __ACTBL1_H__


/*******************************************************************************
 *
 * Additional ACPI Tables (1)
 *
 * These tables are not consumed directly by the ACPICA subsystem, but are
 * included here to support device drivers and the AML disassembler.
 *
 * The tables in this file are fully defined within the ACPI specification.
 *
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_BERT           "BERT"      /* Boot Error Record Table */
#define ACPI_SIG_CPEP           "CPEP"      /* Corrected Platform Error Polling table */
#define ACPI_SIG_ECDT           "ECDT"      /* Embedded Controller Boot Resources Table */
#define ACPI_SIG_EINJ           "EINJ"      /* Error Injection table */
#define ACPI_SIG_ERST           "ERST"      /* Error Record Serialization Table */
#define ACPI_SIG_HEST           "HEST"      /* Hardware Error Source Table */
#define ACPI_SIG_MADT           "APIC"      /* Multiple APIC Description Table */
#define ACPI_SIG_MSCT           "MSCT"      /* Maximum System Characteristics Table */
#define ACPI_SIG_SBST           "SBST"      /* Smart Battery Specification Table */
#define ACPI_SIG_SLIT           "SLIT"      /* System Locality Distance Information Table */
#define ACPI_SIG_SRAT           "SRAT"      /* System Resource Affinity Table */


/*
 * All tables must be byte-packed to match the ACPI specification, since
 * the tables are provided by the system BIOS.
 */
#pragma pack(1)

/*
 * Note about bitfields: The UINT8 type is used for bitfields in ACPI tables.
 * This is the only type that is even remotely portable. Anything else is not
 * portable, so do not use any other bitfield types.
 */


/*******************************************************************************
 *
 * Common subtable headers
 *
 ******************************************************************************/

/* Generic subtable header (used in MADT, SRAT, etc.) */

typedef struct acpi_subtable_header
{
    UINT8                   Type;
    UINT8                   Length;

} ACPI_SUBTABLE_HEADER;


/* Subtable header for WHEA tables (EINJ, ERST, WDAT) */

typedef struct acpi_whea_header
{
    UINT8                   Action;
    UINT8                   Instruction;
    UINT8                   Flags;
    UINT8                   Reserved;
    ACPI_GENERIC_ADDRESS    RegisterRegion;
    UINT64                  Value;              /* Value used with Read/Write register */
    UINT64                  Mask;               /* Bitmask required for this register instruction */

} ACPI_WHEA_HEADER;


/*******************************************************************************
 *
 * BERT - Boot Error Record Table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_bert
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  RegionLength;       /* Length of the boot error region */
    UINT64                  Address;            /* Physical addresss of the error region */

} ACPI_TABLE_BERT;


/* Boot Error Region (not a subtable, pointed to by Address field above) */

typedef struct acpi_bert_region
{
    UINT32                  BlockStatus;        /* Type of error information */
    UINT32                  RawDataOffset;      /* Offset to raw error data */
    UINT32                  RawDataLength;      /* Length of raw error data */
    UINT32                  DataLength;         /* Length of generic error data */
    UINT32                  ErrorSeverity;      /* Severity code */

} ACPI_BERT_REGION;

/* Values for BlockStatus flags above */

#define ACPI_BERT_UNCORRECTABLE             (1)
#define ACPI_BERT_CORRECTABLE               (1<<1)
#define ACPI_BERT_MULTIPLE_UNCORRECTABLE    (1<<2)
#define ACPI_BERT_MULTIPLE_CORRECTABLE      (1<<3)
#define ACPI_BERT_ERROR_ENTRY_COUNT         (0xFF<<4) /* 8 bits, error count */

/* Values for ErrorSeverity above */

enum AcpiBertErrorSeverity
{
    ACPI_BERT_ERROR_CORRECTABLE     = 0,
    ACPI_BERT_ERROR_FATAL           = 1,
    ACPI_BERT_ERROR_CORRECTED       = 2,
    ACPI_BERT_ERROR_NONE            = 3,
    ACPI_BERT_ERROR_RESERVED        = 4     /* 4 and greater are reserved */
};

/*
 * Note: The generic error data that follows the ErrorSeverity field above
 * uses the ACPI_HEST_GENERIC_DATA defined under the HEST table below
 */


/*******************************************************************************
 *
 * CPEP - Corrected Platform Error Polling table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_cpep
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT64                  Reserved;

} ACPI_TABLE_CPEP;


/* Subtable */

typedef struct acpi_cpep_polling
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Id;                 /* Processor ID */
    UINT8                   Eid;                /* Processor EID */
    UINT32                  Interval;           /* Polling interval (msec) */

} ACPI_CPEP_POLLING;


/*******************************************************************************
 *
 * ECDT - Embedded Controller Boot Resources Table
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_ecdt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    ACPI_GENERIC_ADDRESS    Control;            /* Address of EC command/status register */
    ACPI_GENERIC_ADDRESS    Data;               /* Address of EC data register */
    UINT32                  Uid;                /* Unique ID - must be same as the EC _UID method */
    UINT8                   Gpe;                /* The GPE for the EC */
    UINT8                   Id[1];              /* Full namepath of the EC in the ACPI namespace */

} ACPI_TABLE_ECDT;


/*******************************************************************************
 *
 * EINJ - Error Injection Table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_einj
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  HeaderLength;
    UINT8                   Flags;
    UINT8                   Reserved[3];
    UINT32                  Entries;

} ACPI_TABLE_EINJ;


/* EINJ Injection Instruction Entries (actions) */

typedef struct acpi_einj_entry
{
    ACPI_WHEA_HEADER        WheaHeader;         /* Common header for WHEA tables */

} ACPI_EINJ_ENTRY;

/* Masks for Flags field above */

#define ACPI_EINJ_PRESERVE          (1)

/* Values for Action field above */

enum AcpiEinjActions
{
    ACPI_EINJ_BEGIN_OPERATION       = 0,
    ACPI_EINJ_GET_TRIGGER_TABLE     = 1,
    ACPI_EINJ_SET_ERROR_TYPE        = 2,
    ACPI_EINJ_GET_ERROR_TYPE        = 3,
    ACPI_EINJ_END_OPERATION         = 4,
    ACPI_EINJ_EXECUTE_OPERATION     = 5,
    ACPI_EINJ_CHECK_BUSY_STATUS     = 6,
    ACPI_EINJ_GET_COMMAND_STATUS    = 7,
    ACPI_EINJ_ACTION_RESERVED       = 8,     /* 8 and greater are reserved */
    ACPI_EINJ_TRIGGER_ERROR         = 0xFF   /* Except for this value */
};

/* Values for Instruction field above */

enum AcpiEinjInstructions
{
    ACPI_EINJ_READ_REGISTER         = 0,
    ACPI_EINJ_READ_REGISTER_VALUE   = 1,
    ACPI_EINJ_WRITE_REGISTER        = 2,
    ACPI_EINJ_WRITE_REGISTER_VALUE  = 3,
    ACPI_EINJ_NOOP                  = 4,
    ACPI_EINJ_INSTRUCTION_RESERVED  = 5     /* 5 and greater are reserved */
};


/* EINJ Trigger Error Action Table */

typedef struct acpi_einj_trigger
{
    UINT32                  HeaderSize;
    UINT32                  Revision;
    UINT32                  TableSize;
    UINT32                  EntryCount;

} ACPI_EINJ_TRIGGER;

/* Command status return values */

enum AcpiEinjCommandStatus
{
    ACPI_EINJ_SUCCESS               = 0,
    ACPI_EINJ_FAILURE               = 1,
    ACPI_EINJ_INVALID_ACCESS        = 2,
    ACPI_EINJ_STATUS_RESERVED       = 3     /* 3 and greater are reserved */
};


/* Error types returned from ACPI_EINJ_GET_ERROR_TYPE (bitfield) */

#define ACPI_EINJ_PROCESSOR_CORRECTABLE     (1)
#define ACPI_EINJ_PROCESSOR_UNCORRECTABLE   (1<<1)
#define ACPI_EINJ_PROCESSOR_FATAL           (1<<2)
#define ACPI_EINJ_MEMORY_CORRECTABLE        (1<<3)
#define ACPI_EINJ_MEMORY_UNCORRECTABLE      (1<<4)
#define ACPI_EINJ_MEMORY_FATAL              (1<<5)
#define ACPI_EINJ_PCIX_CORRECTABLE          (1<<6)
#define ACPI_EINJ_PCIX_UNCORRECTABLE        (1<<7)
#define ACPI_EINJ_PCIX_FATAL                (1<<8)
#define ACPI_EINJ_PLATFORM_CORRECTABLE      (1<<9)
#define ACPI_EINJ_PLATFORM_UNCORRECTABLE    (1<<10)
#define ACPI_EINJ_PLATFORM_FATAL            (1<<11)


/*******************************************************************************
 *
 * ERST - Error Record Serialization Table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_erst
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  HeaderLength;
    UINT32                  Reserved;
    UINT32                  Entries;

} ACPI_TABLE_ERST;


/* ERST Serialization Entries (actions) */

typedef struct acpi_erst_entry
{
    ACPI_WHEA_HEADER        WheaHeader;         /* Common header for WHEA tables */

} ACPI_ERST_ENTRY;

/* Masks for Flags field above */

#define ACPI_ERST_PRESERVE          (1)

/* Values for Action field above */

enum AcpiErstActions
{
    ACPI_ERST_BEGIN_WRITE           = 0,
    ACPI_ERST_BEGIN_READ            = 1,
    ACPI_ERST_BEGIN_CLEAR           = 2,
    ACPI_ERST_END                   = 3,
    ACPI_ERST_SET_RECORD_OFFSET     = 4,
    ACPI_ERST_EXECUTE_OPERATION     = 5,
    ACPI_ERST_CHECK_BUSY_STATUS     = 6,
    ACPI_ERST_GET_COMMAND_STATUS    = 7,
    ACPI_ERST_GET_RECORD_ID         = 8,
    ACPI_ERST_SET_RECORD_ID         = 9,
    ACPI_ERST_GET_RECORD_COUNT      = 10,
    ACPI_ERST_BEGIN_DUMMY_WRIITE    = 11,
    ACPI_ERST_NOT_USED              = 12,
    ACPI_ERST_GET_ERROR_RANGE       = 13,
    ACPI_ERST_GET_ERROR_LENGTH      = 14,
    ACPI_ERST_GET_ERROR_ATTRIBUTES  = 15,
    ACPI_ERST_ACTION_RESERVED       = 16    /* 16 and greater are reserved */
};

/* Values for Instruction field above */

enum AcpiErstInstructions
{
    ACPI_ERST_READ_REGISTER         = 0,
    ACPI_ERST_READ_REGISTER_VALUE   = 1,
    ACPI_ERST_WRITE_REGISTER        = 2,
    ACPI_ERST_WRITE_REGISTER_VALUE  = 3,
    ACPI_ERST_NOOP                  = 4,
    ACPI_ERST_LOAD_VAR1             = 5,
    ACPI_ERST_LOAD_VAR2             = 6,
    ACPI_ERST_STORE_VAR1            = 7,
    ACPI_ERST_ADD                   = 8,
    ACPI_ERST_SUBTRACT              = 9,
    ACPI_ERST_ADD_VALUE             = 10,
    ACPI_ERST_SUBTRACT_VALUE        = 11,
    ACPI_ERST_STALL                 = 12,
    ACPI_ERST_STALL_WHILE_TRUE      = 13,
    ACPI_ERST_SKIP_NEXT_IF_TRUE     = 14,
    ACPI_ERST_GOTO                  = 15,
    ACPI_ERST_SET_SRC_ADDRESS_BASE  = 16,
    ACPI_ERST_SET_DST_ADDRESS_BASE  = 17,
    ACPI_ERST_MOVE_DATA             = 18,
    ACPI_ERST_INSTRUCTION_RESERVED  = 19    /* 19 and greater are reserved */
};

/* Command status return values */

enum AcpiErstCommandStatus
{
    ACPI_ERST_SUCESS                = 0,
    ACPI_ERST_NO_SPACE              = 1,
    ACPI_ERST_NOT_AVAILABLE         = 2,
    ACPI_ERST_FAILURE               = 3,
    ACPI_ERST_RECORD_EMPTY          = 4,
    ACPI_ERST_NOT_FOUND             = 5,
    ACPI_ERST_STATUS_RESERVED       = 6     /* 6 and greater are reserved */
};


/* Error Record Serialization Information */

typedef struct acpi_erst_info
{
    UINT16                  Signature;          /* Should be "ER" */
    UINT8                   Data[48];

} ACPI_ERST_INFO;


/*******************************************************************************
 *
 * HEST - Hardware Error Source Table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_hest
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  ErrorSourceCount;

} ACPI_TABLE_HEST;


/* HEST subtable header */

typedef struct acpi_hest_header
{
    UINT16                  Type;
    UINT16                  SourceId;

} ACPI_HEST_HEADER;


/* Values for Type field above for subtables */

enum AcpiHestTypes
{
    ACPI_HEST_TYPE_IA32_CHECK           = 0,
    ACPI_HEST_TYPE_IA32_CORRECTED_CHECK = 1,
    ACPI_HEST_TYPE_IA32_NMI             = 2,
    ACPI_HEST_TYPE_NOT_USED3            = 3,
    ACPI_HEST_TYPE_NOT_USED4            = 4,
    ACPI_HEST_TYPE_NOT_USED5            = 5,
    ACPI_HEST_TYPE_AER_ROOT_PORT        = 6,
    ACPI_HEST_TYPE_AER_ENDPOINT         = 7,
    ACPI_HEST_TYPE_AER_BRIDGE           = 8,
    ACPI_HEST_TYPE_GENERIC_ERROR        = 9,
    ACPI_HEST_TYPE_RESERVED             = 10    /* 10 and greater are reserved */
};


/*
 * HEST substructures contained in subtables
 */

/*
 * IA32 Error Bank(s) - Follows the ACPI_HEST_IA_MACHINE_CHECK and
 * ACPI_HEST_IA_CORRECTED structures.
 */
typedef struct acpi_hest_ia_error_bank
{
    UINT8                   BankNumber;
    UINT8                   ClearStatusOnInit;
    UINT8                   StatusFormat;
    UINT8                   Reserved;
    UINT32                  ControlRegister;
    UINT64                  ControlData;
    UINT32                  StatusRegister;
    UINT32                  AddressRegister;
    UINT32                  MiscRegister;

} ACPI_HEST_IA_ERROR_BANK;


/* Common HEST sub-structure for PCI/AER structures below (6,7,8) */

typedef struct acpi_hest_aer_common
{
    UINT16                  Reserved1;
    UINT8                   Flags;
    UINT8                   Enabled;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    UINT32                  Bus;
    UINT16                  Device;
    UINT16                  Function;
    UINT16                  DeviceControl;
    UINT16                  Reserved2;
    UINT32                  UncorrectableMask;
    UINT32                  UncorrectableSeverity;
    UINT32                  CorrectableMask;
    UINT32                  AdvancedCapabilities;

} ACPI_HEST_AER_COMMON;

/* Masks for HEST Flags fields */

#define ACPI_HEST_FIRMWARE_FIRST        (1)
#define ACPI_HEST_GLOBAL                (1<<1)


/* Hardware Error Notification */

typedef struct acpi_hest_notify
{
    UINT8                   Type;
    UINT8                   Length;
    UINT16                  ConfigWriteEnable;
    UINT32                  PollInterval;
    UINT32                  Vector;
    UINT32                  PollingThresholdValue;
    UINT32                  PollingThresholdWindow;
    UINT32                  ErrorThresholdValue;
    UINT32                  ErrorThresholdWindow;

} ACPI_HEST_NOTIFY;

/* Values for Notify Type field above */

enum AcpiHestNotifyTypes
{
    ACPI_HEST_NOTIFY_POLLED     = 0,
    ACPI_HEST_NOTIFY_EXTERNAL   = 1,
    ACPI_HEST_NOTIFY_LOCAL      = 2,
    ACPI_HEST_NOTIFY_SCI        = 3,
    ACPI_HEST_NOTIFY_NMI        = 4,
    ACPI_HEST_NOTIFY_RESERVED   = 5     /* 5 and greater are reserved */
};

/* Values for ConfigWriteEnable bitfield above */

#define ACPI_HEST_TYPE                  (1)
#define ACPI_HEST_POLL_INTERVAL         (1<<1)
#define ACPI_HEST_POLL_THRESHOLD_VALUE  (1<<2)
#define ACPI_HEST_POLL_THRESHOLD_WINDOW (1<<3)
#define ACPI_HEST_ERR_THRESHOLD_VALUE   (1<<4)
#define ACPI_HEST_ERR_THRESHOLD_WINDOW  (1<<5)


/*
 * HEST subtables
 */

/* 0: IA32 Machine Check Exception */

typedef struct acpi_hest_ia_machine_check
{
    ACPI_HEST_HEADER        Header;
    UINT16                  Reserved1;
    UINT8                   Flags;
    UINT8                   Enabled;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    UINT64                  GlobalCapabilityData;
    UINT64                  GlobalControlData;
    UINT8                   NumHardwareBanks;
    UINT8                   Reserved3[7];

} ACPI_HEST_IA_MACHINE_CHECK;


/* 1: IA32 Corrected Machine Check */

typedef struct acpi_hest_ia_corrected
{
    ACPI_HEST_HEADER        Header;
    UINT16                  Reserved1;
    UINT8                   Flags;
    UINT8                   Enabled;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    ACPI_HEST_NOTIFY        Notify;
    UINT8                   NumHardwareBanks;
    UINT8                   Reserved2[3];

} ACPI_HEST_IA_CORRECTED;


/* 2: IA32 Non-Maskable Interrupt */

typedef struct acpi_hest_ia_nmi
{
    ACPI_HEST_HEADER        Header;
    UINT32                  Reserved;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    UINT32                  MaxRawDataLength;

} ACPI_HEST_IA_NMI;


/* 3,4,5: Not used */

/* 6: PCI Express Root Port AER */

typedef struct acpi_hest_aer_root
{
    ACPI_HEST_HEADER        Header;
    ACPI_HEST_AER_COMMON    Aer;
    UINT32                  RootErrorCommand;

} ACPI_HEST_AER_ROOT;


/* 7: PCI Express AER (AER Endpoint) */

typedef struct acpi_hest_aer
{
    ACPI_HEST_HEADER        Header;
    ACPI_HEST_AER_COMMON    Aer;

} ACPI_HEST_AER;


/* 8: PCI Express/PCI-X Bridge AER */

typedef struct acpi_hest_aer_bridge
{
    ACPI_HEST_HEADER        Header;
    ACPI_HEST_AER_COMMON    Aer;
    UINT32                  UncorrectableMask2;
    UINT32                  UncorrectableSeverity2;
    UINT32                  AdvancedCapabilities2;

} ACPI_HEST_AER_BRIDGE;


/* 9: Generic Hardware Error Source */

typedef struct acpi_hest_generic
{
    ACPI_HEST_HEADER        Header;
    UINT16                  RelatedSourceId;
    UINT8                   Reserved;
    UINT8                   Enabled;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    UINT32                  MaxRawDataLength;
    ACPI_GENERIC_ADDRESS    ErrorStatusAddress;
    ACPI_HEST_NOTIFY        Notify;
    UINT32                  ErrorBlockLength;

} ACPI_HEST_GENERIC;


/* Generic Error Status block */

typedef struct acpi_hest_generic_status
{
    UINT32                  BlockStatus;
    UINT32                  RawDataOffset;
    UINT32                  RawDataLength;
    UINT32                  DataLength;
    UINT32                  ErrorSeverity;

} ACPI_HEST_GENERIC_STATUS;

/* Values for BlockStatus flags above */

#define ACPI_HEST_UNCORRECTABLE             (1)
#define ACPI_HEST_CORRECTABLE               (1<<1)
#define ACPI_HEST_MULTIPLE_UNCORRECTABLE    (1<<2)
#define ACPI_HEST_MULTIPLE_CORRECTABLE      (1<<3)
#define ACPI_HEST_ERROR_ENTRY_COUNT         (0xFF<<4) /* 8 bits, error count */


/* Generic Error Data entry */

typedef struct acpi_hest_generic_data
{
    UINT8                   SectionType[16];
    UINT32                  ErrorSeverity;
    UINT16                  Revision;
    UINT8                   ValidationBits;
    UINT8                   Flags;
    UINT32                  ErrorDataLength;
    UINT8                   FruId[16];
    UINT8                   FruText[20];

} ACPI_HEST_GENERIC_DATA;


/*******************************************************************************
 *
 * MADT - Multiple APIC Description Table
 *        Version 3
 *
 ******************************************************************************/

typedef struct acpi_table_madt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Address;            /* Physical address of local APIC */
    UINT32                  Flags;

} ACPI_TABLE_MADT;

/* Masks for Flags field above */

#define ACPI_MADT_PCAT_COMPAT       (1)         /* 00: System also has dual 8259s */

/* Values for PCATCompat flag */

#define ACPI_MADT_DUAL_PIC          0
#define ACPI_MADT_MULTIPLE_APIC     1


/* Values for MADT subtable type in ACPI_SUBTABLE_HEADER */

enum AcpiMadtType
{
    ACPI_MADT_TYPE_LOCAL_APIC           = 0,
    ACPI_MADT_TYPE_IO_APIC              = 1,
    ACPI_MADT_TYPE_INTERRUPT_OVERRIDE   = 2,
    ACPI_MADT_TYPE_NMI_SOURCE           = 3,
    ACPI_MADT_TYPE_LOCAL_APIC_NMI       = 4,
    ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE  = 5,
    ACPI_MADT_TYPE_IO_SAPIC             = 6,
    ACPI_MADT_TYPE_LOCAL_SAPIC          = 7,
    ACPI_MADT_TYPE_INTERRUPT_SOURCE     = 8,
    ACPI_MADT_TYPE_LOCAL_X2APIC         = 9,
    ACPI_MADT_TYPE_LOCAL_X2APIC_NMI     = 10,
    ACPI_MADT_TYPE_RESERVED             = 11    /* 11 and greater are reserved */
};


/*
 * MADT Sub-tables, correspond to Type in ACPI_SUBTABLE_HEADER
 */

/* 0: Processor Local APIC */

typedef struct acpi_madt_local_apic
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   ProcessorId;        /* ACPI processor id */
    UINT8                   Id;                 /* Processor's local APIC id */
    UINT32                  LapicFlags;

} ACPI_MADT_LOCAL_APIC;


/* 1: IO APIC */

typedef struct acpi_madt_io_apic
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Id;                 /* I/O APIC ID */
    UINT8                   Reserved;           /* Reserved - must be zero */
    UINT32                  Address;            /* APIC physical address */
    UINT32                  GlobalIrqBase;      /* Global system interrupt where INTI lines start */

} ACPI_MADT_IO_APIC;


/* 2: Interrupt Override */

typedef struct acpi_madt_interrupt_override
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Bus;                /* 0 - ISA */
    UINT8                   SourceIrq;          /* Interrupt source (IRQ) */
    UINT32                  GlobalIrq;          /* Global system interrupt */
    UINT16                  IntiFlags;

} ACPI_MADT_INTERRUPT_OVERRIDE;


/* 3: NMI Source */

typedef struct acpi_madt_nmi_source
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  IntiFlags;
    UINT32                  GlobalIrq;          /* Global system interrupt */

} ACPI_MADT_NMI_SOURCE;


/* 4: Local APIC NMI */

typedef struct acpi_madt_local_apic_nmi
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   ProcessorId;        /* ACPI processor id */
    UINT16                  IntiFlags;
    UINT8                   Lint;               /* LINTn to which NMI is connected */

} ACPI_MADT_LOCAL_APIC_NMI;


/* 5: Address Override */

typedef struct acpi_madt_local_apic_override
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved, must be zero */
    UINT64                  Address;            /* APIC physical address */

} ACPI_MADT_LOCAL_APIC_OVERRIDE;


/* 6: I/O Sapic */

typedef struct acpi_madt_io_sapic
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Id;                 /* I/O SAPIC ID */
    UINT8                   Reserved;           /* Reserved, must be zero */
    UINT32                  GlobalIrqBase;      /* Global interrupt for SAPIC start */
    UINT64                  Address;            /* SAPIC physical address */

} ACPI_MADT_IO_SAPIC;


/* 7: Local Sapic */

typedef struct acpi_madt_local_sapic
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   ProcessorId;        /* ACPI processor id */
    UINT8                   Id;                 /* SAPIC ID */
    UINT8                   Eid;                /* SAPIC EID */
    UINT8                   Reserved[3];        /* Reserved, must be zero */
    UINT32                  LapicFlags;
    UINT32                  Uid;                /* Numeric UID - ACPI 3.0 */
    char                    UidString[1];       /* String UID  - ACPI 3.0 */

} ACPI_MADT_LOCAL_SAPIC;


/* 8: Platform Interrupt Source */

typedef struct acpi_madt_interrupt_source
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  IntiFlags;
    UINT8                   Type;               /* 1=PMI, 2=INIT, 3=corrected */
    UINT8                   Id;                 /* Processor ID */
    UINT8                   Eid;                /* Processor EID */
    UINT8                   IoSapicVector;      /* Vector value for PMI interrupts */
    UINT32                  GlobalIrq;          /* Global system interrupt */
    UINT32                  Flags;              /* Interrupt Source Flags */

} ACPI_MADT_INTERRUPT_SOURCE;

/* Masks for Flags field above */

#define ACPI_MADT_CPEI_OVERRIDE     (1)


/* 9: Processor Local X2APIC (ACPI 4.0) */

typedef struct acpi_madt_local_x2apic
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved - must be zero */
    UINT32                  LocalApicId;        /* Processor x2APIC ID  */
    UINT32                  LapicFlags;
    UINT32                  Uid;                /* ACPI processor UID */

} ACPI_MADT_LOCAL_X2APIC;


/* 10: Local X2APIC NMI (ACPI 4.0) */

typedef struct acpi_madt_local_x2apic_nmi
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  IntiFlags;
    UINT32                  Uid;                /* ACPI processor UID */
    UINT8                   Lint;               /* LINTn to which NMI is connected */
    UINT8                   Reserved[3];        /* Reserved - must be zero */

} ACPI_MADT_LOCAL_X2APIC_NMI;


/*
 * Common flags fields for MADT subtables
 */

/* MADT Local APIC flags (LapicFlags) */

#define ACPI_MADT_ENABLED           (1)         /* 00: Processor is usable if set */

/* MADT MPS INTI flags (IntiFlags) */

#define ACPI_MADT_POLARITY_MASK     (3)         /* 00-01: Polarity of APIC I/O input signals */
#define ACPI_MADT_TRIGGER_MASK      (3<<2)      /* 02-03: Trigger mode of APIC input signals */

/* Values for MPS INTI flags */

#define ACPI_MADT_POLARITY_CONFORMS       0
#define ACPI_MADT_POLARITY_ACTIVE_HIGH    1
#define ACPI_MADT_POLARITY_RESERVED       2
#define ACPI_MADT_POLARITY_ACTIVE_LOW     3

#define ACPI_MADT_TRIGGER_CONFORMS        (0)
#define ACPI_MADT_TRIGGER_EDGE            (1<<2)
#define ACPI_MADT_TRIGGER_RESERVED        (2<<2)
#define ACPI_MADT_TRIGGER_LEVEL           (3<<2)


/*******************************************************************************
 *
 * MSCT - Maximum System Characteristics Table (ACPI 4.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_msct
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  ProximityOffset;    /* Location of proximity info struct(s) */
    UINT32                  MaxProximityDomains;/* Max number of proximity domains */
    UINT32                  MaxClockDomains;    /* Max number of clock domains */
    UINT64                  MaxAddress;         /* Max physical address in system */

} ACPI_TABLE_MSCT;


/* Subtable - Maximum Proximity Domain Information. Version 1 */

typedef struct acpi_msct_proximity
{
    UINT8                   Revision;
    UINT8                   Length;
    UINT32                  RangeStart;         /* Start of domain range */
    UINT32                  RangeEnd;           /* End of domain range */
    UINT32                  ProcessorCapacity;
    UINT64                  MemoryCapacity;     /* In bytes */

} ACPI_MSCT_PROXIMITY;


/*******************************************************************************
 *
 * SBST - Smart Battery Specification Table
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_sbst
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  WarningLevel;
    UINT32                  LowLevel;
    UINT32                  CriticalLevel;

} ACPI_TABLE_SBST;


/*******************************************************************************
 *
 * SLIT - System Locality Distance Information Table
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_slit
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT64                  LocalityCount;
    UINT8                   Entry[1];           /* Real size = localities^2 */

} ACPI_TABLE_SLIT;


/*******************************************************************************
 *
 * SRAT - System Resource Affinity Table
 *        Version 3
 *
 ******************************************************************************/

typedef struct acpi_table_srat
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  TableRevision;      /* Must be value '1' */
    UINT64                  Reserved;           /* Reserved, must be zero */

} ACPI_TABLE_SRAT;

/* Values for subtable type in ACPI_SUBTABLE_HEADER */

enum AcpiSratType
{
    ACPI_SRAT_TYPE_CPU_AFFINITY         = 0,
    ACPI_SRAT_TYPE_MEMORY_AFFINITY      = 1,
    ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY  = 2,
    ACPI_SRAT_TYPE_RESERVED             = 3     /* 3 and greater are reserved */
};

/*
 * SRAT Sub-tables, correspond to Type in ACPI_SUBTABLE_HEADER
 */

/* 0: Processor Local APIC/SAPIC Affinity */

typedef struct acpi_srat_cpu_affinity
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   ProximityDomainLo;
    UINT8                   ApicId;
    UINT32                  Flags;
    UINT8                   LocalSapicEid;
    UINT8                   ProximityDomainHi[3];
    UINT32                  Reserved;           /* Reserved, must be zero */

} ACPI_SRAT_CPU_AFFINITY;

/* Flags */

#define ACPI_SRAT_CPU_USE_AFFINITY  (1)         /* 00: Use affinity structure */


/* 1: Memory Affinity */

typedef struct acpi_srat_mem_affinity
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  ProximityDomain;
    UINT16                  Reserved;           /* Reserved, must be zero */
    UINT64                  BaseAddress;
    UINT64                  Length;
    UINT32                  Reserved1;
    UINT32                  Flags;
    UINT64                  Reserved2;          /* Reserved, must be zero */

} ACPI_SRAT_MEM_AFFINITY;

/* Flags */

#define ACPI_SRAT_MEM_ENABLED       (1)         /* 00: Use affinity structure */
#define ACPI_SRAT_MEM_HOT_PLUGGABLE (1<<1)      /* 01: Memory region is hot pluggable */
#define ACPI_SRAT_MEM_NON_VOLATILE  (1<<2)      /* 02: Memory region is non-volatile */


/* 2: Processor Local X2_APIC Affinity (ACPI 4.0) */

typedef struct acpi_srat_x2apic_cpu_affinity
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved, must be zero */
    UINT32                  ProximityDomain;
    UINT32                  ApicId;
    UINT32                  Flags;
    UINT32                  ClockDomain;
    UINT32                  Reserved2;

} ACPI_SRAT_X2APIC_CPU_AFFINITY;

/* Flags for ACPI_SRAT_CPU_AFFINITY and ACPI_SRAT_X2APIC_CPU_AFFINITY */

#define ACPI_SRAT_CPU_ENABLED       (1)         /* 00: Use affinity structure */


/* Reset to default packing */

#pragma pack()

#endif /* __ACTBL1_H__ */
