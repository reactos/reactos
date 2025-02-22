/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-beagle/hwuart.c
 * PURPOSE:         LLB UART Initialization Routines for OMAP3 Beagle
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
LlbHwOmap3UartInitialize(VOID)
{

}

VOID
NTAPI
LlbHwUartSendChar(IN CHAR Char)
{

}

BOOLEAN
NTAPI
LlbHwUartTxReady(VOID)
{
    return FALSE;
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

    return 0;
}

/* EOF */
