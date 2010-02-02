/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

ULONG
NTAPI
LlbHwGetBoardType(VOID)
{
    return MACH_TYPE_VERSATILE_PB;
}

ULONG
NTAPI
LlbHwGetPClk(VOID)
{
    return 24000000;
}

ULONG
NTAPI
LlbHwGetTmr0Base(VOID)
{
    return 0x101E2000;
}

ULONG
NTAPI
LlbHwGetSerialUart(VOID)
{
    return 0;
}     

VOID
NTAPI
LlbHwBuildMemoryMap(IN PBIOS_MEMORY_MAP MemoryMap)
{
    /* For now, hard-code 128MB of RAM starting at 0x00000000 */
    LlbAllocateMemoryEntry(BiosMemoryUsable, 0x00000000, 128 * 1024 * 1024);
    
    /* Mark MMIO space as reserved */
    LlbAllocateMemoryEntry(BiosMemoryReserved, 0x10000000, 128 * 1024 * 1024);
}

/* EOF */
