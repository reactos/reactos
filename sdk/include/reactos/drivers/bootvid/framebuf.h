/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Definitions for framebuffer-specific DisplayController
 *              device boot-time configuration data stored in the
 *              \Registry\Machine\Hardware\Description ARC tree.
 * COPYRIGHT:   Copyright 2023-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   ReactOS Framebuffer-specific video device configuration data.
 *
 * Supplemental data that extends CM_VIDEO_DEVICE_DATA.
 * It is appended to the standard configuration resource list,
 * as a CmResourceTypeDeviceSpecific data descriptor.
 *
 * - Any optional Irql/Vector interrupt settings are specified with
 *   a CmResourceTypeInterrupt descriptor, while any other I/O port
 *   is specified with a CmResourceTypePort descriptor.
 *
 * - The video RAM physical address base and size are specified by
 *   the first CmResourceTypeMemory descriptor in the list.
 *
 * - The framebuffer is the actively displayed part of video RAM,
 *   and is specified via the FrameBufferOffset member.
 **/
typedef struct _CM_FRAMEBUF_DEVICE_DATA
{
    CM_VIDEO_DEVICE_DATA;

    /* Absolute offset from the start of the video RAM of the framebuffer
     * to be displayed on the monitor. The framebuffer size is obtained by:
     * FrameBufferSize = ScreenHeight * PixelsPerScanLine * BytesPerPixel */
    ULONG FrameBufferOffset;

    /* Horizontal and Vertical resolution in pixels */
    ULONG ScreenWidth;
    ULONG ScreenHeight;

    /* Number of pixel elements per video memory line. Related to
     * the number of bytes per scan-line (pitch/screen stride) via:
     * Pitch = PixelsPerScanLine * BytesPerPixel */
    ULONG PixelsPerScanLine; ///< Pitch/stride in pixels
    ULONG BitsPerPixel;      ///< Pixel depth

    // MEMMODEL MemoryModel; // Linear, banked, ...

    /*
     * Physical format of the pixel for BPP > 8, specified by bit-mask.
     * A bit being set defines those used for the given color component,
     * such as Red, Green, Blue, or Reserved.
     */
    // UCHAR NumberRedBits;
    // UCHAR NumberGreenBits;
    // UCHAR NumberBlueBits;
    // UCHAR NumberReservedBits;
    struct
    {
        ULONG RedMask;
        ULONG GreenMask;
        ULONG BlueMask;
        ULONG ReservedMask;
    } PixelMasks;

} CM_FRAMEBUF_DEVICE_DATA, *PCM_FRAMEBUF_DEVICE_DATA;


/* EFI 1.x */
#ifdef EFI_UGA_DRAW_PROTOCOL_GUID

/* NOTE: EFI UGA does not support any other format than 32-bit xRGB, and
 * no direct access to the underlying hardware framebuffer is offered */
// C_ASSERT(sizeof(EFI_UGA_PIXEL) == sizeof(ULONG));

#endif /* EFI */

/* UEFI support, see efi/GraphicsOutput.h */
#ifdef EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID // __GRAPHICS_OUTPUT_H__

C_ASSERT(RTL_FIELD_SIZE(CM_FRAMEBUF_DEVICE_DATA, PixelInformation) == sizeof(EFI_PIXEL_BITMASK));
// C_ASSERT(sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) == sizeof(ULONG));

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

// TODO: this version of the struct is temporary
// REACTOS_INTERNAL_BGCONTEXT
typedef struct _ROSEFI_FRAMEBUFFER_DATA
{
    ULONG_PTR BaseAddress;
    ULONG  BufferSize;
    UINT32 ScreenWidth;
    UINT32 ScreenHeight;
    UINT32 PixelsPerScanLine;
    UINT32 PixelFormat;
} ROSEFI_FRAMEBUFFER_DATA, *PROSEFI_FRAMEBUFFER_DATA;

#endif /* UEFI */


/**
 * @brief
 * Calculates the number of bits per pixel ("PixelDepth") for
 * the given pixel format, given by the pixel color masks.
 *
 * @note
 * The calculation is done by finding the highest bit set in
 * the combined pixel color masks.
 *
 * @remark
 * See UEFI Spec Rev.2.10 Section 12.9 "Graphics Output Protocol":
 * example code "GetPixelElementSize()" function.
 **/
FORCEINLINE
ULONG
PixelBitmasksToBpp(
    ULONG RedMask,
    ULONG GreenMask,
    ULONG BlueMask,
    ULONG ReservedMask)
{
    ULONG CompoundMask = (RedMask | GreenMask | BlueMask | ReservedMask);
#if 1
    ULONG ret = 0;
    return (_BitScanReverse(&ret, CompoundMask) ? ret : 0);
#else
    ULONG ret = 32; // (8 * sizeof(ULONG));
    while ((CompoundMask & (1 << 31)) == 0)
    {
        ret--;
        CompoundMask <<= 1;
    }
    return ret;
#endif
}

#ifdef __cplusplus
}
#endif

/* EOF */
