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

#ifndef __VIDEO_H
#define __VIDEO_H

typedef struct
{
  U8 Red;
  U8 Green;
  U8 Blue;
} PACKED PALETTE_ENTRY, *PPALETTE_ENTRY;

extern	PVOID	VideoOffScreenBuffer;

U16		BiosIsVesaSupported(VOID);						// Implemented in i386vid.S, returns the VESA version

PVOID	VideoAllocateOffScreenBuffer(VOID);				// Returns a pointer to an off-screen buffer sufficient for the current video mode

#if 0 /* Not used */
U32	VideoGetMemoryBankForPixel(U32 X, U32 Y);
U32	VideoGetMemoryBankForPixel16(U32 X, U32 Y);
U32	VideoGetBankOffsetForPixel(U32 X, U32 Y);
U32	VideoGetBankOffsetForPixel16(U32 X, U32 Y);
VOID	VideoSetMemoryBank(U16 BankNumber);
U32	VideoGetOffScreenMemoryOffsetForPixel(U32 X, U32 Y);
#endif
VOID	VideoCopyOffScreenBufferToVRAM(VOID);

VOID	VideoSavePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount);
VOID	VideoRestorePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount);

#if 0 /* Not used */
VOID	VideoSetPixel16(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixel256(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixelRGB(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue);
VOID	VideoSetPixel16_OffScreen(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixel256_OffScreen(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixelRGB_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue);
#endif

VOID	VideoSetAllColorsToBlack(U32 ColorCount);
VOID	VideoFadeIn(PPALETTE_ENTRY Palette, U32 ColorCount);
VOID	VideoFadeOut(U32 ColorCount);


#endif  // defined __VIDEO_H
