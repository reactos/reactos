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

#ifndef __VIDEO_H
#define __VIDEO_H


#define	VIDEOCARD_CGA_OR_OTHER		0
#define VIDEOCARD_EGA				1
#define VIDEOCARD_VGA				2

#define VIDEOMODE_NORMAL_TEXT		0
#define VIDEOMODE_EXTENDED_TEXT		1
#define	VIDEOMODE_80X28				0x501C
#define	VIDEOMODE_80X30				0x501E
#define	VIDEOMODE_80X34				0x5022
#define	VIDEOMODE_80X43				0x502B
#define	VIDEOMODE_80X60				0x503C

#define VIDEOPORT_PALETTE_READ		0x03C7
#define VIDEOPORT_PALETTE_WRITE		0x03C8
#define VIDEOPORT_PALETTE_DATA		0x03C9
#define VIDEOPORT_VERTICAL_RETRACE	0x03DA

VOID	BiosSetVideoMode(ULONG VideoMode);				// Implemented in biosvid.S
VOID	BiosSetVideoFont8x8(VOID);						// Implemented in biosvid.S
VOID	BiosSetVideoFont8x14(VOID);						// Implemented in biosvid.S
VOID	BiosSetVideoFont8x16(VOID);						// Implemented in biosvid.S
VOID	BiosSelectAlternatePrintScreen(VOID);			// Implemented in biosvid.S
VOID	BiosDisableCursorEmulation(VOID);				// Implemented in biosvid.S
VOID	BiosDefineCursor(ULONG StartScanLine, ULONG EndScanLine);	// Implemented in biosvid.S
ULONG	BiosDetectVideoCard(VOID);						// Implemented in biosvid.S
VOID	BiosSet200ScanLines(VOID);						// Implemented in biosvid.S, must be called right before BiosSetVideoMode()
VOID	BiosSet350ScanLines(VOID);						// Implemented in biosvid.S, must be called right before BiosSetVideoMode()
VOID	BiosSet400ScanLines(VOID);						// Implemented in biosvid.S, must be called right before BiosSetVideoMode()
VOID	BiosSet480ScanLines(VOID);						// Implemented in biosvid.S, must be called right after BiosSetVideoMode()
VOID	BiosSetVideoDisplayEnd(VOID);					// Implemented in biosvid.S

VOID	VideoSetTextCursorPosition(ULONG X, ULONG Y);	// Implemented in biosvid.S
VOID	VideoHideTextCursor(VOID);						// Implemented in biosvid.S
VOID	VideoShowTextCursor(VOID);						// Implemented in biosvid.S
ULONG	VideoGetTextCursorPositionX(VOID);				// Implemented in biosvid.S
ULONG	VideoGetTextCursorPositionY(VOID);				// Implemented in biosvid.S

BOOL	VideoSetMode(ULONG VideoMode);
BOOL	VideoSetMode80x25(VOID);						// Sets 80x25
BOOL	VideoSetMode80x50_80x43(VOID);					// Sets 80x50 (VGA) or 80x43 (EGA) 8-pixel mode
BOOL	VideoSetMode80x28(VOID);						// Sets 80x28. Works on all VGA's. Standard 80x25 with 14-point font
BOOL	VideoSetMode80x43(VOID);						// Sets 80x43. Works on all VGA's. It's a 350-scanline mode with 8-pixel font.
BOOL	VideoSetMode80x30(VOID);						// Sets 80x30. Works on all VGA's. 480 scanlines, 16-pixel font.
BOOL	VideoSetMode80x34(VOID);						// Sets 80x34. Works on all VGA's. 480 scanlines, 14-pixel font.
BOOL	VideoSetMode80x60(VOID);						// Sets 80x60. Works on all VGA's. 480 scanlines, 8-pixel font.
ULONG	VideoGetCurrentModeResolutionX(VOID);
ULONG	VideoGetCurrentModeResolutionY(VOID);
ULONG	VideoGetCurrentMode(VOID);

VOID	VideoClearScreen(VOID);
VOID	VideoWaitForVerticalRetrace(VOID);
VOID	VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID	VideoGetPaletteColor(UCHAR Color, PUCHAR Red, PUCHAR Green, PUCHAR Blue);


#endif  // defined __VIDEO_H
