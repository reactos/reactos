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

ULONG
LlbFwVideoSetDisplayMode(IN PCHAR DisplayModeName,
                         IN BOOLEAN Init)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
    return 0;
}

VOID
LlbFwVideoGetDisplaySize(OUT PULONG Width,
                         OUT PULONG Height,
                         OUT PULONG Depth)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

ULONG
LlbFwVideoGetBufferSize(VOID)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

VOID
LlbFwVideoSetTextCursorPosition(IN ULONG X,
                                IN ULONG Y)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

VOID
LlbFwVideoHideShowTextCursor(IN BOOLEAN Show)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

VOID
LlbFwVideoCopyOffScreenBufferToVRAM(IN PVOID Buffer)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

VOID
LlbFwVideoClearScreen(IN UCHAR Attr)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

VOID
LlbFwVideoPutChar(IN INT c,
                  IN UCHAR Attr,
                  IN ULONG X,
                  IN ULONG Y)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
}

BOOLEAN
LlbFwVideoIsPaletteFixed(VOID)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
    return TRUE;
}

VOID
LlbFwVideoSetPaletteColor(IN UCHAR Color,
                          IN UCHAR Red,
                          IN UCHAR Green,
                          IN UCHAR Blue)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
    return;
}

VOID
LlbFwVideoGetPaletteColor(IN UCHAR Color,
                          OUT PUCHAR Red,
                          OUT PUCHAR Green,
                          OUT PUCHAR Blue)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
    return;
}

VOID
LlbFwVideoSync(VOID)
{
    printf("%s is UNIMPLEMENTED", __FUNCTION__);
    while (TRUE);
    return;
}

/* EOF */
