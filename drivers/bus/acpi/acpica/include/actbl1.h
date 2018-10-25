/******************************************************************************
 *
 * Name: actbl1.h - Additional ACPI table definitions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2017, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

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
#define ACPI_SIG_HMAT           "HMAT"      /* Heterogeneous Memory Attributes Table */
#define ACPI_SIG_HEST           "HEST"      /* Hardware Error Source Table */
#define ACPI_SIG_MADT           "APIC"      /* Multiple APIC Description Table */
#define ACPI_SIG_MSCT           "MSCT"      /* Maximum System Characteristics Table */
#define ACPI_SIG_PDTT           "PDTT"      /* Platform Debug Trigger Table */
#define ACPI_SIG_PPTT           "PPTT"      /* Processor Properties Topology Table */
#define ACPI_SIG_SBST           "SBST"      /* Smart Battery Specification Table */
#define ACPI_SIG_SDEV           "SDEV"      /* Secure Devices table */
#define ACPI_SIG_SLIT           "SLIT"      /* System Locality Distance Information Table */
#define ACPI_SIG_SRAT           "SRAT"      /* System Resource Affinity Table */
#define ACPI_SIG_NFIT           "NFIT"      /* NVDIMM Firmware Interface Table */


/*
 * All tables must be byte-packed to match the ACPI specification, since
 * the tables are provided by the system BIOS.
 */
#pragma pack(1)

/*
 * Note: C bitfields are not used for this reason:
 *
 * "Bitfields are great and easy to read, but unfortunately the C language
 * does not specify the layout of bitfields in memory, which means they are
 * essentially useless for dealing with packed data in on-disk formats or
 * binary wire protocols." (Or ACPI tables and buffers.) "If you ask me,
 * this decision was a design error in C. Ritchie could have picked an order
 * and stuck with it." Norman Ramsey.
 * See http://stackoverflow.com/a/1053662/41661
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
    UINT64                  Address;            /* Physical address of the error region */

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
    ACPI_EINJ_BEGIN_OPERATION               = 0,
    ACPI_EINJ_GET_TRIGGER_TABLE             = 1,
    ACPI_EINJ_SET_ERROR_TYPE                = 2,
    ACPI_EINJ_GET_ERROR_TYPE                = 3,
    ACPI_EINJ_END_OPERATION                 = 4,
    ACPI_EINJ_EXECUTE_OPERATION             = 5,
    ACPI_EINJ_CHECK_BUSY_STATUS             = 6,
    ACPI_EINJ_GET_COMMAND_STATUS            = 7,
    ACPI_EINJ_SET_ERROR_TYPE_WITH_ADDRESS   = 8,
    ACPI_EINJ_GET_EXECUTE_TIMINGS           = 9,
    ACPI_EINJ_ACTION_RESERVED               = 10,    /* 10 and greater are reserved */
    ACPI_EINJ_TRIGGER_ERROR                 = 0xFF   /* Except for this value */
};

/* Values for Instruction field above */

enum AcpiEinjInstructions
{
    ACPI_EINJ_READ_REGISTER         = 0,
    ACPI_EINJ_READ_REGISTER_VALUE   = 1,
    ACPI_EINJ_WRITE_REGISTER        = 2,
    ACPI_EINJ_WRITE_REGISTER_VALUE  = 3,
    ACPI_EINJ_NOOP                  = 4,
    ACPI_EINJ_FLUSH_CACHELINE       = 5,
    ACPI_EINJ_INSTRUCTION_RESERVED  = 6     /* 6 and greater are reserved */
};

typedef struct acpi_einj_error_type_with_addr
{
    UINT32                  ErrorType;
    UINT32                  VendorStructOffset;
    UINT32                  Flags;
    UINT32                  ApicId;
    UINT64                  Address;
    UINT64                  Range;
    UINT32                  PcieId;

} ACPI_EINJ_ERROR_TYPE_WITH_ADDR;

typedef struct acpi_einj_vendor
{
    UINT32                  Length;
    UINT32                  PcieId;
    UINT16                  VendorId;
    UINT16                  DeviceId;
    UINT8                   RevisionId;
    UINT8                   Reserved[3];

} ACPI_EINJ_VENDOR;


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
#define ACPI_EINJ_VENDOR_DEFINED            (1<<31)


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
    ACPI_ERST_EXECUTE_TIMINGS       = 16,
    ACPI_ERST_ACTION_RESERVED       = 17    /* 17 and greater are reserved */
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
    ACPI_HEST_TYPE_GENERIC_ERROR_V2     = 10,
    ACPI_HEST_TYPE_IA32_DEFERRED_CHECK  = 11,
    ACPI_HEST_TYPE_RESERVED             = 12    /* 12 and greater are reserved */
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
    UINT32                  Bus;                    /* Bus and Segment numbers */
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
#define ACPI_HEST_GHES_ASSIST           (1<<2)

/*
 * Macros to access the bus/segment numbers in Bus field above:
 *  Bus number is encoded in bits 7:0
 *  Segment number is encoded in bits 23:8
 */
#define ACPI_HEST_BUS(Bus)              ((Bus) & 0xFF)
#define ACPI_HEST_SEGMENT(Bus)          (((Bus) >> 8) & 0xFFFF)


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
    ACPI_HEST_NOTIFY_POLLED             = 0,
    ACPI_HEST_NOTIFY_EXTERNAL           = 1,
    ACPI_HEST_NOTIFY_LOCAL              = 2,
    ACPI_HEST_NOTIFY_SCI                = 3,
    ACPI_HEST_NOTIFY_NMI                = 4,
    ACPI_HEST_NOTIFY_CMCI               = 5,    /* ACPI 5.0 */
    ACPI_HEST_NOTIFY_MCE                = 6,    /* ACPI 5.0 */
    ACPI_HEST_NOTIFY_GPIO               = 7,    /* ACPI 6.0 */
    ACPI_HEST_NOTIFY_SEA                = 8,    /* ACPI 6.1 */
    ACPI_HEST_NOTIFY_SEI                = 9,    /* ACPI 6.1 */
    ACPI_HEST_NOTIFY_GSIV               = 10,   /* ACPI 6.1 */
    ACPI_HEST_NOTIFY_SOFTWARE_DELEGATED = 11,   /* ACPI 6.2 */
    ACPI_HEST_NOTIFY_RESERVED           = 12    /* 12 and greater are reserved */
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
    UINT8                   Flags;              /* See flags ACPI_HEST_GLOBAL, etc. above */
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
    UINT8                   Flags;              /* See flags ACPI_HEST_GLOBAL, etc. above */
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


/* 10: Generic Hardware Error Source, version 2 */

typedef struct acpi_hest_generic_v2
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
    ACPI_GENERIC_ADDRESS    ReadAckRegister;
    UINT64                  ReadAckPreserve;
    UINT64                  ReadAckWrite;

} ACPI_HEST_GENERIC_V2;


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

/* Extension for revision 0x0300 */

typedef struct acpi_hest_generic_data_v300
{
    UINT8                   SectionType[16];
    UINT32                  ErrorSeverity;
    UINT16                  Revision;
    UINT8                   ValidationBits;
    UINT8                   Flags;
    UINT32                  ErrorDataLength;
    UINT8                   FruId[16];
    UINT8                   FruText[20];
    UINT64                  TimeStamp;

} ACPI_HEST_GENERIC_DATA_V300;

/* Values for ErrorSeverity above */

#define ACPI_HEST_GEN_ERROR_RECOVERABLE     0
#define ACPI_HEST_GEN_ERROR_FATAL           1
#define ACPI_HEST_GEN_ERROR_CORRECTED       2
#define ACPI_HEST_GEN_ERROR_NONE            3

/* Flags for ValidationBits above */

#define ACPI_HEST_GEN_VALID_FRU_ID          (1)
#define ACPI_HEST_GEN_VALID_FRU_STRING      (1<<1)
#define ACPI_HEST_GEN_VALID_TIMESTAMP       (1<<2)


/* 11: IA32 Deferred Machine Check Exception (ACPI 6.2) */

typedef struct acpi_hest_ia_deferred_check
{
    ACPI_HEST_HEADER        Header;
    UINT16                  Reserved1;
    UINT8                   Flags;              /* See flags ACPI_HEST_GLOBAL, etc. above */
    UINT8                   Enabled;
    UINT32                  RecordsToPreallocate;
    UINT32                  MaxSectionsPerRecord;
    ACPI_HEST_NOTIFY        Notify;
    UINT8                   NumHardwareBanks;
    UINT8                   Reserved2[3];

} ACPI_HEST_IA_DEFERRED_CHECK;


/*******************************************************************************
 *
 * HMAT - Heterogeneous Memory Attributes Table (ACPI 6.2)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_hmat
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Reserved;

} ACPI_TABLE_HMAT;


/* Values for HMAT structure types */

enum AcpiHmatType
{
    ACPI_HMAT_TYPE_ADDRESS_RANGE        = 0,   /* Memory subystem address range */
    ACPI_HMAT_TYPE_LOCALITY             = 1,   /* System locality latency and bandwidth information */
    ACPI_HMAT_TYPE_CACHE                = 2,   /* Memory side cache information */
    ACPI_HMAT_TYPE_RESERVED             = 3    /* 3 and greater are reserved */
};

typedef struct acpi_hmat_structure
{
    UINT16                  Type;
    UINT16                  Reserved;
    UINT32                  Length;

} ACPI_HMAT_STRUCTURE;


/*
 * HMAT Structures, correspond to Type in ACPI_HMAT_STRUCTURE
 */

/* 0: Memory subystem address range */

typedef struct acpi_hmat_address_range
{
    ACPI_HMAT_STRUCTURE     Header;
    UINT16                  Flags;
    UINT16                  Reserved1;
    UINT32                  ProcessorPD;            /* Processor proximity domain */
    UINT32                  MemoryPD;               /* Memory proximity domain */
    UINT32                  Reserved2;
    UINT64                  PhysicalAddressBase;    /* Physical address range base */
    UINT64                  PhysicalAddressLength;  /* Physical address range length */

} ACPI_HMAT_ADDRESS_RANGE;

/* Masks for Flags field above */

#define ACPI_HMAT_PROCESSOR_PD_VALID    (1)     /* 1: ProcessorPD field is valid */
#define ACPI_HMAT_MEMORY_PD_VALID       (1<<1)  /* 1: MemoryPD field is valid */
#define ACPI_HMAT_RESERVATION_HINT      (1<<2)  /* 1: Reservation hint */


/* 1: System locality latency and bandwidth information */

typedef struct acpi_hmat_locality
{
    ACPI_HMAT_STRUCTURE     Header;
    UINT8                   Flags;
    UINT8                   DataType;
    UINT16                  Reserved1;
    UINT32                  NumberOfInitiatorPDs;
    UINT32                  NumberOfTargetPDs;
    UINT32                  Reserved2;
    UINT64                  EntryBaseUnit;

} ACPI_HMAT_LOCALITY;

/* Masks for Flags field above */

#define ACPI_HMAT_MEMORY_HIERARCHY  (0x0F)

/* Values for Memory Hierarchy flag */

#define ACPI_HMAT_MEMORY            0
#define ACPI_HMAT_LAST_LEVEL_CACHE  1
#define ACPI_HMAT_1ST_LEVEL_CACHE   2
#define ACPI_HMAT_2ND_LEVEL_CACHE   3
#define ACPI_HMAT_3RD_LEVEL_CACHE   4

/* Values for DataType field above */

#define ACPI_HMAT_ACCESS_LATENCY    0
#define ACPI_HMAT_READ_LATENCY      1
#define ACPI_HMAT_WRITE_LATENCY     2
#define ACPI_HMAT_ACCESS_BANDWIDTH  3
#define ACPI_HMAT_READ_BANDWIDTH    4
#define ACPI_HMAT_WRITE_BANDWIDTH   5


/* 2: Memory side cache information */

typedef struct acpi_hmat_cache
{
    ACPI_HMAT_STRUCTURE     Header;
    UINT32                  MemoryPD;
    UINT32                  Reserved1;
    UINT64                  CacheSize;
    UINT32                  CacheAttributes;
    UINT16                  Reserved2;
    UINT16                  NumberOfSMBIOSHandles;

} ACPI_HMAT_CACHE;

/* Masks for CacheAttributes field above */

#define ACPI_HMAT_TOTAL_CACHE_LEVEL     (0x0000000F)
#define ACPI_HMAT_CACHE_LEVEL           (0x000000F0)
#define ACPI_HMAT_CACHE_ASSOCIATIVITY   (0x00000F00)
#define ACPI_HMAT_WRITE_POLICY          (0x0000F000)
#define ACPI_HMAT_CACHE_LINE_SIZE       (0xFFFF0000)

/* Values for cache associativity flag */

#define ACPI_HMAT_CA_NONE                     (0)
#define ACPI_HMAT_CA_DIRECT_MAPPED            (1)
#define ACPI_HMAT_CA_COMPLEX_CACHE_INDEXING   (2)

/* Values for write policy flag */

#define ACPI_HMAT_CP_NONE   (0)
#define ACPI_HMAT_CP_WB     (1)
#define ACPI_HMAT_CP_WT     (2)


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

#define ACPI_MADT_DUAL_PIC          1
#define ACPI_MADT_MULTIPLE_APIC     0


/* Values for MADT subtable type in ACPI_SUBTABLE_HEADER */

enum AcpiMadtType
{
    ACPI_MADT_TYPE_LOCAL_APIC               = 0,
    ACPI_MADT_TYPE_IO_APIC                  = 1,
    ACPI_MADT_TYPE_INTERRUPT_OVERRIDE       = 2,
    ACPI_MADT_TYPE_NMI_SOURCE               = 3,
    ACPI_MADT_TYPE_LOCAL_APIC_NMI           = 4,
    ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE      = 5,
    ACPI_MADT_TYPE_IO_SAPIC                 = 6,
    ACPI_MADT_TYPE_LOCAL_SAPIC              = 7,
    ACPI_MADT_TYPE_INTERRUPT_SOURCE         = 8,
    ACPI_MADT_TYPE_LOCAL_X2APIC             = 9,
    ACPI_MADT_TYPE_LOCAL_X2APIC_NMI         = 10,
    ACPI_MADT_TYPE_GENERIC_INTERRUPT        = 11,
    ACPI_MADT_TYPE_GENERIC_DISTRIBUTOR      = 12,
    ACPI_MADT_TYPE_GENERIC_MSI_FRAME        = 13,
    ACPI_MADT_TYPE_GENERIC_REDISTRIBUTOR    = 14,
    ACPI_MADT_TYPE_GENERIC_TRANSLATOR       = 15,
    ACPI_MADT_TYPE_RESERVED                 = 16    /* 16 and greater are reserved */
};


/*
 * MADT Subtables, correspond to Type in ACPI_SUBTABLE_HEADER
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


/* 11: Generic Interrupt (ACPI 5.0 + ACPI 6.0 changes) */

typedef struct acpi_madt_generic_interrupt
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved - must be zero */
    UINT32                  CpuInterfaceNumber;
    UINT32                  Uid;
    UINT32                  Flags;
    UINT32                  ParkingVersion;
    UINT32                  PerformanceInterrupt;
    UINT64                  ParkedAddress;
    UINT64                  BaseAddress;
    UINT64                  GicvBaseAddress;
    UINT64                  GichBaseAddress;
    UINT32                  VgicInterrupt;
    UINT64                  GicrBaseAddress;
    UINT64                  ArmMpidr;
    UINT8                   EfficiencyClass;
    UINT8                   Reserved2[3];

} ACPI_MADT_GENERIC_INTERRUPT;

/* Masks for Flags field above */

/* ACPI_MADT_ENABLED                    (1)      Processor is usable if set */
#define ACPI_MADT_PERFORMANCE_IRQ_MODE  (1<<1)  /* 01: Performance Interrupt Mode */
#define ACPI_MADT_VGIC_IRQ_MODE         (1<<2)  /* 02: VGIC Maintenance Interrupt mode */


/* 12: Generic Distributor (ACPI 5.0 + ACPI 6.0 changes) */

typedef struct acpi_madt_generic_distributor
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved - must be zero */
    UINT32                  GicId;
    UINT64                  BaseAddress;
    UINT32                  GlobalIrqBase;
    UINT8                   Version;
    UINT8                   Reserved2[3];       /* Reserved - must be zero */

} ACPI_MADT_GENERIC_DISTRIBUTOR;

/* Values for Version field above */

enum AcpiMadtGicVersion
{
    ACPI_MADT_GIC_VERSION_NONE          = 0,
    ACPI_MADT_GIC_VERSION_V1            = 1,
    ACPI_MADT_GIC_VERSION_V2            = 2,
    ACPI_MADT_GIC_VERSION_V3            = 3,
    ACPI_MADT_GIC_VERSION_V4            = 4,
    ACPI_MADT_GIC_VERSION_RESERVED      = 5     /* 5 and greater are reserved */
};


/* 13: Generic MSI Frame (ACPI 5.1) */

typedef struct acpi_madt_generic_msi_frame
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* Reserved - must be zero */
    UINT32                  MsiFrameId;
    UINT64                  BaseAddress;
    UINT32                  Flags;
    UINT16                  SpiCount;
    UINT16                  SpiBase;

} ACPI_MADT_GENERIC_MSI_FRAME;

/* Masks for Flags field above */

#define ACPI_MADT_OVERRIDE_SPI_VALUES   (1)


/* 14: Generic Redistributor (ACPI 5.1) */

typedef struct acpi_madt_generic_redistributor
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* reserved - must be zero */
    UINT64                  BaseAddress;
    UINT32                  Length;

} ACPI_MADT_GENERIC_REDISTRIBUTOR;


/* 15: Generic Translator (ACPI 6.0) */

typedef struct acpi_madt_generic_translator
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;           /* reserved - must be zero */
    UINT32                  TranslationId;
    UINT64                  BaseAddress;
    UINT32                  Reserved2;

} ACPI_MADT_GENERIC_TRANSLATOR;


/*
 * Common flags fields for MADT subtables
 */

/* MADT Local APIC flags */

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
 * NFIT - NVDIMM Interface Table (ACPI 6.0+)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_nfit
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Reserved;           /* Reserved, must be zero */

} ACPI_TABLE_NFIT;

/* Subtable header for NFIT */

typedef struct acpi_nfit_header
{
    UINT16                   Type;
    UINT16                   Length;

} ACPI_NFIT_HEADER;


/* Values for subtable type in ACPI_NFIT_HEADER */

enum AcpiNfitType
{
    ACPI_NFIT_TYPE_SYSTEM_ADDRESS       = 0,
    ACPI_NFIT_TYPE_MEMORY_MAP           = 1,
    ACPI_NFIT_TYPE_INTERLEAVE           = 2,
    ACPI_NFIT_TYPE_SMBIOS               = 3,
    ACPI_NFIT_TYPE_CONTROL_REGION       = 4,
    ACPI_NFIT_TYPE_DATA_REGION          = 5,
    ACPI_NFIT_TYPE_FLUSH_ADDRESS        = 6,
    ACPI_NFIT_TYPE_CAPABILITIES         = 7,
    ACPI_NFIT_TYPE_RESERVED             = 8     /* 8 and greater are reserved */
};

/*
 * NFIT Subtables
 */

/* 0: System Physical Address Range Structure */

typedef struct acpi_nfit_system_address
{
    ACPI_NFIT_HEADER        Header;
    UINT16                  RangeIndex;
    UINT16                  Flags;
    UINT32                  Reserved;           /* Reserved, must be zero */
    UINT32                  ProximityDomain;
    UINT8                   RangeGuid[16];
    UINT64                  Address;
    UINT64                  Length;
    UINT64                  MemoryMapping;

} ACPI_NFIT_SYSTEM_ADDRESS;

/* Flags */

#define ACPI_NFIT_ADD_ONLINE_ONLY       (1)     /* 00: Add/Online Operation Only */
#define ACPI_NFIT_PROXIMITY_VALID       (1<<1)  /* 01: Proximity Domain Valid */

/* Range Type GUIDs appear in the include/acuuid.h file */


/* 1: Memory Device to System Address Range Map Structure */

typedef struct acpi_nfit_memory_map
{
    ACPI_NFIT_HEADER        Header;
    UINT32                  DeviceHandle;
    UINT16                  PhysicalId;
    UINT16                  RegionId;
    UINT16                  RangeIndex;
    UINT16                  RegionIndex;
    UINT64                  RegionSize;
    UINT64                  RegionOffset;
    UINT64                  Address;
    UINT16                  InterleaveIndex;
    UINT16                  InterleaveWays;
    UINT16                  Flags;
    UINT16                  Reserved;           /* Reserved, must be zero */

} ACPI_NFIT_MEMORY_MAP;

/* Flags */

#define ACPI_NFIT_MEM_SAVE_FAILED       (1)     /* 00: Last SAVE to Memory Device failed */
#define ACPI_NFIT_MEM_RESTORE_FAILED    (1<<1)  /* 01: Last RESTORE from Memory Device failed */
#define ACPI_NFIT_MEM_FLUSH_FAILED      (1<<2)  /* 02: Platform flush failed */
#define ACPI_NFIT_MEM_NOT_ARMED         (1<<3)  /* 03: Memory Device is not armed */
#define ACPI_NFIT_MEM_HEALTH_OBSERVED   (1<<4)  /* 04: Memory Device observed SMART/health events */
#define ACPI_NFIT_MEM_HEALTH_ENABLED    (1<<5)  /* 05: SMART/health events enabled */
#define ACPI_NFIT_MEM_MAP_FAILED        (1<<6)  /* 06: Mapping to SPA failed */


/* 2: Interleave Structure */

typedef struct acpi_nfit_interleave
{
    ACPI_NFIT_HEADER        Header;
    UINT16                  InterleaveIndex;
    UINT16                  Reserved;           /* Reserved, must be zero */
    UINT32                  LineCount;
    UINT32                  LineSize;
    UINT32                  LineOffset[1];      /* Variable length */

} ACPI_NFIT_INTERLEAVE;


/* 3: SMBIOS Management Information Structure */

typedef struct acpi_nfit_smbios
{
    ACPI_NFIT_HEADER        Header;
    UINT32                  Reserved;           /* Reserved, must be zero */
    UINT8                   Data[1];            /* Variable length */

} ACPI_NFIT_SMBIOS;


/* 4: NVDIMM Control Region Structure */

typedef struct acpi_nfit_control_region
{
    ACPI_NFIT_HEADER        Header;
    UINT16                  RegionIndex;
    UINT16                  VendorId;
    UINT16                  DeviceId;
    UINT16                  RevisionId;
    UINT16                  SubsystemVendorId;
    UINT16                  SubsystemDeviceId;
    UINT16                  SubsystemRevisionId;
    UINT8                   ValidFields;
    UINT8                   ManufacturingLocation;
    UINT16                  ManufacturingDate;
    UINT8                   Reserved[2];        /* Reserved, must be zero */
    UINT32                  SerialNumber;
    UINT16                  Code;
    UINT16                  Windows;
    UINT64                  WindowSize;
    UINT64                  CommandOffset;
    UINT64                  CommandSize;
    UINT64                  StatusOffset;
    UINT64                  StatusSize;
    UINT16                  Flags;
    UINT8                   Reserved1[6];       /* Reserved, must be zero */

} ACPI_NFIT_CONTROL_REGION;

/* Flags */

#define ACPI_NFIT_CONTROL_BUFFERED          (1)     /* Block Data Windows implementation is buffered */

/* ValidFields bits */

#define ACPI_NFIT_CONTROL_MFG_INFO_VALID    (1)     /* Manufacturing fields are valid */


/* 5: NVDIMM Block Data Window Region Structure */

typedef struct acpi_nfit_data_region
{
    ACPI_NFIT_HEADER        Header;
    UINT16                  RegionIndex;
    UINT16                  Windows;
    UINT64                  Offset;
    UINT64                  Size;
    UINT64                  Capacity;
    UINT64                  StartAddress;

} ACPI_NFIT_DATA_REGION;


/* 6: Flush Hint Address Structure */

typedef struct acpi_nfit_flush_address
{
    ACPI_NFIT_HEADER        Header;
    UINT32                  DeviceHandle;
    UINT16                  HintCount;
    UINT8                   Reserved[6];        /* Reserved, must be zero */
    UINT64                  HintAddress[1];     /* Variable length */

} ACPI_NFIT_FLUSH_ADDRESS;


/* 7: Platform Capabilities Structure */

typedef struct acpi_nfit_capabilities
{
    ACPI_NFIT_HEADER        Header;
    UINT8                   HighestCapability;
    UINT8                   Reserved[3];       /* Reserved, must be zero */
    UINT32                  Capabilities;
    UINT32                  Reserved2;

} ACPI_NFIT_CAPABILITIES;

/* Capabilities Flags */

#define ACPI_NFIT_CAPABILITY_CACHE_FLUSH       (1)     /* 00: Cache Flush to NVDIMM capable */
#define ACPI_NFIT_CAPABILITY_MEM_FLUSH         (1<<1)  /* 01: Memory Flush to NVDIMM capable */
#define ACPI_NFIT_CAPABILITY_MEM_MIRRORING     (1<<2)  /* 02: Memory Mirroring capable */


/*
 * NFIT/DVDIMM device handle support - used as the _ADR for each NVDIMM
 */
typedef struct nfit_device_handle
{
    UINT32                  Handle;

} NFIT_DEVICE_HANDLE;

/* Device handle construction and extraction macros */

#define ACPI_NFIT_DIMM_NUMBER_MASK              0x0000000F
#define ACPI_NFIT_CHANNEL_NUMBER_MASK           0x000000F0
#define ACPI_NFIT_MEMORY_ID_MASK                0x00000F00
#define ACPI_NFIT_SOCKET_ID_MASK                0x0000F000
#define ACPI_NFIT_NODE_ID_MASK                  0x0FFF0000

#define ACPI_NFIT_DIMM_NUMBER_OFFSET            0
#define ACPI_NFIT_CHANNEL_NUMBER_OFFSET         4
#define ACPI_NFIT_MEMORY_ID_OFFSET              8
#define ACPI_NFIT_SOCKET_ID_OFFSET              12
#define ACPI_NFIT_NODE_ID_OFFSET                16

/* Macro to construct a NFIT/NVDIMM device handle */

#define ACPI_NFIT_BUILD_DEVICE_HANDLE(dimm, channel, memory, socket, node) \
    ((dimm)                                         | \
    ((channel) << ACPI_NFIT_CHANNEL_NUMBER_OFFSET)  | \
    ((memory)  << ACPI_NFIT_MEMORY_ID_OFFSET)       | \
    ((socket)  << ACPI_NFIT_SOCKET_ID_OFFSET)       | \
    ((node)    << ACPI_NFIT_NODE_ID_OFFSET))

/* Macros to extract individual fields from a NFIT/NVDIMM device handle */

#define ACPI_NFIT_GET_DIMM_NUMBER(handle) \
    ((handle) & ACPI_NFIT_DIMM_NUMBER_MASK)

#define ACPI_NFIT_GET_CHANNEL_NUMBER(handle) \
    (((handle) & ACPI_NFIT_CHANNEL_NUMBER_MASK) >> ACPI_NFIT_CHANNEL_NUMBER_OFFSET)

#define ACPI_NFIT_GET_MEMORY_ID(handle) \
    (((handle) & ACPI_NFIT_MEMORY_ID_MASK)      >> ACPI_NFIT_MEMORY_ID_OFFSET)

#define ACPI_NFIT_GET_SOCKET_ID(handle) \
    (((handle) & ACPI_NFIT_SOCKET_ID_MASK)      >> ACPI_NFIT_SOCKET_ID_OFFSET)

#define ACPI_NFIT_GET_NODE_ID(handle) \
    (((handle) & ACPI_NFIT_NODE_ID_MASK)        >> ACPI_NFIT_NODE_ID_OFFSET)


/*******************************************************************************
 *
 * PDTT - Platform Debug Trigger Table (ACPI 6.2)
 *        Version 0
 *
 ******************************************************************************/

typedef struct acpi_table_pdtt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   TriggerCount;
    UINT8                   Reserved[3];
    UINT32                  ArrayOffset;

} ACPI_TABLE_PDTT;


/*
 * PDTT Communication Channel Identifier Structure.
 * The number of these structures is defined by TriggerCount above,
 * starting at ArrayOffset.
 */
typedef struct acpi_pdtt_channel
{
    UINT8                   SubchannelId;
    UINT8                   Flags;

} ACPI_PDTT_CHANNEL;

/* Flags for above */

#define ACPI_PDTT_RUNTIME_TRIGGER           (1)
#define ACPI_PDTT_WAIT_COMPLETION           (1<<1)


/*******************************************************************************
 *
 * PPTT - Processor Properties Topology Table (ACPI 6.2)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_pptt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_PPTT;

/* Values for Type field above */

enum AcpiPpttType
{
    ACPI_PPTT_TYPE_PROCESSOR            = 0,
    ACPI_PPTT_TYPE_CACHE                = 1,
    ACPI_PPTT_TYPE_ID                   = 2,
    ACPI_PPTT_TYPE_RESERVED             = 3
};


/* 0: Processor Hierarchy Node Structure */

typedef struct acpi_pptt_processor
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;
    UINT32                  Flags;
    UINT32                  Parent;
    UINT32                  AcpiProcessorId;
    UINT32                  NumberOfPrivResources;

} ACPI_PPTT_PROCESSOR;

/* Flags */

#define ACPI_PPTT_PHYSICAL_PACKAGE          (1)     /* Physical package */
#define ACPI_PPTT_ACPI_PROCESSOR_ID_VALID   (2)     /* ACPI Processor ID valid */


/* 1: Cache Type Structure */

typedef struct acpi_pptt_cache
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;
    UINT32                  Flags;
    UINT32                  NextLevelOfCache;
    UINT32                  Size;
    UINT32                  NumberOfSets;
    UINT8                   Associativity;
    UINT8                   Attributes;
    UINT16                  LineSize;

} ACPI_PPTT_CACHE;

/* Flags */

#define ACPI_PPTT_SIZE_PROPERTY_VALID       (1)     /* Physical property valid */
#define ACPI_PPTT_NUMBER_OF_SETS_VALID      (1<<1)  /* Number of sets valid */
#define ACPI_PPTT_ASSOCIATIVITY_VALID       (1<<2)  /* Associativity valid */
#define ACPI_PPTT_ALLOCATION_TYPE_VALID     (1<<3)  /* Allocation type valid */
#define ACPI_PPTT_CACHE_TYPE_VALID          (1<<4)  /* Cache type valid */
#define ACPI_PPTT_WRITE_POLICY_VALID        (1<<5)  /* Write policy valid */
#define ACPI_PPTT_LINE_SIZE_VALID           (1<<6)  /* Line size valid */

/* Masks for Attributes */

#define ACPI_PPTT_MASK_ALLOCATION_TYPE      (0x03)  /* Allocation type */
#define ACPI_PPTT_MASK_CACHE_TYPE           (0x0C)  /* Cache type */
#define ACPI_PPTT_MASK_WRITE_POLICY         (0x10)  /* Write policy */

/* Attributes describing cache */
#define ACPI_PPTT_CACHE_READ_ALLOCATE       (0x0)   /* Cache line is allocated on read */
#define ACPI_PPTT_CACHE_WRITE_ALLOCATE      (0x01)  /* Cache line is allocated on write */
#define ACPI_PPTT_CACHE_RW_ALLOCATE         (0x02)  /* Cache line is allocated on read and write */
#define ACPI_PPTT_CACHE_RW_ALLOCATE_ALT     (0x03)  /* Alternate representation of above */

#define ACPI_PPTT_CACHE_TYPE_DATA           (0x0)   /* Data cache */
#define ACPI_PPTT_CACHE_TYPE_INSTR          (1<<2)  /* Instruction cache */
#define ACPI_PPTT_CACHE_TYPE_UNIFIED        (2<<2)  /* Unified I & D cache */
#define ACPI_PPTT_CACHE_TYPE_UNIFIED_ALT    (3<<2)  /* Alternate representation of above */

#define ACPI_PPTT_CACHE_POLICY_WB           (0x0)   /* Cache is write back */
#define ACPI_PPTT_CACHE_POLICY_WT           (1<<4)  /* Cache is write through */

/* 2: ID Structure */

typedef struct acpi_pptt_id
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Reserved;
    UINT32                  VendorId;
    UINT64                  Level1Id;
    UINT64                  Level2Id;
    UINT16                  MajorRev;
    UINT16                  MinorRev;
    UINT16                  SpinRev;

} ACPI_PPTT_ID;


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
 * SDEV - Secure Devices Table (ACPI 6.2)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_sdev
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_SDEV;


typedef struct acpi_sdev_header
{
    UINT8                   Type;
    UINT8                   Flags;
    UINT16                  Length;

} ACPI_SDEV_HEADER;


/* Values for subtable type above */

enum AcpiSdevType
{
    ACPI_SDEV_TYPE_NAMESPACE_DEVICE     = 0,
    ACPI_SDEV_TYPE_PCIE_ENDPOINT_DEVICE = 1,
    ACPI_SDEV_TYPE_RESERVED             = 2     /* 2 and greater are reserved */
};

/* Values for flags above */

#define ACPI_SDEV_HANDOFF_TO_UNSECURE_OS    (1)

/*
 * SDEV subtables
 */

/* 0: Namespace Device Based Secure Device Structure */

typedef struct acpi_sdev_namespace
{
    ACPI_SDEV_HEADER        Header;
    UINT16                  DeviceIdOffset;
    UINT16                  DeviceIdLength;
    UINT16                  VendorDataOffset;
    UINT16                  VendorDataLength;

} ACPI_SDEV_NAMESPACE;

/* 1: PCIe Endpoint Device Based Device Structure */

typedef struct acpi_sdev_pcie
{
    ACPI_SDEV_HEADER        Header;
    UINT16                  Segment;
    UINT16                  StartBus;
    UINT16                  PathOffset;
    UINT16                  PathLength;
    UINT16                  VendorDataOffset;
    UINT16                  VendorDataLength;

} ACPI_SDEV_PCIE;

/* 1a: PCIe Endpoint path entry */

typedef struct acpi_sdev_pcie_path
{
    UINT8                   Device;
    UINT8                   Function;

} ACPI_SDEV_PCIE_PATH;


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
    ACPI_SRAT_TYPE_GICC_AFFINITY        = 3,
    ACPI_SRAT_TYPE_GIC_ITS_AFFINITY     = 4,    /* ACPI 6.2 */
    ACPI_SRAT_TYPE_RESERVED             = 5     /* 5 and greater are reserved */
};

/*
 * SRAT Subtables, correspond to Type in ACPI_SUBTABLE_HEADER
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
    UINT32                  ClockDomain;

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


/* 3: GICC Affinity (ACPI 5.1) */

typedef struct acpi_srat_gicc_affinity
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  ProximityDomain;
    UINT32                  AcpiProcessorUid;
    UINT32                  Flags;
    UINT32                  ClockDomain;

} ACPI_SRAT_GICC_AFFINITY;

/* Flags for ACPI_SRAT_GICC_AFFINITY */

#define ACPI_SRAT_GICC_ENABLED     (1)         /* 00: Use affinity structure */


/* 4: GCC ITS Affinity (ACPI 6.2) */

typedef struct acpi_srat_gic_its_affinity
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  ProximityDomain;
    UINT16                  Reserved;
    UINT32                  ItsId;

} ACPI_SRAT_GIC_ITS_AFFINITY;


/* Reset to default packing */

#pragma pack()

#endif /* __ACTBL1_H__ */
