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
#include <portio.h>
#include <debug.h>





//
// Arrrggh!
// I really really hate 16 color bit plane modes.
// They should all burn in hell for what they've
// done to my sleeping habits. And I still can't
// get this code to work and I have absolutely
// no idea why...
//
// Someone else can take up this portion of the
// boot loader because my give up...
//
// I was going to store the offscreen buffer as
// a big array of bytes (as opposed to four bits)
// and that makes it a bit easier to set a pixel
// on the offscreen buffer, but will have to be
// handled in the VideoCopyOffScreenBufferToVRAM()
// function.
//
VOID VideoSetPixel16(U32 X, U32 Y, U8 Color)
{
	U8		CurrentColor;
	U8*		MemoryPointer;
	U32		ByteOffset;
	U8		BitInByte;
	U8		ReadByte;
	U8		BitToChange;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	// Calculate the byte offset into the bit-plane
	// where the pixel is to be set and the bit
	// offset in that byte.
	//ByteOffset = (Y * VideoGetBytesPerScanLine()) + (X >> 3);
	//ByteOffset = (Y * 80) + (X >> 3);
	ByteOffset = (Y * 640) + X;
	BitInByte = ByteOffset & 7;
	ByteOffset = ByteOffset >> 3;
	//BitToChange = 0x80 >> BitInByte;
	BitToChange = 0x80;

	DbgPrint((DPRINT_WARNING, "X = %d Y = %d Color = %d ByteOffset = %d BitInByte = %d BitToChange = %d\n", X, Y, Color, ByteOffset, BitInByte, BitToChange));
	//getch();

	// Read the byte of memory to be changed. This is a
	// read for the video card latches and the data read
	// from memory does not need to be used.
	ReadByte = MemoryPointer[ByteOffset];

	// Select the bit or bits in the byte that need to be
	// changed through index 8 of the VGA card address
	// register by sending an 8 out to I/O port 3CEh.
	// Next get the bits to be changed (a one-bit represents
	// a bit to be changed) and send this out to I/O
	// port 3CFh, the bit mask register (BMR).
	//WRITE_PORT_USHORT((U16*)0x3CE, (((U16)BitToChange) << 8) + 0x08);
	WRITE_PORT_UCHAR((U8*)0x3CE, 0x08);
	//WRITE_PORT_UCHAR((U8*)0x3CF, BitToChange);
	WRITE_PORT_UCHAR((U8*)0x3CF, BitInByte);

	// Next set all mask bits to 1111 in the map mask register
	// (MMR) at sequencer offset 2, and write color 0 to the
	// VGA card to set the color to black. The mask bits select
	// the bit planes to be changed. If all are selected and a
	// color of 0 is written, all four bit-planes are clear to zero.
	WRITE_PORT_USHORT((U16*)0x3C4, 0x0F02);
	//WRITE_PORT_UCHAR((U8*)0x3C4, 0x02);
	//WRITE_PORT_UCHAR((U8*)0x3C5, 0x0F);		// Mask to 1111 binary
	MemoryPointer[ByteOffset] = 0x00;

	// Send the desired color number to the map mask register and
	// write an FFh to the video memory. This places a logic one
	// in only the selected bit planes to write a new color to
	// a pixel or dot on the screen.
	WRITE_PORT_UCHAR((U8*)0x3C4, 0x02);
	//WRITE_PORT_UCHAR((U8*)0x3C5, Color);
	WRITE_PORT_UCHAR((U8*)0x3C5, 0x0F);
	//WRITE_PORT_USHORT((U16*)0x3C4, 0x0A02);
	MemoryPointer[ByteOffset] = 0xFF;


	/*CurrentColor = Color;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	WRITE_PORT_USHORT((U16*)0x3CE, 0x00 | (CurrentColor << 8));
	WRITE_PORT_USHORT((U16*)0x3CE, 0x08 | 0x8000 >> (X & 7));

	MemoryPointer += (Y * VideoGetBytesPerScanLine()) + (X >> 3);

	*MemoryPointer = *MemoryPointer;
	getch();*/


	// First select the color plane
	//ColorPlane = Color;
	//ColorPlane = (ColorPlane << 8) + 0x02;
	//WRITE_PORT_USHORT((U16*)0x3C4, ColorPlane);

	// Now calculate the byte offset in the
	// color plane that contains our pixel
	// Since there are 8 pixels per byte we
	// have to adjust accordingly
	/*ByteOffset = (Y * VideoGetCurrentModeResolutionX()) + X;
	BitInByte = ByteOffset % 8;
	ByteOffset = ByteOffset / 8;

	// Shift the color to the right bit
	Color = 1;
	Color = Color << BitInByte;

	// Get the current color
	CurrentColor = MemoryPointer[ByteOffset];

	// Add the new color
	CurrentColor = CurrentColor | Color;

	// Now set the color
	MemoryPointer[ByteOffset] = CurrentColor;*/
}

VOID VideoSetPixel256(U32 X, U32 Y, U8 Color)
{
	U32		Bank;
	U32		Offset;
	U8*		MemoryPointer;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	Bank = VideoGetMemoryBankForPixel(X, Y);
	Offset = VideoGetBankOffsetForPixel(X, Y);

	VideoSetMemoryBank(Bank);

	MemoryPointer[Offset] = Color;
}

VOID VideoSetPixelRGB_15Bit(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Bank;
	U32		Offset;
	U8*		MemoryPointer;
	U16		Pixel;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	Bank = VideoGetMemoryBankForPixel(X, Y);
	Offset = VideoGetBankOffsetForPixel(X, Y);

	VideoSetMemoryBank(Bank);

	Red = Red >> 3;
	Green = Green >> 3;
	Blue = Blue >> 3;

	Pixel = Red << 11 | Green << 6 | Blue << 1;

	MemoryPointer[Offset] = Pixel & 0xFF;
	MemoryPointer[Offset+1] = Pixel >> 8;
}

VOID VideoSetPixelRGB_16Bit(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Bank;
	U32		Offset;
	U8*		MemoryPointer;
	U16		Pixel;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	Bank = VideoGetMemoryBankForPixel(X, Y);
	Offset = VideoGetBankOffsetForPixel(X, Y);

	VideoSetMemoryBank(Bank);

	Red = Red >> 3;
	Green = Green >> 2;
	Blue = Blue >> 3;

	Pixel = (U16)Red << 11 | (U16)Green << 5 | (U16)Blue << 0;

	MemoryPointer[Offset] = Pixel & 0xFF;
	MemoryPointer[Offset+1] = Pixel >> 8;
}

VOID VideoSetPixelRGB_24Bit(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Bank;
	U32		Offset;
	U8*		MemoryPointer;

	MemoryPointer = (U8*)(VIDEOVGA_MEM_ADDRESS);

	Bank = VideoGetMemoryBankForPixel(X, Y);
	Offset = VideoGetBankOffsetForPixel(X, Y);

	VideoSetMemoryBank(Bank);

	MemoryPointer[Offset] = Blue;
	MemoryPointer[Offset+1] = Green;
	MemoryPointer[Offset+2] = Red;
}

VOID VideoSetPixelRGB(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	if (VesaVideoModeInformation.BitsPerPixel >= 24)
	{
		VideoSetPixelRGB_24Bit(X, Y, Red, Green, Blue);
	}
	else if (VesaVideoModeInformation.BitsPerPixel >= 16)
	{
		// 16-bit color modes give green an extra bit (5:6:5)
		// 15-bit color modes have just 5:5:5 for R:G:B
		if (VesaVideoModeInformation.GreenMaskSize == 6)
		{
			VideoSetPixelRGB_16Bit(X, Y, Red, Green, Blue);
		}
		else
		{
			VideoSetPixelRGB_15Bit(X, Y, Red, Green, Blue);
		}
	}
	else
	{
		BugCheck((DPRINT_UI, "This function does not support %d bits per pixel!", VesaVideoModeInformation.BitsPerPixel));
	}
}

VOID VideoSetPixel16_OffScreen(U32 X, U32 Y, U8 Color)
{
}

VOID VideoSetPixel256_OffScreen(U32 X, U32 Y, U8 Color)
{
	U8*		MemoryPointer;

	MemoryPointer = (U8*)(VideoOffScreenBuffer + VideoGetOffScreenMemoryOffsetForPixel(X, Y));

	*MemoryPointer = Color;
}

VOID VideoSetPixelRGB_15Bit_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Offset;
	U8*		MemoryPointer;
	U16		Pixel;

	MemoryPointer = (U8*)(VideoOffScreenBuffer);
	Offset = VideoGetOffScreenMemoryOffsetForPixel(X, Y);

	Red = Red >> 3;
	Green = Green >> 3;
	Blue = Blue >> 3;

	Pixel = Red << 11 | Green << 6 | Blue << 1;

	MemoryPointer[Offset] = Pixel & 0xFF;
	MemoryPointer[Offset+1] = Pixel >> 8;
}

VOID VideoSetPixelRGB_16Bit_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Offset;
	U8*		MemoryPointer;
	U16		Pixel;

	MemoryPointer = (U8*)(VideoOffScreenBuffer);
	Offset = VideoGetOffScreenMemoryOffsetForPixel(X, Y);

	Red = Red >> 3;
	Green = Green >> 2;
	Blue = Blue >> 3;

	Pixel = (U16)Red << 11 | (U16)Green << 5 | (U16)Blue << 0;

	MemoryPointer[Offset] = Pixel & 0xFF;
	MemoryPointer[Offset+1] = Pixel >> 8;
}

VOID VideoSetPixelRGB_24Bit_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	U32		Offset;
	U8*		MemoryPointer;

	MemoryPointer = (U8*)(VideoOffScreenBuffer);
	Offset = VideoGetOffScreenMemoryOffsetForPixel(X, Y);

	MemoryPointer[Offset] = Blue;
	MemoryPointer[Offset+1] = Green;
	MemoryPointer[Offset+2] = Red;
}

VOID VideoSetPixelRGB_OffScreen(U32 X, U32 Y, U8 Red, U8 Green, U8 Blue)
{
	if (VesaVideoModeInformation.BitsPerPixel >= 24)
	{
		VideoSetPixelRGB_24Bit_OffScreen(X, Y, Red, Green, Blue);
	}
	else if (VesaVideoModeInformation.BitsPerPixel >= 16)
	{
		// 16-bit color modes give green an extra bit (5:6:5)
		// 15-bit color modes have just 5:5:5 for R:G:B
		if (VesaVideoModeInformation.GreenMaskSize == 6)
		{
			VideoSetPixelRGB_16Bit_OffScreen(X, Y, Red, Green, Blue);
		}
		else
		{
			VideoSetPixelRGB_15Bit_OffScreen(X, Y, Red, Green, Blue);
		}
	}
	else
	{
		BugCheck((DPRINT_UI, "This function does not support %d bits per pixel!", VesaVideoModeInformation.BitsPerPixel));
	}
}
