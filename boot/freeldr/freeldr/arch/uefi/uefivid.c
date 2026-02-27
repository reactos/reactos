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

ULONG_PTR VramAddress;
ULONG VramSize;
PCM_FRAMEBUF_DEVICE_DATA FrameBufferData = NULL;

#define LOWEST_SUPPORTED_RES 1

/* FUNCTIONS ******************************************************************/

/* EFI 1.x */
#ifdef EFI_UGA_DRAW_PROTOCOL_GUID

/* NOTE: EFI UGA does not support any other format than 32-bit xRGB, and
 * no direct access to the underlying hardware framebuffer is offered */
C_ASSERT(sizeof(EFI_UGA_PIXEL) == sizeof(ULONG));

#endif /* EFI */

/* UEFI support, see efi/GraphicsOutput.h */
#ifdef EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID // __GRAPHICS_OUTPUT_H__

C_ASSERT(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) == sizeof(ULONG));

/**
 * @brief   Maps UEFI GOP pixel format to pixel masks.
 * @see     EFI_PIXEL_BITMASK
 **/
static EFI_PIXEL_BITMASK EfiPixelMasks[] =
{ /* Red,        Green,      Blue,       Reserved */
    {0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000},   // PixelRedGreenBlueReserved8BitPerColor
    {0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000},   // PixelBlueGreenRedReserved8BitPerColor
    {0,          0,          0,          0}             // PixelBitMask, PixelBltOnly, ...
};

#endif /* UEFI */

EFI_STATUS
UefiInitializeVideo(VOID)
{
    EFI_STATUS Status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;

    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK* pPixelBitmask;
    ULONG BitsPerPixel;

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
            pPixelBitmask = &EfiPixelMasks[PixelFormat];
            BitsPerPixel = RTL_BITS_OF(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
            break;
        }

        case PixelBitMask:
        {
            /*
             * When the GOP pixel format is given by PixelBitMask, the pixel
             * element size _may be_ different from 4 bytes.
             * See UEFI Spec Rev.2.10 Section 12.9 "Graphics Output Protocol":
             * example code "GetPixelElementSize()" function.
             */
            pPixelBitmask = &gop->Mode->Info->PixelInformation;
            BitsPerPixel =
                PixelBitmasksToBpp(pPixelBitmask->RedMask,
                                   pPixelBitmask->GreenMask,
                                   pPixelBitmask->BlueMask,
                                   pPixelBitmask->ReservedMask);
            break;
        }

        case PixelBltOnly:
        default:
        {
            ERR("Unsupported UEFI GOP format %lu\n", PixelFormat);
            pPixelBitmask = NULL;
            BitsPerPixel = 0;
            break;
        }
    }

    VramAddress = (ULONG_PTR)gop->Mode->FrameBufferBase;
    VramSize = gop->Mode->FrameBufferSize;
    if (!VidFbInitializeVideo(&FrameBufferData,
                              VramAddress,
                              VramSize,
                              gop->Mode->Info->HorizontalResolution,
                              gop->Mode->Info->VerticalResolution,
                              gop->Mode->Info->PixelsPerScanLine,
                              BitsPerPixel,
                              (PPIXEL_BITMASK)pPixelBitmask))
    {
        ERR("Couldn't initialize video framebuffer\n");
        Status = EFI_UNSUPPORTED;
    }
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
