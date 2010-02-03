/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/osloader.h
 * PURPOSE:         Shared header between LLB and OS Loader
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

//
// OS Loader Main Routine
// 
typedef
VOID (*POSLOADER_INIT)(
    IN PVOID BoardInit
);

#ifndef __REGISTRY_H
//
// Type of memory detected by LLB
//
typedef enum
{
    BiosMemoryUsable = 1,
    BiosMemoryBootLoader,
    BiosMemoryBootStrap,
    BiosMemoryReserved
} BIOS_MEMORY_TYPE;

//
// Firmware Memory Map
//
typedef struct
{
    LONGLONG BaseAddress;
    LONGLONG Length;
    ULONG Type;
    ULONG Reserved;
} BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;
#endif

//
// Information sent from LLB to OS Loader
//
#define ARM_BOARD_CONFIGURATION_MAJOR_VERSION 1
#define ARM_BOARD_CONFIGURATION_MINOR_VERSION 2
typedef struct _ARM_BOARD_CONFIGURATION_BLOCK
{
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BoardType;
    ULONG ClockRate;
    ULONG TimerRegisterBase;
    ULONG UartRegisterBase;
    ULONG MemoryMapEntryCount;
    PBIOS_MEMORY_MAP MemoryMap;
    CHAR CommandLine[256];
    PVOID ConsPutChar;
    PVOID ConsKbHit;
    PVOID ConsGetCh;
} ARM_BOARD_CONFIGURATION_BLOCK, *PARM_BOARD_CONFIGURATION_BLOCK;

VOID
NTAPI
LlbAllocateMemoryEntry(
    IN BIOS_MEMORY_TYPE Type,
    IN ULONG BaseAddress,
    IN ULONG Length
);

VOID
NTAPI
LlbSetCommandLine(
    IN PCHAR CommandLine
);

VOID
NTAPI
LlbBuildArmBlock(
    VOID
);

VOID
NTAPI
LlbBuildMemoryMap(
    VOID
);

VOID
NTAPI
LlbLoadOsLoader(
    VOID
);

VOID
NTAPI
LlbBoot(
    IN PCHAR CommandLine
);

/* EOF */
