/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/marcharm.c
 * PURPOSE:         Provides abstraction between the ARM Boot Loader and FreeLDR
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;
ULONG BootDrive, BootPartition;

/* FUNCTIONS ******************************************************************/

VOID
ArmInit(IN PARM_BOARD_CONFIGURATION_BLOCK BootContext)
{
    //
    // Remember the pointer
    //
    ArmBoardBlock = BootContext;
    
    //
    // Let's make sure we understand the boot-loader
    //
    ASSERT(ArmBoardBlock->MajorVersion == ARM_BOARD_CONFIGURATION_MAJOR_VERSION);
    ASSERT(ArmBoardBlock->MinorVersion == ARM_BOARD_CONFIGURATION_MINOR_VERSION);
    
    //
    // This should probably go away once we support more boards
    //
    ASSERT(ArmBoardBlock->BoardType == ARM_FEROCEON);

    //
    // Call FreeLDR's portable entrypoint with our command-line
    //
    BootMain(ArmBoardBlock->CommandLine);
}

