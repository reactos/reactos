/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     UI Video helpers for special effects.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 */

#pragma once

#include <pshpack1.h>
typedef struct _PALETTE_ENTRY
{
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;
} PALETTE_ENTRY, *PPALETTE_ENTRY;
#include <poppack.h>

// extern PVOID VideoOffScreenBuffer;

PVOID VideoAllocateOffScreenBuffer(VOID);   // Returns a pointer to an off-screen buffer sufficient for the current video mode

VOID VideoFreeOffScreenBuffer(VOID);
VOID VideoCopyOffScreenBufferToVRAM(VOID);

VOID VideoSavePaletteState(PPALETTE_ENTRY Palette, ULONG ColorCount);
VOID VideoRestorePaletteState(PPALETTE_ENTRY Palette, ULONG ColorCount);

VOID VideoSetAllColorsToBlack(ULONG ColorCount);
VOID VideoFadeIn(PPALETTE_ENTRY Palette, ULONG ColorCount);
VOID VideoFadeOut(ULONG ColorCount);
