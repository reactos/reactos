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
VOID (*OSLOADER_INIT)(
    IN PVOID BoardInit
);

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

//
// Information sent from LLB to OS Loader
//
#define ARM_BOARD_CONFIGURATION_MAJOR_VERSION 1
#define ARM_BOARD_CONFIGURATION_MINOR_VERSION 1
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
} ARM_BOARD_CONFIGURATION_BLOCK, *PARM_BOARD_CONFIGURATION_BLOCK;

/* EOF */
