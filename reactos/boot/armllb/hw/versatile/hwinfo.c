/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

#define PL031_RTC_DR                (LlbHwVersaRtcBase + 0x00)
static const ULONG LlbHwVersaRtcBase = 0x101E8000;

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
    /* Mark MMIO space as reserved */
    LlbAllocateMemoryEntry(BiosMemoryReserved, 0x10000000, 128 * 1024 * 1024);
}

ULONG
LlbHwRtcRead(VOID)
{
    /* Read RTC value */
    return READ_REGISTER_ULONG(PL031_RTC_DR);
}
/* EOF */
