/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/os/loader.c
 * PURPOSE:         OS Loader Code for LLB
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

BIOS_MEMORY_MAP MemoryMap[32];
ARM_BOARD_CONFIGURATION_BLOCK ArmBlock;
POSLOADER_INIT LoaderInit;
    
VOID
NTAPI
LlbAllocateMemoryEntry(IN BIOS_MEMORY_TYPE Type,
                      IN ULONG BaseAddress,
                      IN ULONG Length)
{
    PBIOS_MEMORY_MAP Entry;
    
    /* Get the next memory entry */
    Entry = MemoryMap;
    while (Entry->Length) Entry++;
    
    /* Fill it out */
    Entry->Length = Length;
    Entry->BaseAddress = BaseAddress;
    Entry->Type = Type;
    
    /* Block count */
    ArmBlock.MemoryMapEntryCount++;
}

VOID
NTAPI
LlbSetCommandLine(IN PCHAR CommandLine)
{
    /* Copy the command line in the ARM block */
    strcpy(ArmBlock.CommandLine, CommandLine);
}

VOID
NTAPI
LlbBuildArmBlock(VOID)
{
    /* Write version number */
    ArmBlock.MajorVersion = ARM_BOARD_CONFIGURATION_MAJOR_VERSION;
    ArmBlock.MinorVersion = ARM_BOARD_CONFIGURATION_MINOR_VERSION;
    
    /* Get arch type */
    ArmBlock.BoardType = LlbHwGetBoardType();
    
    /* Get peripheral clock rate */
    ArmBlock.ClockRate = LlbHwGetPClk();
    
    /* Get timer and serial port base addresses */
    ArmBlock.TimerRegisterBase = LlbHwGetTmr0Base();
    ArmBlock.UartRegisterBase = LlbHwGetUartBase(LlbHwGetSerialUart());
    
    /* Debug */
    DbgPrint("Machine Identifier: %lx\nPCLK: %d\nTIMER 0: %p\nSERIAL UART: %p\n",
             ArmBlock.BoardType,
             ArmBlock.ClockRate,
             ArmBlock.TimerRegisterBase,
             ArmBlock.UartRegisterBase);
    
    /* Now load the memory map */
    ArmBlock.MemoryMap = MemoryMap;
    
    /* Write firmware callbacks */
    ArmBlock.ConsPutChar = LlbFwPutChar;
    ArmBlock.ConsKbHit = LlbFwKbHit;
    ArmBlock.ConsGetCh = LlbFwGetCh;
    ArmBlock.VideoClearScreen = LlbFwVideoClearScreen;
    ArmBlock.VideoSetDisplayMode = LlbFwVideoSetDisplayMode;
    ArmBlock.VideoGetDisplaySize = LlbFwVideoGetDisplaySize;
    ArmBlock.VideoGetBufferSize = LlbFwVideoGetBufferSize;
    ArmBlock.VideoSetTextCursorPosition = LlbFwVideoSetTextCursorPosition;
    ArmBlock.VideoSetTextCursorPosition = LlbFwVideoSetTextCursorPosition;
    ArmBlock.VideoHideShowTextCursor = LlbFwVideoHideShowTextCursor;
    ArmBlock.VideoPutChar = LlbFwVideoPutChar;
    ArmBlock.VideoCopyOffScreenBufferToVRAM = LlbFwVideoCopyOffScreenBufferToVRAM;
    ArmBlock.VideoIsPaletteFixed = LlbFwVideoIsPaletteFixed;
    ArmBlock.VideoSetPaletteColor = LlbFwVideoSetPaletteColor;
    ArmBlock.VideoGetPaletteColor = LlbFwVideoGetPaletteColor;
    ArmBlock.VideoSync = LlbFwVideoSync;
}

VOID
NTAPI
LlbBuildMemoryMap(VOID)
{
    ULONG Base, Size;
    
    /* Zero out the memory map */
    memset(MemoryMap, 0, sizeof(MemoryMap));
        
    /* Query memory information */
    LlbEnvGetMemoryInformation(&Base, &Size);
    
    /* Don't use memory that the RAMDISK is using */
    /* HACK HACK */
    Base += 32 * 1024 * 1024;
    Size -= 32 * 1024 * 1024;
    
    /* Allocate an entry for it */
    LlbAllocateMemoryEntry(BiosMemoryUsable, Base, Size);

    /* Call the hardware-specific function for hardware-defined regions */
    LlbHwBuildMemoryMap(MemoryMap);
}

VOID
NTAPI
LlbLoadOsLoader(VOID)
{
    PCHAR BootDevice;
    
    /* Read the current boot device */
    BootDevice = LlbEnvRead("boot-device");
    printf("Loading OS Loader from: %s...\n", BootDevice);
    if (!strcmp(BootDevice, "NAND"))
    {
        // todo
    }
    else if (!strcmp(BootDevice, "RAMDISK"))
    {
        /* Call the hardware-specific function */
        LoaderInit = LlbHwLoadOsLoaderFromRam();
    }
    else if (!strcmp(BootDevice, "MMC") ||
             !strcmp(BootDevice, "SD"))
    {
        //todo
    }
    else if (!strcmp(BootDevice, "HDD"))
    {
        //todo
    }
    printf("OS Loader loaded at 0x%p...JUMP!\n\n\n\n\n", LoaderInit);
}

VOID
NTAPI
LlbBoot(VOID)
{
    /* Setup the ARM block */
    LlbBuildArmBlock();
    
    /* Build the memory map */
    LlbBuildMemoryMap();
    
    /* Load the OS loader */
    LlbLoadOsLoader();
    
    /* Jump to the OS Loader (FreeLDR in this case) */
    LoaderInit(&ArmBlock);
}

/* EOF */
