/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *  Portions from Linux video.S - Display adapter & video mode setup, version 2.13 (14-May-99)
 *  Copyright (C) 1995 -- 1999 Martin Mares <mj@ucw.cz>
 *  Based on the original setup.S code (C) Linus Torvalds and Mats Anderson
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
#include <arch.h>
#include <video.h>
#include <comm.h>
#include <rtl.h>
#include <debug.h>

typedef struct
{
	U8	Signature[4];				// (ret) signature ("VESA")
									// (call) VESA 2.0 request signature ("VBE2"), required to receive
									// version 2.0 info
	U16	VesaVersion;				// VESA version number (one-digit minor version -- 0102h = v1.2)
	U32 OemNamePtr;					// pointer to OEM name
									// "761295520" for ATI
	U32	Capabilities;				// capabilities flags (see #00078)
	U32	SupportedModeListPtr;		// pointer to list of supported VESA and OEM video modes
									// (list of words terminated with FFFFh)
	U16	TotalVideoMemory;			// total amount of video memory in 64K blocks

	// ---VBE v1.x ---
	//U8	Reserved[236];

	// ---VBE v2.0 ---
	U16	OemSoftwareVersion;			// OEM software version (BCD, high byte = major, low byte = minor)
	U32	VendorNamePtr;				// pointer to vendor name
	U32	ProductNamePtr;				// pointer to product name
	U32	ProductRevisionStringPtr;	// pointer to product revision string
	U16	VBE_AF_Version;				// (if capabilities bit 3 set) VBE/AF version (BCD)
									// 0100h for v1.0P
	U32	AcceleratedModeListPtr;		// (if capabilities bit 3 set) pointer to list of supported
									// accelerated video modes (list of words terminated with FFFFh)
	U8	Reserved[216];				// reserved for VBE implementation
	U8	ScratchPad[256];			// OEM scratchpad (for OEM strings, etc.)
} PACKED VESA_SVGA_INFO, *PVESA_SVGA_INFO;

// Bitfields for VESA capabilities:
//
// Bit(s)  Description     (Table 00078)
// 0      DAC can be switched into 8-bit mode
// 1      non-VGA controller
// 2      programmed DAC with blank bit (i.e. only during blanking interval)
// 3      (VBE v3.0) controller supports hardware stereoscopic signalling
// 3      controller supports VBE/AF v1.0P extensions
// 4      (VBE v3.0) if bit 3 set:
// =0 stereo signalling via external VESA stereo connector
// =1 stereo signalling via VESA EVC connector
// 4      (VBE/AF v1.0P) must call EnableDirectAccess to access framebuffer
// 5      (VBE/AF v1.0P) controller supports hardware mouse cursor
// 6      (VBE/AF v1.0P) controller supports hardware clipping
// 7      (VBE/AF v1.0P) controller supports transparent BitBLT
// 8-31   reserved (0)

// Notes: The list of supported video modes is stored in the reserved
// portion of the SuperVGA information record by some implementations,
// and it may thus be necessary to either copy the mode list or use a
// different buffer for all subsequent VESA calls. Not all of the video
// modes in the list of mode numbers may be supported, e.g. if they require
// more memory than currently installed or are not supported by the
// attached monitor. Check any mode you intend to use through AX=4F01h first..
// The 1.1 VESA document specifies 242 reserved bytes at the end, so the
// buffer should be 262 bytes to ensure that it is not overrun; for v2.0,
// the buffer should be 512 bytes. The S3 specific video modes will most
// likely follow the FFFFh terminator at the end of the standard modes.
// A search must then be made to find them, FFFFh will also terminate this
// second list. In some cases, only a "stub" VBE may be present, supporting
// only AX=4F00h; this case may be assumed if the list of supported video modes
// is empty (consisting of a single word of FFFFh) 


VOID BiosSetVideoMode(U32 VideoMode)
{
	REGS	Regs;

	// Int 10h AH=00h
	// VIDEO - SET VIDEO MODE
	//
	// AH = 00h
	// AL = desired video mode
	// Return:
	// AL = video mode flag (Phoenix, AMI BIOS)
	// 20h mode > 7
	// 30h modes 0-5 and 7
	// 3Fh mode 6
	// AL = CRT controller mode byte (Phoenix 386 BIOS v1.10)
	Regs.b.ah = 0x00;
	Regs.b.al = VideoMode;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosSetVideoFont8x8(VOID)
{
	REGS	Regs;

	// Int 10h AX=1112h
	// VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x8 DBL-DOT PATTERNS (PS,EGA,VGA)
	//
	// AX = 1112h
	// BL = block to load
	// Return:
	// Nothing
	Regs.w.ax = 0x1112;
	Regs.b.bl = 0x00;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosSetVideoFont8x14(VOID)
{
	REGS	Regs;

	// Int 10h AX=1111h
	// VIDEO - TEXT-MODE CHARGEN - LOAD ROM MONOCHROME PATTERNS (PS,EGA,VGA)
	//
	// AX = 1111h
	// BL = block to load
	// Return:
	// Nothing
	Regs.w.ax = 0x1111;
	Regs.b.bl = 0;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosSetVideoFont8x16(VOID)
{
	REGS	Regs;

	// Int 10h AX=1114h
	// VIDEO - TEXT-MODE CHARGEN - LOAD ROM 8x16 CHARACTER SET (VGA)
	//
	// AX = 1114h
	// BL = block to load
	// Return:
	// Nothing
	Regs.w.ax = 0x1114;
	Regs.b.bl = 0;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosSelectAlternatePrintScreen(VOID)
{
	REGS	Regs;

	// Int 10h AH=12h BL=20h
	// VIDEO - ALTERNATE FUNCTION SELECT (PS,EGA,VGA,MCGA) - ALTERNATE PRTSC
	//
	// AH = 12h
	// BL = 20h select alternate print screen routine
	// Return:
	// Nothing
	//
	// Installs a PrtSc routine from the video card's BIOS to replace the
	// default PrtSc handler from the ROM BIOS, which usually does not
	// understand screen heights other than 25 lines.
	//
	// Some adapters disable print-screen instead of enhancing it.
	Regs.b.ah = 0x12;
	Regs.b.bl = 0x20;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosDisableCursorEmulation(VOID)
{
	REGS	Regs;

	// Int 10h AH=12h BL=34h
	// VIDEO - ALTERNATE FUNCTION SELECT (VGA) - CURSOR EMULATION
	//
	// AH = 12h
	// BL = 34h
	// AL = new state
	// 00h enable alphanumeric cursor emulation
	// 01h disable alphanumeric cursor emulation
	// Return:
	// AL = 12h if function supported
	//
	// Specify whether the BIOS should automatically remap cursor start/end
	// according to the current character height in text modes.
	Regs.b.ah = 0x12;
	Regs.b.bl = 0x34;
	Regs.b.al = 0x01;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosDefineCursor(U32 StartScanLine, U32 EndScanLine)
{
	REGS	Regs;

	// Int 10h AH=01h
	// VIDEO - SET TEXT-MODE CURSOR SHAPE
	//
	// AH = 01h
	// CH = cursor start and options
	// CL = bottom scan line containing cursor (bits 0-4)
	// Return:
	// Nothing
	//
	// Specify the starting and ending scan lines to be occupied
	// by the hardware cursor in text modes.
	//
	// AMI 386 BIOS and AST Premier 386 BIOS will lock up the
	// system if AL is not equal to the current video mode.
	//
	// Bitfields for cursor start and options:
	//
	// Bit(s)    Description
	// 7         should be zero
	// 6,5       cursor blink
	// (00=normal, 01=invisible, 10=erratic, 11=slow).
	// (00=normal, other=invisible on EGA/VGA)
	// 4-0       topmost scan line containing cursor
	Regs.b.ah = 0x01;
	Regs.b.al = 0x03;
	Regs.b.ch = StartScanLine;
	Regs.b.cl = EndScanLine;
	Int386(0x10, &Regs, &Regs);
}

U32 BiosDetectVideoCard(VOID)
{
	REGS	Regs;

	// Int 10h AH=12h BL=10h
	// VIDEO - ALTERNATE FUNCTION SELECT (PS,EGA,VGA,MCGA) - GET EGA INFO
	//
	// AH = 12h
	// BL = 10h
	// Return:
	// BH = video state
	// 00h color mode in effect (I/O port 3Dxh)
	// 01h mono mode in effect (I/O port 3Bxh)
	// BL = installed memory (00h = 64K, 01h = 128K, 02h = 192K, 03h = 256K)
	// CH = feature connector bits
	// CL = switch settings
	// AH destroyed (at least by Tseng ET4000 BIOS v8.00n)
	//
	// Installation check;EGA
	Regs.b.ah = 0x12;
	Regs.b.bl = 0x10;
	Int386(0x10, &Regs, &Regs);

	// If BL is still equal to 0x10 then there is no EGA/VGA present
	if (Regs.b.bl == 0x10)
	{
		return VIDEOCARD_CGA_OR_OTHER;
	}

	// Int 10h AX=1A00h
	// VIDEO - GET DISPLAY COMBINATION CODE (PS,VGA/MCGA)
	//
	// AX = 1A00h
	// Return:
	// AL = 1Ah if function was supported
	// BL = active display code
	// BH = alternate display code
	//
	// This function is commonly used to check for the presence of a VGA.
	//
	// Installation check;VGA
	//
	// Values for display combination code:
	// 00h    no display
	// 01h    monochrome adapter w/ monochrome display
	// 02h    CGA w/ color display
	// 03h    reserved
	// 04h    EGA w/ color display
	// 05h    EGA w/ monochrome display
	// 06h    PGA w/ color display
	// 07h    VGA w/ monochrome analog display
	// 08h    VGA w/ color analog display
	// 09h    reserved
	// 0Ah    MCGA w/ digital color display
	// 0Bh    MCGA w/ monochrome analog display
	// 0Ch    MCGA w/ color analog display
	// FFh    unknown display type
	Regs.b.ah = 0x12;
	Regs.b.bl = 0x10;
	Int386(0x10, &Regs, &Regs);

	if (Regs.b.al == 0x1A)
	{
		return VIDEOCARD_VGA;
	}
	else
	{
		return VIDEOCARD_EGA;
	}
}

VOID BiosSetVerticalResolution(U32 ScanLines)
{
	REGS	Regs;

	// Int 10h AH=12h BL=30h
	// VIDEO - ALTERNATE FUNCTION SELECT (VGA) - SELECT VERTICAL RESOLUTION
	//
	// AH = 12h
	// BL = 30h
	// AL = vertical resolution
	// 00h 200 scan lines
	// 01h 350 scan lines
	// 02h 400 scan lines
	// Return:
	// AL = 12h if function supported
	//
	// Specifiy the number of scan lines used to display text modes.
	//
	// The specified resolution will take effect on the next mode set.
	Regs.b.ah = 0x12;
	Regs.b.bl = 0x30;
	Regs.b.al = ScanLines;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosSet480ScanLines(VOID)
{
	int	CRTC;

	// Read CRTC port
	CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

	if (CRTC & 1)
	{
		CRTC = 0x3D4;
	}
	else
	{
		CRTC = 0x3B4;
	}

	// Vertical sync end (also unlocks CR0-7)
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x11);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x0C);

	// Vertical total
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x06);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x0B);

	// (vertical) overflow
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x07);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x3E);

	// Vertical sync start
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x10);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xEA);

	// Vertical display end
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x12);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xDF);

	// Vertical blank start
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x15);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xE7);

	// Vertical blank end
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x16);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0x04);

	// Misc output register (read)
	CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

	// Preserve clock select bits and color bit
	CRTC = (CRTC & 0x0D);
	// Set correct sync polarity
	CRTC = (CRTC | 0xE2);

	// (write)
	WRITE_PORT_UCHAR((PUCHAR)0x03C2, CRTC);
}

VOID BiosSetVideoDisplayEnd(VOID)
{
	int	CRTC;

	// Read CRTC port
	CRTC = READ_PORT_UCHAR((PUCHAR)0x03CC);

	if (CRTC & 1)
	{
		CRTC = 0x3D4;
	}
	else
	{
		CRTC = 0x3B4;
	}

	// Vertical display end
	WRITE_PORT_UCHAR((PUCHAR)CRTC, 0x12);
	WRITE_PORT_UCHAR((PUCHAR)CRTC+1, 0xDF);
}

VOID VideoSetTextCursorPosition(U32 X, U32 Y)
{
	REGS	Regs;

	// Int 10h AH=02h
	// VIDEO - SET CURSOR POSITION
	//
	// AH = 02h
	// BH = page number
	// 0-3 in modes 2&3
	// 0-7 in modes 0&1
	// 0 in graphics modes
	// DH = row (00h is top)
	// DL = column (00h is left)
	// Return:
	// Nothing
	Regs.b.ah = 0x01;
	Regs.b.bh = 0x00;
	Regs.b.dh = Y;
	Regs.b.dl = X;
	Int386(0x10, &Regs, &Regs);
}

VOID VideoHideTextCursor(VOID)
{
	BiosDefineCursor(0x20, 0x00);
}

VOID VideoShowTextCursor(VOID)
{
	BiosDefineCursor(0x0D, 0x0E);
}

U32 VideoGetTextCursorPositionX(VOID)
{
	REGS	Regs;

	// Int 10h AH=03h
	// VIDEO - GET CURSOR POSITION AND SIZE
	//
	// AH = 03h
	// BH = page number
	// 0-3 in modes 2&3
	// 0-7 in modes 0&1
	// 0 in graphics modes
	// Return:
	// AX = 0000h (Phoenix BIOS)
	// CH = start scan line
	// CL = end scan line
	// DH = row (00h is top)
	// DL = column (00h is left)
	Regs.b.ah = 0x03;
	Regs.b.bh = 0x00;
	Int386(0x10, &Regs, &Regs);

	return Regs.b.dl;
}

U32 VideoGetTextCursorPositionY(VOID)
{
	REGS	Regs;

	// Int 10h AH=03h
	// VIDEO - GET CURSOR POSITION AND SIZE
	//
	// AH = 03h
	// BH = page number
	// 0-3 in modes 2&3
	// 0-7 in modes 0&1
	// 0 in graphics modes
	// Return:
	// AX = 0000h (Phoenix BIOS)
	// CH = start scan line
	// CL = end scan line
	// DH = row (00h is top)
	// DL = column (00h is left)
	Regs.b.ah = 0x03;
	Regs.b.bh = 0x00;
	Int386(0x10, &Regs, &Regs);

	return Regs.b.dh;
}

VOID BiosVideoDisableBlinkBit(VOID)
{
	REGS	Regs;

	// Int 10h AX=1003h
	// VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
	//
	// AX = 1003h
	// BL = new state
	// 00h background intensity enabled
	// 01h blink enabled
	// BH = 00h to avoid problems on some adapters
	// Return:
	// Nothing
	//
	// Note: although there is no function to get
	// the current status, bit 5 of 0040h:0065h
	// indicates the state.
	Regs.w.ax = 0x1003;
	Regs.w.bx = 0x0000;
	Int386(0x10, &Regs, &Regs);
}

VOID BiosVideoEnableBlinkBit(VOID)
{
	REGS	Regs;

	// Int 10h AX=1003h
	// VIDEO - TOGGLE INTENSITY/BLINKING BIT (Jr, PS, TANDY 1000, EGA, VGA)
	//
	// AX = 1003h
	// BL = new state
	// 00h background intensity enabled
	// 01h blink enabled
	// BH = 00h to avoid problems on some adapters
	// Return:
	// Nothing
	//
	// Note: although there is no function to get
	// the current status, bit 5 of 0040h:0065h
	// indicates the state.
	Regs.w.ax = 0x1003;
	Regs.w.bx = 0x0001;
	Int386(0x10, &Regs, &Regs);
}

U16 BiosIsVesaSupported(VOID)
{
	REGS			Regs;
	PVESA_SVGA_INFO	SvgaInfo = (PVESA_SVGA_INFO)BIOSCALLBUFFER;
#ifdef DEBUG
	//U16*			VideoModes;
	//U16			Index;
#endif // defined DEBUG

	DbgPrint((DPRINT_UI, "BiosIsVesaSupported()\n"));

	RtlZeroMemory(SvgaInfo, sizeof(VESA_SVGA_INFO));

	// Make sure we receive version 2.0 info
	SvgaInfo->Signature[0] = 'V';
	SvgaInfo->Signature[1] = 'B';
	SvgaInfo->Signature[2] = 'E';
	SvgaInfo->Signature[3] = '2';

	// Int 10h AX=4F00h
	// VESA SuperVGA BIOS (VBE) - GET SuperVGA INFORMATION
	//
	// AX = 4F00h
	// ES:DI -> buffer for SuperVGA information (see #00077)
	// Return:
	// AL = 4Fh if function supported
	// AH = status
	//   00h successful
	// ES:DI buffer filled
	//   01h failed
	//   ---VBE v2.0---
	//   02h function not supported by current hardware configuration
	//   03h function invalid in current video mode
	//
	// Determine whether VESA BIOS extensions are present and the
	// capabilities supported by the display adapter
	//
	// Installation check;VESA SuperVGA
	Regs.w.ax = 0x4F00;
	Regs.w.es = BIOSCALLBUFSEGMENT;
	Regs.w.di = BIOSCALLBUFOFFSET;
	Int386(0x10, &Regs, &Regs);

	DbgPrint((DPRINT_UI, "AL = 0x%x\n", Regs.b.al));
	DbgPrint((DPRINT_UI, "AH = 0x%x\n", Regs.b.ah));

	if (Regs.w.ax != 0x004F)
	{
		DbgPrint((DPRINT_UI, "Failed.\n"));
		return 0x0000;
	}

#ifdef DEBUG
	DbgPrint((DPRINT_UI, "Supported.\n"));
	DbgPrint((DPRINT_UI, "SvgaInfo->Signature[4] = %c%c%c%c\n", SvgaInfo->Signature[0], SvgaInfo->Signature[1], SvgaInfo->Signature[2], SvgaInfo->Signature[3]));
	DbgPrint((DPRINT_UI, "SvgaInfo->VesaVersion = v%d.%d\n", ((SvgaInfo->VesaVersion >> 8) & 0xFF), (SvgaInfo->VesaVersion & 0xFF)));
	DbgPrint((DPRINT_UI, "SvgaInfo->OemNamePtr = 0x%x\n", SvgaInfo->OemNamePtr));
	DbgPrint((DPRINT_UI, "SvgaInfo->Capabilities = 0x%x\n", SvgaInfo->Capabilities));
	DbgPrint((DPRINT_UI, "SvgaInfo->VideoMemory = %dK\n", SvgaInfo->TotalVideoMemory * 64));
	DbgPrint((DPRINT_UI, "---VBE v2.0 ---\n"));
	DbgPrint((DPRINT_UI, "SvgaInfo->OemSoftwareVersion = v%d.%d\n", ((SvgaInfo->OemSoftwareVersion >> 8) & 0x0F) + (((SvgaInfo->OemSoftwareVersion >> 12) & 0x0F) * 10), (SvgaInfo->OemSoftwareVersion & 0x0F) + (((SvgaInfo->OemSoftwareVersion >> 4) & 0x0F) * 10)));
	DbgPrint((DPRINT_UI, "SvgaInfo->VendorNamePtr = 0x%x\n", SvgaInfo->VendorNamePtr));
	DbgPrint((DPRINT_UI, "SvgaInfo->ProductNamePtr = 0x%x\n", SvgaInfo->ProductNamePtr));
	DbgPrint((DPRINT_UI, "SvgaInfo->ProductRevisionStringPtr = 0x%x\n", SvgaInfo->ProductRevisionStringPtr));
	DbgPrint((DPRINT_UI, "SvgaInfo->VBE/AF Version = 0x%x (BCD WORD)\n", SvgaInfo->VBE_AF_Version));

	//DbgPrint((DPRINT_UI, "\nSupported VESA and OEM video modes:\n"));
	//VideoModes = (U16*)SvgaInfo->SupportedModeListPtr;
	//for (Index=0; VideoModes[Index]!=0xFFFF; Index++)
	//{
	//	DbgPrint((DPRINT_UI, "Mode %d: 0x%x\n", Index, VideoModes[Index]));
	//}

	//if (SvgaInfo->VesaVersion >= 0x0200)
	//{
	//	DbgPrint((DPRINT_UI, "\nSupported accelerated video modes (VESA v2.0):\n"));
	//	VideoModes = (U16*)SvgaInfo->AcceleratedModeListPtr;
	//	for (Index=0; VideoModes[Index]!=0xFFFF; Index++)
	//	{
	//		DbgPrint((DPRINT_UI, "Mode %d: 0x%x\n", Index, VideoModes[Index]));
	//	}
	//}

	DbgPrint((DPRINT_UI, "\n"));
	//getch();
#endif // defined DEBUG

	return SvgaInfo->VesaVersion;
}

BOOL BiosVesaSetBank(U16 Bank)
{
	REGS	Regs;

	// Int 10h AX=4F05h
	// VESA SuperVGA BIOS - CPU VIDEO MEMORY CONTROL
	//
	// AX = 4F05h
	// BH = subfunction
	//   00h select video memory window
	//   01h get video memory window
	// DX = window address in video memory (in granularity units)
	// Return:
	// DX = window address in video memory (in gran. units)
	// BL = window number
	//   00h window A
	//   01h window B.
	// Return:
	// AL = 4Fh if function supported
	// AH = status
	//   00h successful
	//   01h failed

	Regs.w.ax = 0x4F05;
	Regs.w.bx = 0x0000;
	Regs.w.dx = Bank;
	Int386(0x10, &Regs, &Regs);

	if (Regs.w.ax != 0x004F)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL BiosVesaSetVideoMode(U16 Mode)
{
	REGS	Regs;

	// Int 10h AX=4F02h
	// VESA SuperVGA BIOS - SET SuperVGA VIDEO MODE
	//
	// AX = 4F02h
	// BX = new video mode
	// ES:DI -> (VBE 3.0+) CRTC information block, bit mode bit 11 set
	// Return:
	// AL = 4Fh if function supported
	// AH = status
	//   00h successful
	//   01h failed
	//
	// Values for VESA video mode:
	// 00h-FFh OEM video modes (see #00010 at AH=00h)
	// 100h   640x400x256
	// 101h   640x480x256
	// 102h   800x600x16
	// 103h   800x600x256
	// 104h   1024x768x16
	// 105h   1024x768x256
	// 106h   1280x1024x16
	// 107h   1280x1024x256
	// 108h   80x60 text
	// 109h   132x25 text
	// 10Ah   132x43 text
	// 10Bh   132x50 text
	// 10Ch   132x60 text
	// ---VBE v1.2+ ---
	// 10Dh   320x200x32K
	// 10Eh   320x200x64K
	// 10Fh   320x200x16M
	// 110h   640x480x32K
	// 111h   640x480x64K
	// 112h   640x480x16M
	// 113h   800x600x32K
	// 114h   800x600x64K
	// 115h   800x600x16M
	// 116h   1024x768x32K
	// 117h   1024x768x64K
	// 118h   1024x768x16M
	// 119h   1280x1024x32K (1:5:5:5)
	// 11Ah   1280x1024x64K (5:6:5)
	// 11Bh   1280x1024x16M
	// ---VBE 2.0+ ---
	// 120h   1600x1200x256
	// 121h   1600x1200x32K
	// 122h   1600x1200x64K
	// 81FFh   special full-memory access mode

	// Notes: The special mode 81FFh preserves the contents of the video memory and gives
	// access to all of the memory; VESA recommends that the special mode be a packed-pixel
	// mode. For VBE 2.0+, it is required that the VBE implement the mode, but not place it
	// in the list of available modes (mode information for this mode can be queried
	// directly, however).. As of VBE 2.0, VESA will no longer define video mode numbers 
	Regs.w.ax = 0x4F02;
	Regs.w.bx = Mode;
	Int386(0x10, &Regs, &Regs);

	if (Regs.w.ax != 0x004F)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL BiosVesaGetSVGAModeInformation(U16 Mode, PSVGA_MODE_INFORMATION ModeInformation)
{
	REGS	Regs;

	RtlZeroMemory((PVOID)BIOSCALLBUFFER, 256);

	// VESA SuperVGA BIOS - GET SuperVGA MODE INFORMATION
	// AX = 4F01h
	// CX = SuperVGA video mode (see #04082 for bitfields)
	// ES:DI -> 256-byte buffer for mode information (see #00079)
	// Return:
	// AL = 4Fh if function supported
	// AH = status
	// 00h successful
	// ES:DI buffer filled
	// 01h failed
	//
	// Desc: Determine the attributes of the specified video mode
	//
	// Note: While VBE 1.1 and higher will zero out all unused bytes
	// of the buffer, v1.0 did not, so applications that want to be
	// backward compatible should clear the buffer before calling 
	Regs.w.ax = 0x4F01;
	Regs.w.cx = Mode;
	Regs.w.es = BIOSCALLBUFSEGMENT;
	Regs.w.di = BIOSCALLBUFOFFSET;
	Int386(0x10, &Regs, &Regs);

	if (Regs.w.ax != 0x004F)
	{
		return FALSE;
	}

	RtlCopyMemory(ModeInformation, (PVOID)BIOSCALLBUFFER, sizeof(SVGA_MODE_INFORMATION));

	DbgPrint((DPRINT_UI, "\n"));
	DbgPrint((DPRINT_UI, "BiosVesaGetSVGAModeInformation() mode 0x%x\n", Mode));
	DbgPrint((DPRINT_UI, "ModeAttributes = 0x%x\n", ModeInformation->ModeAttributes));
	DbgPrint((DPRINT_UI, "WindowAttributesA = 0x%x\n", ModeInformation->WindowAttributesA));
	DbgPrint((DPRINT_UI, "WindowAttributesB = 0x%x\n", ModeInformation->WindowsAttributesB));
	DbgPrint((DPRINT_UI, "WindowGranularity = %dKB\n", ModeInformation->WindowGranularity));
	DbgPrint((DPRINT_UI, "WindowSize = %dKB\n", ModeInformation->WindowSize));
	DbgPrint((DPRINT_UI, "WindowAStartSegment = 0x%x\n", ModeInformation->WindowAStartSegment));
	DbgPrint((DPRINT_UI, "WindowBStartSegment = 0x%x\n", ModeInformation->WindowBStartSegment));
	DbgPrint((DPRINT_UI, "WindowPositioningFunction = 0x%x\n", ModeInformation->WindowPositioningFunction));
	DbgPrint((DPRINT_UI, "BytesPerScanLine = %d\n", ModeInformation->BytesPerScanLine));
	DbgPrint((DPRINT_UI, "WidthInPixels = %d\n", ModeInformation->WidthInPixels));
	DbgPrint((DPRINT_UI, "HeightInPixels = %d\n", ModeInformation->HeightInPixels));
	DbgPrint((DPRINT_UI, "CharacterWidthInPixels = %d\n", ModeInformation->CharacterWidthInPixels));
	DbgPrint((DPRINT_UI, "CharacterHeightInPixels = %d\n", ModeInformation->CharacterHeightInPixels));
	DbgPrint((DPRINT_UI, "NumberOfMemoryPlanes = %d\n", ModeInformation->NumberOfMemoryPlanes));
	DbgPrint((DPRINT_UI, "BitsPerPixel = %d\n", ModeInformation->BitsPerPixel));
	DbgPrint((DPRINT_UI, "NumberOfBanks = %d\n", ModeInformation->NumberOfBanks));
	DbgPrint((DPRINT_UI, "MemoryModel = %d\n", ModeInformation->MemoryModel));
	DbgPrint((DPRINT_UI, "BankSize = %d\n", ModeInformation->BankSize));
	DbgPrint((DPRINT_UI, "NumberOfImagePlanes = %d\n", ModeInformation->NumberOfImagePanes));
	DbgPrint((DPRINT_UI, "---VBE v1.2+ ---\n"));
	DbgPrint((DPRINT_UI, "RedMaskSize = %d\n", ModeInformation->RedMaskSize));
	DbgPrint((DPRINT_UI, "RedMaskPosition = %d\n", ModeInformation->RedMaskPosition));
	DbgPrint((DPRINT_UI, "GreenMaskSize = %d\n", ModeInformation->GreenMaskSize));
	DbgPrint((DPRINT_UI, "GreenMaskPosition = %d\n", ModeInformation->GreenMaskPosition));
	DbgPrint((DPRINT_UI, "BlueMaskSize = %d\n", ModeInformation->BlueMaskSize));
	DbgPrint((DPRINT_UI, "BlueMaskPosition = %d\n", ModeInformation->BlueMaskPosition));
	DbgPrint((DPRINT_UI, "ReservedMaskSize = %d\n", ModeInformation->ReservedMaskSize));
	DbgPrint((DPRINT_UI, "ReservedMaskPosition = %d\n", ModeInformation->ReservedMaskPosition));
	DbgPrint((DPRINT_UI, "\n"));

	return TRUE;
}
