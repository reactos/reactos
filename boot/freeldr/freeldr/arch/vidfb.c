/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for linear framebuffers
 * COPYRIGHT:   Authors of uefivid.c and xboxvideo.c
 *              Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <freeldr.h>
#include "vidfb.h"
#include "vgafont.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

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

    /* Number of pixel elements per video memory line */
    ULONG PixelsPerScanLine; // aka. "Pitch" or "ScreenStride", but Stride is in bytes or bits...
    ULONG BitsPerPixel;      // aka. "PixelStride".

    /* Physical format of the pixel for BPP > 8, specified by bit-mask */
    PIXEL_BITMASK PixelMasks;

/** Calculated values */

    ULONG BytesPerPixel;
    ULONG Delta;             // aka. "Pitch": actual size in bytes of a scanline.
} FRAMEBUFFER_INFO, *PFRAMEBUFFER_INFO;

static FRAMEBUFFER_INFO framebufInfo = {0};
static CM_FRAMEBUF_DEVICE_DATA FrameBufferData = {0};


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
    TRACE("    ARGB masks:       : %08x/%08x/%08x/%08x\n",
          framebufInfo.PixelMasks.ReservedMask,
          framebufInfo.PixelMasks.RedMask,
          framebufInfo.PixelMasks.GreenMask,
          framebufInfo.PixelMasks.BlueMask);
}
#endif

/**
 * @brief
 * Initializes internal framebuffer information based on the given parameters.
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
 * @param[in]   PixelMasks
 * Optional pointer to a PIXEL_BITMASK structure describing the pixel
 * format used by the framebuffer.
 *
 * @return
 * TRUE if initialization is successful; FALSE if not.
 **/
BOOLEAN
VidFbInitializeVideo(
    _Out_opt_ PCM_FRAMEBUF_DEVICE_DATA* pFbData,
    _In_ ULONG_PTR BaseAddress,
    _In_ ULONG BufferSize,
    _In_ UINT32 ScreenWidth,
    _In_ UINT32 ScreenHeight,
    _In_ UINT32 PixelsPerScanLine,
    _In_ UINT32 BitsPerPixel,
    _In_opt_ PPIXEL_BITMASK PixelMasks)
{
    PPIXEL_BITMASK BitMasks = &framebufInfo.PixelMasks;

    if (pFbData)
        *pFbData = NULL;

    RtlZeroMemory(&framebufInfo, sizeof(framebufInfo));

    /* Verify framebuffer dimensions */
    if ((ScreenWidth < 1) || (ScreenHeight < 1))
    {
        ERR("Invalid framebuffer dimensions\n");
        return FALSE;
    }

    framebufInfo.BaseAddress  = BaseAddress;
    framebufInfo.BufferSize   = BufferSize;
    framebufInfo.ScreenWidth  = ScreenWidth;
    framebufInfo.ScreenHeight = ScreenHeight;
    framebufInfo.PixelsPerScanLine = PixelsPerScanLine;
    framebufInfo.BitsPerPixel = BitsPerPixel;

    framebufInfo.BytesPerPixel = (BitsPerPixel + 7) / 8; // Round up to nearest byte.
    framebufInfo.Delta = (PixelsPerScanLine * framebufInfo.BytesPerPixel + 3) & ~3;

    /* Verify that the framebuffer fits inside the video RAM */
    if (!(ScreenHeight * framebufInfo.Delta <= BufferSize))
    {
        ERR("Framebuffer doesn't fit inside the video RAM (FB size: %lu, VRAM size: %lu)\n",
            ScreenHeight * framebufInfo.Delta, BufferSize);
        return FALSE;
    }

    /* We currently only support 32bpp */
    if (BitsPerPixel != 32)
    {
        /* Unsupported BPP */
        ERR("Unsupported %lu bits per pixel format\n", BitsPerPixel);
        return FALSE;
    }

    //ASSERT((BitsPerPixel <= 8 && !PixelMasks) || (BitsPerPixel > 8));
    if (BitsPerPixel > 8)
    {
        if (!PixelMasks ||
            (PixelMasks->RedMask   == 0 &&
             PixelMasks->GreenMask == 0 &&
             PixelMasks->BlueMask  == 0 /* &&
             PixelMasks->ReservedMask == 0 */))
        {
            /* Determine pixel mask given color depth and color channel */
            switch (BitsPerPixel)
            {
                case 32:
                case 24: /* 8:8:8 */
                    BitMasks->RedMask   = 0x00FF0000; // 0x00FF0000;
                    BitMasks->GreenMask = 0x0000FF00; // 0x00FF0000 >> 8;
                    BitMasks->BlueMask  = 0x000000FF; // 0x00FF0000 >> 16;
                    BitMasks->ReservedMask = ((1 << (BitsPerPixel - 24)) - 1) << 24;
                    break;
                case 16: /* 5:6:5 */
                    BitMasks->RedMask   = 0xF800; // 0xF800;
                    BitMasks->GreenMask = 0x07E0; // (0xF800 >> 5) | 0x20;
                    BitMasks->BlueMask  = 0x001F; // 0xF800 >> 11;
                    BitMasks->ReservedMask = 0;
                    break;
                case 15: /* 5:5:5 */
                    BitMasks->RedMask   = 0x7C00; // 0x7C00;
                    BitMasks->GreenMask = 0x03E0; // 0x7C00 >> 5;
                    BitMasks->BlueMask  = 0x001F; // 0x7C00 >> 10;
                    BitMasks->ReservedMask = 0x8000;
                    break;
                default:
                    /* Unsupported BPP */
                    UNIMPLEMENTED;
                    RtlZeroMemory(BitMasks, sizeof(*BitMasks));
            }
        }
        else
        {
            /* Copy the pixel masks */
            RtlCopyMemory(BitMasks, PixelMasks, sizeof(*BitMasks));
        }
    }
    else
    {
        /* Palettized modes don't use masks */
        RtlZeroMemory(BitMasks, sizeof(*BitMasks));
    }

#if DBG
    VidFbPrintFramebufferInfo();
    {
    ULONG BppFromMasks =
        PixelBitmasksToBpp(BitMasks->RedMask,
                           BitMasks->GreenMask,
                           BitMasks->BlueMask,
                           BitMasks->ReservedMask);
    TRACE("BitsPerPixel = %lu , BppFromMasks = %lu\n", BitsPerPixel, BppFromMasks);
    //ASSERT(BitsPerPixel == BppFromMasks);
    }
#endif

    /* Initialize the hardware device configuration data if specified */
    if (pFbData)
    {
        FrameBufferData.FrameBufferOffset = 0;
        FrameBufferData.ScreenWidth  = framebufInfo.ScreenWidth;
        FrameBufferData.ScreenHeight = framebufInfo.ScreenHeight;
        FrameBufferData.PixelsPerScanLine = framebufInfo.PixelsPerScanLine;
        FrameBufferData.BitsPerPixel = framebufInfo.BitsPerPixel;

        RtlCopyMemory(&FrameBufferData.PixelMasks,
                      &framebufInfo.PixelMasks, sizeof(framebufInfo.PixelMasks));

        *pFbData = &FrameBufferData;
    }

    return TRUE;
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

/**
 * @brief
 * Displays a character at a given pixel position with specific foreground
 * and background colors.
 **/
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

    /* Don't display outside of the screen, nor partial characters */
    if ((X + CHAR_WIDTH - 1 >= framebufInfo.ScreenWidth) ||
        (Y + CHAR_HEIGHT - 1 >= (framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES)))
    {
        return;
    }

    FontPtr = BitmapFont8x16 + Char * CHAR_HEIGHT;
    Pixel = (PUINT32)((PUCHAR)framebufInfo.BaseAddress +
            (Y + TOP_BOTTOM_LINES) * framebufInfo.Delta + X * sizeof(UINT32));

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

/**
 * @brief
 * Returns the width and height in pixels, of the whole visible area
 * of the graphics framebuffer.
 **/
VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    *Width  = framebufInfo.ScreenWidth;
    *Height = framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES;
    *Depth  = framebufInfo.BitsPerPixel;
}

/**
 * @brief
 * Returns the size in bytes, of a full graphics pixel buffer rectangle
 * that can fill the whole visible area of the graphics framebuffer.
 **/
ULONG
VidFbGetBufferSize(VOID)
{
    return ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) *
            framebufInfo.ScreenWidth * framebufInfo.BytesPerPixel);
}

VOID
VidFbScrollUp(
    _In_ UINT32 Color,
    _In_ ULONG Scroll)
{
    PUINT32 Dst = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + TOP_BOTTOM_LINES * framebufInfo.Delta);
    PUINT32 Src = (PUINT32)((PUCHAR)framebufInfo.BaseAddress + (TOP_BOTTOM_LINES + Scroll) * framebufInfo.Delta);
    ULONG PixelCount = framebufInfo.ScreenWidth * ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) - Scroll);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < framebufInfo.ScreenWidth * Scroll; PixelCount++)
        *Dst++ = Color;
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



/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Linear framebuffer based console support
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#define VGA_CHAR_SIZE 2

#define FBCONS_WIDTH    (framebufInfo.ScreenWidth / CHAR_WIDTH)
#define FBCONS_HEIGHT   ((framebufInfo.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT)

static inline
UINT32
FbConsAttrToSingleColor(
    _In_ UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

/**
 * @brief
 * Maps a text-mode CGA-style character attribute to separate
 * foreground and background ARGB colors.
 **/
static VOID
FbConsAttrToColors(
    _In_ UCHAR Attr,
    _Out_ PUINT32 FgColor,
    _Out_ PUINT32 BgColor)
{
    *FgColor = FbConsAttrToSingleColor(Attr & 0x0F);
    *BgColor = FbConsAttrToSingleColor((Attr >> 4) & 0x0F);
}

VOID
FbConsClearScreen(
    _In_ UCHAR Attr)
{
    UINT32 FgColor, BgColor;
    FbConsAttrToColors(Attr, &FgColor, &BgColor);
    VidFbClearScreenColor(BgColor, FALSE);
}

/**
 * @brief
 * Displays a character at a given position with specific foreground
 * and background colors.
 **/
VOID
FbConsOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG Column,
    _In_ ULONG Row,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor)
{
    /* Don't display outside of the screen */
    if ((Column >= FBCONS_WIDTH) || (Row >= FBCONS_HEIGHT))
        return;
    VidFbOutputChar(Char, Column * CHAR_WIDTH, Row * CHAR_HEIGHT, FgColor, BgColor);
}

/**
 * @brief
 * Displays a character with specific text attributes at a given position.
 **/
VOID
FbConsPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG Column,
    _In_ ULONG Row)
{
    UINT32 FgColor, BgColor;
    FbConsAttrToColors(Attr, &FgColor, &BgColor);
    FbConsOutputChar(Char, Column, Row, FgColor, BgColor);
}

/**
 * @brief
 * Returns the width and height in number of CGA characters/attributes, of a
 * full text-mode CGA-style character buffer rectangle that can fill the whole console.
 **/
VOID
FbConsGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth)
{
    // VidFbGetDisplaySize(Width, Height, Depth);
    // *Width  /= CHAR_WIDTH;
    // *Height /= CHAR_HEIGHT;
    *Width  = FBCONS_WIDTH;
    *Height = FBCONS_HEIGHT;
    *Depth  = framebufInfo.BitsPerPixel;
}

/**
 * @brief
 * Returns the size in bytes, of a full text-mode CGA-style
 * character buffer rectangle that can fill the whole console.
 **/
ULONG
FbConsGetBufferSize(VOID)
{
    return (FBCONS_HEIGHT * FBCONS_WIDTH * VGA_CHAR_SIZE);
}

/**
 * @brief
 * Copies a full text-mode CGA-style character buffer rectangle to the console.
 **/
// TODO: Write a VidFb "BitBlt" equivalent.
VOID
FbConsCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;
    ULONG Row, Col;

    // ULONG Width, Height, Depth;
    // FbConsGetDisplaySize(&Width, &Height, &Depth);
    ULONG Width = FBCONS_WIDTH, Height = FBCONS_HEIGHT;

    for (Row = 0; Row < Height; ++Row)
    {
        for (Col = 0; Col < Width; ++Col)
        {
            FbConsPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Row);
            OffScreenBuffer += VGA_CHAR_SIZE;
        }
    }
}

VOID
FbConsScrollUp(
    _In_ UCHAR Attr)
{
    UINT32 BgColor, Dummy;
    FbConsAttrToColors(Attr, &Dummy, &BgColor);
    VidFbScrollUp(BgColor, CHAR_HEIGHT);
}
