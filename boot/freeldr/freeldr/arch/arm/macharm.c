/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/arm/macharm.c
 * PURPOSE:         Provides abstraction between the ARM Boot Loader and FreeLDR
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <internal/arm/intrin_i.h>

/* GLOBALS ********************************************************************/

PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;
ULONG gDiskReadBuffer, gFileSysBuffer;
BOOLEAN ArmHwDetectRan;
PCONFIGURATION_COMPONENT_DATA RootNode;

#ifndef UEFIBOOT
BOOLEAN AcpiPresent = FALSE;
#endif

ULONG FirstLevelDcacheSize;
ULONG FirstLevelDcacheFillSize;
ULONG FirstLevelIcacheSize;
ULONG FirstLevelIcacheFillSize;
ULONG SecondLevelDcacheSize;
ULONG SecondLevelDcacheFillSize;
ULONG SecondLevelIcacheSize;
ULONG SecondLevelIcacheFillSize;

extern ULONG reactos_disk_count;

ULONG SizeBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    1 << 12, // 4KB
    1 << 13, // 8KB
    1 << 14, // 16KB
    1 << 15, // 32KB
    1 << 16, // 64KB
    1 << 17  // 128KB
};

ULONG AssocBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    4        // 4-way associative
};

ULONG LenBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    8        // 8 words per line (32 bytes)
};

/* FUNCTIONS ******************************************************************/

VOID
FrLdrCheckCpuCompatibility(VOID)
{
    /* Nothing for now */
}

VOID
ArmInit(IN PARM_BOARD_CONFIGURATION_BLOCK BootContext)
{
    /* Remember the pointer */
    ArmBoardBlock = BootContext;

    /* Let's make sure we understand the LLB */
    ASSERT(ArmBoardBlock->MajorVersion == ARM_BOARD_CONFIGURATION_MAJOR_VERSION);
    ASSERT(ArmBoardBlock->MinorVersion == ARM_BOARD_CONFIGURATION_MINOR_VERSION);

    /* This should probably go away once we support more boards */
    ASSERT((ArmBoardBlock->BoardType == MACH_TYPE_FEROCEON) ||
           (ArmBoardBlock->BoardType == MACH_TYPE_VERSATILE_PB) ||
           (ArmBoardBlock->BoardType == MACH_TYPE_OMAP3_BEAGLE) ||
           (ArmBoardBlock->BoardType == MACH_TYPE_OMAP_ZOOM2));

    /* Call FreeLDR's portable entrypoint with our command-line */
    BootMain(ArmBoardBlock->CommandLine);
}

VOID
ArmPrepareForReactOS(VOID)
{
    return;
}

PCONFIGURATION_COMPONENT_DATA
ArmHwDetect(
    _In_opt_ PCSTR Options)
{
    ARM_CACHE_REGISTER CacheReg;

    /* Create the root node */
    if (ArmHwDetectRan++) return RootNode;
    FldrCreateSystemKey(&RootNode, "");

    /*
     * TODO:
     * There's no such thing as "PnP" on embedded hardware.
     * The boot loader will send us a device tree, similar to ACPI
     * or OpenFirmware device trees, and we will convert it to ARC.
     */

    /* Get cache information */
    CacheReg = KeArmCacheRegisterGet();
    FirstLevelDcacheSize = SizeBits[CacheReg.DSize];
    FirstLevelDcacheFillSize = LenBits[CacheReg.DLength];
    FirstLevelDcacheFillSize <<= 2;
    FirstLevelIcacheSize = SizeBits[CacheReg.ISize];
    FirstLevelIcacheFillSize = LenBits[CacheReg.ILength];
    FirstLevelIcacheFillSize <<= 2;
    SecondLevelDcacheSize =
    SecondLevelDcacheFillSize =
    SecondLevelIcacheSize =
    SecondLevelIcacheFillSize = 0;

    /* Initialize the RAMDISK Device */
    RamDiskInitialize(TRUE, NULL, NULL);

    /* Fill out the ARC disk block */
    AddReactOSArcDiskInfo("ramdisk(0)", 0xBADAB00F, 0xDEADBABE, TRUE);
    ASSERT(reactos_disk_count == 1);

    /* Return the root node */
    return RootNode;
}

BOOLEAN
ArmInitializeBootDevices(VOID)
{
    /* Emulate old behavior */
    if (ArmHwDetect(NULL) == NULL)
        return FALSE;

    /* On ARM platforms, the loader is always in RAM */
    strcpy(FrLdrBootPath, "ramdisk(0)");
    return TRUE;
}

FREELDR_MEMORY_DESCRIPTOR ArmMemoryMap[32];

PFREELDR_MEMORY_DESCRIPTOR
ArmMemGetMemoryMap(OUT ULONG *MemoryMapSize)
{
    ULONG i;
    ASSERT(ArmBoardBlock->MemoryMapEntryCount <= 32);

    /* Return whatever the board returned to us (CS0 Base + Size and FLASH0) */
    for (i = 0; i < ArmBoardBlock->MemoryMapEntryCount; i++)
    {
        ArmMemoryMap[i].BasePage = ArmBoardBlock->MemoryMap[i].BaseAddress / PAGE_SIZE;
        ArmMemoryMap[i].PageCount = ArmBoardBlock->MemoryMap[i].Length / PAGE_SIZE;
        if (ArmBoardBlock->MemoryMap[i].Type == BiosMemoryUsable)
            ArmMemoryMap[i].MemoryType = MemoryFree;
        else
            ArmMemoryMap[i].MemoryType = MemoryFirmwarePermanent;
    }

    *MemoryMapSize = ArmBoardBlock->MemoryMapEntryCount;

    // FIXME
    return NULL;
}

VOID
ArmHwIdle(VOID)
{
    /* UNIMPLEMENTED */
}

#ifndef UEFIBOOT
VOID
MachInit(IN PCCH CommandLine)
{
    /* Copy Machine Routines from Firmware Table */
    MachVtbl.ConsPutChar = ArmBoardBlock->ConsPutChar;
    MachVtbl.ConsKbHit = ArmBoardBlock->ConsKbHit;
    MachVtbl.ConsGetCh = ArmBoardBlock->ConsGetCh;
    MachVtbl.VideoClearScreen = ArmBoardBlock->VideoClearScreen;
    MachVtbl.VideoSetDisplayMode = ArmBoardBlock->VideoSetDisplayMode;
    MachVtbl.VideoGetDisplaySize = ArmBoardBlock->VideoGetDisplaySize;
    MachVtbl.VideoPutChar = ArmBoardBlock->VideoPutChar;
    MachVtbl.GetTime = ArmBoardBlock->GetTime;

    /* Setup board-specific ARM routines */
    switch (ArmBoardBlock->BoardType)
    {
        /* Check for Feroceon-base boards */
        case MACH_TYPE_FEROCEON:
            TuiPrintf("Not implemented\n");
            while (TRUE);
            break;

        /* Check for TI OMAP3 ZOOM-II MDK */
        case MACH_TYPE_OMAP_ZOOM2:

            /* Setup the disk and file system buffers */
            gDiskReadBuffer = 0x81094000;
            gFileSysBuffer = 0x81094000;
            break;

        /* Check for ARM Versatile PB boards */
        case MACH_TYPE_VERSATILE_PB:

            /* Setup the disk and file system buffers */
            gDiskReadBuffer = 0x00090000;
            gFileSysBuffer = 0x00090000;
            break;

        /* Check for TI OMAP3 Beagleboard */
        case MACH_TYPE_OMAP3_BEAGLE:
            TuiPrintf("Not implemented\n");
            while (TRUE);
            break;

        default:
            ASSERT(FALSE);
    }

    /* Setup generic ARM routines for all boards */
    MachVtbl.PrepareForReactOS = ArmPrepareForReactOS;
    MachVtbl.GetMemoryMap = ArmMemGetMemoryMap;
    MachVtbl.InitializeBootDevices = ArmInitializeBootDevices;
    MachVtbl.HwDetect = ArmHwDetect;
    MachVtbl.HwIdle = ArmHwIdle;
}
#endif
