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

VOID
NTAPI
AllocateMemoryEntry(IN BIOS_MEMORY_TYPE Type,
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
    
    /* Now load the memory map */
    ArmBlock.MemoryMap = MemoryMap;
}

/* EOF */
