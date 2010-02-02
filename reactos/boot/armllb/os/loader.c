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
    printf("Command Line %s\n", CommandLine);
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
    printf("Machine Identifier: %lx\nPCLK: %d\nTIMER 0: %p\nSERIAL UART: %p\n",
            ArmBlock.BoardType,
            ArmBlock.ClockRate,
            ArmBlock.TimerRegisterBase,
            ArmBlock.UartRegisterBase);
    
    /* Now load the memory map */
    ArmBlock.MemoryMap = MemoryMap;
}

VOID
NTAPI
LlbBuildMemoryMap(VOID)
{
    /* Zero out the memory map */
    memset(MemoryMap, 0, sizeof(MemoryMap));

    /* Call the hardware-specific function */
    LlbHwBuildMemoryMap(MemoryMap);
}

VOID
NTAPI
LlbLoadOsLoader(VOID)
{
    PCHAR BootDevice;
    
    /* Read the current boot device */
    BootDevice = LlbHwEnvRead("boot-device");
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
    printf("OS Loader loaded at 0x%p...JUMP!\n", LoaderInit);
}

VOID
NTAPI
LlbBoot(IN PCHAR CommandLine)
{
    /* Setup the ARM block */
    LlbBuildArmBlock();
    
    /* Build the memory map */
    LlbBuildMemoryMap();
    
    /* Set the command-line */
    LlbSetCommandLine(CommandLine);
    
    /* Load the OS loader */
    LlbLoadOsLoader();
    
    /* Jump to the OS Loader (FreeLDR in this case) */
    LoaderInit(&ArmBlock);
}

/* EOF */
