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

#include "precomp.hpp"

//
// Lookup tables
//

// UnpremultiplyTable[x] = floor(65536 * (255.0 / x))
const ARGB UnpremultiplyTable[256] =
{
    0x000000,0xff0000,0x7f8000,0x550000,0x3fc000,0x330000,0x2a8000,0x246db6,
    0x1fe000,0x1c5555,0x198000,0x172e8b,0x154000,0x139d89,0x1236db,0x110000,
    0x0ff000,0x0f0000,0x0e2aaa,0x0d6bca,0x0cc000,0x0c2492,0x0b9745,0x0b1642,
    0x0aa000,0x0a3333,0x09cec4,0x0971c7,0x091b6d,0x08cb08,0x088000,0x0839ce,
    0x07f800,0x07ba2e,0x078000,0x074924,0x071555,0x06e453,0x06b5e5,0x0689d8,
    0x066000,0x063831,0x061249,0x05ee23,0x05cba2,0x05aaaa,0x058b21,0x056cef,
    0x055000,0x05343e,0x051999,0x050000,0x04e762,0x04cfb2,0x04b8e3,0x04a2e8,
    0x048db6,0x047943,0x046584,0x045270,0x044000,0x042e29,0x041ce7,0x040c30,
    0x03fc00,0x03ec4e,0x03dd17,0x03ce54,0x03c000,0x03b216,0x03a492,0x03976f,
    0x038aaa,0x037e3f,0x037229,0x036666,0x035af2,0x034fca,0x0344ec,0x033a54,
    0x033000,0x0325ed,0x031c18,0x031281,0x030924,0x030000,0x02f711,0x02ee58,
    0x02e5d1,0x02dd7b,0x02d555,0x02cd5c,0x02c590,0x02bdef,0x02b677,0x02af28,
    0x02a800,0x02a0fd,0x029a1f,0x029364,0x028ccc,0x028656,0x028000,0x0279c9,
    0x0273b1,0x026db6,0x0267d9,0x026217,0x025c71,0x0256e6,0x025174,0x024c1b,
    0x0246db,0x0241b2,0x023ca1,0x0237a6,0x0232c2,0x022df2,0x022938,0x022492,
    0x022000,0x021b81,0x021714,0x0212bb,0x020e73,0x020a3d,0x020618,0x020204,
    0x01fe00,0x01fa0b,0x01f627,0x01f252,0x01ee8b,0x01ead3,0x01e72a,0x01e38e,
    0x01e000,0x01dc7f,0x01d90b,0x01d5a3,0x01d249,0x01cefa,0x01cbb7,0x01c880,
    0x01c555,0x01c234,0x01bf1f,0x01bc14,0x01b914,0x01b61e,0x01b333,0x01b051,
    0x01ad79,0x01aaaa,0x01a7e5,0x01a529,0x01a276,0x019fcb,0x019d2a,0x019a90,
    0x019800,0x019577,0x0192f6,0x01907d,0x018e0c,0x018ba2,0x018940,0x0186e5,
    0x018492,0x018245,0x018000,0x017dc1,0x017b88,0x017957,0x01772c,0x017507,
    0x0172e8,0x0170d0,0x016ebd,0x016cb1,0x016aaa,0x0168a9,0x0166ae,0x0164b8,
    0x0162c8,0x0160dd,0x015ef7,0x015d17,0x015b3b,0x015965,0x015794,0x0155c7,
    0x015400,0x01523d,0x01507e,0x014ec4,0x014d0f,0x014b5e,0x0149b2,0x01480a,
    0x014666,0x0144c6,0x01432b,0x014193,0x014000,0x013e70,0x013ce4,0x013b5c,
    0x0139d8,0x013858,0x0136db,0x013562,0x0133ec,0x01327a,0x01310b,0x012fa0,
    0x012e38,0x012cd4,0x012b73,0x012a15,0x0128ba,0x012762,0x01260d,0x0124bc,
    0x01236d,0x012222,0x0120d9,0x011f93,0x011e50,0x011d10,0x011bd3,0x011a98,
    0x011961,0x01182b,0x0116f9,0x0115c9,0x01149c,0x011371,0x011249,0x011123,
    0x011000,0x010edf,0x010dc0,0x010ca4,0x010b8a,0x010a72,0x01095d,0x01084a,
    0x010739,0x01062b,0x01051e,0x010414,0x01030c,0x010206,0x010102,0x010000,
};


//
// Pixel format functions
//

//+---------------------------------------------------------------------------
//
//  Function:   HasAlphaChannel
//
//  Synopsis:   Returns whether or not the pixel format supports an
//              alpha channel.
//
//  Returns:    TRUE if the pixel format supports an alpha channel
//              FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL
HasAlphaChannel(
    REFWICPixelFormatGUID fmt  // Pixel format
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum pf;

    hr = WicPfToMil(fmt, &pf);
    if (SUCCEEDED(hr))
        return HasAlphaChannel(pf);
    else
        return FALSE;
}

#pragma warning(push)
// Disable explicit handling of every enum type in a switch because
// we want to fall through deliberately on many pixel format types
#pragma warning(disable:4061)

BOOL
HasAlphaChannel(
    MilPixelFormat::Enum fmt  // Pixel format
    )
{
    switch(fmt)
    {

    // gray
    case MilPixelFormat::BlackWhite:
    case MilPixelFormat::Gray2bpp:
    case MilPixelFormat::Gray4bpp:
    case MilPixelFormat::Gray8bpp:
    case MilPixelFormat::Gray32bppFloat: // float

    // 16bpp
    case MilPixelFormat::BGR16bpp555:
    case MilPixelFormat::BGR16bpp565:
    case MilPixelFormat::Gray16bppFixedPoint:
    case MilPixelFormat::Gray16bpp:

    // 24bpp
    case MilPixelFormat::RGB24bpp:
    case MilPixelFormat::BGR24bpp:

    // 32bpp
    case MilPixelFormat::BGR32bpp:
    case MilPixelFormat::CMYK32bpp:
    case MilPixelFormat::BGR32bpp101010:

    // 48bpp
    case MilPixelFormat::RGB48bppFixedPoint:
    case MilPixelFormat::RGB48bpp:

    // 128bpp
    case MilPixelFormat::RGB128bppFloat:

        return FALSE;

    // indexed - the palette may contain alpha
    case MilPixelFormat::Indexed1bpp:
    case MilPixelFormat::Indexed2bpp:
    case MilPixelFormat::Indexed4bpp:
    case MilPixelFormat::Indexed8bpp:

    // alpha formats
    case MilPixelFormat::BGRA32bpp:
    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::RGBA64bpp:
    case MilPixelFormat::PRGBA64bpp:
    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
        return TRUE;

    // It's possible that this gets hit for extended pixelformats
    // In this case currently we have no way of determining whether
    // the format has alpha or not regardless, so its OK to assume FALSE.
    default:
        return FALSE;
        break;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:   IsPremultipliedFormOf
//
//  Synopsis:   Returns whether the first pixel format is simply a premultiplied
//              form of the second pixel format - and identical in every other
//              way.
//
//  Returns:    TRUE if fmt1 is a premultiplied form of fmt2
//              FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
IsPremultipliedFormOf(
    MilPixelFormat::Enum fmt1,    // Premultiplied format
    MilPixelFormat::Enum fmt2     // Unpermultiplied format
    )
{
    switch (fmt1)
    {
    case MilPixelFormat::PBGRA32bpp:
        return (fmt2 == MilPixelFormat::BGRA32bpp);

    case MilPixelFormat::PRGBA64bpp:
        return (fmt2 == MilPixelFormat::RGBA64bpp);

    case MilPixelFormat::PRGBA128bppFloat:
        return (fmt2 == MilPixelFormat::RGBA128bppFloat);

    default:
        return FALSE;
    }
}

#pragma warning(pop)

//+-----------------------------------------------------------------------------
//
//  Function:   IsNoAlphaFormOf
//
//  Synopsis:   Returns whether the first pixel format is simply an alpha-less
//              form of the second pixel format - and identical in every other
//              way.
//
//  Returns:    TRUE if fmt1 is a non-alpha form of fmt2
//              FALSE otherwise
//
//----------------------------------------------------------------------------
BOOL
IsNoAlphaFormOf(
    MilPixelFormat::Enum fmt1,    // Non-alpha format
    MilPixelFormat::Enum fmt2     // Alpha format
    )
{
    BOOL fRet = FALSE;

    switch (fmt1)
    {
    case MilPixelFormat::BGR32bpp:
        fRet = (fmt2 == MilPixelFormat::BGRA32bpp);
        break;

    case MilPixelFormat::RGB128bppFloat:
        fRet = (fmt2 == MilPixelFormat::RGBA128bppFloat);
        break;
    }

    if (fRet)
    {
        Assert(!HasAlphaChannel(fmt1));
        Assert(HasAlphaChannel(fmt2));
    }

    return fRet;
}

//+-----------------------------------------------------------------------------
//
//  Function:   D3DFormatToPixelFormat
//
//  Synopsis:   Given a direct 3D format return an appropriate pixel format if
//              possible otherwise return undefined.
//
//  Returns:    MilPixelFormat::Enum that corresponds to the input
//
//----------------------------------------------------------------------------
MilPixelFormat::Enum
D3DFormatToPixelFormat(
    D3DFORMAT d3dFmt,   // D3D Pixel Format
    BOOL bPremultiplied // Whether or not the format is premultiplied
    )
{
    //   Need gamma to map D3D formats to MIL formats
    //   D3D formats don't specify gamma. So this function needs a ColorSpace
    //   argument.

    switch (d3dFmt)
    {
        case D3DFMT_R8G8B8:
            return MilPixelFormat::BGR24bpp;

        case D3DFMT_A8R8G8B8:
            if (bPremultiplied)
            {
                return MilPixelFormat::PBGRA32bpp;
            }
            else
            {
                return MilPixelFormat::BGRA32bpp;
            }

        case D3DFMT_X8R8G8B8:
            return MilPixelFormat::BGR32bpp;

        case D3DFMT_R5G6B5:
            return MilPixelFormat::BGR16bpp565;

        case D3DFMT_X1R5G5B5:
            return MilPixelFormat::BGR16bpp555;

        case D3DFMT_P8:
            return MilPixelFormat::Indexed8bpp;

        case D3DFMT_L8:
            return MilPixelFormat::Gray8bpp;

        case D3DFMT_A2R10G10B10:
            return MilPixelFormat::BGR32bpp101010;

        case D3DFMT_A32B32G32R32F:
            if (bPremultiplied)
            {
                return MilPixelFormat::PRGBA128bppFloat;
            }
            else
            {
                return MilPixelFormat::RGBA128bppFloat;
            }
    }

    return MilPixelFormat::Undefined;
}

//+-----------------------------------------------------------------------------
//
//  Function:   PixelFormatToD3DFormat
//
//  Synopsis:   Given a pixel format return an appropriate direct 3D format if
//              possible otherwise return D3DFMT_UNKNOWN.
//
//  Returns:    D3DFORMAT that corresponds to the input
//
//----------------------------------------------------------------------------
D3DFORMAT
PixelFormatToD3DFormat(
    MilPixelFormat::Enum pixelFormat
    )
{
    switch (pixelFormat)
    {
    case MilPixelFormat::BGR24bpp:
        return D3DFMT_R8G8B8;

    case MilPixelFormat::PBGRA32bpp:
    case MilPixelFormat::BGRA32bpp:
        return D3DFMT_A8R8G8B8;

    case MilPixelFormat::BGR32bpp:
        return D3DFMT_X8R8G8B8;

    case MilPixelFormat::BGR16bpp565:
        return D3DFMT_R5G6B5;

    case MilPixelFormat::BGR16bpp555:
        return D3DFMT_X1R5G5B5;

    case MilPixelFormat::Indexed8bpp:
        return D3DFMT_P8;

    case MilPixelFormat::Gray8bpp:
        return D3DFMT_L8;

    case MilPixelFormat::BGR32bpp101010:
        return D3DFMT_A2R10G10B10;

    case MilPixelFormat::RGBA128bppFloat:
    case MilPixelFormat::PRGBA128bppFloat:
        return D3DFMT_A32B32G32R32F;
    }

    return D3DFMT_UNKNOWN;
}

//+-----------------------------------------------------------------------------
//
//  Function:   Unpremultiply
//
//  Synopsis:   Unpremultiplies an ARGB value
//
//  Returns:    Unpremultipied ARGB value
//
//----------------------------------------------------------------------------
ARGB
Unpremultiply(
    ARGB argb   // Premultied ARGB value to convert
    )
{
    // Get alpha value

    ARGB a = argb >> MIL_ALPHA_SHIFT;

    // Special case: fully transparent or fully opaque

    if (a == 0 || a == 255)
        return argb;

    ARGB f = UnpremultiplyTable[a];

    ARGB r = ((argb >> MIL_RED_SHIFT) & 0xff) * f >> 16;
    ARGB g = ((argb >> MIL_GREEN_SHIFT) & 0xff) * f >> 16;
    ARGB b = ((argb >> MIL_BLUE_SHIFT) & 0xff) * f >> 16;

    return (a << MIL_ALPHA_SHIFT) |
           ((r > 255 ? 255 : r) << MIL_RED_SHIFT) |
           ((g > 255 ? 255 : g) << MIL_GREEN_SHIFT) |
           ((b > 255 ? 255 : b) << MIL_BLUE_SHIFT);
}

//+-----------------------------------------------------------------------------
//
//  Function:   Premultiply
//
//  Synopsis:   Premultiplies an ARGB value
//
//  Returns:    Premultipied ARGB value
//
//----------------------------------------------------------------------------
ARGB
Premultiply(
    ARGB argb   // Non-premultiplied ARGB value
    )
{
    ARGB a = (argb >> MIL_ALPHA_SHIFT);

    if (a == 255)
        return argb;
    else if (a == 0)
        return 0;

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

//+-----------------------------------------------------------------------------
//
//  Function:   Unpremultiply
//
//  Synopsis:   Unpremultiplies a MilColorF
//
//  Returns:    void
//
//----------------------------------------------------------------------------
void
Unpremultiply(
    __inout_ecount(1) MilColorF *pColor // Color to unpremultiply
    )
{
    if (pColor->a > 0)
    {
        pColor->r /= pColor->a;
        pColor->g /= pColor->a;
        pColor->b /= pColor->a;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:   Premultiply
//
//  Synopsis:   Premultiplies a MilColorF
//
//  Returns:    Premultipied MilColorF
//
//------------------------------------------------------------------------------
void
Premultiply(
    __inout_ecount(1) MilColorF *pColor   // Color to premultiply
    )
{
    pColor->r *= pColor->a;
    pColor->g *= pColor->a;
    pColor->b *= pColor->a;
}

//+-----------------------------------------------------------------------------
//
//  Function:   D3DFormatSize
//
//  Synopsis:   Given a direct 3D format return the number of bytes need to store
//              one pixel.  Returns 0 for unsupported formats.
//
//  Returns:    Byte count of pixel format.
//
//------------------------------------------------------------------------------
UINT
D3DFormatSize(
    D3DFORMAT d3dFmt    // Format to return the size of
    )
{
    switch (d3dFmt)
    {
    case D3DFMT_A32B32G32R32F:
        return 16;

    case D3DFMT_A8R8G8B8:
    case D3DFMT_X8R8G8B8:
    case D3DFMT_D24S8:
    case D3DFMT_A2R10G10B10:
        return 4;

    case D3DFMT_R8G8B8:
        return 3;

    case D3DFMT_R5G6B5:
    case D3DFMT_X1R5G5B5:
    case D3DFMT_D16:
        return 2;

    case D3DFMT_P8:
    case D3DFMT_L8:
        return 1;
    }

    AssertMsg(0, "Can't get size of unsupported format");

    return 0;
}

//+-----------------------------------------------------------------------------
//
//  Function:   Convert_MilColorF_scRGB_To_MilColorB_sRGB
//
//  Synopsis:   Given a non-premultiplied scRGB MilColorF, return a non-premultiplied
//              MilColorB in sRGB space.
//
//              This method does not require aligned memory.  It does an alignment
//              cast before conversion.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//
//  Returns:    sRGB MilColorB
//
//------------------------------------------------------------------------------
MilColorB Convert_MilColorF_scRGB_To_MilColorB_sRGB(
    const MilColorF * pColor    // Non-premultiplied scRGB MilColorF to convert
    )
{
    Assert(pColor);

    MilColorB resultColor;

    // Perform the unaligned read
    MilColorF colorf = *static_cast<const MilColorF UNALIGNED *>(pColor);

    // Convert the color
    Inline_Convert_MilColorF_scRGB_To_MilColorB_sRGB(&colorf, &resultColor);

    return resultColor;
}

//+-----------------------------------------------------------------------------
//
//  Function:   Convert_MilColorF_scRGB_To_MilColorF_sRGB
//
//  Synopsis:   Given a non-premultiplied scRGB MilColorF return a non-premultiplied
//              MilColorF in normalized sRGB space.
//
//              This method does not require aligned memory.  It does an alignment
//              cast before conversion.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//  Returns:    sRGB MilColorF
//
//------------------------------------------------------------------------------
MilColorF
Convert_MilColorF_scRGB_To_MilColorF_sRGB(
    const MilColorF * pColor    // scRGB MilColorF to convert
    )
{
    Assert(pColor);

    MilColorF colorf = *static_cast<const MilColorF UNALIGNED *>(pColor);

    colorf.a = ClampAlpha(pColor->a);

    colorf.r = Convert_scRGB_Channel_To_sRGB_Float(pColor->r);
    colorf.g = Convert_scRGB_Channel_To_sRGB_Float(pColor->g);
    colorf.b = Convert_scRGB_Channel_To_sRGB_Float(pColor->b);

    return colorf;
}

//+-----------------------------------------------------------------------------
//
//  Function:   Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB
//
//  Synopsis:   Given a non-premultiplied scRGB MilColorF, return a premultiplied
//              MilColorB in sRGB space.
//
//              This method does not require aligned memory.  It does an alignment
//              cast before conversion.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//
//  Returns:    Premultiplied sRGB  MilColorB
//
//------------------------------------------------------------------------------
MilColorB
Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(
    const MilColorF * pColor    // scRGB MilColorF to convert
    )
{
    MilColorF colorf = *static_cast<const MilColorF UNALIGNED *>(pColor);

    colorf = Convert_MilColorF_scRGB_To_MilColorF_sRGB(pColor);

    return MIL_COLOR(CFloatFPU::SmallRound(colorf.a*255.0f),
                     CFloatFPU::SmallRound(colorf.r*colorf.a*255.0f),
                     CFloatFPU::SmallRound(colorf.g*colorf.a*255.0f),
                     CFloatFPU::SmallRound(colorf.b*colorf.a*255.0f));
}

//+------------------------------------------------------------------------
//
//  Function:  Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha
//
//  Synopsis:  Converts an sRGB MilColorF into a D3DCOLOR. The alpha value
//             in "color" is ignored and 0.0 is substituted.
//
//  Assumptions: R, G, B <= 1.0 so we don't need to & w/ 255 and that
//               we do not care about the value of Alpha in lighting.
//
//-------------------------------------------------------------------------
D3DCOLOR
Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(
    __in_ecount(1) const MilColorF * pColor
    )
{
    // Funny Assert conditions are to let NaN through
    Assert(!(pColor->r > 1.0));
    Assert(!(pColor->g > 1.0));
    Assert(!(pColor->b > 1.0));

    return ((GpRound(pColor->r * 255.0f) << 16) | (GpRound(pColor->g * 255.0f) << 8) | GpRound(pColor->b * 255.0f));
}

//+------------------------------------------------------------------------
//
//  Function:  Convert_MilColorF_sRGB_To_D3DCOLOR_OneAlpha
//
//  Synopsis:  Converts an sRGB MilColorF into a D3DCOLOR. The alpha value
//             in "color" is ignored and 1.0 is substituted.
//
//  Assumptions: R, G, B <= 1.0 so we don't need to & w/ 255 and that
//               we do not care about the value of Alpha in lighting.
//
//-------------------------------------------------------------------------
D3DCOLOR
Convert_MilColorF_sRGB_To_D3DCOLOR_OneAlpha(
    __in_ecount(1) const MilColorF * pColor
    )
{
    return ((255 << 24) | Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(pColor));
}


//+----------------------------------------------------------------------------
//
//  Member:
//      Put_sRGB_Alpha_In_D3DCOLOR_WithNoAlpha
//
//  Synopsis:
//      Takes the alpha value in sRGB and places it in a D3DCOLOR where the
//      D3DCOLOR has an alpha value of 0.
//
//  Assumptions: A, R, G, B <= 1.0 so we don't need to & w/ 255
//

void
Put_sRGB_Alpha_In_D3DCOLOR_WithNoAlpha(
    FLOAT alpha,
    __inout_ecount(1) D3DCOLOR *pD3DColor
    )
{
    // Funny Assert conditions are to let NaN through
    Assert(!(alpha > 1.0));

    Assert(MIL_COLOR_GET_ALPHA(*pD3DColor) == 0);

    *pD3DColor |= (GpRound(alpha * 255.0f) << 24);
}

//+---------------------------------------------------------------------------
//
//  Function:   Convert_sRGB_UINT16_To_scRGB_float
//
//  Synopsis:   Convert non-premultiplied unsigned 16-bit value in sRGB space,
//              in the range 0 <= value <= 0xFFFF,
//              to a non-premultiplied 32-bit floating point value in 1.0 gamma space,
//              in the range 0 <= value <= 1.
//
//  Note:       sRGB values are close but not equal to ones
//              in gamma 2.2 space.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//
//  Returns:    scRGB float
//
//----------------------------------------------------------------------------
float
Convert_sRGB_UINT16_To_scRGB_float(UINT16 v)
{
    // We'll use Gamma_sRGBLUT[256] table with linear
    // interpolation between two neighboring values.
    // The first step is to map the range [0,0xffff]
    // to fixed point 16.16 value so as 0xffff will
    // precisely give 255.0. You may verify that
    // ratio*0xFFFF = 0xFF0000FF so it works.

    static const UINT ratio = 0xFF01;  // == (0xFF000000/0xFFFF)
                                       // [but rounded up instead of down]
    UINT v16_16 = (ratio*v) >> 8;

    UINT index = v16_16 >> 16;
    UINT fraction = v16_16 & 0xFFFF;

    double r = GammaLUT_sRGB_to_scRGB[index];
    if (fraction)
    {
        r += (GammaLUT_sRGB_to_scRGB[index+1] - r)*fraction*(1./0x10000);
    }

    r *= 1./255;
    Assert(r >= 0 && r <= 1);

    return float(r);
}

//+---------------------------------------------------------------------------
//
// Function:    Convert_scRGB_float_To_sRGB_UINT16
//
// Synopsis:    Convert non-premultiplied 32-bit floating point value in 1.0
//              gamma space,
//              in the range 0 <= value <= 1,
//              to unsigned non-premultiplied 16-bit value in sRGB space,
//              in the range 0 <= value <= 0xFFFF.
//
//              The premultiplication state is important when converting between
//              different color spaces because the conversion must be done on
//              non-premultiplied colors.
//
//
//  Returns:    sRGB UINT16
//
//----------------------------------------------------------------------------
UINT16
Convert_scRGB_float_To_sRGB_UINT16(float v)
{
    // Convert given value to the range used in Gamma_sRGBLUT
    double r = v*255.;
    CDoubleFPU dfpu;

    if (!(r > 0)) return 0; // this works for NaNs also
    if (r >= 255) return 0xFFFF;

    // Find a neighbouring pair in the lookup table.
    // Use ElevenBitInvGamma_sRGB table to get a hint.
    UINT index = Convert_scRGB_Channel_To_sRGB_Byte(v);

    // We need to guarantee that
    // GammaLUT_sRGB_to_scRGB[index] <= r and  r < GammaLUT_sRGB_to_scRGB[index+1].
    // Do some iterations for it.

    Assert(GammaLUT_sRGB_to_scRGB[0] <= r &&  r < GammaLUT_sRGB_to_scRGB[255]);

    Assert(index <= 255);
    index = min(index, (UINT)ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2);

    while (index <= ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2 &&
           r < GammaLUT_sRGB_to_scRGB[index])
    {
        Assert(index > 0);
        index--;
    }

    Assert(index <= ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2);
    index = min(index, (UINT)ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2);

    while (index <= ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2 &&
           r >= GammaLUT_sRGB_to_scRGB[index+1])
    {
        index++;
    }

    Assert(index <= ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2);
    index = min(index, (UINT)ARRAYSIZE(GammaLUT_sRGB_to_scRGB)-2);

    double f = (r - GammaLUT_sRGB_to_scRGB[index]) / (GammaLUT_sRGB_to_scRGB[index+1] - GammaLUT_sRGB_to_scRGB[index]);
    UINT fraction = GpRound(f*256);
    Assert (fraction <= 256);

    // Compose the result from index and fraction.

    static const UINT ratio = 0x10101;    // == 0xFFFFFF00/0xFF00
    return static_cast<UINT16>((((index<<8) + fraction) * ratio) >> 16);
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetRequiredBufferSize
//
//  Synopsis:   Returns the number of bytes required to complete a copypixels
//              operation with a given pixelformat, stride, and width and height.
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
    )
{
    RRETURN( HrGetRequiredBufferSize(GetPixelFormatSize(fmt), cbStride, width, height, pcbSize) );
}


HRESULT HrGetRequiredBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __out_ecount(1) UINT *pcbSize
    )
{
    HRESULT hr = S_OK;
    UINT cbSize = 0;

    if (height != 0)
    {
        UINT requiredSize = static_cast<UINT>(height-1);
        UINT tmp = width;

        IFC(UIntMult(tmp, fmtBpp, &tmp));
        tmp = BitsToBytes(tmp);

        if (cbStride < tmp)
        {
            IFC(WINCODEC_ERR_INVALIDPARAMETER);
        }

        IFC(UIntMult(requiredSize, cbStride, &requiredSize));
        IFC(UIntAdd(requiredSize, tmp, &requiredSize));

        cbSize = requiredSize;
    }

    *pcbSize = cbSize;

Cleanup:
    RRETURN(hr);
}


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
    )
{
    HRESULT hr = S_OK;

    if (!prc || prc->Height < 0 || prc->Width < 0)
    {
        IFC(WINCODEC_ERR_INVALIDPARAMETER);
    }

    IFC(HrGetRequiredBufferSize(
            fmt,
            cbStride,
            (UINT)(prc->Width),
            (UINT)(prc->Height),
            pcbSize));

Cleanup:

    RRETURN(hr);
}

HRESULT HrGetRequiredBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __out_ecount(1) UINT *pcbSize
    )
{
    HRESULT hr = S_OK;

    if (!prc || prc->Height < 0 || prc->Width < 0)
    {
        IFC(WINCODEC_ERR_INVALIDPARAMETER);
    }

    IFC(HrGetRequiredBufferSize(
            fmtBpp,
            cbStride,
            (UINT)(prc->Width),
            (UINT)(prc->Height),
            pcbSize));

Cleanup:

    RRETURN(hr);    
}

//+---------------------------------------------------------------------------
//
//  Function:   HrCheckBufferSize
//
//  Synopsis:   Returns an error if the provided buffer size isn't large enough
//              for the given pixelformat, stride, and width and height.
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
    )
{
    HRESULT hr = S_OK;
    UINT requiredSize = 0;

    IFC(HrGetRequiredBufferSize(
            fmt,
            cbStride,
            width,
            height,
            &requiredSize));

    if (requiredSize > cbBufferSize)
    {
        IFC(WINCODEC_ERR_INSUFFICIENTBUFFER);
    }

Cleanup:
    RRETURN(hr);
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
    )
{
    HRESULT hr = S_OK;
    UINT requiredSize = 0;

    IFC(HrGetRequiredBufferSize(
            fmt,
            cbStride,
            prc,
            &requiredSize));

    if (requiredSize > cbBufferSize)
    {
        IFC(WINCODEC_ERR_INSUFFICIENTBUFFER);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT HrCheckBufferSize(
    __in UINT fmtBpp,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __in UINT cbBufferSize
    )
{
    HRESULT hr = S_OK;
    UINT requiredSize = 0;

    IFC(HrGetRequiredBufferSize(
            fmtBpp,
            cbStride,
            prc,
            &requiredSize));

    if (requiredSize > cbBufferSize)
    {
        IFC(WINCODEC_ERR_INSUFFICIENTBUFFER);
    }

Cleanup:
    RRETURN(hr);
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
    __in REFWICPixelFormatGUID fmt,  // Pixel format
    __in UINT cbStride,
    __in_ecount(1) const WICRect *prc,
    __in UINT cbBufferSize
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum pf;

    IFC(WicPfToMil(fmt, &pf));

    IFC(HrCheckBufferSize(pf, cbStride, prc, cbBufferSize));

Cleanup:
    RRETURN(hr);
}

HRESULT HrCheckBufferSize(
    __in REFWICPixelFormatGUID fmt,  // Pixel format
    __in UINT cbStride,
    __in UINT width,
    __in UINT height,
    __in UINT cbBufferSize
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum pf;

    IFC(WicPfToMil(fmt, &pf));

    IFC(HrCheckBufferSize(pf, cbStride, width, height, cbBufferSize));

Cleanup:
    RRETURN(hr);
}



