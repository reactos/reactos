/******************************************************************************
 *
 * Name: actbl.h - Basic ACPI Table Definitions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2018, Intel Corp.
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

#ifndef __ACTBL_H__
#define __ACTBL_H__


/*******************************************************************************
 *
 * Fundamental ACPI tables
 *
 * This file contains definitions for the ACPI tables that are directly consumed
 * by ACPICA. All other tables are consumed by the OS-dependent ACPI-related
 * device drivers and other OS support code.
 *
 * The RSDP and FACS do not use the common ACPI table header. All other ACPI
 * tables use the header.
 *
 ******************************************************************************/


/*
 * Values for description table header signatures for tables defined in this
 * file. Useful because they make it more difficult to inadvertently type in
 * the wrong signature.
 */
#define ACPI_SIG_DSDT           "DSDT"      /* Differentiated System Description Table */
#define ACPI_SIG_FADT           "FACP"      /* Fixed ACPI Description Table */
#define ACPI_SIG_FACS           "FACS"      /* Firmware ACPI Control Structure */
#define ACPI_SIG_OSDT           "OSDT"      /* Override System Description Table */
#define ACPI_SIG_PSDT           "PSDT"      /* Persistent System Description Table */
#define ACPI_SIG_RSDP           "RSD PTR "  /* Root System Description Pointer */
#define ACPI_SIG_RSDT           "RSDT"      /* Root System Description Table */
#define ACPI_SIG_XSDT           "XSDT"      /* Extended  System Description Table */
#define ACPI_SIG_SSDT           "SSDT"      /* Secondary System Description Table */
#define ACPI_RSDP_NAME          "RSDP"      /* Short name for RSDP, not signature */


/*
 * All tables and structures must be byte-packed to match the ACPI
 * specification, since the tables are provided by the system BIOS
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
 * Master ACPI Table Header. This common header is used by all ACPI tables
 * except the RSDP and FACS.
 *
 ******************************************************************************/

typedef struct acpi_table_header
{
    char                    Signature[ACPI_NAME_SIZE];          /* ASCII table signature */
    UINT32                  Length;                             /* Length of table in bytes, including this header */
    UINT8                   Revision;                           /* ACPI Specification minor version number */
    UINT8                   Checksum;                           /* To make sum of entire table == 0 */
    char                    OemId[ACPI_OEM_ID_SIZE];            /* ASCII OEM identification */
    char                    OemTableId[ACPI_OEM_TABLE_ID_SIZE]; /* ASCII OEM table identification */
    UINT32                  OemRevision;                        /* OEM revision number */
    char                    AslCompilerId[ACPI_NAME_SIZE];      /* ASCII ASL compiler vendor ID */
    UINT32                  AslCompilerRevision;                /* ASL compiler version */

} ACPI_TABLE_HEADER;


/*******************************************************************************
 *
 * GAS - Generic Address Structure (ACPI 2.0+)
 *
 * Note: Since this structure is used in the ACPI tables, it is byte aligned.
 * If misaligned access is not supported by the hardware, accesses to the
 * 64-bit Address field must be performed with care.
 *
 ******************************************************************************/

typedef struct acpi_generic_address
{
    UINT8                   SpaceId;                /* Address space where struct or register exists */
    UINT8                   BitWidth;               /* Size in bits of given register */
    UINT8                   BitOffset;              /* Bit offset within the register */
    UINT8                   AccessWidth;            /* Minimum Access size (ACPI 3.0) */
    UINT64                  Address;                /* 64-bit address of struct or register */

} ACPI_GENERIC_ADDRESS;


/*******************************************************************************
 *
 * RSDP - Root System Description Pointer (Signature is "RSD PTR ")
 *        Version 2
 *
 ******************************************************************************/

typedef struct acpi_table_rsdp
{
    char                    Signature[8];               /* ACPI signature, contains "RSD PTR " */
    UINT8                   Checksum;                   /* ACPI 1.0 checksum */
    char                    OemId[ACPI_OEM_ID_SIZE];    /* OEM identification */
    UINT8                   Revision;                   /* Must be (0) for ACPI 1.0 or (2) for ACPI 2.0+ */
    UINT32                  RsdtPhysicalAddress;        /* 32-bit physical address of the RSDT */
    UINT32                  Length;                     /* Table length in bytes, including header (ACPI 2.0+) */
    UINT64                  XsdtPhysicalAddress;        /* 64-bit physical address of the XSDT (ACPI 2.0+) */
    UINT8                   ExtendedChecksum;           /* Checksum of entire table (ACPI 2.0+) */
    UINT8                   Reserved[3];                /* Reserved, must be zero */

} ACPI_TABLE_RSDP;

/* Standalone struct for the ACPI 1.0 RSDP */

typedef struct acpi_rsdp_common
{
    char                    Signature[8];
    UINT8                   Checksum;
    char                    OemId[ACPI_OEM_ID_SIZE];
    UINT8                   Revision;
    UINT32                  RsdtPhysicalAddress;

} ACPI_RSDP_COMMON;

/* Standalone struct for the extended part of the RSDP (ACPI 2.0+) */

typedef struct acpi_rsdp_extension
{
    UINT32                  Length;
    UINT64                  XsdtPhysicalAddress;
    UINT8                   ExtendedChecksum;
    UINT8                   Reserved[3];

} ACPI_RSDP_EXTENSION;


/*******************************************************************************
 *
 * RSDT/XSDT - Root System Description Tables
 *             Version 1 (both)
 *
 ******************************************************************************/

typedef struct acpi_table_rsdt
{
    ACPI_TABLE_HEADER       Header;                 /* Common ACPI table header */
    UINT32                  TableOffsetEntry[1];    /* Array of pointers to ACPI tables */

} ACPI_TABLE_RSDT;

typedef struct acpi_table_xsdt
{
    ACPI_TABLE_HEADER       Header;                 /* Common ACPI table header */
    UINT64                  TableOffsetEntry[1];    /* Array of pointers to ACPI tables */

} ACPI_TABLE_XSDT;

#define ACPI_RSDT_ENTRY_SIZE        (sizeof (UINT32))
#define ACPI_XSDT_ENTRY_SIZE        (sizeof (UINT64))


/*******************************************************************************
 *
 * FACS - Firmware ACPI Control Structure (FACS)
 *
 ******************************************************************************/

typedef struct acpi_table_facs
{
    char                    Signature[4];           /* ASCII table signature */
    UINT32                  Length;                 /* Length of structure, in bytes */
    UINT32                  HardwareSignature;      /* Hardware configuration signature */
    UINT32                  FirmwareWakingVector;   /* 32-bit physical address of the Firmware Waking Vector */
    UINT32                  GlobalLock;             /* Global Lock for shared hardware resources */
    UINT32                  Flags;
    UINT64                  XFirmwareWakingVector;  /* 64-bit version of the Firmware Waking Vector (ACPI 2.0+) */
    UINT8                   Version;                /* Version of this table (ACPI 2.0+) */
    UINT8                   Reserved[3];            /* Reserved, must be zero */
    UINT32                  OspmFlags;              /* Flags to be set by OSPM (ACPI 4.0) */
    UINT8                   Reserved1[24];          /* Reserved, must be zero */

} ACPI_TABLE_FACS;

/* Masks for GlobalLock flag field above */

#define ACPI_GLOCK_PENDING          (1)             /* 00: Pending global lock ownership */
#define ACPI_GLOCK_OWNED            (1<<1)          /* 01: Global lock is owned */

/* Masks for Flags field above  */

#define ACPI_FACS_S4_BIOS_PRESENT   (1)             /* 00: S4BIOS support is present */
#define ACPI_FACS_64BIT_WAKE        (1<<1)          /* 01: 64-bit wake vector supported (ACPI 4.0) */

/* Masks for OspmFlags field above */

#define ACPI_FACS_64BIT_ENVIRONMENT (1)             /* 00: 64-bit wake environment is required (ACPI 4.0) */


/*******************************************************************************
 *
 * FADT - Fixed ACPI Description Table (Signature "FACP")
 *        Version 6
 *
 ******************************************************************************/

/* Fields common to all versions of the FADT */

typedef struct acpi_table_fadt
{
    ACPI_TABLE_HEADER       Header;             /* Common ACPI table header */
    UINT32                  Facs;               /* 32-bit physical address of FACS */
    UINT32                  Dsdt;               /* 32-bit physical address of DSDT */
    UINT8                   Model;              /* System Interrupt Model (ACPI 1.0) - not used in ACPI 2.0+ */
    UINT8                   PreferredProfile;   /* Conveys preferred power management profile to OSPM. */
    UINT16                  SciInterrupt;       /* System vector of SCI interrupt */
    UINT32                  SmiCommand;         /* 32-bit Port address of SMI command port */
    UINT8                   AcpiEnable;         /* Value to write to SMI_CMD to enable ACPI */
    UINT8                   AcpiDisable;        /* Value to write to SMI_CMD to disable ACPI */
    UINT8                   S4BiosRequest;      /* Value to write to SMI_CMD to enter S4BIOS state */
    UINT8                   PstateControl;      /* Processor performance state control*/
    UINT32                  Pm1aEventBlock;     /* 32-bit port address of Power Mgt 1a Event Reg Blk */
    UINT32                  Pm1bEventBlock;     /* 32-bit port address of Power Mgt 1b Event Reg Blk */
    UINT32                  Pm1aControlBlock;   /* 32-bit port address of Power Mgt 1a Control Reg Blk */
    UINT32                  Pm1bControlBlock;   /* 32-bit port address of Power Mgt 1b Control Reg Blk */
    UINT32                  Pm2ControlBlock;    /* 32-bit port address of Power Mgt 2 Control Reg Blk */
    UINT32                  PmTimerBlock;       /* 32-bit port address of Power Mgt Timer Ctrl Reg Blk */
    UINT32                  Gpe0Block;          /* 32-bit port address of General Purpose Event 0 Reg Blk */
    UINT32                  Gpe1Block;          /* 32-bit port address of General Purpose Event 1 Reg Blk */
    UINT8                   Pm1EventLength;     /* Byte Length of ports at Pm1xEventBlock */
    UINT8                   Pm1ControlLength;   /* Byte Length of ports at Pm1xControlBlock */
    UINT8                   Pm2ControlLength;   /* Byte Length of ports at Pm2ControlBlock */
    UINT8                   PmTimerLength;      /* Byte Length of ports at PmTimerBlock */
    UINT8                   Gpe0BlockLength;    /* Byte Length of ports at Gpe0Block */
    UINT8                   Gpe1BlockLength;    /* Byte Length of ports at Gpe1Block */
    UINT8                   Gpe1Base;           /* Offset in GPE number space where GPE1 events start */
    UINT8                   CstControl;         /* Support for the _CST object and C-States change notification */
    UINT16                  C2Latency;          /* Worst case HW latency to enter/exit C2 state */
    UINT16                  C3Latency;          /* Worst case HW latency to enter/exit C3 state */
    UINT16                  FlushSize;          /* Processor memory cache line width, in bytes */
    UINT16                  FlushStride;        /* Number of flush strides that need to be read */
    UINT8                   DutyOffset;         /* Processor duty cycle index in processor P_CNT reg */
    UINT8                   DutyWidth;          /* Processor duty cycle value bit width in P_CNT register */
    UINT8                   DayAlarm;           /* Index to day-of-month alarm in RTC CMOS RAM */
    UINT8                   MonthAlarm;         /* Index to month-of-year alarm in RTC CMOS RAM */
    UINT8                   Century;            /* Index to century in RTC CMOS RAM */
    UINT16                  BootFlags;          /* IA-PC Boot Architecture Flags (see below for individual flags) */
    UINT8                   Reserved;           /* Reserved, must be zero */
    UINT32                  Flags;              /* Miscellaneous flag bits (see below for individual flags) */
    ACPI_GENERIC_ADDRESS    ResetRegister;      /* 64-bit address of the Reset register */
    UINT8                   ResetValue;         /* Value to write to the ResetRegister port to reset the system */
    UINT16                  ArmBootFlags;       /* ARM-Specific Boot Flags (see below for individual flags) (ACPI 5.1) */
    UINT8                   MinorRevision;      /* FADT Minor Revision (ACPI 5.1) */
    UINT64                  XFacs;              /* 64-bit physical address of FACS */
    UINT64                  XDsdt;              /* 64-bit physical address of DSDT */
    ACPI_GENERIC_ADDRESS    XPm1aEventBlock;    /* 64-bit Extended Power Mgt 1a Event Reg Blk address */
    ACPI_GENERIC_ADDRESS    XPm1bEventBlock;    /* 64-bit Extended Power Mgt 1b Event Reg Blk address */
    ACPI_GENERIC_ADDRESS    XPm1aControlBlock;  /* 64-bit Extended Power Mgt 1a Control Reg Blk address */
    ACPI_GENERIC_ADDRESS    XPm1bControlBlock;  /* 64-bit Extended Power Mgt 1b Control Reg Blk address */
    ACPI_GENERIC_ADDRESS    XPm2ControlBlock;   /* 64-bit Extended Power Mgt 2 Control Reg Blk address */
    ACPI_GENERIC_ADDRESS    XPmTimerBlock;      /* 64-bit Extended Power Mgt Timer Ctrl Reg Blk address */
    ACPI_GENERIC_ADDRESS    XGpe0Block;         /* 64-bit Extended General Purpose Event 0 Reg Blk address */
    ACPI_GENERIC_ADDRESS    XGpe1Block;         /* 64-bit Extended General Purpose Event 1 Reg Blk address */
    ACPI_GENERIC_ADDRESS    SleepControl;       /* 64-bit Sleep Control register (ACPI 5.0) */
    ACPI_GENERIC_ADDRESS    SleepStatus;        /* 64-bit Sleep Status register (ACPI 5.0) */
    UINT64                  HypervisorId;       /* Hypervisor Vendor ID (ACPI 6.0) */

} ACPI_TABLE_FADT;


/* Masks for FADT IA-PC Boot Architecture Flags (boot_flags) [Vx]=Introduced in this FADT revision */

#define ACPI_FADT_LEGACY_DEVICES    (1)         /* 00: [V2] System has LPC or ISA bus devices */
#define ACPI_FADT_8042              (1<<1)      /* 01: [V3] System has an 8042 controller on port 60/64 */
#define ACPI_FADT_NO_VGA            (1<<2)      /* 02: [V4] It is not safe to probe for VGA hardware */
#define ACPI_FADT_NO_MSI            (1<<3)      /* 03: [V4] Message Signaled Interrupts (MSI) must not be enabled */
#define ACPI_FADT_NO_ASPM           (1<<4)      /* 04: [V4] PCIe ASPM control must not be enabled */
#define ACPI_FADT_NO_CMOS_RTC       (1<<5)      /* 05: [V5] No CMOS real-time clock present */

/* Masks for FADT ARM Boot Architecture Flags (arm_boot_flags) ACPI 5.1 */

#define ACPI_FADT_PSCI_COMPLIANT    (1)         /* 00: [V5+] PSCI 0.2+ is implemented */
#define ACPI_FADT_PSCI_USE_HVC      (1<<1)      /* 01: [V5+] HVC must be used instead of SMC as the PSCI conduit */

/* Masks for FADT flags */

#define ACPI_FADT_WBINVD            (1)         /* 00: [V1] The WBINVD instruction works properly */
#define ACPI_FADT_WBINVD_FLUSH      (1<<1)      /* 01: [V1] WBINVD flushes but does not invalidate caches */
#define ACPI_FADT_C1_SUPPORTED      (1<<2)      /* 02: [V1] All processors support C1 state */
#define ACPI_FADT_C2_MP_SUPPORTED   (1<<3)      /* 03: [V1] C2 state works on MP system */
#define ACPI_FADT_POWER_BUTTON      (1<<4)      /* 04: [V1] Power button is handled as a control method device */
#define ACPI_FADT_SLEEP_BUTTON      (1<<5)      /* 05: [V1] Sleep button is handled as a control method device */
#define ACPI_FADT_FIXED_RTC         (1<<6)      /* 06: [V1] RTC wakeup status is not in fixed register space */
#define ACPI_FADT_S4_RTC_WAKE       (1<<7)      /* 07: [V1] RTC alarm can wake system from S4 */
#define ACPI_FADT_32BIT_TIMER       (1<<8)      /* 08: [V1] ACPI timer width is 32-bit (0=24-bit) */
#define ACPI_FADT_DOCKING_SUPPORTED (1<<9)      /* 09: [V1] Docking supported */
#define ACPI_FADT_RESET_REGISTER    (1<<10)     /* 10: [V2] System reset via the FADT RESET_REG supported */
#define ACPI_FADT_SEALED_CASE       (1<<11)     /* 11: [V3] No internal expansion capabilities and case is sealed */
#define ACPI_FADT_HEADLESS          (1<<12)     /* 12: [V3] No local video capabilities or local input devices */
#define ACPI_FADT_SLEEP_TYPE        (1<<13)     /* 13: [V3] Must execute native instruction after writing  SLP_TYPx register */
#define ACPI_FADT_PCI_EXPRESS_WAKE  (1<<14)     /* 14: [V4] System supports PCIEXP_WAKE (STS/EN) bits (ACPI 3.0) */
#define ACPI_FADT_PLATFORM_CLOCK    (1<<15)     /* 15: [V4] OSPM should use platform-provided timer (ACPI 3.0) */
#define ACPI_FADT_S4_RTC_VALID      (1<<16)     /* 16: [V4] Contents of RTC_STS valid after S4 wake (ACPI 3.0) */
#define ACPI_FADT_REMOTE_POWER_ON   (1<<17)     /* 17: [V4] System is compatible with remote power on (ACPI 3.0) */
#define ACPI_FADT_APIC_CLUSTER      (1<<18)     /* 18: [V4] All local APICs must use cluster model (ACPI 3.0) */
#define ACPI_FADT_APIC_PHYSICAL     (1<<19)     /* 19: [V4] All local xAPICs must use physical dest mode (ACPI 3.0) */
#define ACPI_FADT_HW_REDUCED        (1<<20)     /* 20: [V5] ACPI hardware is not implemented (ACPI 5.0) */
#define ACPI_FADT_LOW_POWER_S0      (1<<21)     /* 21: [V5] S0 power savings are equal or better than S3 (ACPI 5.0) */


/* Values for PreferredProfile (Preferred Power Management Profiles) */

enum AcpiPreferredPmProfiles
{
    PM_UNSPECIFIED          = 0,
    PM_DESKTOP              = 1,
    PM_MOBILE               = 2,
    PM_WORKSTATION          = 3,
    PM_ENTERPRISE_SERVER    = 4,
    PM_SOHO_SERVER          = 5,
    PM_APPLIANCE_PC         = 6,
    PM_PERFORMANCE_SERVER   = 7,
    PM_TABLET               = 8
};

/* Values for SleepStatus and SleepControl registers (V5+ FADT) */

#define ACPI_X_WAKE_STATUS          0x80
#define ACPI_X_SLEEP_TYPE_MASK      0x1C
#define ACPI_X_SLEEP_TYPE_POSITION  0x02
#define ACPI_X_SLEEP_ENABLE         0x20


/* Reset to default packing */

#pragma pack()


/*
 * Internal table-related structures
 */
typedef union acpi_name_union
{
    UINT32                          Integer;
    char                            Ascii[4];

} ACPI_NAME_UNION;


/* Internal ACPI Table Descriptor. One per ACPI table. */

typedef struct acpi_table_desc
{
    ACPI_PHYSICAL_ADDRESS           Address;
    ACPI_TABLE_HEADER               *Pointer;
    UINT32                          Length;     /* Length fixed at 32 bits (fixed in table header) */
    ACPI_NAME_UNION                 Signature;
    ACPI_OWNER_ID                   OwnerId;
    UINT8                           Flags;
    UINT16                          ValidationCount;

} ACPI_TABLE_DESC;

/*
 * Maximum value of the ValidationCount field in ACPI_TABLE_DESC.
 * When reached, ValidationCount cannot be changed any more and the table will
 * be permanently regarded as validated.
 *
 * This is to prevent situations in which unbalanced table get/put operations
 * may cause premature table unmapping in the OS to happen.
 *
 * The maximum validation count can be defined to any value, but should be
 * greater than the maximum number of OS early stage mapping slots to avoid
 * leaking early stage table mappings to the late stage.
 */
#define ACPI_MAX_TABLE_VALIDATIONS          ACPI_UINT16_MAX

/* Masks for Flags field above */

#define ACPI_TABLE_ORIGIN_EXTERNAL_VIRTUAL  (0) /* Virtual address, external maintained */
#define ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL (1) /* Physical address, internally mapped */
#define ACPI_TABLE_ORIGIN_INTERNAL_VIRTUAL  (2) /* Virtual address, internallly allocated */
#define ACPI_TABLE_ORIGIN_MASK              (3)
#define ACPI_TABLE_IS_VERIFIED              (4)
#define ACPI_TABLE_IS_LOADED                (8)


/*
 * Get the remaining ACPI tables
 */
#include "actbl1.h"
#include "actbl2.h"
#include "actbl3.h"

/* Macros used to generate offsets to specific table fields */

#define ACPI_FADT_OFFSET(f)             (UINT16) ACPI_OFFSET (ACPI_TABLE_FADT, f)

/*
 * Sizes of the various flavors of FADT. We need to look closely
 * at the FADT length because the version number essentially tells
 * us nothing because of many BIOS bugs where the version does not
 * match the expected length. In other words, the length of the
 * FADT is the bottom line as to what the version really is.
 *
 * For reference, the values below are as follows:
 *     FADT V1 size: 0x074
 *     FADT V2 size: 0x084
 *     FADT V3 size: 0x0F4
 *     FADT V4 size: 0x0F4
 *     FADT V5 size: 0x10C
 *     FADT V6 size: 0x114
 */
#define ACPI_FADT_V1_SIZE       (UINT32) (ACPI_FADT_OFFSET (Flags) + 4)
#define ACPI_FADT_V2_SIZE       (UINT32) (ACPI_FADT_OFFSET (MinorRevision) + 1)
#define ACPI_FADT_V3_SIZE       (UINT32) (ACPI_FADT_OFFSET (SleepControl))
#define ACPI_FADT_V5_SIZE       (UINT32) (ACPI_FADT_OFFSET (HypervisorId))
#define ACPI_FADT_V6_SIZE       (UINT32) (sizeof (ACPI_TABLE_FADT))

#define ACPI_FADT_CONFORMANCE   "ACPI 6.1 (FADT version 6)"

#endif /* __ACTBL_H__ */
