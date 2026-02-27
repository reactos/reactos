/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Video support for linear framebuffers
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#include "twidbits.h"

/**
 * @brief
 * Physical format of an RGB pixel, specified with per-component bit-masks.
 * A bit being set defines those used for the given color component, such
 * as Red, Green, Blue, or Reserved.
 *
 * @note
 * Supports up to 32 bits-per-pixel deep pixels.
 **/
typedef struct _PIXEL_BITMASK
{
    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;
    ULONG ReservedMask;
} PIXEL_BITMASK, *PPIXEL_BITMASK;


/**
 * @brief
 * Calculates the number of bits per pixel ("PixelDepth") for
 * the given pixel format, given by the pixel color masks.
 *
 * @remark
 * See UEFI Spec Rev.2.10 Section 12.9 "Graphics Output Protocol":
 * example code "GetPixelElementSize()" function.
 **/
FORCEINLINE
ULONG
PixelBitmasksToBpp(
    _In_ ULONG RedMask,
    _In_ ULONG GreenMask,
    _In_ ULONG BlueMask,
    _In_ ULONG ReservedMask)
{
    ULONG CompoundMask = (RedMask | GreenMask | BlueMask | ReservedMask);
    /* Alternatively, the calculation could be done by finding the highest
     * bit set in the combined pixel color masks, if they are packed together. */
    return CountNumberOfBits(CompoundMask); // FindHighestSetBit(CompoundMask);
}

#include <drivers/bootvid/framebuf.h>
BOOLEAN
VidFbInitializeVideo(
    _Out_opt_ PCM_FRAMEBUF_DEVICE_DATA* pFbData,
    _In_ ULONG_PTR BaseAddress,
    _In_ ULONG BufferSize,
    _In_ UINT32 ScreenWidth,
    _In_ UINT32 ScreenHeight,
    _In_ UINT32 PixelsPerScanLine,
    _In_ UINT32 BitsPerPixel,
    _In_opt_ PPIXEL_BITMASK PixelMasks);

VOID
VidFbClearScreenColor(
    _In_ UINT32 Color,
    _In_ BOOLEAN FullScreen);

VOID
VidFbOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG X,
    _In_ ULONG Y,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor);

VOID
VidFbGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth);

ULONG
VidFbGetBufferSize(VOID);

VOID
VidFbScrollUp(
    _In_ UINT32 Color,
    _In_ ULONG Scroll);

#if 0
VOID
VidFbSetTextCursorPosition(UCHAR X, UCHAR Y);

VOID
VidFbHideShowTextCursor(BOOLEAN Show);

BOOLEAN
VidFbIsPaletteFixed(VOID);

VOID
VidFbSetPaletteColor(
    _In_ UCHAR Color,
    _In_ UCHAR Red, _In_ UCHAR Green, _In_ UCHAR Blue);

VOID
VidFbGetPaletteColor(
    _In_ UCHAR Color,
    _Out_ PUCHAR Red, _Out_ PUCHAR Green, _Out_ PUCHAR Blue);
#endif



/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Linear framebuffer based console support
 * COPYRIGHT:   Copyright 2025-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

VOID
FbConsClearScreen(
    _In_ UCHAR Attr);

VOID
FbConsOutputChar(
    _In_ UCHAR Char,
    _In_ ULONG Column,
    _In_ ULONG Row,
    _In_ UINT32 FgColor,
    _In_ UINT32 BgColor);

VOID
FbConsPutChar(
    _In_ UCHAR Char,
    _In_ UCHAR Attr,
    _In_ ULONG Column,
    _In_ ULONG Row);

VOID
FbConsGetDisplaySize(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ PULONG Depth);

ULONG
FbConsGetBufferSize(VOID);

VOID
FbConsCopyOffScreenBufferToVRAM(
    _In_ PVOID Buffer);

VOID
FbConsScrollUp(
    _In_ UCHAR Attr);
