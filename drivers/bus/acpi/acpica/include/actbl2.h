/******************************************************************************
 *
 * Name: actbl2.h - ACPI Table Definitions (tables not in ACPI spec)
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2022, Intel Corp.
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
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
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
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_AGDI           "AGDI"      /* Arm Generic Diagnostic Dump and Reset Device Interface */
#define ACPI_SIG_APMT           "APMT"      /* Arm Performance Monitoring Unit table */
#define ACPI_SIG_BDAT           "BDAT"      /* BIOS Data ACPI Table */
#define ACPI_SIG_CCEL           "CCEL"      /* CC Event Log Table */
#define ACPI_SIG_CDAT           "CDAT"      /* Coherent Device Attribute Table */
#define ACPI_SIG_IORT           "IORT"      /* IO Remapping Table */
#define ACPI_SIG_IVRS           "IVRS"      /* I/O Virtualization Reporting Structure */
#define ACPI_SIG_LPIT           "LPIT"      /* Low Power Idle Table */
#define ACPI_SIG_MADT           "APIC"      /* Multiple APIC Description Table */
#define ACPI_SIG_MCFG           "MCFG"      /* PCI Memory Mapped Configuration table */
#define ACPI_SIG_MCHI           "MCHI"      /* Management Controller Host Interface table */
#define ACPI_SIG_MPST           "MPST"      /* Memory Power State Table */
#define ACPI_SIG_MSDM           "MSDM"      /* Microsoft Data Management Table */
#define ACPI_SIG_NFIT           "NFIT"      /* NVDIMM Firmware Interface Table */
#define ACPI_SIG_NHLT           "NHLT"      /* Non HD Audio Link Table */
#define ACPI_SIG_PCCT           "PCCT"      /* Platform Communications Channel Table */
#define ACPI_SIG_PDTT           "PDTT"      /* Platform Debug Trigger Table */
#define ACPI_SIG_PHAT           "PHAT"      /* Platform Health Assessment Table */
#define ACPI_SIG_PMTT           "PMTT"      /* Platform Memory Topology Table */
#define ACPI_SIG_PPTT           "PPTT"      /* Processor Properties Topology Table */
#define ACPI_SIG_PRMT           "PRMT"      /* Platform Runtime Mechanism Table */
#define ACPI_SIG_RASF           "RASF"      /* RAS Feature table */
#define ACPI_SIG_RGRT           "RGRT"      /* Regulatory Graphics Resource Table */
#define ACPI_SIG_SBST           "SBST"      /* Smart Battery Specification Table */
#define ACPI_SIG_SDEI           "SDEI"      /* Software Delegated Exception Interface Table */
#define ACPI_SIG_SDEV           "SDEV"      /* Secure Devices table */
#define ACPI_SIG_SVKL           "SVKL"      /* Storage Volume Key Location Table */
#define ACPI_SIG_TDEL           "TDEL"      /* TD Event Log Table */


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
 * AEST - Arm Error Source Table
 *
 * Conforms to: ACPI for the Armv8 RAS Extensions 1.1 Platform Design Document
 * September 2020.
 *
 ******************************************************************************/

typedef struct acpi_table_aest
{
    ACPI_TABLE_HEADER       Header;
    void                    *NodeArray[];

} ACPI_TABLE_AEST;

/* Common Subtable header - one per Node Structure (Subtable) */

typedef struct              acpi_aest_hdr
{
    UINT8                   Type;
    UINT16                  Length;
    UINT8                   Reserved;
    UINT32                  NodeSpecificOffset;
    UINT32                  NodeInterfaceOffset;
    UINT32                  NodeInterruptOffset;
    UINT32                  NodeInterruptCount;
    UINT64                  TimestampRate;
    UINT64                  Reserved1;
    UINT64                  ErrorInjectionRate;

} ACPI_AEST_HEADER;

/* Values for Type above */

#define ACPI_AEST_PROCESSOR_ERROR_NODE      0
#define ACPI_AEST_MEMORY_ERROR_NODE         1
#define ACPI_AEST_SMMU_ERROR_NODE           2
#define ACPI_AEST_VENDOR_ERROR_NODE         3
#define ACPI_AEST_GIC_ERROR_NODE            4
#define ACPI_AEST_NODE_TYPE_RESERVED        5 /* 5 and above are reserved */


/*
 * AEST subtables (Error nodes)
 */

/* 0: Processor Error */

typedef struct              acpi_aest_processor
{
    UINT32                  ProcessorId;
    UINT8                   ResourceType;
    UINT8                   Reserved;
    UINT8                   Flags;
    UINT8                   Revision;
    UINT64                  ProcessorAffinity;

} ACPI_AEST_PROCESSOR;

/* Values for ResourceType above, related structs below */

#define ACPI_AEST_CACHE_RESOURCE            0
#define ACPI_AEST_TLB_RESOURCE              1
#define ACPI_AEST_GENERIC_RESOURCE          2
#define ACPI_AEST_RESOURCE_RESERVED         3   /* 3 and above are reserved */

/* 0R: Processor Cache Resource Substructure */

typedef struct              acpi_aest_processor_cache
{
    UINT32                  CacheReference;
    UINT32                  Reserved;

} ACPI_AEST_PROCESSOR_CACHE;

/* Values for CacheType above */

#define ACPI_AEST_CACHE_DATA                0
#define ACPI_AEST_CACHE_INSTRUCTION         1
#define ACPI_AEST_CACHE_UNIFIED             2
#define ACPI_AEST_CACHE_RESERVED            3   /* 3 and above are reserved */

/* 1R: Processor TLB Resource Substructure */

typedef struct              acpi_aest_processor_tlb
{
    UINT32                  TlbLevel;
    UINT32                  Reserved;

} ACPI_AEST_PROCESSOR_TLB;

/* 2R: Processor Generic Resource Substructure */

typedef struct              acpi_aest_processor_generic
{
    UINT32                   Resource;

} ACPI_AEST_PROCESSOR_GENERIC;

/* 1: Memory Error */

typedef struct              acpi_aest_memory
{
    UINT32                  SratProximityDomain;

} ACPI_AEST_MEMORY;

/* 2: Smmu Error */

typedef struct              acpi_aest_smmu
{
    UINT32                  IortNodeReference;
    UINT32                  SubcomponentReference;

} ACPI_AEST_SMMU;

/* 3: Vendor Defined */

typedef struct              acpi_aest_vendor
{
    UINT32                  AcpiHid;
    UINT32                  AcpiUid;
    UINT8                   VendorSpecificData[16];

} ACPI_AEST_VENDOR;

/* 4: Gic Error */

typedef struct              acpi_aest_gic
{
    UINT32                  InterfaceType;
    UINT32                  InstanceId;

} ACPI_AEST_GIC;

/* Values for InterfaceType above */

#define ACPI_AEST_GIC_CPU                   0
#define ACPI_AEST_GIC_DISTRIBUTOR           1
#define ACPI_AEST_GIC_REDISTRIBUTOR         2
#define ACPI_AEST_GIC_ITS                   3
#define ACPI_AEST_GIC_RESERVED              4   /* 4 and above are reserved */


/* Node Interface Structure */

typedef struct              acpi_aest_node_interface
{
    UINT8                   Type;
    UINT8                   Reserved[3];
    UINT32                  Flags;
    UINT64                  Address;
    UINT32                  ErrorRecordIndex;
    UINT32                  ErrorRecordCount;
    UINT64                  ErrorRecordImplemented;
    UINT64                  ErrorStatusReporting;
    UINT64                  AddressingMode;

} ACPI_AEST_NODE_INTERFACE;

/* Values for Type field above */

#define ACPI_AEST_NODE_SYSTEM_REGISTER      0
#define ACPI_AEST_NODE_MEMORY_MAPPED        1
#define ACPI_AEST_XFACE_RESERVED            2   /* 2 and above are reserved */

/* Node Interrupt Structure */

typedef struct              acpi_aest_node_interrupt
{
    UINT8                   Type;
    UINT8                   Reserved[2];
    UINT8                   Flags;
    UINT32                  Gsiv;
    UINT8                   IortId;
    UINT8                   Reserved1[3];

} ACPI_AEST_NODE_INTERRUPT;

/* Values for Type field above */

#define ACPI_AEST_NODE_FAULT_HANDLING       0
#define ACPI_AEST_NODE_ERROR_RECOVERY       1
#define ACPI_AEST_XRUPT_RESERVED            2   /* 2 and above are reserved */


/*******************************************************************************
 * AGDI - Arm Generic Diagnostic Dump and Reset Device Interface
 *
 * Conforms to "ACPI for Arm Components 1.1, Platform Design Document"
 * ARM DEN0093 v1.1
 *
 ******************************************************************************/
typedef struct acpi_table_agdi
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   Flags;
    UINT8                   Reserved[3];
    UINT32                  SdeiEvent;
    UINT32                  Gsiv;

} ACPI_TABLE_AGDI;

/* Mask for Flags field above */

#define ACPI_AGDI_SIGNALING_MODE (1)


/*******************************************************************************
 *
 * APMT - ARM Performance Monitoring Unit Table
 *
 * Conforms to:
 * ARM Performance Monitoring Unit Architecture 1.0 Platform Design Document
 * ARM DEN0117 v1.0 November 25, 2021
 *
 ******************************************************************************/

typedef struct acpi_table_apmt {
    ACPI_TABLE_HEADER Header;    /* Common ACPI table header */
} ACPI_TABLE_APMT;

#define ACPI_APMT_NODE_ID_LENGTH                4

/*
 * APMT subtables
 */
typedef struct acpi_apmt_node {
    UINT16                       Length;
    UINT8                        Flags;
    UINT8                        Type;
    UINT32                       Id;
    UINT64                       InstPrimary;
    UINT32                       InstSecondary;
    UINT64                       BaseAddress0;
    UINT64                       BaseAddress1;
    UINT32                       OvflwIrq;
    UINT32                       Reserved;
    UINT32                       OvflwIrqFlags;
    UINT32                       ProcAffinity;
    UINT32                       ImplId;
} ACPI_APMT_NODE;

/* Masks for Flags field above */

#define ACPI_APMT_FLAGS_DUAL_PAGE               (1<<0)
#define ACPI_APMT_FLAGS_AFFINITY                (1<<1)
#define ACPI_APMT_FLAGS_ATOMIC                  (1<<2)

/* Values for Flags dual page field above */

#define ACPI_APMT_FLAGS_DUAL_PAGE_NSUPP         (0<<0)
#define ACPI_APMT_FLAGS_DUAL_PAGE_SUPP          (1<<0)

/* Values for Flags processor affinity field above */
#define ACPI_APMT_FLAGS_AFFINITY_PROC           (0<<1)
#define ACPI_APMT_FLAGS_AFFINITY_PROC_CONTAINER (1<<1)

/* Values for Flags 64-bit atomic field above */
#define ACPI_APMT_FLAGS_ATOMIC_NSUPP            (0<<2)
#define ACPI_APMT_FLAGS_ATOMIC_SUPP             (1<<2)

/* Values for Type field above */

enum acpi_apmt_node_type {
    ACPI_APMT_NODE_TYPE_MC                      = 0x00,
    ACPI_APMT_NODE_TYPE_SMMU                    = 0x01,
    ACPI_APMT_NODE_TYPE_PCIE_ROOT               = 0x02,
    ACPI_APMT_NODE_TYPE_ACPI                    = 0x03,
    ACPI_APMT_NODE_TYPE_CACHE                   = 0x04,
    ACPI_APMT_NODE_TYPE_COUNT
};

/* Masks for ovflw_irq_flags field above */

#define ACPI_APMT_OVFLW_IRQ_FLAGS_MODE          (1<<0)
#define ACPI_APMT_OVFLW_IRQ_FLAGS_TYPE          (1<<1)

/* Values for ovflw_irq_flags mode field above */

#define ACPI_APMT_OVFLW_IRQ_FLAGS_MODE_LEVEL    (0<<0)
#define ACPI_APMT_OVFLW_IRQ_FLAGS_MODE_EDGE     (1<<0)

/* Values for ovflw_irq_flags type field above */

#define ACPI_APMT_OVFLW_IRQ_FLAGS_TYPE_WIRED    (0<<1)


/*******************************************************************************
 *
 * BDAT - BIOS Data ACPI Table
 *
 * Conforms to "BIOS Data ACPI Table", Interface Specification v4.0 Draft 5
 * Nov 2020
 *
 ******************************************************************************/

typedef struct acpi_table_bdat
{
    ACPI_TABLE_HEADER       Header;
    ACPI_GENERIC_ADDRESS    Gas;

} ACPI_TABLE_BDAT;

/*******************************************************************************
 *
 * CCEL - CC-Event Log
 *        From: "Guest-Host-Communication Interface (GHCI) for Intel
 *        Trust Domain Extensions (Intel TDX)". Feb 2022
 *
 ******************************************************************************/

typedef struct acpi_table_ccel
{
    ACPI_TABLE_HEADER       Header; /* Common ACPI table header */
    UINT8                   CCType;
    UINT8                   CCSubType;
    UINT16                  Reserved;
    UINT64                  LogAreaMinimumLength;
    UINT64                  LogAreaStartAddress;

} ACPI_TABLE_CCEL;

/*******************************************************************************
 *
 * IORT - IO Remapping Table
 *
 * Conforms to "IO Remapping Table System Software on ARM Platforms",
 * Document number: ARM DEN 0049E.e, Sep 2022
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
    UINT32                  Identifier;
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
    ACPI_IORT_NODE_SMMU_V3              = 0x04,
    ACPI_IORT_NODE_PMCG                 = 0x05,
    ACPI_IORT_NODE_RMR                  = 0x06,
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
    UINT32                  Identifiers[1];         /* GIC ITS identifier array */

} ACPI_IORT_ITS_GROUP;


typedef struct acpi_iort_named_component
{
    UINT32                  NodeFlags;
    UINT64                  MemoryProperties;       /* Memory access properties */
    UINT8                   MemoryAddressLimit;     /* Memory address size limit */
    char                    DeviceName[1];          /* Path of namespace object */

} ACPI_IORT_NAMED_COMPONENT;

/* Masks for Flags field above */

#define ACPI_IORT_NC_STALL_SUPPORTED    (1)
#define ACPI_IORT_NC_PASID_BITS         (31<<1)

typedef struct acpi_iort_root_complex
{
    UINT64                  MemoryProperties;       /* Memory access properties */
    UINT32                  AtsAttribute;
    UINT32                  PciSegmentNumber;
    UINT8                   MemoryAddressLimit;     /* Memory address size limit */
    UINT16                  PasidCapabilities;      /* PASID Capabilities */
    UINT8                   Reserved[1];            /* Reserved, must be zero */

} ACPI_IORT_ROOT_COMPLEX;

/* Masks for AtsAttribute field above */

#define ACPI_IORT_ATS_SUPPORTED         (1)     /* The root complex ATS support */
#define ACPI_IORT_PRI_SUPPORTED         (1<<1)  /* The root complex PRI support */
#define ACPI_IORT_PASID_FWD_SUPPORTED   (1<<2)  /* The root complex PASID forward support */

/* Masks for PasidCapabilities field above */
#define ACPI_IORT_PASID_MAX_WIDTH       (0x1F)  /* Bits 0-4 */

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
#define ACPI_IORT_SMMU_CORELINK_MMU401  0x00000004  /* ARM Corelink MMU-401 */
#define ACPI_IORT_SMMU_CAVIUM_THUNDERX  0x00000005  /* Cavium ThunderX SMMUv2 */

/* Masks for Flags field above */

#define ACPI_IORT_SMMU_DVM_SUPPORTED    (1)
#define ACPI_IORT_SMMU_COHERENT_WALK    (1<<1)

/* Global interrupt format */

typedef struct acpi_iort_smmu_gsi
{
    UINT32                  NSgIrpt;
    UINT32                  NSgIrptFlags;
    UINT32                  NSgCfgIrpt;
    UINT32                  NSgCfgIrptFlags;

} ACPI_IORT_SMMU_GSI;


typedef struct acpi_iort_smmu_v3
{
    UINT64                  BaseAddress;            /* SMMUv3 base address */
    UINT32                  Flags;
    UINT32                  Reserved;
    UINT64                  VatosAddress;
    UINT32                  Model;
    UINT32                  EventGsiv;
    UINT32                  PriGsiv;
    UINT32                  GerrGsiv;
    UINT32                  SyncGsiv;
    UINT32                  Pxm;
    UINT32                  IdMappingIndex;

} ACPI_IORT_SMMU_V3;

/* Values for Model field above */

#define ACPI_IORT_SMMU_V3_GENERIC           0x00000000  /* Generic SMMUv3 */
#define ACPI_IORT_SMMU_V3_HISILICON_HI161X  0x00000001  /* HiSilicon Hi161x SMMUv3 */
#define ACPI_IORT_SMMU_V3_CAVIUM_CN99XX     0x00000002  /* Cavium CN99xx SMMUv3 */

/* Masks for Flags field above */

#define ACPI_IORT_SMMU_V3_COHACC_OVERRIDE   (1)
#define ACPI_IORT_SMMU_V3_HTTU_OVERRIDE     (3<<1)
#define ACPI_IORT_SMMU_V3_PXM_VALID         (1<<3)
#define ACPI_IORT_SMMU_V3_DEVICEID_VALID    (1<<4)

typedef struct acpi_iort_pmcg
{
    UINT64                  Page0BaseAddress;
    UINT32                  OverflowGsiv;
    UINT32                  NodeReference;
    UINT64                  Page1BaseAddress;

} ACPI_IORT_PMCG;

typedef struct acpi_iort_rmr {
    UINT32 Flags;
    UINT32 RmrCount;
    UINT32 RmrOffset;

} ACPI_IORT_RMR;

/* Masks for Flags field above */
#define ACPI_IORT_RMR_REMAP_PERMITTED      (1)
#define ACPI_IORT_RMR_ACCESS_PRIVILEGE     (1<<1)

/*
 * Macro to access the Access Attributes in flags field above:
 *  Access Attributes is encoded in bits 9:2
 */
#define ACPI_IORT_RMR_ACCESS_ATTRIBUTES(flags)          (((flags) >> 2) & 0xFF)

/* Values for above Access Attributes */

#define ACPI_IORT_RMR_ATTR_DEVICE_NGNRNE   0x00
#define ACPI_IORT_RMR_ATTR_DEVICE_NGNRE    0x01
#define ACPI_IORT_RMR_ATTR_DEVICE_NGRE     0x02
#define ACPI_IORT_RMR_ATTR_DEVICE_GRE      0x03
#define ACPI_IORT_RMR_ATTR_NORMAL_NC       0x04
#define ACPI_IORT_RMR_ATTR_NORMAL_IWB_OWB  0x05

typedef struct acpi_iort_rmr_desc {
    UINT64 BaseAddress;
    UINT64 Length;
    UINT32 Reserved;

} ACPI_IORT_RMR_DESC;

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
    ACPI_IVRS_TYPE_HARDWARE1        = 0x10,
    ACPI_IVRS_TYPE_HARDWARE2        = 0x11,
    ACPI_IVRS_TYPE_HARDWARE3        = 0x40,
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

typedef struct acpi_ivrs_hardware_10
{
    ACPI_IVRS_HEADER        Header;
    UINT16                  CapabilityOffset;   /* Offset for IOMMU control fields */
    UINT64                  BaseAddress;        /* IOMMU control registers */
    UINT16                  PciSegmentGroup;
    UINT16                  Info;               /* MSI number and unit ID */
    UINT32                  FeatureReporting;

} ACPI_IVRS_HARDWARE1;

/* 0x11: I/O Virtualization Hardware Definition Block (IVHD) */

typedef struct acpi_ivrs_hardware_11
{
    ACPI_IVRS_HEADER        Header;
    UINT16                  CapabilityOffset;   /* Offset for IOMMU control fields */
    UINT64                  BaseAddress;        /* IOMMU control registers */
    UINT16                  PciSegmentGroup;
    UINT16                  Info;               /* MSI number and unit ID */
    UINT32                  Attributes;
    UINT64                  EfrRegisterImage;
    UINT64                  Reserved;
} ACPI_IVRS_HARDWARE2;

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
    ACPI_IVRS_TYPE_SPECIAL          = 72, /* Uses ACPI_IVRS_DEVICE8C */

    /* Variable-length device entries */

    ACPI_IVRS_TYPE_HID              = 240 /* Uses ACPI_IVRS_DEVICE_HID */
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

/* Type 240: variable-length device entry */

typedef struct acpi_ivrs_device_hid
{
    ACPI_IVRS_DE_HEADER     Header;
    UINT64                  AcpiHid;
    UINT64                  AcpiCid;
    UINT8                   UidType;
    UINT8                   UidLength;

} ACPI_IVRS_DEVICE_HID;

/* Values for UidType above */

#define ACPI_IVRS_UID_NOT_PRESENT   0
#define ACPI_IVRS_UID_IS_INTEGER    1
#define ACPI_IVRS_UID_IS_STRING     2

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
    ACPI_MADT_TYPE_MULTIPROC_WAKEUP         = 16,
    ACPI_MADT_TYPE_CORE_PIC                 = 17,
    ACPI_MADT_TYPE_LIO_PIC                  = 18,
    ACPI_MADT_TYPE_HT_PIC                   = 19,
    ACPI_MADT_TYPE_EIO_PIC                  = 20,
    ACPI_MADT_TYPE_MSI_PIC                  = 21,
    ACPI_MADT_TYPE_BIO_PIC                  = 22,
    ACPI_MADT_TYPE_LPC_PIC                  = 23,
    ACPI_MADT_TYPE_RESERVED                 = 24,   /* 24 to 0x7F are reserved */
    ACPI_MADT_TYPE_OEM_RESERVED             = 0x80  /* 0x80 to 0xFF are reserved for OEM use */
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


/* 11: Generic Interrupt - GICC (ACPI 5.0 + ACPI 6.0 + ACPI 6.3 changes) */

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
    UINT8                   Reserved2[1];
    UINT16                  SpeInterrupt;       /* ACPI 6.3 */

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

/* 16: Multiprocessor wakeup (ACPI 6.4) */

typedef struct acpi_madt_multiproc_wakeup
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  MailboxVersion;
    UINT32                  Reserved;           /* reserved - must be zero */
    UINT64                  BaseAddress;

} ACPI_MADT_MULTIPROC_WAKEUP;

#define ACPI_MULTIPROC_WAKEUP_MB_OS_SIZE        2032
#define ACPI_MULTIPROC_WAKEUP_MB_FIRMWARE_SIZE  2048

typedef struct acpi_madt_multiproc_wakeup_mailbox
{
    UINT16                  Command;
    UINT16                  Reserved; /* reserved - must be zero */
    UINT32                  ApicId;
    UINT64                  WakeupVector;
    UINT8                   ReservedOs[ACPI_MULTIPROC_WAKEUP_MB_OS_SIZE]; /* reserved for OS use */
    UINT8                   ReservedFirmware[ACPI_MULTIPROC_WAKEUP_MB_FIRMWARE_SIZE]; /* reserved for firmware use */

} ACPI_MADT_MULTIPROC_WAKEUP_MAILBOX;

#define ACPI_MP_WAKE_COMMAND_WAKEUP    1

/* 17: CPU Core Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_core_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT32                  ProcessorId;
    UINT32                  CoreId;
    UINT32                  Flags;
} ACPI_MADT_CORE_PIC;

/* Values for Version field above */

enum AcpiMadtCorePicVersion {
    ACPI_MADT_CORE_PIC_VERSION_NONE     = 0,
    ACPI_MADT_CORE_PIC_VERSION_V1       = 1,
    ACPI_MADT_CORE_PIC_VERSION_RESERVED = 2	/* 2 and greater are reserved */
};

/* 18: Legacy I/O Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_lio_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT64                  Address;
    UINT16                  Size;
    UINT8                   Cascade[2];
    UINT32                  CascadeMap[2];
} ACPI_MADT_LIO_PIC;

/* Values for Version field above */

enum AcpiMadtLioPicVersion {
    ACPI_MADT_LIO_PIC_VERSION_NONE      = 0,
    ACPI_MADT_LIO_PIC_VERSION_V1        = 1,
    ACPI_MADT_LIO_PIC_VERSION_RESERVED  = 2	/* 2 and greater are reserved */
};

/* 19: HT Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_ht_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT64                  Address;
    UINT16                  Size;
    UINT8                   Cascade[8];
} ACPI_MADT_HT_PIC;

/* Values for Version field above */

enum AcpiMadtHtPicVersion {
    ACPI_MADT_HT_PIC_VERSION_NONE       = 0,
    ACPI_MADT_HT_PIC_VERSION_V1         = 1,
    ACPI_MADT_HT_PIC_VERSION_RESERVED   = 2	/* 2 and greater are reserved */
};

/* 20: Extend I/O Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_eio_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT8                   Cascade;
    UINT8                   Node;
    UINT64                  NodeMap;
} ACPI_MADT_EIO_PIC;

/* Values for Version field above */

enum AcpiMadtEioPicVersion {
    ACPI_MADT_EIO_PIC_VERSION_NONE      = 0,
    ACPI_MADT_EIO_PIC_VERSION_V1        = 1,
    ACPI_MADT_EIO_PIC_VERSION_RESERVED  = 2	/* 2 and greater are reserved */
};

/* 21: MSI Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_msi_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT64                  MsgAddress;
    UINT32                  Start;
    UINT32                  Count;
} ACPI_MADT_MSI_PIC;

/* Values for Version field above */

enum AcpiMadtMsiPicVersion {
    ACPI_MADT_MSI_PIC_VERSION_NONE      = 0,
    ACPI_MADT_MSI_PIC_VERSION_V1        = 1,
    ACPI_MADT_MSI_PIC_VERSION_RESERVED  = 2	/* 2 and greater are reserved */
};

/* 22: Bridge I/O Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_bio_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT64                  Address;
    UINT16                  Size;
    UINT16                  Id;
    UINT16                  GsiBase;
} ACPI_MADT_BIO_PIC;

/* Values for Version field above */

enum AcpiMadtBioPicVersion {
    ACPI_MADT_BIO_PIC_VERSION_NONE        = 0,
    ACPI_MADT_BIO_PIC_VERSION_V1          = 1,
    ACPI_MADT_BIO_PIC_VERSION_RESERVED    = 2	/* 2 and greater are reserved */
};

/* 23: LPC Interrupt Controller (ACPI 6.5) */

typedef struct acpi_madt_lpc_pic {
    ACPI_SUBTABLE_HEADER    Header;
    UINT8                   Version;
    UINT64                  Address;
    UINT16                  Size;
    UINT8                   Cascade;
} ACPI_MADT_LPC_PIC;

/* Values for Version field above */

enum AcpiMadtLpcPicVersion {
    ACPI_MADT_LPC_PIC_VERSION_NONE       = 0,
    ACPI_MADT_LPC_PIC_VERSION_V1         = 1,
    ACPI_MADT_LPC_PIC_VERSION_RESERVED   = 2	/* 2 and greater are reserved */
};

/* 80: OEM data */

typedef struct acpi_madt_oem_data
{
    UINT8                   OemData[0];
} ACPI_MADT_OEM_DATA;


/*
 * Common flags fields for MADT subtables
 */

/* MADT Local APIC flags */

#define ACPI_MADT_ENABLED           (1)         /* 00: Processor is usable if set */
#define ACPI_MADT_ONLINE_CAPABLE    (2)         /* 01: System HW supports enabling processor at runtime */

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
    UINT64                  LocationCookie;     /* ACPI 6.4 */

} ACPI_NFIT_SYSTEM_ADDRESS;

/* Flags */

#define ACPI_NFIT_ADD_ONLINE_ONLY       (1)     /* 00: Add/Online Operation Only */
#define ACPI_NFIT_PROXIMITY_VALID       (1<<1)  /* 01: Proximity Domain Valid */
#define ACPI_NFIT_LOCATION_COOKIE_VALID (1<<2)  /* 02: SPA location cookie valid (ACPI 6.4) */

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
 * NHLT - Non HD Audio Link Table
 *
 * Conforms to: Intel Smart Sound Technology NHLT Specification
 * Version 0.8.1, January 2020.
 *
 ******************************************************************************/

/* Main table */

typedef struct acpi_table_nhlt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT8                   EndpointCount;

} ACPI_TABLE_NHLT;

typedef struct acpi_table_nhlt_endpoint_count
{
    UINT8                   EndpointCount;

} ACPI_TABLE_NHLT_ENDPOINT_COUNT;

typedef struct acpi_nhlt_endpoint
{
    UINT32                  DescriptorLength;
    UINT8                   LinkType;
    UINT8                   InstanceId;
    UINT16                  VendorId;
    UINT16                  DeviceId;
    UINT16                  RevisionId;
    UINT32                  SubsystemId;
    UINT8                   DeviceType;
    UINT8                   Direction;
    UINT8                   VirtualBusId;

} ACPI_NHLT_ENDPOINT;

/* Types for LinkType field above */

#define ACPI_NHLT_RESERVED_HD_AUDIO         0
#define ACPI_NHLT_RESERVED_DSP              1
#define ACPI_NHLT_PDM                       2
#define ACPI_NHLT_SSP                       3
#define ACPI_NHLT_RESERVED_SLIMBUS          4
#define ACPI_NHLT_RESERVED_SOUNDWIRE        5
#define ACPI_NHLT_TYPE_RESERVED             6 /* 6 and above are reserved */

/* All other values above are reserved */

/* Values for DeviceId field above */

#define ACPI_NHLT_PDM_DMIC                  0xAE20
#define ACPI_NHLT_BT_SIDEBAND               0xAE30
#define ACPI_NHLT_I2S_TDM_CODECS            0xAE23

/* Values for DeviceType field above */

/* SSP Link */

#define ACPI_NHLT_LINK_BT_SIDEBAND          0
#define ACPI_NHLT_LINK_FM                   1
#define ACPI_NHLT_LINK_MODEM                2
/* 3 is reserved */
#define ACPI_NHLT_LINK_SSP_ANALOG_CODEC     4

/* PDM Link */

#define ACPI_NHLT_PDM_ON_CAVS_1P8           0
#define ACPI_NHLT_PDM_ON_CAVS_1P5           1

/* Values for Direction field above */

#define ACPI_NHLT_DIR_RENDER                0
#define ACPI_NHLT_DIR_CAPTURE               1
#define ACPI_NHLT_DIR_RENDER_LOOPBACK       2
#define ACPI_NHLT_DIR_RENDER_FEEDBACK       3
#define ACPI_NHLT_DIR_RESERVED              4   /* 4 and above are reserved */

/* Capabilities = 2 */

typedef struct acpi_nhlt_device_specific_config
{
    UINT32                  CapabilitiesSize;
    UINT8                   VirtualSlot;
    UINT8                   ConfigType;

} ACPI_NHLT_DEVICE_SPECIFIC_CONFIG;

/* Capabilities = 3 */

typedef struct acpi_nhlt_device_specific_config_a
{
    UINT32                  CapabilitiesSize;
    UINT8                   VirtualSlot;
    UINT8                   ConfigType;
    UINT8                   ArrayType;

} ACPI_NHLT_DEVICE_SPECIFIC_CONFIG_A;

/* Capabilities = 3 */

typedef struct acpi_nhlt_device_specific_config_d
{
    UINT8                   VirtualSlot;
    UINT8                   ConfigType;
    UINT8                   ArrayType;

} ACPI_NHLT_DEVICE_SPECIFIC_CONFIG_D;

/* Values for Config Type above */

#define ACPI_NHLT_CONFIG_TYPE_GENERIC              0x00
#define ACPI_NHLT_CONFIG_TYPE_MIC_ARRAY            0x01
#define ACPI_NHLT_CONFIG_TYPE_RENDER_FEEDBACK      0x03
#define ACPI_NHLT_CONFIG_TYPE_RESERVED             0x04   /* 4 and above are reserved */

/* Capabilities = 0 */

typedef struct acpi_nhlt_device_specific_config_b
{
    UINT32                  CapabilitiesSize;

} ACPI_NHLT_DEVICE_SPECIFIC_CONFIG_B;

/* Capabilities = 1 */

typedef struct acpi_nhlt_device_specific_config_c
{
    UINT32                  CapabilitiesSize;
    UINT8                   VirtualSlot;

} ACPI_NHLT_DEVICE_SPECIFIC_CONFIG_C;

typedef struct acpi_nhlt_render_device_specific_config
{
    UINT32                  CapabilitiesSize;
    UINT8                   VirtualSlot;

} ACPI_NHLT_RENDER_DEVICE_SPECIFIC_CONFIG;

typedef struct acpi_nhlt_wave_extensible
{
    UINT16                  FormatTag;
    UINT16                  ChannelCount;
    UINT32                  SamplesPerSec;
    UINT32                  AvgBytesPerSec;
    UINT16                  BlockAlign;
    UINT16                  BitsPerSample;
    UINT16                  ExtraFormatSize;
    UINT16                  ValidBitsPerSample;
    UINT32                  ChannelMask;
    UINT8                   SubFormatGuid[16];

} ACPI_NHLT_WAVE_EXTENSIBLE;

/* Values for ChannelMask above */

#define ACPI_NHLT_SPKR_FRONT_LEFT             0x1
#define ACPI_NHLT_SPKR_FRONT_RIGHT            0x2
#define ACPI_NHLT_SPKR_FRONT_CENTER           0x4
#define ACPI_NHLT_SPKR_LOW_FREQ               0x8
#define ACPI_NHLT_SPKR_BACK_LEFT              0x10
#define ACPI_NHLT_SPKR_BACK_RIGHT             0x20
#define ACPI_NHLT_SPKR_FRONT_LEFT_OF_CENTER   0x40
#define ACPI_NHLT_SPKR_FRONT_RIGHT_OF_CENTER  0x80
#define ACPI_NHLT_SPKR_BACK_CENTER            0x100
#define ACPI_NHLT_SPKR_SIDE_LEFT              0x200
#define ACPI_NHLT_SPKR_SIDE_RIGHT             0x400
#define ACPI_NHLT_SPKR_TOP_CENTER             0x800
#define ACPI_NHLT_SPKR_TOP_FRONT_LEFT         0x1000
#define ACPI_NHLT_SPKR_TOP_FRONT_CENTER       0x2000
#define ACPI_NHLT_SPKR_TOP_FRONT_RIGHT        0x4000
#define ACPI_NHLT_SPKR_TOP_BACK_LEFT          0x8000
#define ACPI_NHLT_SPKR_TOP_BACK_CENTER        0x10000
#define ACPI_NHLT_SPKR_TOP_BACK_RIGHT         0x20000

typedef struct acpi_nhlt_format_config
{
    ACPI_NHLT_WAVE_EXTENSIBLE   Format;
    UINT32                      CapabilitySize;
    UINT8                       Capabilities[];

} ACPI_NHLT_FORMAT_CONFIG;

typedef struct acpi_nhlt_formats_config
{
    UINT8                   FormatsCount;

} ACPI_NHLT_FORMATS_CONFIG;

typedef struct acpi_nhlt_device_specific_hdr
{
    UINT8                   VirtualSlot;
    UINT8                   ConfigType;

} ACPI_NHLT_DEVICE_SPECIFIC_HDR;

/* Types for ConfigType above */

#define ACPI_NHLT_GENERIC                   0
#define ACPI_NHLT_MIC                       1
#define ACPI_NHLT_RENDER                    3

typedef struct acpi_nhlt_mic_device_specific_config
{
    ACPI_NHLT_DEVICE_SPECIFIC_HDR   DeviceConfig;
    UINT8                           ArrayTypeExt;

} ACPI_NHLT_MIC_DEVICE_SPECIFIC_CONFIG;

/* Values for ArrayTypeExt above */

#define ACPI_NHLT_ARRAY_TYPE_RESERVED               0x09    /* 9 and below are reserved */
#define ACPI_NHLT_SMALL_LINEAR_2ELEMENT             0x0A
#define ACPI_NHLT_BIG_LINEAR_2ELEMENT               0x0B
#define ACPI_NHLT_FIRST_GEOMETRY_LINEAR_4ELEMENT    0x0C
#define ACPI_NHLT_PLANAR_LSHAPED_4ELEMENT           0x0D
#define ACPI_NHLT_SECOND_GEOMETRY_LINEAR_4ELEMENT   0x0E
#define ACPI_NHLT_VENDOR_DEFINED                    0x0F
#define ACPI_NHLT_ARRAY_TYPE_MASK                   0x0F
#define ACPI_NHLT_ARRAY_TYPE_EXT_MASK               0x10

#define ACPI_NHLT_NO_EXTENSION                      0x0
#define ACPI_NHLT_MIC_SNR_SENSITIVITY_EXT           (1<<4)

typedef struct acpi_nhlt_vendor_mic_count
{
    UINT8                           MicrophoneCount;

} ACPI_NHLT_VENDOR_MIC_COUNT;

typedef struct acpi_nhlt_vendor_mic_config
{
    UINT8                   Type;
    UINT8                   Panel;
    UINT16                  SpeakerPositionDistance;    /* mm */
    UINT16                  HorizontalOffset;           /* mm */
    UINT16                  VerticalOffset;             /* mm */
    UINT8                   FrequencyLowBand;           /* 5*Hz */
    UINT8                   FrequencyHighBand;          /* 500*Hz */
    UINT16                  DirectionAngle;             /* -180 - + 180 */
    UINT16                  ElevationAngle;             /* -180 - + 180 */
    UINT16                  WorkVerticalAngleBegin;     /* -180 - + 180 with 2 deg step */
    UINT16                  WorkVerticalAngleEnd;       /* -180 - + 180 with 2 deg step */
    UINT16                  WorkHorizontalAngleBegin;   /* -180 - + 180 with 2 deg step */
    UINT16                  WorkHorizontalAngleEnd;     /* -180 - + 180 with 2 deg step */

} ACPI_NHLT_VENDOR_MIC_CONFIG;

/* Values for Type field above */

#define ACPI_NHLT_MIC_OMNIDIRECTIONAL       0
#define ACPI_NHLT_MIC_SUBCARDIOID           1
#define ACPI_NHLT_MIC_CARDIOID              2
#define ACPI_NHLT_MIC_SUPER_CARDIOID        3
#define ACPI_NHLT_MIC_HYPER_CARDIOID        4
#define ACPI_NHLT_MIC_8_SHAPED              5
#define ACPI_NHLT_MIC_RESERVED6             6   /* 6 is reserved */
#define ACPI_NHLT_MIC_VENDOR_DEFINED        7
#define ACPI_NHLT_MIC_RESERVED              8   /* 8 and above are reserved */

/* Values for Panel field above */

#define ACPI_NHLT_MIC_POSITION_TOP          0
#define ACPI_NHLT_MIC_POSITION_BOTTOM       1
#define ACPI_NHLT_MIC_POSITION_LEFT         2
#define ACPI_NHLT_MIC_POSITION_RIGHT        3
#define ACPI_NHLT_MIC_POSITION_FRONT        4
#define ACPI_NHLT_MIC_POSITION_BACK         5
#define ACPI_NHLT_MIC_POSITION_RESERVED     6   /* 6 and above are reserved */

typedef struct acpi_nhlt_vendor_mic_device_specific_config
{
    ACPI_NHLT_MIC_DEVICE_SPECIFIC_CONFIG    MicArrayDeviceConfig;
    UINT8                                   NumberOfMicrophones;
    ACPI_NHLT_VENDOR_MIC_CONFIG             MicConfig[];            /* Indexed by NumberOfMicrophones */

} ACPI_NHLT_VENDOR_MIC_DEVICE_SPECIFIC_CONFIG;

/* Microphone SNR and Sensitivity extension */

typedef struct acpi_nhlt_mic_snr_sensitivity_extension
{
    UINT32                  SNR;
    UINT32                  Sensitivity;

} ACPI_NHLT_MIC_SNR_SENSITIVITY_EXTENSION;

/* Render device with feedback */

typedef struct acpi_nhlt_render_feedback_device_specific_config
{
    UINT8                               FeedbackVirtualSlot;        /* Render slot in case of capture */
    UINT16                              FeedbackChannels;           /* Informative only */
    UINT16                              FeedbackValidBitsPerSample;

} ACPI_NHLT_RENDER_FEEDBACK_DEVICE_SPECIFIC_CONFIG;

/* Non documented structures */

typedef struct acpi_nhlt_device_info_count
{
    UINT8                               StructureCount;

} ACPI_NHLT_DEVICE_INFO_COUNT;

typedef struct acpi_nhlt_device_info
{
    UINT8                               DeviceId[16];
    UINT8                               DeviceInstanceId;
    UINT8                               DevicePortId;

} ACPI_NHLT_DEVICE_INFO;


/*******************************************************************************
 *
 * PCCT - Platform Communications Channel Table (ACPI 5.0)
 *        Version 2 (ACPI 6.2)
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
    ACPI_PCCT_TYPE_EXT_PCC_MASTER_SUBSPACE      = 3,    /* ACPI 6.2 */
    ACPI_PCCT_TYPE_EXT_PCC_SLAVE_SUBSPACE       = 4,    /* ACPI 6.2 */
    ACPI_PCCT_TYPE_HW_REG_COMM_SUBSPACE         = 5,    /* ACPI 6.4 */
    ACPI_PCCT_TYPE_RESERVED                     = 6     /* 6 and greater are reserved */
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
    UINT32                  PlatformInterrupt;
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
    UINT32                  PlatformInterrupt;
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
    ACPI_GENERIC_ADDRESS    PlatformAckRegister;
    UINT64                  AckPreserveMask;
    UINT64                  AckWriteMask;

} ACPI_PCCT_HW_REDUCED_TYPE2;


/* 3: Extended PCC Master Subspace Type 3 (ACPI 6.2) */

typedef struct acpi_pcct_ext_pcc_master
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  PlatformInterrupt;
    UINT8                   Flags;
    UINT8                   Reserved1;
    UINT64                  BaseAddress;
    UINT32                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  PreserveMask;
    UINT64                  WriteMask;
    UINT32                  Latency;
    UINT32                  MaxAccessRate;
    UINT32                  MinTurnaroundTime;
    ACPI_GENERIC_ADDRESS    PlatformAckRegister;
    UINT64                  AckPreserveMask;
    UINT64                  AckSetMask;
    UINT64                  Reserved2;
    ACPI_GENERIC_ADDRESS    CmdCompleteRegister;
    UINT64                  CmdCompleteMask;
    ACPI_GENERIC_ADDRESS    CmdUpdateRegister;
    UINT64                  CmdUpdatePreserveMask;
    UINT64                  CmdUpdateSetMask;
    ACPI_GENERIC_ADDRESS    ErrorStatusRegister;
    UINT64                  ErrorStatusMask;

} ACPI_PCCT_EXT_PCC_MASTER;


/* 4: Extended PCC Slave Subspace Type 4 (ACPI 6.2) */

typedef struct acpi_pcct_ext_pcc_slave
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT32                  PlatformInterrupt;
    UINT8                   Flags;
    UINT8                   Reserved1;
    UINT64                  BaseAddress;
    UINT32                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  PreserveMask;
    UINT64                  WriteMask;
    UINT32                  Latency;
    UINT32                  MaxAccessRate;
    UINT32                  MinTurnaroundTime;
    ACPI_GENERIC_ADDRESS    PlatformAckRegister;
    UINT64                  AckPreserveMask;
    UINT64                  AckSetMask;
    UINT64                  Reserved2;
    ACPI_GENERIC_ADDRESS    CmdCompleteRegister;
    UINT64                  CmdCompleteMask;
    ACPI_GENERIC_ADDRESS    CmdUpdateRegister;
    UINT64                  CmdUpdatePreserveMask;
    UINT64                  CmdUpdateSetMask;
    ACPI_GENERIC_ADDRESS    ErrorStatusRegister;
    UINT64                  ErrorStatusMask;

} ACPI_PCCT_EXT_PCC_SLAVE;

/* 5: HW Registers based Communications Subspace */

typedef struct acpi_pcct_hw_reg
{
    ACPI_SUBTABLE_HEADER    Header;
    UINT16                  Version;
    UINT64                  BaseAddress;
    UINT64                  Length;
    ACPI_GENERIC_ADDRESS    DoorbellRegister;
    UINT64                  DoorbellPreserve;
    UINT64                  DoorbellWrite;
    ACPI_GENERIC_ADDRESS    CmdCompleteRegister;
    UINT64                  CmdCompleteMask;
    ACPI_GENERIC_ADDRESS    ErrorStatusRegister;
    UINT64                  ErrorStatusMask;
    UINT32                  NominalLatency;
    UINT32                  MinTurnaroundTime;

} ACPI_PCCT_HW_REG;


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


/* Extended PCC Subspace Shared Memory Region (ACPI 6.2) */

typedef struct acpi_pcct_ext_pcc_shared_memory
{
    UINT32                  Signature;
    UINT32                  Flags;
    UINT32                  Length;
    UINT32                  Command;

} ACPI_PCCT_EXT_PCC_SHARED_MEMORY;


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
#define ACPI_PDTT_TRIGGER_ORDER             (1<<2)


/*******************************************************************************
 *
 * PHAT - Platform Health Assessment Table (ACPI 6.4)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_phat
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_PHAT;

/* Common header for PHAT subtables that follow main table */

typedef struct acpi_phat_header
{
    UINT16                  Type;
    UINT16                  Length;
    UINT8                   Revision;

} ACPI_PHAT_HEADER;


/* Values for Type field above */

#define ACPI_PHAT_TYPE_FW_VERSION_DATA  0
#define ACPI_PHAT_TYPE_FW_HEALTH_DATA   1
#define ACPI_PHAT_TYPE_RESERVED         2 /* 0x02-0xFFFF are reserved */

/*
 * PHAT subtables, correspond to Type in ACPI_PHAT_HEADER
 */

/* 0: Firmware Version Data Record */

typedef struct acpi_phat_version_data
{
    ACPI_PHAT_HEADER        Header;
    UINT8                   Reserved[3];
    UINT32                  ElementCount;

} ACPI_PHAT_VERSION_DATA;

typedef struct acpi_phat_version_element
{
    UINT8                   Guid[16];
    UINT64                  VersionValue;
    UINT32                  ProducerId;

} ACPI_PHAT_VERSION_ELEMENT;


/* 1: Firmware Health Data Record */

typedef struct acpi_phat_health_data
{
    ACPI_PHAT_HEADER        Header;
    UINT8                   Reserved[2];
    UINT8                   Health;
    UINT8                   DeviceGuid[16];
    UINT32                  DeviceSpecificOffset; /* Zero if no Device-specific data */

} ACPI_PHAT_HEALTH_DATA;

/* Values for Health field above */

#define ACPI_PHAT_ERRORS_FOUND          0
#define ACPI_PHAT_NO_ERRORS             1
#define ACPI_PHAT_UNKNOWN_ERRORS        2
#define ACPI_PHAT_ADVISORY              3


/*******************************************************************************
 *
 * PMTT - Platform Memory Topology Table (ACPI 5.0)
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_pmtt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  MemoryDeviceCount;
    /*
     * Immediately followed by:
     * MEMORY_DEVICE MemoryDeviceStruct[MemoryDeviceCount];
     */

} ACPI_TABLE_PMTT;


/* Common header for PMTT subtables that follow main table */

typedef struct acpi_pmtt_header
{
    UINT8                   Type;
    UINT8                   Reserved1;
    UINT16                  Length;
    UINT16                  Flags;
    UINT16                  Reserved2;
    UINT32                  MemoryDeviceCount;  /* Zero means no memory device structs follow */
    /*
     * Immediately followed by:
     * UINT8 TypeSpecificData[]
     * MEMORY_DEVICE MemoryDeviceStruct[MemoryDeviceCount];
     */

} ACPI_PMTT_HEADER;

/* Values for Type field above */

#define ACPI_PMTT_TYPE_SOCKET           0
#define ACPI_PMTT_TYPE_CONTROLLER       1
#define ACPI_PMTT_TYPE_DIMM             2
#define ACPI_PMTT_TYPE_RESERVED         3 /* 0x03-0xFE are reserved */
#define ACPI_PMTT_TYPE_VENDOR           0xFF

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
    /*
     * Immediately followed by:
     * MEMORY_DEVICE MemoryDeviceStruct[MemoryDeviceCount];
     */


/* 1: Memory Controller subtable */

typedef struct acpi_pmtt_controller
{
    ACPI_PMTT_HEADER        Header;
    UINT16                  ControllerId;
    UINT16                  Reserved;

} ACPI_PMTT_CONTROLLER;
    /*
     * Immediately followed by:
     * MEMORY_DEVICE MemoryDeviceStruct[MemoryDeviceCount];
     */


/* 2: Physical Component Identifier (DIMM) */

typedef struct acpi_pmtt_physical_component
{
    ACPI_PMTT_HEADER        Header;
    UINT32                  BiosHandle;

} ACPI_PMTT_PHYSICAL_COMPONENT;


/* 0xFF: Vendor Specific Data */

typedef struct acpi_pmtt_vendor_specific
{
    ACPI_PMTT_HEADER        Header;
    UINT8                   TypeUuid[16];
    UINT8                   Specific[];
    /*
     * Immediately followed by:
     * UINT8 VendorSpecificData[];
     * MEMORY_DEVICE MemoryDeviceStruct[MemoryDeviceCount];
     */

} ACPI_PMTT_VENDOR_SPECIFIC;


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

#define ACPI_PPTT_PHYSICAL_PACKAGE          (1)
#define ACPI_PPTT_ACPI_PROCESSOR_ID_VALID   (1<<1)
#define ACPI_PPTT_ACPI_PROCESSOR_IS_THREAD  (1<<2)  /* ACPI 6.3 */
#define ACPI_PPTT_ACPI_LEAF_NODE            (1<<3)  /* ACPI 6.3 */
#define ACPI_PPTT_ACPI_IDENTICAL            (1<<4)  /* ACPI 6.3 */


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

/* 1: Cache Type Structure for PPTT version 3 */

typedef struct acpi_pptt_cache_v1
{
    UINT32                  CacheId;

} ACPI_PPTT_CACHE_V1;


/* Flags */

#define ACPI_PPTT_SIZE_PROPERTY_VALID       (1)     /* Physical property valid */
#define ACPI_PPTT_NUMBER_OF_SETS_VALID      (1<<1)  /* Number of sets valid */
#define ACPI_PPTT_ASSOCIATIVITY_VALID       (1<<2)  /* Associativity valid */
#define ACPI_PPTT_ALLOCATION_TYPE_VALID     (1<<3)  /* Allocation type valid */
#define ACPI_PPTT_CACHE_TYPE_VALID          (1<<4)  /* Cache type valid */
#define ACPI_PPTT_WRITE_POLICY_VALID        (1<<5)  /* Write policy valid */
#define ACPI_PPTT_LINE_SIZE_VALID           (1<<6)  /* Line size valid */
#define ACPI_PPTT_CACHE_ID_VALID            (1<<7)  /* Cache ID valid */

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
 * PRMT - Platform Runtime Mechanism Table
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_prmt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_PRMT;

typedef struct acpi_table_prmt_header
{
    UINT8                   PlatformGuid[16];
    UINT32                  ModuleInfoOffset;
    UINT32                  ModuleInfoCount;

} ACPI_TABLE_PRMT_HEADER;

typedef struct acpi_prmt_module_header
{
    UINT16                  Revision;
    UINT16                  Length;

} ACPI_PRMT_MODULE_HEADER;

typedef struct acpi_prmt_module_info
{
    UINT16                  Revision;
    UINT16                  Length;
    UINT8                   ModuleGuid[16];
    UINT16                  MajorRev;
    UINT16                  MinorRev;
    UINT16                  HandlerInfoCount;
    UINT32                  HandlerInfoOffset;
    UINT64                  MmioListPointer;

} ACPI_PRMT_MODULE_INFO;

typedef struct acpi_prmt_handler_info
{
    UINT16                  Revision;
    UINT16                  Length;
    UINT8                   HandlerGuid[16];
    UINT64                  HandlerAddress;
    UINT64                  StaticDataBufferAddress;
    UINT64                  AcpiParamBufferAddress;

} ACPI_PRMT_HANDLER_INFO;


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
 * RGRT - Regulatory Graphics Resource Table
 *        Version 1
 *
 * Conforms to "ACPI RGRT" available at:
 * https://microsoft.github.io/mu/dyn/mu_plus/MsCorePkg/AcpiRGRT/feature_acpi_rgrt/
 *
 ******************************************************************************/

typedef struct acpi_table_rgrt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT16                  Version;
    UINT8                   ImageType;
    UINT8                   Reserved;
    UINT8                   Image[];

} ACPI_TABLE_RGRT;

/* ImageType values */

enum AcpiRgrtImageType
{
    ACPI_RGRT_TYPE_RESERVED0            = 0,
    ACPI_RGRT_IMAGE_TYPE_PNG            = 1,
    ACPI_RGRT_TYPE_RESERVED             = 2     /* 2 and greater are reserved */
};


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
 * SDEI - Software Delegated Exception Interface Descriptor Table
 *
 * Conforms to "Software Delegated Exception Interface (SDEI)" ARM DEN0054A,
 * May 8th, 2017. Copyright 2017 ARM Ltd.
 *
 ******************************************************************************/

typedef struct acpi_table_sdei
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */

} ACPI_TABLE_SDEI;


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
#define ACPI_SDEV_SECURE_COMPONENTS_PRESENT (1<<1)

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

typedef struct acpi_sdev_secure_component
{
    UINT16                  SecureComponentOffset;
    UINT16                  SecureComponentLength;

} ACPI_SDEV_SECURE_COMPONENT;


/*
 * SDEV sub-subtables ("Components") for above
 */
typedef struct acpi_sdev_component
{
    ACPI_SDEV_HEADER        Header;

} ACPI_SDEV_COMPONENT;


/* Values for sub-subtable type above */

enum AcpiSacType
{
    ACPI_SDEV_TYPE_ID_COMPONENT     = 0,
    ACPI_SDEV_TYPE_MEM_COMPONENT    = 1
};

typedef struct acpi_sdev_id_component
{
    ACPI_SDEV_HEADER        Header;
    UINT16                  HardwareIdOffset;
    UINT16                  HardwareIdLength;
    UINT16                  SubsystemIdOffset;
    UINT16                  SubsystemIdLength;
    UINT16                  HardwareRevision;
    UINT8                   HardwareRevPresent;
    UINT8                   ClassCodePresent;
    UINT8                   PciBaseClass;
    UINT8                   PciSubClass;
    UINT8                   PciProgrammingXface;

} ACPI_SDEV_ID_COMPONENT;

typedef struct acpi_sdev_mem_component
{
    ACPI_SDEV_HEADER        Header;
    UINT32                  Reserved;
    UINT64                  MemoryBaseAddress;
    UINT64                  MemoryLength;

} ACPI_SDEV_MEM_COMPONENT;


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
 * SVKL - Storage Volume Key Location Table (ACPI 6.4)
 *        From: "Guest-Host-Communication Interface (GHCI) for Intel
 *        Trust Domain Extensions (Intel TDX)".
 *        Version 1
 *
 ******************************************************************************/

typedef struct acpi_table_svkl
{
    ACPI_TABLE_HEADER       Header; /* Common ACPI table header */
    UINT32                  Count;

} ACPI_TABLE_SVKL;

typedef struct acpi_svkl_key
{
    UINT16                  Type;
    UINT16                  Format;
    UINT32                  Size;
    UINT64                  Address;

} ACPI_SVKL_KEY;

enum acpi_svkl_type
{
    ACPI_SVKL_TYPE_MAIN_STORAGE = 0,
    ACPI_SVKL_TYPE_RESERVED     = 1 /* 1 and greater are reserved */
};

enum acpi_svkl_format
{
    ACPI_SVKL_FORMAT_RAW_BINARY = 0,
    ACPI_SVKL_FORMAT_RESERVED   = 1 /* 1 and greater are reserved */
};


/*******************************************************************************
 *
 * TDEL - TD-Event Log
 *        From: "Guest-Host-Communication Interface (GHCI) for Intel
 *        Trust Domain Extensions (Intel TDX)".
 *        September 2020
 *
 ******************************************************************************/

typedef struct acpi_table_tdel
{
    ACPI_TABLE_HEADER       Header; /* Common ACPI table header */
    UINT32                  Reserved;
    UINT64                  LogAreaMinimumLength;
    UINT64                  LogAreaStartAddress;

} ACPI_TABLE_TDEL;

/* Reset to default packing */

#pragma pack()

#endif /* __ACTBL2_H__ */
