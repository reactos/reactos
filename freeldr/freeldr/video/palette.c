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


VOID VideoSetPaletteColor(U8 Color, U8 Red, U8 Green, U8 Blue)
{
	WRITE_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_WRITE, Color);
	WRITE_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA, Red);
	WRITE_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA, Green);
	WRITE_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA, Blue);
}

VOID VideoGetPaletteColor(U8 Color, U8* Red, U8* Green, U8* Blue)
{
	WRITE_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_READ, Color);
	*Red = READ_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA);
	*Green = READ_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA);
	*Blue = READ_PORT_UCHAR((U8*)VIDEOPORT_PALETTE_DATA);
}

VOID VideoSavePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount)
{
	U32		Color;

	for (Color=0; Color<ColorCount; Color++)
	{
		VideoGetPaletteColor(Color, &Palette[Color].Red, &Palette[Color].Green, &Palette[Color].Blue);
	}
}

VOID VideoRestorePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount)
{
	U32		Color;

	VideoWaitForVerticalRetrace();

	for (Color=0; Color<ColorCount; Color++)
	{
		VideoSetPaletteColor(Color, Palette[Color].Red, Palette[Color].Green, Palette[Color].Blue);
	}
}
