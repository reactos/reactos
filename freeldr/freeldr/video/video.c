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
#include <comm.h>
#include <mm.h>


PVOID	VideoOffScreenBuffer = NULL;

VOID VideoClearScreen(VOID)
{
	VideoSetMode(VideoGetCurrentMode());
}

VOID VideoWaitForVerticalRetrace(VOID)
{

	while ((READ_PORT_UCHAR((U8*)VIDEOPORT_VERTICAL_RETRACE) & 0x08) == 1)
	{
		// Keep reading the port until bit 3 is clear
		// This waits for the current retrace to end and
		// we can catch the next one so we know we are
		// getting a full retrace.
	}

	while ((READ_PORT_UCHAR((U8*)VIDEOPORT_VERTICAL_RETRACE) & 0x08) == 0)
	{
		// Keep reading the port until bit 3 is set
		// Now that we know we aren't doing a vertical
		// retrace we need to wait for the next one.
	}
}

PVOID VideoAllocateOffScreenBuffer(VOID)
{
	U32		BufferSize;

	if (VideoOffScreenBuffer != NULL)
	{
		MmFreeMemory(VideoOffScreenBuffer);
		VideoOffScreenBuffer = NULL;
	}

	BufferSize = VideoGetCurrentModeResolutionX() * VideoGetBytesPerScanLine();

	VideoOffScreenBuffer = MmAllocateMemory(BufferSize);

	return VideoOffScreenBuffer;
}
