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

#define VERTRES_200_SCANLINES		0x00
#define VERTRES_350_SCANLINES		0x01
#define VERTRES_400_SCANLINES		0x02

#define MODETYPE_TEXT				0
#define MODETYPE_GRAPHICS			1

#define VIDEOMODE_NORMAL_TEXT		0
#define VIDEOMODE_EXTENDED_TEXT		1
#define	VIDEOMODE_80X28				0x501C
#define	VIDEOMODE_80X30				0x501E
#define	VIDEOMODE_80X34				0x5022
#define	VIDEOMODE_80X43				0x502B
#define	VIDEOMODE_80X60				0x503C
#define	VIDEOMODE_132X25			0x8419
#define	VIDEOMODE_132X43			0x842B
#define	VIDEOMODE_132X50			0x8432
#define	VIDEOMODE_132X60			0x843C

#define VIDEOPORT_PALETTE_READ		0x03C7
#define VIDEOPORT_PALETTE_WRITE		0x03C8
#define VIDEOPORT_PALETTE_DATA		0x03C9
#define VIDEOPORT_VERTICAL_RETRACE	0x03DA

#define VIDEOVGA_MEM_ADDRESS		0xA0000
#define VIDEOTEXT_MEM_ADDRESS		0xB8000

typedef struct
{
	U8	Red;
	U8	Green;
	U8	Blue;
} PACKED PALETTE_ENTRY, *PPALETTE_ENTRY;

typedef struct
{
	U16	ModeAttributes;				// mode attributes (see #00080)
	U8	WindowAttributesA;			// window attributes, window A (see #00081)
	U8	WindowsAttributesB;			// window attributes, window B (see #00081)
	U16	WindowGranularity;			// window granularity in KB
	U16	WindowSize;					// window size in KB
	U16	WindowAStartSegment;		// start segment of window A (0000h if not supported)
	U16	WindowBStartSegment;		// start segment of window B (0000h if not supported)
	U32	WindowPositioningFunction;	// -> FAR window positioning function (equivalent to AX=4F05h)
	U16	BytesPerScanLine;			// bytes per scan line
	//---remainder is optional for VESA modes in v1.0/1.1, needed for OEM modes---
	U16	WidthInPixels;				// width in pixels (graphics) or characters (text)
	U16	HeightInPixels;				// height in pixels (graphics) or characters (text)
	U8	CharacterWidthInPixels;		// width of character cell in pixels
	U8	CharacterHeightInPixels;	// height of character cell in pixels
	U8	NumberOfMemoryPlanes;		// number of memory planes
	U8	BitsPerPixel;				// number of bits per pixel
	U8	NumberOfBanks;				// number of banks
	U8	MemoryModel;				// memory model type (see #00082)
	U8	BankSize;					// size of bank in KB
	U8	NumberOfImagePanes;			// number of image pages (less one) that will fit in video RAM
	U8	Reserved1;					// reserved (00h for VBE 1.0-2.0, 01h for VBE 3.0)
	//---VBE v1.2+ ---
	U8	RedMaskSize;				// red mask size
	U8	RedMaskPosition;			// red field position
	U8	GreenMaskSize;				// green mask size
	U8	GreenMaskPosition;			// green field size
	U8	BlueMaskSize;				// blue mask size
	U8	BlueMaskPosition;			// blue field size
	U8	ReservedMaskSize;			// reserved mask size
	U8	ReservedMaskPosition;		// reserved mask position
	U8	DirectColorModeInfo;		// direct color mode info
									// bit 0:Color ramp is programmable
									// bit 1:Bytes in reserved field may be used by application
	//---VBE v2.0+ ---
	U32	LinearVideoBufferAddress;	// physical address of linear video buffer
	U32	OffscreenMemoryPointer;		// pointer to start of offscreen memory
	U16	OffscreenMemorySize;		// KB of offscreen memory
	//---VBE v3.0 ---
	U16	LinearBytesPerScanLine;		// bytes per scan line in linear modes
	U8	BankedNumberOfImages;		// number of images (less one) for banked video modes
	U8	LinearNumberOfImages;		// number of images (less one) for linear video modes
	U8	LinearRedMaskSize;			// linear modes:Size of direct color red mask (in bits)
	U8	LinearRedMaskPosition;		// linear modes:Bit position of red mask LSB (e.g. shift count)
	U8	LinearGreenMaskSize;		// linear modes:Size of direct color green mask (in bits)
	U8	LinearGreenMaskPosition;	// linear modes:Bit position of green mask LSB (e.g. shift count)
	U8	LinearBlueMaskSize;			// linear modes:Size of direct color blue mask (in bits)
	U8	LinearBlueMaskPosition;		// linear modes:Bit position of blue mask LSB (e.g. shift count)
	U8	LinearReservedMaskSize;		// linear modes:Size of direct color reserved mask (in bits)
	U8	LinearReservedMaskPosition;	// linear modes:Bit position of reserved mask LSB
	U32	MaximumPixelClock;			// maximum pixel clock for graphics video mode, in Hz
	U8	Reserved2[190];				// 190 BYTEs  reserved (0)
} PACKED SVGA_MODE_INFORMATION, *PSVGA_MODE_INFORMATION;


extern	U32						CurrentMemoryBank;
extern	SVGA_MODE_INFORMATION	VesaVideoModeInformation;

extern	PVOID					VideoOffScreenBuffer;




VOID	BiosSetVideoMode(U32 VideoMode);				// Implemented in i386vid.S
VOID	BiosSetVideoFont8x8(VOID);						// Implemented in i386vid.S
VOID	BiosSetVideoFont8x14(VOID);						// Implemented in i386vid.S
VOID	BiosSetVideoFont8x16(VOID);						// Implemented in i386vid.S
VOID	BiosSelectAlternatePrintScreen(VOID);			// Implemented in i386vid.S
VOID	BiosDisableCursorEmulation(VOID);				// Implemented in i386vid.S
VOID	BiosDefineCursor(U32 StartScanLine, U32 EndScanLine);	// Implemented in i386vid.S
U32		BiosDetectVideoCard(VOID);						// Implemented in i386vid.S
VOID	BiosSetVerticalResolution(U32 ScanLines);		// Implemented in i386vid.S, must be called right before BiosSetVideoMode()
VOID	BiosSet480ScanLines(VOID);						// Implemented in i386vid.S, must be called right after BiosSetVideoMode()
VOID	BiosSetVideoDisplayEnd(VOID);					// Implemented in i386vid.S
VOID	BiosVideoDisableBlinkBit(VOID);					// Implemented in i386vid.S
VOID	BiosVideoEnableBlinkBit(VOID);					// Implemented in i386vid.S

U16		BiosIsVesaSupported(VOID);						// Implemented in i386vid.S, returns the VESA version
BOOL	BiosVesaSetBank(U16 Bank);						// Implemented in i386vid.S
BOOL	BiosVesaSetVideoMode(U16 Mode);					// Implemented in i386vid.S
BOOL	BiosVesaGetSVGAModeInformation(U16 Mode, PSVGA_MODE_INFORMATION ModeInformation);	// Implemented in i386vid.S

VOID	VideoSetTextCursorPosition(U32 X, U32 Y);		// Implemented in i386vid.S
VOID	VideoHideTextCursor(VOID);						// Implemented in i386vid.S
VOID	VideoShowTextCursor(VOID);						// Implemented in i386vid.S
U32		VideoGetTextCursorPositionX(VOID);				// Implemented in i386vid.S
U32		VideoGetTextCursorPositionY(VOID);				// Implemented in i386vid.S

BOOL	VideoSetMode(U32 VideoMode);
BOOL	VideoSetMode80x25(VOID);						// Sets 80x25
BOOL	VideoSetMode80x50_80x43(VOID);					// Sets 80x50 (VGA) or 80x43 (EGA) 8-pixel mode
BOOL	VideoSetMode80x28(VOID);						// Sets 80x28. Works on all VGA's. Standard 80x25 with 14-point font
BOOL	VideoSetMode80x43(VOID);						// Sets 80x43. Works on all VGA's. It's a 350-scanline mode with 8-pixel font.
BOOL	VideoSetMode80x30(VOID);						// Sets 80x30. Works on all VGA's. 480 scanlines, 16-pixel font.
BOOL	VideoSetMode80x34(VOID);						// Sets 80x34. Works on all VGA's. 480 scanlines, 14-pixel font.
BOOL	VideoSetMode80x60(VOID);						// Sets 80x60. Works on all VGA's. 480 scanlines, 8-pixel font.
U32		VideoGetCurrentModeResolutionX(VOID);
U32		VideoGetCurrentModeResolutionY(VOID);
U32		VideoGetBytesPerScanLine(VOID);
U32		VideoGetCurrentMode(VOID);
U32		VideoGetCurrentModeType(VOID);					// MODETYPE_TEXT or MODETYPE_GRAPHICS
BOOL	VideoIsCurrentModeVesa(VOID);
U32		VideoGetCurrentModeColorDepth(VOID);			// Returns 0 for text mode, 16 for 4-bit, 256 for 8-bit, 32768 for 15-bit, 65536 for 16-bit, etc.

VOID	VideoClearScreen(VOID);
VOID	VideoWaitForVerticalRetrace(VOID);
PVOID	VideoAllocateOffScreenBuffer(VOID);				// Returns a pointer to an off-screen buffer sufficient for the current video mode

U32		VideoGetMemoryBankForPixel(U32 X, U32 Y);
U32		VideoGetMemoryBankForPixel16(U32 X, U32 Y);
U32		VideoGetBankOffsetForPixel(U32 X, U32 Y);
U32		VideoGetBankOffsetForPixel16(U32 X, U32 Y);
VOID	VideoSetMemoryBank(U16 BankNumber);
U32		VideoGetOffScreenMemoryOffsetForPixel(U32 X, U32 Y);
VOID	VideoCopyOffScreenBufferToVRAM(VOID);

VOID	VideoSetPaletteColor(U8 Color, U8 Red, U8 Green, U8 Blue);
VOID	VideoGetPaletteColor(U8 Color, U8* Red, U8* Green, U8* Blue);
VOID	VideoSavePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount);
VOID	VideoRestorePaletteState(PPALETTE_ENTRY Palette, U32 ColorCount);

VOID	VideoSetPixel16(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixel256(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixelRGB(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue);
VOID	VideoSetPixel16_OffScreen(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixel256_OffScreen(U32 X, U32 Y, U8 Color);
VOID	VideoSetPixelRGB_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue);

VOID	VideoSetAllColorsToBlack(U32 ColorCount);
VOID	VideoFadeIn(PPALETTE_ENTRY Palette, U32 ColorCount);
VOID	VideoFadeOut(U32 ColorCount);


#endif  // defined __VIDEO_H
