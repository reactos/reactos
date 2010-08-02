/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3/hwinfo.c
 * PURPOSE:         LLB Hardware Info Routines for OMAP3
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

ULONG
NTAPI
LlbHwGetScreenWidth(VOID)
{
    return 1280;
}

ULONG
NTAPI
LlbHwGetScreenHeight(VOID)
{
     return 720;
}
 
PVOID
NTAPI
LlbHwGetFrameBuffer(VOID)
{
    return (PVOID)0x80500000;
}

ULONG
NTAPI
LlbHwGetBoardType(VOID)
{
    return MACH_TYPE_OMAP3_BEAGLE;
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
LlbHwGetUartBase(IN ULONG Port)
{
    if (Port == 1)
    {
        return 0x4806A000;
    }
    else if (Port == 2)
    {
        return 0x4806C000;        
    }
    else if (Port == 3)
    {
        return 0x49020000;        
    }
}

ULONG
NTAPI
LlbHwGetSerialUart(VOID)
{
    return 3;
} 

/* EOF */
