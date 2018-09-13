/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mrcf.c

Abstract:

    This module implements the Mrcf compression engine.

Author:

    Gary Kimura     [GaryKi]    21-Jan-1994

Revision History:

--*/

#include "ntrtlp.h"

#include <stdio.h>


//
//  To decompress/compress a block of data the user needs to
//  provide a work space as an extra parameter to all the exported
//  procedures.  That way the routines will not need to use excessive
//  stack space and will still be multithread safe
//

//
//  Variables for reading and writing bits
//

typedef struct _MRCF_BIT_IO {

    USHORT  abitsBB;        //  16-bit buffer being read
    LONG    cbitsBB;        //  Number of bits left in abitsBB

    PUCHAR  pbBB;           //  Pointer to byte stream being read
    ULONG   cbBB;           //  Number of bytes left in pbBB
    ULONG   cbBBInitial;    //  Initial size of pbBB

} MRCF_BIT_IO;
typedef MRCF_BIT_IO *PMRCF_BIT_IO;

//
//  Maximum back-pointer value, also used to indicate end of compressed stream!
//

#define wBACKPOINTERMAX                  (4415)

//
//  MDSIGNATURE - Signature at start of each compressed block
//
//      This 4-byte signature is used as a check to ensure that we
//      are decompressing data we compressed, and also to indicate
//      which compression method was used.
//
//      NOTE: A compressed block consists of one or more "chunks", separated
//            by the bitsEND_OF_STREAM pattern.
//
//            Byte          Word
//        -----------    ---------
//         0  1  2  3      0    1      Meaning
//        -- -- -- --    ---- ----     ----------------
//        44 53 00 01    5344 0100     MaxCompression
//        44 53 00 02    5344 0200     StandardCompression
//
//      NOTE: The *WORD* values are listed to be clear about the
//            byte ordering!
//

typedef struct _MDSIGNATURE {

    //
    //  Must be MD_STAMP
    //

    USHORT sigStamp;

    //
    //  mdsSTANDARD or mdsMAX
    //

    USHORT sigType;

} MDSIGNATURE;
typedef MDSIGNATURE *PMDSIGNATURE;

#define MD_STAMP        0x5344  // Signature stamp at start of compressed blk
#define MASK_VALID_mds  0x0300  // All other bits must be zero


//
//  Local procedure declarations and macros
//

#define minimum(a,b) (a < b ? a : b)

//
//  Local procedure prototypes
//

VOID
MrcfSetBitBuffer (
    PUCHAR pb,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    );

VOID
MrcfFillBitBuffer (
    PMRCF_BIT_IO BitIo
    );

USHORT
MrcfReadBit (
    PMRCF_BIT_IO BitIo
    );

USHORT
MrcfReadNBits (
    LONG cbits,
    PMRCF_BIT_IO BitIo
    );


NTSTATUS
RtlDecompressBufferMrcf (
    OUT PUCHAR UncompressedBuffer,
    IN ULONG UncompressedBufferSize,
    IN PUCHAR CompressedBuffer,
    IN ULONG CompressedBufferSize,
    OUT PULONG FinalUncompressedSize
    )

/*++

Routine Description:

    This routine decompresses a buffer of StandardCompressed or MaxCompressed
    data.

Arguments:

    UncompressedBuffer - buffer to receive uncompressed data

    UncompressedBufferSize - length of UncompressedBuffer

          NOTE: UncompressedBufferSize must be the EXACT length of the uncompressed
                data, as Decompress uses this information to detect
                when decompression is complete.  If this value is
                incorrect, Decompress may crash!

    CompressedBuffer - buffer containing compressed data

    CompressedBufferSize - length of CompressedBuffer

    WorkSpace - pointer to a private work area for use by this operation

Return Value:

    ULONG - Returns the size of the decompressed data in bytes. Returns 0 if
        there was an error in the decompress.

--*/

{
    MRCF_BIT_IO WorkSpace;

    ULONG  cbMatch; //  Length of match string
    ULONG  i;       //  Index in UncompressedBuffer to receive decoded data
    ULONG  iMatch;  //  Index in UncompressedBuffer of matched string
    ULONG  k;       //  Number of bits in length string
    ULONG  off;     //  Offset from i in UncompressedBuffer of match string
    USHORT x;       //  Current bit being examined
    ULONG  y;

    //
    //  verify that compressed data starts with proper signature
    //

    if (CompressedBufferSize < sizeof(MDSIGNATURE) ||                            // Must have signature
        ((PMDSIGNATURE)CompressedBuffer)->sigStamp != MD_STAMP ||            // Stamp must be OK
        ((PMDSIGNATURE)CompressedBuffer)->sigType & (~MASK_VALID_mds)) {     // Type must be OK

        *FinalUncompressedSize = 0;
        return STATUS_BAD_COMPRESSION_BUFFER;
    }

    //
    //  Skip over the valid signature
    //

    CompressedBufferSize -= sizeof(MDSIGNATURE);
    CompressedBuffer += sizeof(MDSIGNATURE);

    //
    //  Set up for decompress, start filling UncompressedBuffer at front
    //

    i = 0;

    //
    //  Set statics to save parm passing
    //

    MrcfSetBitBuffer(CompressedBuffer,CompressedBufferSize,&WorkSpace);

    while (TRUE) {

        y = MrcfReadNBits(2,&WorkSpace);

        //
        //  Check if next 7 bits are a byte
        //  1 if 128..255 (0x80..0xff), 2 if 0..127 (0x00..0x7f)
        //

        if (y == 1 || y == 2) {

            ASSERTMSG("Don't exceed expected length ", i<UncompressedBufferSize);

            UncompressedBuffer[i] = (UCHAR)((y == 1 ? 0x80 : 0) | MrcfReadNBits(7,&WorkSpace));

            i++;

        } else {

            //
            //  Have match sequence
            //

            //
            // Get the offset
            //

            if (y == 0) {

                //
                //  next 6 bits are offset
                //

                off = MrcfReadNBits(6,&WorkSpace);

                ASSERTMSG("offset 0 is invalid ", off != 0);

            } else {

                x = MrcfReadBit(&WorkSpace);

                if (x == 0) {

                    //
                    //  next 8 bits are offset-64 (0x40)
                    //

                    off = MrcfReadNBits(8, &WorkSpace) + 64;

                } else {

                    //
                    //  next 12 bits are offset-320 (0x140)
                    //

                    off = MrcfReadNBits(12, &WorkSpace) + 320;

                    if (off == wBACKPOINTERMAX) {

                        //
                        //  EOS marker
                        //

                        if (i >= UncompressedBufferSize) {

                            //
                            // Done with entire buffer
                            //

                            *FinalUncompressedSize = i;
                            return STATUS_SUCCESS;

                        } else {

                            //
                            //  More to do
                            //  Done with a 512-byte chunk
                            //

                            continue;
                        }
                    }
                }
            }

            ASSERTMSG("Don't exceed expected length ", i<UncompressedBufferSize);
            ASSERTMSG("Cannot match before start of uncoded buffer! ", off <= i);

            //
            //  Get the length  - logarithmically encoded
            //

            for (k=0; (x=MrcfReadBit(&WorkSpace)) == 0; k++) { NOTHING; }

            ASSERT(k <= 8);

            if (k == 0) {

                //
                //  All matches at least 2 chars long
                //

                cbMatch = 2;

            } else {

                cbMatch = (1 << k) + 1 + MrcfReadNBits(k, &WorkSpace);
            }

            ASSERTMSG("Don't exceed buffer size ", (i - off + cbMatch - 1) <= UncompressedBufferSize);

            //
            //  Copy the matched string
            //

            iMatch = i - off;

            while ( (cbMatch > 0) && (i<UncompressedBufferSize) ) {

                UncompressedBuffer[i++] = UncompressedBuffer[iMatch++];
                cbMatch--;
            }

            ASSERTMSG("Should have copied it all ", cbMatch == 0);
        }
    }
}


//
//  Internal Support Routine
//

VOID
MrcfSetBitBuffer (
    PUCHAR pb,
    ULONG cb,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Set statics with coded buffer pointer and length

Arguments:

    pb - pointer to compressed data buffer

    cb - length of compressed data buffer

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    None.

--*/

{
    BitIo->pbBB        = pb;
    BitIo->cbBB        = cb;
    BitIo->cbBBInitial = cb;
    BitIo->cbitsBB     = 0;
    BitIo->abitsBB     = 0;
}


//
//  Internal Support Routine
//

USHORT
MrcfReadBit (
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Get next bit from bit buffer

Arguments:

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    USHORT - Returns next bit (0 or 1)

--*/

{
    USHORT bit;

    //
    //  Check if no bits available
    //

    if ((BitIo->cbitsBB) == 0) {

        MrcfFillBitBuffer(BitIo);
    }

    //
    //  Decrement the bit count
    //  get the bit, remove it, and return the bit
    //

    (BitIo->cbitsBB)--;
    bit = (BitIo->abitsBB) & 1;
    (BitIo->abitsBB) >>= 1;

    return bit;
}


//
//  Internal Support Routine
//

USHORT
MrcfReadNBits (
    LONG cbits,
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Get next N bits from bit buffer

Arguments:

    cbits - count of bits to get

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    USHORT - Returns next cbits bits.

--*/

{
    ULONG abits;        // Bits to return
    LONG cbitsPart;    // Partial count of bits
    ULONG cshift;       // Shift count
    ULONG mask;         // Mask

    //
    //  Largest number of bits we should read at one time is 12 bits for
    //  a 12-bit offset.  The largest length field component that we
    //  read is 8 bits.  If this routine were used for some other purpose,
    //  it can support up to 15 (NOT 16) bit reads, due to how the masking
    //  code works.
    //

    ASSERT(cbits <= 12);

    //
    //  No shift and no bits yet
    //

    cshift = 0;
    abits = 0;

    while (cbits > 0) {

        //
        //  If not bits available get some bits
        //

        if ((BitIo->cbitsBB) == 0) {

            MrcfFillBitBuffer(BitIo);
        }

        //
        //  Number of bits we can read
        //

        cbitsPart = minimum((BitIo->cbitsBB), cbits);

        //
        //  Mask for bits we want, extract and store them
        //

        mask = (1 << cbitsPart) - 1;
        abits |= ((BitIo->abitsBB) & mask) << cshift;

        //
        //  Remember the next chunk of bits
        //

        cshift = cbitsPart;

        //
        //  Update bit buffer, move remaining bits down and
        //  update count of bits left
        //

        (BitIo->abitsBB) >>= cbitsPart;
        (BitIo->cbitsBB) -= cbitsPart;

        //
        //  Update count of bits left to read
        //

        cbits -= cbitsPart;
    }

    //
    //  Return requested bits
    //

    return (USHORT)abits;
}


//
//  Internal Support Routine
//

VOID
MrcfFillBitBuffer (
    PMRCF_BIT_IO BitIo
    )

/*++

Routine Description:

    Fill abitsBB from static bit buffer

Arguments:

    BitIo - Supplies a pointer to the bit buffer statics

Return Value:

    None.

--*/

{
    ASSERT((BitIo->cbitsBB) == 0);

    switch (BitIo->cbBB) {

    case 0:

        ASSERTMSG("no bits left in coded buffer!", FALSE);

        break;

    case 1:

        //
        //  Get last byte and adjust count
        //

        BitIo->cbitsBB = 8;
        BitIo->abitsBB = *(BitIo->pbBB)++;
        BitIo->cbBB--;

        break;

    default:

        //
        //  Get word and adjust count
        //

        BitIo->cbitsBB = 16;
        BitIo->abitsBB = *((USHORT *)(BitIo->pbBB))++;
        BitIo->cbBB -= 2;

        break;
    }
}

