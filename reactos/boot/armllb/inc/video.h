/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/inc/video.h
 * PURPOSE:         LLB Videl Output Functions
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

VOID
NTAPI
LlbVideoClearScreen(
    VOID
);

VOID
NTAPI
LlbVideoPutChar(
    IN CHAR c
);

/* EOF */
