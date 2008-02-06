/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/marcharm.c
 * PURPOSE:         Implements ARM-specific machine initialization
 * PROGRAMMERS:     alex@winsiderss.com
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

//
// The only things we support
//
typedef enum _ARM_BOARD_TYPE
{
    //
    // Marvell Feroceon-based SoC:
    // Buffalo Linkstation, KuroBox Pro, D-Link DS323 and others
    //
    ARM_FEROCEON = 1,
} ARM_BOARD_TYPE;

//
// Compatible boot-loaders should return us this information
//
#define ARM_BOARD_CONFIGURATION_MAJOR_VERSION 1
#define ARM_BOARD_CONFIGURATION_MINOR_VERSION 1
typedef struct _ARM_BOARD_CONFIGURATION_BLOCK
{
    ULONG MajorVersion;
    ULONG MinorVersion;
    ARM_BOARD_TYPE BoardType;
    ULONG TimerRegisterBase;
    ULONG UartRegisterBase;
    PBIOS_MEMORY_MAP MemoryMap;
    CHAR CommandLine[256];
} ARM_BOARD_CONFIGURATION_BLOCK, *PARM_BOARD_CONFIGURATION_BLOCK;

/* FUNCTIONS ******************************************************************/

PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;

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

