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
#include <comm.h>
#include <rtl.h>
#include <debug.h>


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

VOID VideoCopyOffScreenBufferToVRAM(VOID)
{
	U32		BanksToCopy;
	U32		BytesInLastBank;
	U32		CurrentBank;
	U32		BankSize;
	U32		BufferSize;

	//VideoWaitForVerticalRetrace();

	// Text mode (BIOS or VESA)
	if (VideoGetCurrentModeType() == MODETYPE_TEXT)
	{
		BufferSize = VideoGetCurrentModeResolutionX() * VideoGetBytesPerScanLine();
		RtlCopyMemory((PVOID)VIDEOTEXT_MEM_ADDRESS, VideoOffScreenBuffer, BufferSize);
	}
	// VESA graphics mode
	else if (VideoGetCurrentModeType() == MODETYPE_GRAPHICS && VideoIsCurrentModeVesa())
	{
		BankSize = VesaVideoModeInformation.WindowGranularity << 10;
		BanksToCopy = (VesaVideoModeInformation.HeightInPixels * VesaVideoModeInformation.BytesPerScanLine) / BankSize;
		BytesInLastBank = (VesaVideoModeInformation.HeightInPixels * VesaVideoModeInformation.BytesPerScanLine) % BankSize;

		// Copy all the banks but the last one because
		// it is probably a partial bank
		for (CurrentBank=0; CurrentBank<BanksToCopy; CurrentBank++)
		{
			VideoSetMemoryBank(CurrentBank);
			RtlCopyMemory((PVOID)VIDEOVGA_MEM_ADDRESS, VideoOffScreenBuffer, BankSize);
		}

		// Copy the remaining bytes into the last bank
		VideoSetMemoryBank(CurrentBank);
		RtlCopyMemory((PVOID)VIDEOVGA_MEM_ADDRESS, VideoOffScreenBuffer, BytesInLastBank);
	}
	// BIOS graphics mode
	else
	{
		UNIMPLEMENTED();
	}
}
