/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     UI Video helpers for special effects.
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 */

#ifndef _M_ARM
#include <freeldr.h>

#define RGB_MAX                 64
#define RGB_MAX_PER_ITERATION   64
#define TAG_PALETTE_COLORS      'claP'

static PVOID VideoOffScreenBuffer = NULL;

PVOID VideoAllocateOffScreenBuffer(VOID)
{
    ULONG BufferSize;

    VideoFreeOffScreenBuffer();

    BufferSize = MachVideoGetBufferSize();

    VideoOffScreenBuffer = MmAllocateMemoryWithType(BufferSize, LoaderFirmwareTemporary);

    return VideoOffScreenBuffer;
}

VOID VideoFreeOffScreenBuffer(VOID)
{
    if (!VideoOffScreenBuffer)
        return;

    MmFreeMemory(VideoOffScreenBuffer);
    VideoOffScreenBuffer = NULL;
}

VOID VideoCopyOffScreenBufferToVRAM(VOID)
{
    MachVideoCopyOffScreenBufferToVRAM(VideoOffScreenBuffer);
}


VOID VideoSavePaletteState(PPALETTE_ENTRY Palette, ULONG ColorCount)
{
    ULONG Color;

    for (Color = 0; Color < ColorCount; Color++)
    {
        MachVideoGetPaletteColor((UCHAR)Color, &Palette[Color].Red, &Palette[Color].Green, &Palette[Color].Blue);
    }
}

VOID VideoRestorePaletteState(PPALETTE_ENTRY Palette, ULONG ColorCount)
{
    ULONG Color;

    MachVideoSync();

    for (Color = 0; Color < ColorCount; Color++)
    {
        MachVideoSetPaletteColor((UCHAR)Color, Palette[Color].Red, Palette[Color].Green, Palette[Color].Blue);
    }
}


VOID VideoSetAllColorsToBlack(ULONG ColorCount)
{
    UCHAR Color;

    MachVideoSync();

    for (Color = 0; Color < ColorCount; Color++)
    {
        MachVideoSetPaletteColor(Color, 0, 0, 0);
    }
}

VOID VideoFadeIn(PPALETTE_ENTRY Palette, ULONG ColorCount)
{
    ULONG Index;
    UCHAR Color;
    PPALETTE_ENTRY PaletteColors;

    PaletteColors = FrLdrTempAlloc(sizeof(PALETTE_ENTRY) * ColorCount, TAG_PALETTE_COLORS);
    if (!PaletteColors) return;

    for (Index = 0; Index < RGB_MAX; Index++)
    {
        for (Color = 0; Color < ColorCount; Color++)
        {
            MachVideoGetPaletteColor(Color, &PaletteColors[Color].Red, &PaletteColors[Color].Green, &PaletteColors[Color].Blue);

            // Increment each color so it approaches its real value
            if (PaletteColors[Color].Red < Palette[Color].Red)
            {
                PaletteColors[Color].Red++;
            }
            if (PaletteColors[Color].Green < Palette[Color].Green)
            {
                PaletteColors[Color].Green++;
            }
            if (PaletteColors[Color].Blue < Palette[Color].Blue)
            {
                PaletteColors[Color].Blue++;
            }

            // Make sure we haven't exceeded the real value
            if (PaletteColors[Color].Red > Palette[Color].Red)
            {
                PaletteColors[Color].Red = Palette[Color].Red;
            }
            if (PaletteColors[Color].Green > Palette[Color].Green)
            {
                PaletteColors[Color].Green = Palette[Color].Green;
            }
            if (PaletteColors[Color].Blue > Palette[Color].Blue)
            {
                PaletteColors[Color].Blue = Palette[Color].Blue;
            }
        }

        // Set the colors
        for (Color = 0; Color < ColorCount; Color++)
        {
            if ((Color % RGB_MAX_PER_ITERATION) == 0)
            {
                MachVideoSync();
            }

            MachVideoSetPaletteColor(Color, PaletteColors[Color].Red, PaletteColors[Color].Green, PaletteColors[Color].Blue);
        }
    }

    FrLdrTempFree(PaletteColors, TAG_PALETTE_COLORS);
}

VOID VideoFadeOut(ULONG ColorCount)
{
    ULONG Index;
    UCHAR Color;
    UCHAR Red;
    UCHAR Green;
    UCHAR Blue;

    for (Index = 0; Index < RGB_MAX; Index++)
    {
        for (Color = 0; Color < ColorCount; Color++)
        {
            if ((Color % RGB_MAX_PER_ITERATION) == 0)
            {
                MachVideoSync();
            }

            MachVideoGetPaletteColor(Color, &Red, &Green, &Blue);

            if (Red > 0)
            {
                Red--;
            }
            if (Green > 0)
            {
                Green--;
            }
            if (Blue > 0)
            {
                Blue--;
            }

            MachVideoSetPaletteColor(Color, Red, Green, Blue);
        }
    }
}

#endif
