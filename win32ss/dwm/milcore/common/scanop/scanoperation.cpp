// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Definitions for the ScanOperation namespace.
//
//  Notes:
//
//      We make use of these subsets of the pixel formats:
//        * "Indexed" vs. "Non-indexed" - indexed formats use a color palette.
//
//        * "Interchange" - 32bppARGB, 128bppABGR and 64bppARGB. When
//          converting from one format to another, if more than one step is
//          needed, we go through one or more of these interchange formats.
//
//          These interchange formats do not use premultiplied alpha, because
//          it's incorrect to perform gamma-conversion directly on PARGB data.
//          (However, since conversion between 128bppPABGR and 64bppPARGB
//          wouldn't need gamma-conversion, we should consider adding those as
//          possible interchange formats, if we continue to support 64bpp.)
//
//        * "Rendering" - the rasterizer can render directly into a surface of
//          this format.
//

#include "precomp.hpp"

BOOL IsInterchangeFormat(MilPixelFormat::Enum fmt)
{
    switch (fmt)
    {
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::RGBA64bpp:
        return TRUE;
    }
    return FALSE;
}

MilPixelFormat::Enum GetNearestInterchangeFormat(MilPixelFormat::Enum fmt)
{
    MilPixelFormat::Enum fmtRet;

    switch (fmt)
    {
    case MilPixelFormat::RGB48bppFixedPoint:
    case MilPixelFormat::RGBA64bpp:
    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::Gray16bppFixedPoint:
    case MilPixelFormat::CMYK32bpp:
    case MilPixelFormat::RGB48bpp:        
        fmtRet = MilPixelFormat::RGBA64bpp;
        break;

    case MilPixelFormat::Gray32bppFloat:
    case MilPixelFormat::RGB128bppFloat:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
    case MilPixelFormat::Gray16bpp:
    case MilPixelFormat::BGR32bpp101010:
        fmtRet = MilPixelFormat::RGBA128bppFloat;
        break;

    case MilPixelFormat::Indexed1bpp:
    case MilPixelFormat::Indexed2bpp:
    case MilPixelFormat::Indexed4bpp:
    case MilPixelFormat::Indexed8bpp:
    case MilPixelFormat::BlackWhite:
    case MilPixelFormat::Gray2bpp:
    case MilPixelFormat::Gray4bpp:
    case MilPixelFormat::Gray8bpp:
    case MilPixelFormat::BGR16bpp555:
    case MilPixelFormat::BGR16bpp565:
    case MilPixelFormat::BGR24bpp:
    case MilPixelFormat::RGB24bpp:
    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::PBGRA32bpp:
        fmtRet = MilPixelFormat::BGRA32bpp;
        break;

    default:
        AssertMsg(FALSE, "Unexpected pixel format");
        fmtRet = MilPixelFormat::BGRA32bpp;
        break;
    }

    Assert(IsInterchangeFormat(fmtRet));
    return fmtRet;
}

//+-----------------------------------------------------------------------------
//
//  Function:  GetOp_SrcOver_or_SrcOverAL
//
//  Synopsis:
//
//      Return a special-case SrcOver or SrcOverAL operation (if one exists),
//      which blends directly to a given non-halftoned destination format (with
//      the source in 32bppPARGB or 128bppPABGR).
//
//  Return value:
//
//      NULL if no operation exists for the given destination format.
//
//  Notes:
//
//      The 555/565 cases handle both dithering and non-dithering, selected via
//      pPP->m_fDither16bpp.
//
//      Returns NULL for 32bppARGB and 128bppABGR, because we don't have an
//      operation which includes the necessary AlphaDivide step after the blend.
//
//      For 32bppRGB and 128bppBGR destinations, we use the same function as for
//      32bppPARGB and 128bppPABGR. This is okay because the destination alpha
//      doesn't affect other channels in a SrcOver operation.
//
//------------------------------------------------------------------------------

ScanOpFunc GetOp_SrcOver_or_SrcOverAL(
    MilPixelFormat::Enum fmtDest,        // Non-indexed destination format.
    MilPixelFormat::Enum fmtColorData    // Source format, either 32bppPARGB or
                                   // 128bppPABGR. If 32bppPARGB: Return op is a
                                   // SrcOverAL. If 128bppPABGR: Return op is a
                                   // SrcOver.
    )
{
    Assert(!IsIndexedPixelFormat(fmtDest));

    // If this switch statement is expensive, we could instead look only at the
    // lower 16 bits (and assert that the upper 16 are PIXELFORMAT_MIL).
    //
    // Likewise for the other GetOp_* statements, and the pixel format util
    // functions.

    switch (fmtColorData)
    {
    case MilPixelFormat::PBGRA32bpp:
        switch (fmtDest)
        {
        case MilPixelFormat::BGR16bpp555:
            return CCPUInfo::HasMMX() ?
                SrcOverAL_32bppPARGB_555_MMX :
                SrcOverAL_32bppPARGB_555;

        case MilPixelFormat::BGR16bpp565:
            return CCPUInfo::HasMMX() ?
                SrcOverAL_32bppPARGB_565_MMX :
                SrcOverAL_32bppPARGB_565;

        case MilPixelFormat::BGR24bpp:
            return SrcOverAL_32bppPARGB_24;

        case MilPixelFormat::BGR32bpp:    // See Notes above
        case MilPixelFormat::PBGRA32bpp:
            if (CCPUInfo::HasSSE2())    
            {
                return SrcOverAL_32bppPARGB_32bppPARGB_SSE2;
            }
            else
            {
                return CCPUInfo::HasMMX() ? 
                    SrcOverAL_32bppPARGB_32bppPARGB_MMX :
                    SrcOverAL_32bppPARGB_32bppPARGB;
            }

        case MilPixelFormat::RGB24bpp:
            return SrcOverAL_32bppPARGB_24BGR;

        default:
            return NULL;
        }
        break;
        
    case MilPixelFormat::BGR32bpp:
        switch (fmtDest)
        {
        case MilPixelFormat::BGR32bpp:    // See Notes above
            return SrcOver_32bppRGB_32bppRGB;
            
        case MilPixelFormat::PBGRA32bpp:
            return SrcOver_32bppRGB_32bppPARGB;

        default:
            return NULL;
        }
        break;

    case MilPixelFormat::PRGBA128bppFloat:
        switch (fmtDest)
        {
        case MilPixelFormat::RGB128bppFloat:   // See Notes above
        case MilPixelFormat::PRGBA128bppFloat:
            if (CCPUInfo::HasSSE2())    
            {
                return SrcOver_128bppPABGR_128bppPABGR_SSE2;
            }
            else
            {
                return SrcOver_128bppPABGR_128bppPABGR;
            }

        default:
            return NULL;
        }
        break;

#if NEVER
    // If we supported 64bpp color data, we'd include this (or put it in a
    // separate GetOp function)

    case MilPixelFormat::PRGBA64bpp:
        switch (fmtDest)
        {
            case MilPixelFormat::PRGBA64bpp:
                return CCPUInfo::HasMMX() ?
                    SrcOver_64bppPARGB_64bppPARGB_MMX :
                    SrcOver_64bppPARGB_64bppPARGB;
        }
        break;
#endif // NEVER

    default:
        AssertMsg(FALSE, "Unexpected pixel format");
        return NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:  GetOp_ConvertFormat_InterchangeToNonHalftoned
//
//  Synopsis:
//
//      Returns an operation which converts from the closest interchange format
//      to the given non-halftoned, non-interchange format.
//
//  Notes:
//
//   The 555/565 cases handle both dithering and non-dithering, selected via
//   OtherParams::DoingDither.
//
//------------------------------------------------------------------------------

ScanOpFunc GetOp_ConvertFormat_InterchangeToNonHalftoned(
    MilPixelFormat::Enum fmt  // A non-halftoned, non-interchange destination format.
    )
{
    Assert(!IsIndexedPixelFormat(fmt));
    Assert(GetNearestInterchangeFormat(fmt) != fmt); // Caller should handle this case

    ScanOpFunc pfnRet;
    switch (fmt)
    {
    //
    // Nearest interchange format: 32bppARGB
    //

    // case 1555:
    // return Quantize_32bppARGB_1555;

    case MilPixelFormat::Gray8bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_32bppARGB_8Gray;
        break;

    case MilPixelFormat::BGR16bpp555:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = CCPUInfo::HasMMX() ?
            Dither_32bppARGB_555_MMX :
            Dither_32bppARGB_555;
        break;

    case MilPixelFormat::BGR16bpp565:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = CCPUInfo::HasMMX() ?
            Dither_32bppARGB_565_MMX :
            Dither_32bppARGB_565;
        break;

    case MilPixelFormat::BGR24bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Quantize_32bppARGB_24;
        break;

    case MilPixelFormat::BGR32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        // We could spec this to be a NOP. But this way could be considered more consistent.
        // (and it's up to higher-level code to NOP this out when it would make no difference.)

        pfnRet = Quantize_32bppARGB_32RGB;
        break;

    case MilPixelFormat::PBGRA32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = AlphaMultiply_32bppARGB;
        break;

    case MilPixelFormat::RGB24bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Quantize_32bppARGB_24BGR;
        break;

    //
    // Nearest interchange format: 64bppARGB
    //

    case MilPixelFormat::RGB48bppFixedPoint:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Quantize_64bppARGB_48;
        break;

    case MilPixelFormat::PRGBA64bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = AlphaMultiply_64bppARGB;
        break;

    case MilPixelFormat::Gray16bppFixedPoint:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Quantize_64bppARGB_16bppGray;
        break;

    case MilPixelFormat::CMYK32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Quantize_64bppARGB_32bppCMYK;
        break;

    case MilPixelFormat::RGB48bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Convert_64bppARGB_48bppRGB;
        break;

    //
    // Nearest interchange format: 128bppABGR
    //

    case MilPixelFormat::RGB128bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        // We could spec this to be a NOP. But this way could be considered more consistent.
        // (and it's up to higher-level code to NOP this out when it would make no difference.)

        pfnRet = Quantize_128bppABGR_128RGB;
        break;

    case MilPixelFormat::PRGBA128bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = AlphaMultiply_128bppABGR;
        break;

    case MilPixelFormat::Gray16bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = GammaConvert_128bppABGR_16bppGrayInt;
        break;

    case MilPixelFormat::Gray32bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = GammaConvert_128bppABGR_32bppGrayFloat;
        break;

    case MilPixelFormat::BGR32bpp101010:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = Quantize_128bppABGR_32bppRGB101010;
        break;

        
    default:
        pfnRet = NULL;
    }

    return pfnRet;
}

//+-----------------------------------------------------------------------------
//
//  Function:  GetOp_ConvertFormat_ToInterchange
//
//  Synopsis:
//
//      Returns an operation which converts from a non-interchange format, to
//      the closest interchange format.
//
//  Notes:
//
//      If the source format is indexed, the scan operation will need palette
//      information - setting that up is the caller's responsibility.
//
//------------------------------------------------------------------------------

ScanOpFunc GetOp_ConvertFormat_ToInterchange(
    MilPixelFormat::Enum fmt  // A non-interchange source format.
    )
{
    Assert(fmt != GetNearestInterchangeFormat(fmt)); // Caller should handle this case

    ScanOpFunc pfnRet;
    switch (fmt)
    {
    //
    // Nearest interchange format: 32bppARGB
    //

    // case 1555:
    // return Convert_1555_32bppARGB;

    case MilPixelFormat::BlackWhite:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_1BW_32bppARGB;
        break;

    case MilPixelFormat::Indexed1bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_1_32bppARGB;
        break;

    case MilPixelFormat::Indexed2bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_2_32bppARGB;
        break;

    case MilPixelFormat::Gray2bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_2Gray_32bppARGB;
        break;

    case MilPixelFormat::Gray4bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_4Gray_32bppARGB;
        break;

    case MilPixelFormat::Indexed4bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_4_32bppARGB;
        break;

    case MilPixelFormat::Indexed8bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_8_32bppARGB;
        break;

    case MilPixelFormat::Gray8bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_8Gray_32bppARGB;
        break;

    case MilPixelFormat::BGR16bpp555:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_555_32bppARGB;
        break;

    case MilPixelFormat::BGR16bpp565:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_565_32bppARGB;
        break;

    case MilPixelFormat::BGR24bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_24_32bppARGB;
        break;

    case MilPixelFormat::BGR32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_32RGB_32bppARGB;
        break;

    case MilPixelFormat::PBGRA32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = AlphaDivide_32bppPARGB;
        break;

    case MilPixelFormat::RGB24bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::BGRA32bpp);
        pfnRet = Convert_24BGR_32bppARGB;
        break;

    //
    // Nearest interchange format: 64bppARGB
    //

    case MilPixelFormat::RGB48bppFixedPoint:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Convert_48_64bppARGB;
        break;

    case MilPixelFormat::PRGBA64bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = AlphaDivide_64bppPARGB;
        break;

    case MilPixelFormat::Gray16bppFixedPoint:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Convert_16bppGray_64bppARGB;
        break;

    case MilPixelFormat::CMYK32bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Convert_32bppCMYK_64bppARGB;
        break;

    case MilPixelFormat::RGB48bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA64bpp);
        pfnRet = Convert_48bppRGB_64bppARGB;
        break;

    //
    // Nearest interchange format: 128bppABGR
    //
    case MilPixelFormat::Gray32bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = GammaConvert_32bppGrayFloat_128bppABGR;
        break;

    case MilPixelFormat::RGB128bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = Convert_128RGB_128bppABGR;
        break;

    case MilPixelFormat::PRGBA128bppFloat:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = AlphaDivide_128bppPABGR;
        break;

    case MilPixelFormat::Gray16bpp:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = GammaConvert_16bppGrayInt_128bppABGR;
        break;

    case MilPixelFormat::BGR32bpp101010:
        Assert(GetNearestInterchangeFormat(fmt) == MilPixelFormat::RGBA128bppFloat);
        pfnRet = Convert_32bppRGB101010_128bppABGR;
        break;

        
    default:
        pfnRet = NULL;
    }

    return pfnRet;
}

//+-----------------------------------------------------------------------------
//
//  Function:  GetOp_Copy
//
//  Synopsis:
//
//      Returns an operation which copies data of the given format.
//
//  Notes:
//
//      If the source format is indexed, this only works if the destination
//      palette is the same as the source palette. Caller is responsible for
//      ensuring this.
//
//------------------------------------------------------------------------------

ScanOpFunc GetOp_Copy(
    MilPixelFormat::Enum fmt  // The format to copy.
    )
{
    ScanOpFunc pfnRet;
    switch (fmt)
    {
    // case 1555:
    // return Copy_16;

    // case MilPixelFormat::Indexed2bpp:  // Unsupported

    case MilPixelFormat::BlackWhite:
    case MilPixelFormat::Indexed1bpp:
        pfnRet = Copy_1;
        break;

    case MilPixelFormat::Indexed4bpp:
        pfnRet = Copy_4;
        break;

    case MilPixelFormat::Indexed8bpp:
    case MilPixelFormat::Gray8bpp:
        pfnRet = Copy_8;
        break;

    case MilPixelFormat::BGR16bpp555:
    case MilPixelFormat::BGR16bpp565:
        pfnRet = Copy_16;
        break;

    case MilPixelFormat::BGR24bpp:
    case MilPixelFormat::RGB24bpp:
        pfnRet = Copy_24;
        break;

    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::CMYK32bpp:
    case MilPixelFormat::Gray32bppFloat:
    case MilPixelFormat::BGR32bpp101010:
        pfnRet = Copy_32;
        break;

    case MilPixelFormat::RGB48bpp:
        pfnRet = Copy_48;
        break;

    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::RGBA64bpp:
        pfnRet = Copy_64;
        break;

    case MilPixelFormat::RGB128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
    case MilPixelFormat::RGBA128bppFloat:
        pfnRet = Copy_128;
        break;

    default:
        pfnRet = NULL;
    }

    return pfnRet;
}






