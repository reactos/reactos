/******************************************************************************
 *
 * Name: actbl2.h - ACPI Specification Revision 2.0 Tables
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2009, Intel Corp.
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

#ifndef __ACTBL2_H__
#define __ACTBL2_H__


/*******************************************************************************
 *
 * Additional ACPI Tables (2)
 *
 * These tables are not consumed directly by the ACPICA subsystem, but are
 * included here to support device drivers and the AML disassembler.
 *
 * The tables in this file are defined by third-party specifications, and are
 * not defined directly by the ACPI specification itself.
 *
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_ASF            "ASF!"      /* Alert Standard Format table */
#define ACPI_SIG_BOOT           "BOOT"      /* Simple Boot Flag Table */
#define ACPI_SIG_DBGP           "DBGP"      /* Debug Port table */
#define ACPI_SIG_DMAR           "DMAR"      /* DMA Remapping table */
#define ACPI_SIG_HPET           "HPET"      /* High Precision Event Timer table */
#define ACPI_SIG_IBFT           "IBFT"      /* iSCSI Boot Firmware Table */
#define ACPI_SIG_IVRS           "IVRS"      /* I/O Virtualization Reporting Structure */
#define ACPI_SIG_MCFG           "MCFG"      /* PCI Memory Mapped Configuration table */
#define ACPI_SIG_SLIC           "SLIC"      /* Software Licensing Description Table */
#define ACPI_SIG_SPCR           "SPCR"      /* Serial Port Console Redirection table */
#define ACPI_SIG_SPMI           "SPMI"      /* Server Platform Management Interface table */
#define ACPI_SIG_TCPA           "TCPA"      /* Trusted Computing Platform Alliance table */
#define ACPI_SIG_UEFI           "UEFI"      /* Uefi Boot Optimization Table */
#define ACPI_SIG_WAET           "WAET"      /* Windows ACPI Emulated devices Table */
#define ACPI_SIG_WDAT           "WDAT"      /* Watchdog Action Table */
#define ACPI_SIG_WDRT           "WDRT"      /* Watchdog Resource Table */


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
 * Version 1.2, Sept. 2008
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
    ACPI_DMAR_TYPE_ATSR                 = 2,
    ACPI_DMAR_HARDWARE_AFFINITY         = 3,
    ACPI_DMAR_TYPE_RESERVED             = 4     /* 4 and greater are reserved */
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

/* Values for EntryType in ACPI_DMAR_DEVICE_SCOPE */

enum AcpiDmarScopeType
{
    ACPI_DMAR_SCOPE_TYPE_NOT_USED       = 0,
    ACPI_DMAR_SCOPE_TYPE_ENDPOINT       = 1,
    ACPI_DMAR_SCOPE_TYPE_BRIDGE         = 2,
    ACPI_DMAR_SCOPE_TYPE_IOAPIC         = 3,
    ACPI_DMAR_SCOPE_TYPE_HPET           = 4,
    ACPI_DMAR_SCOPE_TYPE_RESERVED       = 5     /* 5 and greater are reserved */
};

typedef struct acpi_dmar_pci_path
{
    UINT8                   Device;
    UINT8                   Function;

} ACPI_DMAR_PCI_PATH;


/*
 * DMAR Sub-tables, correspond to Type in ACPI_DMAR_HEADER
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
 * MCFG - PCI Memory Mapped Configuration table and sub-table
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
 * SPCR - Serial Port Console Redirection table
 *        Version 1
 *
 * Conforms to "Serial Port Console Redirection Table",
 * Version 1.00, January 11, 2002
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
 *        Version 1
 *
 * Conforms to "TCG PC Specific Implementation Specification",
 * Version 1.1, August 18, 2003
 *
 ******************************************************************************/

typedef struct acpi_table_tcpa
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  Reserved;
    UINT32                  MaxLogLength;       /* Maximum length for the event log area */
    UINT64                  LogAddress;         /* Address of the event log area */

} ACPI_TABLE_TCPA;


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

