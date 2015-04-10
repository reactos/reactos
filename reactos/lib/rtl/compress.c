/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Compression and decompression functions
 * FILE:            lib/rtl/compress.c
 * PROGRAMER:       Eric Kohl
                    Sebastian Lackner
                    Michael Müller
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* MACROS *******************************************************************/

#define COMPRESSION_FORMAT_MASK  0x00FF
#define COMPRESSION_ENGINE_MASK  0xFF00




/* FUNCTIONS ****************************************************************/

/* Based on Wine Staging */

/* decompress a single LZNT1 chunk */
static PUCHAR lznt1_decompress_chunk(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size)
{
    UCHAR *src_cur, *src_end, *dst_cur, *dst_end;
    ULONG displacement_bits, length_bits;
    ULONG code_displacement, code_length;
    WORD flags, code;

    src_cur = src;
    src_end = src + src_size;
    dst_cur = dst;
    dst_end = dst + dst_size;

    /* Partial decompression is no error on Windows. */
    while (src_cur < src_end && dst_cur < dst_end)
    {
        /* read flags header */
        flags = 0x8000 | *src_cur++;

        /* parse following 8 entities, either uncompressed data or backwards reference */
        while ((flags & 0xFF00) && src_cur < src_end)
        {
            if (flags & 1)
            {
                /* backwards reference */
                if (src_cur + sizeof(WORD) > src_end)
                    return NULL;
                code = *(WORD *)src_cur;
                src_cur += sizeof(WORD);

                /* find length / displacement bits */
                for (displacement_bits = 12; displacement_bits > 4; displacement_bits--)
                    if ((1 << (displacement_bits - 1)) < dst_cur - dst) break;
                length_bits       = 16 - displacement_bits;
                code_length       = (code & ((1 << length_bits) - 1)) + 3;
                code_displacement = (code >> length_bits) + 1;

                /* ensure reference is valid */
                if (dst_cur < dst + code_displacement)
                    return NULL;

                /* copy bytes of chunk - we can't use memcpy()
                 * since source and dest can be overlapping */
                while (code_length--)
                {
                    if (dst_cur >= dst_end) return dst_cur;
                    *dst_cur = *(dst_cur - code_displacement);
                    dst_cur++;
                }
            }
            else
            {
                /* uncompressed data */
                if (dst_cur >= dst_end) return dst_cur;
                *dst_cur++ = *src_cur++;
            }
            flags >>= 1;
        }

    }

    return dst_cur;
}

/* decompress data encoded with LZNT1 */
static NTSTATUS lznt1_decompress(UCHAR *dst, ULONG dst_size, UCHAR *src, ULONG src_size,
                                 ULONG offset, ULONG *final_size, UCHAR *workspace)
{
    UCHAR *src_cur, *src_end, *dst_cur, *dst_end, *ptr;
    ULONG chunk_size, block_size;
    WORD chunk_header;

    src_cur = src;
    src_end = src + src_size;
    dst_cur = dst;
    dst_end = dst + dst_size;

    if (src_cur + sizeof(WCHAR) > src_end)
        return STATUS_BAD_COMPRESSION_BUFFER;

    /* skip over chunks which have a big distance (>= 0x1000) to the destination offset */
    while (offset >= 0x1000 && src_cur + sizeof(WCHAR) <= src_end)
    {
        /* read chunk header and extract size */
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WCHAR);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xFFF) + 1;

        /* ensure we have enough buffer to process chunk */
        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        src_cur += chunk_size;
        offset  -= 0x1000;
    }

    /* this chunk is can be included partially */
    if (offset && src_cur + sizeof(WCHAR) <= src_end)
    {
        /* read chunk header and extract size */
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WCHAR);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xFFF) + 1;

        /* ensure we have enough buffer to process chunk */
        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            if (!workspace) return STATUS_ACCESS_VIOLATION;
            ptr = lznt1_decompress_chunk(workspace, 0x1000, src_cur, chunk_size);
            if (!ptr) return STATUS_BAD_COMPRESSION_BUFFER;
            if (ptr - workspace > offset)
            {
                block_size = min((ptr - workspace) - offset, dst_end - dst_cur);
                memcpy(dst_cur, workspace + offset, block_size);
                dst_cur += block_size;
            }
        }
        else
        {
            /* uncompressed chunk */
            if (chunk_size > offset)
            {
                block_size = min(chunk_size - offset, dst_end - dst_cur);
                memcpy(dst_cur, src_cur + offset, block_size);
                dst_cur += block_size;
            }
        }

        src_cur += chunk_size;
    }

    while (src_cur + sizeof(WCHAR) <= src_end)
    {
        /* read chunk header and extract size */
        chunk_header = *(WORD *)src_cur;
        src_cur += sizeof(WCHAR);
        if (!chunk_header) goto out;
        chunk_size = (chunk_header & 0xFFF) + 1;

        /* ensure we have enough buffer to process chunk */
        if (src_cur + chunk_size > src_end)
            return STATUS_BAD_COMPRESSION_BUFFER;

        /* add padding if required */
        block_size = ((dst_cur - dst) + offset) & 0xFFF;
        if (block_size)
        {
            block_size = 0x1000 - block_size;
            if (dst_cur + block_size >= dst_end)
                goto out;
            memset(dst_cur, 0, block_size);
            dst_cur += block_size;
        }
        else if (dst_cur >= dst_end)
            goto out;

        if (chunk_header & 0x8000)
        {
            /* compressed chunk */
            dst_cur = lznt1_decompress_chunk(dst_cur, dst_end - dst_cur, src_cur, chunk_size);
            if (!dst_cur) return STATUS_BAD_COMPRESSION_BUFFER;
        }
        else
        {
            /* uncompressed chunk */
            block_size = min(chunk_size, dst_end - dst_cur);
            memcpy(dst_cur, src_cur, block_size);
            dst_cur += block_size;
        }

        src_cur += chunk_size;
    }

out:
    if (final_size)
        *final_size = dst_cur - dst;

    return STATUS_SUCCESS;

}


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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
 * @implemented
 */
NTSTATUS NTAPI
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
NTSTATUS NTAPI
RtlCompressChunks(IN PUCHAR UncompressedBuffer,
                  IN ULONG UncompressedBufferSize,
                  OUT PUCHAR CompressedBuffer,
                  IN ULONG CompressedBufferSize,
                  IN OUT PCOMPRESSED_DATA_INFO CompressedDataInfo,
                  IN ULONG CompressedDataInfoLength,
                  IN PVOID WorkSpace)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlDecompressChunks(OUT PUCHAR UncompressedBuffer,
                    IN ULONG UncompressedBufferSize,
                    IN PUCHAR CompressedBuffer,
                    IN ULONG CompressedBufferSize,
                    IN PUCHAR CompressedTail,
                    IN ULONG CompressedTailSize,
                    IN PCOMPRESSED_DATA_INFO CompressedDataInfo)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlDecompressFragment(IN USHORT format,
                      OUT PUCHAR uncompressed,
                      IN ULONG uncompressed_size,
                      IN PUCHAR compressed,
                      IN ULONG compressed_size,
                      IN ULONG offset,
                      OUT PULONG final_size,
                      IN PVOID workspace)
{
    DPRINT("0x%04x, %p, %u, %p, %u, %u, %p, %p :stub\n", format, uncompressed,
           uncompressed_size, compressed, compressed_size, offset, final_size, workspace);

    switch (format & ~COMPRESSION_ENGINE_MAXIMUM)
    {
        case COMPRESSION_FORMAT_LZNT1:
            return lznt1_decompress(uncompressed, uncompressed_size, compressed,
                                    compressed_size, offset, final_size, workspace);

        case COMPRESSION_FORMAT_NONE:
        case COMPRESSION_FORMAT_DEFAULT:
            return STATUS_INVALID_PARAMETER;

        default:
            DPRINT1("format %d not implemented\n", format);
            return STATUS_UNSUPPORTED_COMPRESSION;
    }
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlDecompressBuffer(IN USHORT CompressionFormat,
                    OUT PUCHAR UncompressedBuffer,
                    IN ULONG UncompressedBufferSize,
                    IN PUCHAR CompressedBuffer,
                    IN ULONG CompressedBufferSize,
                    OUT PULONG FinalUncompressedSize)
{
    return RtlDecompressFragment(CompressionFormat, UncompressedBuffer, UncompressedBufferSize,
                                 CompressedBuffer, CompressedBufferSize, 0, FinalUncompressedSize, NULL);
}

/*
 * @unimplemented
 */
NTSTATUS NTAPI
RtlDescribeChunk(IN USHORT CompressionFormat,
                 IN OUT PUCHAR *CompressedBuffer,
                 IN PUCHAR EndOfCompressedBufferPlus1,
                 OUT PUCHAR *ChunkBuffer,
                 OUT PULONG ChunkSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS NTAPI
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
NTSTATUS NTAPI
RtlReserveChunk(IN USHORT CompressionFormat,
                IN OUT PUCHAR *CompressedBuffer,
                IN PUCHAR EndOfCompressedBufferPlus1,
                OUT PUCHAR *ChunkBuffer,
                IN ULONG ChunkSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
