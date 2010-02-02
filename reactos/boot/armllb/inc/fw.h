/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/fw.h
 * PURPOSE:         LLB Firmware Functions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

VOID
LlbFwPutChar(
    INT Ch
);

BOOLEAN
LlbFwKbHit(
    VOID
);

INT
LlbFwGetCh(
    VOID
);

/* EOF */
