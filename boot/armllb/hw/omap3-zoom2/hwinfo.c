/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines for OMAP3 ZOOM2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

TIMEINFO LlbTime;

#define BCD_INT(bcd) (((bcd & 0xf0) >> 4) * 10 + (bcd &0x0f))

ULONG
NTAPI
LlbHwGetBoardType(VOID)
{
    return MACH_TYPE_OMAP_ZOOM2;
}

ULONG
NTAPI
LlbHwGetPClk(VOID)
{
    return 48000000;
}

ULONG
NTAPI
LlbHwGetTmr0Base(VOID)
{
    return 0x48318000;
}

ULONG
NTAPI
LlbHwGetSerialUart(VOID)
{
    return 0;
}

ULONG
LlbHwRtcRead(VOID)
{
    /* Issue the GET_TIME request on the RTC control register */
    LlbHwOmap3TwlWrite1(0x4B, 0x29, 0x41);

    /* Read the BCD registers and convert them */
    LlbTime.Second = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x1C));
    LlbTime.Minute = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x1D));
    LlbTime.Hour = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x1E));
    LlbTime.Day = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x1F));
    LlbTime.Month = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x20));
    LlbTime.Year = BCD_INT(LlbHwOmap3TwlRead1(0x4B, 0x21));
    LlbTime.Year += (LlbTime.Year > 80) ? 1900 : 2000;
    return 0;
}

/* EOF */
