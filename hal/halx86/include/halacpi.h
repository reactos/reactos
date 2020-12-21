#pragma once

/* ACPI Specification constants */

/* Names within the namespace are 4 bytes long */
#define ACPI_NAMESEG_SIZE               4     // Fixed by ACPI spec

/* Sizes for ACPI table headers */
#define ACPI_OEM_ID_SIZE                6
#define ACPI_OEM_TABLE_ID_SIZE          8

/* Master ACPI Table Header.
   This common header is used by all ACPI tables except the RSDP and FACS.
*/
typedef struct _ACPI_TABLE_HEADER
{
    CHAR Signature[ACPI_NAMESEG_SIZE];        // ASCII table signature
    ULONG Length;                             // Length of table in bytes, including this header
    UCHAR Revision;                           // ACPI Specification minor version number
    UCHAR Checksum;                           // To make sum of entire table == 0
    CHAR OemId[ACPI_OEM_ID_SIZE];             // ASCII OEM identification
    CHAR OemTableId[ACPI_OEM_TABLE_ID_SIZE];  // ASCII OEM table identification
    ULONG OemRevision;                        // OEM revision number
    CHAR AslCompilerId[ACPI_NAMESEG_SIZE];    // ASCII ASL compiler vendor ID
    ULONG AslCompilerRevision;                // ASL compiler version

} ACPI_TABLE_HEADER, *PACPI_TABLE_HEADER;

/* Common subtable headers */

/* Generic subtable header (used in MADT, SRAT, etc.) */
typedef struct _ACPI_SUBTABLE_HEADER
{
    UCHAR Type;
    UCHAR Length;

} ACPI_SUBTABLE_HEADER, *PACPI_SUBTABLE_HEADER;

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
    ACPI_MADT_TYPE_RESERVED                 = 16 // 16 and greater are reserved
};

/* MADT Subtables, correspond to Type in ACPI_SUBTABLE_HEADER */

/* MADT Local APIC flags */
#define ACPI_MADT_ENABLED  1 // Processor is usable if set

/* MADT MPS INTI flags (IntiFlags) */

#define ACPI_MADT_POLARITY_CONFORMS     0
#define ACPI_MADT_POLARITY_ACTIVE_HIGH  1
#define ACPI_MADT_POLARITY_RESERVED     2
#define ACPI_MADT_POLARITY_ACTIVE_LOW   3
#define ACPI_MADT_POLARITY_MASK         (3)      // Polarity of APIC I/O input signals

#define ACPI_MADT_TRIGGER_CONFORMS      (0)
#define ACPI_MADT_TRIGGER_EDGE          (1 << 2)
#define ACPI_MADT_TRIGGER_RESERVED      (2 << 2)
#define ACPI_MADT_TRIGGER_LEVEL         (3 << 2)
#define ACPI_MADT_TRIGGER_MASK          (3 << 2) // Trigger mode of APIC input signals

/* 0: Processor Local APIC */
typedef struct _ACPI_MADT_LOCAL_APIC
{
    ACPI_SUBTABLE_HEADER Header;
    UCHAR ProcessorId;          // ACPI processor id
    UCHAR Id;                   // Processor's local APIC id
    ULONG LapicFlags;

} ACPI_MADT_LOCAL_APIC, *PACPI_MADT_LOCAL_APIC;

/* 1: IO APIC */
typedef struct _ACPI_MADT_IO_APIC
{
    ACPI_SUBTABLE_HEADER Header;
    UCHAR Id;                   // I/O APIC ID
    UCHAR Reserved;             // Reserved - must be zero
    ULONG Address;              // APIC physical address
    ULONG GlobalIrqBase;        // Global system interrupt where INTI lines start

} ACPI_MADT_IO_APIC, *PACPI_MADT_IO_APIC;

/* 2: Interrupt Override */
#include <pshpack1.h>
typedef struct _ACPI_MADT_INTERRUPT_OVERRIDE
{
    ACPI_SUBTABLE_HEADER Header;
    UCHAR Bus;                  // 0 - ISA
    UCHAR SourceIrq;            // Interrupt source (IRQ)
    ULONG GlobalIrq;            // Global system interrupt
    USHORT IntiFlags;

} ACPI_MADT_INTERRUPT_OVERRIDE, *PACPI_MADT_INTERRUPT_OVERRIDE;
#include <poppack.h>

/* 3: NMI Source */
typedef struct _ACPI_MADT_NMI_SOURCE
{
    ACPI_SUBTABLE_HEADER Header;
    USHORT IntiFlags;
    ULONG GlobalIrq;            // Global system interrupt

} ACPI_MADT_NMI_SOURCE, *PACPI_MADT_NMI_SOURCE;

/* 4: Local APIC NMI */
typedef struct _ACPI_MADT_LOCAL_APIC_NMI
{
    ACPI_SUBTABLE_HEADER Header;
    UCHAR ProcessorId;          // ACPI processor id
    USHORT IntiFlags;
    UCHAR Lint;                 // LINTn to which NMI is connected
} ACPI_MADT_LOCAL_APIC_NMI;

/* MADT - Multiple APIC Description Table */

/* Values for PCATCompat flag */
#define ACPI_MADT_MULTIPLE_APIC     0
#define ACPI_MADT_DUAL_PIC          1

typedef struct _ACPI_TABLE_MADT
{
    ACPI_TABLE_HEADER Header;   // Common ACPI table header
    ULONG Address;              // Physical address of local APIC
    ULONG Flags;

} ACPI_TABLE_MADT, *PACPI_TABLE_MADT;

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

PVOID
NTAPI
HalAcpiGetTable(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN ULONG Signature
);

NTSTATUS
NTAPI
HalAcpiGetInterruptTranslator(
    _In_ INTERFACE_TYPE ParentInterfaceType,
    _In_ ULONG ParentBusNumber,
    _In_ INTERFACE_TYPE BridgeInterfaceType,
    _In_ USHORT Size,
    _In_ USHORT Version,
    _Out_ PTRANSLATOR_INTERFACE Translator,
    _Out_ PULONG BridgeBusNumber
);

NTSTATUS
NTAPI
HaliInitPowerManagement(
    _In_ PPM_DISPATCH_TABLE PmDriverDispatchTable,
    _Out_ PPM_DISPATCH_TABLE * PmHalDispatchTable
);

VOID
NTAPI
HalAcpiHaltSystem(
    VOID
);

/* EOF */
