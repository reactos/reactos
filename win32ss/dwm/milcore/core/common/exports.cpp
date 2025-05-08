// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//-------------------------------------------------------------------------

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  MIL3DCalcProjected2DBounds
//
//  Synopsis:  Computes the 2D screen bounds of the CMilPointAndSize3F after
//             projecting with the current 3D world, view, and projection
//             transforms and clipping to the camera's Near and Far
//             planes.
//
//-------------------------------------------------------------------------

HRESULT WINAPI
MIL3DCalcProjected2DBounds(
    __in_ecount(1) const CMatrix<CoordinateSpace::Local3D,CoordinateSpace::PageInPixels> *pFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F *pboxBounds,
    __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> *prcTargetRect
    )
{
    HRESULT hr =  S_OK;

    CFloatFPU oGuard;

    if (pFullTransform3D == NULL || pboxBounds == NULL || prcTargetRect == NULL)
    {
        IFC(E_INVALIDARG);
    }

    CalcProjectedBounds(*pFullTransform3D, pboxBounds, prcTargetRect);

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
// Calculate a mask for the number of "bits to mask" at a "bit offset" from
// the left (high-order bit) of a single byte.
//
// Consider this example:
//
// bitOffset=3
// bitsToMask=3
//
// In memory, this is laid out as:
//
// -------------------------------------------------
// |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
// -------------------------------------------------
// <--- bitOffset --->
//                    <-- bitsToMask -->
//
// The general algorithm is to start with 0xFF, shift to the right such that
// only bitsToMask number of bits are left on, and then shift back to the
// left to align with the requested bitOffset.
//
// The result will be:
//
// -------------------------------------------------
// |  0  |  0  |  0  |  1  |  1  |  1  |  0  |  0  |
// -------------------------------------------------
//
//------------------------------------------------------------------------------

BYTE
GetOffsetMask(
    __in_range(0, 7)    UINT    bitOffset,
    __in_range(1, 8)    UINT    bitsToMask
    )
{
    Assert((bitOffset + bitsToMask) <= 8);

    BYTE mask = 0xFF;
    UINT maskShift = 8 - bitsToMask;

    mask = static_cast<BYTE>(mask >> maskShift);
    mask <<= (maskShift - bitOffset);

    return mask;
}

//+-----------------------------------------------------------------------------
//
// Return the next byte (or partial byte) from the input buffer starting at the
// specified bit offset and containing no more than the specified remaining
// bits to copy.  In the case of a partial byte, the results are left-aligned.
//
// Consider this example:
//
// inputBufferOffsetInBits=5
// bitsRemainingToCopy=4
//
// In memory, this is laid out as:
//
// ---------------------------------------------------------------
// |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |  7  |  6  ...
// ---------------------------------------------------------------
// <-- inputBufferOffsetInBits -->
//                                <- bitsRemainingToCopy->
//
// The result will be a single byte containing the 3 lower bits of the first
// byte plus the 1 upper bit of the second byte.
//
//------------------------------------------------------------------------------

BYTE
GetNextByteFromInputBuffer(
    __in_bcount(2)                  BYTE *  pInputBuffer,  // Some cases only require 1 byte...
    __in_range(0, 7)                UINT    inputBufferOffsetInBits,
    __in_range(1, UINT_MAX)         UINT    bitsRemainingToCopy
    )
{
    // bitsRemainingToCopy could be some huge number.  We only care about the
    // next byte's worth.
    if (bitsRemainingToCopy > 8) bitsRemainingToCopy = 8;

    if (inputBufferOffsetInBits == 0)
    {
        BYTE mask = GetOffsetMask(0, bitsRemainingToCopy);
        return pInputBuffer[0] & mask;
    }

    BYTE nextByte = 0;
    UINT bitsFromFirstByte = 8 - inputBufferOffsetInBits;

    // Read from the first byte.  The results are left-aligned.
    BYTE mask = GetOffsetMask(inputBufferOffsetInBits, bitsFromFirstByte);
    nextByte = pInputBuffer[0] & mask;
    nextByte <<= inputBufferOffsetInBits;

    bitsRemainingToCopy -= bitsFromFirstByte;

    // Read from the second byte, if needed
    if (bitsRemainingToCopy > 0)
    {
        // Note: bitsRemainingToCopy and bitsFromFirstByte are both small
        // numbers, so addition is safe from overflow.
        if (bitsRemainingToCopy + bitsFromFirstByte == 8)
        {
            // This is a common case where we are reading 8 bits of data
            // straddled across a byte boundary.  We can simply invert the
            // mask from the first byte for the second byte.
            mask = ~mask;
        }
        else
        {
            mask = GetOffsetMask(0, bitsRemainingToCopy);
        }
        nextByte |= static_cast<BYTE>(pInputBuffer[1] & mask) >> bitsFromFirstByte;
    }

    return nextByte;
}

//+-----------------------------------------------------------------------------
//
// Copies bytes and partial bytes from the input buffer to the output buffer.
// This function handles the case where the bit offsets are different for the
// input and output buffers.
//
//------------------------------------------------------------------------------

VOID
CopyUnalignedPixelBuffer(
    __out_bcount(outputBufferSize)  BYTE *  pOutputBuffer,
    __in_range(1, UINT_MAX)         UINT    outputBufferSize,
    __in_range(1, UINT_MAX)         UINT    outputBufferStride,
    __in_range(0, 7)                UINT    outputBufferOffsetInBits,
    __in_bcount(inputBufferSize)    BYTE *  pInputBuffer,
    __in_range(1, UINT_MAX)         UINT    inputBufferSize,
    __in_range(1, UINT_MAX)         UINT    inputBufferStride,
    __in_range(0, 7)                UINT    inputBufferOffsetInBits,
    __in_range(1, UINT_MAX)         UINT    height,
    __in_range(1, UINT_MAX)         UINT    copyWidthInBits
    )
{
    for (UINT row = 0; row < height; row++)
    {
        UINT    bitsRemaining =     copyWidthInBits;
        BYTE *  pInputPosition =    pInputBuffer;
        BYTE *  pOutputPosition =   pOutputBuffer;

        while (bitsRemaining > 0)
        {
            BYTE nextByte = GetNextByteFromInputBuffer(
                pInputPosition,
                inputBufferOffsetInBits,
                bitsRemaining);

            if (bitsRemaining >= 8)
            {
                if (outputBufferOffsetInBits == 0)
                {
                    // The output buffer is at a byte boundary, so we can just
                    // write the next byte.
                    pOutputPosition[0] = nextByte;
                }
                else
                {
                    // The output buffer has a bit-offset, so the next byte
                    // will straddle two bytes.
                    UINT bitsCopiedToFirstByte = 8 - outputBufferOffsetInBits;

                    // Write to the first byte...
                    BYTE mask = GetOffsetMask(outputBufferOffsetInBits, bitsCopiedToFirstByte);
                    pOutputPosition[0] = (pOutputPosition[0] & ~mask) | (static_cast<BYTE>(nextByte >> outputBufferOffsetInBits) & mask);

                    // Write to the second byte...
                    pOutputPosition[1] = static_cast<BYTE>((pOutputPosition[1] & mask) | ((nextByte << bitsCopiedToFirstByte) & ~mask));
                }

                bitsRemaining -= 8;
            }
            else
            {
                // Note: by the time we get to this condition, both bitsRemaining and
                // outputBufferOffsetInBits are both small numbers, making them safe
                // from overflow.
                UINT relativeOffsetOfLastBit = outputBufferOffsetInBits + bitsRemaining;

                if (relativeOffsetOfLastBit <= 8)
                {
                    // The remaining bits fit inside a single byte.
                    BYTE mask = GetOffsetMask(outputBufferOffsetInBits, bitsRemaining);
                    pOutputPosition[0] = (pOutputPosition[0] & ~mask) | (static_cast<BYTE>(nextByte >> outputBufferOffsetInBits) & mask);
                }
                else
                {
                    // The remaining bits will cross a byte boundary.
                    UINT bitsCopiedToFirstByte = 8 - outputBufferOffsetInBits;

                    // Write to the first byte...
                    BYTE mask = GetOffsetMask(outputBufferOffsetInBits, bitsCopiedToFirstByte);
                    pOutputPosition[0] = (pOutputPosition[0] & ~mask) | (static_cast<BYTE>(nextByte >> outputBufferOffsetInBits) & mask);

                    // Write to the second byte...
                    mask = GetOffsetMask(0, bitsRemaining - bitsCopiedToFirstByte);
                    pOutputPosition[1] = static_cast<BYTE>((pOutputPosition[1] & ~mask) | ((nextByte << bitsCopiedToFirstByte) & mask));
                }

                bitsRemaining = 0;
            }

            pOutputPosition++;
            pInputPosition++;
        }

        pOutputBuffer += outputBufferStride;
        pInputBuffer += inputBufferStride;
    }
}

//+-----------------------------------------------------------------------------
//
//  MilUtility_CopyPixelBuffer
//
//  This function copies memory from the input buffer to the output buffer,
//  with explicit support for sub-byte pixel formats.  Generally speaking, this
//  functions treats memory as 2D (width*height).  However, the width of the
//  buffer often differs from the natural width of the pixels (width *
//  bits-per-pixel, converted to bytes), due to memory alignment requirements.
//  The actual distance between adjacent rows is known as the stride, and this
//  is always specified in bytes.
//
//  The buffers are therefore specified by a pointer, a size, and a stride.
//  As usual, the size and stride are specified in bytes.
//
//  However, the requested area to copy is specified in bits.  This includes
//  bit offsets into both the input and output buffers, as well as number of
//  bits to copy for each row.  The number of rows to copy is specified as the
//  height.  The bit offsets must only specify the offset within the first
//  byte (they must range from 0 to 7, inclusive).  The buffer pointers should
//  be adjusted before calling this method if the bit offset is large.
//
//------------------------------------------------------------------------------

HRESULT WINAPI
MilUtility_CopyPixelBuffer(
    __out_bcount(outputBufferSize) BYTE* pOutputBuffer,
    UINT outputBufferSize,
    UINT outputBufferStride,
    UINT outputBufferOffsetInBits,
    __in_bcount(inputBufferSize) BYTE* pInputBuffer,
    UINT inputBufferSize,
    UINT inputBufferStride,
    UINT inputBufferOffsetInBits,
    UINT height,
    UINT copyWidthInBits
    )
{
    HRESULT hr = S_OK;

    if (height == 0 || copyWidthInBits == 0)
    {
        // Nothing to do.
        goto Cleanup;
    }

    if (outputBufferOffsetInBits > 7 || inputBufferOffsetInBits > 7)
    {
        // Bit offsets should be 0..7, inclusive.
        IFC(E_INVALIDARG);
    }

    UINT minimumOutputBufferStrideInBits = 0;
    IFC(UIntAdd(outputBufferOffsetInBits, copyWidthInBits, &minimumOutputBufferStrideInBits));
    UINT minimumOutputBufferStride = BitsToBytes(minimumOutputBufferStrideInBits);
    if (outputBufferStride < minimumOutputBufferStride)
    {
        // The stride of the output buffer is too small.
        IFC(E_INVALIDARG);
    }

    UINT minimumOutputBufferSize = 0;
    IFC(UIntMult(outputBufferStride, (height-1), &minimumOutputBufferSize));
    IFC(UIntAdd(minimumOutputBufferSize, minimumOutputBufferStride, &minimumOutputBufferSize));
    if (outputBufferSize < minimumOutputBufferSize)
    {
        // The output buffer is too small.
        IFC(E_INVALIDARG);
    }

    UINT minimumInputBufferStrideInBits = 0;
    IFC(UIntAdd(inputBufferOffsetInBits, copyWidthInBits, &minimumInputBufferStrideInBits));
    UINT minimumInputBufferStride = BitsToBytes(minimumInputBufferStrideInBits);
    if (inputBufferStride < minimumInputBufferStride)
    {
        // The stride of the input buffer is too small.
        IFC(E_INVALIDARG);
    }

    UINT minimumInputBufferSize = 0;
    IFC(UIntMult(inputBufferStride, (height-1), &minimumInputBufferSize));
    IFC(UIntAdd(minimumInputBufferSize, minimumInputBufferStride, &minimumInputBufferSize));
    if (inputBufferSize < minimumInputBufferSize)
    {
        // The input buffer is too small.
        IFC(E_INVALIDARG);
    }

    if (outputBufferOffsetInBits != inputBufferOffsetInBits)
    {
        CopyUnalignedPixelBuffer(
            pOutputBuffer,
            outputBufferSize,
            outputBufferStride,
            outputBufferOffsetInBits,
            pInputBuffer,
            inputBufferSize,
            inputBufferStride,
            inputBufferOffsetInBits,
            height,
            copyWidthInBits);
    }
    else
    {
        // Since the input and output offsets are the same, we use more general
        // variable names here.
        UINT minimumBufferStrideInBits = minimumInputBufferStrideInBits;
        UINT minimumBufferStride = minimumInputBufferStride;
        UINT bufferOffsetInBits = inputBufferOffsetInBits;
        UINT finalByteOffset = minimumBufferStride - 1;

        if ((bufferOffsetInBits == 0) && (copyWidthInBits % 8 == 0))
        {
            if (minimumBufferStride == inputBufferStride &&
                inputBufferStride == outputBufferStride)
            {
                // Fast-Path
                // The input and output buffers are on byte boundaries, both
                // share the same stride, and we are copying the entire
                // stride of each row.  These conditions allow us to do a
                // single large memory copy instead of multiple copies per row.
                memcpy(pOutputBuffer, pInputBuffer, inputBufferStride*height);
            }
            else
            {
                // We are copying whole bytes from the input buffer to the
                // output buffer, but we still need to copy row-by-row since
                // the width is not the same as the stride.
                for (UINT i = 0; i < height; i++)
                {
                    memcpy(pOutputBuffer, pInputBuffer, minimumBufferStride);

                    pOutputBuffer += outputBufferStride;
                    pInputBuffer += inputBufferStride;
                }
            }
        }
        else if (finalByteOffset == 0)
        {
            // If the first byte is also the final byte, we need to double mask.
            BYTE mask = GetOffsetMask(bufferOffsetInBits, copyWidthInBits);

            for (UINT i = 0; i < height; i++)
            {
                pOutputBuffer[0] = (pOutputBuffer[0] & ~mask) | (pInputBuffer[0] & mask);

                pOutputBuffer += outputBufferStride;
                pInputBuffer += inputBufferStride;
            }
        }
        else
        {
            // In this final case:
            //   - only part of either the first or last byte should be copied,
            //   - the first byte is not the last byte,
            //   - and there are between 0 and n whole bytes in between to copy.
            bool firstByteIsWhole = bufferOffsetInBits == 0;
            bool finalByteIsWhole = minimumBufferStrideInBits % 8 == 0;
            UINT wholeBytesPerRow = minimumBufferStride - (finalByteIsWhole ? 0 : 1) - (firstByteIsWhole ? 0 : 1);

            for (UINT i = 0; i < height; i++)
            {
                if (!firstByteIsWhole)
                {
                    BYTE mask = GetOffsetMask(bufferOffsetInBits, 8 - bufferOffsetInBits);
                    pOutputBuffer[0] = (pOutputBuffer[0] & ~mask) | (pInputBuffer[0] & mask);
                }

                if (wholeBytesPerRow > 0)
                {
                    UINT firstByteOffset = (firstByteIsWhole ? 0 : 1);
                    memcpy(
                        pOutputBuffer + firstByteOffset,
                        pInputBuffer + firstByteOffset,
                        wholeBytesPerRow);
                }

                if (!finalByteIsWhole)
                {
                    BYTE mask = GetOffsetMask(0, minimumBufferStrideInBits % 8);
                    pOutputBuffer[finalByteOffset] = (pOutputBuffer[finalByteOffset] & ~mask) | (pInputBuffer[finalByteOffset] & mask);
                }

                pOutputBuffer += outputBufferStride;
                pInputBuffer += inputBufferStride;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

