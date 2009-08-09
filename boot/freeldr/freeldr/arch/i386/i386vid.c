/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#define NDEBUG
#include <debug.h>

/* non-standard specifier from windef.h -- please deprecate */
#undef PACKED
#ifdef __GNUC__
#define PACKED __attribute__((packed))
#endif

typedef struct
{
	UCHAR	Signature[4];				// (ret) signature ("VESA")
									// (call) VESA 2.0 request signature ("VBE2"), required to receive
									// version 2.0 info
	USHORT	VesaVersion;				// VESA version number (one-digit minor version -- 0102h = v1.2)
	ULONG OemNamePtr;					// pointer to OEM name
									// "761295520" for ATI
	ULONG	Capabilities;				// capabilities flags (see #00078)
	ULONG	SupportedModeListPtr;		// pointer to list of supported VESA and OEM video modes
									// (list of words terminated with FFFFh)
	USHORT	TotalVideoMemory;			// total amount of video memory in 64K blocks

	// ---VBE v1.x ---
	//UCHAR	Reserved[236];

	// ---VBE v2.0 ---
	USHORT	OemSoftwareVersion;			// OEM software version (BCD, high byte = major, low byte = minor)
	ULONG	VendorNamePtr;				// pointer to vendor name
	ULONG	ProductNamePtr;				// pointer to product name
	ULONG	ProductRevisionStringPtr;	// pointer to product revision string
	USHORT	VBE_AF_Version;				// (if capabilities bit 3 set) VBE/AF version (BCD)
									// 0100h for v1.0P
	ULONG	AcceleratedModeListPtr;		// (if capabilities bit 3 set) pointer to list of supported
									// accelerated video modes (list of words terminated with FFFFh)
	UCHAR	Reserved[216];				// reserved for VBE implementation
	UCHAR	ScratchPad[256];			// OEM scratchpad (for OEM strings, etc.)
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
#if 0
static VOID BiosSetVideoFont8x16(VOID)
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

static VOID VideoSetTextCursorPosition(ULONG X, ULONG Y)
{
}

static ULONG VideoGetTextCursorPositionX(VOID)
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

static ULONG VideoGetTextCursorPositionY(VOID)
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
#endif

USHORT BiosIsVesaSupported(VOID)
{
	REGS			Regs;
	PVESA_SVGA_INFO	SvgaInfo = (PVESA_SVGA_INFO)BIOSCALLBUFFER;
	//USHORT*			VideoModes;
	//USHORT			Index;

	DPRINTM(DPRINT_UI, "BiosIsVesaSupported()\n");

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

	DPRINTM(DPRINT_UI, "AL = 0x%x\n", Regs.b.al);
	DPRINTM(DPRINT_UI, "AH = 0x%x\n", Regs.b.ah);

	if (Regs.w.ax != 0x004F)
	{
		DPRINTM(DPRINT_UI, "Failed.\n");
		return 0x0000;
	}

	DPRINTM(DPRINT_UI, "Supported.\n");
	DPRINTM(DPRINT_UI, "SvgaInfo->Signature[4] = %c%c%c%c\n", SvgaInfo->Signature[0], SvgaInfo->Signature[1], SvgaInfo->Signature[2], SvgaInfo->Signature[3]);
	DPRINTM(DPRINT_UI, "SvgaInfo->VesaVersion = v%d.%d\n", ((SvgaInfo->VesaVersion >> 8) & 0xFF), (SvgaInfo->VesaVersion & 0xFF));
	DPRINTM(DPRINT_UI, "SvgaInfo->OemNamePtr = 0x%x\n", SvgaInfo->OemNamePtr);
	DPRINTM(DPRINT_UI, "SvgaInfo->Capabilities = 0x%x\n", SvgaInfo->Capabilities);
	DPRINTM(DPRINT_UI, "SvgaInfo->VideoMemory = %dK\n", SvgaInfo->TotalVideoMemory * 64);
	DPRINTM(DPRINT_UI, "---VBE v2.0 ---\n");
	DPRINTM(DPRINT_UI, "SvgaInfo->OemSoftwareVersion = v%d.%d\n", ((SvgaInfo->OemSoftwareVersion >> 8) & 0x0F) + (((SvgaInfo->OemSoftwareVersion >> 12) & 0x0F) * 10), (SvgaInfo->OemSoftwareVersion & 0x0F) + (((SvgaInfo->OemSoftwareVersion >> 4) & 0x0F) * 10));
	DPRINTM(DPRINT_UI, "SvgaInfo->VendorNamePtr = 0x%x\n", SvgaInfo->VendorNamePtr);
	DPRINTM(DPRINT_UI, "SvgaInfo->ProductNamePtr = 0x%x\n", SvgaInfo->ProductNamePtr);
	DPRINTM(DPRINT_UI, "SvgaInfo->ProductRevisionStringPtr = 0x%x\n", SvgaInfo->ProductRevisionStringPtr);
	DPRINTM(DPRINT_UI, "SvgaInfo->VBE/AF Version = 0x%x (BCD WORD)\n", SvgaInfo->VBE_AF_Version);

	//DPRINTM(DPRINT_UI, "\nSupported VESA and OEM video modes:\n");
	//VideoModes = (USHORT*)SvgaInfo->SupportedModeListPtr;
	//for (Index=0; VideoModes[Index]!=0xFFFF; Index++)
	//{
	//	DPRINTM(DPRINT_UI, "Mode %d: 0x%x\n", Index, VideoModes[Index]);
	//}

	//if (SvgaInfo->VesaVersion >= 0x0200)
	//{
	//	DPRINTM(DPRINT_UI, "\nSupported accelerated video modes (VESA v2.0):\n");
	//	VideoModes = (USHORT*)SvgaInfo->AcceleratedModeListPtr;
	//	for (Index=0; VideoModes[Index]!=0xFFFF; Index++)
	//	{
	//		DPRINTM(DPRINT_UI, "Mode %d: 0x%x\n", Index, VideoModes[Index]);
	//	}
	//}

	DPRINTM(DPRINT_UI, "\n");

	return SvgaInfo->VesaVersion;
}
