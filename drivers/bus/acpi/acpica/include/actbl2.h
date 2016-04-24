/******************************************************************************
 *
 * Name: actbl2.h - ACPI Table Definitions (tables not in ACPI spec)
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

#ifndef __ACTBL2_H__
#define __ACTBL2_H__


/*******************************************************************************
 *
 * Additional ACPI Tables (2)
 *
 * These tables are not consumed directly by the ACPICA subsystem, but are
 * included here to support device drivers and the AML disassembler.
 *
 * Generally, the tables in this file are defined by third-party specifications,
 * and are not defined directly by the ACPI specification itself.
 *
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_ASF            "ASF!"      /* Alert Standard Format table */
#define ACPI_SIG_BOOT           "BOOT"      /* Simple Boot Flag Table */
#define ACPI_SIG_CSRT           "CSRT"      /* Core System Resource Table */
#define ACPI_SIG_DBG2           "DBG2"      /* Debug Port table type 2 */
#define ACPI_SIG_DBGP           "DBGP"      /* Debug Port table */
#define ACPI_SIG_DMAR           "DMAR"      /* DMA Remapping table */
#define ACPI_SIG_HPET           "HPET"      /* High Precision Event Timer table */
#define ACPI_SIG_IBFT           "IBFT"      /* iSCSI Boot Firmware Table */
#define ACPI_SIG_IORT           "IORT"      /* IO Remapping Table */
#define ACPI_SIG_IVRS           "IVRS"      /* I/O Virtualization Reporting Structure */
#define ACPI_SIG_LPIT           "LPIT"      /* Low Power Idle Table */
#define ACPI_SIG_MCFG           "MCFG"      /* PCI Memory Mapped Configuration table */
#define ACPI_SIG_MCHI           "MCHI"      /* Management Controller Host Interface table */
#define ACPI_SIG_MSDM           "MSDM"      /* Microsoft Data Management Table */
#define ACPI_SIG_MTMR           "MTMR"      /* MID Timer table */
#define ACPI_SIG_SLIC           "SLIC"      /* Software Licensing Description Table */
#define ACPI_SIG_SPCR           "SPCR"      /* Serial Port Console Redirection table */
#define ACPI_SIG_SPMI           "SPMI"      /* Server Platform Management Interface table */
#define ACPI_SIG_TCPA           "TCPA"      /* Trusted Computing Platform Alliance table */
#define ACPI_SIG_TPM2           "TPM2"      /* Trusted Platform Module 2.0 H/W interface table */
#define ACPI_SIG_UEFI           "UEFI"      /* Uefi Boot Optimization Table */
#define ACPI_SIG_VRTC           "VRTC"      /* Virtual Real Time Clock Table */
#define ACPI_SIG_WAET           "WAET"      /* Windows ACPI Emulated devices Table */
#define ACPI_SIG_WDAT           "WDAT"      /* Watchdog Action Table */
#define ACPI_SIG_WDDT           "WDDT"      /* Watchdog Timer Description Table */
#define ACPI_SIG_WDRT           "WDRT"      /* Watchdog Resource Table */

#ifdef ACPI_UNDEFINED_TABLES
/*
 * These tables have been seen in the field, but no definition has been found
 */
#define ACPI_SIG_ATKG           "ATKG"
#define ACPI_SIG_GSCI           "GSCI"      /* GMCH SCI table */
#define ACPI_SIG_IEIT           "IEIT"
#endif

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
 * ASF - Alert Standard Format table (Signature "ASF!")
 *       Revision 0x10
 *
 * Conforms to the Alert Standard Format Specification V2.0, 23 April 2003
 *
 ******************************************************************************/

typedef struct acpi_table_asf
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_ASF;


/* ASF subtable header */

typedef struct acpi_asf_header
{
    UINT8                   Type;
    UINT8                   Reserved;
    UINT16                  Length;

} ACPI_ASF_HEADER;


/* Values for Type field above */

enum AcpiAsfType
{
    ACPI_ASF_TYPE_INFO          = 0,
    ACPI_ASF_TYPE_ALERT         = 1,
    ACPI_ASF_TYPE_CONTROL       = 2,
    ACPI_ASF_TYPE_BOOT          = 3,
    ACPI_ASF_TYPE_ADDRESS       = 4,
    ACPI_ASF_TYPE_RESERVED      = 5
};

/*
 * ASF subtables
 */

/* 0: ASF Information */

typedef struct acpi_asf_info
{
    ACPI_ASF_HEADER         Header;
    UINT8                   MinResetValue;
    UINT8                   MinPollInterval;
    UINT16                  SystemId;
    UINT32                  MfgId;
    UINT8                   Flags;
    UINT8                   Reserved2[3];

} ACPI_ASF_INFO;

/* Masks for Flags field above */

#define ACPI_ASF_SMBUS_PROTOCOLS    (1)


/* 1: ASF Alerts */

typedef struct acpi_asf_alert
{
    ACPI_ASF_HEADER         Header;
    UINT8                   AssertMask;
    UINT8                   DeassertMask;
    UINT8                   Alerts;
    UINT8                   DataLength;

} ACPI_ASF_ALERT;

typedef struct acpi_asf_alert_data
{
    UINT8                   Address;
    UINT8                   Command;
    UINT8                   Mask;
    UINT8                   Value;
    UINT8                   SensorType;
    UINT8                   Type;
    UINT8                   Offset;
    UINT8                   SourceType;
    UINT8                   Severity;
    UINT8                   SensorNumber;
    UINT8                   Entity;
    UINT8                   Instance;

} ACPI_ASF_ALERT_DATA;


/* 2: ASF Remote Control */

typedef struct acpi_asf_remote
{
    ACPI_ASF_HEADER         Header;
    UINT8                   Controls;
    UINT8                   DataLength;
    UINT16                  Reserved2;

} ACPI_ASF_REMOTE;

typedef struct acpi_asf_control_data
{
    UINT8                   Function;
    UINT8                   Address;
    UINT8                   Command;
    UINT8                   Value;

} ACPI_ASF_CONTROL_DATA;


/* 3: ASF RMCP Boot Options */

typedef struct acpi_asf_rmcp
{
    ACPI_ASF_HEADER         Header;
    UINT8                   Capabilities[7];
    UINT8                   CompletionCode;
    UINT32                  EnterpriseId;
    UINT8                   Command;
    UINT16                  Parameter;
    UINT16                  BootOptions;
    UINT16                  OemParameters;

} ACPI_ASF_RMCP;


/* 4: ASF Address */

typedef struct acpi_asf_address
{
    ACPI_ASF_HEADER         Header;
    UINT8                   EpromAddress;
    UINT8                   Devices;

} ACPI_ASF_ADDRESS;


/*******************************************************************************
 *
 * BOOT - Simple Boot Flag Table
 *        Version 1
 *
 * Conforms to the "Simple Boot Flag Specification", Version 2.1
 *
 ******************************************************************************/

typedef struct acpi_table_boot
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   CmosIndex;          /* Index in CMOS RAM for the boot register */
    UINT8                   Reserved[3];

} ACPI_TABLE_BOOT;


/*******************************************************************************
 *
 * CSRT - Core System Resource Table
 *        Version 0
 *
 * Conforms to the "Core System Resource Table (CSRT)", November 14, 2011
 *
 ******************************************************************************/

typedef struct acpi_table_csrt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_CSRT;


/* Resource Group subtable */

typedef struct acpi_csrt_group
{
    UINT32                  Length;
    UINT32                  VendorId;
    UINT32                  SubvendorId;
    UINT16                  DeviceId;
    UINT16                  SubdeviceId;
    UINT16                  Revision;
    UINT16                  Reserved;
    UINT32                  SharedInfoLength;

    /* Shared data immediately follows (Length = SharedInfoLength) */

} ACPI_CSRT_GROUP;

/* Shared Info subtable */

typedef struct acpi_csrt_shared_info
{
    UINT16                  MajorVersion;
    UINT16                  MinorVersion;
    UINT32                  MmioBaseLow;
    UINT32                  MmioBaseHigh;
    UINT32                  GsiInterrupt;
    UINT8                   InterruptPolarity;
    UINT8                   InterruptMode;
    UINT8                   NumChannels;
    UINT8                   DmaAddressWidth;
    UINT16                  BaseRequestLine;
    UINT16                  NumHandshakeSignals;
    UINT32                  MaxBlockSize;

    /* Resource descriptors immediately follow (Length = Group Length - SharedInfoLength) */

} ACPI_CSRT_SHARED_INFO;

/* Resource Descriptor subtable */

typedef struct acpi_csrt_descriptor
{
    UINT32                  Length;
    UINT16                  Type;
    UINT16                  Subtype;
    UINT32                  Uid;

    /* Resource-specific information immediately follows */

} ACPI_CSRT_DESCRIPTOR;


/* Resource Types */

#define ACPI_CSRT_TYPE_INTERRUPT    0x0001
#define ACPI_CSRT_TYPE_TIMER        0x0002
#define ACPI_CSRT_TYPE_DMA          0x0003

/* Resource Subtypes */

#define ACPI_CSRT_XRUPT_LINE        0x0000
#define ACPI_CSRT_XRUPT_CONTROLLER  0x0001
#define ACPI_CSRT_TIMER             0x0000
#define ACPI_CSRT_DMA_CHANNEL       0x0000
#define ACPI_CSRT_DMA_CONTROLLER    0x0001


/*******************************************************************************
 *
 * DBG2 - Debug Port Table 2
 *        Version 0 (Both main table and subtables)
 *
 * Conforms to "Microsoft Debug Port Table 2 (DBG2)", December 10, 2015
 *
 ******************************************************************************/

typedef struct acpi_table_dbg2
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  InfoOffset;
    UINT32                  InfoCount;

} ACPI_TABLE_DBG2;


typedef struct acpi_dbg2_header
{
    UINT32                  InfoOffset;
    UINT32                  InfoCount;

} ACPI_DBG2_HEADER;


/* Debug Device Information Subtable */

typedef struct acpi_dbg2_device
{
    UINT8                   Revision;
    UINT16                  Length;
    UINT8                   RegisterCount;      /* Number of BaseAddress registers */
    UINT16                  NamepathLength;
    UINT16                  NamepathOffset;
    UINT16                  OemDataLength;
    UINT16                  OemDataOffset;
    UINT16                  PortType;
    UINT16                  PortSubtype;
    UINT16                  Reserved;
    UINT16                  BaseAddressOffset;
    UINT16                  AddressSizeOffset;
    /*
     * Data that follows:
     *    BaseAddress (required) - Each in 12-byte Generic Address Structure format.
     *    AddressSize (required) - Array of UINT32 sizes corresponding to each BaseAddress register.
     *    Namepath    (required) - Null terminated string. Single dot if not supported.
     *    OemData     (optional) - Length is OemDataLength.
     */
} ACPI_DBG2_DEVICE;

/* Types for PortType field above */

#define ACPI_DBG2_SERIAL_PORT       0x8000
#define ACPI_DBG2_1394_PORT         0x8001
#define ACPI_DBG2_USB_PORT          0x8002
#define ACPI_DBG2_NET_PORT          0x8003

/* Subtypes for PortSubtype field above */

#define ACPI_DBG2_16550_COMPATIBLE  0x0000
#define ACPI_DBG2_16550_SUBSET      0x0001
#define ACPI_DBG2_ARM_PL011         0x0003
#define ACPI_DBG2_ARM_SBSA_32BIT    0x000D
#define ACPI_DBG2_ARM_SBSA_GENERIC  0x000E
#define ACPI_DBG2_ARM_DCC           0x000F
#define ACPI_DBG2_BCM2835           0x0010

#define ACPI_DBG2_1394_STANDARD     0x0000

#define ACPI_DBG2_USB_XHCI          0x0000
#define ACPI_DBG2_USB_EHCI          0x0001


/*******************************************************************************
 *
 * DBGP - Debug Port table
 *        Version 1
 *
 * Conforms to the "Debug Port Specification", Version 1.00, 2/9/2000
 *
 ******************************************************************************/

typedef struct acpi_table_dbgp
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Type;               /* 0=full 16550, 1=subset of 16550 */
    UINT8                   Reserved[3];
    ACPI_GENERIC_ADDRESS    DebugPort;

} ACPI_TABLE_DBGP;


/*******************************************************************************
 *
 * DMAR - DMA Remapping table
 *        Version 1
 *
 * Conforms to "Intel Virtualization Technology for Directed I/O",
 * Version 2.3, October 2014
 *
 ******************************************************************************/

typedef struct acpi_table_dmar
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Width;              /* Host Address Width */
    UINT8                   Flags;
    UINT8                   Reserved[10];

} ACPI_TABLE_DMAR;

/* Masks for Flags field above */

#define ACPI_DMAR_INTR_REMAP        (1)
#define ACPI_DMAR_X2APIC_OPT_OUT    (1<<1)
#define ACPI_DMAR_X2APIC_MODE       (1<<2)


/* DMAR subtable header */

typedef struct acpi_dmar_header
{
    UINT16                  Type;
    UINT16                  Length;

} ACPI_DMAR_HEADER;

/* Values for subtable type in ACPI_DMAR_HEADER */

enum AcpiDmarType
{
    ACPI_DMAR_TYPE_HARDWARE_UNIT        = 0,
    ACPI_DMAR_TYPE_RESERVED_MEMORY      = 1,
    ACPI_DMAR_TYPE_ROOT_ATS             = 2,
    ACPI_DMAR_TYPE_HARDWARE_AFFINITY    = 3,
    ACPI_DMAR_TYPE_NAMESPACE            = 4,
    ACPI_DMAR_TYPE_RESERVED             = 5     /* 5 and greater are reserved */
};


/* DMAR Device Scope structure */

typedef struct acpi_dmar_device_scope
{
    UINT8                   EntryType;
    UINT8                   Length;
    UINT16                  Reserved;
    UINT8                   EnumerationId;
    UINT8                   Bus;

} ACPI_DMAR_DEVICE_SCOPE;

/* Values for EntryType in ACPI_DMAR_DEVICE_SCOPE - device types */

enum AcpiDmarScopeType
{
    ACPI_DMAR_SCOPE_TYPE_NOT_USED       = 0,
    ACPI_DMAR_SCOPE_TYPE_ENDPOINT       = 1,
    ACPI_DMAR_SCOPE_TYPE_BRIDGE         = 2,
    ACPI_DMAR_SCOPE_TYPE_IOAPIC         = 3,
    ACPI_DMAR_SCOPE_TYPE_HPET           = 4,
    ACPI_DMAR_SCOPE_TYPE_NAMESPACE      = 5,
    ACPI_DMAR_SCOPE_TYPE_RESERVED       = 6     /* 6 and greater are reserved */
};

typedef struct acpi_dmar_pci_path
{
    UINT8                   Device;
    UINT8                   Function;

} ACPI_DMAR_PCI_PATH;


/*
 * DMAR Subtables, correspond to Type in ACPI_DMAR_HEADER
 */

/* 0: Hardware Unit Definition */

typedef struct acpi_dmar_hardware_unit
{
    ACPI_DMAR_HEADER        Header;
    UINT8                   Flags;
    UINT8                   Reserved;
    UINT16                  Segment;
    UINT64                  Address;            /* Register Base Address */

} ACPI_DMAR_HARDWARE_UNIT;

/* Masks for Flags field above */

#define ACPI_DMAR_INCLUDE_ALL       (1)


/* 1: Reserved Memory Defininition */

typedef struct acpi_dmar_reserved_memory
{
    ACPI_DMAR_HEADER        Header;
    UINT16                  Reserved;
    UINT16                  Segment;
    UINT64                  BaseAddress;        /* 4K aligned base address */
    UINT64                  EndAddress;         /* 4K aligned limit address */

} ACPI_DMAR_RESERVED_MEMORY;

/* Masks for Flags field above */

#define ACPI_DMAR_ALLOW_ALL         (1)


/* 2: Root Port ATS Capability Reporting Structure */

typedef struct acpi_dmar_atsr
{
    ACPI_DMAR_HEADER        Header;
    UINT8                   Flags;
    UINT8                   Reserved;
    UINT16                  Segment;

} ACPI_DMAR_ATSR;

/* Masks for Flags field above */

#define ACPI_DMAR_ALL_PORTS         (1)


/* 3: Remapping Hardware Static Affinity Structure */

typedef struct acpi_dmar_rhsa
{
    ACPI_DMAR_HEADER        Header;
    UINT32                  Reserved;
    UINT64                  BaseAddress;
    UINT32                  ProximityDomain;

} ACPI_DMAR_RHSA;


/* 4: ACPI Namespace Device Declaration Structure */

typedef struct acpi_dmar_andd
{
    ACPI_DMAR_HEADER        Header;
    UINT8                   Reserved[3];
    UINT8                   DeviceNumber;
    char                    DeviceName[1];

} ACPI_DMAR_ANDD;


/*******************************************************************************
 *
 * HPET - High Precision Event Timer table
 *        Version 1
 *
 * Conforms to "IA-PC HPET (High Precision Event Timers) Specification",
 * Version 1.0a, October 2004
 *
 ******************************************************************************/

typedef struct acpi_table_hpet
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Id;                 /* Hardware ID of event timer block */
    ACPI_GENERIC_ADDRESS    Address;            /* Address of event timer block */
    UINT8                   Sequence;           /* HPET sequence number */
    UINT16                  MinimumTick;        /* Main counter min tick, periodic mode */
    UINT8                   Flags;

} ACPI_TABLE_HPET;

/* Masks for Flags field above */

#define ACPI_HPET_PAGE_PROTECT_MASK (3)

/* Values for Page Protect flags */

enum AcpiHpetPageProtect
{
    ACPI_HPET_NO_PAGE_PROTECT       = 0,
    ACPI_HPET_PAGE_PROTECT4         = 1,
    ACPI_HPET_PAGE_PROTECT64        = 2
};


/*******************************************************************************
 *
 * IBFT - Boot Firmware Table
 *        Version 1
 *
 * Conforms to "iSCSI Boot Firmware Table (iBFT) as Defined in ACPI 3.0b
 * Specification", Version 1.01, March 1, 2007
 *
 * Note: It appears that this table is not intended to appear in the RSDT/XSDT.
 * Therefore, it is not currently supported by the disassembler.
 *
 ******************************************************************************/

typedef struct acpi_table_ibft
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Reserved[12];

} ACPI_TABLE_IBFT;


/* IBFT common subtable header */

typedef struct acpi_ibft_header
{
    UINT8                   Type;
    UINT8                   Version;
    UINT16                  Length;
    UINT8                   Index;
    UINT8                   Flags;

} ACPI_IBFT_HEADER;

/* Values for Type field above */

enum AcpiIbftType
{
    ACPI_IBFT_TYPE_NOT_USED         = 0,
    ACPI_IBFT_TYPE_CONTROL          = 1,
    ACPI_IBFT_TYPE_INITIATOR        = 2,
    ACPI_IBFT_TYPE_NIC              = 3,
    ACPI_IBFT_TYPE_TARGET           = 4,
    ACPI_IBFT_TYPE_EXTENSIONS       = 5,
    ACPI_IBFT_TYPE_RESERVED         = 6     /* 6 and greater are reserved */
};


/* IBFT subtables */

typedef struct acpi_ibft_control
{
    ACPI_IBFT_HEADER        Header;
    UINT16                  Extensions;
    UINT16                  InitiatorOffset;
    UINT16                  Nic0Offset;
    UINT16                  Target0Offset;
    UINT16                  Nic1Offset;
    UINT16                  Target1Offset;

} ACPI_IBFT_CONTROL;

typedef struct acpi_ibft_initiator
{
    ACPI_IBFT_HEADER        Header;
    UINT8                   SnsServer[16];
    UINT8                   SlpServer[16];
    UINT8                   PrimaryServer[16];
    UINT8                   SecondaryServer[16];
    UINT16                  NameLength;
    UINT16                  NameOffset;

} ACPI_IBFT_INITIATOR;

typedef struct acpi_ibft_nic
{
    ACPI_IBFT_HEADER        Header;
    UINT8                   IpAddress[16];
    UINT8                   SubnetMaskPrefix;
    UINT8                   Origin;
    UINT8                   Gateway[16];
    UINT8                   PrimaryDns[16];
    UINT8                   SecondaryDns[16];
    UINT8                   Dhcp[16];
    UINT16                  Vlan;
    UINT8                   MacAddress[6];
    UINT16                  PciAddress;
    UINT16                  NameLength;
    UINT16                  NameOffset;

} ACPI_IBFT_NIC;

typedef struct acpi_ibft_target
{
    ACPI_IBFT_HEADER        Header;
    UINT8                   TargetIpAddress[16];
    UINT16                  TargetIpSocket;
    UINT8                   TargetBootLun[8];
    UINT8                   ChapType;
    UINT8                   NicAssociation;
    UINT16                  TargetNameLength;
    UINT16                  TargetNameOffset;
    UINT16                  ChapNameLength;
    UINT16                  ChapNameOffset;
    UINT16                  ChapSecretLength;
    UINT16                  ChapSecretOffset;
    UINT16                  ReverseChapNameLength;
    UINT16                  ReverseChapNameOffset;
    UINT16                  ReverseChapSecretLength;
    UINT16                  ReverseChapSecretOffset;

} ACPI_IBFT_TARGET;


/*******************************************************************************
 *
 * IORT - IO Remapping Table
 *
 * Conforms to "IO Remapping Table System Software on ARM Platforms",
 * Document number: ARM DEN 0049B, October 2015
 *
 ******************************************************************************/

typedef struct acpi_table_iort
{
    ACPI_TABLE_HEADER       Header;
    UINT32                  NodeCount;
    UINT32                  NodeOffset;
    UINT32                  Reserved;

} ACPI_TABLE_IORT;


/*
 * IORT subtables
 */
typedef struct acpi_iort_node
{
    UINT8                   Type;
    UINT16                  Length;
    UINT8                   Revision;
    UINT32                  Reserved;
    UINT32                  MappingCount;
    UINT32                  MappingOffset;
    char                    NodeData[1];

} ACPI_IORT_NODE;

/* Values for subtable Type above */

enum AcpiIortNodeType
{
    ACPI_IORT_NODE_ITS_GROUP            = 0x00,
    ACPI_IORT_NODE_NAMED_COMPONENT      = 0x01,
    ACPI_IORT_NODE_PCI_ROOT_COMPLEX     = 0x02,
    ACPI_IORT_NODE_SMMU                 = 0x03,
    ACPI_IORT_NODE_SMMU_V3              = 0x04
};


typedef struct acpi_iort_id_mapping
{
    UINT32                  InputBase;          /* Lowest value in input range */
    UINT32                  IdCount;            /* Number of IDs */
    UINT32                  OutputBase;         /* Lowest value in output range */
    UINT32                  OutputReference;    /* A reference to the output node */
    UINT32                  Flags;

} ACPI_IORT_ID_MAPPING;

/* Masks for Flags field above for IORT subtable */

#define ACPI_IORT_ID_SINGLE_MAPPING (1)


typedef struct acpi_iort_memory_access
{
    UINT32                  CacheCoherency;
    UINT8                   Hints;
    UINT16                  Reserved;
    UINT8                   MemoryFlags;

} ACPI_IORT_MEMORY_ACCESS;

/* Values for CacheCoherency field above */

#define ACPI_IORT_NODE_COHERENT         0x00000001  /* The device node is fully coherent */
#define ACPI_IORT_NODE_NOT_COHERENT     0x00000000  /* The device node is not coherent */

/* Masks for Hints field above */

#define ACPI_IORT_HT_TRANSIENT          (1)
#define ACPI_IORT_HT_WRITE              (1<<1)
#define ACPI_IORT_HT_READ               (1<<2)
#define ACPI_IORT_HT_OVERRIDE           (1<<3)

/* Masks for MemoryFlags field above */

#define ACPI_IORT_MF_COHERENCY          (1)
#define ACPI_IORT_MF_ATTRIBUTES         (1<<1)


/*
 * IORT node specific subtables
 */
typedef struct acpi_iort_its_group
{
    UINT32                  ItsCount;
    UINT32                  Identifiers[1];         /* GIC ITS identifier arrary */

} ACPI_IORT_ITS_GROUP;


typedef struct acpi_iort_named_component
{
    UINT32                  NodeFlags;
    UINT64                  MemoryProperties;       /* Memory access properties */
    UINT8                   MemoryAddressLimit;     /* Memory address size limit */
    char                    DeviceName[1];          /* Path of namespace object */

} ACPI_IORT_NAMED_COMPONENT;


typedef struct acpi_iort_root_complex
{
    UINT64                  MemoryProperties;       /* Memory access properties */
    UINT32                  AtsAttribute;
    UINT32                  PciSegmentNumber;

} ACPI_IORT_ROOT_COMPLEX;

/* Values for AtsAttribute field above */

#define ACPI_IORT_ATS_SUPPORTED         0x00000001  /* The root complex supports ATS */
#define ACPI_IORT_ATS_UNSUPPORTED       0x00000000  /* The root complex doesn't support ATS */


typedef struct acpi_iort_smmu
{
    UINT64                  BaseAddress;            /* SMMU base address */
    UINT64                  Span;                   /* Length of memory range */
    UINT32                  Model;
    UINT32                  Flags;
    UINT32                  GlobalInterruptOffset;
    UINT32                  ContextInterruptCount;
    UINT32                  ContextInterruptOffset;
    UINT32                  PmuInterruptCount;
    UINT32                  PmuInterruptOffset;
    UINT64                  Interrupts[1];          /* Interrupt array */

} ACPI_IORT_SMMU;

/* Values for Model field above */

#define ACPI_IORT_SMMU_V1               0x00000000  /* Generic SMMUv1 */
#define ACPI_IORT_SMMU_V2               0x00000001  /* Generic SMMUv2 */
#define ACPI_IORT_SMMU_CORELINK_MMU400  0x00000002  /* ARM Corelink MMU-400 */
#define ACPI_IORT_SMMU_CORELINK_MMU500  0x00000003  /* ARM Corelink MMU-500 */

/* Masks for Flags field above */

#define ACPI_IORT_SMMU_DVM_SUPPORTED    (1)
#define ACPI_IORT_SMMU_COHERENT_WALK    (1<<1)


typedef struct acpi_iort_smmu_v3
{
    UINT64                  BaseAddress;            /* SMMUv3 base address */
    UINT32                  Flags;
    UINT32                  Reserved;
    UINT64                  VatosAddress;
    UINT32                  Model;                 /* O: generic SMMUv3 */
    UINT32                  EventGsiv;
    UINT32                  PriGsiv;
    UINT32                  GerrGsiv;
    UINT32                  SyncGsiv;

} ACPI_IORT_SMMU_V3;

/* Masks for Flags field above */

#define ACPI_IORT_SMMU_V3_COHACC_OVERRIDE   (1)
#define ACPI_IORT_SMMU_V3_HTTU_OVERRIDE     (1<<1)


/*******************************************************************************
 *
 * IVRS - I/O Virtualization Reporting Structure
 *        Version 1
 *
 * Conforms to "AMD I/O Virtualization Technology (IOMMU) Specification",
 * Revision 1.26, February 2009.
 *
 ******************************************************************************/

typedef struct acpi_table_ivrs
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Info;               /* Common virtualization info */
    UINT64                  Reserved;

} ACPI_TABLE_IVRS;

/* Values for Info field above */

#define ACPI_IVRS_PHYSICAL_SIZE     0x00007F00  /* 7 bits, physical address size */
#define ACPI_IVRS_VIRTUAL_SIZE      0x003F8000  /* 7 bits, virtual address size */
#define ACPI_IVRS_ATS_RESERVED      0x00400000  /* ATS address translation range reserved */


/* IVRS subtable header */

typedef struct acpi_ivrs_header
{
    UINT8                   Type;               /* Subtable type */
    UINT8                   Flags;
    UINT16                  Length;             /* Subtable length */
    UINT16                  DeviceId;           /* ID of IOMMU */

} ACPI_IVRS_HEADER;

/* Values for subtable Type above */

enum AcpiIvrsType
{
    ACPI_IVRS_TYPE_HARDWARE         = 0x10,
    ACPI_IVRS_TYPE_MEMORY1          = 0x20,
    ACPI_IVRS_TYPE_MEMORY2          = 0x21,
    ACPI_IVRS_TYPE_MEMORY3          = 0x22
};

/* Masks for Flags field above for IVHD subtable */

#define ACPI_IVHD_TT_ENABLE         (1)
#define ACPI_IVHD_PASS_PW           (1<<1)
#define ACPI_IVHD_RES_PASS_PW       (1<<2)
#define ACPI_IVHD_ISOC              (1<<3)
#define ACPI_IVHD_IOTLB             (1<<4)

/* Masks for Flags field above for IVMD subtable */

#define ACPI_IVMD_UNITY             (1)
#define ACPI_IVMD_READ              (1<<1)
#define ACPI_IVMD_WRITE             (1<<2)
#define ACPI_IVMD_EXCLUSION_RANGE   (1<<3)


/*
 * IVRS subtables, correspond to Type in ACPI_IVRS_HEADER
 */

/* 0x10: I/O Virtualization Hardware Definition Block (IVHD) */

typedef struct acpi_ivrs_hardware
{
    ACPI_IVRS_HEADER        Header;
    UINT16                  CapabilityOffset;   /* Offset for IOMMU control fields */
    UINT64                  BaseAddress;        /* IOMMU control registers */
    UINT16                  PciSegmentGroup;
    UINT16                  Info;               /* MSI number and unit ID */
    UINT32                  Reserved;

} ACPI_IVRS_HARDWARE;

/* Masks for Info field above */

#define ACPI_IVHD_MSI_NUMBER_MASK   0x001F      /* 5 bits, MSI message number */
#define ACPI_IVHD_UNIT_ID_MASK      0x1F00      /* 5 bits, UnitID */


/*
 * Device Entries for IVHD subtable, appear after ACPI_IVRS_HARDWARE structure.
 * Upper two bits of the Type field are the (encoded) length of the structure.
 * Currently, only 4 and 8 byte entries are defined. 16 and 32 byte entries
 * are reserved for future use but not defined.
 */
typedef struct acpi_ivrs_de_header
{
    UINT8                   Type;
    UINT16                  Id;
    UINT8                   DataSetting;

} ACPI_IVRS_DE_HEADER;

/* Length of device entry is in the top two bits of Type field above */

#define ACPI_IVHD_ENTRY_LENGTH      0xC0

/* Values for device entry Type field above */

enum AcpiIvrsDeviceEntryType
{
    /* 4-byte device entries, all use ACPI_IVRS_DEVICE4 */

    ACPI_IVRS_TYPE_PAD4             = 0,
    ACPI_IVRS_TYPE_ALL              = 1,
    ACPI_IVRS_TYPE_SELECT           = 2,
    ACPI_IVRS_TYPE_START            = 3,
    ACPI_IVRS_TYPE_END              = 4,

    /* 8-byte device entries */

    ACPI_IVRS_TYPE_PAD8             = 64,
    ACPI_IVRS_TYPE_NOT_USED         = 65,
    ACPI_IVRS_TYPE_ALIAS_SELECT     = 66, /* Uses ACPI_IVRS_DEVICE8A */
    ACPI_IVRS_TYPE_ALIAS_START      = 67, /* Uses ACPI_IVRS_DEVICE8A */
    ACPI_IVRS_TYPE_EXT_SELECT       = 70, /* Uses ACPI_IVRS_DEVICE8B */
    ACPI_IVRS_TYPE_EXT_START        = 71, /* Uses ACPI_IVRS_DEVICE8B */
    ACPI_IVRS_TYPE_SPECIAL          = 72  /* Uses ACPI_IVRS_DEVICE8C */
};

/* Values for Data field above */

#define ACPI_IVHD_INIT_PASS         (1)
#define ACPI_IVHD_EINT_PASS         (1<<1)
#define ACPI_IVHD_NMI_PASS          (1<<2)
#define ACPI_IVHD_SYSTEM_MGMT       (3<<4)
#define ACPI_IVHD_LINT0_PASS        (1<<6)
#define ACPI_IVHD_LINT1_PASS        (1<<7)


/* Types 0-4: 4-byte device entry */

typedef struct acpi_ivrs_device4
{
    ACPI_IVRS_DE_HEADER     Header;

} ACPI_IVRS_DEVICE4;

/* Types 66-67: 8-byte device entry */

typedef struct acpi_ivrs_device8a
{
    ACPI_IVRS_DE_HEADER     Header;
    UINT8                   Reserved1;
    UINT16                  UsedId;
    UINT8                   Reserved2;

} ACPI_IVRS_DEVICE8A;

/* Types 70-71: 8-byte device entry */

typedef struct acpi_ivrs_device8b
{
    ACPI_IVRS_DE_HEADER     Header;
    UINT32                  ExtendedData;

} ACPI_IVRS_DEVICE8B;

/* Values for ExtendedData above */

#define ACPI_IVHD_ATS_DISABLED      (1<<31)

/* Type 72: 8-byte device entry */

typedef struct acpi_ivrs_device8c
{
    ACPI_IVRS_DE_HEADER     Header;
    UINT8                   Handle;
    UINT16                  UsedId;
    UINT8                   Variety;

} ACPI_IVRS_DEVICE8C;

/* Values for Variety field above */

#define ACPI_IVHD_IOAPIC            1
#define ACPI_IVHD_HPET              2


/* 0x20, 0x21, 0x22: I/O Virtualization Memory Definition Block (IVMD) */

typedef struct acpi_ivrs_memory
{
    ACPI_IVRS_HEADER        Header;
    UINT16                  AuxData;
    UINT64                  Reserved;
    UINT64                  StartAddress;
    UINT64                  MemoryLength;

} ACPI_IVRS_MEMORY;


/*******************************************************************************
 *
 * LPIT - Low Power Idle Table
 *
 * Conforms to "ACPI Low Power Idle Table (LPIT)" July 2014.
 *
 ******************************************************************************/

typedef struct acpi_table_lpit
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_LPIT;


/* LPIT subtable header */

typedef struct acpi_lpit_header
{
    UINT32                  Type;               /* Subtable type */
    UINT32                  Length;             /* Subtable length */
    UINT16                  UniqueId;
    UINT16                  Reserved;
    UINT32                  Flags;

} ACPI_LPIT_HEADER;

/* Values for subtable Type above */

enum AcpiLpitType
{
    ACPI_LPIT_TYPE_NATIVE_CSTATE    = 0x00,
    ACPI_LPIT_TYPE_RESERVED         = 0x01      /* 1 and above are reserved */
};

/* Masks for Flags field above  */

#define ACPI_LPIT_STATE_DISABLED    (1)
#define ACPI_LPIT_NO_COUNTER        (1<<1)

/*
 * LPIT subtables, correspond to Type in ACPI_LPIT_HEADER
 */

/* 0x00: Native C-state instruction based LPI structure */

typedef struct acpi_lpit_native
{
    ACPI_LPIT_HEADER        Header;
    ACPI_GENERIC_ADDRESS    EntryTrigger;
    UINT32                  Residency;
    UINT32                  Latency;
    ACPI_GENERIC_ADDRESS    ResidencyCounter;
    UINT64                  CounterFrequency;

} ACPI_LPIT_NATIVE;


/*******************************************************************************
 *
 * MCFG - PCI Memory Mapped Configuration table and subtable
 *        Version 1
 *
 * Conforms to "PCI Firmware Specification", Revision 3.0, June 20, 2005
 *
 ******************************************************************************/

typedef struct acpi_table_mcfg
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Reserved[8];

} ACPI_TABLE_MCFG;


/* Subtable */

typedef struct acpi_mcfg_allocation
{
    UINT64                  Address;            /* Base address, processor-relative */
    UINT16                  PciSegment;         /* PCI segment group number */
    UINT8                   StartBusNumber;     /* Starting PCI Bus number */
    UINT8                   EndBusNumber;       /* Final PCI Bus number */
    UINT32                  Reserved;

} ACPI_MCFG_ALLOCATION;


/*******************************************************************************
 *
 * MCHI - Management Controller Host Interface Table
 *        Version 1
 *
 * Conforms to "Management Component Transport Protocol (MCTP) Host
 * Interface Specification", Revision 1.0.0a, October 13, 2009
 *
 ******************************************************************************/

typedef struct acpi_table_mchi
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   InterfaceType;
    UINT8                   Protocol;
    UINT64                  ProtocolData;
    UINT8                   InterruptType;
    UINT8                   Gpe;
    UINT8                   PciDeviceFlag;
    UINT32                  GlobalInterrupt;
    ACPI_GENERIC_ADDRESS    ControlRegister;
    UINT8                   PciSegment;
    UINT8                   PciBus;
    UINT8                   PciDevice;
    UINT8                   PciFunction;

} ACPI_TABLE_MCHI;


/*******************************************************************************
 *
 * MSDM - Microsoft Data Management table
 *
 * Conforms to "Microsoft Software Licensing Tables (SLIC and MSDM)",
 * November 29, 2011. Copyright 2011 Microsoft
 *
 ******************************************************************************/

/* Basic MSDM table is only the common ACPI header */

typedef struct acpi_table_msdm
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_MSDM;


/*******************************************************************************
 *
 * MTMR - MID Timer Table
 *        Version 1
 *
 * Conforms to "Simple Firmware Interface Specification",
 * Draft 0.8.2, Oct 19, 2010
 * NOTE: The ACPI MTMR is equivalent to the SFI MTMR table.
 *
 ******************************************************************************/

typedef struct acpi_table_mtmr
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_MTMR;

/* MTMR entry */

typedef struct acpi_mtmr_entry
{
    ACPI_GENERIC_ADDRESS    PhysicalAddress;
    UINT32                  Frequency;
    UINT32                  Irq;

} ACPI_MTMR_ENTRY;


/*******************************************************************************
 *
 * SLIC - Software Licensing Description Table
 *
 * Conforms to "Microsoft Software Licensing Tables (SLIC and MSDM)",
 * November 29, 2011. Copyright 2011 Microsoft
 *
 ******************************************************************************/

/* Basic SLIC table is only the common ACPI header */

typedef struct acpi_table_slic
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_SLIC;


/*******************************************************************************
 *
 * SPCR - Serial Port Console Redirection table
 *        Version 2
 *
 * Conforms to "Serial Port Console Redirection Table",
 * Version 1.03, August 10, 2015
 *
 ******************************************************************************/

typedef struct acpi_table_spcr
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   InterfaceType;      /* 0=full 16550, 1=subset of 16550 */
    UINT8                   Reserved[3];
    ACPI_GENERIC_ADDRESS    SerialPort;
    UINT8                   InterruptType;
    UINT8                   PcInterrupt;
    UINT32                  Interrupt;
    UINT8                   BaudRate;
    UINT8                   Parity;
    UINT8                   StopBits;
    UINT8                   FlowControl;
    UINT8                   TerminalType;
    UINT8                   Reserved1;
    UINT16                  PciDeviceId;
    UINT16                  PciVendorId;
    UINT8                   PciBus;
    UINT8                   PciDevice;
    UINT8                   PciFunction;
    UINT32                  PciFlags;
    UINT8                   PciSegment;
    UINT32                  Reserved2;

} ACPI_TABLE_SPCR;

/* Masks for PciFlags field above */

#define ACPI_SPCR_DO_NOT_DISABLE    (1)

/* Values for Interface Type: See the definition of the DBG2 table */


/*******************************************************************************
 *
 * SPMI - Server Platform Management Interface table
 *        Version 5
 *
 * Conforms to "Intelligent Platform Management Interface Specification
 * Second Generation v2.0", Document Revision 1.0, February 12, 2004 with
 * June 12, 2009 markup.
 *
 ******************************************************************************/

typedef struct acpi_table_spmi
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   InterfaceType;
    UINT8                   Reserved;           /* Must be 1 */
    UINT16                  SpecRevision;       /* Version of IPMI */
    UINT8                   InterruptType;
    UINT8                   GpeNumber;          /* GPE assigned */
    UINT8                   Reserved1;
    UINT8                   PciDeviceFlag;
    UINT32                  Interrupt;
    ACPI_GENERIC_ADDRESS    IpmiRegister;
    UINT8                   PciSegment;
    UINT8                   PciBus;
    UINT8                   PciDevice;
    UINT8                   PciFunction;
    UINT8                   Reserved2;

} ACPI_TABLE_SPMI;

/* Values for InterfaceType above */

enum AcpiSpmiInterfaceTypes
{
    ACPI_SPMI_NOT_USED              = 0,
    ACPI_SPMI_KEYBOARD              = 1,
    ACPI_SPMI_SMI                   = 2,
    ACPI_SPMI_BLOCK_TRANSFER        = 3,
    ACPI_SPMI_SMBUS                 = 4,
    ACPI_SPMI_RESERVED              = 5         /* 5 and above are reserved */
};


/*******************************************************************************
 *
 * TCPA - Trusted Computing Platform Alliance table
 *        Version 2
 *
 * Conforms to "TCG ACPI Specification, Family 1.2 and 2.0",
 * December 19, 2014
 *
 * NOTE: There are two versions of the table with the same signature --
 * the client version and the server version. The common PlatformClass
 * field is used to differentiate the two types of tables.
 *
 ******************************************************************************/

typedef struct acpi_table_tcpa_hdr
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  PlatformClass;

} ACPI_TABLE_TCPA_HDR;

/*
 * Values for PlatformClass above.
 * This is how the client and server subtables are differentiated
 */
#define ACPI_TCPA_CLIENT_TABLE          0
#define ACPI_TCPA_SERVER_TABLE          1


typedef struct acpi_table_tcpa_client
{
    UINT32                  MinimumLogLength;   /* Minimum length for the event log area */
    UINT64                  LogAddress;         /* Address of the event log area */

} ACPI_TABLE_TCPA_CLIENT;

typedef struct acpi_table_tcpa_server
{
    UINT16                  Reserved;
    UINT64                  MinimumLogLength;   /* Minimum length for the event log area */
    UINT64                  LogAddress;         /* Address of the event log area */
    UINT16                  SpecRevision;
    UINT8                   DeviceFlags;
    UINT8                   InterruptFlags;
    UINT8                   GpeNumber;
    UINT8                   Reserved2[3];
    UINT32                  GlobalInterrupt;
    ACPI_GENERIC_ADDRESS    Address;
    UINT32                  Reserved3;
    ACPI_GENERIC_ADDRESS    ConfigAddress;
    UINT8                   Group;
    UINT8                   Bus;                /* PCI Bus/Segment/Function numbers */
    UINT8                   Device;
    UINT8                   Function;

} ACPI_TABLE_TCPA_SERVER;

/* Values for DeviceFlags above */

#define ACPI_TCPA_PCI_DEVICE            (1)
#define ACPI_TCPA_BUS_PNP               (1<<1)
#define ACPI_TCPA_ADDRESS_VALID         (1<<2)

/* Values for InterruptFlags above */

#define ACPI_TCPA_INTERRUPT_MODE        (1)
#define ACPI_TCPA_INTERRUPT_POLARITY    (1<<1)
#define ACPI_TCPA_SCI_VIA_GPE           (1<<2)
#define ACPI_TCPA_GLOBAL_INTERRUPT      (1<<3)


/*******************************************************************************
 *
 * TPM2 - Trusted Platform Module (TPM) 2.0 Hardware Interface Table
 *        Version 4
 *
 * Conforms to "TCG ACPI Specification, Family 1.2 and 2.0",
 * December 19, 2014
 *
 ******************************************************************************/

typedef struct acpi_table_tpm2
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  PlatformClass;
    UINT16                  Reserved;
    UINT64                  ControlAddress;
    UINT32                  StartMethod;

    /* Platform-specific data follows */

} ACPI_TABLE_TPM2;

/* Values for StartMethod above */

#define ACPI_TPM2_NOT_ALLOWED                       0
#define ACPI_TPM2_START_METHOD                      2
#define ACPI_TPM2_MEMORY_MAPPED                     6
#define ACPI_TPM2_COMMAND_BUFFER                    7
#define ACPI_TPM2_COMMAND_BUFFER_WITH_START_METHOD  8


/*******************************************************************************
 *
 * UEFI - UEFI Boot optimization Table
 *        Version 1
 *
 * Conforms to "Unified Extensible Firmware Interface Specification",
 * Version 2.3, May 8, 2009
 *
 ******************************************************************************/

typedef struct acpi_table_uefi
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Identifier[16];     /* UUID identifier */
    UINT16                  DataOffset;         /* Offset of remaining data in table */

} ACPI_TABLE_UEFI;


/*******************************************************************************
 *
 * VRTC - Virtual Real Time Clock Table
 *        Version 1
 *
 * Conforms to "Simple Firmware Interface Specification",
 * Draft 0.8.2, Oct 19, 2010
 * NOTE: The ACPI VRTC is equivalent to The SFI MRTC table.
 *
 ******************************************************************************/

typedef struct acpi_table_vrtc
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_VRTC;

/* VRTC entry */

typedef struct acpi_vrtc_entry
{
    ACPI_GENERIC_ADDRESS    PhysicalAddress;
    UINT32                  Irq;

} ACPI_VRTC_ENTRY;


/*******************************************************************************
 *
 * WAET - Windows ACPI Emulated devices Table
 *        Version 1
 *
 * Conforms to "Windows ACPI Emulated Devices Table", version 1.0, April 6, 2009
 *
 ******************************************************************************/

typedef struct acpi_table_waet
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Flags;

} ACPI_TABLE_WAET;

/* Masks for Flags field above */

#define ACPI_WAET_RTC_NO_ACK        (1)         /* RTC requires no int acknowledge */
#define ACPI_WAET_TIMER_ONE_READ    (1<<1)      /* PM timer requires only one read */


/*******************************************************************************
 *
 * WDAT - Watchdog Action Table
 *        Version 1
 *
 * Conforms to "Hardware Watchdog Timers Design Specification",
 * Copyright 2006 Microsoft Corporation.
 *
 ******************************************************************************/

typedef struct acpi_table_wdat
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  HeaderLength;       /* Watchdog Header Length */
    UINT16                  PciSegment;         /* PCI Segment number */
    UINT8                   PciBus;             /* PCI Bus number */
    UINT8                   PciDevice;          /* PCI Device number */
    UINT8                   PciFunction;        /* PCI Function number */
    UINT8                   Reserved[3];
    UINT32                  TimerPeriod;        /* Period of one timer count (msec) */
    UINT32                  MaxCount;           /* Maximum counter value supported */
    UINT32                  MinCount;           /* Minimum counter value */
    UINT8                   Flags;
    UINT8                   Reserved2[3];
    UINT32                  Entries;            /* Number of watchdog entries that follow */

} ACPI_TABLE_WDAT;

/* Masks for Flags field above */

#define ACPI_WDAT_ENABLED           (1)
#define ACPI_WDAT_STOPPED           0x80


/* WDAT Instruction Entries (actions) */

typedef struct acpi_wdat_entry
{
    UINT8                   Action;
    UINT8                   Instruction;
    UINT16                  Reserved;
    ACPI_GENERIC_ADDRESS    RegisterRegion;
    UINT32                  Value;              /* Value used with Read/Write register */
    UINT32                  Mask;               /* Bitmask required for this register instruction */

} ACPI_WDAT_ENTRY;

/* Values for Action field above */

enum AcpiWdatActions
{
    ACPI_WDAT_RESET                 = 1,
    ACPI_WDAT_GET_CURRENT_COUNTDOWN = 4,
    ACPI_WDAT_GET_COUNTDOWN         = 5,
    ACPI_WDAT_SET_COUNTDOWN         = 6,
    ACPI_WDAT_GET_RUNNING_STATE     = 8,
    ACPI_WDAT_SET_RUNNING_STATE     = 9,
    ACPI_WDAT_GET_STOPPED_STATE     = 10,
    ACPI_WDAT_SET_STOPPED_STATE     = 11,
    ACPI_WDAT_GET_REBOOT            = 16,
    ACPI_WDAT_SET_REBOOT            = 17,
    ACPI_WDAT_GET_SHUTDOWN          = 18,
    ACPI_WDAT_SET_SHUTDOWN          = 19,
    ACPI_WDAT_GET_STATUS            = 32,
    ACPI_WDAT_SET_STATUS            = 33,
    ACPI_WDAT_ACTION_RESERVED       = 34    /* 34 and greater are reserved */
};

/* Values for Instruction field above */

enum AcpiWdatInstructions
{
    ACPI_WDAT_READ_VALUE            = 0,
    ACPI_WDAT_READ_COUNTDOWN        = 1,
    ACPI_WDAT_WRITE_VALUE           = 2,
    ACPI_WDAT_WRITE_COUNTDOWN       = 3,
    ACPI_WDAT_INSTRUCTION_RESERVED  = 4,    /* 4 and greater are reserved */
    ACPI_WDAT_PRESERVE_REGISTER     = 0x80  /* Except for this value */
};


/*******************************************************************************
 *
 * WDDT - Watchdog Descriptor Table
 *        Version 1
 *
 * Conforms to "Using the Intel ICH Family Watchdog Timer (WDT)",
 * Version 001, September 2002
 *
 ******************************************************************************/

typedef struct acpi_table_wddt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  SpecVersion;
    UINT16                  TableVersion;
    UINT16                  PciVendorId;
    ACPI_GENERIC_ADDRESS    Address;
    UINT16                  MaxCount;           /* Maximum counter value supported */
    UINT16                  MinCount;           /* Minimum counter value supported */
    UINT16                  Period;
    UINT16                  Status;
    UINT16                  Capability;

} ACPI_TABLE_WDDT;

/* Flags for Status field above */

#define ACPI_WDDT_AVAILABLE     (1)
#define ACPI_WDDT_ACTIVE        (1<<1)
#define ACPI_WDDT_TCO_OS_OWNED  (1<<2)
#define ACPI_WDDT_USER_RESET    (1<<11)
#define ACPI_WDDT_WDT_RESET     (1<<12)
#define ACPI_WDDT_POWER_FAIL    (1<<13)
#define ACPI_WDDT_UNKNOWN_RESET (1<<14)

/* Flags for Capability field above */

#define ACPI_WDDT_AUTO_RESET    (1)
#define ACPI_WDDT_ALERT_SUPPORT (1<<1)


/*******************************************************************************
 *
 * WDRT - Watchdog Resource Table
 *        Version 1
 *
 * Conforms to "Watchdog Timer Hardware Requirements for Windows Server 2003",
 * Version 1.01, August 28, 2006
 *
 ******************************************************************************/

typedef struct acpi_table_wdrt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    ACPI_GENERIC_ADDRESS    ControlRegister;
    ACPI_GENERIC_ADDRESS    CountRegister;
    UINT16                  PciDeviceId;
    UINT16                  PciVendorId;
    UINT8                   PciBus;             /* PCI Bus number */
    UINT8                   PciDevice;          /* PCI Device number */
    UINT8                   PciFunction;        /* PCI Function number */
    UINT8                   PciSegment;         /* PCI Segment number */
    UINT16                  MaxCount;           /* Maximum counter value supported */
    UINT8                   Units;

} ACPI_TABLE_WDRT;


/* Reset to default packing */

#pragma pack()

#endif /* __ACTBL2_H__ */
