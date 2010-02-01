/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwinit.c
 * PURPOSE:         LLB Hardware Initialization Routines for Versatile
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

VOID
NTAPI
LlbHwInitialize(VOID)
{
    /* Setup the CLCD (PL110) */
    LlbHwVersaClcdInitialize();
    
    /* Setup the UART (PL011) */
    LlbHwVersaUartInitialize();
}

/* EOF */
