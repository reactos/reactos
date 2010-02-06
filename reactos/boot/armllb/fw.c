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
    /* Check RX buffer */
    return LlbHwKbdReady();
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
    /* Return text mode */
    return 0;
}

VOID
LlbFwVideoGetDisplaySize(OUT PULONG Width,
                         OUT PULONG Height,
                         OUT PULONG Depth)
{
    /* Query static settings */
    *Width = LlbHwGetScreenWidth() / 8;
    *Height = LlbHwGetScreenHeight() / 16;
    
    /* Depth is always 16 bpp */
    *Depth = 16;
}

ULONG
LlbFwVideoGetBufferSize(VOID)
{
    /* X * Y * BPP */
    return LlbHwGetScreenWidth() * LlbHwGetScreenHeight() * 2;
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
    /* Nothing to do */
    return;
}

USHORT ColorPalette[16][3] =
{
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0xAA},
    {0x00, 0xAA, 0x00},
    {0x00, 0xAA, 0xAA},
    {0xAA, 0x00, 0x00},
    {0xAA, 0x00, 0xAA},
    {0xAA, 0x55, 0x00},
    {0xAA, 0xAA, 0xAA},
    {0x55, 0x55, 0x55},
    {0x55, 0x55, 0xFF},
    {0x55, 0xFF, 0x55},
    {0x55, 0xFF, 0xFF},
    {0xFF, 0x55, 0x55},
    {0xFF, 0x55, 0xFF},
    {0xFF, 0xFF, 0x55},
    {0xFF, 0xFF, 0xFF},
};

VOID
LlbFwVideoCopyOffScreenBufferToVRAM(IN PVOID Buffer)
{
    /* No double-buffer is used on ARM */
    return;
}

VOID
LlbFwVideoClearScreen(IN UCHAR Attr)
{
    /* Clear the screen */
    LlbVideoClearScreen(TRUE);
}

VOID
LlbFwVideoPutChar(IN INT c,
                  IN UCHAR Attr,
                  IN ULONG X,
                  IN ULONG Y)
{
    ULONG Color, BackColor;
    PUSHORT Buffer;
    
    /* Convert EGA index to color used by hardware */
    Color = LlbHwVideoCreateColor(ColorPalette[Attr & 0xF][0],
                                  ColorPalette[Attr & 0xF][1],
                                  ColorPalette[Attr & 0xF][2]);
    BackColor = LlbHwVideoCreateColor(ColorPalette[Attr >> 4][0],
                                      ColorPalette[Attr >> 4][1],
                                      ColorPalette[Attr >> 4][2]);
                                      
    /* Compute buffer address */
    Buffer = (PUSHORT)LlbHwGetFrameBuffer() + (LlbHwGetScreenWidth() * (Y * 16)) + (X * 8);
                                      
    /* Draw it */
    LlbVideoDrawChar(c, Buffer, Color, BackColor);
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

TIMEINFO*
LlbFwGetTime(VOID)
{
    /* Call existing function */
    return LlbGetTime();
}

/* EOF */
