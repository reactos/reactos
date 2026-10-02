// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  Defines CHalftone Object Class
//

// convert to 8bpp gray.

void FASTCALL Convert_32bppARGB_8Gray(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    );

// convert to 32bppARGB grayscale.

void FASTCALL Convert_32bppARGB_Grayscale(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    );

// convert from 2bpp gray to 32bpp.

void FASTCALL Convert_2Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    );

// convert from 4bpp gray to 32bpp.

void FASTCALL Convert_4Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    );

// convert from 8bpp gray to 32bpp.

void FASTCALL Convert_8Gray_32bppARGB(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    );


/**************************************************************************
*
* Description:
*
*    Routines for converting a bitmap from one pixel format to another.
*    Includes palette reduction, dithering, etc.
*
* Created:
*
*   09/25/2001 asecchia
*      Created it.
*
**************************************************************************/



VOID
ReadUnalignedScanline(
    __out_bcount_full((totalBits + 7) >> 3) BYTE* dst,
    __in_bcount(((totalBits + startBit) >> 3) + 1) const BYTE* src,
    UINT totalBits,
    __in_range(1, 7) UINT startBit
    );

VOID
WriteUnalignedScanline(
    __inout_bcount(((totalBits + startBit) >> 3) + 1) BYTE* dst,
    __in_bcount((totalBits + 7) >> 3) const BYTE* src,
    UINT totalBits,
    __in_range(1, 7) UINT startBit
    );





