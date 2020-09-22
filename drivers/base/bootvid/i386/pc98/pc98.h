/*
 * PROJECT:     ReactOS Boot Video Driver for NEC PC-98 series
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Arch-specific header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/* INCLUDES *******************************************************************/

#include <drivers/pc98/video.h>

/* GLOBALS ********************************************************************/

#define BYTES_PER_SCANLINE (SCREEN_WIDTH / 8)
#define FB_OFFSET(x, y)    ((y) * SCREEN_WIDTH + (x))

extern ULONG_PTR FrameBuffer;

/* PROTOTYPES *****************************************************************/

VOID
NTAPI
DisplayCharacter(
    _In_ CHAR Character,
    _In_ ULONG Left,
    _In_ ULONG Top,
    _In_ ULONG TextColor,
    _In_ ULONG BackColor);

VOID
NTAPI
DoScroll(
    _In_ ULONG Scroll);

VOID
NTAPI
InitPaletteWithTable(
    _In_ PULONG Table,
    _In_ ULONG Count);

VOID
NTAPI
PreserveRow(
    _In_ ULONG CurrentTop,
    _In_ ULONG TopDelta,
    _In_ BOOLEAN Restore);

VOID
PrepareForSetPixel(VOID);

/* FUNCTIONS ******************************************************************/

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
