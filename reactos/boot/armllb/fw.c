/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/fw.c
 * PURPOSE:         LLB Firmware Routines (accessible by OS Loader)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

VOID
LlbFwPutChar(INT Ch)
{
    /* Just call directly the video function */
    LlbVideoPutChar(Ch);

    /* DEBUG ONLY */
    LlbSerialPutChar(Ch);
}

BOOLEAN
LlbFwKbHit(VOID)
{
    /* Not yet implemented */
    return FALSE;
}

INT
LlbFwGetCh(VOID)
{
    /* Return the key pressed */
    return LlbKeyboardGetChar();
}

/* EOF */
