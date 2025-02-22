/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwinit.c
 * PURPOSE:         LLB UART Initialization Routines for OMAP3 ZOOM2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

//
// OMAP3 Memory Map
//
// 0x00000000 - 0x3FFFFFFF GPMC                                     [  1 GB]
// 0x40000000 - 0x47FFFFFF On-Chip Memory (ROM/SRAM Address Space)  [128 MB]
// 0x48000000 - 0x4FFFFFFF L4 Interconnects (All system peripherals)[128 MB]
// 0x50000000 - 0x53FFFFFF SGX Graphics Accelerator Slave Port      [ 64 MB]
// 0x54000000 - 0x57FFFFFF L4 Emulation                             [128 MB]
// 0x58000000 - 0x58FFFFFF Reserved                                 [ 64 MB]
// 0x5C000000 - 0x5FFFFFFF IVA2.2 Subsystem                         [ 64 MB]
// 0x60000000 - 0x67FFFFFF Reserved                                 [128 MB]
// 0x68000000 - 0x6FFFFFFF L3 Interconnect (Control Registers)      [128 MB]
// 0x70000000 - 0x7FFFFFFF SDRC/SMS Virtual Address Space 0         [256 MB]
// 0x80000000 - 0x9FFFFFFF SDRC/SMS CS0 SDRAM                       [512 MB]
    // 0x80000000 - 0x80FFFFFF KERNEL, HAL, BOOT DRIVERS            [ 16 MB]
// THIS IS THE x86-STYLE "LOW 1MB" THAT IS IDENTITY MAPPED
    // 0x81000000 - 0x8100FFFF ARM LLB                              [ 64 KB]
    // 0x81010000 - 0x81013FFF ARM BOOT STACK                       [ 16 KB]
    // 0x81014000 - 0x81073FFF ARM FRAMEBUFFER                      [384 KB]
    // 0x81070000 - 0x81093FFF ARM OS LOADER                        [128 KB]
    // 0x81094000 - 0x810FFFFF RESERVED FOR BOOT LOADER EXPANSION   [432 KB]
// END OF THE x86-STYLE "LOW 1MB" THAT IS IDENTITY MAPPED
    // 0x81100000 - 0x8FFFFFFF FREE RAM                             [ 15 MB]
    // 0x82000000 - 0x83FFFFFF ARM RAMDISK                          [ 32 MB]
    // 0x84000000 - 0x8FFFFFFF FREE RAM                             [192 MB]
    // 0x90000000 - 0x9FFFFFFF FREE RAM IF > 256MB INSTALLED        [256 MB]
// 0xA0000000 - 0xBFFFFFFF SDRC/SMS CS1 SDRAM                       [512 MB]
    // 0xA0000000 - 0xAFFFFFFF FREE RAM IF > 512MB INSTALLED        [256 MB]
    // 0xB0000000 - 0xBFFFFFFF FREE RAM IF > 768MB INSTALLED        [256 MB]
// 0xC0000000 - 0xDFFFFFFF Reserved                                 [512 MB]
// 0xE0000000 - 0xFFFFFFFF SDRC/SMS Virtual Address Space 1         [512 MB]
BIOS_MEMORY_MAP LlbHwOmap3MemoryMap[] =
{
    {0x00000000, 0x80000000, BiosMemoryReserved,   0}, /* Device Registers */
    {0x80000000, 0x01000000, BiosMemoryUsable,     0}, /* 16 MB RAM for Kernel map */
    {0x81000000, 0x00010000, BiosMemoryBootLoader, 0}, /* Arm LLB */
    {0x81010000, 0x00004000, BiosMemoryBootStrap,  0}, // LLB Stack
    {0x81014000, 0x00060000, BiosMemoryBootLoader, 0}, /* Kernel Framebuffer */
    {0x81070000, 0x00020000, BiosMemoryBootStrap,  0}, /* ARM OS Loader */
    {0x81094000, 0x0006C000, BiosMemoryBootStrap,  0}, /* ARM OS Loader Expansion */
    {0x81100000, 0x00F00000, BiosMemoryUsable,     0}, /* 15 MB Free RAM */
    {0x82000000, 0x02000000, BiosMemoryBootStrap,  0}, /* 32MB RAMDISK */
    {0x84000000, 0x0C000000, BiosMemoryUsable,     0}, /* 192 MB Free RAM */
    {0x90000000, 0x70000000, BiosMemoryReserved,   0},
    {0, 0, 0, 0}
};

VOID
NTAPI
LlbHwBuildMemoryMap(IN PBIOS_MEMORY_MAP MemoryMap)
{
    PBIOS_MEMORY_MAP MapEntry;
    ULONG Base, Size, FsBase, FsSize;

    /* Parse hardware memory map */
    MapEntry = LlbHwOmap3MemoryMap;
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
    if (!FsSize) return;
#ifdef _BEAGLE_
    /* Add-in the size of the ramdisk */
    Base = FsBase + FsSize;

    /* Subtract size of ramdisk and anything else before it */
    Size -= Base;

    /* Allocate an entry for it */
    LlbAllocateMemoryEntry(BiosMemoryUsable, Base, Size);
#endif
}

VOID
NTAPI
LlbHwInitialize(VOID)
{
    /* Setup the UART (NS16550) */
    LlbHwOmap3UartInitialize();

    /* Setup the NEC WVGA LCD Panel and the Display Controller */
    LlbHwOmap3LcdInitialize();

    /* Setup the keyboard */
    LlbHwOmap3SynKpdInitialize();
}

/* EOF */
