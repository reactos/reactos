/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define PL031_RTC_DR                (LlbHwVersaRtcBase + 0x00)
static const ULONG LlbHwVersaRtcBase = 0x101E8000;

ULONG
NTAPI
LlbHwGetBoardType(VOID)
{
    return MACH_TYPE_VERSATILE_PB;
}

ULONG
NTAPI
LlbHwGetPClk(VOID)
{
    return 24000000;
}

ULONG
NTAPI
LlbHwGetTmr0Base(VOID)
{
    return 0x101E2000;
}

ULONG
NTAPI
LlbHwGetSerialUart(VOID)
{
    return 0;
}

//
// Versatile Memory Map
//
// 0x00000000 - 0x000000FF ARM Vectors                              [  1 KB]
// 0x00000100 - 0x000001FF ATAG Structures                          [  1 KB]
// 0x00000200 - 0x0000FFFF ARM STACK                                [ 62 KB]
// 0x00010000 - 0x0001FFFF ARM LLB                                  [ 64 KB]
// 0x00020000 - 0x0009FFFF ARM OS LOADER                            [512 KB]
// 0x000A0000 - 0x0013FFFF ARM FRAMEBUFFER                          [640 KB]
// 0x00140000 - 0x007FFFFF OS LOADER FREE/UNUSED                    [  6 MB]
// 0x00800000 - 0x017FFFFF KERNEL, HAL, INITIAL DRIVER LOAD ADDR    [ 16 MB]
// 0x01800000 - 0x037FFFFF RAM DISK                                 [ 32 MB]
// 0x03800000 - 0x07FFFFFF FREE RAM                                 [ 72 MB]
// 0x08000000 - 0x0FFFFFFF FREE RAM IF 256MB DEVICE                 [128 MB]
// 0x10000000 - 0x1FFFFFFF MMIO DEVICES                             [256 MB]
BIOS_MEMORY_MAP LlbHwVersaMemoryMap[] =
{
    {0x00000000, 0x00000100, BiosMemoryReserved,   0},
    {0x00000100, 0x00000100, BiosMemoryBootStrap,  0},
    {0x00000200, 0x0000FE00, BiosMemoryBootStrap,  0},
    {0x00010000, 0x00010000, BiosMemoryBootStrap,  0},
    {0x00020000, 0x00080000, BiosMemoryBootLoader, 0},
    {0x000A0000, 0x000A0000, BiosMemoryBootLoader, 0},
    {0x00140000, 0x016C0000, BiosMemoryUsable,     0},
    {0x01800000, 0x02000000, BiosMemoryReserved,   0},
    {0x10000000, 0x10000000, BiosMemoryReserved,   0},
    {0, 0, 0, 0}
};

VOID
NTAPI
LlbHwBuildMemoryMap(IN PBIOS_MEMORY_MAP MemoryMap)
{
    PBIOS_MEMORY_MAP MapEntry;
    ULONG Base, Size, FsBase, FsSize;

    /* Parse hardware memory map */
    MapEntry = LlbHwVersaMemoryMap;
    while (MapEntry->Length)
    {
        /* Add this entry */
        LlbAllocateMemoryEntry(MapEntry->Type, MapEntry->BaseAddress, MapEntry->Length);

        /* Move to the next one */
        MapEntry++;
    }

    /* Query memory and RAMDISK information */
    LlbEnvGetMemoryInformation(&Base, &Size);
    LlbEnvGetRamDiskInformation(&FsBase, &FsSize);

    /* Add-in the size of the ramdisk */
    Base = FsBase + FsSize;

    /* Subtract size of ramdisk and anything else before it */
    Size -= Base;

    /* Allocate an entry for it */
    LlbAllocateMemoryEntry(BiosMemoryUsable, Base, Size);
}

ULONG
LlbHwRtcRead(VOID)
{
    /* Read RTC value */
    return READ_REGISTER_ULONG(PL031_RTC_DR);
}
/* EOF */
