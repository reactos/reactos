// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Pixel utility & conversion functions for various formats.
//
//------------------------------------------------------------------------------


#pragma once

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE   8
#endif

#pragma warning(push)
// Disable explicit handling of every enum type in a switch because
// we want to fall through deliberately on many pixel format types
#pragma warning(disable:4061)

//
//
// Pixel format type definitions
//
//

// We use two related color spaces:
enum ColorSpace {
    CS_sRGB,    // sRGB is our "legacy" standard.

    CS_scRGB    // scRGB is an improvement - similar to sRGB, but with linear
                // gamma and extended range.
};

//
// Lookup tables defined in pixelformatutils.cpp
//

extern const ARGB UnpremultiplyTable[];

//+-----------------------------------------------------------------------
//
//  Class:       CMilColorF
//
//  Synopsis:    An "energized" version of MilColorF, currently just adding
//               simple  construction.
//
//------------------------------------------------------------------------
class CMilColorF : public MilColorF
{
public:

    // Constructors
    CMilColorF()
    {
        // We require that you can typecast between CMilColorF and MilColorF.
        // To achieve this, CMilColorF must have no data members or virtual functions.

        Assert( sizeof(MilColorF) == sizeof(CMilColorF) );
        r = 0.0;
        g = 0.0;
        b = 0.0;
        a = 0.0;
    }

    CMilColorF(IN FLOAT r1, IN FLOAT g1, IN FLOAT b1, IN FLOAT a1 )
    {
        r = r1;
        g = g1;
        b = b1;
        a = a1;
    }
};

//
//
// Function prototypes
//
//

BOOL HasAlphaChannel(MilPixelFormat::Enum fmt);
BOOL HasAlphaChannel(REFWICPixelFormatGUID fmt);

BOOL IsPremultipliedFormOf(MilPixelFormat::Enum fmt1, MilPixelFormat::Enum fmt2);
BOOL IsNoAlphaFormOf(MilPixelFormat::Enum fmt1, MilPixelFormat::Enum fmt2);

D3DFORMAT
PixelFormatToD3DFormat(
    MilPixelFormat::Enum pixelFormat
    );

MilPixelFormat::Enum
D3DFormatToPixelFormat(
    D3DFORMAT d3dFmt,
    BOOL bPremultiplied
    );

ARGB Unpremultiply(ARGB argb);
ARGB Premultiply(ARGB argb);

void Unpremultiply(__inout_ecount(1) MilColorF *pColor);
void Premultiply(__inout_ecount(1) MilColorF *pColor);

UINT D3DFormatSize(D3DFORMAT d3dFmt);

//
//
// Inline pixel format functions
//
//

//+---------------------------------------------------------------------------
//
//  Function:   GetPixelFormatSize
//
//  Synopsis:   Returns the number of bits required for a single pixel
//              of the given format.
//
//  Returns:    Bit count of the pixel format
//
//----------------------------------------------------------------------------
inline BYTE
GetPixelFormatSize(
    MilPixelFormat::Enum fmt  // Pixel format
    )
{
    switch (fmt)
    {
    case MilPixelFormat::Indexed1bpp:
    case MilPixelFormat::BlackWhite:
        return 1;

    case MilPixelFormat::Indexed2bpp:
    case MilPixelFormat::Gray2bpp:
        return 2;

    case MilPixelFormat::Indexed4bpp:
    case MilPixelFormat::Gray4bpp:
        return 4;

    case MilPixelFormat::Indexed8bpp:
    case MilPixelFormat::Gray8bpp:
        return 8;

    case MilPixelFormat::BGR16bpp555:
    case MilPixelFormat::BGR16bpp565:
    case MilPixelFormat::Gray16bppFixedPoint:
    case MilPixelFormat::Gray16bpp:
        return 16;

    case MilPixelFormat::BGR24bpp:
    case MilPixelFormat::RGB24bpp:
        return 24;

    case MilPixelFormat::Gray32bppFloat:
    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::CMYK32bpp:
    case MilPixelFormat::BGR32bpp101010:
        return 32;

    case MilPixelFormat::CMYKAlpha40bpp:
        return 40;

    case MilPixelFormat::RGB48bpp:
    case MilPixelFormat::RGB48bppFixedPoint:
        return 48;

    case MilPixelFormat::RGBA64bpp:
    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::RGBA64bppFixedPoint:
    case MilPixelFormat::CMYK64bpp:
        return 64;

    case MilPixelFormat::CMYKAlpha80bpp:
        return 80;

    case MilPixelFormat::BGR96bppFixedPoint:
        return 96;

    case MilPixelFormat::RGB128bppFloat:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
    case MilPixelFormat::RGBA128bppFixedPoint:
        return 128;

    default:
        AssertMsg(FALSE, "Unsupported pixel format");
        return 0;
    }
}

inline BYTE
GetPixelFormatSize(
    REFWICPixelFormatGUID fmt  // Pixel format
    )
{
    HRESULT hr;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);

    if(FAILED(hr))
    {
        Assert (0);
        return 0;
    }

    return GetPixelFormatSize(pf);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValidPixelFormat
//
//  Synopsis:   Validates that an input DWORD is valid MilPixelFormat::Enum value
//
//  Returns:    Byte count of the pixel format
//
//----------------------------------------------------------------------------
inline BOOL
IsValidPixelFormat(
    MilPixelFormat::Enum fmt  // Pixel format
    )
{
    switch (fmt)
    {
    case MilPixelFormat::RGB48bppFixedPoint:
    case MilPixelFormat::RGB48bpp:
    case MilPixelFormat::RGBA64bpp:
    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::CMYK64bpp:
    case MilPixelFormat::Gray32bppFloat:
    case MilPixelFormat::CMYKAlpha80bpp:
    case MilPixelFormat::RGB128bppFloat:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
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
    case MilPixelFormat::Gray16bpp:
    case MilPixelFormat::Gray16bppFixedPoint:
    case MilPixelFormat::BGR24bpp:
    case MilPixelFormat::RGB24bpp:
    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::CMYK32bpp:
    case MilPixelFormat::CMYKAlpha40bpp:
    case MilPixelFormat::BGR32bpp101010:
    case MilPixelFormat::BGR96bppFixedPoint:
        return TRUE;

    default:
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   IsIndexedPixelFormat
//
//  Synopsis:   Returns whether or not the values of the pixel format
//              are indicies into a color palette.
//
//  Returns:    TRUE if this is an indexed pixel format, FALSE otherwise
//
//----------------------------------------------------------------------------
inline BOOL
IsIndexedPixelFormat(
    MilPixelFormat::Enum fmt // Pixel format
    )
{
    switch(fmt)
    {
    case MilPixelFormat::Indexed1bpp:
    case MilPixelFormat::Indexed2bpp:
    case MilPixelFormat::Indexed4bpp:
    case MilPixelFormat::Indexed8bpp:
        return true;

    default:
        return false;
    };
}

inline BOOL
IsIndexedPixelFormat(
    REFWICPixelFormatGUID fmt // Pixel format
    )
{
    HRESULT hr;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);

    if(FAILED(hr))
    {
        return FALSE;
    }

    return IsIndexedPixelFormat(pf);
}


//+---------------------------------------------------------------------------
//
//  Function:   IsIndexedPixelFormat
//
//  Synopsis:   Returns whether or not the rasterizer can render directly to
//              a surface in this format.
//
//  Returns:    TRUE if this is a rendering pixel format, FALSE otherwise
//
//----------------------------------------------------------------------------
inline BOOL
IsRenderingPixelFormat(
    MilPixelFormat::Enum fmt // Pixel format
)
{
    return (!IsIndexedPixelFormat(fmt)) && (GetPixelFormatSize(fmt) > 8);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetPixelFormatColorSpace
//
//  Synopsis:   Identifies which color space is used by the pixel format.
//
//  Returns:    HRESULT success or failure
//
//----------------------------------------------------------------------------
inline HRESULT
GetPixelFormatColorSpace(
    MilPixelFormat::Enum fmt,     // Pixel format
    OUT ColorSpace *pcs     // Output color space of pixel format
    )
{
    Assert(pcs);

    HRESULT hr = S_OK;

    switch (fmt)
    {
    case MilPixelFormat::BGR32bpp101010:
    case MilPixelFormat::RGB48bppFixedPoint:
    case MilPixelFormat::BGR96bppFixedPoint:
    case MilPixelFormat::RGB128bppFloat:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
    case MilPixelFormat::Gray16bppFixedPoint:
    case MilPixelFormat::Gray32bppFloat:
        *pcs = CS_scRGB;
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
    case MilPixelFormat::Gray16bpp:
    case MilPixelFormat::BGR24bpp:
    case MilPixelFormat::RGB24bpp:
    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::RGB48bpp:
    case MilPixelFormat::RGBA64bpp:
    case MilPixelFormat::PRGBA64bpp:
        *pcs = CS_sRGB;
        break;

    case MilPixelFormat::CMYK32bpp:
        // CMYK is special format and can't be classified as CS_sRGB or CS_scRGB.
        // This routine should not be called for CMYK format[s].
    default:
        AssertMsg(FALSE, "Unexpected pixel format");
        hr = THR(WINCODEC_ERR_INTERNALERROR);
        break;
    }
    INLINED_RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:   GetBestBlendingFormat
//
//  Synopsis:   Get the best format to use for an intermediate surface, which
//              will be blended to a surface of the given format.
//
//  Notes:      For blending, we need a format which has an alpha channel, and
//              sufficient precision in the color and alpha channels to hold
//              intermediate results.
//
//              The current implementation only returns one of the 2 internal
//              pipeline formats - it never returns MilPixelFormat::PRGBA64bpp,
//              which would probably be better for related input formats.
//

inline HRESULT
GetBestBlendingFormat(
    MilPixelFormat::Enum fmtIn,       // Pixel format
    OUT MilPixelFormat::Enum *pfmtOut // Output blending format
    )
{
    HRESULT hr = S_OK;

    ColorSpace cs;
    IFC(GetPixelFormatColorSpace(fmtIn, &cs));

    if (cs == CS_scRGB)
    {
        *pfmtOut = MilPixelFormat::PRGBA128bppFloat;
    }
    else
    {
        Assert(cs == CS_sRGB);

        *pfmtOut = MilPixelFormat::PBGRA32bpp;
    }

Cleanup:
    INLINED_RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   BitsToBytes
//
//  Synopsis:   Converts a count of bits into a count of bytes.  The basic
//              formula is (cBits + 7)/8, which takes advantage of integer
//              division to perform rounding.
//
//              0    --> 0
//              1-8  --> 1
//              9-16 --> 2
//              etc
//
//              Will not overflow.
//
//----------------------------------------------------------------------------
MIL_FORCEINLINE UINT BitsToBytes(UINT cBits)
{
    return cBits/8 + ((cBits & 7) != 0);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCheckBufferSize
//
//  Synopsis:   Returns an error if the provided buffer size isn't large enough
//              for the given pixelformat, stride, and ROI.
//
//              Checks for integer overflow.
//
//----------------------------------------------------------------------------
HRESULT HrCheckBufferSize(
    __in MilPixelFormat::Enum fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __in UINT cbBufferSize
    );

HRESULT HrCheckBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __in UINT cbBufferSize
    );

//+---------------------------------------------------------------------------
//
//  Function:   HrCheckBufferSize
//
//  Synopsis:   Returns an error if the provided buffer size isn't large enough
//              for the given pixelformat, stride, and width and height
//
//              Checks for integer overflow.
//
//----------------------------------------------------------------------------
HRESULT HrCheckBufferSize(
    __in MilPixelFormat::Enum fmt,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __in UINT cbBufferSize
    );

HRESULT HrCheckBufferSize(
    __in REFWICPixelFormatGUID fmt,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __in UINT cbBufferSize
    );

//+---------------------------------------------------------------------------
//
//  Function:   HrCheckBufferSize
//
//  Synopsis:   Returns an error if the provided buffer size isn't large enough
//              for the given pixelformat, stride, and ROI.
//
//              Checks for integer overflow.
//
//----------------------------------------------------------------------------
HRESULT HrCheckBufferSize(
    __in REFWICPixelFormatGUID fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __in UINT cbBufferSize
    );

//+---------------------------------------------------------------------------
//
//  Function:   HrGetRequiredBufferSize
//
//  Synopsis:   Returns the number of bytes required to complete a copypixels
//              operation with a given pixelformat, stride, and ROI.
//
//              Checks for integer overflow.
//
//----------------------------------------------------------------------------
HRESULT HrGetRequiredBufferSize(
    __in MilPixelFormat::Enum fmt,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __out_ecount(1) UINT *pcbSize
    );

HRESULT HrGetRequiredBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __out_ecount(1) UINT *pcbSize
    );


//+---------------------------------------------------------------------------
//
//  Function:   HrGetRequiredBufferSize
//
//  Synopsis:   Returns the number of bytes required to complete a copypixels
//              operation with a given pixelformat, stride, and ROI.
//
//              Checks for integer overflow.
//
//----------------------------------------------------------------------------
HRESULT HrGetRequiredBufferSize(
    __in MilPixelFormat::Enum fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __out_ecount(1) UINT *pcbSize
    );


HRESULT HrGetRequiredBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __out_ecount(1) UINT *pcbSize
    );

//+---------------------------------------------------------------------------
//
//  Function:   GetRequiredBufferSize
//
//  Synopsis:   Returns the number of bytes required to complete a copypixels
//              operation with a given pixelformat, stride, and ROI.
//
//              Assumes that the dimensions are not large enough to cause
//              integer overflow.
//
//----------------------------------------------------------------------------
inline UINT
GetRequiredBufferSize(
    __in MilPixelFormat::Enum fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc
    )
{
    UINT size = 0;

#if DBG
    // Make sure there's no arithmetic overflow errors.
    UINT requiredSize = 0;

    Verify(SUCCEEDED(HrGetRequiredBufferSize(
            fmt,
            cbStride,
            prc,
            &requiredSize
            )));
#endif // DBG

    if (prc->Height == 0)
    {
        size = 0;
    }
    else
    {
        size = (prc->Height-1)*cbStride + (prc->Width*GetPixelFormatSize(fmt)+7)/8;
    }

#if DBG
    Verify(size == requiredSize);
#endif // DBG

    return size;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetRequiredBufferSize
//
//  Synopsis:   Returns the number of bytes required to complete a copypixels
//              operation with a given pixelformat, stride, and ROI.
//
//              Assumes that the dimensions are not large enough to cause
//              integer overflow.
//
//----------------------------------------------------------------------------
inline UINT
GetRequiredBufferSize(
    __in REFWICPixelFormatGUID fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc
    )
{
    HRESULT hr;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);

    Verify(SUCCEEDED(hr));

    if(FAILED(hr))
    {
        return 0;
    }

    return GetRequiredBufferSize(pf, cbStride, prc);
}

//+----------------------------------------------------------------------------
//
//  Function:   HrCalcDWordAlignedScanlineStride
//
//  Synopsis:   Calculates a stride that maintains DWORD-alignment.
//
//  Returns:    WINCODEC_ERR_VALUEOVERFLOW on failure
//
//-----------------------------------------------------------------------------
inline HRESULT
HrCalcDWordAlignedScanlineStride(
    UINT nWidth,
    UINT pixsize,
    UINT & stride
    )
{
    // Ensure that the stride calculation won't overflow.
    if ((pixsize > 0) && (nWidth <= ((INT_MAX - 7) / pixsize)))
    {
        stride = ((((nWidth * pixsize) + 7) >> 3) + 3) & ~3;
        return S_OK;
    }
    stride = 0;
    INLINED_RRETURN(THR(WINCODEC_ERR_VALUEOVERFLOW));
}

//+----------------------------------------------------------------------------
//
//  Function:   HrCalcDWordAlignedScanlineStride
//
//  Synopsis:   Calculates a stride that maintains DWORD-alignment.
//
//  Returns:    WINCODEC_ERR_VALUEOVERFLOW on failure
//
//-----------------------------------------------------------------------------
inline HRESULT
HrCalcDWordAlignedScanlineStride(
    UINT nWidth,
    MilPixelFormat::Enum fmt,
    UINT & stride
    )
{
    INLINED_RRETURN(HrCalcDWordAlignedScanlineStride(nWidth, GetPixelFormatSize(fmt), stride));
}

inline HRESULT
HrCalcDWordAlignedScanlineStride(
    UINT nWidth,
    REFWICPixelFormatGUID fmt,
    UINT & stride
    )
{
    HRESULT hr;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);
    if (SUCCEEDED(hr))
        hr = HrCalcDWordAlignedScanlineStride(nWidth, GetPixelFormatSize(pf), stride);

    INLINED_RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   HrCalcByteAlignedScanlineStride
//
//  Synopsis:   Calculates a stride that maintains BYTE-alignment.
//
//  Returns:    WINCODEC_ERR_VALUEOVERFLOW on failure
//
//-----------------------------------------------------------------------------
inline HRESULT
HrCalcByteAlignedScanlineStride(
    UINT nWidth,
    UINT pixsize,
    UINT & stride
    )
{
    // Ensure that the stride calculation won't overflow.
    if ((pixsize > 0) && (nWidth <= ((INT_MAX - 7) / pixsize)))
    {
        stride = ((nWidth * pixsize) + 7) >> 3;
        return S_OK;
    }
    stride = 0;
    INLINED_RRETURN(THR(WINCODEC_ERR_VALUEOVERFLOW));
}

//+----------------------------------------------------------------------------
//
//  Function:   HrCalcByteAlignedScanlineStride
//
//  Synopsis:   Calculates a stride that maintains BYTE-alignment.
//
//  Returns:    WINCODEC_ERR_VALUEOVERFLOW on failure
//
//-----------------------------------------------------------------------------
inline HRESULT
HrCalcByteAlignedScanlineStride(
    UINT nWidth,
    MilPixelFormat::Enum fmt,
    UINT & stride
    )
{
    INLINED_RRETURN(HrCalcByteAlignedScanlineStride(nWidth, GetPixelFormatSize(fmt), stride));
}

inline HRESULT
HrCalcByteAlignedScanlineStride(
    UINT nWidth,
    REFWICPixelFormatGUID fmt,
    UINT & stride
    )
{
    HRESULT hr;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);
    if (SUCCEEDED(hr))
        hr = HrCalcByteAlignedScanlineStride(nWidth, GetPixelFormatSize(pf), stride);

    INLINED_RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Function:   ByteSaturate
//
//  Synopsis:   Converts an integer to a byte by clamping it to the range
//              that can be represented by a byte//
//  Returns:    BYTE containing converted integer value
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE BYTE
ByteSaturate(
    INT i   // Integer value of
    )
{
    return static_cast<BYTE>(ClampInteger(i, 0, 255));
}

//+----------------------------------------------------------------------------
//
//  Function:   MyPremultiply
//
//  Synopsis:   Premultiplication method used by the PNG decoder and
//              soalphamultiply.cpp.
//
//  Returns:    INT containing saturated integer value
//
//-----------------------------------------------------------------------------
inline ARGB
MyPremultiply(
    ARGB argb
    )
{
    ARGB a = (argb >> MIL_ALPHA_SHIFT);

    ARGB _000000gg = (argb >> 8) & 0x000000ff;
    ARGB _00rr00bb = (argb & 0x00ff00ff);

    ARGB _0000gggg = _000000gg * a + 0x00000080;
    _0000gggg += (_0000gggg >> 8);

    ARGB _rrrrbbbb = _00rr00bb * a + 0x00800080;
    _rrrrbbbb += ((_rrrrbbbb >> 8) & 0x00ff00ff);

    return (a << MIL_ALPHA_SHIFT) |
           (_0000gggg & 0x0000ff00) |
           ((_rrrrbbbb >> 8) & 0x00ff00ff);
}



//============================================================================
//
// Pixel format conversion prototypes & inline methods
//
//----------------------------------------------------------------------------

//============================================================================
//
// Unaligned & non-inlined pixel format conversions
//
// These functions perform conversions on unaligned memory.
//

//
// scRGB -> sRGB conversions
//

MilColorF Convert_MilColorF_scRGB_To_MilColorF_sRGB(const MilColorF * pColor);
MilColorB Convert_MilColorF_scRGB_To_MilColorB_sRGB(const MilColorF* pColor);
MilColorB Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(const MilColorF* pColor);

//
// sRGB -> Hardware
//

D3DCOLOR Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(__in_ecount(1) const MilColorF * pColor);
D3DCOLOR Convert_MilColorF_sRGB_To_D3DCOLOR_OneAlpha(__in_ecount(1) const MilColorF * pColor);
void Put_sRGB_Alpha_In_D3DCOLOR_WithNoAlpha(
    FLOAT alpha,
    __inout_ecount(1) D3DCOLOR *pD3DColor
    );

//============================================================================
//
// Aligned pixel format conversions
//
// These functions do not perform UNALIGNED casts.  Thus, the input
// must be guaranteed to be aligned.
//

//
// scRGB -> sRGB conversions
//

UINT16 Convert_scRGB_float_To_sRGB_UINT16(float v);

// Definition for Convert_scRGB_Channel_To_sRGB_Byte
#include "gammaluts.inc"

//+----------------------------------------------------------------------------
//
//  Function:  Convert_scRGB_Channel_To_sRGB_Float
//
//  Synopsis:  Convert a color channel from scRGB to "sRGB float" (0.0f to 1.0f)
//
//  Returns:   Converted color channel
//
//-----------------------------------------------------------------------------
inline float Convert_scRGB_Channel_To_sRGB_Float(
    float rColorComponent
    )
{
    return static_cast<float>(Convert_scRGB_Channel_To_sRGB_Byte(rColorComponent))/255.0f;
}

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MilColorF_scRGB_To_MilColorB_sRGB
//
//  Synopsis:  Converts a non-premultiplied scRGB MilColorF to sRGB MilColorB inline
//             without performing unaligned casts.
//
//             The premultiplication state is important when converting between
//             different color spaces because the conversion must be done on
//             non-premultiplied colors.
//
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE VOID
Inline_Convert_MilColorF_scRGB_To_MilColorB_sRGB(
    IN const MilColorF *pInputColor,
    OUT MilColorB *pOutputColor
    )
{
    // Convert MilColorF scRGB color channels to MilColorB sRGB
    *pOutputColor = MIL_COLOR(
        ByteSaturate(GpRound(255.0f*pInputColor->a)),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->r),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->g),
        Convert_scRGB_Channel_To_sRGB_Byte(pInputColor->b)
        );
}

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_Premultiplied_MilColorF_scRGB_To_Premultipled_MilColorB_sRGB
//
//  Synopsis:  Converts a premultipled scRGB MilColorF to sRGB MilColorB inline
//             without performing unaligned casts.
//
//             The premultiplication state is important when converting between
//             different color spaces because the conversion must be done on
//             non-premultiplied colors.
//
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE VOID
Inline_Convert_Premultiplied_MilColorF_scRGB_To_Premultipled_MilColorB_sRGB(
    IN const MilColorF *pInputColor,
    OUT MilColorB *pOutputColor
    )
{
    MilColorF unPremultipliedColor = *pInputColor;
    Unpremultiply(&unPremultipliedColor);

    Inline_Convert_MilColorF_scRGB_To_MilColorB_sRGB(
        &unPremultipliedColor,
        pOutputColor
        );

    *pOutputColor = Premultiply(*pOutputColor);
}

//
// sRGB -> sRGB conversions
//

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MilColorF_sRGB_To_MilColorB_sRGB
//
//  Synopsis:  Converts a sRGB MilColorF to a sRGB MilColorB inline
//             without performing unaligned casts.
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE VOID
Inline_Convert_MilColorF_sRGB_To_MilColorB_sRGB(
    IN const MilColorF *pInputColor,
    OUT MilColorB *pOutputColor
    )
{
    *pOutputColor = MIL_COLOR(
        ByteSaturate(GpRound(pInputColor->a * 255.0f)),
        ByteSaturate(GpRound(pInputColor->r * 255.0f)),
        ByteSaturate(GpRound(pInputColor->g * 255.0f)),
        ByteSaturate(GpRound(pInputColor->b * 255.0f))
        );
}

//+---------------------------------------------------------------------------
//
//  Member:    Inline_Convert_MilColorB_sRGB_To_AGRB64TEXEL_sRGB
//
//  Synopsis:  Converts a sRGB MilColorB to a sRGB AGRB64TEXEL.
//
//-----------------------------------------------------------------------------
MIL_FORCEINLINE VOID
Inline_Convert_MilColorB_sRGB_To_AGRB64TEXEL_sRGB(
    IN const MilColorB inputColor,
    OUT AGRB64TEXEL *pOutputColor
    )
{
    pOutputColor->A00aa00gg =
        (MIL_COLOR_GET_ALPHA(inputColor) << 16) | MIL_COLOR_GET_GREEN(inputColor);

    pOutputColor->A00rr00bb =
        (MIL_COLOR_GET_RED(inputColor) << 16) | MIL_COLOR_GET_BLUE(inputColor);

}

//
// sRGB -> scRGB conversions
//

float Convert_sRGB_UINT16_To_scRGB_float(UINT16 v);


#pragma warning(pop)



