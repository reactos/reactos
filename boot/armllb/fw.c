/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/fw.c
 * PURPOSE:         LLB Firmware Routines (accessible by OS Loader)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

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
#ifdef _ZOOM2_
    return LlbKeypadGetChar();
#else
    return LlbKeyboardGetChar();
#endif
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


TIMEINFO*
LlbFwGetTime(VOID)
{
    /* Call existing function */
    return LlbGetTime();
}

/* EOF */
