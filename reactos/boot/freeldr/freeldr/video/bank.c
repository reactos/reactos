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

#if 0 /* This stuff isn't used and as far as I'm concerned it can go - GvG */
U32		CurrentMemoryBank = 0;

VOID VideoSetMemoryBank(U16 BankNumber)
{
	if (CurrentMemoryBank != BankNumber)
	{
		BiosVesaSetBank(BankNumber);
		CurrentMemoryBank = BankNumber;
	}
}

U32 VideoGetMemoryBankForPixel(U32 X, U32 Y)
{
	U32		Bank;
	U32		MemoryPos;
	U32		BankSize;
	U32		BytesPerPixel;

	//BytesPerPixel = ROUND_UP(VesaVideoModeInformation.BitsPerPixel, 8) / 8;
	BytesPerPixel = (VesaVideoModeInformation.BitsPerPixel + 7) >> 3;
	MemoryPos = (Y * VideoGetBytesPerScanLine()) + (X * BytesPerPixel);
	//BankSize = VesaVideoModeInformation.WindowGranularity * 1024;
	BankSize = VesaVideoModeInformation.WindowGranularity << 10;
	Bank = MemoryPos / BankSize;

	return Bank;
}

U32 VideoGetMemoryBankForPixel16(U32 X, U32 Y)
{
	U32		Bank;
	U32		MemoryPos;
	U32		BankSize;

	MemoryPos = (Y * VideoGetBytesPerScanLine()) + (X / 2);
	//BankSize = VesaVideoModeInformation.WindowGranularity * 1024;
	BankSize = VesaVideoModeInformation.WindowGranularity << 10;
	Bank = MemoryPos / BankSize;

	return Bank;
}

U32 VideoGetBankOffsetForPixel(U32 X, U32 Y)
{
	U32		BankOffset;
	U32		MemoryPos;
	U32		BankSize;
	U32		BytesPerPixel;

	//BytesPerPixel = ROUND_UP(VesaVideoModeInformation.BitsPerPixel, 8) / 8;
	BytesPerPixel = (VesaVideoModeInformation.BitsPerPixel + 7) >> 3;
	MemoryPos = (Y * VideoGetBytesPerScanLine()) + (X * BytesPerPixel);
	//BankSize = VesaVideoModeInformation.WindowGranularity * 1024;
	BankSize = VesaVideoModeInformation.WindowGranularity << 10;
	BankOffset = MemoryPos % BankSize;

	return BankOffset;
}

U32 VideoGetBankOffsetForPixel16(U32 X, U32 Y)
{
	U32		BankOffset;
	U32		MemoryPos;
	U32		BankSize;

	MemoryPos = (Y * VideoGetBytesPerScanLine()) + (X / 2);
	//BankSize = VesaVideoModeInformation.WindowGranularity * 1024;
	BankSize = VesaVideoModeInformation.WindowGranularity << 10;
	BankOffset = MemoryPos % BankSize;

	return BankOffset;
}

U32 VideoGetOffScreenMemoryOffsetForPixel(U32 X, U32 Y)
{
	U32		MemoryPos;
	U32		BytesPerPixel;

	//BytesPerPixel = ROUND_UP(VesaVideoModeInformation.BitsPerPixel, 8) / 8;
	BytesPerPixel = (VesaVideoModeInformation.BitsPerPixel + 7) >> 3;
	MemoryPos = (Y * VesaVideoModeInformation.BytesPerScanLine) + (X * BytesPerPixel);

	return MemoryPos;
}
#endif

VOID VideoCopyOffScreenBufferToVRAM(VOID)
{
	MachVideoCopyOffScreenBufferToVRAM(VideoOffScreenBuffer);
}
