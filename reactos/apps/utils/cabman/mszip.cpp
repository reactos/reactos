/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/mszip.cpp
 * PURPOSE:     CAB codec for MSZIP compressed data
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:       The ZLIB does the real work. Get the full version
 *              from http://www.cdrom.com/pub/infozip/zlib/
 * REVISIONS:
 *   CSH 21/03-2001 Created
 */
#include <windows.h>
#include <stdio.h>
#include "mszip.h"


/* Memory functions */

voidpf MSZipAlloc(voidpf opaque, uInt items, uInt size)
{
    DPRINT(DEBUG_MEMORY, ("items = (%d)  size = (%d)\n", items, size));
    return HeapAlloc(GetProcessHeap(), 0, items * size);
}

void MSZipFree (voidpf opaque, voidpf address)
{
    DPRINT(DEBUG_MEMORY, ("\n"));
    HeapFree(GetProcessHeap(), 0, address);
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


ULONG CMSZipCodec::Compress(PVOID OutputBuffer,
                            PVOID InputBuffer,
                            DWORD InputLength,
                            PDWORD OutputLength)
/*
 * FUNCTION: Compresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer   = Pointer to buffer to place compressed data
 *     InputBuffer    = Pointer to buffer with data to be compressed
 *     InputLength    = Length of input buffer
 *     OutputLength   = Address of buffer to place size of compressed data
 */
{
    PUSHORT Magic;

    DPRINT(MAX_TRACE, ("InputLength (%d).\n", InputLength));

    Magic  = (PUSHORT)OutputBuffer;
    *Magic = MSZIP_MAGIC;

    ZStream.next_in   = (PUCHAR)InputBuffer;
    ZStream.avail_in  = InputLength;
    ZStream.next_out  = (PUCHAR)((ULONG)OutputBuffer + 2);
    ZStream.avail_out = CAB_BLOCKSIZE + 12;

    /* WindowBits is passed < 0 to tell that there is no zlib header */
    Status = deflateInit2(&ZStream,
                          Z_BEST_COMPRESSION,
                          Z_DEFLATED,
                          -MAX_WBITS,
                          8, /* memLevel */
                          Z_DEFAULT_STRATEGY);
    if (Status != Z_OK) {
        DPRINT(MIN_TRACE, ("deflateInit() returned (%d).\n", Status));
        return CS_NOMEMORY;
    }

    Status = deflate(&ZStream, Z_FINISH);
    if ((Status != Z_OK) && (Status != Z_STREAM_END)) {
        DPRINT(MIN_TRACE, ("deflate() returned (%d) (%s).\n", Status, ZStream.msg));
        if (Status == Z_MEM_ERROR)
            return CS_NOMEMORY;
        return CS_BADSTREAM;
    }

    *OutputLength = ZStream.total_out + 2;

    Status = deflateEnd(&ZStream);
    if (Status != Z_OK) {
        DPRINT(MIN_TRACE, ("deflateEnd() returned (%d).\n", Status));
        return CS_BADSTREAM;
    }

    return CS_SUCCESS;
}


ULONG CMSZipCodec::Uncompress(PVOID OutputBuffer,
                              PVOID InputBuffer,
                              DWORD InputLength,
                              PDWORD OutputLength)
/*
 * FUNCTION: Uncompresses data in a buffer
 * ARGUMENTS:
 *     OutputBuffer = Pointer to buffer to place uncompressed data
 *     InputBuffer  = Pointer to buffer with data to be uncompressed
 *     InputLength  = Length of input buffer
 *     OutputLength = Address of buffer to place size of uncompressed data
 */
{
    USHORT Magic;

    DPRINT(MAX_TRACE, ("InputLength (%d).\n", InputLength));

    Magic = *((PUSHORT)InputBuffer);

    if (Magic != MSZIP_MAGIC) {
        DPRINT(MID_TRACE, ("Bad MSZIP block header magic (0x%X)\n", Magic));
        return CS_BADSTREAM;
    }
	
	ZStream.next_in   = (PUCHAR)((ULONG)InputBuffer + 2);
	ZStream.avail_in  = InputLength - 2;
	ZStream.next_out  = (PUCHAR)OutputBuffer;
    ZStream.avail_out = CAB_BLOCKSIZE + 12;

    /* WindowBits is passed < 0 to tell that there is no zlib header.
     * Note that in this case inflate *requires* an extra "dummy" byte
     * after the compressed stream in order to complete decompression and
     * return Z_STREAM_END.
     */
    Status = inflateInit2(&ZStream, -MAX_WBITS);
    if (Status != Z_OK) {
        DPRINT(MIN_TRACE, ("inflateInit2() returned (%d).\n", Status));
        return CS_BADSTREAM;
    }

    while ((ZStream.total_out < CAB_BLOCKSIZE + 12) &&
        (ZStream.total_in < InputLength - 2)) {
        Status = inflate(&ZStream, Z_NO_FLUSH);
        if (Status == Z_STREAM_END) break;
        if (Status != Z_OK) {
            DPRINT(MIN_TRACE, ("inflate() returned (%d) (%s).\n", Status, ZStream.msg));
            if (Status == Z_MEM_ERROR)
                return CS_NOMEMORY;
            return CS_BADSTREAM;
        }
    }

    *OutputLength = ZStream.total_out;

    Status = inflateEnd(&ZStream);
    if (Status != Z_OK) {
        DPRINT(MIN_TRACE, ("inflateEnd() returned (%d).\n", Status));
        return CS_BADSTREAM;
    }
    return CS_SUCCESS;
}

/* EOF */
