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

ULONG	CurrentVideoMode	= VIDEOMODE_NORMAL_TEXT;
ULONG	VideoResolutionX	= 80;
ULONG	VideoResolutionY	= 25;

BOOL VideoSetMode(ULONG VideoMode)
{
	switch (VideoMode)
	{
	case VIDEOMODE_NORMAL_TEXT:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x25();
	case VIDEOMODE_EXTENDED_TEXT:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x50_80x43();
	case VIDEOMODE_80X28:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x28();
	case VIDEOMODE_80X30:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x30();
	case VIDEOMODE_80X34:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x34();
	case VIDEOMODE_80X43:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x43();
	case VIDEOMODE_80X60:
		CurrentVideoMode = VideoMode;
		return VideoSetMode80x60();
	default:
		return FALSE;
	}

	return TRUE;
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
	BiosSet350ScanLines();
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

ULONG VideoGetCurrentModeResolutionX(VOID)
{
	return VideoResolutionX;
}

ULONG VideoGetCurrentModeResolutionY(VOID)
{
	return VideoResolutionY;
}

ULONG VideoGetCurrentMode(VOID)
{
	return CurrentVideoMode;
}
