/*
 * PROJECT:     ReactOS Boot Video Driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch-specific header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

/* GLOBALS ********************************************************************/

#define FB_OFFSET(x, y)    ((y) * SCREEN_WIDTH + (x))

extern ULONG_PTR FrameBuffer;

/* FUNCTIONS ******************************************************************/

VOID
InitPaletteWithTable(
    _In_reads_(Count) const ULONG* Table,
    _In_ ULONG Count);

#define PrepareForSetPixel()

FORCEINLINE
VOID
SetPixel(
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ UCHAR Color)
{
    PUCHAR PixelPosition = (PUCHAR)(FrameBuffer + FB_OFFSET(Left, Top));

    WRITE_REGISTER_UCHAR(PixelPosition, Color);
}
