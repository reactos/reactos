/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/marcharm.c
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

ULONG FirstLevelDcacheSize;
ULONG FirstLevelDcacheFillSize;
ULONG FirstLevelIcacheSize;
ULONG FirstLevelIcacheFillSize;
ULONG SecondLevelDcacheSize;
ULONG SecondLevelDcacheFillSize;
ULONG SecondLevelIcacheSize;
ULONG SecondLevelIcacheFillSize;
  
ARC_DISK_SIGNATURE reactos_arc_disk_info;
ULONG reactos_disk_count;
CHAR reactos_arc_hardware_data[256];

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
           (ArmBoardBlock->BoardType == MACH_TYPE_OMAP3_BEAGLE));

    /* Call FreeLDR's portable entrypoint with our command-line */
    BootMain(ArmBoardBlock->CommandLine);
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{
    return;
}

BOOLEAN
ArmDiskGetBootPath(OUT PCHAR BootPath,
                   IN unsigned Size)
{
    PCCH Path = "ramdisk(0)";
    
    /* Make sure enough space exists */
    if (Size < sizeof(Path)) return FALSE;
    
    /* On ARM platforms, the loader is always in RAM */
    strcpy(BootPath, Path);
    return TRUE;
}

PCONFIGURATION_COMPONENT_DATA
ArmHwDetect(VOID)
{
    ARM_CACHE_REGISTER CacheReg;
    
    /* Create the root node */
    if (ArmHwDetectRan++) return RootNode;
    FldrCreateSystemKey(&RootNode);
    
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
    
    /* Register RAMDISK Device */
    RamDiskInitialize();
    
    /* Fill out the ARC disk block */
    reactos_arc_disk_info.Signature = 0xBADAB00F;
    reactos_arc_disk_info.CheckSum = 0xDEADBABE;
    reactos_arc_disk_info.ArcName = "ramdisk(0)";
    reactos_disk_count = 1;
    
    /* Return the root node */
    return RootNode;
}

ULONG
ArmMemGetMemoryMap(OUT PBIOS_MEMORY_MAP BiosMemoryMap,
                   IN ULONG MaxMemoryMapSize)
{
    /* Return whatever the board returned to us (CS0 Base + Size and FLASH0) */
    memcpy(BiosMemoryMap,
           ArmBoardBlock->MemoryMap,
           ArmBoardBlock->MemoryMapEntryCount * sizeof(BIOS_MEMORY_MAP));
    return ArmBoardBlock->MemoryMapEntryCount;
}

VOID
MachInit(IN PCCH CommandLine)
{
    /* Setup board-specific ARM routines */
    switch (ArmBoardBlock->BoardType)
    {
        /* Check for Feroceon-base boards */
        case MACH_TYPE_FEROCEON:
            TuiPrintf("Not implemented\n");
            while (TRUE);
            break;
            
        /* Check for ARM Versatile PB boards */
        case MACH_TYPE_VERSATILE_PB:
            
            /* Copy Machine Routines from Firmware Table */
            MachVtbl.ConsPutChar = ArmBoardBlock->ConsPutChar;
            MachVtbl.ConsKbHit = ArmBoardBlock->ConsKbHit;
            MachVtbl.ConsGetCh = ArmBoardBlock->ConsGetCh;
            MachVtbl.VideoClearScreen = ArmBoardBlock->VideoClearScreen;
            MachVtbl.VideoSetDisplayMode = ArmBoardBlock->VideoSetDisplayMode;
            MachVtbl.VideoGetDisplaySize = ArmBoardBlock->VideoGetDisplaySize;
            MachVtbl.VideoPutChar = ArmBoardBlock->VideoPutChar;
            MachVtbl.GetTime = ArmBoardBlock->GetTime;
                        
            /* Setup the disk and file system buffers */
            gDiskReadBuffer = 0x00090000;
            gFileSysBuffer = 0x00090000;
            break;
            
        /* 
         * Check for TI OMAP3 boards
         * For now that means only Beagle, but ZOOM and others should be ok too
         */
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
    MachVtbl.HwDetect = ArmHwDetect;
    MachVtbl.DiskGetBootPath = ArmDiskGetBootPath;
}
