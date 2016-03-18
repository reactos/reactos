/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _M_ARM
#include <freeldr.h>

#define RGB_MAX                      64
#define RGB_MAX_PER_ITERATION        64
#define TAG_PALETTE_COLORS           'claP'

VOID VideoSetAllColorsToBlack(ULONG ColorCount)
{
    UCHAR        Color;

    MachVideoSync();

    for (Color=0; Color<ColorCount; Color++)
    {
        MachVideoSetPaletteColor(Color, 0, 0, 0);
    }
}

VOID VideoFadeIn(PPALETTE_ENTRY Palette, ULONG ColorCount)
{
    ULONG                Index;
    UCHAR                Color;
    PPALETTE_ENTRY    PaletteColors;

    PaletteColors = FrLdrTempAlloc(sizeof(PALETTE_ENTRY) * ColorCount, TAG_PALETTE_COLORS);
    if (!PaletteColors) return;

    for (Index=0; Index<RGB_MAX; Index++)
    {

        for (Color=0; Color<ColorCount; Color++)
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
        for (Color=0; Color<ColorCount; Color++)
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
    ULONG        Index;
    UCHAR        Color;
    UCHAR        Red;
    UCHAR        Green;
    UCHAR        Blue;

    for (Index=0; Index<RGB_MAX; Index++)
    {
        for (Color=0; Color<ColorCount; Color++)
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
