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
    ArmBlock.VideoPutChar = LlbFwVideoPutChar;
    ArmBlock.GetTime = LlbFwGetTime;
}

VOID
NTAPI
LlbBuildMemoryMap(VOID)
{
    /* Zero out the memory map */
    memset(MemoryMap, 0, sizeof(MemoryMap));

    /* Call the hardware-specific function for hardware-defined regions */
    LlbHwBuildMemoryMap(MemoryMap);
}

//
// Should go to hwdev.c
//
POSLOADER_INIT
NTAPI
LlbHwLoadOsLoaderFromRam(VOID)
{
    ULONG Base, RootFs, Size;
    PCHAR Offset;
    CHAR CommandLine[64];

    /* On versatile we load the RAMDISK with initrd */
    LlbEnvGetRamDiskInformation(&RootFs, &Size);
    DbgPrint("Root fs: %lx, size: %lx\n", RootFs, Size);

    /* The OS Loader is at 0x20000, always */
    Base = 0x20000;

    /* Read image offset */
    Offset = LlbEnvRead("rdoffset");

    /* Set parameters for the OS loader */
    snprintf(CommandLine,
             sizeof(CommandLine),
             "rdbase=0x%lx rdsize=0x%lx rdoffset=%s",
             RootFs, Size, Offset);
    LlbSetCommandLine(CommandLine);

    /* Return the OS loader base address */
    return (POSLOADER_INIT)Base;
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

    LoaderInit = (PVOID)0x80000000;
#ifdef _ZOOM2_ // need something better than this...
    LoaderInit = (PVOID)0x81070000;
#endif
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
