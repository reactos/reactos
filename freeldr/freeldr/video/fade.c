/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#include <freeldr.h>
#include <video.h>
#include <comm.h>


#define RGB_MAX						64
#define RGB_MAX_PER_ITERATION		64

VOID VideoSetAllColorsToBlack(U32 ColorCount)
{
	U32		Color;

	VideoWaitForVerticalRetrace();

	for (Color=0; Color<ColorCount; Color++)
	{
		VideoSetPaletteColor(Color, 0, 0, 0);
	}
}

VOID VideoFadeIn(PPALETTE_ENTRY Palette, U32 ColorCount)
{
	U32				Index;
	U32				Color;
	PALETTE_ENTRY	PaletteColors[ColorCount];

	for (Index=0; Index<RGB_MAX; Index++)
	{

		for (Color=0; Color<ColorCount; Color++)
		{
			VideoGetPaletteColor(Color, &PaletteColors[Color].Red, &PaletteColors[Color].Green, &PaletteColors[Color].Blue);

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
				VideoWaitForVerticalRetrace();
			}

			VideoSetPaletteColor(Color, PaletteColors[Color].Red, PaletteColors[Color].Green, PaletteColors[Color].Blue);
		}
	}
}

VOID VideoFadeOut(U32 ColorCount)
{
	U32		Index;
	U32		Color;
	U8		Red;
	U8		Green;
	U8		Blue;

	for (Index=0; Index<RGB_MAX; Index++)
	{
		for (Color=0; Color<ColorCount; Color++)
		{
			if ((Color % RGB_MAX_PER_ITERATION) == 0)
			{
				VideoWaitForVerticalRetrace();
			}

			VideoGetPaletteColor(Color, &Red, &Green, &Blue);

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

			VideoSetPaletteColor(Color, Red, Green, Blue);
		}
	}
}
