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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
	
#include <freeldr.h>
#include <video.h>
#include <debug.h>
#include <portio.h>

U32						CurrentVideoMode		= VIDEOMODE_NORMAL_TEXT;
U32						VideoResolutionX		= 80;
U32						VideoResolutionY		= 25;
U32						VideoBytesPerScanLine	= 0;	// Number of bytes per scan line
U32						CurrentModeType			= MODETYPE_TEXT;
BOOL					VesaVideoMode			= FALSE;

SVGA_MODE_INFORMATION	VesaVideoModeInformation;

BOOL VideoSetMode(U32 VideoMode)
{
	CurrentMemoryBank = 0;

	// Set the values for the default text modes
	// If they are setting a graphics mode then
	// these values will be changed.
	CurrentVideoMode = VideoMode;
	VideoResolutionX = 80;
	VideoResolutionY = 25;
	VideoBytesPerScanLine = 160;
	CurrentModeType = MODETYPE_TEXT;
	VesaVideoMode = FALSE;

	switch (VideoMode)
	{
	case VIDEOMODE_NORMAL_TEXT:
	case 0x03: /* BIOS 80x25 text mode number */
		return VideoSetMode80x25();
	case VIDEOMODE_EXTENDED_TEXT:
		return VideoSetMode80x50_80x43();
	case VIDEOMODE_80X28:
		return VideoSetMode80x28();
	case VIDEOMODE_80X30:
		return VideoSetMode80x30();
	case VIDEOMODE_80X34:
		return VideoSetMode80x34();
	case VIDEOMODE_80X43:
		return VideoSetMode80x43();
	case VIDEOMODE_80X60:
		return VideoSetMode80x60();
	}

	if (VideoMode == 0x12)
	{
		// 640x480x16
		BiosSetVideoMode(VideoMode);
		//WRITE_PORT_USHORT((U16*)0x03CE, 0x0205); // For some reason this is necessary?
		WRITE_PORT_USHORT((U16*)0x03CE, 0x0F01); // For some reason this is necessary?
		VideoResolutionX = 640;
		VideoResolutionY = 480;
		VideoBytesPerScanLine = 80;
		CurrentVideoMode = 0x12;
		CurrentModeType = MODETYPE_GRAPHICS;

		return TRUE;
	}
	else if (VideoMode == 0x13)
	{
		// 320x200x256
		BiosSetVideoMode(VideoMode);
		VideoResolutionX = 320;
		VideoResolutionY = 200;
		VideoBytesPerScanLine = 320;
		CurrentVideoMode = 0x13;
		CurrentModeType = MODETYPE_GRAPHICS;

		return TRUE;
	}
	else if (VideoMode >= 0x0108 && VideoMode <= 0x010C)
	{
		// VESA Text Mode
		if (!BiosVesaGetSVGAModeInformation(VideoMode, &VesaVideoModeInformation))
		{
			return FALSE;
		}

		if (!BiosVesaSetVideoMode(VideoMode))
		{
			return FALSE;
		}

		VideoResolutionX = VesaVideoModeInformation.WidthInPixels;
		VideoResolutionY = VesaVideoModeInformation.HeightInPixels;
		VideoBytesPerScanLine = VesaVideoModeInformation.BytesPerScanLine;
		CurrentVideoMode = VideoMode;
		CurrentModeType = MODETYPE_TEXT;
		VesaVideoMode = TRUE;

		return TRUE;
	}
	else
	{
		// VESA Graphics Mode
		if (!BiosVesaGetSVGAModeInformation(VideoMode, &VesaVideoModeInformation))
		{
			return FALSE;
		}

		if (!BiosVesaSetVideoMode(VideoMode))
		{
			return FALSE;
		}

		VideoResolutionX = VesaVideoModeInformation.WidthInPixels;
		VideoResolutionY = VesaVideoModeInformation.HeightInPixels;
		VideoBytesPerScanLine = VesaVideoModeInformation.BytesPerScanLine;
		CurrentVideoMode = VideoMode;
		CurrentModeType = MODETYPE_GRAPHICS;
		VesaVideoMode = TRUE;

		return TRUE;
	}

	return FALSE;
}

BOOL VideoSetMode80x25(VOID)
{
	BiosSetVideoMode(0x03);
	VideoResolutionX = 80;
	VideoResolutionY = 25;

	return TRUE;
}

BOOL VideoSetMode80x50_80x43(VOID)
{
	if (BiosDetectVideoCard() == VIDEOCARD_VGA)
	{
		BiosSetVideoMode(0x03);
		BiosSetVideoFont8x8();
		BiosSelectAlternatePrintScreen();
		BiosDisableCursorEmulation();
		BiosDefineCursor(6, 7);
		VideoResolutionX = 80;
		VideoResolutionY = 50;
	}
	else if (BiosDetectVideoCard() == VIDEOCARD_EGA)
	{
		BiosSetVideoMode(0x03);
		BiosSetVideoFont8x8();
		BiosSelectAlternatePrintScreen();
		BiosDisableCursorEmulation();
		BiosDefineCursor(6, 7);
		VideoResolutionX = 80;
		VideoResolutionY = 43;
	}
	else // VIDEOCARD_CGA_OR_OTHER
	{
		return FALSE;
	}

	return TRUE;
}

BOOL VideoSetMode80x28(VOID)
{
	// FIXME: Is this VGA-only?
	VideoSetMode80x25();
	BiosSetVideoFont8x14();
	BiosDefineCursor(11, 12);
	VideoResolutionX = 80;
	VideoResolutionY = 28;

	return TRUE;
}

BOOL VideoSetMode80x43(VOID)
{
	// FIXME: Is this VGA-only?
	BiosSetVerticalResolution(VERTRES_350_SCANLINES);
	VideoSetMode80x25();
	BiosSetVideoFont8x8();
	BiosSelectAlternatePrintScreen();
	BiosDisableCursorEmulation();
	BiosDefineCursor(6, 7);
	VideoResolutionX = 80;
	VideoResolutionY = 43;

	return TRUE;
}

BOOL VideoSetMode80x30(VOID)
{
	// FIXME: Is this VGA-only?
	VideoSetMode80x25();
	BiosSet480ScanLines();
	VideoResolutionX = 80;
	VideoResolutionY = 30;

	return TRUE;
}

BOOL VideoSetMode80x34(VOID)
{
	// FIXME: Is this VGA-only?
	VideoSetMode80x25();
	BiosSet480ScanLines();
	BiosSetVideoFont8x14();
	BiosDefineCursor(11, 12);
	BiosSetVideoDisplayEnd();
	VideoResolutionX = 80;
	VideoResolutionY = 34;

	return TRUE;
}

BOOL VideoSetMode80x60(VOID)
{
	// FIXME: Is this VGA-only?
	VideoSetMode80x25();
	BiosSet480ScanLines();
	BiosSetVideoFont8x8();
	BiosSelectAlternatePrintScreen();
	BiosDisableCursorEmulation();
	BiosDefineCursor(6, 7);
	BiosSetVideoDisplayEnd();
	VideoResolutionX = 80;
	VideoResolutionY = 60;

	return TRUE;
}

U32 VideoGetCurrentModeResolutionX(VOID)
{
	return VideoResolutionX;
}

U32 VideoGetCurrentModeResolutionY(VOID)
{
	return VideoResolutionY;
}

U32 VideoGetBytesPerScanLine(VOID)
{
	return VideoBytesPerScanLine;
}

U32 VideoGetCurrentMode(VOID)
{
	return CurrentVideoMode;
}

U32 VideoGetCurrentModeType(VOID)
{
	return CurrentModeType;
}

BOOL VideoIsCurrentModeVesa(VOID)
{
	return VesaVideoMode;
}

U32 VideoGetCurrentModeColorDepth(VOID)
{
	U32	ColorDepth = 2;
	U32	i;

	if (CurrentModeType == MODETYPE_TEXT)
	{
		return 0;
	}
	else // CurrentModeType == MODETYPE_GRAPHICS
	{
		// Calculate 2^BitsPerPixel'th power
		for (i=0; i<(VesaVideoModeInformation.BitsPerPixel-1); i++)
		{
			ColorDepth *= 2;
		}

		// This algorithm works great for 24-bit & 8-bit color modes
		// but 15-bit color modes really use 16 bits per pixel.
		// So check to see if green has an extra bit, if so then
		// it is 16-bit, if not it is 15-bit
		if (ColorDepth == 65536)
		{
			if (VesaVideoModeInformation.GreenMaskSize == 6)
			{
				return ColorDepth;
			}
			else
			{
				return 32768;
			}
		}

		return ColorDepth;
	}
}
