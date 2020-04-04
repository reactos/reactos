/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware-specific creating a memory map routine for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(MEMORY);

/* pcmem.c */
extern VOID
SetMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType);

/* pcmem.c */
extern VOID
ReserveMemory(
    PFREELDR_MEMORY_DESCRIPTOR MemoryMap,
    ULONG_PTR BaseAddress,
    SIZE_T Size,
    TYPE_OF_MEMORY MemoryType,
    PCHAR Usage);

/* pcmem.c */
extern ULONG
PcMemFinalizeMemoryMap(PFREELDR_MEMORY_DESCRIPTOR MemoryMap);

extern BOOLEAN HiResoMachine;

/* GLOBALS ********************************************************************/

#define KB 1024
#define MB (KB * KB)

static FREELDR_MEMORY_DESCRIPTOR Pc98MemoryMap[MAX_BIOS_DESCRIPTORS + 1];

/* FUNCTIONS ******************************************************************/

PFREELDR_MEMORY_DESCRIPTOR
Pc98MemGetMemoryMap(ULONG *MemoryMapSize)
{
    USHORT ConventionalMemory, ExtendedMemory;
    ULONG ExtendedMemory16;

    TRACE("Pc98MemGetMemoryMap()\n");

    RtlZeroMemory(&PcBiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * MAX_BIOS_DESCRIPTORS);
    PcBiosMapCount = 0;

    ConventionalMemory = ((*(PUCHAR)MEM_BIOS_FLAG1 & CONVENTIONAL_MEMORY_SIZE) + 1) * 128;
    ExtendedMemory = *(PUCHAR)MEM_EXPMMSZ * 128;
    ExtendedMemory16 = (*(PUCHAR)MEM_EXPMMSZ16M_LOW + (*(PUCHAR)MEM_EXPMMSZ16M_HIGH << 8)) * 1024;

    if (ConventionalMemory > 640 && !HiResoMachine)
        ConventionalMemory = 640;

    TRACE("Total conventional memory %d kB available.\n", ConventionalMemory);
    TRACE("Total extended memory %d kB available.\n", ExtendedMemory);
    TRACE("Total extended high memory %d kB available.\n", ExtendedMemory16);
    TRACE("Installed physical memory %d kB.\n", ConventionalMemory + ExtendedMemory + ExtendedMemory16);

    /* First, setup allowed ranges */
    SetMemory(Pc98MemoryMap, 0x0000600, ConventionalMemory * KB, LoaderFree);
    SetMemory(Pc98MemoryMap, 0x0100000, ExtendedMemory * KB, LoaderFree);
    SetMemory(Pc98MemoryMap, 0x1000000, ExtendedMemory16 * KB, LoaderFree);

    /* Next, setup some protected ranges */
    if (HiResoMachine)
    {
        SetMemory(Pc98MemoryMap, 0x000000, 1 * KB, LoaderFirmwarePermanent);  /* Real mode IVT */
        SetMemory(Pc98MemoryMap, 0x000400, 512, LoaderFirmwarePermanent);     /* Real mode BDA */
        SetMemory(Pc98MemoryMap, 0x080000, 256 * KB, LoaderFirmwarePermanent);/* Memory Window */
        SetMemory(Pc98MemoryMap, 0x0C0000, 128 * KB, LoaderFirmwarePermanent);/* VRAM */
        SetMemory(Pc98MemoryMap, 0x0E0000, 16 * KB, LoaderFirmwarePermanent); /* Text VRAM */
        SetMemory(Pc98MemoryMap, 0x0E4000, 4 * KB, LoaderFirmwarePermanent);  /* CG Window */
        SetMemory(Pc98MemoryMap, 0x0E5000, 103 * KB, LoaderSpecialMemory);    /* BIOS ROM */
        SetMemory(Pc98MemoryMap, 0xF00000, 640 * KB, LoaderSpecialMemory);    /* Reserved */
    }
    else
    {
        SetMemory(Pc98MemoryMap, 0x000000, 1 * KB, LoaderFirmwarePermanent);  /* Real mode IVT */
        SetMemory(Pc98MemoryMap, 0x000400, 512, LoaderFirmwarePermanent);     /* Real mode BDA */
        SetMemory(Pc98MemoryMap, 0x000600 + ConventionalMemory * KB,
                  (640 - ConventionalMemory) * KB, LoaderSpecialMemory);      /* External bus */
        SetMemory(Pc98MemoryMap, 0x0A0000, 16 * KB, LoaderFirmwarePermanent); /* Text VRAM */
        SetMemory(Pc98MemoryMap, 0x0A4000, 4 * KB, LoaderFirmwarePermanent);  /* CG Window */
        SetMemory(Pc98MemoryMap, 0x0A5000, 12 * KB, LoaderFirmwarePermanent); /* Reserved */
        SetMemory(Pc98MemoryMap, 0x0A8000, 96 * KB, LoaderFirmwarePermanent); /* VRAM (Plane B, R, G) */
        SetMemory(Pc98MemoryMap, 0x0C0000, 128 * KB, LoaderSpecialMemory);    /* BIOS ROM (Peripherals) */
        SetMemory(Pc98MemoryMap, 0x0E0000, 32 * KB, LoaderFirmwarePermanent); /* VRAM (Plane I) */
        SetMemory(Pc98MemoryMap, 0x0E8000, 96 * KB, LoaderSpecialMemory);     /* BIOS ROM */
        SetMemory(Pc98MemoryMap, 0xF00000, 640 * KB, LoaderSpecialMemory);    /* Reserved */
    }

    *MemoryMapSize = PcMemFinalizeMemoryMap(Pc98MemoryMap);
    return Pc98MemoryMap;
}
