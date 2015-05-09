/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            bios32.h
 * PURPOSE:         VDM 32-bit BIOS
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

#ifndef _BIOS32_H_
#define _BIOS32_H_

/* INCLUDES *******************************************************************/

// #include <bios/bios.h>

/* DEFINES ********************************************************************/

enum
{
    BIOS_MEMORY_AVAILABLE        = 1,
    BIOS_MEMORY_RESERVED         = 2,
    BIOS_MEMORY_ACPI_RECLAIMABLE = 3,
    BIOS_MEMORY_ACPI_NVS         = 4
};

typedef struct
{
    ULONGLONG BaseAddress;
    ULONGLONG Length;
    ULONG Type;
} BIOS_MEMORY_MAP, *PBIOS_MEMORY_MAP;

// #define BIOS_EQUIPMENT_INTERRUPT    0x11
// #define BIOS_MEMORY_SIZE            0x12
// #define BIOS_MISC_INTERRUPT         0x15
// #define BIOS_TIME_INTERRUPT         0x1A
// #define BIOS_SYS_TIMER_INTERRUPT    0x1C

/* FUNCTIONS ******************************************************************/

BOOLEAN Bios32Initialize(VOID);
VOID Bios32Cleanup(VOID);

#endif // _BIOS32_H_

/* EOF */
