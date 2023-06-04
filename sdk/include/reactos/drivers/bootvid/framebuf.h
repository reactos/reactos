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

    /* Pixel physical format for BPP > 8 */
    struct
    {
        ULONG RedMask;
        ULONG GreenMask;
        ULONG BlueMask;
        ULONG ReservedMask;
    } PixelMasks;

} CM_FRAMEBUF_DEVICE_DATA, *PCM_FRAMEBUF_DEVICE_DATA;

#ifdef __cplusplus
}
#endif

/* EOF */
