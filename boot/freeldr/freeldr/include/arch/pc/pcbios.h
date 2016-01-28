#ifndef _PCBIOS_H_
#define _PCBIOS_H_

#ifndef __ASM__

typedef enum
{
    BiosMemoryUsable=1,
    BiosMemoryReserved,
    BiosMemoryAcpiReclaim,
    BiosMemoryAcpiNvs
} BIOS_MEMORY_TYPE;

typedef struct
{
    ULONGLONG        BaseAddress;
    ULONGLONG        Length;
    ULONG        Type;
    ULONG        Reserved;
} BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;

/* FIXME: Should be moved to NDK, and respective ACPI header files */
typedef struct _ACPI_BIOS_DATA
{
    PHYSICAL_ADDRESS RSDTAddress;
    ULONGLONG Count;
    BIOS_MEMORY_MAP MemoryMap[1]; /* Count of BIOS memory map entries */
} ACPI_BIOS_DATA, *PACPI_BIOS_DATA;

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

void    EnableA20(void);
VOID __cdecl ChainLoadBiosBootSectorCode(VOID);    // Implemented in boot.S
VOID __cdecl Reboot(VOID);                    // Implemented in boot.S
VOID    DetectHardware(VOID);                 // Implemented in hardware.c

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
