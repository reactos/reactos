/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for linear framebuffers
 * COPYRIGHT:   Authors of uefivid.c and xboxvideo.c
 *              Copyright 2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <freeldr.h>
#include "vidfb.h"
#include "vgafont.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

#define VGA_CHAR_SIZE 2

/* This is used to introduce artificial symmetric borders at the top and bottom */
#define TOP_BOTTOM_LINES 0


/* GLOBALS ********************************************************************/

typedef struct _FRAMEBUFFER_INFO
{
    ULONG_PTR BaseAddress;
    ULONG BufferSize;

    /* Horizontal and Vertical resolution in pixels */
    ULONG ScreenWidth;
    ULONG ScreenHeight;

    ULONG PixelsPerScanLine; // aka. "Pitch" or "ScreenStride", but Stride is in bytes or bits...
    ULONG BitsPerPixel;      // aka. "PixelStride".

/** Calculated values */

    ULONG BytesPerPixel;
    ULONG Delta;             // aka. "Pitch": actual size in bytes of a scanline.
} FRAMEBUFFER_INFO, *PFRAMEBUFFER_INFO;

FRAMEBUFFER_INFO framebufInfo;


/* FUNCTIONS ******************************************************************/

#if DBG
static VOID
VidFbPrintFramebufferInfo(VOID)
{
    TRACE("Framebuffer format:\n");
    TRACE("    BaseAddress       : 0x%X\n", framebufInfo.BaseAddress);
    TRACE("    BufferSize        : %lu\n", framebufInfo.BufferSize);
    TRACE("    ScreenWidth       : %lu\n", framebufInfo.ScreenWidth);
    TRACE("    ScreenHeight      : %lu\n", framebufInfo.ScreenHeight);
    TRACE("    PixelsPerScanLine : %lu\n", framebufInfo.PixelsPerScanLine);
    TRACE("    BitsPerPixel      : %lu\n", framebufInfo.BitsPerPixel);
    TRACE("    BytesPerPixel     : %lu\n", framebufInfo.BytesPerPixel);
    TRACE("    Delta             : %lu\n", framebufInfo.Delta);
}
#endif

/**
 * @brief
 * Initializes internal framebuffer data based on the given parameters.
 *
 * @param[in]   BaseAddress
 * The framebuffer physical base address.
 *
 * @param[in]   BufferSize
 * The framebuffer size, in bytes.
 *
 * @param[in]   ScreenWidth
 * @param[in]   ScreenHeight
 * The width and height of the visible framebuffer area, in pixels.
 *
 * @param[in]   PixelsPerScanLine
 * The size in number of pixels of a whole horizontal video memory scanline.
 *
 * @param[in]   BitsPerPixel
 * The number of usable bits (not counting the reserved ones) per pixel.
 *
 * @return
 * TRUE if initialization is successful; FALSE if not.
 **/
BOOLEAN
VidFbInitializeVideo(
    _In_ ULONG_PTR BaseAddress,
    _In_ ULONG BufferSize,
    _In_ UINT32 ScreenWidth,
    _In_ UINT32 ScreenHeight,
    _In_ UINT32 PixelsPerScanLine,
    _In_ UINT32 BitsPerPixel)
{
    RtlZeroMemory(&framebufInfo, sizeof(framebufInfo));

    framebufInfo.BaseAddress  = BaseAddress;
    framebufInfo.BufferSize   = BufferSize;
    framebufInfo.ScreenWidth  = ScreenWidth;
    framebufInfo.ScreenHeight = ScreenHeight;
    framebufInfo.PixelsPerScanLine = PixelsPerScanLine;
    framebufInfo.BitsPerPixel = BitsPerPixel;

    framebufInfo.BytesPerPixel = ((BitsPerPixel + 7) & ~7) / 8; // Round up to nearest byte.
    framebufInfo.Delta = (PixelsPerScanLine * framebufInfo.BytesPerPixel + 3) & ~3;

    /* We currently only support 32bpp */
    if (BitsPerPixel != 32)
    {
        /* Unsupported BPP */
        ERR("Unsupported %lu bits per pixel format\n", BitsPerPixel);
        return FALSE;
    }

#if DBG
    VidFbPrintFramebufferInfo();
#endif

    return TRUE;
}

static inline
UINT32
VidFbAttrToSingleColor(
    _In_ UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
VidFbAttrToColors(
    _In_ UCHAR Attr,
    _Out_ PUINT32 FgColor,
    _Out_ PUINT32 BgColor)
{
    *FgColor = VidFbAttrToSingleColor(Attr & 0x0F);
    *BgColor = VidFbAttrToSingleColor((Attr >> 4) & 0x0F);
}

VOID
VidFbClearScreenColor(
    _In_ UINT32 Color,
    _In_ BOOLEAN FullScreen)
{
    ULONG Line, Col;
    PUINT32 p;

    for (Line = 0; Line < framebufInfo.ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * framebufInfo.Delta);
        for (Col = 0; Col < framebufInfo.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

VOID
VidFbClearScreen(
    _In_ UCHAR Attr)
{
    UINT32 FgColor, BgColor;
    VidFbAttrToColors(Attr, &FgColor, &BgColor);
    VidFbClearScreenColor(BgColor, FALSE);
}

VOID
VidFbOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor)
{
    const UCHAR* FontPtr;
    PUINT32 Pixel;
    UCHAR Mask;
    ULONG Line, Col;

    FontPtr = BitmapFont8x16 + Char * CHAR_HEIGHT;
    Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
            (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) * framebufInfo.Delta + X * CHAR_WIDTH * 4);

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (PUINT32)((PUCHAR)Pixel + framebufInfo.Delta);
    }
}

VOID
VidFbPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG X,
    _In_ ULONG Y)
{
    UINT32 FgColor, BgColor;
    VidFbAttrToColors(Attr, &FgColor, &BgColor);
    VidFbOutputChar(Char, X, Y, FgColor, BgColor);
}

VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    *Width  = framebufInfo.ScreenWidth / CHAR_WIDTH;
    *Height = (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    *Depth  = framebufInfo.BitsPerPixel;
}

ULONG
VidFbGetBufferSize(VOID)
{
    return ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT *
            (framebufInfo.ScreenWidth / CHAR_WIDTH) * VGA_CHAR_SIZE);
}

VOID
VidFbCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;
    ULONG Line, Col;

    for (Line = 0; Line < (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < framebufInfo.ScreenWidth / CHAR_WIDTH; Col++)
        {
            VidFbPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += VGA_CHAR_SIZE;
        }
    }
}

VOID
VidFbScrollUp(
    _In_ UCHAR Attr)
{
    UINT32 BgColor, Dummy;
    ULONG PixelCount = framebufInfo.ScreenWidth * CHAR_HEIGHT *
                       (((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PUINT32 Src = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * framebufInfo.Delta);
    PUINT32 Dst = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + TOP_BOTTOM_LINES * framebufInfo.Delta);

    VidFbAttrToColors(Attr, &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < framebufInfo.ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

#if 0
VOID
VidFbSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
VidFbHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
VidFbIsPaletteFixed(VOID)
{
    return FALSE;
}

VOID
VidFbSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red, _In_ UCHAR Green, _In_ UCHAR Blue)
{
    /* Not supported */
}

VOID
VidFbGetPaletteColor(
    _In_ UCHAR Color,
    _Out_ PUCHAR Red, _Out_ PUCHAR Green, _Out_ PUCHAR Blue)
{
    /* Not supported */
}
#endif
