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
    IN BOOLEAN OsLoader
);

VOID
NTAPI
LlbVideoPutChar(
    IN UCHAR c
);

VOID
NTAPI
LlbVideoDrawChar(
    IN UCHAR c,
    IN PUSHORT Buffer,
    IN USHORT Color,
    IN USHORT BackColor
);

/* EOF */
