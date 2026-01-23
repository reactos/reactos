/*
 * PROJECT:     ReactOS Boot Video Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Definitions for framebuffer-specific DisplayController
 *              device boot-time configuration data stored in the
 *              \Registry\Machine\Hardware\Description ARC tree.
 * COPYRIGHT:   Copyright 2023-2025 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Framebuffer-specific device data.
 *
 * Supplemental data, extends CM_VIDEO_DEVICE_DATA.
 * Gets appended to the standard configuration resource list.
 * Any optional Irql/Vector interrupt settings are specified with
 * a CmResourceTypeInterrupt descriptor, while any other I/O port
 * is specified with a CmResourceTypePort descriptor.
 * The framebuffer base and size are specified by the first
 * CmResourceTypeMemory descriptor.
 **/
typedef struct _CM_FRAMEBUF_DEVICE_DATA
{
    CM_VIDEO_DEVICE_DATA;

    /* NOTE: FrameBufferSize == PixelsPerScanLine * ScreenHeight * BytesPerPixel */

    /* Horizontal and Vertical resolution in pixels */
    ULONG ScreenWidth;
    ULONG ScreenHeight;

    /* Number of pixel elements per video memory line. Related to the
     * number of bytes per scan-line ("Pitch", or "ScreenStride") via:
     * Pitch = PixelsPerScanLine * BytesPerPixel */
    ULONG PixelsPerScanLine;

    ULONG BitsPerPixel; // aka. "PixelStride" or "PixelDepth"

    /*
     * Physical format of the pixel for BPP > 8, specified by bit-mask.
     * A bit being set defines those used for the given color component,
     * such as Red, Green, Blue, or Reserved.
     */
    struct /*_PIXEL_BITMASK*/
    {
        ULONG RedMask;
        ULONG GreenMask;
        ULONG BlueMask;
        ULONG ReservedMask;
    } PixelMasks; /*PIXEL_BITMASK, *PPIXEL_BITMASK*/

} CM_FRAMEBUF_DEVICE_DATA, *PCM_FRAMEBUF_DEVICE_DATA;


/* UEFI support, see efi/GraphicsOutput.h */
#ifdef EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID // __GRAPHICS_OUTPUT_H__
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
#endif

#ifdef __cplusplus
}
#endif

/* EOF */
