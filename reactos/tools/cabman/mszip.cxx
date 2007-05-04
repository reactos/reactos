/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/mszip.cpp
 * PURPOSE:     CAB codec for MSZIP compressed data
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Colin Finck <mail@colinfinck.de>
 * NOTES:       The ZLIB does the real work. Get the full version
 *              from http://www.cdrom.com/pub/infozip/zlib/
 * REVISIONS:
 *   CSH 21/03-2001 Created
 *   CSH 15/08-2003 Made it portable
 *   CF  04/05-2007 Reformatted the code to be more consistent and use TABs instead of spaces
 */
#include <stdio.h>
#include "mszip.h"


/* Memory functions */

voidpf MSZipAlloc(voidpf opaque, uInt items, uInt size)
{
	DPRINT(DEBUG_MEMORY, ("items = (%d)  size = (%d)\n", items, size));
	return AllocateMemory(items * size);
}

void MSZipFree (voidpf opaque, voidpf address)
{
	DPRINT(DEBUG_MEMORY, ("\n"));
	FreeMemory(address);
}


/* CMSZipCodec */

CMSZipCodec::CMSZipCodec()
/*
 * FUNCTION: Default constructor
 */
{
	ZStream.zalloc = MSZipAlloc;
	ZStream.zfree  = MSZipFree;
	ZStream.opaque = (voidpf)0;
}


CMSZipCodec::~CMSZipCodec()
/*
 * FUNCTION: Default destructor
 */
{
}


unsigned long CMSZipCodec::Compress(void* OutputBuffer,
                            void* InputBuffer,
                            unsigned long InputLength,
                            unsigned long* OutputLength)
/*
 * FUNCTION: Compresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer   = Pointer to buffer to place compressed data
 *     InputBuffer    = Pointer to buffer with data to be compressed
 *     InputLength    = Length of input buffer
 *     OutputLength   = Address of buffer to place size of compressed data
 */
{
	unsigned short* Magic;

	DPRINT(MAX_TRACE, ("InputLength (%lu).\n", InputLength));

	Magic  = (unsigned short*)OutputBuffer;
	*Magic = MSZIP_MAGIC;

	ZStream.next_in   = (unsigned char*)InputBuffer;
	ZStream.avail_in  = InputLength;
	ZStream.next_out  = (unsigned char*)((unsigned long)OutputBuffer + 2);
	ZStream.avail_out = CAB_BLOCKSIZE + 12;

	/* WindowBits is passed < 0 to tell that there is no zlib header */
	Status = deflateInit2(&ZStream,
						  Z_BEST_COMPRESSION,
						  Z_DEFLATED,
						  -MAX_WBITS,
						  8, /* memLevel */
						  Z_DEFAULT_STRATEGY);
	if (Status != Z_OK)
	{
		DPRINT(MIN_TRACE, ("deflateInit() returned (%d).\n", Status));
		return CS_NOMEMORY;
	}

	Status = deflate(&ZStream, Z_FINISH);
	if ((Status != Z_OK) && (Status != Z_STREAM_END))
	{
		DPRINT(MIN_TRACE, ("deflate() returned (%d) (%s).\n", Status, ZStream.msg));
		if (Status == Z_MEM_ERROR)
			return CS_NOMEMORY;
		return CS_BADSTREAM;
	}

	*OutputLength = ZStream.total_out + 2;

	Status = deflateEnd(&ZStream);
	if (Status != Z_OK)
	{
		DPRINT(MIN_TRACE, ("deflateEnd() returned (%d).\n", Status));
		return CS_BADSTREAM;
	}

	return CS_SUCCESS;
}


unsigned long CMSZipCodec::Uncompress(void* OutputBuffer,
                              void* InputBuffer,
                              unsigned long InputLength,
                              unsigned long* OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of uncompressed data
 */
{
	unsigned short Magic;

	DPRINT(MAX_TRACE, ("InputLength (%lu).\n", InputLength));

	Magic = *((unsigned short*)InputBuffer);

	if (Magic != MSZIP_MAGIC)
	{
		DPRINT(MID_TRACE, ("Bad MSZIP block header magic (0x%X)\n", Magic));
		return CS_BADSTREAM;
	}

	ZStream.next_in   = (unsigned char*)((unsigned long)InputBuffer + 2);
	ZStream.avail_in  = InputLength - 2;
	ZStream.next_out  = (unsigned char*)OutputBuffer;
	ZStream.avail_out = CAB_BLOCKSIZE + 12;

	/* WindowBits is passed < 0 to tell that there is no zlib header.
	 * Note that in this case inflate *requires* an extra "dummy" byte
	 * after the compressed stream in order to complete decompression and
	 * return Z_STREAM_END.
	 */
	Status = inflateInit2(&ZStream, -MAX_WBITS);
	if (Status != Z_OK)
	{
		DPRINT(MIN_TRACE, ("inflateInit2() returned (%d).\n", Status));
		return CS_BADSTREAM;
	}

	while ((ZStream.total_out < CAB_BLOCKSIZE + 12) &&
		(ZStream.total_in < InputLength - 2))
	{
		Status = inflate(&ZStream, Z_NO_FLUSH);
		if (Status == Z_STREAM_END) break;
		if (Status != Z_OK)
		{
			DPRINT(MIN_TRACE, ("inflate() returned (%d) (%s).\n", Status, ZStream.msg));
			if (Status == Z_MEM_ERROR)
				return CS_NOMEMORY;
			return CS_BADSTREAM;
		}
	}

	*OutputLength = ZStream.total_out;

	Status = inflateEnd(&ZStream);
	if (Status != Z_OK)
	{
		DPRINT(MIN_TRACE, ("inflateEnd() returned (%d).\n", Status));
		return CS_BADSTREAM;
	}
	return CS_SUCCESS;
}

/* EOF */
