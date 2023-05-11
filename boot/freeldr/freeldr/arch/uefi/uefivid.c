/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WARNING);

#define CHAR_WIDTH  8
#define CHAR_HEIGHT 16
#define TOP_BOTTOM_LINES 0
#define LOWEST_SUPPORTED_RES 1

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
extern UCHAR BitmapFont8x16[256 * 16];

UCHAR MachDefaultTextColor = COLOR_GRAY;
REACTOS_INTERNAL_BGCONTEXT framebufferData;
EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/* FUNCTIONS ******************************************************************/

EFI_STATUS
UefiInitalizeVideo(VOID)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    RtlZeroMemory(&framebufferData, sizeof(framebufferData));
    Status = GlobalSystemTable->BootServices->LocateProtocol(&EfiGraphicsOutputProtocol, 0, (void**)&gop);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find GOP with status %d\n", Status);
        return Status;
    }

    /* We don't need high resolutions for freeldr */
    gop->SetMode(gop, LOWEST_SUPPORTED_RES);

    framebufferData.BaseAddress        = (ULONG_PTR)gop->Mode->FrameBufferBase;
    framebufferData.BufferSize         = gop->Mode->FrameBufferSize;
    framebufferData.ScreenWidth        = gop->Mode->Info->HorizontalResolution;
    framebufferData.ScreenHeight       = gop->Mode->Info->VerticalResolution;
    framebufferData.PixelsPerScanLine  = gop->Mode->Info->PixelsPerScanLine;
    framebufferData.PixelFormat        = gop->Mode->Info->PixelFormat;

    return Status;
}

VOID
UefiPrintFramebufferData(VOID)
{
    TRACE("Framebuffer BaseAddress       : %X\n", framebufferData.BaseAddress);
    TRACE("Framebuffer BufferSize        : %X\n", framebufferData.BufferSize);
    TRACE("Framebuffer ScreenWidth       : %d\n", framebufferData.ScreenWidth);
    TRACE("Framebuffer ScreenHeight      : %d\n", framebufferData.ScreenHeight);
    TRACE("Framebuffer PixelsPerScanLine : %d\n", framebufferData.PixelsPerScanLine);
    TRACE("Framebuffer PixelFormat       : %d\n", framebufferData.PixelFormat);
}

static ULONG
UefiVideoAttrToSingleColor(UCHAR Attr)
{
    UCHAR Intensity;
    Intensity = (0 == (Attr & 0x08) ? 127 : 255);

    return 0xff000000 |
           (0 == (Attr & 0x04) ? 0 : (Intensity << 16)) |
           (0 == (Attr & 0x02) ? 0 : (Intensity << 8)) |
           (0 == (Attr & 0x01) ? 0 : Intensity);
}

static VOID
UefiVideoAttrToColors(UCHAR Attr, ULONG *FgColor, ULONG *BgColor)
{
    *FgColor = UefiVideoAttrToSingleColor(Attr & 0xf);
    *BgColor = UefiVideoAttrToSingleColor((Attr >> 4) & 0xf);
}


static VOID
UefiVideoClearScreenColor(ULONG Color, BOOLEAN FullScreen)
{
    ULONG Delta;
    ULONG Line, Col;
    PULONG p;

    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    for (Line = 0; Line < framebufferData.ScreenHeight - (FullScreen ? 0 : 2 * TOP_BOTTOM_LINES); Line++)
    {
        p = (PULONG) ((char *) framebufferData.BaseAddress + (Line + (FullScreen ? 0 : TOP_BOTTOM_LINES)) * Delta);
        for (Col = 0; Col < framebufferData.ScreenWidth; Col++)
        {
            *p++ = Color;
        }
    }
}

VOID
UefiVideoClearScreen(UCHAR Attr)
{
    ULONG FgColor, BgColor;

    UefiVideoAttrToColors(Attr, &FgColor, &BgColor);
    UefiVideoClearScreenColor(BgColor, FALSE);
}

VOID
UefiVideoOutputChar(UCHAR Char, unsigned X, unsigned Y, ULONG FgColor, ULONG BgColor)
{
    PUCHAR FontPtr;
    PULONG Pixel;
    UCHAR Mask;
    unsigned Line;
    unsigned Col;
    ULONG Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    FontPtr = BitmapFont8x16 + Char * 16;
    Pixel = (PULONG) ((char *) framebufferData.BaseAddress +
            (Y * CHAR_HEIGHT + TOP_BOTTOM_LINES) *  Delta + X * CHAR_WIDTH * 4);

    for (Line = 0; Line < CHAR_HEIGHT; Line++)
    {
        Mask = 0x80;
        for (Col = 0; Col < CHAR_WIDTH; Col++)
        {
            Pixel[Col] = (0 != (FontPtr[Line] & Mask) ? FgColor : BgColor);
            Mask = Mask >> 1;
        }
        Pixel = (PULONG) ((char *) Pixel + Delta);
    }
}

VOID
UefiVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    ULONG FgColor = 0;
    ULONG BgColor = 0;
    if (Ch != 0)
    {
        UefiVideoAttrToColors(Attr, &FgColor, &BgColor);
        UefiVideoOutputChar(Ch, X, Y, FgColor, BgColor);
    }
}

VOID
UefiVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    *Width =  framebufferData.ScreenWidth / CHAR_WIDTH;
    *Height = (framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT;
    *Depth =  0;
}

VIDEODISPLAYMODE
UefiVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

ULONG
UefiVideoGetBufferSize(VOID)
{
    return ((framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT * (framebufferData.ScreenWidth / CHAR_WIDTH) * 2);
}

VOID
UefiVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    PUCHAR OffScreenBuffer = (PUCHAR)Buffer;

    ULONG Col, Line;
    for (Line = 0; Line < (framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT; Line++)
    {
        for (Col = 0; Col < framebufferData.ScreenWidth / CHAR_WIDTH; Col++)
        {
            UefiVideoPutChar(OffScreenBuffer[0], OffScreenBuffer[1], Col, Line);
            OffScreenBuffer += 2;
        }
    }
}

VOID
UefiVideoScrollUp(VOID)
{
    ULONG BgColor, Dummy;
    ULONG Delta;
    Delta = (framebufferData.PixelsPerScanLine * 4 + 3) & ~ 0x3;
    ULONG PixelCount = framebufferData.ScreenWidth * CHAR_HEIGHT *
                       (((framebufferData.ScreenHeight - 2 * TOP_BOTTOM_LINES) / CHAR_HEIGHT) - 1);
    PULONG Src = (PULONG)((PUCHAR)framebufferData.BaseAddress + (CHAR_HEIGHT + TOP_BOTTOM_LINES) * Delta);
    PULONG Dst = (PULONG)((PUCHAR)framebufferData.BaseAddress + TOP_BOTTOM_LINES * Delta);

    UefiVideoAttrToColors(ATTR(COLOR_WHITE, COLOR_BLACK), &Dummy, &BgColor);

    while (PixelCount--)
        *Dst++ = *Src++;

    for (PixelCount = 0; PixelCount < framebufferData.ScreenWidth * CHAR_HEIGHT; PixelCount++)
        *Dst++ = BgColor;
}

VOID
UefiVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    /* We don't have a cursor yet */
}

VOID
UefiVideoHideShowTextCursor(BOOLEAN Show)
{
    /* We don't have a cursor yet */
}

BOOLEAN
UefiVideoIsPaletteFixed(VOID)
{
    return 0;
}

VOID
UefiVideoSetPaletteColor(UCHAR Color, UCHAR Red,
                         UCHAR Green, UCHAR Blue)
{
    /* Not supported */
}

VOID
UefiVideoGetPaletteColor(UCHAR Color, UCHAR* Red,
                         UCHAR* Green, UCHAR* Blue)
{
    /* Not supported */
}
