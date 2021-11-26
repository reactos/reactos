/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-beagle/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines for OMAP3 Beagle
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

ULONG
NTAPI
LlbHwGetBoardType(VOID)
{
    return MACH_TYPE_OMAP3_BEAGLE;
}

ULONG
NTAPI
LlbHwGetPClk(VOID)
{
    return 48000000;
}

ULONG
NTAPI
LlbHwGetTmr0Base(VOID)
{
    return 0x48318000;
}

ULONG
NTAPI
LlbHwGetSerialUart(VOID)
{
    return 3;
}

VOID
NTAPI
LlbHwKbdSend(IN ULONG Value)
{

}

BOOLEAN
NTAPI
LlbHwKbdReady(VOID)
{
    return FALSE;
}

INT
NTAPI
LlbHwKbdRead(VOID)
{
    return 0;
}

ULONG
NTAPI
LlbHwGetScreenWidth(VOID)
{
    return 1280;
}

ULONG
NTAPI
LlbHwGetScreenHeight(VOID)
{
     return 720;
}

PVOID
NTAPI
LlbHwGetFrameBuffer(VOID)
{
    return (PVOID)0x80500000;
}

ULONG
NTAPI
LlbHwVideoCreateColor(IN ULONG Red,
                      IN ULONG Green,
                      IN ULONG Blue)
{
    return 0;
}


//
// OMAP3 Memory Map
//
BIOS_MEMORY_MAP LlbHwOmap3MemoryMap[] =
{
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
    return 0;
}

/* EOF */
