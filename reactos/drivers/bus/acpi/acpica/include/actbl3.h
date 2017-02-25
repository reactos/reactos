/******************************************************************************
 *
 * Name: actbl3.h - ACPI Table Definitions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
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

#ifndef __ACTBL3_H__
#define __ACTBL3_H__


/*******************************************************************************
 *
 * Additional ACPI Tables (3)
 *
 * These tables are not consumed directly by the ACPICA subsystem, but are
 * included here to support device drivers and the AML disassembler.
 *
 * In general, the tables in this file are fully defined within the ACPI
 * specification.
 *
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_BGRT           "BGRT"      /* Boot Graphics Resource Table */
#define ACPI_SIG_DRTM           "DRTM"      /* Dynamic Root of Trust for Measurement table */
#define ACPI_SIG_FPDT           "FPDT"      /* Firmware Performance Data Table */
#define ACPI_SIG_GTDT           "GTDT"      /* Generic Timer Description Table */
#define ACPI_SIG_MPST           "MPST"      /* Memory Power State Table */
#define ACPI_SIG_PCCT           "PCCT"      /* Platform Communications Channel Table */
#define ACPI_SIG_PMTT           "PMTT"      /* Platform Memory Topology Table */
#define ACPI_SIG_RASF           "RASF"      /* RAS Feature table */
#define ACPI_SIG_STAO           "STAO"      /* Status Override table */
#define ACPI_SIG_WPBT           "WPBT"      /* Windows Platform Binary Table */
#define ACPI_SIG_XENV           "XENV"      /* Xen Environment table */

#define ACPI_SIG_S3PT           "S3PT"      /* S3 Performance (sub)Table */
#define ACPI_SIG_PCCS           "PCC"       /* PCC Shared Memory Region */

/* Reserved table signatures */

#define ACPI_SIG_MATR           "MATR"      /* Memory Address Translation Table */
#define ACPI_SIG_MSDM           "MSDM"      /* Microsoft Data Management Table */

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
 * BGRT - Boot Graphics Resource Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_bgrt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  Version;
    UINT8                   Status;
    UINT8                   ImageType;
    UINT64                  ImageAddress;
    UINT32                  ImageOffsetX;
    UINT32                  ImageOffsetY;

} ACPI_TABLE_BGRT;


/*******************************************************************************
 *
 * DRTM - Dynamic Root of Trust for Measurement table
 * Conforms to "TCG D-RTM Architecture" June 17 2013, Version 1.0.0
 * Table version 1
 *
 ******************************************************************************/

typedef struct acpi_table_drtm
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT64                  EntryBaseAddress;
    UINT64                  EntryLength;
    UINT32                  EntryAddress32;
    UINT64                  EntryAddress64;
    UINT64                  ExitAddress;
    UINT64                  LogAreaAddress;
    UINT32                  LogAreaLength;
    UINT64                  ArchDependentAddress;
    UINT32                  Flags;

} ACPI_TABLE_DRTM;

/* Flag Definitions for above */

#define ACPI_DRTM_ACCESS_ALLOWED            (1)
#define ACPI_DRTM_ENABLE_GAP_CODE           (1<<1)
#define ACPI_DRTM_INCOMPLETE_MEASUREMENTS   (1<<2)
#define ACPI_DRTM_AUTHORITY_ORDER           (1<<3)


/* 1) Validated Tables List (64-bit addresses) */

typedef struct acpi_drtm_vtable_list
{
    UINT32                  ValidatedTableCount;
    UINT64                  ValidatedTables[1];

} ACPI_DRTM_VTABLE_LIST;

/* 2) Resources List (of Resource Descriptors) */

/* Resource Descriptor */

typedef struct acpi_drtm_resource
{
    UINT8                   Size[7];
    UINT8                   Type;
    UINT64                  Address;

} ACPI_DRTM_RESOURCE;

typedef struct acpi_drtm_resource_list
{
    UINT32                  ResourceCount;
    ACPI_DRTM_RESOURCE      Resources[1];

} ACPI_DRTM_RESOURCE_LIST;

/* 3) Platform-specific Identifiers List */

typedef struct acpi_drtm_dps_id
{
    UINT32                  DpsIdLength;
    UINT8                   DpsId[16];

} ACPI_DRTM_DPS_ID;


/*******************************************************************************
 *
 * FPDT - Firmware Performance Data Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_fpdt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_FPDT;


/* FPDT subtable header (Performance Record Structure) */

typedef struct acpi_fpdt_header
{
    UINT16                  Type;
    UINT8                   Length;
    UINT8                   Revision;

} ACPI_FPDT_HEADER;

/* Values for Type field above */

enum AcpiFpdtType
{
    ACPI_FPDT_TYPE_BOOT                 = 0,
    ACPI_FPDT_TYPE_S3PERF               = 1
};


/*
 * FPDT subtables
 */

/* 0: Firmware Basic Boot Performance Record */

typedef struct acpi_fpdt_boot_pointer
{
    ACPI_FPDT_HEADER        Header;
    UINT8                   Reserved[4];
    UINT64                  Address;

} ACPI_FPDT_BOOT_POINTER;


/* 1: S3 Performance Table Pointer Record */

typedef struct acpi_fpdt_s3pt_pointer
{
    ACPI_FPDT_HEADER        Header;
    UINT8                   Reserved[4];
    UINT64                  Address;

} ACPI_FPDT_S3PT_POINTER;


/*
 * S3PT - S3 Performance Table. This table is pointed to by the
 * S3 Pointer Record above.
 */
typedef struct acpi_table_s3pt
{
    UINT8                   Signature[4]; /* "S3PT" */
    UINT32                  Length;

} ACPI_TABLE_S3PT;


/*
 * S3PT Subtables (Not part of the actual FPDT)
 */

/* Values for Type field in S3PT header */

enum AcpiS3ptType
{
    ACPI_S3PT_TYPE_RESUME               = 0,
    ACPI_S3PT_TYPE_SUSPEND              = 1,
    ACPI_FPDT_BOOT_PERFORMANCE          = 2
};

typedef struct acpi_s3pt_resume
{
    ACPI_FPDT_HEADER        Header;
    UINT32                  ResumeCount;
    UINT64                  FullResume;
    UINT64                  AverageResume;

} ACPI_S3PT_RESUME;

typedef struct acpi_s3pt_suspend
{
    ACPI_FPDT_HEADER        Header;
    UINT64                  SuspendStart;
    UINT64                  SuspendEnd;

} ACPI_S3PT_SUSPEND;


/*
 * FPDT Boot Performance Record (Not part of the actual FPDT)
 */
typedef struct acpi_fpdt_boot
{
    ACPI_FPDT_HEADER        Header;
    UINT8                   Reserved[4];
    UINT64                  ResetEnd;
    UINT64                  LoadStart;
    UINT64                  StartupStart;
    UINT64                  ExitServicesEntry;
    UINT64                  ExitServicesExit;

} ACPI_FPDT_BOOT;


/*******************************************************************************
 *
 * GTDT - Generic Timer Description Table (ACPI 5.1)
 *        Version 2
 *
 ******************************************************************************/

typedef struct acpi_table_gtdt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT64                  CounterBlockAddresss;
    UINT32                  Reserved;
    UINT32                  SecureEl1Interrupt;
    UINT32                  SecureEl1Flags;
    UINT32                  NonSecureEl1Interrupt;
    UINT32                  NonSecureEl1Flags;
    UINT32                  VirtualTimerInterrupt;
    UINT32                  VirtualTimerFlags;
    UINT32                  NonSecureEl2Interrupt;
    UINT32                  NonSecureEl2Flags;
    UINT64                  CounterReadBlockAddress;
    UINT32                  PlatformTimerCount;
    UINT32                  PlatformTimerOffset;

} ACPI_TABLE_GTDT;

/* Flag Definitions: Timer Block Physical Timers and Virtual timers */

#define ACPI_GTDT_INTERRUPT_MODE        (1)
#define ACPI_GTDT_INTERRUPT_POLARITY    (1<<1)
#define ACPI_GTDT_ALWAYS_ON             (1<<2)


/* Common GTDT subtable header */

typedef struct acpi_gtdt_header
{
    UINT8                   Type;
    UINT16                  Length;

} ACPI_GTDT_HEADER;

/* Values for GTDT subtable type above */

enum AcpiGtdtType
{
    ACPI_GTDT_TYPE_TIMER_BLOCK      = 0,
    ACPI_GTDT_TYPE_WATCHDOG         = 1,
    ACPI_GTDT_TYPE_RESERVED         = 2    /* 2 and greater are reserved */
};


/* GTDT Subtables, correspond to Type in acpi_gtdt_header */

/* 0: Generic Timer Block */

typedef struct acpi_gtdt_timer_block
{
    ACPI_GTDT_HEADER        Header;
    UINT8                   Reserved;
    UINT64                  BlockAddress;
    UINT32                  TimerCount;
    UINT32                  TimerOffset;

} ACPI_GTDT_TIMER_BLOCK;

/* Timer Sub-Structure, one per timer */

typedef struct acpi_gtdt_timer_entry
{
    UINT8                   FrameNumber;
    UINT8                   Reserved[3];
    UINT64                  BaseAddress;
    UINT64                  El0BaseAddress;
    UINT32                  TimerInterrupt;
    UINT32                  TimerFlags;
    UINT32                  VirtualTimerInterrupt;
    UINT32                  VirtualTimerFlags;
    UINT32                  CommonFlags;

} ACPI_GTDT_TIMER_ENTRY;

/* Flag Definitions: TimerFlags and VirtualTimerFlags above */

#define ACPI_GTDT_GT_IRQ_MODE               (1)
#define ACPI_GTDT_GT_IRQ_POLARITY           (1<<1)

/* Flag Definitions: CommonFlags above */

#define ACPI_GTDT_GT_IS_SECURE_TIMER        (1)
#define ACPI_GTDT_GT_ALWAYS_ON              (1<<1)


/* 1: SBSA Generic Watchdog Structure */

typedef struct acpi_gtdt_watchdog
{
    ACPI_GTDT_HEADER        Header;
    UINT8                   Reserved;
    UINT64                  RefreshFrameAddress;
    UINT64                  ControlFrameAddress;
    UINT32                  TimerInterrupt;
    UINT32                  TimerFlags;

} ACPI_GTDT_WATCHDOG;

/* Flag Definitions: TimerFlags above */

#define ACPI_GTDT_WATCHDOG_IRQ_MODE         (1)
#define ACPI_GTDT_WATCHDOG_IRQ_POLARITY     (1<<1)
#define ACPI_GTDT_WATCHDOG_SECURE           (1<<2)


/*******************************************************************************
 *
 * MPST - Memory Power State Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

#define ACPI_MPST_CHANNEL_INFO \
    UINT8                   ChannelId; \
    UINT8                   Reserved1[3]; \
    UINT16                  PowerNodeCount; \
    UINT16                  Reserved2;

/* Main table */

typedef struct acpi_table_mpst
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    ACPI_MPST_CHANNEL_INFO                      /* Platform Communication Channel */

} ACPI_TABLE_MPST;


/* Memory Platform Communication Channel Info */

typedef struct acpi_mpst_channel
{
    ACPI_MPST_CHANNEL_INFO                      /* Platform Communication Channel */

} ACPI_MPST_CHANNEL;


/* Memory Power Node Structure */

typedef struct acpi_mpst_power_node
{
    UINT8                   Flags;
    UINT8                   Reserved1;
    UINT16                  NodeId;
    UINT32                  Length;
    UINT64                  RangeAddress;
    UINT64                  RangeLength;
    UINT32                  NumPowerStates;
    UINT32                  NumPhysicalComponents;

} ACPI_MPST_POWER_NODE;

/* Values for Flags field above */

#define ACPI_MPST_ENABLED               1
#define ACPI_MPST_POWER_MANAGED         2
#define ACPI_MPST_HOT_PLUG_CAPABLE      4


/* Memory Power State Structure (follows POWER_NODE above) */

typedef struct acpi_mpst_power_state
{
    UINT8                   PowerState;
    UINT8                   InfoIndex;

} ACPI_MPST_POWER_STATE;


/* Physical Component ID Structure (follows POWER_STATE above) */

typedef struct acpi_mpst_component
{
    UINT16                  ComponentId;

} ACPI_MPST_COMPONENT;


/* Memory Power State Characteristics Structure (follows all POWER_NODEs) */

typedef struct acpi_mpst_data_hdr
{
    UINT16                  CharacteristicsCount;
    UINT16                  Reserved;

} ACPI_MPST_DATA_HDR;

typedef struct acpi_mpst_power_data
{
    UINT8                   StructureId;
    UINT8                   Flags;
    UINT16                  Reserved1;
    UINT32                  AveragePower;
    UINT32                  PowerSaving;
    UINT64                  ExitLatency;
    UINT64                  Reserved2;

} ACPI_MPST_POWER_DATA;

/* Values for Flags field above */

#define ACPI_MPST_PRESERVE              1
#define ACPI_MPST_AUTOENTRY             2
#define ACPI_MPST_AUTOEXIT              4


/* Shared Memory Region (not part of an ACPI table) */

typedef struct acpi_mpst_shared
{
    UINT32                  Signature;
    UINT16                  PccCommand;
    UINT16                  PccStatus;
    UINT32                  CommandRegister;
    UINT32                  StatusRegister;
    UINT32                  PowerStateId;
    UINT32                  PowerNodeId;
    UINT64                  EnergyConsumed;
    UINT64                  AveragePower;

} ACPI_MPST_SHARED;


/*******************************************************************************
 *
 * PCCT - Platform Communications Channel Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_pcct
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Flags;
    UINT64                  Reserved;

} ACPI_TABLE_PCCT;

/* Values for Flags field above */

#define ACPI_PCCT_DOORBELL              1

/* Values for subtable type in ACPI_SUBTABLE_HEADER */

enum AcpiPcctType
{
    ACPI_PCCT_TYPE_GENERIC_SUBSPACE             = 0,
    ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE          = 1,
    ACPI_PCCT_TYPE_HW_REDUCED_SUBSPACE_TYPE2    = 2,    /* ACPI 6.1 */
    ACPI_PCCT_TYPE_RESERVED                     = 3     /* 3 and greater are reserved */
};

/*
 * PCCT Subtables, correspond to Type in ACPI_SUBTABLE_HEADER
 */

/* 0: Generic Communications Subspace */

typedef struct acpi_pcct_subspace
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Reserved[6];
    UINT64                  BaseAddress;
    UINT64                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  PreserveMask;
    UINT64                  WriteMask;
    UINT32                  Latency;
    UINT32                  MaxAccessRate;
    UINT16                  MinTurnaroundTime;

} ACPI_PCCT_SUBSPACE;


/* 1: HW-reduced Communications Subspace (ACPI 5.1) */

typedef struct acpi_pcct_hw_reduced
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  DoorbellInterrupt;
    UINT8                   Flags;
    UINT8                   Reserved;
    UINT64                  BaseAddress;
    UINT64                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  PreserveMask;
    UINT64                  WriteMask;
    UINT32                  Latency;
    UINT32                  MaxAccessRate;
    UINT16                  MinTurnaroundTime;

} ACPI_PCCT_HW_REDUCED;


/* 2: HW-reduced Communications Subspace Type 2 (ACPI 6.1) */

typedef struct acpi_pcct_hw_reduced_type2
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  DoorbellInterrupt;
    UINT8                   Flags;
    UINT8                   Reserved;
    UINT64                  BaseAddress;
    UINT64                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  PreserveMask;
    UINT64                  WriteMask;
    UINT32                  Latency;
    UINT32                  MaxAccessRate;
    UINT16                  MinTurnaroundTime;
    ACPI_GENERIC_ADDRESS    DoorbellAckRegister;
    UINT64                  AckPreserveMask;
    UINT64                  AckWriteMask;

} ACPI_PCCT_HW_REDUCED_TYPE2;


/* Values for doorbell flags above */

#define ACPI_PCCT_INTERRUPT_POLARITY    (1)
#define ACPI_PCCT_INTERRUPT_MODE        (1<<1)


/*
 * PCC memory structures (not part of the ACPI table)
 */

/* Shared Memory Region */

typedef struct acpi_pcct_shared_memory
{
    UINT32                  Signature;
    UINT16                  Command;
    UINT16                  Status;

} ACPI_PCCT_SHARED_MEMORY;


/*******************************************************************************
 *
 * PMTT - Platform Memory Topology Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_pmtt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Reserved;

} ACPI_TABLE_PMTT;


/* Common header for PMTT subtables that follow main table */

typedef struct acpi_pmtt_header
{
    UINT8                   Type;
    UINT8                   Reserved1;
    UINT16                  Length;
    UINT16                  Flags;
    UINT16                  Reserved2;

} ACPI_PMTT_HEADER;

/* Values for Type field above */

#define ACPI_PMTT_TYPE_SOCKET           0
#define ACPI_PMTT_TYPE_CONTROLLER       1
#define ACPI_PMTT_TYPE_DIMM             2
#define ACPI_PMTT_TYPE_RESERVED         3 /* 0x03-0xFF are reserved */

/* Values for Flags field above */

#define ACPI_PMTT_TOP_LEVEL             0x0001
#define ACPI_PMTT_PHYSICAL              0x0002
#define ACPI_PMTT_MEMORY_TYPE           0x000C


/*
 * PMTT subtables, correspond to Type in acpi_pmtt_header
 */


/* 0: Socket Structure */

typedef struct acpi_pmtt_socket
{
    ACPI_PMTT_HEADER        Header;
    UINT16                  SocketId;
    UINT16                  Reserved;

} ACPI_PMTT_SOCKET;


/* 1: Memory Controller subtable */

typedef struct acpi_pmtt_controller
{
    ACPI_PMTT_HEADER        Header;
    UINT32                  ReadLatency;
    UINT32                  WriteLatency;
    UINT32                  ReadBandwidth;
    UINT32                  WriteBandwidth;
    UINT16                  AccessWidth;
    UINT16                  Alignment;
    UINT16                  Reserved;
    UINT16                  DomainCount;

} ACPI_PMTT_CONTROLLER;

/* 1a: Proximity Domain substructure */

typedef struct acpi_pmtt_domain
{
    UINT32                  ProximityDomain;

} ACPI_PMTT_DOMAIN;


/* 2: Physical Component Identifier (DIMM) */

typedef struct acpi_pmtt_physical_component
{
    ACPI_PMTT_HEADER        Header;
    UINT16                  ComponentId;
    UINT16                  Reserved;
    UINT32                  MemorySize;
    UINT32                  BiosHandle;

} ACPI_PMTT_PHYSICAL_COMPONENT;


/*******************************************************************************
 *
 * RASF - RAS Feature Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_rasf
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   ChannelId[12];

} ACPI_TABLE_RASF;

/* RASF Platform Communication Channel Shared Memory Region */

typedef struct acpi_rasf_shared_memory
{
    UINT32                  Signature;
    UINT16                  Command;
    UINT16                  Status;
    UINT16                  Version;
    UINT8                   Capabilities[16];
    UINT8                   SetCapabilities[16];
    UINT16                  NumParameterBlocks;
    UINT32                  SetCapabilitiesStatus;

} ACPI_RASF_SHARED_MEMORY;

/* RASF Parameter Block Structure Header */

typedef struct acpi_rasf_parameter_block
{
    UINT16                  Type;
    UINT16                  Version;
    UINT16                  Length;

} ACPI_RASF_PARAMETER_BLOCK;

/* RASF Parameter Block Structure for PATROL_SCRUB */

typedef struct acpi_rasf_patrol_scrub_parameter
{
    ACPI_RASF_PARAMETER_BLOCK   Header;
    UINT16                      PatrolScrubCommand;
    UINT64                      RequestedAddressRange[2];
    UINT64                      ActualAddressRange[2];
    UINT16                      Flags;
    UINT8                       RequestedSpeed;

} ACPI_RASF_PATROL_SCRUB_PARAMETER;

/* Masks for Flags and Speed fields above */

#define ACPI_RASF_SCRUBBER_RUNNING      1
#define ACPI_RASF_SPEED                 (7<<1)
#define ACPI_RASF_SPEED_SLOW            (0<<1)
#define ACPI_RASF_SPEED_MEDIUM          (4<<1)
#define ACPI_RASF_SPEED_FAST            (7<<1)

/* Channel Commands */

enum AcpiRasfCommands
{
    ACPI_RASF_EXECUTE_RASF_COMMAND      = 1
};

/* Platform RAS Capabilities */

enum AcpiRasfCapabiliities
{
    ACPI_HW_PATROL_SCRUB_SUPPORTED      = 0,
    ACPI_SW_PATROL_SCRUB_EXPOSED        = 1
};

/* Patrol Scrub Commands */

enum AcpiRasfPatrolScrubCommands
{
    ACPI_RASF_GET_PATROL_PARAMETERS     = 1,
    ACPI_RASF_START_PATROL_SCRUBBER     = 2,
    ACPI_RASF_STOP_PATROL_SCRUBBER      = 3
};

/* Channel Command flags */

#define ACPI_RASF_GENERATE_SCI          (1<<15)

/* Status values */

enum AcpiRasfStatus
{
    ACPI_RASF_SUCCESS                   = 0,
    ACPI_RASF_NOT_VALID                 = 1,
    ACPI_RASF_NOT_SUPPORTED             = 2,
    ACPI_RASF_BUSY                      = 3,
    ACPI_RASF_FAILED                    = 4,
    ACPI_RASF_ABORTED                   = 5,
    ACPI_RASF_INVALID_DATA              = 6
};

/* Status flags */

#define ACPI_RASF_COMMAND_COMPLETE      (1)
#define ACPI_RASF_SCI_DOORBELL          (1<<1)
#define ACPI_RASF_ERROR                 (1<<2)
#define ACPI_RASF_STATUS                (0x1F<<3)


/*******************************************************************************
 *
 * STAO - Status Override Table (_STA override) - ACPI 6.0
 *        Version 1
 *
 * Conforms to "ACPI Specification for Status Override Table"
 * 6 January 2015
 *
 ******************************************************************************/

typedef struct acpi_table_stao
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   IgnoreUart;

} ACPI_TABLE_STAO;


/*******************************************************************************
 *
 * WPBT - Windows Platform Environment Table (ACPI 6.0)
 *        Version 1
 *
 * Conforms to "Windows Platform Binary Table (WPBT)" 29 November 2011
 *
 ******************************************************************************/

typedef struct acpi_table_wpbt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  HandoffSize;
    UINT64                  HandoffAddress;
    UINT8                   Layout;
    UINT8                   Type;
    UINT16                  ArgumentsLength;

} ACPI_TABLE_WPBT;


/*******************************************************************************
 *
 * XENV - Xen Environment Table (ACPI 6.0)
 *        Version 1
 *
 * Conforms to "ACPI Specification for Xen Environment Table" 4 January 2015
 *
 ******************************************************************************/

typedef struct acpi_table_xenv
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT64                  GrantTableAddress;
    UINT64                  GrantTableSize;
    UINT32                  EventInterrupt;
    UINT8                   EventFlags;

} ACPI_TABLE_XENV;


/* Reset to default packing */

#pragma pack()

#endif /* __ACTBL3_H__ */
