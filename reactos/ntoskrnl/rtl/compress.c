/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: compress.c,v 1.2 2003/07/11 01:23:15 royce Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Compression and decompression functions
 * FILE:              ntoskrnl/rtl/compress.c
 * PROGRAMER:         Eric Kohl (ekohl@zr-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>


/* MACROS *******************************************************************/

#define COMPRESSION_FORMAT_MASK  0x00FF
#define COMPRESSION_ENGINE_MASK  0xFF00


/* FUNCTIONS ****************************************************************/


static NTSTATUS
RtlpCompressBufferLZNT1(USHORT Engine,
			PUCHAR UncompressedBuffer,
			ULONG UncompressedBufferSize,
			PUCHAR CompressedBuffer,
			ULONG CompressedBufferSize,
			ULONG UncompressedChunkSize,
			PULONG FinalCompressedSize,
			PVOID WorkSpace)
{
  return(STATUS_NOT_IMPLEMENTED);
}


static NTSTATUS
RtlpWorkSpaceSizeLZNT1(USHORT Engine,
		       PULONG BufferAndWorkSpaceSize,
		       PULONG FragmentWorkSpaceSize)
{
  if (Engine == COMPRESSION_ENGINE_STANDARD)
    {
      *BufferAndWorkSpaceSize = 0x8010;
      *FragmentWorkSpaceSize = 0x1000;
      return(STATUS_SUCCESS);
    }
  else if (Engine == COMPRESSION_ENGINE_MAXIMUM)
    {
      *BufferAndWorkSpaceSize = 0x10;
      *FragmentWorkSpaceSize = 0x1000;
      return(STATUS_SUCCESS);
    }

  return(STATUS_NOT_SUPPORTED);
}




/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlCompressBuffer(IN USHORT CompressionFormatAndEngine,
		  IN PUCHAR UncompressedBuffer,
		  IN ULONG UncompressedBufferSize,
		  OUT PUCHAR CompressedBuffer,
		  IN ULONG CompressedBufferSize,
		  IN ULONG UncompressedChunkSize,
		  OUT PULONG FinalCompressedSize,
		  IN PVOID WorkSpace)
{
  USHORT Format = CompressionFormatAndEngine & COMPRESSION_FORMAT_MASK;
  USHORT Engine = CompressionFormatAndEngine & COMPRESSION_ENGINE_MASK;

  if ((Format == COMPRESSION_FORMAT_NONE) ||
      (Format == COMPRESSION_FORMAT_DEFAULT))
    return(STATUS_INVALID_PARAMETER);

  if (Format == COMPRESSION_FORMAT_LZNT1)
    return(RtlpCompressBufferLZNT1(Engine,
				   UncompressedBuffer,
				   UncompressedBufferSize,
				   CompressedBuffer,
				   CompressedBufferSize,
				   UncompressedChunkSize,
				   FinalCompressedSize,
				   WorkSpace));

  return(STATUS_UNSUPPORTED_COMPRESSION);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlCompressChunks(IN PUCHAR UncompressedBuffer,
		  IN ULONG UncompressedBufferSize,
		  OUT PUCHAR CompressedBuffer,
		  IN ULONG CompressedBufferSize,
		  IN OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
		  IN ULONG CompressedDataInfoLength,
		  IN PVOID WorkSpace)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDecompressBuffer(IN USHORT CompressionFormat,
		    OUT PUCHAR UncompressedBuffer,
		    IN ULONG UncompressedBufferSize,
		    IN PUCHAR CompressedBuffer,
		    IN ULONG CompressedBufferSize,
		    OUT PULONG FinalUncompressedSize)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDecompressChunks(OUT PUCHAR UncompressedBuffer,
		    IN ULONG UncompressedBufferSize,
		    IN PUCHAR CompressedBuffer,
		    IN ULONG CompressedBufferSize,
		    IN PUCHAR CompressedTail,
		    IN ULONG CompressedTailSize,
		    IN PCOMPRESSED_DATA_INFO CompressedDataInfo)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDecompressFragment(IN USHORT CompressionFormat,
		      OUT PUCHAR UncompressedFragment,
		      IN ULONG UncompressedFragmentSize,
		      IN PUCHAR CompressedBuffer,
		      IN ULONG CompressedBufferSize,
		      IN ULONG FragmentOffset,
		      OUT PULONG FinalUncompressedSize,
		      IN PVOID WorkSpace)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlDescribeChunk(IN USHORT CompressionFormat,
		 IN OUT PUCHAR *CompressedBuffer,
		 IN PUCHAR EndOfCompressedBufferPlus1,
		 OUT PUCHAR *ChunkBuffer,
		 OUT PULONG ChunkSize)
{
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlGetCompressionWorkSpaceSize(IN USHORT CompressionFormatAndEngine,
			       OUT PULONG CompressBufferAndWorkSpaceSize,
			       OUT PULONG CompressFragmentWorkSpaceSize)
{
  USHORT Format = CompressionFormatAndEngine & COMPRESSION_FORMAT_MASK;
  USHORT Engine = CompressionFormatAndEngine & COMPRESSION_ENGINE_MASK;

  if ((Format == COMPRESSION_FORMAT_NONE) ||
      (Format == COMPRESSION_FORMAT_DEFAULT))
    return(STATUS_INVALID_PARAMETER);

  if (Format == COMPRESSION_FORMAT_LZNT1)
    return(RtlpWorkSpaceSizeLZNT1(Engine,
				  CompressBufferAndWorkSpaceSize,
				  CompressFragmentWorkSpaceSize));

  return(STATUS_UNSUPPORTED_COMPRESSION);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
RtlReserveChunk(IN USHORT CompressionFormat,
		IN OUT PUCHAR *CompressedBuffer,
		IN PUCHAR EndOfCompressedBufferPlus1,
		OUT PUCHAR *ChunkBuffer,
		IN ULONG ChunkSize)
{
  return(STATUS_NOT_IMPLEMENTED);
}

/* EOF */
