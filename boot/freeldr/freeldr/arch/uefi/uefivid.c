/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video output
 * COPYRIGHT:   Copyright 2022 Justin Miller <justinmiller100@gmail.com>
 */

#include <uefildr.h>
#include "../vidfb.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(UI);

/* GLOBALS ********************************************************************/

extern EFI_SYSTEM_TABLE* GlobalSystemTable;
extern EFI_HANDLE GlobalImageHandle;
EFI_GUID EfiGraphicsOutputProtocol = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

#define LOWEST_SUPPORTED_RES 1

/* FUNCTIONS ******************************************************************/

EFI_STATUS
UefiInitializeVideo(VOID)
{
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    ULONG BitsPerPixel;

    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    Status = GlobalSystemTable->BootServices->LocateProtocol(&EfiGraphicsOutputProtocol, 0, (void**)&gop);
    if (Status != EFI_SUCCESS)
    {
        TRACE("Failed to find GOP with status %d\n", Status);
        return Status;
    }

    /* We don't need high resolutions for freeldr */
    gop->SetMode(gop, LOWEST_SUPPORTED_RES);

    /* Physical format of the pixel */
    PixelFormat = gop->Mode->Info->PixelFormat;
    switch (PixelFormat)
    {
        case PixelRedGreenBlueReserved8BitPerColor:
        case PixelBlueGreenRedReserved8BitPerColor:
        {
            BitsPerPixel = RTL_BITS_OF(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
            break;
        }

        case PixelBitMask:
        case PixelBltOnly:
        default:
        {
            ERR("Unsupported UFEI GOP format %lu\n", PixelFormat);
            BitsPerPixel = 0;
            break;
        }
    }

    VidFbInitializeVideo((ULONG_PTR)gop->Mode->FrameBufferBase,
                         gop->Mode->FrameBufferSize,
                         gop->Mode->Info->HorizontalResolution,
                         gop->Mode->Info->VerticalResolution,
                         gop->Mode->Info->PixelsPerScanLine,
                         BitsPerPixel);
    return Status;
}

VOID
UefiVideoClearScreen(UCHAR Attr)
{
    FbConsClearScreen(Attr);
}

VOID
UefiVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
    FbConsPutChar(Ch, Attr, X, Y);
}

VOID
UefiVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
    FbConsGetDisplaySize(Width, Height, Depth);
}

VIDEODISPLAYMODE
UefiVideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init)
{
    /* We only have one mode, semi-text */
    return VideoTextMode;
}

ULONG
UefiVideoGetBufferSize(VOID)
{
    return FbConsGetBufferSize();
}

VOID
UefiVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
    FbConsCopyOffScreenBufferToVRAM(Buffer);
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
