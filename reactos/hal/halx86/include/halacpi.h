#pragma once

//
// Should be shared with FreeLDR
//
typedef struct _ACPI_E820_ENTRY
{
    PHYSICAL_ADDRESS Base;
    LARGE_INTEGER Length;
    ULONGLONG Type;
} ACPI_E820_ENTRY, *PACPI_E820_ENTRY;

typedef struct _ACPI_BIOS_MULTI_NODE
{
    PHYSICAL_ADDRESS RsdtAddress;
    ULONGLONG Count;
    ACPI_E820_ENTRY E820Entry[1];
} ACPI_BIOS_MULTI_NODE, *PACPI_BIOS_MULTI_NODE;

//
// ACPI Signatures
//
#define RSDP_SIGNATURE 0x2052545020445352       // "RSD PTR "
#define FACS_SIGNATURE 0x53434146               // "FACS"
#define FADT_SIGNATURE 0x50434146               // "FACP"
#define RSDT_SIGNATURE 0x54445352               // "RSDT"
#define APIC_SIGNATURE 0x43495041               // "APIC"
#define DSDT_SIGNATURE 0x54445344               // "DSDT"
#define SSDT_SIGNATURE 0x54445353               // "SSDT"
#define PSDT_SIGNATURE 0x54445350               // "PSDT"
#define SBST_SIGNATURE 0x54534253               // "SBST"
#define DBGP_SIGNATURE 0x50474244               // "DBGP"

//
// FADT Flags
//
#define ACPI_TMR_VAL_EXT 0x100

//
// ACPI Generic Register Address
//
typedef struct _GEN_ADDR
{
    UCHAR AddressSpaceID;
    UCHAR BitWidth;
    UCHAR BitOffset;
    UCHAR Reserved;
    PHYSICAL_ADDRESS Address;
} GEN_ADDR, *PGEN_ADDR;

//
// ACPI BIOS Structures (packed)
//
#include <pshpack1.h>
typedef struct  _RSDP
{
    ULONGLONG Signature;
    UCHAR Checksum;
    UCHAR OEMID[6];
    UCHAR Reserved[1];
    ULONG RsdtAddress;
} RSDP;
typedef RSDP *PRSDP;

typedef struct _DESCRIPTION_HEADER
{
    ULONG Signature;
    ULONG Length;
    UCHAR Revision;
    UCHAR Checksum;
    UCHAR OEMID[6];
    UCHAR OEMTableID[8];
    ULONG OEMRevision;
    UCHAR CreatorID[4];
    ULONG CreatorRev;
} DESCRIPTION_HEADER;
typedef DESCRIPTION_HEADER *PDESCRIPTION_HEADER;

typedef struct _FACS
{
    ULONG Signature;
    ULONG Length;
    ULONG HardwareSignature;
    ULONG pFirmwareWakingVector;
    ULONG GlobalLock;
    ULONG Flags;
    PHYSICAL_ADDRESS x_FirmwareWakingVector;
    UCHAR version;
    UCHAR Reserved[32];
} FACS;
typedef FACS *PFACS;

typedef struct _FADT
{
    DESCRIPTION_HEADER Header;
    ULONG facs;
    ULONG dsdt;
    UCHAR int_model;
    UCHAR pm_profile;
    USHORT sci_int_vector;
    ULONG smi_cmd_io_port;
    UCHAR acpi_on_value;
    UCHAR acpi_off_value;
    UCHAR s4bios_req;
    UCHAR pstate_control;
    ULONG pm1a_evt_blk_io_port;
    ULONG pm1b_evt_blk_io_port;
    ULONG pm1a_ctrl_blk_io_port;
    ULONG pm1b_ctrl_blk_io_port;
    ULONG pm2_ctrl_blk_io_port;
    ULONG pm_tmr_blk_io_port;
    ULONG gp0_blk_io_port;
    ULONG gp1_blk_io_port;
    UCHAR pm1_evt_len;
    UCHAR pm1_ctrl_len;
    UCHAR pm2_ctrl_len;
    UCHAR pm_tmr_len;
    UCHAR gp0_blk_len;
    UCHAR gp1_blk_len;
    UCHAR gp1_base;
    UCHAR cstate_control;
    USHORT lvl2_latency;
    USHORT lvl3_latency;
    USHORT flush_size;
    USHORT flush_stride;
    UCHAR duty_offset;
    UCHAR duty_width;
    UCHAR day_alarm_index;
    UCHAR month_alarm_index;
    UCHAR century_alarm_index;
    USHORT boot_arch;
    UCHAR reserved3[1];
    ULONG flags;
    GEN_ADDR reset_reg;
    UCHAR reset_val;
    UCHAR reserved4[3];
    PHYSICAL_ADDRESS x_firmware_ctrl;
    PHYSICAL_ADDRESS x_dsdt;
    GEN_ADDR x_pm1a_evt_blk;
    GEN_ADDR x_pm1b_evt_blk;
    GEN_ADDR x_pm1a_ctrl_blk;
    GEN_ADDR x_pm1b_ctrl_blk;
    GEN_ADDR x_pm2_ctrl_blk;
    GEN_ADDR x_pm_tmr_blk;
    GEN_ADDR x_gp0_blk;
    GEN_ADDR x_gp1_blk;
} FADT;
typedef FADT *PFADT;

typedef struct _DSDT
{
    DESCRIPTION_HEADER Header;
    UCHAR DiffDefBlock[ANYSIZE_ARRAY];
} DSDT;
typedef DSDT *PDSDT;

typedef struct _RSDT
{
    DESCRIPTION_HEADER Header;
    ULONG Tables[ANYSIZE_ARRAY];
} RSDT;
typedef RSDT *PRSDT;

typedef struct _XSDT
{
    DESCRIPTION_HEADER Header;
    PHYSICAL_ADDRESS Tables[ANYSIZE_ARRAY];
} XSDT;
typedef XSDT *PXSDT;
#include <poppack.h>

//
// Microsoft-specific (pretty much) ACPI Tables, normal MS ABI packing
//
typedef struct _DEBUG_PORT_TABLE
{
    DESCRIPTION_HEADER Header;
    UCHAR InterfaceType;
    UCHAR Reserved[3];
    GEN_ADDR BaseAddress;
} DEBUG_PORT_TABLE, *PDEBUG_PORT_TABLE;

typedef struct _BOOT_TABLE
{
    DESCRIPTION_HEADER Header;
    UCHAR CMOSIndex;
    UCHAR Reserved[3];
} BOOT_TABLE, *PBOOT_TABLE;

typedef struct _ACPI_SRAT
{
    DESCRIPTION_HEADER Header;
    UCHAR TableRevision;
    ULONG Reserved[2];
} ACPI_SRAT, *PACPI_SRAT;

//
// Internal HAL structure
//
typedef struct _ACPI_CACHED_TABLE
{
    LIST_ENTRY Links;
    DESCRIPTION_HEADER Header;
    // table follows
    // ...
} ACPI_CACHED_TABLE, *PACPI_CACHED_TABLE;

NTSTATUS
NTAPI
HalpAcpiTableCacheInit(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

PVOID
NTAPI
HalpAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

NTSTATUS
NTAPI
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

/* EOF */
