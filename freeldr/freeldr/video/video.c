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


VOID VideoClearScreen(VOID)
{
	VideoSetMode(VideoGetCurrentMode());
}

VOID VideoWaitForVerticalRetrace(VOID)
{
	while ((READ_PORT_UCHAR((PUCHAR)VIDEOPORT_VERTICAL_RETRACE) & 0x08))
	{
		// Keep reading the port until bit 4 is clear
	}

	while (!(READ_PORT_UCHAR((PUCHAR)VIDEOPORT_VERTICAL_RETRACE) & 0x08))
	{
		// Keep reading the port until bit 4 is set
	}
}

VOID VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
	WRITE_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_WRITE, Color);
	WRITE_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA, Red);
	WRITE_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA, Green);
	WRITE_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA, Blue);
}

VOID VideoGetPaletteColor(UCHAR Color, PUCHAR Red, PUCHAR Green, PUCHAR Blue)
{
	WRITE_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_READ, Color);
	*Red = READ_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA);
	*Green = READ_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA);
	*Blue = READ_PORT_UCHAR((PUCHAR)VIDEOPORT_PALETTE_DATA);
}
