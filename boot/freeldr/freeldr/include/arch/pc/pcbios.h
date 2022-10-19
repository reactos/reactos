#ifndef _PCBIOS_H_
#define _PCBIOS_H_

#ifdef __ASM__
#define EFLAGS_CF HEX(01)
#define EFLAGS_ZF HEX(40)
#define EFLAGS_SF HEX(80)
#endif

#ifndef __ASM__

#define MAX_BIOS_DESCRIPTORS 80

typedef enum
{
    // ACPI 1.0.
    BiosMemoryUsable        =  1,
    BiosMemoryReserved      =  2,
    BiosMemoryAcpiReclaim   =  3,
    BiosMemoryAcpiNvs       =  4,
    // ACPI 3.0.
    BiosMemoryUnusable      =  5,
    // ACPI 4.0.
    BiosMemoryDisabled      =  6,
    // ACPI 6.0.
    BiosMemoryPersistent    =  7,
    BiosMemoryUndefined08   =  8,
    BiosMemoryUndefined09   =  9,
    BiosMemoryUndefined10   = 10,
    BiosMemoryUndefined11   = 11,
    BiosMemoryOemDefined12  = 12
    // BiosMemoryUndefinedNN   = 13-0xEFFFFFFF
    // BiosMemoryOemDefinedNN  = 0xF0000000-0xFFFFFFFF
} BIOS_MEMORY_TYPE;

typedef struct
{
    // ACPI 1.0.
    ULONGLONG   BaseAddress;
    ULONGLONG   Length;
    ULONG       Type;
    // ACPI 3.0.
    union
    {
        ULONG   ExtendedAttributesAsULONG;

        struct
        {
            // Bit 0. ACPI 3.0.
            // As of ACPI 4.0, became "Reserved -> must be 1".
            ULONG Enabled_Reserved : 1;
            // Bit 1. ACPI 3.0.
            // As of ACPI 6.1, became "Unimplemented -> Deprecated".
            // As of ACPI 6.3, became "Reserved -> must be 0".
            ULONG NonVolatile_Deprecated_Reserved : 1;
            // Bit 2. ACPI 4.0.
            // As of ACPI 6.1, became "Unimplemented -> Deprecated".
            // As of ACPI 6.3, became "Reserved -> must be 0".
            ULONG SlowAccess_Deprecated_Reserved : 1;
            // Bit 3. ACPI 4.0.
            // ACPI 5.0-A added "Used only on PC-AT BIOS" (not UEFI).
            ULONG ErrorLog : 1;
            // Bits 4-31. ACPI 3.0.
            ULONG Reserved : 28;
        } ExtendedAttributes;
    };
} BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;

/* Int 15h AX=E820h Entry minimal size. */
C_ASSERT(FIELD_OFFSET(BIOS_MEMORY_MAP, ExtendedAttributes) == 20);
/* Int 15h AX=E820h Entry maximal size. */
C_ASSERT(sizeof(BIOS_MEMORY_MAP) == 24);

/* FIXME: Should be moved to NDK, and respective ACPI header files */
typedef struct _ACPI_BIOS_DATA
{
    PHYSICAL_ADDRESS RSDTAddress;
    ULONGLONG Count;
    BIOS_MEMORY_MAP MemoryMap[1]; /* Count of BIOS memory map entries */
} ACPI_BIOS_DATA, *PACPI_BIOS_DATA;

typedef struct _DOCKING_STATE_INFORMATION
{
    USHORT Unused[5];
    USHORT ReturnCode;
} DOCKING_STATE_INFORMATION, *PDOCKING_STATE_INFORMATION;

#include <pshpack1.h>
typedef struct
{
    unsigned long    eax;
    unsigned long    ebx;
    unsigned long    ecx;
    unsigned long    edx;

    unsigned long    esi;
    unsigned long    edi;
    unsigned long    ebp;

    unsigned short    ds;
    unsigned short    es;
    unsigned short    fs;
    unsigned short    gs;

    unsigned long    eflags;

} DWORDREGS;

typedef struct
{
    unsigned short    ax, _upper_ax;
    unsigned short    bx, _upper_bx;
    unsigned short    cx, _upper_cx;
    unsigned short    dx, _upper_dx;

    unsigned short    si, _upper_si;
    unsigned short    di, _upper_di;
    unsigned short    bp, _upper_bp;

    unsigned short    ds;
    unsigned short    es;
    unsigned short    fs;
    unsigned short    gs;

    unsigned short    flags, _upper_flags;

} WORDREGS;

typedef struct
{
    unsigned char    al;
    unsigned char    ah;
    unsigned short    _upper_ax;
    unsigned char    bl;
    unsigned char    bh;
    unsigned short    _upper_bx;
    unsigned char    cl;
    unsigned char    ch;
    unsigned short    _upper_cx;
    unsigned char    dl;
    unsigned char    dh;
    unsigned short    _upper_dx;

    unsigned short    si, _upper_si;
    unsigned short    di, _upper_di;
    unsigned short    bp, _upper_bp;

    unsigned short    ds;
    unsigned short    es;
    unsigned short    fs;
    unsigned short    gs;

    unsigned short    flags, _upper_flags;

} BYTEREGS;


typedef union
{
    DWORDREGS    x;
    DWORDREGS    d;
    WORDREGS    w;
    BYTEREGS    b;
} REGS;
#include <poppack.h>

// Int386()
//
// Real mode interrupt vector interface
//
// (E)FLAGS can *only* be returned by this function, not set.
// Make sure all memory pointers are in SEG:OFFS format and
// not linear addresses, unless the interrupt handler
// specifically handles linear addresses.
int __cdecl Int386(int ivec, REGS* in, REGS* out);

// This macro tests the Carry Flag
// If CF is set then the call failed (usually)
#define INT386_SUCCESS(regs)    ((regs.x.eflags & EFLAGS_CF) == 0)

VOID __cdecl ChainLoadBiosBootSectorCode(
    IN UCHAR BootDrive OPTIONAL,
    IN ULONG BootPartition OPTIONAL);

VOID __cdecl Relocator16Boot(
    IN REGS*  In,
    IN USHORT StackSegment,
    IN USHORT StackPointer,
    IN USHORT CodeSegment,
    IN USHORT CodePointer);

VOID __cdecl Reboot(VOID);
VOID DetectHardware(VOID);

#endif /* ! __ASM__ */

/* Layout of the REGS structure */
#define REGS_EAX 0
#define REGS_EBX 4
#define REGS_ECX 8
#define REGS_EDX 12
#define REGS_ESI 16
#define REGS_EDI 20
#define REGS_EBP 24
#define REGS_DS 28
#define REGS_ES 30
#define REGS_FS 32
#define REGS_GS 34
#define REGS_EFLAGS 36
#define REGS_SIZE 40

#endif /* _PCBIOS_H_ */
