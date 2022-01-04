/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/omap3-zoom2/hwuart.c
 * PURPOSE:         LLB UART Initialization Routines for OMAP3 ZOOM2
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"
#define SERIAL_REGISTER_STRIDE 2
#include "lib/cportlib/cport.c"

/* GLOBALS ********************************************************************/

#define SERIAL_TL16CP754C_QUAD0_BASE (PVOID)0x10000000

CPPORT LlbHwOmap3UartPorts[4] =
{
    {NULL, 0, 0},
    {NULL, 0, 0},
    {NULL, 0, 0},
    {NULL, 0, 0}
};

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
LlbHwOmap3UartInitialize(VOID)
{
    CpInitialize(&LlbHwOmap3UartPorts[0], SERIAL_TL16CP754C_QUAD0_BASE, 115200);
}

VOID
NTAPI
LlbHwUartSendChar(IN CHAR Char)
{
    /* Send the character */
    CpPutByte(&LlbHwOmap3UartPorts[0], Char);
}

BOOLEAN
NTAPI
LlbHwUartTxReady(VOID)
{
    /* TX output buffer is ready? */
    return TRUE;
}

ULONG
NTAPI
LlbHwGetUartBase(IN ULONG Port)
{
    if (Port == 0)
    {
        return 0x10000000;
    }

    return 0;
}

/* EOF */
