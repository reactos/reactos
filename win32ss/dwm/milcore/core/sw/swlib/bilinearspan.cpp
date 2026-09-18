// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Texture interpolation (bilinear and others)
//
//  $Notes:
//      Implementation of WrapModeTile:
//          We tile by translating our sample coordinates into the 'canonical
//          tile'. For a single dimension, we call it a 'canonical tile range'.
//
//      Implementation of flipping: ('tile' versus 'subtile')
//
//          We treat a 'flipped' dimension as a tiling of 2 adjacent 'subtile'
//          ranges.
//
//          In WrapModeFlipXY, the canonical tile consists of 4 subtiles. For
//          example, one of these subtiles spans from (ModulusWidth/2, 0) to
//          (ModulusWidth, ModulusHeight/2) in sample space. This one is flipped
//          in the u direction but not the v direction.
//
//          To get from 'canonical sample coordinates' (i.e. sample coordinates
//          translated to the canonical tile) into texture coordinates, we
//          translate from the canonical subtile we're in, flipping the result
//          according to which one we were in. (Look for tests like 'u >=
//          flipTileUMin'.)
//
//          For WrapModeFlipX and WrapModeFlipY, we do the above in one
//          dimension, and normal tiling in the other.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

DeclarePerfAcc(ColorSource_Image_ScanOp);

MtDefine(CIdentitySpan, MILRender, "CIdentitySpan");
MtDefine(CNearestNeighborSpan, MILRender, "CNearestNeighborSpan");
MtDefine(CBilinearSpan, MILRender, "CBilinearSpan");
MtDefine(CBilinearSpan_MMX, MILRender, "CBilinearSpan_MMX");
MtDefine(MBilinearSpanBuffer, MILRawMemory, "MBilinearSpanBuffer");

MtDefine(CNearestNeighborSpan_scRGB, MILRender, "CNearestNeighborSpan_scRGB");
MtDefine(CBilinearSpan_scRGB, MILRender, "CBilinearSpan_scRGB");

MtDefine(CConstantAlphaSpan, MILRender, "CConstantAlphaSpan");
MtDefine(CMaskAlphaSpan, MILRender, "CMaskAlphaSpan");

MtDefine(CConstantAlphaSpan_scRGB, MILRender, "CConstantAlphaSpan_scRGB");
MtDefine(CMaskAlphaSpan_scRGB, MILRender, "CMaskAlphaSpan_scRGB");

//+-----------------------------------------------------------------------------
//
//  Function:
//      getBilinearFilteredARGB
//
//  Synopsis:
//      From the ARGB value of the four corners, this returns the bilinearly
//      interpolated ARGB value.
//
//  Arguments:
//      colors - ARGB values at the four corners.
//
//      xFrac  - the fractional value of the x-coordinates.
//
//      yFrac  - the fractional value of the y-coordinates.
//
//      one, shift, half2, shift2 - the extra arguments used in the
//                                  calculations.
//
//  Returns:
//      ARGB: returns the biliearly interpolated ARGB.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE ARGB
getBilinearFilteredARGB(
    __in_bcount(sizeof(ARGB)*4) const ARGB (&colors)[4], // Reference to a const array of 4 ARGB values
    INT xFrac,
    INT yFrac,
    INT one,
    INT shift,
    INT half2,
    INT shift2)
{

    INT a[4], r[4], g[4], b[4];
    INT alpha, red, green, blue;

    for(INT k = 0; k < 4; k++)
    {
        ARGB c = colors[k];
        a[k] = MIL_COLOR_GET_ALPHA(c);
        r[k] = MIL_COLOR_GET_RED(c);
        g[k] = MIL_COLOR_GET_GREEN(c);
        b[k] = MIL_COLOR_GET_BLUE(c);
    }
    alpha =
        (
            (one - yFrac)*((a[0] << shift) + (a[1] - a[0])*xFrac)
            + yFrac*((a[2] << shift) + (a[3] - a[2])*xFrac)
            + half2
        ) >> shift2;
    red =
        (
            (one - yFrac)*((r[0] << shift) + (r[1] - r[0])*xFrac)
            + yFrac*((r[2] << shift) + (r[3] - r[2])*xFrac)
            + half2
        ) >> shift2;
    green =
        (
            (one - yFrac)*((g[0] << shift) + (g[1] - g[0])*xFrac)
            + yFrac*((g[2] << shift) + (g[3] - g[2])*xFrac)
            + half2
        ) >> shift2;
    blue =
        (
            (one - yFrac)*((b[0] << shift) + (b[1] - b[0])*xFrac)
            + yFrac*((b[2] << shift) + (b[3] - b[2])*xFrac)
            + half2
        ) >> shift2;

    return  MIL_COLOR
                (
                    (BYTE) alpha,
                    (BYTE) red,
                    (BYTE) green,
                    (BYTE) blue
                );
}

#if defined(_X86_)

//+-----------------------------------------------------------------------------
//
//  Function:
//      InterpolateWords_SSE2
//
//  Synopsis:
//      8-channel linear interpolation.
//
//  Arguments:
//      start    - values at the beginning of the interval
//      finish   - values at the end of the interval
//      progress - interpolation ratios
//
//      The words in 'start' and 'finish' should be in the range 0..255.
//      The words in 'progress' might be in the range 0..256.
//
//  Returns:
//      The vector of 16-bit words where each of them
//      is calculated independently from corresponding
//      component of arguments.
//      When progress.m_word[i] == 0   then result.m_word[i] = start.m_word[i].
//      When progress.m_word[i] == 256 then result.m_word[i] = finish.m_word[i].
//      Otherwise result.m_word[i] gets intermediate value.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE CXmmWords
InterpolateWords_SSE2(
    __in_ecount(1) CXmmWords const &start,
    __in_ecount(1) CXmmWords const &finish,
    __in_ecount(1) CXmmWords const &progress
    )
{
    // Given start and finish values are used to represent colors of two pixels:
    // AsBytes():
    // {0, alpha1, 0, red1, 0, green1, 0, blue1, 0, alpha0, 0, red0, 0, green0, 0, blue0};

    CXmmWords result = start;
    CXmmWords delta = ((finish - start) * progress + CXmmWords::Half8dot8()) >> 8;

    // Use byte addition to avoid garbage in high bits.
    // If we'll apply word addition, like "result += delta", negative values in delta
    // will cause carry to high bytes. Consequent PackWordsToBytes will saturate word
    // to 0xFF that's not the thing that we need.
    result.AddBytes(delta);
    return result;
}

#endif //defined(_X86_)

//+-----------------------------------------------------------------------------
//
//  Function:
//      getBilinearFilteredARGB_Fixed16
//
//  Synopsis:
//      Bilinear interpolation using 16.16 fractional position and rounding
//      constants.  Also has an optimized SSE2 version
//
//  Arguments:
//      colors - ARGB values at the four corners:
//
//      colors[0] | colors[1]
//      ----------+------------>"X"
//      colors[2] | colors[3]
//                v
//               "Y"
//
//      xFrac  - the fractional value of the x-coordinate in fixed 24.8 format
//
//      yFrac  - the fractional value of the y-coordinate in fixed 24.8 format
//
//  Returns:
//      ARGB: returns the biliearly interpolated ARGB.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE ARGB
getBilinearFilteredARGB_Fixed16(
    __in_bcount(sizeof(ARGB)*4) const ARGB (&colors)[4], // Reference to a const array of 4 ARGB values
    INT xFrac,
    INT yFrac)
{
    ARGB retval;

#if defined(_X86_)
    if (g_fUseSSE2)
    {
        CXmmWords xFrac4, yFrac8, color10, color32;

        color10.Load2DWords(colors[1], colors[0]);
        color32.Load2DWords(colors[3], colors[2]);

        xFrac4.LoadDWord(xFrac);
        yFrac8.LoadDWord(yFrac);

        xFrac4.ReplicateWord4Times<0>();
        yFrac8.ReplicateWord8Times<0>();

        color10.UnpackBytesToWords();
        color32.UnpackBytesToWords();

        // interpolation in the Y direction
        CXmmWords yResult = InterpolateWords_SSE2(color10, color32, yFrac8);

        // interpolation in the X direction
        CXmmWords xResult = InterpolateWords_SSE2(yResult, yResult.GetHighQWord(), xFrac4);

        xResult.PackWordsToBytes();
        retval = xResult.GetLowDWord();
    }
    else
#endif
    {
        const INT shift = 8;
        const INT shift2 = shift + shift;
        const INT one = 1 << shift;
        const INT half2 = 1 << (shift2 - 1);

        // Get interpolated value of 4 pixels
        retval = ::getBilinearFilteredARGB(
                        colors,
                        xFrac,
                        yFrac,
                        one,
                        shift,
                        half2,
                        shift2);
    }

    return retval;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      ClampPointToRectangle
//
//  Synopsis:
//      Clamps a point to the perimiter of a rectangle (from (0, 0) to (w-1,
//      h-1)).
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void ClampPointToRectangle(
    __inout_ecount(1) INT &x,
    __inout_ecount(1) INT &y,
    INT w,
    INT h
    )
{
    x = (x < 0) ? 0 : (x > w-1) ? w-1 : x;
    y = (y < 0) ? 0 : (y > h-1) ? h-1 : y;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      ApplyWrapMode
//
//  Synopsis:
//      Applies the correct wrap mode to a set of coordinates
//
//------------------------------------------------------------------------------
void ApplyWrapMode(
    INT WrapMode,
    __inout_ecount(1) INT &x,
    __inout_ecount(1) INT &y,
    INT w,
    INT h)
{
    INT xm, ym;
    switch(WrapMode)
    {
    case MilBitmapWrapMode::Extend:
        ClampPointToRectangle(x,y,w,h);
        break;

    case MilBitmapWrapMode::Tile:
        x = RemainderI(x, w);
        y = RemainderI(y, h);
        break;

    case MilBitmapWrapMode::FlipX:
        xm = RemainderI(x, w);
        if (((x-xm)/w) & 1)
        {
            x = w-1-xm;
        }
        else
        {
            x = xm;
        }
        y = RemainderI(y, h);
        break;

    case MilBitmapWrapMode::FlipY:
        x = RemainderI(x, w);
        ym = RemainderI(y, h);
        if (((y-ym)/h) & 1)
        {
            y = h-1-ym;
        }
        else
        {
            y = ym;
        }
        break;

    case MilBitmapWrapMode::FlipXY:
        xm = RemainderI(x, w);
        if (((x-xm)/w) & 1)
        {
            x = w-1-xm;
        }
        else
        {
            x = xm;
        }
        ym = RemainderI(y, h);
        if (((y-ym)/h) & 1)
        {
            y = h-1-ym;
        }
        else
        {
            y = ym;
        }
        break;

    case MilBitmapWrapMode::Border:
        // Don't do anything - the filter code will substitute the border
        // color when it detects border.

    default:
        break;
    }
}

//
//   Implementation of CResampleSpan
//

template <class TColor>
CResampleSpan<TColor>::CResampleSpan()
{
    m_pILock = NULL;
    m_pvBits = NULL;
    m_pIBitmap = NULL;
    m_pIBitmapSource = NULL;
    m_BorderColor.a = m_BorderColor.r = m_BorderColor.g = m_BorderColor.b = 0;
}

template <class TColor>
CResampleSpan<TColor>::~CResampleSpan()
{
    ReleaseExpensiveResources();
}

template <class TColor>
HRESULT CResampleSpan<TColor>::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
    )
{
    Assert(m_pILock == NULL);
    Assert(m_pvBits == NULL);
    Assert(m_pIBitmap == NULL);
    Assert(m_pIBitmapSource == NULL);

    HRESULT hr = S_OK;

    m_matDeviceToTexture = *pmatTextureHPCToDeviceHPC;
    if (!m_matDeviceToTexture.Invert())
    {
        // Return failure.
        //
        // This is an interim solution, to make SW behavior consistent with HW. Later steps:
        //   Task #21184: Draw nothing and return success.
        //   Task #15687: Handle non-invertible and near-non-invertible transforms correctly.

        IFC(WGXERR_NONINVERTIBLEMATRIX);
    }

    //
    // m_matDeviceToTexture now transforms from Device HPC to Texture HPC space,
    // but we need integer pixel center notation here. See AdjustForIPC() usage
    // in brushspan.cpp for more details.
    //
    m_matDeviceToTexture.AdjustForIPC();

    IFC(InitializeBitmapPointer(pIBitmapSource));

    // nothing can fail from now on.

    m_pIBitmapSource = pIBitmapSource;
    m_pIBitmapSource->AddRef();

    m_WrapMode = wrapMode;

    InitializeColors(
        pBorderColor
        );

Cleanup:
    RRETURN(hr);
}

template <class TColor>
HRESULT CResampleSpan<TColor>::InitializeBitmapPointer(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource
    )
{
    HRESULT hr = S_OK;

    Assert( m_pvBits == NULL );
    Assert( m_pILock == NULL );

    if (SUCCEEDED(hr))
    {
        MIL_THR(pIBitmapSource->GetSize(&m_nWidth, &m_nHeight));
    }

    if (   SUCCEEDED(hr)
        && (   m_nWidth  < 1 || m_nWidth  > INT_MAX
            || m_nHeight < 1 || m_nHeight > INT_MAX))
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(pIBitmapSource->GetPixelFormat(&m_PixelFormat));

        Assert(m_PixelFormat == MilPixelFormat::PBGRA32bpp ||
               m_PixelFormat == MilPixelFormat::BGR32bpp ||
               m_PixelFormat == MilPixelFormat::PRGBA128bppFloat);
    }

    WICRect rcLock = {0, 0, m_nWidth, m_nHeight};

    UINT cbBufferSize = 0;

    if (SUCCEEDED(hr))
    {
        IWGXBitmap *pBitmap = NULL;

        hr = THR(pIBitmapSource->QueryInterface(IID_IWGXBitmap, reinterpret_cast<void**>(&pBitmap)));
        if (SUCCEEDED(hr))
        {
            IWGXBitmapLock *pILock = NULL;

            MIL_THR(pBitmap->Lock(&rcLock, MilBitmapLock::Read, &pILock));
            if (SUCCEEDED(hr))
            {
                hr = pILock->GetStride(&m_cbStride);
                Assert(SUCCEEDED(hr));

                hr = pILock->GetDataPointer(&cbBufferSize, (BYTE**)&m_pvBits);
                Assert(SUCCEEDED(hr));

                m_pILock = pILock;

                m_pIBitmap = pBitmap;
                m_pIBitmap->AddRef();

            }

            pBitmap->Release();
        }
        else
        {
            MIL_THR(HrCalcDWordAlignedScanlineStride(m_nWidth, m_PixelFormat, OUT m_cbStride));

            if (SUCCEEDED(hr))
            {
                if (m_nHeight < INT_MAX / m_cbStride)
                {
                    VOID *pvBits =
                        GpMalloc(Mt(MBilinearSpanBuffer), m_cbStride * m_nHeight);
                    if (pvBits == NULL)
                    {
                        MIL_THR(E_OUTOFMEMORY);
                    }

                    if (SUCCEEDED(hr))
                    {
                        MIL_THR(pIBitmapSource->CopyPixels(&rcLock, m_cbStride, m_cbStride*m_nHeight, reinterpret_cast<BYTE*>(pvBits)));
                    }

                    if (SUCCEEDED(hr))
                    {
                        m_pvBits = pvBits;
                        pvBits = NULL;
                    }

                    if (pvBits != NULL)
                    {
                        GpFree(pvBits);
                    }
                }
                else
                {
                    MIL_THR(WGXERR_VALUEOVERFLOW);
                }
            }
        }
    }
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CResampleSpan<TColor>::InitializeColors
//
//  Synopsis:
//      Initialize color type specific members
//

template <>
void CResampleSpan<GpCC>::InitializeColors(
    __in_ecount_opt(1) const MilColorF *pBorderColor
    )
{
    if (pBorderColor != NULL)
    {

        m_BorderColor.argb = Premultiply(Convert_MilColorF_scRGB_To_MilColorB_sRGB(pBorderColor));
    }
}

template <>
void CResampleSpan<MilColorF>::InitializeColors(
    __in_ecount_opt(1) const MilColorF *pBorderColor
    )
{
    if (pBorderColor != NULL)
    {
        m_BorderColor = *pBorderColor;
        Premultiply(&m_BorderColor);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CResampleSpan::ReleaseExpensiveResources, CColorSource
//
//  Synopsis:
//      Release expensive resources
//
//------------------------------------------------------------------------------

template <class TColor>
VOID CResampleSpan<TColor>::ReleaseExpensiveResources()
{
    if (m_pILock != NULL)
    {
        Assert( m_pIBitmap != NULL );

        ReleaseInterface(m_pILock);
        m_pIBitmap->Release();

        m_pvBits = NULL;
        m_pIBitmap = NULL;
    }
    else
    {
        Assert( m_pIBitmap == NULL);  // Should only be non-NULL if m_pILock is non-NULL.

        // If m_pILock is NULL, then we own m_pvBits.

        if (m_pvBits != NULL)
        {
            GpFree(m_pvBits);
            m_pvBits = NULL;
        }
    }

    if (m_pIBitmapSource != NULL)
    {
        m_pIBitmapSource->Release();
        m_pIBitmapSource = NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Template Function:
//      ColorSource_Image_ScanOp
//
//  Synopsis:
//      Templatized function to "generate" specialized Resample ColorSource
//      ScanOps
//

template <class TResampleClass, class TColor>
VOID FASTCALL ColorSource_Image_ScanOp(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
MeasurePerf(ColorSource_Image_ScanOp, pPP->m_uiCount);
    TResampleClass *pColorSource =
        DYNCAST(TResampleClass, pSOP->m_posd);
    Assert(pColorSource);
    
    pColorSource->GenerateColors(pPP->m_iX, pPP->m_iY, pPP->m_uiCount, static_cast<TColor *>(pSOP->m_pvDest));
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CNearestNeighborSpan
//
//  Synopsis:
//      Resampling span using nearest pixel filtering.
//
//------------------------------------------------------------------------------

CNearestNeighborSpan::CNearestNeighborSpan()
    : CResampleSpan_sRGB()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CNearestNeighborSpan::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_NearestNeighbor_32bpp =
    ColorSource_Image_ScanOp<CNearestNeighborSpan, GpCC>;

ScanOpFunc CNearestNeighborSpan::GetScanOp() const
{
    return ColorSource_Image_NearestNeighbor_32bpp;
}

void CNearestNeighborSpan::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) GpCC *pargbDest
    ) const
{
    Assert(uiCount > 0);

    MilPoint2F pt1, pt2;
    pt1.X = (REAL) x;
    pt1.Y = pt2.Y = (REAL) y;
    pt2.X = (REAL) x + uiCount;

    m_matDeviceToTexture.Transform(&pt1, &pt1);
    m_matDeviceToTexture.Transform(&pt2, &pt2);

    // Convert to Fixed point notation - 16 bits of fractional precision.
    FIX16 dx, dy, x0, y0;
    x0 = CFloatFPU::Round(pt1.X * FIX16_ONE);
    y0 = CFloatFPU::Round(pt1.Y * FIX16_ONE);

    dx = CFloatFPU::Round(((pt2.X - pt1.X)*FIX16_ONE) / uiCount);
    dy = CFloatFPU::Round(((pt2.Y - pt1.Y)*FIX16_ONE) / uiCount);

    const ARGB *srcPtr0 = static_cast<ARGB*>(m_pvBits);
    INT stride = m_cbStride/sizeof(ARGB);

    INT ix;
    INT iy;

    // For all pixels in the destination span...
    for (UINT i=0; i<uiCount; i++)
    {
        // .. compute the position in source space.

        // round to the nearest neighbor
        ix = (x0 + FIX16_HALF) >> FIX16_SHIFT;
        iy = (y0 + FIX16_HALF) >> FIX16_SHIFT;

        // Make sure the pixel is within the bounds of the source before
        // accessing it.

        if( (ix >= 0) &&
            (iy >= 0) &&
            (ix < (INT)m_nWidth) &&
            (iy < (INT)m_nHeight) )
        {
            pargbDest->argb = *(srcPtr0+stride*iy+ix);
        }
        else
        {
            if (m_WrapMode != MilBitmapWrapMode::Border)
            {
                ApplyWrapMode(m_WrapMode, ix, iy, m_nWidth, m_nHeight);

                Assert (ix >= 0);
                Assert (iy >= 0);
                Assert (ix < (INT)m_nWidth);
                Assert (iy < (INT)m_nHeight);

                pargbDest->argb = *(srcPtr0+stride*iy+ix);
            }
            else
            {
                // This means that this source pixel is outside of the valid
                // bits in the source. (edge condition)
                *pargbDest = m_BorderColor;
            }
        }

        pargbDest++;

        // Update source position
        x0 += dx;
        y0 += dy;
    }
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBilinearSpan
//
//  Synopsis:
//      Resampling span using bilinear filtering.
//
//------------------------------------------------------------------------------

CBilinearSpan::CBilinearSpan()
    : CResampleSpan_sRGB()
{
    m_matDeviceToTexture.SetToIdentity();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::Initialize
//
//  Synopsis:
//      Initializes this filter with a source bitmap & other filtering
//      parameters.
//
//------------------------------------------------------------------------------
HRESULT CBilinearSpan::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
    )
{
    HRESULT hr = S_OK;

    MIL_THR(CResampleSpan_sRGB::Initialize(
        pIBitmapSource,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC
        ));

    if (SUCCEEDED(hr))
    {
        InitializeFixedPointState();
    }

    return hr;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::InitializeFixedPointState
//
//  Synopsis:
//      Initializes the fixed point variables needed for texture mapping.
//
//------------------------------------------------------------------------------
void CBilinearSpan::InitializeFixedPointState()
{
    M11 = CFloatFPU::Round(m_matDeviceToTexture.GetM11() * (1L << 16));
    M12 = CFloatFPU::Round(m_matDeviceToTexture.GetM12() * (1L << 16));
    M21 = CFloatFPU::Round(m_matDeviceToTexture.GetM21() * (1L << 16));
    M22 = CFloatFPU::Round(m_matDeviceToTexture.GetM22() * (1L << 16));
    Dx  = CFloatFPU::Round(m_matDeviceToTexture.GetDx() * (1L << 16));
    Dy  = CFloatFPU::Round(m_matDeviceToTexture.GetDy() * (1L << 16));

    SetDeviceOffset();

    UIncrement = M11;
    VIncrement = M12;

    ModulusWidth = (m_nWidth << 16);
    ModulusHeight = (m_nHeight << 16);

    canonicalWidth = m_nWidth;
    canonicalHeight = m_nHeight;


    // When the u,v coordinates have the pixel in the last row or column
    // of the texture space, the offset of the pixel to the right and the
    // pixel below (for bilinear filtering) is the following (for tile modes)
    // because they wrap around the texture space.

    // The XEdgeIncrement is the byte increment of the pixel to the right of
    // the pixel on the far right hand column of the texture. In tile mode,
    // we want the pixel on the same scanline, but in the first column of the
    // texture hence 4bytes - stride
    XEdgeIncrement = 4 * (1 - m_nWidth);

    // The YEdgeIncrement is the byte increment of the pixel below the current
    // pixel when the current pixel is in the last scanline of the texture.
    // In tile mode the correct pixel is the one directly above this one in
    // the first scanline - hence the increment below:

    YEdgeIncrement = -(INT)(m_nHeight-1)*(INT)(m_cbStride);

    if ((m_WrapMode == MilBitmapWrapMode::FlipX) ||
            (m_WrapMode == MilBitmapWrapMode::FlipXY))
    {
        ModulusWidth *= 2;
        canonicalWidth *= 2;

        // Wrap increment is zero for Flip mode
        XEdgeIncrement = 0;
    }
    if ((m_WrapMode == MilBitmapWrapMode::FlipY) ||
            (m_WrapMode == MilBitmapWrapMode::FlipXY))
    {
        ModulusHeight *= 2;
        canonicalHeight *= 2;

        // Wrap increment is zero for Flip mode
        YEdgeIncrement = 0;
    }

    // Wrapmode border or extend:
    //
    //  |<------ModulusWidth----->|
    //  |<------flipTileUMin----->|
    //  |<--inflipTileUMax----->| |
    //  |<--inTileUMax--------->| |
    //  |                       | |
    //  |                       V V
    //  0 x x x x x x x x x x x x 1 x x x x x x x x x x x x 2 (tile width of 13 pels)
    //
    // Wrapmode tile or flip
    //
    //  |<------------------ModulusWidth------------------->|
    //  |<-----------------inflipTileUMax---------------->| |
    //  |<------flipTileUMin----->|                       | |
    //  |<--inTileUMax--------->| |                       | |
    //  |                       | |                       | |
    //  |                       V V                       V V
    //  0 x x x x x x x x x x x x 1 x x x x x x x x x x x x 2 (tile width of 13 pels)


    // initialize precomputed tile constraints.
    inTileUMax = static_cast<__int64>(m_nWidth - 1) << 16;
    inTileVMax = static_cast<__int64>(m_nHeight - 1) << 16;
    flipTileUMin =  static_cast<__int64>(m_nWidth) << 16;
    flipTileVMin =  static_cast<__int64>(m_nHeight) << 16;
    inflipTileUMax = ModulusWidth - (1 << 16);
    inflipTileVMax = ModulusHeight - (1 << 16);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::SetDeviceOffset
//
//  Synopsis:
//      The device-to-texture translations (Dx,Dy) can overflow the 16.16 field,
//      if the scaling and position are large enough.
//      To work around this, use a "position-independent" transform by using
//      device coordinates relative to an "origin" near to the destination.
//
//------------------------------------------------------------------------------
VOID CBilinearSpan::SetDeviceOffset()
{
    if (Dx != 0x80000000 && Dy != 0x80000000 &&
        Dx != 0x7fffffff && Dy != 0x7fffffff)
    {
        // if no overflow, use the surface origin.  No mapping adjustments needed.
        XDeviceOffset = 0;
        YDeviceOffset = 0;
        return;
    }

    // The position of the destination isn't available here (it's available
    // about 10 callers upstream).  As a heuristic, use the point in surface
    // space that maps to (0,0) in texture space.

    CMILMatrix matTextureToSurface;
    if (matTextureToSurface.Invert(m_matDeviceToTexture))
    {
        XDeviceOffset = CFloatFPU::Round(matTextureToSurface.GetDx());
        YDeviceOffset = CFloatFPU::Round(matTextureToSurface.GetDy());

        // GenerateColors subtracts the "origin" before applying the
        // mapping, so adjust the existing mapping by adding a translation.
        float fx = static_cast<float>(XDeviceOffset);
        float fy = static_cast<float>(YDeviceOffset);
        CMILMatrix adjustedDeviceToTexture( 1,  0,  0,  0,
                                            0,  1,  0,  0,
                                            0,  0,  1,  0,
                                            fx, fy, 0,  1 );
        adjustedDeviceToTexture.Multiply(m_matDeviceToTexture);

        // the adjusted mapping only differs from the existing one in the
        // translation components
        Dx  = CFloatFPU::Round(adjustedDeviceToTexture.GetDx() * (1L << 16));
        Dy  = CFloatFPU::Round(adjustedDeviceToTexture.GetDy() * (1L << 16));
    }

    // if the surface-to-texture mapping isn't invertible, or if the pre-image
    // of texture point (0,0) is still too far away from the destination,
    // (Dx,Dy) is still wrong due to overflow of the 16.16 field.  In practice,
    // neither of these conditions is likely to happen, so don't try to fix them.
    // Instead, just live with the "failure to render" bug.
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_Bilinear_32bppPARGB_or_32bppRGB =
    ColorSource_Image_ScanOp<CBilinearSpan, GpCC>;

ScanOpFunc CBilinearSpan::GetScanOp() const
{
    return ColorSource_Image_Bilinear_32bppPARGB_or_32bppRGB;
}


// -------------------------------------------------
// Inline Functions or Non-CBilinearSpan Member Functions

inline ARGB getLinearFilteredARGB(
    __in_bcount(sizeof(ARGB)*2) const ARGB (&colors)[2], // Reference to a const array of 2 ARGB values
    INT Frac)
{
    INT a[2], r[2], g[2], b[2];
    INT alpha, red, green, blue;
    INT shift = 8;  // same accuracy as MMX

    for(INT k = 0; k < 2; k++)
    {
        ARGB c = colors[k];

        a[k] = MIL_COLOR_GET_ALPHA(c);
        r[k] = MIL_COLOR_GET_RED(c);
        g[k] = MIL_COLOR_GET_GREEN(c);
        b[k] = MIL_COLOR_GET_BLUE(c);
    }

    alpha = (((a[0] << shift) + (a[1] - a[0])*Frac)+ 0x80) >> shift;
    red   = (((r[0] << shift) + (r[1] - r[0])*Frac)+ 0x80) >> shift;
    green = (((g[0] << shift) + (g[1] - g[0])*Frac)+ 0x80) >> shift;
    blue  = (((b[0] << shift) + (b[1] - b[0])*Frac)+ 0x80) >> shift;

    return  MIL_COLOR
                (
                    (BYTE) alpha,
                    (BYTE) red,
                    (BYTE) green,
                    (BYTE) blue
                );
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      IsInRange
//
//  Synopsis:
//      Returns whether the single-dimension texture coordinate is in the given
//      range, such that sMin <= s < sMax.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE bool IsInRange(INT s, INT sMin, INT sMax)
{
    return (sMin <= s) && (s < sMax);
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      SpanLengthTo
//
//  Synopsis:
//      For a single texture dimension (u or v - represented by 's').
//
//      Returns the length of the longest pixel span starting at sStart, moving
//      towards the given limit, which would not exceed the limit.
//
//      Not verified for large textures
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE UINT SpanLengthTo(
    INT sStart,
    INT sIncrement,
    INT sLimit
    )
{
    Assert(sIncrement != 0); // Precondition

    // Precondition: The sign of sIncrement must match that of sLimit - sStart.
    //               (Otherwise we'd be moving away from the limit.)

    Assert( ((sIncrement > 0) && (sStart <= sLimit)) ||
            ((sIncrement < 0) && (sStart >= sLimit)));

    // 1 for the first pixel, plus the number of times we can increment
    // before s would exceed sLimit.
    UINT length = 1 + (sLimit - sStart)/sIncrement;

    return length;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      number_pix_insideTexture_s
//
//  Synopsis:
//      For a single texture dimension (u or v - represented by 's').
//
//      Returns the length of the longest pixel span which will map into the
//      given range. Infinity is returned as INT_MAX.
//
//      The range is given using sMin and sMax, such that sMin <= s < sMax.
//
//      The first pixel must already be in the given range. The mapping is given
//      as sStart (for the first pixel) and sIncrement (the delta for subsequent
//      pixels in the span).
//
//      Not verified for large textures
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE UINT number_pix_insideTexture_s(
    INT sStart,
    INT sIncrement,
    INT sMax,
    INT sMin
    )
{
    // Precondition:  sStart is in the given range.
    Assert(IsInRange(sStart, sMin, sMax));

    UINT length;

    // We want:                sMin <= s <  sMax
    // This is equivalent to:  sMin <= s <= sMax - 1

    if (sIncrement > 0)
    {
        // Increasing towards the limit of sMax - 1
        length = SpanLengthTo(sStart, sIncrement, sMax - 1);
    }
    else if (sIncrement < 0)
    {
        // Decreasing towards the limit of sMin.
        length = SpanLengthTo(sStart, sIncrement, sMin);
    }
    else
    {
        // Infinity. (We're not moving).
        length = INT_MAX;
    }

    Assert(length > 0); // Postcondition

    return length;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      number_pix_inside_Texture
//
//  Synopsis:
//      Returns the length of the longest pixel span (up to uiCount), which will
//      map into the given area of the texture space.
//
//      The area is given by uMin, uMax, vMin, and vMax, which define
//      inclusive-exclusive texture coordinate ranges.
//
//      The first pixel in the span maps to the given (u,v), which must already
//      be in the given area. The delta for subsequent texels is (uIncrement,
//      vIncrement).
//
//      Not verified for large textures
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __range(1,uiCount) INT number_pix_inside_Texture(
    __range(uMin,uMax-1) INT u,
    __range(vMin,vMax-1) INT v,
    INT uIncrement,
    INT vIncrement,
    INT uMax,
    INT vMax,
    INT uMin,
    INT vMin,
    __range(1,INT_MAX) UINT uiCount
    )
{
    // Precondition:  The first texel is inside the given area.
    Assert(IsInRange(u, uMin, uMax));
    Assert(IsInRange(v, vMin, vMax));

    Assert(uiCount > 0);  // Precondition

    // Calculate for each dimension, then take the minimum.

    UINT uLength = number_pix_insideTexture_s(u, uIncrement, uMax, uMin);
    UINT vLength = number_pix_insideTexture_s(v, vIncrement, vMax, vMin);

    UINT length = min(uLength,vLength);

    // Clamp to uiCount
    if (length > uiCount)
    {
        length = uiCount;
    }

    Assert(length > 0);  // Postcondition

    // Postcondition:  The last texel in the span we've calculated, is still
    //                  inside the given area.

#if DBG
    INT uLast = u + (length-1) * uIncrement;
    INT vLast = v + (length-1) * vIncrement;

    Assert(IsInRange(uLast, uMin, uMax));
    Assert(IsInRange(vLast, vMin, vMax));
#endif

    return length;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      getdistancefromtexture
//
//  Synopsis:
//      For a single texture dimension (u or v - represented by 's').
//
//      Returns the length of the longest pixel span which will map outside the
//      'canonical range'. Infinity is returned as INT_MAX.
//
//      The 'canonical range' is all values s, such that 0 <= s < sMax. It may
//      be empty (i.e. sMax == 0).
//
//      The mapping is given as sStart (for the first pixel) and sIncrement (the
//      delta for subsequent pixels in the span).
//
//      If the first pixel is inside the canonical range, then infinity will be
//      returned.
//
//      Not verified for large textures
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE INT getdistancefromtexture(INT sStart, INT sMax, INT sIncrement)
{
    Assert(sMax >= 0);  // Precondition.

    int length;

    if (sStart < 0 && sIncrement > 0)
    {
        // Increasing towards the limit of -1.
        length = SpanLengthTo(sStart, sIncrement, -1);
    }
    else if (sStart >= sMax && sIncrement < 0)
    {
        // Decreasing towards the limit of sMax.
        length = SpanLengthTo(sStart, sIncrement, sMax);
    }
    else
    {
        // Infinity. This covers three cases:
        //
        // 1) sIncrement is 0 - we're not moving.
        //
        // 2) We're moving further away from the canonical range.
        //
        // 3) sStart is inside the canonical range. We return infinity so that
        //    this texture dimension is ignored by number_pix_outside_Texture.
        //
        //    That function protects against the case when both dimensions are
        //    inside the canonical range.

        length = INT_MAX;
    }

    Assert(length > 0); // Postcondition

    return length;
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      number_pix_outside_Texture
//
//  Synopsis:
//      Returns the length of a pixel span (up to uiCount), which will map
//      outside the 'canonical area' of the sample space. This does not always
//      calculate the *longest* such span - instead, it may return an
//      under-estimate.
//
//      The canonical area is from (0, 0) inclusive, to (uMax, vMax) exclusive.
//
//      The first pixel in the span maps to the given (u,v), which must already
//      be outside the given area. The delta for subsequent texels is
//      (uIncrement, vIncrement).
//
//      Not verified for large textures
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE __range(1,uiCount) INT number_pix_outside_Texture(
    INT u,
    INT v,
    INT uIncrement,
    INT vIncrement,
    INT uMax,
    INT vMax,
    __range(1,INT_MAX) UINT uiCount
    )
{
    // Precondition:  The first texel is outside the given area.
    //Assert(! (IsInRange(u, uMin, uMax) && IsInRange(v, vMin, vMax)));
    Assert(! (IsInRange(u, 0, uMax) && IsInRange(v, 0, vMax)));

    Assert(uiCount > 0);  // Precondition

    UINT uLength = getdistancefromtexture(u,uMax,uIncrement);
    UINT vLength = getdistancefromtexture(v,vMax,vIncrement);

    UINT length = min(uLength,vLength);

    if (uiCount < length)
    {
        length = uiCount;
    }


    Assert(length > 0);  // Postcondition

    // Postcondition:  The last texel in the span we've calculated, is still
    //                  outside the given area.

#if DBG
    INT uLast = u + (length-1) * uIncrement;
    INT vLast = v + (length-1) * vIncrement;

    //Assert(! (IsInRange(uLast, uMin, uMax) && IsInRange(vLast, vMin, vMax)));
    Assert(! (IsInRange(uLast, 0, uMax) && IsInRange(vLast, 0, vMax)) );
#endif

    return length;
}



// -------------------------------------------------
// CBilinearSpan Member Functions


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::IsLargeTexture
//
//  Synopsis:
//      Returns true if the texture falls outside the range which can be
//      accelerated using existing 16.16 code
//
//  Arguments:
//
//  Notes:
//      The current code should be valid for textures up to and including 0x3FFF
//      in width or height; the canonical width may be twice this (0x7FFE) and
//      this value is used internally in fixed 1.15.16 format (0x7FFE0000)
//
//------------------------------------------------------------------------------
bool CBilinearSpan::IsLargeTexture() const
{
    if (m_nWidth > 0x3FFF || m_nHeight > 0x3FFF)
        return true;

    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::IsLargeSpan
//
//  Synopsis:
//      Returns true if any of the sample positions would exceed the 16.16 range
//      used by the accelerated methods
//
//  Arguments:
//      [IN] u: x coordinate of sample at the beginning of the span
//      [IN] v: y coordinate of sample at the end of the span
//      [IN] uiCount  - number of pixels in the span
//
//  Notes:
//      We need to check both endpoints of the span and ensure they are within
//      the bounding area used by IsLargeTexture (above).
//
//------------------------------------------------------------------------------
bool CBilinearSpan::IsLargeSpan(__int64 u, __int64 v, UINT uiCount) const
{
    if ((_abs64(u) > 0x3FFE0000)
        || (_abs64(v) > 0x3FFE0000)
        || (_abs64(u+uiCount*UIncrement) > 0x3FFE0000)
        || (_abs64(v+uiCount*VIncrement) > 0x3FFE0000))
    {
        return true;
    }
    else
        return false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::GenerateColors
//
//  Synopsis:
//      Calls either a C or SSE2 version of the integer bilinear filter
//      operation.
//
//  Arguments:
//      [IN] x - x coordinate of first pixel.
//
//      [IN] y - y coordinate of first pixel.
//
//      [IN] uiCount  - number of pixels bilinearspan on.
//
//      [IN] pargbDest - pointer to the Dest.
//
//  Notes:
//      For details about 'tiles' and 'subtiles' -- i.e. implementation of
//      tiling and flipping wrap modes -- see notes at top of file.
//
//------------------------------------------------------------------------------
void CBilinearSpan::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) GpCC *pgpccDest
    ) const
{
    Assert((((ULONG_PTR) m_pvBits) & 3) == 0);
    Assert((m_cbStride & 3) == 0);

    static const UINT SSE_THRESHOLD = 1; // SSE_THRESHOLD is the value where the setup costs of SSE2
                                         // are less than the performance gain of using SSE2

    // Transform an array of points using the matrix v' = v M:
    //
    //                                  ( M11 M12 0 )
    //      (vx', vy', 1) = (vx, vy, 1) ( M21 M22 0 )
    //
    // All (u, v) calculations are done in 16.16 fixed point.

    __int64 u64, v64;

    ARGB *pargbDest = &pgpccDest->argb;

    //
    // Transform x & y into texture coordinates
    //
    // No overflow should happen here so long as x and y are coordinates on a
    // render target surface. The maximum that the result could be is
    //      2^32 * 2^28 + 2^32 + 2^28 + 2^32
    // This only requires 62 bits of precision and we have 63 available.
    //
    C_ASSERT(SURFACE_RECT_MAX <= (1 << 27));
    Assert(x >= 0); // overagressive- routine could work with values as low as SURFACE_RECT_MIN
    Assert(y >= 0); // overagressive- routine could work with values as low as SURFACE_RECT_MIN
    Assert(x <= SURFACE_RECT_MAX);
    Assert(y <= SURFACE_RECT_MAX);

    u64 = M11 * static_cast<__int64>(x-XDeviceOffset) + M21 * static_cast<__int64>(y-YDeviceOffset) + Dx;
    v64 = M12 * static_cast<__int64>(x-XDeviceOffset) + M22 * static_cast<__int64>(y-YDeviceOffset) + Dy;

    // check if texture or span endpoints would lie outside safe canonical range
    if ( IsLargeTexture() || IsLargeSpan(u64, v64, uiCount) )
    {
        // Only Handle_OutsideTexture_C has large texture support.  The
        // optimized methods used Fixed16 (1.15.16), which limits their range
        // to 0x3FFF in the worst case (flipping).
        while (uiCount > 0)
        {
            UINT N = Handle_OutsideTexture_C(u64, v64, uiCount, pargbDest);
            uiCount -= N;
            pargbDest += N;
        }
    }
    else
    {
        // IsLargeSpan ensures u & v are within the 1.15.16 range
        INT u = static_cast<INT>(u64);
        INT v = static_cast<INT>(v64);

        // Tiled/flipped cases:
        //     This loop typically processes the span overlaying one texture per
        //     pass (so if your span spans the texture tiled 4 times, expect 4
        //     iterations).
        // Extend and Border cases:
        //     The number of iterations ranges from 1 to about 4, depending on
        //     exactly how the span crosses the texture or the space outside.
        while (uiCount > 0)
        {

            // When dealing with flipped or tiled mode, we should
            // wrap the sample coordinates to within the (possibly flipped) texture.
            //
            // This includes boundary cases that must be handled separately later.
            // We do this to prevent the first pixel in a span from always running through the (slower) fallback,
            // as well as to ensure that we correctly update flipping modes when crossing over tiles (having used
            // optimized code previously
            if (m_WrapMode != MilBitmapWrapMode::Extend && m_WrapMode != MilBitmapWrapMode::Border)
            {
                WrapPositionAndFlipState(u,v);
            }

            // If the sample doesn't lie within a canonical subtile,
            // we fall back to considerably slower interpolation operations
            // (these are at least a third as fast as the SSE2 versions below)

            if (   (u < 0)
                || (v < 0)
                || IsOnBorder(u,v))
            {
                int N = 0;


                if (m_WrapMode != MilBitmapWrapMode::Border && m_WrapMode != MilBitmapWrapMode::Extend)
                {
                    // Often this code is only producing a single pixel
                    // (exceptions including small U/v increments,
                    // cases where we're walking near an edge, etc

                    // Postcondition:  If uiCount-N is greater than zero,
                    // Handle_OutsideTexture_C must not return a pixel which lies outside the
                    // texture
                    N = Handle_OutsideTexture_C(u, v, uiCount, pargbDest);
                }
                else
                {
                    // number_pix_outside_texture may underestimate the actual number of pixels in the texture,
                    // forcing us to do the check below.  This underestimation allows us to get by with a far
                    // simpler calculation than the traditional ray-box intercept

                    // These assertions are guaranteed by IsLargeTexture
                    Assert (inTileUMax <= INT_MAX);
                    Assert (inTileVMax <= INT_MAX);

                    N = number_pix_outside_Texture(
                        u,
                        v,
                        UIncrement,
                        VIncrement,
                        static_cast<INT>(inTileUMax),
                        static_cast<INT>(inTileVMax),
                        uiCount
                    );

                    // Optimization for cases where we are outside the texture.  This is common, for example, when
                    // dealing with one-dimensional textures (e.g. for gradients).  At least 50% faster than the old
                    // MMX version, far faster than the C fallback
#if defined(_X86_)
                    if (g_fUseSSE2 && m_WrapMode == MilBitmapWrapMode::Extend && uiCount > SSE_THRESHOLD)
                    {
                        // handles a 1 dimensional interpolation in SSE2 (intrinsics)
                        Handle_Extend_OutsideTexture_SSE2(u, v, N, pargbDest);

                        u += N*UIncrement;
                        v += N*VIncrement;
                    }
                    else
                    {
#endif
                        N = Handle_OutsideTexture_C(u, v, N, pargbDest);
#if defined(_X86_)
                    }
#endif

                    // required due to number_pix_outside_Texture() underestimate, as noted above
                    if (u >= inTileUMax || v >= inTileVMax || v < 0 || u < 0 )
                    {
                        uiCount -= N;
                        pargbDest += N;
                        continue;
                    }

                }

                uiCount -= N;
                pargbDest += N;
                // Small optimization; if we're done with the span, don't bother trying to continue the
                // expensive tests below
                if (uiCount == 0)
                {
                    break;
                }
            }

            // calculate number of pixels in flip-tile and not at the boundary

            int horiz_min;
            int vert_min;
            int horiz_max;
            int vert_max;
            int isFlipped = 0;

            if (u >= flipTileUMin)
            {
                // These assertions are guaranteed by IsLargeTexture
                Assert(inflipTileUMax <= INT_MAX);
                Assert(flipTileUMin <= INT_MAX);

                horiz_max = static_cast<INT>(inflipTileUMax);
                horiz_min = static_cast<INT>(flipTileUMin);
                isFlipped = 1;
            }
            else
            {
                // This assertions is guaranteed by IsLargeTexture
                Assert(inTileUMax <= INT_MAX);
                horiz_max = static_cast<INT>(inTileUMax);
                horiz_min = 0;
            }

            if (v >= flipTileVMin)
            {
                // These assertions are guaranteed by IsLargeTexture
                Assert(inflipTileVMax <= INT_MAX);
                Assert(flipTileVMin <= INT_MAX);
                vert_max = static_cast<INT>(inflipTileVMax);
                vert_min = static_cast<INT>(flipTileVMin);
                isFlipped = 1;
            }
            else
            {
                // This assertions is guaranteed by IsLargeTexture
                Assert(inTileVMax <= INT_MAX);
                vert_max = static_cast<INT>(inTileVMax);
                vert_min = 0;
            }

            Assert(u >= horiz_min); // Precondition:  horiz_min <= u < horiz_max
            Assert(u <  horiz_max);
            Assert(v >= vert_min);  // Precondition:  vert_min <= v < vert_max
            Assert(v <  vert_max);

            UINT N = number_pix_inside_Texture(
                u,
                v,
                UIncrement,
                VIncrement,
                horiz_max,
                vert_max,
                horiz_min,
                vert_min,
                uiCount
                );
            // Postcondition:  N > 0
            // Postcondition:  horiz_min <= u+(N-1)*UIncrement < horiz_max
            // Postcondition:  vert_min <= v+(N-1)*VIncrement < vert_max

#if defined(_X86_)
            if (g_fUseSSE2 && N > SSE_THRESHOLD)
            {
                // If either the x or y flip modes are set, use the slower version that supports flipping
                if (isFlipped)
                {
                    FlippedTile_Interpolation_SSE2(u, v, N, pargbDest);
                }
                else
                {
                    InTile_Interpolation_SSE2(u, v, N, pargbDest);
                }
            }
            else
#endif
            {
                FlippedTile_Interpolation_C(u, v, N, pargbDest);
            }

            uiCount -= N;
            pargbDest += N;
            u += N*UIncrement;
            v += N*VIncrement;

            // Having updated u/v, we need to update flip flag for the next
            // iteration; this is done at the top of the loop
        }
    }

    return;
}

// 64-bit safe version
MIL_FORCEINLINE __int64 getinnofliptile64(__int64 s, UINT tilesize)
{
    Assert(tilesize > 0);

    //INT integerpos = s >> 16;

    // This complex code is a fairly significant optimization for 32-bit targets
    // we'd far prefer to write
    // INT integerpos = s >> 16;
    // please revert to the above if the MS compiler generates better code
    INT integerpos = (((UINT *)&s)[0] >> 16) | (((INT *)&s)[1] << 16);

    // This is equivalent to:
    //   if ((s >= 0) && (s < tilesize))
    // but quicker to evaluate.
    if ( (UINT) integerpos < (UINT) tilesize )
    {
        // do nothing
    }
    // This is equivalent to:
    //   if ((s >= 0) && (s < 2*tilesize))
    // but quicker to evaluate.
    else if ((UINT) integerpos < (UINT) 2*tilesize)
    {
        s -= ((__int64)tilesize) << 16;
    }
    // This is equivalent to:
    //   if ((s <= 0) && (s >= -tilesize))
    // but quicker to evaluate.
    else if ((UINT) (-integerpos) <= (UINT) tilesize)
    {
        s += ((__int64)tilesize) << 16;
    }
    else
    {
        s = RemainderI(s, (((__int64)tilesize) << 16));
    }

    return s;
}


MIL_FORCEINLINE INT getinnofliptile(INT s, INT tilesize)
{
    Assert(tilesize > 0);

    // This is equivalent to:
    //   if ((s >= 0) && (s < tilesize))
    // but quicker to evaluate.
    if ( (UINT) s < (UINT) tilesize )
    {
        // do nothing
    }
    // This is equivalent to:
    //   if ((s >= 0) && (s < 2*tilesize))
    // but quicker to evaluate.
    else if ((UINT) s < (UINT) 2*tilesize)
    {
        s -= tilesize;
    }
    // This is equivalent to:
    //   if ((s <= 0) && (s >= -tilesize))
    // but quicker to evaluate.
    else if ((UINT) (-s) <= (UINT) tilesize)
    {
        s += tilesize;
    }
    else
    {
        s = RemainderI(s, tilesize);
    }

    return s;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::WrapPositionAndFlipState
//
//  Synopsis:
//      If u or v are outside the canonical tile, modifies them to the
//      equivalent position inside the canonical tile.
//
//      This function should not be called in WrapModeBorder or WrapModeExtend
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE void CBilinearSpan::WrapPositionAndFlipState(
    __inout_ecount(1) INT &u,
    __inout_ecount(1) INT &v) const
{
    Assert(!IsLargeTexture());

    u = getinnofliptile(u, ModulusWidth);
    v = getinnofliptile(v, ModulusHeight);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::IsOnBorder
//
//  Synopsis:
//      Determines if the pixel is in the border region of a tile
//
//      This function should not be called in WrapModeBorder or WrapModeExtend
//
//------------------------------------------------------------------------------
//
//  precondition:
//      u, v are in the canonical tile for the current flipping mode.
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE bool CBilinearSpan::IsOnBorder(__int64 u, __int64 v) const
{
    if ((u < inflipTileUMax) && (v < inflipTileVMax))
    {
        if ((u >= flipTileUMin || u < inTileUMax) && (v >= flipTileVMin || v < inTileVMax))
        {
            return false;
        }
    }
    return true;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::Handle_OutsideTexture_C
//
//  Synopsis:
//      Performs common logic of casting u & v to and from an __int64 before and
//      after calling Handle_OutsideTexture_C.  This method should only be
//      called by code-paths after u & v have been verified to stay within the
//      FIXED16 range in IsLargeSpan
//
//------------------------------------------------------------------------------
__range(0,uiCount)
UINT CBilinearSpan::Handle_OutsideTexture_C(
    __inout_ecount(1) INT &u,
    __inout_ecount(1) INT &v,
    UINT uiCount,
    __out_ecount_part(uiCount,uiCount-return) ARGB *pargbDest
    ) const
{
    __int64 u64 = u;
    __int64 v64 = v;

    uiCount = Handle_OutsideTexture_C(u64, v64, uiCount, pargbDest);

    Assert(u64 <= INT_MAX);
    Assert(v64 <= INT_MAX);

    u = static_cast<INT>(u64);
    v = static_cast<INT>(v64);

    return uiCount;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::Handle_OutsideTexture_C
//
//  Synopsis:
//      Handles bilinear interpolation on samples in the the texture interior,
//      boundary or exterior for all wrapmodes. No assumption is made that the
//      sample lies within the texture
//
//------------------------------------------------------------------------------
__range(0,uiCount)
UINT CBilinearSpan::Handle_OutsideTexture_C(
    __inout_ecount(1) __int64 &u,
    __inout_ecount(1) __int64 &v,
    UINT uiCount,
    __out_ecount_part(uiCount,uiCount-return) ARGB *pargbDest
    ) const
{
    ARGB *srcPtr = static_cast<ARGB*>(m_pvBits);
    const UINT uiCountIn = uiCount;
    const ARGB *srcPtr1, *srcPtr2;
    INT w = m_nWidth;
    INT h = m_nHeight;
    INT stride = m_cbStride/sizeof(ARGB);

    // Formation of pixel to interpolate on
    // (notation used below)
    //
    //  A  |  B
    //  -  +  -
    //  C  |  D

    while (uiCount > 0)
    {
        // Get values of four texels

        // This complex code is a fairly significant optimization for 32-bit targets
        // we'd far prefer to write
        // INT x1 = u >> 16;
        // INT y1 = v >> 16;
        INT x1 = (((UINT *)&u)[0] >> 16) | (((INT *)&u)[1] << 16);
        INT y1 = (((UINT *)&v)[0] >> 16) | (((INT *)&v)[1] << 16);

        INT x2=x1+1;
        INT y2=y1+1;
        // Get fractional values
        //INT xFrac = (u >> 8) & 0xff;
        //INT yFrac = (v >> 8) & 0xff;
        INT xFrac = (((INT)u) >> 8) & 0xff;
        INT yFrac = (((INT)v) >> 8) & 0xff;

        if (m_WrapMode == MilBitmapWrapMode::Extend)
        {
            if ( (x1 >= 0) && (x1 < (INT) m_nWidth) &&
                 (x2 >= 0) && (x2 < (INT) m_nWidth) &&
                 (y1 >= 0) && (y1 < (INT) m_nHeight) &&
                 (y2 >= 0) && (y2 < (INT) m_nHeight))
            {
                // get srcPtr to A & C lines
                srcPtr1 = srcPtr + stride*y1;
                srcPtr2 = srcPtr + stride*y2;

                ARGB colors[4];

                colors[0] = *(srcPtr1 + x1);  // A
                colors[1] = *(srcPtr1 + x2);  // B
                colors[2] = *(srcPtr2 + x1);  // C
                colors[3] = *(srcPtr2 + x2);  // D

                *pargbDest++ = getBilinearFilteredARGB_Fixed16(colors, xFrac, yFrac);
            }
            else
            {
                // Adjust texel values if lie outside border to border value
                ClampPointToRectangle(x1,y1,w,h);
                ClampPointToRectangle(x2,y2,w,h);

                // get srcPtr to A & C lines
                srcPtr1 = srcPtr + stride*y1;

                if (x1 == x2)   //  A = B, C = D
                {
                    ARGB colors[2];
                    srcPtr2 = srcPtr + stride*y2;
                    colors[0] = *(srcPtr1 + x1);  // A
                    colors[1] = *(srcPtr2 + x1);  // C
                    *pargbDest++ = ::getLinearFilteredARGB(colors,yFrac);
                }
                else            // y1 == y2   A = C, B = D
                {
                    ARGB colors[2];

                    colors[0] = *(srcPtr1 + x1);  // A
                    colors[1] = *(srcPtr1 + x2);  // B
                    *pargbDest++ = ::getLinearFilteredARGB(colors, xFrac);
                }
            }

            u += UIncrement;
            v += VIncrement;
            uiCount--;
        }
        else if (m_WrapMode == MilBitmapWrapMode::Border)
        {
            if ( (x2 >= 0) && (x1 < (INT) m_nWidth) &&
                 (y2 >= 0) && (y1 < (INT) m_nHeight) )
            {
                ARGB colors[4];

                //preinitialize variable to bordercolor
                colors[0] = m_BorderColor.argb;
                colors[1] = m_BorderColor.argb;
                colors[2] = m_BorderColor.argb;
                colors[3] = m_BorderColor.argb;

                srcPtr1 = NULL;    //reinitialize
                srcPtr2 = NULL;    //reinitialize

                // The next statement is equivalent to:
                //   if ((y1 >= 0) || (y1 < m_nHeight))
                // but quicker to evaluate.
                if ( (UINT) y1 < (UINT) m_nHeight )
                {
                    srcPtr1 = srcPtr + stride*y1;
                }

                // The next statement is equivalent to:
                //   if ((y2 >= 0) || (y2 < m_nHeight))
                // but quicker to evaluate.
                if ( (UINT) y2 < (UINT) m_nHeight )
                {
                    srcPtr2 = srcPtr + stride*y2;
                }

                // Check values of x1 with y1 & y2
                // The next statement is equivalent to:
                //   if ((x1 >= 0) || (x1 < m_nWidth))
                // but quicker to evaluate.
                if ( (UINT) x1 < (UINT) m_nWidth )
                {
                    if (srcPtr1)     // y1 and x1 inside
                    {
                        colors[0] = *(srcPtr1 + x1);
                    }

                    if (srcPtr2)     // y2 and x1 inside
                    {
                        colors[2] = *(srcPtr2 + x1);
                    }
                }

                // Check values of x1 with y1 & y2
                // The next statement is equivalent to:
                //   if ((x2 >= 0) || (x2 < m_nWidth))
                // but quicker to evaluate.
                if ( (UINT) x2 < (UINT) m_nWidth )
                {
                    if (srcPtr1)     // y1 and x2 inside
                    {
                        colors[1] = *(srcPtr1 + x2);
                    }

                    if (srcPtr2)     // y2 and x2 inside
                    {
                        colors[3] = *(srcPtr2 + x2);
                    }
                }

                *pargbDest++ = getBilinearFilteredARGB_Fixed16(colors, xFrac, yFrac);
            }
            else
            {
                *pargbDest++ = m_BorderColor.argb;
            }

            u += UIncrement;
            v += VIncrement;
            uiCount--;

        }
        else // flipping or tiling
        {
            x1 = getinnofliptile(x1, canonicalWidth);
            if (static_cast<UINT>(x1) >= m_nWidth)
            {
                x1 = canonicalWidth-1-x1;
            }

            x2 = getinnofliptile(x2, canonicalWidth);
            if (static_cast<UINT>(x2) >= m_nWidth)
            {
                x2 = canonicalWidth-1-x2;
            }

            y1 = getinnofliptile(y1, canonicalHeight);
            if (static_cast<UINT>(y1) >= m_nHeight)
            {
                y1 = canonicalHeight-1-y1;
            }

            y2 = getinnofliptile(y2, canonicalHeight);
            if (static_cast<UINT>(y2) >= m_nHeight)
            {
                y2 = canonicalHeight-1-y2;
            }


            // get srcPtr to A & C lines
            srcPtr1 = srcPtr + stride*y1;
            srcPtr2 = srcPtr + stride*y2;

            ARGB colors[4];

            colors[0] = *(srcPtr1 + x1);  // A
            colors[1] = *(srcPtr1 + x2);  // B
            colors[2] = *(srcPtr2 + x1);  // C
            colors[3] = *(srcPtr2 + x2);  // D

            *pargbDest++ = getBilinearFilteredARGB_Fixed16(colors, xFrac, yFrac);

            u += UIncrement;
            v += VIncrement;
            uiCount--;

            u = getinnofliptile64(u, canonicalWidth);
            v = getinnofliptile64(v, canonicalHeight);

            /// early out if we're not on the border any more.
            // This early out hurts us somewhat for large textures.
            if (!IsOnBorder(u,v))
            {
                break;
            }
        } // flipping or tiling
    } // while (uiCount > 0)

    Assert(!(((u>>16) >= canonicalWidth || (v>>16) >= canonicalHeight || v < 0 || u < 0) && (uiCount > 0)));

    // Return number of elements remaining to fill
    return (uiCountIn - uiCount);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::FlippedTile_Interpolation_C
//
//  Synopsis:
//      Handles bilinear interpolation within the canonical tile, for wrapmodes
//      FlipX, FlipY, and FlipXY.
//
//------------------------------------------------------------------------------
void CBilinearSpan::FlippedTile_Interpolation_C(
    INT u,
    INT v,
    UINT uiCount,
    __out_ecount_full(uiCount) ARGB *pargbDest
    ) const
{
    Assert(!IsLargeTexture());

    ARGB *srcPtr0 = static_cast<ARGB*>(m_pvBits);
    INT stride = m_cbStride/sizeof(ARGB);
    INT w = m_nWidth;
    INT h = m_nHeight;

    for (UINT i = 0; i < uiCount; i++)
    {
        INT x1 = u >> 16;   // x offset of A
        INT y1 = v >> 16;   // y offset of A

        int flipped_x = 0;
        if (x1 >= w)
        {
            x1 = 2*w - x1 - 2;
            flipped_x = 1;
        }
        int flipped_y = 0;
        if (y1 >= h)
        {
            y1 = 2*h - y1 - 2;
            flipped_y = 1;
        }

        INT x2 = x1+1;
        INT y2 = y1+1;

        INT xFrac = (u >> 8) & 0xff;
        INT yFrac = (v >> 8) & 0xff;

        const ARGB *srcPtr1, *srcPtr2;

        if (flipped_y)
        {
            // get srcPtr to A & C lines
            srcPtr1 = srcPtr0 + stride*y2;
            srcPtr2 = srcPtr0 + stride*y1;
        }
        else
        {
            // get srcPtr to A & C lines
            srcPtr1 = srcPtr0 + stride*y1;
            srcPtr2 = srcPtr0 + stride*y2;
        }

        ARGB colors[4];

        if (flipped_x)
        {
            colors[0] = *(srcPtr1 + x2);  // A
            colors[1] = *(srcPtr1 + x1);  // B
            colors[2] = *(srcPtr2 + x2);  // C
            colors[3] = *(srcPtr2 + x1);  // D
        }
        else
        {
            colors[0] = *(srcPtr1 + x1);  // A
            colors[1] = *(srcPtr1 + x2);  // B
            colors[2] = *(srcPtr2 + x1);  // C
            colors[3] = *(srcPtr2 + x2);  // D
        }

        *pargbDest++ = getBilinearFilteredARGB_Fixed16(colors, xFrac, yFrac);

        u += UIncrement;
        v += VIncrement;
    }
}

#if defined(_X86_)
//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::Handle_Extend_OutsideTexture_SSE2
//
//  Synopsis:
//      Performs GenerateColors for pixels situated in boundary
//
//  Notes:
//      Handles bilinear interpolation on pixels mapping outside the texture
//      boundary only for the wrapmode Extend.  This is a linear intepolation.
//
//------------------------------------------------------------------------------
void CBilinearSpan::Handle_Extend_OutsideTexture_SSE2(
    INT u,
    INT v,
    UINT uiCount,
    __out_ecount_full(uiCount) ARGB *pargbDest
    ) const
{
    Assert(!IsLargeTexture());

    INT_PTR ipSrcPtr0 = reinterpret_cast<INT_PTR>(m_pvBits);
    INT w_minus1 = m_nWidth - 1;
    INT h_minus1 = m_nHeight - 1;

    // load variables with 128 byte quantities
    CXmmDWords uv, uv_inc, uv_max, stride;

    uv.Load4DWords(v+0x10000,u+0x10000,v,u);
    uv_inc.Load4DWords(VIncrement,UIncrement,VIncrement,UIncrement);
    uv_max.Load4DWords(h_minus1,w_minus1,h_minus1,w_minus1);
    stride.Load4DWords(0,m_cbStride,0,m_cbStride);

    for (UINT i=0; i < uiCount; i++)
    {
        CXmmDWords uv_int = uv >> 16;                       // get integer part of (v2, u2, v1, u1)

        uv_int.AsWords().Max(CXmmValue::Zero());            // clamp low
        uv_int.AsWords().Min(uv_max);                       // clamp high
            // uv_int = (v2_clamped, u2_clamped, v1_clamped, u1_clamped)

        INT x1 = uv_int.GetLowDWord();                      // convert u1_clamped to int

        CXmmDWords v_int = uv_int.AsQWords() >> 32;         // (0, v2_clamped, 0, v1_clamped)
        v_int *= stride;                                    // (0, v2_offset, 0, v1_offset)

        uv_int.DuplicateHighQWord();                        // (v2_clamped, u2_clamped, v2_clamped, u2_clamped)

        INT x2 = uv_int.GetLowDWord();                      // convert u2_clamped to int
        INT y1 = v_int.GetLowDWord();                       // convert v1_clamped to int
        v_int.DuplicateHighQWord();                         // (0, v2_offset, 0, v2_offset)
        INT y2 = v_int.GetLowDWord();                       // convert v2_offset to int

        if (x1 == x2 && y1 == y2)
        {
            // we're in a corner.
            const INT *pi = reinterpret_cast<int *>(ipSrcPtr0 + y1 + 4*x1);
            pargbDest[i] = pi[0];
        }
        else
        {
            // We're on a side.  Either y1 != y2 or x1 != x2.
            // It is the responsibility of caller's code.
            Assert(x1 == x2 || y1 == y2);

            // get the filtering coefficient = uf or vf
            CXmmWords frac = uv.AsWords() >> 8;
            if (y1 != y2)
            {
                frac.ReplicateWord4Times<2>();       // frac = [ufrac, ufrac, ufrac,...]
            }
            else
            {
                frac.ReplicateWord4Times<0>();       // frac = [vfrac, vfrac, vfrac,...]
            }

            // load the two texels.
            CXmmWords texel_1, texel_2;

            const INT *pi1 = reinterpret_cast<int *>(ipSrcPtr0 + y1 + 4*x1);  // get first address
            texel_1.LoadDWord(pi1[0]);                                  // load first texel
            texel_1.UnpackBytesToWords();                               // texel_1 now holds first texel

            const INT *pi2 = reinterpret_cast<int *>(ipSrcPtr0 + y2 + 4*x2);  // get second address
            texel_2.LoadDWord(pi2[0]);                                  // load second texel
            texel_2.UnpackBytesToWords();                               // texel_2 now holds second texel

            // interpolation
            CXmmWords final_value = InterpolateWords_SSE2(texel_1, texel_2, frac);
            final_value.PackWordsToBytes();

            pargbDest[i] = final_value.GetLowDWord();  // write the final value to dest
        }

        uv += uv_inc;                 // inc u,v by uInc and vInc
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::InTile_Interpolation_SSE2_Nonrotated
//
//  Synopsis:
//      Handles bilinear interpolation on pixels mapping inside the texture
//      boundary.  The function does bilinear interpolation on two pixels per
//      pass for uiCount pixels.  It takes advantage of the 128 bit registers of
//      SSE2.  It also has an optimization for non-rotated textures.
//
//  Notes:
//      15-20% gains for nonrotated, nonflipped textures
//
//      This has to be in a second function for now; merging it with the
//      function below plays havoc with intrinsics generation (on the MS v.14
//      compiler).
//
//------------------------------------------------------------------------------
void CBilinearSpan::InTile_Interpolation_SSE2_Nonrotated(
    INT u,
    INT v,
    UINT uiCount,
    __out_ecount_full(uiCount) ARGB *pargbDest
    ) const
{
    Assert(!IsLargeTexture());
    Assert(UIncrement == 0x10000 && VIncrement == 0);

    // Set up variables with 128 bit quantities
    CXmmDWords uv;
    uv.Load4DWords(v, u+0x10000, v, u);

    const int pixels_ahead=6;
    INT prefetchoffset = pixels_ahead*4;

    // get addresses for two src pixels, src + y*stride + x*4
    INT_PTR a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 (v >> 16) * static_cast<INT_PTR>(m_cbStride) +
                 (u >> 16) * 4;
    // assuming INT_PTR a1 = a0 + 4;

    // Pixels formation that are used to grab texels from texture
    //
    //   P2 | P3
    //   ---+---
    //   P0 | P1
    //
    // The variables a0 and a1 point to P0 for first and second pixels.

    // Variables u and v are in 16.16 fixed point format.
    // uv.m_words[0] = fractional part of u
    // uv.m_words[1] = integer part of u
    // and so on.
    // Following shift converts fractional parts to format 8.8.
    CXmmWords frac = uv.AsWords() >> 8;

    CXmmWords yfrac = frac;
    yfrac.ReplicateWord8Times<2>();         // holds the yFrac for both pixels

    CXmmWords xfrac = frac;
    xfrac.ReplicateWord8Times<0>();         // get xFrac for both pixels

    while (uiCount >= 2)
    {
        CXmmWords cp1_P1P0, cp1_P3P2, cp2_P1P0, cp2_P3P2;

        // Load 4 neighboring texels for the first pixel
        cp1_P1P0.LoadQWord(reinterpret_cast<__int64*>(a0                 ));
        cp1_P3P2.LoadQWord(reinterpret_cast<__int64*>(a0 +     m_cbStride));

        // interpolation in the y direction on pixel 1 (cp1)
        cp1_P1P0.UnpackBytesToWords();
        cp1_P3P2.UnpackBytesToWords();
        CXmmWords cp1_yResult = InterpolateWords_SSE2(cp1_P1P0, cp1_P3P2, yfrac);

        // Load 4 neighboring texels for the second pixel
        cp2_P1P0.LoadQWord(reinterpret_cast<__int64*>(a0 + 4             ));
        cp2_P3P2.LoadQWord(reinterpret_cast<__int64*>(a0 + 4 + m_cbStride));

        {
            // get addresses for next two src pixels
            a0 += 8;
            // assuming a1 = a0 + 4;
        }

        // interpolation in the y direction on pixel 2 (cp2)
        cp2_P1P0.UnpackBytesToWords();
        cp2_P3P2.UnpackBytesToWords();
        CXmmWords cp2_yResult = InterpolateWords_SSE2(cp2_P1P0, cp2_P3P2, yfrac);

        // prefetch the next cacheline
        // This gets us 6% for larger textures
        // Note that address can turn to be out of texture and maybe
        // outside of legal memory, but this is okay for prefetching.
        _mm_prefetch(reinterpret_cast<const char*>(a0 + 4 + prefetchoffset), _MM_HINT_T0);

        // shuffle values to prepare for interpolation in x direction
        CXmmWords P3P1, P2P0;
        P3P1.LoadHighQWords(cp2_yResult, cp1_yResult);  // P3P1 = [p31',p31]
        P2P0.LoadLowQWords(cp2_yResult, cp1_yResult);   // P2P0 = [p02',p02]

        // do the final interpolation in x direction on both pixels
        CXmmWords xResult = InterpolateWords_SSE2(P2P0, P3P1, xfrac);

        xResult.PackWordsToBytes();
        xResult.StoreQWord(reinterpret_cast<__int64*>(pargbDest));

        uiCount -= 2;
        pargbDest += 2;         // increment the dest ptr
    }

    // If we have an odd pixel at the end:
    if (uiCount != 0)
    {
        // preload the src addresses for the next two pixels
        CXmmWords P1P0, P3P2;

        P1P0.LoadQWord(reinterpret_cast<__int64*>(a0           ));
        P3P2.LoadQWord(reinterpret_cast<__int64*>(a0+m_cbStride));

        // interpolation in the y direction on last pixel
        P1P0.UnpackBytesToWords();
        P3P2.UnpackBytesToWords();
        CXmmWords yResult = InterpolateWords_SSE2(P1P0, P3P2, yfrac);

        // interpolation in the X direction on last pixel
        CXmmWords xResult = InterpolateWords_SSE2(yResult, yResult.GetHighQWord(), xfrac);

        xResult.PackWordsToBytes();
        pargbDest[0] = xResult.GetLowDWord();
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::InTile_Interpolation_SSE2
//
//  Synopsis:
//      Handles bilinear interpolation on pixels mapping inside the texture
//      boundary.  The function does bilinear interpolation on two pixels per
//      pass for uiCount pixels.  It takes advantage of the 128 bit registers of
//      SSE2.
//
//------------------------------------------------------------------------------
void CBilinearSpan::InTile_Interpolation_SSE2(
    INT u,
    INT v,
    UINT uiCount,
    __out_ecount_full(uiCount) ARGB *pargbDest
    ) const
{
    Assert(!IsLargeTexture());

    if (UIncrement == 0x10000 && VIncrement == 0)
    {
        InTile_Interpolation_SSE2_Nonrotated(u, v, uiCount, pargbDest);
        return;
    }

    const int pixels_ahead=6;
    INT prefetchoffset = ((UIncrement*pixels_ahead)>>16)*4 + ((VIncrement*pixels_ahead)>>16)*m_cbStride;

    // Set up variables with 128 bit quantities
    CXmmDWords uv, uv_inc;
    uv.Load4DWords(v+VIncrement, u+UIncrement, v, u);

    // Set up uv_inc with following: (2*VIncrement, 2*UIncrement, 2*VIncrement, 2*UIncrement).
    // Ensure that VIncrement follows UIncrement so we can fetch them together
    C_ASSERT(offsetof(CBilinearSpan, UIncrement) + sizeof(UIncrement) == offsetof(CBilinearSpan, VIncrement));
    uv_inc.LoadQWord(reinterpret_cast<__int64 const *>(&UIncrement));
    uv_inc.DuplicateLowQWord();
    uv_inc <<= 1;
    // uv_inc now contains (2*VIncrement, 2*UIncrement, 2*VIncrement, 2*UIncrement)

    // get addresses for two src pixels, src + y*stride + x*4
    INT_PTR a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 (v >> 16) * static_cast<INT_PTR>(m_cbStride) +
                 (u >> 16) * 4;
    INT_PTR a1 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 ((v + VIncrement) >> 16) * static_cast<INT_PTR>(m_cbStride) +
                 ((u + UIncrement) >> 16) * 4;

    // Pixels formation that are used to grab texels from texture
    //
    //   P2 | P3
    //   ---+---
    //   P0 | P1
    //
    // The variables a0 and a1 point to P0 for first and second pixels.

    while (uiCount >= 2)
    {
        CXmmWords cp1_P1P0, cp1_P3P2, cp2_P1P0, cp2_P3P2;

        // Load 4 neighboring texels for the first pixel
        cp1_P1P0.LoadQWord(reinterpret_cast<__int64*>(a0           ));
        cp1_P3P2.LoadQWord(reinterpret_cast<__int64*>(a0+m_cbStride));

        // Variables u, v and increments are in 16.16 fixed point format.
        // uv.m_words[0] = fractional part of u
        // uv.m_words[1] = integer part of u
        // and so on.
        // Following shift converts fractional parts to format 8.8.
        CXmmWords frac = uv.AsWords() >> 8;

        CXmmWords p1_yfrac = frac;
        p1_yfrac.ReplicateWord8Times<2>();      // holds the yFrac for pixel 1

        CXmmWords p2_yfrac = frac;
        p2_yfrac.ReplicateWord8Times<6>();      // holds the yFrac for pixel 2

        CXmmWords xfrac = frac;
        xfrac.ReplicateWord4Times<0>();         // get xFrac for pixel 1 in low 64 bits
        xfrac.ReplicateWord4Times<4>();         // get xFrac for pixel 2 in high 64 bits

        // interpolation in the y direction on pixel 1 (cp1)
        cp1_P1P0.UnpackBytesToWords();
        cp1_P3P2.UnpackBytesToWords();
        CXmmWords cp1_yResult = InterpolateWords_SSE2(cp1_P1P0, cp1_P3P2, p1_yfrac);

        // Load 4 neighboring texels for the second pixel
        cp2_P1P0.LoadQWord(reinterpret_cast<__int64*>(a1           ));
        cp2_P3P2.LoadQWord(reinterpret_cast<__int64*>(a1+m_cbStride));

        {
            // increment u and v
            u += 2*UIncrement;
            v += 2*VIncrement;

            // get addresses for next two src pixels
            a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 (v >> 16) * static_cast<INT_PTR>(m_cbStride) +
                 (u >> 16) * 4;
            a1 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 ((v + VIncrement) >> 16) * static_cast<INT_PTR>(m_cbStride) +
                 ((u + UIncrement) >> 16) * 4;
        }

        // interpolation in the y direction on pixel 2 (cp2)
        cp2_P1P0.UnpackBytesToWords();
        cp2_P3P2.UnpackBytesToWords();
        CXmmWords cp2_yResult = InterpolateWords_SSE2(cp2_P1P0, cp2_P3P2, p2_yfrac);

        // prefetch the next cacheline
        // This gets us 6% for larger textures
        // Note that address can turn to be out of texture and maybe
        // outside of legal memory, but this is okay for prefetching.
        _mm_prefetch(reinterpret_cast<const char*>(a1 + prefetchoffset), _MM_HINT_T0);

        // shuffle values to prepare for interpolation in x direction
        CXmmWords P3P1, P2P0;
        P3P1.LoadHighQWords(cp2_yResult, cp1_yResult);  // P3P1 = [p31',p31]
        P2P0.LoadLowQWords(cp2_yResult, cp1_yResult);   // P2P0 = [p02',p02]

        // do the final interpolation in x direction on both pixels
        CXmmWords xResult = InterpolateWords_SSE2(P2P0, P3P1, xfrac);

        xResult.PackWordsToBytes();
        xResult.StoreQWord(reinterpret_cast<__int64*>(pargbDest));

        pargbDest += 2;         // increment the dest ptr

        uiCount -= 2;

        uv += uv_inc;           // u += uInc, v += vInc
    }

    // If we have an odd pixel at the end:
    if (uiCount != 0)
    {
        // preload the src addresses for the next two pixels
        CXmmWords P1P0, P3P2;

        P1P0.LoadQWord(reinterpret_cast<__int64*>(a0           ));
        P3P2.LoadQWord(reinterpret_cast<__int64*>(a0+m_cbStride));

        CXmmWords frac = uv.AsWords() >> 8;

        CXmmWords yfrac = frac;
        yfrac.ReplicateWord8Times<2>();      // holds the yFrac for pixel

        CXmmWords xfrac = frac;
        xfrac.ReplicateWord4Times<0>();         // get xFrac for pixel

        // interpolation in the y direction on last pixel
        P1P0.UnpackBytesToWords();
        P3P2.UnpackBytesToWords();
        CXmmWords yResult = InterpolateWords_SSE2(P1P0, P3P2, yfrac);

        // interpolation in the X direction on last pixel
        CXmmWords xResult = InterpolateWords_SSE2(yResult, yResult.GetHighQWord(), xfrac);

        xResult.PackWordsToBytes();
        pargbDest[0] = xResult.GetLowDWord();
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan::FlippedTile_Interpolation_SSE2
//
//  Synopsis:
//      Handles bilinear interpolation on pixels mapping with the texture
//      falling inside a flipped tile for wrapmodes FlipX, FlipY, and FlipXY.
//      Function written in SSE2 intrinsics.
//
//------------------------------------------------------------------------------
void CBilinearSpan::FlippedTile_Interpolation_SSE2(
    INT u,
    INT v,
    UINT uiCount,
    __out_ecount_full(uiCount) ARGB *pargbDest
    ) const
{
    Assert(!IsLargeTexture());

    INT w = m_nWidth;
    INT h = m_nHeight;

    INT uInc = UIncrement;
    INT vInc = VIncrement;
    INT stride = m_cbStride;

    INT W2m2 = 2*w-2;
    INT H2m2 = 2*h-2;

    // set up variables with 128 bit quantities
    CXmmDWords uv, uv_inc;
    uv.Load4DWords(0,0,v,u);    // [0,0,vIntvFrac,uIntuFrac]

    // Ensure that VIncrement follows UIncrement so we can fetch them together
    C_ASSERT(offsetof(CBilinearSpan, UIncrement) + sizeof(UIncrement) == offsetof(CBilinearSpan, VIncrement));
    uv_inc.LoadQWord(reinterpret_cast<__int64 const *>(&UIncrement)); //(0,0,vInc, uInc)

    INT x = u >> 16;    // x offset of A
    INT y = v >> 16;    // y offset of A

    int flip_x = 0;
    if (x >= w)
    {
        x = W2m2 - x;
        flip_x = 1;
    }
    int flip_y = 0;
    if (y >= h)
    {
        y = H2m2 - y;
        flip_y = 1;
    }

    // obtain address for the first upper left texel
    INT_PTR a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                 y * static_cast<INT_PTR>(stride) +
                 x * 4;

    //
    // There's about a 20% advantage to separating these three cases from the branchy version
    // This is largely due to relatively poor code generated by the Intrinsics compiler
    //

    const int pixels_ahead=6;
    if (flip_x & flip_y)
    {
        // Prefetch buys us 1.5% geomean in these functions
        INT prefetchoffset = -((uInc*pixels_ahead)>>16)*4 - ((vInc*pixels_ahead)>>16)*m_cbStride;

        while (uiCount > 0)
        {
            CXmmWords P1P0, P3P2;
            // load the values of the four texels (2 rows 2 texels each)
            P1P0.LoadQWord(reinterpret_cast<__int64*>(a0         ));
            P3P2.LoadQWord(reinterpret_cast<__int64*>(a0 + stride));

            // set up variables for interpolation
            CXmmWords frac = uv.AsWords() >> 8;

            CXmmWords yFrac = frac;
            yFrac.ReplicateWord8Times<2>();         // 8 times v-fraction, fixed 8.8

            CXmmWords xFrac = frac;
            xFrac.ReplicateWord4Times<0>();         // 4 times u-fraction, fixed 8.8

            // get src addresses for next pixel in loop
            if (uiCount > 1)
            {
                u += uInc;
                v += vInc;
                x = u >> 16;    // x offset of A
                y = v >> 16;    // y offset of A

                // convert to the flipped value
                x = W2m2 - x;
                y = H2m2 - y;

                // obtain address for the first upper left pixel
                a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                    y * static_cast<INT_PTR>(stride) +
                    x * 4;
            }

            // prefetch the next cacheline
            _mm_prefetch(((const char *)a0)+prefetchoffset, _MM_HINT_T0);

            // interpolation in the y direction
            P1P0.UnpackBytesToWords();
            P3P2.UnpackBytesToWords();
            CXmmWords yResult = InterpolateWords_SSE2(P3P2, P1P0, yFrac);

            // interpolation in the x direction
            CXmmWords xResult = InterpolateWords_SSE2(yResult.GetHighQWord(), yResult, xFrac);
            xResult.PackWordsToBytes();

            // Poor compiler treatment of the following DW sized store costs us about 5% performance
            *pargbDest++ = xResult.GetLowDWord(); // store the 32 bit value in the dest

            uv += uv_inc;
            uiCount--;
        }
    }
    else if (flip_x)
    {
        INT prefetchoffset = -((uInc*pixels_ahead)>>16)*4 + ((vInc*pixels_ahead)>>16)*m_cbStride;
        while (uiCount > 0)
        {
            CXmmWords P1P0, P3P2;
            // load the values of the four texels (2 rows 2 texels each)
            P1P0.LoadQWord(reinterpret_cast<__int64*>(a0         ));
            P3P2.LoadQWord(reinterpret_cast<__int64*>(a0 + stride));

            // set up variables for interpolation
            CXmmWords frac = uv.AsWords() >> 8;

            CXmmWords yFrac = frac;
            yFrac.ReplicateWord8Times<2>();         // 8 times v-fraction, fixed 8.8

            CXmmWords xFrac = frac;
            xFrac.ReplicateWord4Times<0>();         // 4 times u-fraction, fixed 8.8

            // get src addresses for next pixel in loop
            if (uiCount > 1)
            {
                u += uInc;
                v += vInc;
                x = u >> 16;    // x offset of A
                y = v >> 16;    // y offset of A

                // convert to the flipped value
                x = W2m2 - x;

                // obtain address for the first upper left pixel
                a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                    y * static_cast<INT_PTR>(stride) +
                    x * 4;
            }

            // prefetch the next cacheline
            _mm_prefetch(((const char *)a0)+prefetchoffset, _MM_HINT_T0);

            // interpolation in the y direction
            P1P0.UnpackBytesToWords();
            P3P2.UnpackBytesToWords();
            CXmmWords yResult = InterpolateWords_SSE2(P1P0, P3P2, yFrac);

            // interpolation in the x direction
            CXmmWords xResult = InterpolateWords_SSE2(yResult.GetHighQWord(), yResult, xFrac);
            xResult.PackWordsToBytes();

            // Poor compiler treatment of the following DW sized store costs us about 5% performance
            *pargbDest++ = xResult.GetLowDWord(); // store the 32 bit value in the dest

            uv += uv_inc;
            uiCount--;
        }
    }
    else //flipy
    {
        INT prefetchoffset = ((uInc*pixels_ahead)>>16)*4 - ((vInc*pixels_ahead)>>16)*m_cbStride;

        while (uiCount > 0)
        {
            CXmmWords P1P0, P3P2;
            // load the values of the four texels (2 rows 2 texels each)
            P1P0.LoadQWord(reinterpret_cast<__int64*>(a0         ));
            P3P2.LoadQWord(reinterpret_cast<__int64*>(a0 + stride));

            // set up variables for interpolation
            CXmmWords frac = uv.AsWords() >> 8;

            CXmmWords yFrac = frac;
            yFrac.ReplicateWord8Times<2>();         // 8 times v-fraction, fixed 8.8

            CXmmWords xFrac = frac;
            xFrac.ReplicateWord4Times<0>();         // 4 times u-fraction, fixed 8.8

            // get src addresses for next pixel in loop
            if (uiCount > 1)
            {
                u += uInc;
                v += vInc;
                x = u >> 16;    // x offset of A
                y = v >> 16;    // y offset of A

                // convert to the flipped value
                y = H2m2 - y;

                // obtain address for the first upper left pixel
                a0 = reinterpret_cast<INT_PTR>(m_pvBits) +
                    y * static_cast<INT_PTR>(stride) +
                    x * 4;
            }

            // prefetch the next cacheline
            _mm_prefetch(((const char *)a0)+prefetchoffset, _MM_HINT_T0);

            // interpolation in the y direction
            P1P0.UnpackBytesToWords();
            P3P2.UnpackBytesToWords();
            CXmmWords yResult = InterpolateWords_SSE2(P3P2, P1P0, yFrac);

            // interpolation in the x direction
            CXmmWords xResult = InterpolateWords_SSE2(yResult, yResult.GetHighQWord(), xFrac);
            xResult.PackWordsToBytes();

            // Poor compiler treatment of the following DW sized store costs us about 5% performance
            *pargbDest++ = xResult.GetLowDWord(); // store the 32 bit value in the dest

            uv += uv_inc;
            uiCount--;
        }
    }
}
#endif

// -------------------------------------------------
// End of CBilinearSpan Member Functions

CUnoptimizedBilinearSpan::CUnoptimizedBilinearSpan()
    : CResampleSpan_sRGB()
{
    m_matDeviceToTexture.SetToIdentity();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CUnoptimizedBilinearSpan::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_UnoptimizedBilinear_32bppPARGB_or_32bppRGB =
    ColorSource_Image_ScanOp<CUnoptimizedBilinearSpan, GpCC>;

ScanOpFunc CUnoptimizedBilinearSpan::GetScanOp() const
{
    return ColorSource_Image_UnoptimizedBilinear_32bppPARGB_or_32bppRGB;
}


void CUnoptimizedBilinearSpan::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) GpCC *pargbDest
    ) const
{
    Assert(uiCount > 0);

    MilPoint2F pt1, pt2;
    pt1.X = (REAL) x;
    pt1.Y = pt2.Y = (REAL) y;
    pt2.X = (REAL) x + uiCount;

    m_matDeviceToTexture.Transform(&pt1, &pt1);
    m_matDeviceToTexture.Transform(&pt2, &pt2);

    ARGB *srcPtr0 = static_cast<ARGB*>(m_pvBits);
    INT stride = m_cbStride/sizeof(ARGB);
    UINT i;

    REAL dx, dy, x0, y0;
    INT ix, iy;

    x0 = pt1.X;
    y0 = pt1.Y;

    dx = (pt2.X - pt1.X)/uiCount;
    dy = (pt2.Y - pt1.Y)/uiCount;

    // Filtered image stretch.

    const ARGB *srcPtr1, *srcPtr2;
    INT shift = 11;  // (2*shift + 8 < 32 bits --> shift < 12)
    INT shift2 = shift + shift;
    INT one = 1 << shift;
    INT half2 = 1 << (shift2 - 1);
    INT xFrac, yFrac;
    ARGB colors[4];

    INT x1, y1, x2, y2;
    for (i = 0; i < uiCount; i++)
    {
        iy = GpFloor(y0);
        ix = GpFloor(x0);
        xFrac = CFloatFPU::Round((x0 - ix)*one);
        yFrac = CFloatFPU::Round((y0 - iy)*one);

        x1=ix;
        x2=ix+1;
        y1=iy;
        y2=iy+1;

        if (((UINT)ix >= (m_nWidth-1) ) ||
            ((UINT)iy >= (m_nHeight-1)) )
        {
            ApplyWrapMode(m_WrapMode, x1, y1, m_nWidth, m_nHeight);
            ApplyWrapMode(m_WrapMode, x2, y2, m_nWidth, m_nHeight);
        }

        if (y1 >= 0 && y1 < (INT) m_nHeight)
        {
            srcPtr1 = srcPtr0 + stride*y1;
        }
        else
        {
            srcPtr1 = NULL;
        }

        if (y2 >= 0 && y2 < (INT) m_nHeight)
        {
            srcPtr2 = srcPtr0 + stride*y2;
        }
        else
        {
            srcPtr2 = NULL;
        }

        if (x1 >= 0 && x1 < (INT) m_nWidth)
        {
            if(srcPtr1)
            {
                colors[0] = *(srcPtr1 + x1);
            }
            else
            {
                colors[0] = m_BorderColor.argb;
            }

            if(srcPtr2)
            {
                colors[2] = *(srcPtr2 + x1);
            }
            else
            {
                colors[2] = m_BorderColor.argb;
            }
        }
        else
        {
            colors[0] = m_BorderColor.argb;
            colors[2] = m_BorderColor.argb;
        }

        if (x2 >= 0 && x2 < (INT) m_nWidth)
        {
            if (srcPtr1)
            {
                colors[1] = *(srcPtr1 + x2);
            }
            else
            {
                colors[1] = m_BorderColor.argb;
            }

            if (srcPtr2)
            {
                colors[3] = *(srcPtr2 + x2);
            }
            else
            {
                colors[3] = m_BorderColor.argb;
            }
        }
        else
        {
            colors[1] = m_BorderColor.argb;
            colors[3] = m_BorderColor.argb;
        }

        if ((x2 >= 0) &&
            (x1 < (INT) m_nWidth) &&
            (y2 >= 0) &&
            (y1 < (INT) m_nHeight))
        {
            pargbDest->argb = ::getBilinearFilteredARGB(
                colors,
                xFrac,
                yFrac,
                one,
                shift,
                half2,
                shift2
            );
        }
        else
        {
            *pargbDest = m_BorderColor;
        }

        pargbDest++;

        x0 += dx;
        y0 += dy;
    }
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CBilinearSpan_MMX
//
//  Synopsis:
//      Resampling span using bilinear filtering. Code optimized using MMX
//      instruction set.
//
//------------------------------------------------------------------------------

CBilinearSpan_MMX::CBilinearSpan_MMX()
    : CUnoptimizedBilinearSpan()
{
}

HRESULT CBilinearSpan_MMX::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
    )
{
    HRESULT hr = S_OK;

    MIL_THR(CUnoptimizedBilinearSpan::Initialize(
        pIBitmapSource,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC
        ));

    if (SUCCEEDED(hr))
    {
        InitializeFixedPointState();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan_MMX::CanHandleInputRange
//
//  Synopsis:
//      Determines whether or not the inputs can be handled in the FIXED16
//      format used by this class
//
//------------------------------------------------------------------------------
BOOL CBilinearSpan_MMX::CanHandleInputRange(
    UINT uBitmapWidth,
        // Width of the source bitmap
    UINT uBitmapHeight,
        // Height of the source bitmap
    MilBitmapWrapMode::Enum wrapMode
        // Bitmap wrap mode
    )
{
    BOOL fInputValid = TRUE;

    if ((wrapMode == MilBitmapWrapMode::FlipX) ||
        (wrapMode == MilBitmapWrapMode::FlipXY))
    {
        // The width is multiplied by 2 for FlipX wrap modes
        // during InitializeFixedPointState
        fInputValid = uBitmapWidth <= FIXED16_INT_MAX/2;
    }
    else
    {
        fInputValid = uBitmapWidth <= FIXED16_INT_MAX;
    }

    if ((wrapMode == MilBitmapWrapMode::FlipY) ||
        (wrapMode == MilBitmapWrapMode::FlipXY))
    {
        // The height is multiplied by 2 for FlipY wrap modes
        // during InitializeFixedPointState
        fInputValid = fInputValid && (uBitmapHeight <= FIXED16_INT_MAX/2);
    }
    else
    {
        fInputValid = fInputValid && (uBitmapHeight <= FIXED16_INT_MAX);
    }

    return fInputValid;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the fixed point variables needed for texture mapping.
//

VOID CBilinearSpan_MMX::InitializeFixedPointState()
{
    M11 = CFloatFPU::Round(m_matDeviceToTexture.GetM11() * (1L << 16));
    M12 = CFloatFPU::Round(m_matDeviceToTexture.GetM12() * (1L << 16));
    M21 = CFloatFPU::Round(m_matDeviceToTexture.GetM21() * (1L << 16));
    M22 = CFloatFPU::Round(m_matDeviceToTexture.GetM22() * (1L << 16));
    Dx  = CFloatFPU::Round(m_matDeviceToTexture.GetDx() * (1L << 16));
    Dy  = CFloatFPU::Round(m_matDeviceToTexture.GetDy() * (1L << 16));

    SetDeviceOffset();

    UIncrement = M11;
    VIncrement = M12;

    // Guard that overflow doesn't happen when converting the modulus to FIXED16.
    //
    // This is important because we use this modulus to avoid reading outside of
    // the source bitmap, and is checked for during CanHandleInputRange.
    Assert (m_nWidth <= FIXED16_INT_MAX);
    Assert (m_nHeight <= FIXED16_INT_MAX);

    ModulusWidth = (m_nWidth << 16);
    ModulusHeight = (m_nHeight << 16);

    // When the u,v coordinates have the pixel in the last row or column
    // of the texture space, the offset of the pixel to the right and the
    // pixel below (for bilinear filtering) is the following (for tile modes)
    // because they wrap around the texture space.

    // The XEdgeIncrement is the byte increment of the pixel to the right of
    // the pixel on the far right hand column of the texture. In tile mode,
    // we want the pixel on the same scanline, but in the first column of the
    // texture hence 4bytes - stride

    XEdgeIncrement = 4 * (1 - m_nWidth);

    // The YEdgeIncrement is the byte increment of the pixel below the current
    // pixel when the current pixel is in the last scanline of the texture.
    // In tile mode the correct pixel is the one directly above this one in
    // the first scanline - hence the increment below:

    YEdgeIncrement = -(INT)(m_nHeight-1)*(INT)(m_cbStride);

    if ((m_WrapMode == MilBitmapWrapMode::FlipX) ||
        (m_WrapMode == MilBitmapWrapMode::FlipXY))
    {

        // Guard that overflow doesn't happen during this multiplication
        //
        // This is important because we use this modulus to avoid reading outside of
        // the source bitmap, and is checked for during CanHandleInputRange.
        Assert (ModulusWidth <= INT_MAX/2);

        ModulusWidth *= 2;

        // Wrap increment is zero for Flip mode

        XEdgeIncrement = 0;
    }
    if ((m_WrapMode == MilBitmapWrapMode::FlipY) ||
        (m_WrapMode == MilBitmapWrapMode::FlipXY))
    {
        // Guard that overflow doesn't happen during this multiplication
        //
        // This is important because we use this modulus to avoid reading outside of
        // the source bitmap, and is checked for during CanHandleInputRange.
        Assert (ModulusHeight <= INT_MAX/2);
        ModulusHeight *= 2;

        // Wrap increment is zero for Flip mode

        YEdgeIncrement = 0;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan_MMX::SetDeviceOffset
//
//  Synopsis:
//      The device-to-texture translations (Dx,Dy) can overflow the 16.16 field,
//      if the scaling and position are large enough.
//      To work around this, use a "position-independent" transform by using
//      device coordinates relative to an "origin" near to the destination.
//
//------------------------------------------------------------------------------
VOID CBilinearSpan_MMX::SetDeviceOffset()
{
    if (Dx != 0x80000000 && Dy != 0x80000000 &&
        Dx != 0x7fffffff && Dy != 0x7fffffff)
    {
        // if no overflow, use the surface origin.  No mapping adjustments needed.
        XDeviceOffset = 0;
        YDeviceOffset = 0;
        return;
    }

    // The position of the destination isn't available here (it's available
    // about 10 callers upstream).  As a heuristic, use the point in surface
    // space that maps to (0,0) in texture space.

    CMILMatrix matTextureToSurface;
    if (matTextureToSurface.Invert(m_matDeviceToTexture))
    {
        XDeviceOffset = CFloatFPU::Round(matTextureToSurface.GetDx());
        YDeviceOffset = CFloatFPU::Round(matTextureToSurface.GetDy());

        // GenerateColors subtracts the "origin" before applying the
        // mapping, so adjust the existing mapping by adding a translation.
        float fx = static_cast<float>(XDeviceOffset);
        float fy = static_cast<float>(YDeviceOffset);
        CMILMatrix adjustedDeviceToTexture( 1,  0,  0,  0,
                                            0,  1,  0,  0,
                                            0,  0,  1,  0,
                                            fx, fy, 0,  1 );
        adjustedDeviceToTexture.Multiply(m_matDeviceToTexture);

        // the adjusted mapping only differs from the existing one in the
        // translation components
        Dx  = CFloatFPU::Round(adjustedDeviceToTexture.GetDx() * (1L << 16));
        Dy  = CFloatFPU::Round(adjustedDeviceToTexture.GetDy() * (1L << 16));
    }

    // if the surface-to-texture mapping isn't invertible, or if the pre-image
    // of texture point (0,0) is still too far away from the destination,
    // (Dx,Dy) is still wrong due to overflow of the 16.16 field.  In practice,
    // neither of these conditions is likely to happen, so don't try to fix them.
    // Instead, just live with the "failure to render" bug.
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan_MMX::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_Bilinear_32bppPARGB_or_32bppRGB_MMX =
    ColorSource_Image_ScanOp<CBilinearSpan_MMX, GpCC>;

ScanOpFunc CBilinearSpan_MMX::GetScanOp() const
{
    return ColorSource_Image_Bilinear_32bppPARGB_or_32bppRGB_MMX;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan_MMX::GenerateColors
//
//  Synopsis:
//      Handles bilinear texture drawing with arbitrary rotation using MMX.
//

void CBilinearSpan_MMX::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) GpCC *pargbDest
    ) const
{
    // Be a little paranoid in checking some state.

    Assert((((ULONG_PTR) m_pvBits) & 3) == 0);
    Assert((m_cbStride & 3) == 0);

#if defined(_X86_)

    // Transform an array of points using the matrix v' = v M:
    //
    //                                  ( M11 M12 0 )
    //      (vx', vy', 1) = (vx, vy, 1) ( M21 M22 0 )
    //                                  ( dx  dy  1 )
    //
    // All (u, v) calculations are done in 16.16 fixed point.

    INT u;
    INT v;

    // Transform x & y into texture coordinates
    //
    // Note: If the result is out of the 16.16 range, we allow it to overflow.
    u = M11 * (x-XDeviceOffset) + M21 * (y-YDeviceOffset) + Dx;
    v = M12 * (x-XDeviceOffset) + M22 * (y-YDeviceOffset) + Dy;

    INT uIncrement = UIncrement;
    INT vIncrement = VIncrement;
    INT modulusWidth = ModulusWidth;
    INT modulusHeight = ModulusHeight;
    VOID *scan0 = m_pvBits;
    INT stride = m_cbStride;
    INT width = m_nWidth;
    INT height = m_nHeight;
    INT xEdgeIncrement = XEdgeIncrement;
    INT yEdgeIncrement = YEdgeIncrement;

    INT widthMinus1 = width - 1;
    INT heightMinus1 = height - 1;
    UINT uMax = widthMinus1 << 16;
    UINT vMax = heightMinus1 << 16;
    BOOL extendMode = (m_WrapMode == MilBitmapWrapMode::Extend);
    BOOL borderMode = (m_WrapMode == MilBitmapWrapMode::Border);
    ARGB borderColor = m_BorderColor.argb;
    static const ULONGLONG u64Half8dot8 = 0x0080008000800080;

    _asm
    {
        mov         eax, u
        mov         ebx, v
        mov         ecx, stride
        mov         edi, pargbDest
        pxor        mm0, mm0
        movq        mm3, u64Half8dot8

        ; edx = scratch
        ; esi = source pixel

    PixelLoop:

        ; Most of the time, our texture coordinate will be from the interior
        ; of the texture.  Things only really get tricky when we have to
        ; span the texture edges.
        ;
        ; Fortunately, the interior case will happen most of the time,
        ; so we make that as fast as possible.  We pop out-of-line to
        ; handle the tricky cases.

        cmp         eax, uMax
        jae         HandleTiling            ; Note unsigned compare

        cmp         ebx, vMax
        jae         HandleTiling            ; Note unsigned compare

        mov         edx, eax
        shr         edx, 14
        and         edx, 0xfffffffc

        mov         esi, ebx
        shr         esi, 16
        imul        esi, ecx

        add         esi, edx
        add         esi, scan0              ; esi = upper left pixel

        ; Stall city.  Write first, then reorder with VTune.

        movd        mm4, [esi]
        movd        mm5, [esi+4]
        movd        mm6, [esi+ecx]
        movd        mm7, [esi+ecx+4]

    ContinueLoop:
        movd        mm1, eax
        punpcklwd   mm1, mm1
        punpckldq   mm1, mm1
        psrlw       mm1, 8                  ; mm1 = x fraction in low bytes

        movd        mm2, ebx
        punpcklwd   mm2, mm2
        punpckldq   mm2, mm2
        psrlw       mm2, 8                  ; mm2 = y fraction in low bytes

        punpcklbw   mm4, mm0
        punpcklbw   mm5, mm0                ; unpack pixels A & B to low bytes

        psubw       mm5, mm4
        pmullw      mm5, mm1
        paddw       mm5, mm3
        psrlw       mm5, 8
        paddb       mm5, mm4                ; mm5 = A' = A + xFrac * (B - A)

        punpcklbw   mm6, mm0
        punpcklbw   mm7, mm0                ; unpack pixels C & D to low bytes

        psubw       mm7, mm6
        pmullw      mm7, mm1
        paddw       mm7, mm3
        psrlw       mm7, 8
        paddb       mm7, mm6                ; mm7 = B' = C + xFrac * (D - C)

        psubw       mm7, mm5
        pmullw      mm7, mm2
        paddw       mm7, mm3
        psrlw       mm7, 8
        paddb       mm7, mm5                ; mm7 = A' + yFrac * (B' - A')

        packuswb    mm7, mm7
        movd        [edi], mm7              ; write the final pixel

        add         eax, uIncrement
        add         ebx, vIncrement
        add         edi, 4

        dec         uiCount
        jnz         PixelLoop
        jmp         AllDone

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Handle tiling cases here
    ;
    ; All the tough edge cases are handled here, where we deal with
    ; texture coordinates that span the texture boundary.

    HandleTiling:
        cmp         borderMode, 0
        jnz         HandleBorder

        cmp         extendMode, 0
        jnz         HandleExtend

        ; Get 'u' in the range 0 <= u < modulusWidth:

        cmp         eax, modulusWidth
        jb          WidthInRange            ; note unsigned compare

        cdq
        idiv        modulusWidth
        mov         eax, edx                ; u %= modulusWidth

        cmp         eax, 0
        jge         WidthInRange
        add         eax, modulusWidth       ; 0 <= u < modulusWidth

    WidthInRange:

        ; Get 'v' in the range 0 <= v < modulusHeight:

        cmp         ebx, modulusHeight
        jb          HeightInRange

        push        eax
        mov         eax, ebx
        cdq
        idiv        modulusHeight
        mov         ebx, edx                ; v %= modulusHeight
        pop         eax

        cmp         ebx, 0
        jge         HeightInRange
        add         ebx, modulusHeight      ; 0 <= v < modulusHeight

    HeightInRange:

        ; Now we're going to need to convert our 'u' and 'v' values
        ; to integers, so save the 16.16 versions:

        push        eax
        push        ebx
        push        ecx
        push        edi

        sar         eax, 16
        sar         ebx, 16                 ; note arithmetic shift

        ; Handle 'flipping'.  Note that edi hold flipping flags, where
        ; the bits have the following meanings:
        ;   1 = X flip in progress
        ;   2 = Y flip in progress
        ;   4 = X flip end boundary not yet reached
        ;   8 = Y flip end boundary not yet reached.

        xor         edi, edi
        cmp         eax, width
        jb          XFlipHandled

        ; u is in the range (width <= u < 2*width).
        ;
        ; We want to flip it such that (0 <= u' < width), which we do by
        ; u' = 2*width - u - 1.  Don't forget ~u = -u - 1.

        or          edi, 1                  ; mark the flip
        not         eax
        add         eax, width
        add         eax, width
        jz          XFlipHandled
        sub         eax, 1
        or          edi, 4                  ; mark flip where adjacent pixels available


    XFlipHandled:
        cmp         ebx, height
        jb          YFlipHandled


        ; v is in the range (height <= v < 2*height).
        ;
        ; We want to flip it such that (0 <= v' < height), which we do by
        ; v' = 2*height - v - 1.  Don't forget ~v = -v - 1.

        or          edi, 2                  ; mark the flip
        not         ebx
        add         ebx, height
        add         ebx, height
        jz          YFlipHandled
        sub         ebx, 1
        or          edi, 8                  ; mark flip where adjacent pixels available

    YFlipHandled:
        mov         esi, ebx
        imul        esi, ecx                ; esi = y * stride

        ; Set 'edx' to the byte offset to the pixel one to the right, accounting
        ; for wrapping past the edge of the bitmap.  Only set the byte offset to
        ; point to right pixel for non edge cases.

        mov         edx, 4
        test        edi, 4
        jnz         RightIncrementCalculated
        test        edi, 1
        jnz         SetXEdgeInc
        cmp         eax, widthMinus1
        jb          RightIncrementCalculated
    SetXEdgeInc:
        mov         edx, xEdgeIncrement

        ; When we flipX and the current pixel is the last pixel in the texture
        ; line, wrapping past the end of the bitmap wraps back in the same side
        ; of the bitmap. I.e. for this one specific pixel we can set the pixel
        ; on-the-right to be the same as this pixel (increment of zero).
        ; Only valid because this is the edge condition.
        ; Note that this will occur for two successive pixels as the texture
        ; wrap occurs - first at width-1 and then at width-1 after wrapping.
        ;
        ; A | B
        ; --+--
        ; C | D
        ;
        ; At this point, pixel A has been computed correctly accounting for the
        ; flip/tile and wrapping beyond the edge of the texture. We work out
        ; the offset of B from A, but we again need to take into account the
        ; possible flipX mode if pixel A happens to be the last pixel in the
        ; texture scanline (the code immediately above takes into account
        ; tiling across the texture boundary, but not the flip)


    RightIncrementCalculated:

        ; Set 'ecx' to the byte offset to the pixel one down, accounting for
        ; wrapping past the edge of the bitmap.  Only set the byte offset to
        ; point to one pixel down for non edge cases.

        test        edi, 8
        jnz         DownIncrementCalculated
        test        edi, 2
        jnz         SetYEdgeInc
        cmp         ebx, heightMinus1
        jb          DownIncrementCalculated
    SetYEdgeInc:
        mov         ecx, yEdgeIncrement

        ; When we flipY and the current pixel is in the last scanline in the
        ; texture, wrapping past the end of the bitmap wraps back in the same
        ; side of the bitmap. I.e. for this one specific scanline we can set
        ; the pixel offset one down to be the same as this pixel
        ; (increment of zero).
        ; Only valid because this is the edge condition.
        ; (see comment above RightIncrementCalculated:)

    DownIncrementCalculated:

        ; Finish calculating the upper-left pixel address:

        add         esi, scan0
        shl         eax, 2
        add         esi, eax                ; esi = upper left pixel

        ; Load the 4 pixels:

        movd        mm4, [esi]
        movd        mm5, [esi+edx]
        add         esi, ecx
        movd        mm6, [esi]
        movd        mm7, [esi+edx]

        ; Finish handling the flip:

        test        edi, 1
        jz          XSwapDone

        movq        mm1, mm5
        movq        mm5, mm4
        movq        mm4, mm1                ; swap pixels A and B

        movq        mm1, mm6
        movq        mm6, mm7
        movq        mm7, mm1                ; swap pixels C and D

    XSwapDone:
        test        edi, 2
        jz          YSwapDone

        movq        mm1, mm4
        movq        mm4, mm6
        movq        mm6, mm1                ; swap pixels A and C

        movq        mm1, mm5
        movq        mm5, mm7
        movq        mm7, mm1                ; swap pixels B and D

    YSwapDone:

        ; Restore everything and get out:

        pop         edi
        pop         ecx
        pop         ebx
        pop         eax
        jmp         ContinueLoop

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Border mode.
    ;
    ; Set the pixel values to 0 for any not on the texture.

    HandleBorder:
        push        eax
        push        ebx

        movd        mm4, borderColor
        movq        mm5, mm4
        movq        mm6, mm4                ; initialize invisible pixels to
        movq        mm7, mm4                ; the borderColor.

        sar         eax, 16
        sar         ebx, 16                 ; note these are arithmetic shifts

        ; We need to look at a 2x2 square of pixels in the texture, where
        ; (eax, ebx) represents the (x, y) texture coordinates.  First we
        ; check for the case where none of the four pixel locations are
        ; actually anywhere on the texture.

        cmp         eax, -1
        jl          BorderFinish
        cmp         eax, width
        jge         BorderFinish            ; early out if (x < -1) or (x >= width)

        cmp         ebx, -1
        jl          BorderFinish
        cmp         ebx, height
        jge         BorderFinish            ; handle trivial rejection

        ; Okay, now we know that we have to pull at least one pixel from
        ; the texture.  Find the address of the upper-left pixel:

        mov         edx, eax
        shl         edx, 2
        mov         esi, ebx
        imul        esi, ecx
        add         esi, edx
        add         esi, scan0              ; esi = upper left pixel

        ; Our pixel nomenclature for the 2x2 square is as follows:
        ;
        ;   A | B
        ;  ---+---
        ;   C | D

        cmp         ebx, 0                  ; if (y < 0), we can't look at
        jl          BorderHandleCD          ;   row y
        cmp         eax, 0                  ; if (x < 0), we can't look at
        jl          BorderDoneA             ;   column x
        movd        mm4, [esi]              ; read pixel (x, y)

    BorderDoneA:
        cmp         eax, widthMinus1        ; if (x >= width - 1), we can't
        jge         BorderHandleCD          ;   look at column x
        movd        mm5, [esi+4]            ; read pixel (x+1, y)

    BorderHandleCD:
        cmp         ebx, heightMinus1       ; if (y >= height - 1), we can't
        jge         BorderFinish            ;   look at row y
        cmp         eax, 0                  ; if (x < 0), we can't look at
        jl          BorderDoneC             ;   column x
        movd        mm6, [esi+ecx]          ; read pixel (x, y+1)

    BorderDoneC:
        cmp         eax, widthMinus1        ; if (x >= width - 1), we can't
        jge         BorderFinish            ;   look at column x
        movd        mm7, [esi+ecx+4]        ; read pixel (x+1, y+1)

    BorderFinish:
        pop         ebx
        pop         eax
        jmp         ContinueLoop

    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ;
    ; Extend mode.
    ;

    HandleExtend:
        push        eax
        push        ebx

        sar         eax, 16
        sar         ebx, 16                 ; note these are arithmetic shifts

                                            ; edx keeps flags telling us how to find
                                            ; the next pixels in both directions
        mov         edx, 0                  ; 1 - add 4 in to move in horizonal dir
                                            ; 2 - add stride to move in vertical dir

        cmp         eax, 0
        jl          ExtendAdjustLeft
        cmp         eax, widthMinus1
        jge         ExtendAdjustLeft
        or          edx, 1

    ExtendAdjustLeft:                        ; clamp x
        cmp         eax, 0
        jge         ExtendAdjustRight
        mov         eax, 0
    ExtendAdjustRight:
        cmp         eax, widthMinus1
        jle         ExtendProcessY
        mov         eax, widthMinus1

    ExtendProcessY:
        cmp         ebx, 0
        jl          ExtendAdjustTop
        cmp         ebx, heightMinus1
        jge         ExtendAdjustTop
        or          edx, 2

    ExtendAdjustTop:                         ; clamp y
        cmp         ebx, 0
        jge         ExtendAdjustBottom
        mov         ebx, 0
    ExtendAdjustBottom:
        cmp         ebx, heightMinus1
        jle         ExtendLoadPixels
        mov         ebx, heightMinus1

    ExtendLoadPixels:
        shl         eax, 2
        mov         esi, ebx
        imul        esi, ecx
        add         esi, eax
        add         esi, scan0              ; esi = upper left pixel

        movd        mm4, [esi]              ; read pixel (x, y)
        test        edx, 1
        je          ExtendSameAB
        movd        mm5, [esi+4]            ; read pixel (x+1, y)
        jmp         ExtendLoadCD
    ExtendSameAB:
        movd        mm5, [esi]

    ExtendLoadCD:
        test        edx, 2
        je          ExtendSameCD
        add         esi, ecx

    ExtendSameCD:
        movd        mm6, [esi]              ; read pixel (x, y+1)

        test        edx, 1
        je          ExtendSameCDAB
        movd        mm7, [esi+4]            ; read pixel (x+1, y+1)
        jmp         FinishExtent
    ExtendSameCDAB:
        movd        mm7, [esi]

    FinishExtent:
        pop         ebx
        pop         eax
        jmp         ContinueLoop

    AllDone:
        emms

    }

#endif

}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CIdentitySpan
//
//  Synopsis:
//      Identity resampling span. Used when there is no complicated affine
//      operation on the input bitmap just integer translation from one location
//      to another.
//
//------------------------------------------------------------------------------

CIdentitySpan::CIdentitySpan()
    : CResampleSpan_sRGB()
{
}

HRESULT CIdentitySpan::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount_opt(1) const MilColorF *pBorderColor,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatTextureHPCToDeviceHPC
    )
{
    HRESULT hr = S_OK;

    MIL_THR(CResampleSpan_sRGB::Initialize(
        pIBitmapSource,
        wrapMode,
        pBorderColor,
        pmatTextureHPCToDeviceHPC
        ));

    if (SUCCEEDED(hr))
    {
        PowerOfTwo = !(m_nWidth & (m_nWidth - 1)) &&
                     !(m_nHeight & (m_nHeight - 1));

        // Compute the device-to-world transform (easy, eh?):

        Dx = -CFloatFPU::Round(pmatTextureHPCToDeviceHPC->GetDx());
        Dy = -CFloatFPU::Round(pmatTextureHPCToDeviceHPC->GetDy());
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CIdentitySpan::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_Identity_32bpp =
    ColorSource_Image_ScanOp<CIdentitySpan, GpCC>;

ScanOpFunc CIdentitySpan::GetScanOp() const
{
    return ColorSource_Image_Identity_32bpp;
}

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Output routine for handling texture brushes with indentity transforms
//      and either 'Tile' or 'Border' wrap modes.
//
//------------------------------------------------------------------------------
VOID CIdentitySpan::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) GpCC *pargbDest
    ) const
{
    Assert(m_nWidth > 0);
    Assert(m_nWidth <= INT_MAX);
    Assert(m_nHeight > 0);
    Assert(m_nHeight <= INT_MAX);

    INT u = x + Dx;
    INT v = y + Dy;
    const UINT uiHeight = m_nHeight;
    UINT i;

    if (m_WrapMode == MilBitmapWrapMode::Tile)
    {
        const UINT uiWidth = m_nWidth;

        if (PowerOfTwo)
        {
            u &= (uiWidth - 1);
            v &= (uiHeight - 1);
        }
        else
        {
            // Single unsigned compare handles (u < 0) and (u >= width)

            if (static_cast<unsigned>(u) >= uiWidth)
            {
                u = RemainderI(u, uiWidth);
            }

            // Single unsigned compare handles (v < 0) and (v >= width)

            if (static_cast<unsigned>(v) >= uiHeight)
            {
                v = RemainderI(v, uiHeight);
            }
        }

        ARGB *row = reinterpret_cast<ARGB*>
                    (static_cast<BYTE*>(m_pvBits) + (v * m_cbStride));
        const ARGB *src;

        Assert(u < static_cast<int>(uiWidth));

        src = row + u;
        i = min(static_cast<UINT>(uiWidth - u), uiCount);
        Assert(i > 0);
        uiCount -= i;

        // We don't call GpMemcpy here because by doing the copy explicitly,
        // the compiler still converts to a 'rep movsd', but it doesn't have
        // to add to pargbDest when done:

        do {
            pargbDest++->argb = *src++;

        } while (--i != 0);

        while (uiCount > 0)
        {
            src = row;
            i = min(uiWidth, uiCount);
            Assert(i > 0);
            uiCount -= i;

            do {
                pargbDest++->argb = *src++;

            } while (--i != 0);
        }
    }
    else
    {
        UINT uiWidth = m_nWidth;

        Assert(m_WrapMode == MilBitmapWrapMode::Border);

        GpCC borderColor = m_BorderColor;

        // Check for trivial rejection.  Unsigned compare handles
        // (v < 0) and (v >= uiHeight).

        if ((static_cast<unsigned>(v) >= uiHeight) ||
            (static_cast<int>(uiWidth) < u) ||
            (u < 0 && uiCount <= static_cast<unsigned>(-u)))
        {
            // The whole scan should be the border color:

            i = uiCount;
            do {
                *pargbDest++ = borderColor;

            } while (--i != 0);
        }
        else
        {
            const ARGB *src = reinterpret_cast<ARGB*>
                        (static_cast<BYTE*>(m_pvBits) + (v * m_cbStride));

            if (u < 0)
            {
                i = -u;
                uiCount -= i;
                do {
                    *pargbDest++ = borderColor;

                } while (--i != 0);
            }
            else
            {
                src += u;
                Assert(static_cast<int>(uiWidth) >= u);
                uiWidth -= u;
            }

            i = min(uiCount, uiWidth);
            Assert(i > 0);              // Trivial rejection ensures this
            uiCount -= i;


            /*
            The compiler was generating particularly ineffective code
            for this loop.

            do {
                *pargbDest++ = *src++;

            } while (--i != 0);
            */

            UINT uiBufferSize = sizeof(*pargbDest);
            if (SUCCEEDED(UIntMult(uiBufferSize, i, &uiBufferSize)))
            {
                GpMemcpy(pargbDest, src, uiBufferSize);
                pargbDest += i;
            }

            while (uiCount-- > 0)
            {
                *pargbDest++ = borderColor;
            }
        }
    }

    return;
}



//+-----------------------------------------------------------------------------
//
//  Class:
//      CNearestNeighborSpan_scRGB
//
//  Synopsis:
//      Resampling span using nearest pixel filtering in scRGB space.
//
//------------------------------------------------------------------------------

CNearestNeighborSpan_scRGB::CNearestNeighborSpan_scRGB()
    : CResampleSpan_scRGB()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CNearestNeighborSpan_scRGB::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_NearestNeighbor_128bppPABGR =
    ColorSource_Image_ScanOp<CNearestNeighborSpan_scRGB, MilColorF>;

ScanOpFunc CNearestNeighborSpan_scRGB::GetScanOp() const
{
    return ColorSource_Image_NearestNeighbor_128bppPABGR;
}

void CNearestNeighborSpan_scRGB::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) MilColorF *pcolDest
    ) const
{
    MilPoint2F pt1, pt2;
    pt1.X = (REAL) x;
    pt1.Y = (REAL) y;
    pt2.X = (REAL) x + uiCount;
    pt2.Y = (REAL) y;

    m_matDeviceToTexture.Transform(&pt1, &pt1);
    m_matDeviceToTexture.Transform(&pt2, &pt2);

    FLOAT dx = (pt2.X - pt1.X) / (FLOAT)uiCount;
    FLOAT dy = (pt2.Y - pt1.Y) / (FLOAT)uiCount;

    const MilColorF *srcBuffer = (MilColorF*) m_pvBits;
    INT stride = (m_cbStride / sizeof(MilColorF));

    INT ix;
    INT iy;

    // For all pixels in the destination span...
    for(UINT i=0; i<uiCount; i++)
    {
        // .. compute the position in source space.

        // round to the nearest neighbor
        ix = CFloatFPU::Round(pt1.X);
        iy = CFloatFPU::Round(pt1.Y);

        // Make sure the pixel is within the bounds of the source before
        // accessing it.

        if( (ix >= 0) &&
            (iy >= 0) &&
            (ix < (INT)m_nWidth) &&
            (iy < (INT)m_nHeight) )
        {
            *pcolDest++ = *(srcBuffer + stride*iy + ix);
        }
        else
        {
            if (m_WrapMode != MilBitmapWrapMode::Border)
            {
                ApplyWrapMode(m_WrapMode, ix, iy, m_nWidth, m_nHeight);

                Assert (ix >= 0);
                Assert (iy >= 0);
                Assert (ix < (INT)m_nWidth);
                Assert (iy < (INT)m_nHeight);

                *pcolDest++ = *(srcBuffer + stride*iy + ix);
            }
            else
            {
                // This means that this source pixel is outside of the valid
                // bits in the source. (edge condition)
                *pcolDest++ = m_BorderColor;
            }
        }

        // Update source position
        pt1.X += dx;
        pt1.Y += dy;
    }
}

//+-----------------------------------------------------------------------------
//
//  Class:
//      CBilinearSpan_scRGB
//
//  Synopsis:
//      Resampling span using bilinear filtering in scRGB space.
//
//------------------------------------------------------------------------------

CBilinearSpan_scRGB::CBilinearSpan_scRGB()
    : CResampleSpan_scRGB()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBilinearSpan_scRGB::GetScanOp
//
//  Synopsis:
//      Return ScanOp function that will generate colors
//
//------------------------------------------------------------------------------

const ScanOpFunc ColorSource_Image_Bilinear_128bppPABGR =
    ColorSource_Image_ScanOp<CBilinearSpan_scRGB, MilColorF>;

ScanOpFunc CBilinearSpan_scRGB::GetScanOp() const
{
    return ColorSource_Image_Bilinear_128bppPABGR;
}

void CBilinearSpan_scRGB::GenerateColors(
    INT x,
    INT y,
    __range(1,INT_MAX) UINT uiCount,
    __out_ecount_full(uiCount) MilColorF *pcolDest
    ) const
{
    Assert(uiCount > 0);

    MilPoint2F pt1, pt2;
    pt1.X = (REAL) x;
    pt1.Y = (REAL) y;
    pt2.X = (REAL) x + uiCount;
    pt2.Y = (REAL) y;

    m_matDeviceToTexture.Transform(&pt1, &pt1);
    m_matDeviceToTexture.Transform(&pt2, &pt2);

    const MilColorF *srcBuffer = (MilColorF*) m_pvBits;
    INT stride = (m_cbStride / sizeof(MilColorF));

    FLOAT dx, dy, x0, y0;
    FLOAT xFrac, yFrac;
    INT ix, iy;

    x0 = pt1.X;
    y0 = pt1.Y;

    dx = (pt2.X - pt1.X) / (FLOAT)uiCount;
    dy = (pt2.Y - pt1.Y) / (FLOAT)uiCount;

    // Filtered image stretch.

    MilColorF colors[4];

    INT x1, y1, x2, y2;
    for (UINT i=0; i < uiCount; i++)
    {
        iy = GpFloor(y0);
        ix = GpFloor(x0);
        xFrac = x0 - (FLOAT)ix;
        yFrac = y0 - (FLOAT)iy;

        x1=ix;
        x2=ix+1;
        y1=iy;
        y2=iy+1;

        if (m_WrapMode != MilBitmapWrapMode::Border)
        {
            ApplyWrapMode(m_WrapMode, x1, y1, m_nWidth, m_nHeight);
            ApplyWrapMode(m_WrapMode, x2, y2, m_nWidth, m_nHeight);

            colors[0] = *(srcBuffer + stride*y1 + x1);
            colors[1] = *(srcBuffer + stride*y1 + x2);
            colors[2] = *(srcBuffer + stride*y2 + x1);
            colors[3] = *(srcBuffer + stride*y2 + x2);
        }
        else
        {
            if (y1 >= 0 && y1 < (INT)m_nHeight)
            {
                if (x1 >= 0 && x1 < (INT)m_nWidth)
                {
                    colors[0] = *(srcBuffer + stride*y1 + x1);
                }
                else
                {
                    colors[0] = m_BorderColor;
                }

                if (x2 >= 0 && x2 < (INT)m_nWidth)
                {
                    colors[1] = *(srcBuffer + stride*y1 + x2);
                }
                else
                {
                    colors[1] = m_BorderColor;
                }
            }
            else
            {
                colors[0] = m_BorderColor;
                colors[1] = m_BorderColor;
            }

            if (y2 >= 0 && y2 < (INT)m_nHeight)
            {
                if (x1 >= 0 && x1 < (INT)m_nWidth)
                {
                    colors[2] = *(srcBuffer + stride*y2 + x1);
                }
                else
                {
                    colors[2] = m_BorderColor;
                }

                if (x2 >= 0 && x2 < (INT)m_nWidth)
                {
                    colors[3] = *(srcBuffer + stride*y2 + x2);
                }
                else
                {
                    colors[3] = m_BorderColor;
                }
            }
            else
            {
                colors[2] = m_BorderColor;
                colors[3] = m_BorderColor;
            }
        }

        pcolDest->a =
            (1.0f - yFrac) * (colors[0].a + xFrac * (colors[1].a - colors[0].a)) +
            yFrac * (colors[2].a + xFrac * (colors[3].a - colors[2].a));

        pcolDest->r =
            (1.0f - yFrac) * (colors[0].r + xFrac * (colors[1].r - colors[0].r)) +
            yFrac * (colors[2].r + xFrac * (colors[3].r - colors[2].r));

        pcolDest->g =
            (1.0f - yFrac) * (colors[0].g + xFrac * (colors[1].g - colors[0].g)) +
            yFrac * (colors[2].g + xFrac * (colors[3].g - colors[2].g));

        pcolDest->b =
            (1.0f - yFrac) * (colors[0].b + xFrac * (colors[1].b - colors[0].b)) +
            yFrac * (colors[2].b + xFrac * (colors[3].b - colors[2].b));

        pcolDest++;

        x0 += dx;
        y0 += dy;
    }
}



//+-----------------------------------------------------------------------------
//
//  Class:
//      CConstantAlphaSpan
//
//  Synopsis:
//      Span class applying constant alpha on its input.
//
//------------------------------------------------------------------------------

CConstantAlphaSpan::CConstantAlphaSpan()
{
    m_nAlpha = 0;
}

HRESULT CConstantAlphaSpan::Initialize(FLOAT flAlpha)
{
    if (flAlpha < 0.0f)
    {
        flAlpha = 0.0f;
    }
    else if (flAlpha > 1.0f)
    {
        flAlpha = 1.0f;
    }

    m_nAlpha = CFloatFPU::Round(flAlpha * 65536.0f);

    return S_OK;
}

static VOID MIL_FORCEINLINE ConstantAlpha_32bppPARGB_or_32bppRGB_Slow(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP,
    bool fHasAlpha
    )
{
    INT nAlpha = DYNCAST(CConstantAlphaSpan, pSOP->m_posd)->m_nAlpha;

    BYTE *pOutput = static_cast<BYTE*>(pSOP->m_pvDest);
    Assert( pOutput != NULL );

    //
    // Apply the constant alpha to every pixel.
    //

    UINT nCount = pPP->m_uiCount;

    const UINT uiRound = (1 << 15);
    BYTE byteConstantAlpha = static_cast<BYTE>((255 * nAlpha + uiRound) >> 16);

    while (nCount-- > 0)
    {
        pOutput[0] = static_cast<BYTE>((pOutput[0] * nAlpha + uiRound) >> 16);
        pOutput[1] = static_cast<BYTE>((pOutput[1] * nAlpha + uiRound) >> 16);
        pOutput[2] = static_cast<BYTE>((pOutput[2] * nAlpha + uiRound) >> 16);
        if (fHasAlpha)
        {
            pOutput[3] = static_cast<BYTE>((pOutput[3] * nAlpha + uiRound) >> 16);
        }
        else
        {
            pOutput[3] = byteConstantAlpha;
        }

        pOutput += 4;
    }
}

VOID FASTCALL ConstantAlpha_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ConstantAlpha_32bppPARGB_or_32bppRGB_Slow(
        pPP,
        pSOP,
        true // Has alpha
        );
}

VOID FASTCALL ConstantAlpha_32bppRGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    ConstantAlpha_32bppPARGB_or_32bppRGB_Slow(
        pPP,
        pSOP,
        false // Has no alpha
        );
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CMaskAlphaSpan
//
//  Synopsis:
//      Span class applying alpha mask on its input.
//
//------------------------------------------------------------------------------

CMaskAlphaSpan::CMaskAlphaSpan()
{
    m_pBuffer = NULL;
    m_nBufferLen = 0;
}

CMaskAlphaSpan::~CMaskAlphaSpan()
{
    GpFree(m_pBuffer);
}

HRESULT CMaskAlphaSpan::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIMask,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
    MilBitmapInterpolationMode::Enum interpolationMode,
    bool prefilterEnable,
    FLOAT prefilterThreshold,
    INT spanWidth
    )
{
    HRESULT hr = S_OK;

    UINT cbSpan;

    Assert(spanWidth >= 0);

    IFC(MultiplyUINT(static_cast<UINT>(spanWidth), sizeof(MilColorB), OUT cbSpan));

    IFC(EnsureBufferSize(
        Mt(CMaskAlphaSpan),
        cbSpan,
        &m_nBufferLen,
        reinterpret_cast<void**>(&m_pBuffer)
        ));

    IFC( m_Creator_sRGB.GetCS_PrefilterAndResample(
        pIMask,
        MilBitmapWrapMode::Extend,
        NULL,
        pmatMaskToDevice,
        interpolationMode,
        prefilterEnable,
        prefilterThreshold,
        NULL,
        &m_pMaskResampleCS) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//      MaskAlpha: Unary operation; multiplies color channels by the alpha
//                  values of a (resampled) ARGB bitmap. (The RGB values are
//                  ignored).
//
//  Notes:
//      MaskAlpha is used to implement alpha-mask effects.
//
//      This operation is "unary" in that the mask bitmap is provided as
//      op-specific data - not as a pipeline buffer pointer.
//
//  Inputs:
//      pSOP->m_pvDest:   The destination scan.
//      pPP->m_uiCount:   Scan length, in pixels.
//
//  Return Value:
//      None
//
//------------------------------------------------------------------------------

// MaskAlpha a 32bppPARGB mask bitmap over 32bppPARGB or 32bppRGB color data

static VOID MIL_FORCEINLINE MaskAlpha_32bpp_Slow_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP,
    bool fHasAlpha
    )
{
    BYTE *pOutput = static_cast<BYTE*>(pSOP->m_pvDest);
    UINT nCount = pPP->m_uiCount;
    Assert( pOutput != NULL );

    const CMaskAlphaSpan *pMAS = DYNCAST(CMaskAlphaSpan, pSOP->m_posd);
    Assert(pMAS);

    //
    // Produce the pixels from the mask
    //

    ScanOpParams sopMask;

    sopMask.m_pvDest = pMAS->m_pBuffer;
    sopMask.m_posd = const_cast<CColorSource *>(pMAS->m_pMaskResampleCS);

    pMAS->m_pMaskResampleCS->GetScanOp()(pPP, &sopMask);

    //
    // Now scale each pixel by the alpha channel of the mask pixel
    //

    const BYTE *pMask = pMAS->m_pBuffer + 3;

    Assert ((static_cast<UINT>(nCount)*4) <= pMAS->m_nBufferLen);

    INT nMask;
    while (nCount-- > 0)
    {
        // The exact calculation (ignoring rounding) would be:
        //
        //   channel' = (channel * mask) / 255
        //
        // We approximate this using (257 / 65536) instead of (1 / 255). We add
        // a rounding step to minimize error. (Otherwise, in particular, we'd
        // output "channel' = 254", for inputs of "channel = 255, mask = 255".)

        nMask = (*pMask) * 257;

        const UINT uiRound = (1 << 15);

        pOutput[0] = static_cast<BYTE>((pOutput[0] * nMask + uiRound) >> 16);
        pOutput[1] = static_cast<BYTE>((pOutput[1] * nMask + uiRound) >> 16);
        pOutput[2] = static_cast<BYTE>((pOutput[2] * nMask + uiRound) >> 16);
        if (fHasAlpha)
        {
            pOutput[3] = static_cast<BYTE>((pOutput[3] * nMask + uiRound) >> 16);
        }
        else
        {
            pOutput[3] = static_cast<BYTE>((255 * nMask + uiRound) >> 16);
        }

        pOutput += 4;
        pMask += 4;
    }
}

VOID FASTCALL MaskAlpha_32bppPARGB_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    MaskAlpha_32bpp_Slow_32bppPARGB(
        pPP,
        pSOP,
        true // The destination scan has alpha
        );
}

VOID FASTCALL MaskAlpha_32bppRGB_32bppPARGB(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    MaskAlpha_32bpp_Slow_32bppPARGB(
        pPP,
        pSOP,
        false // The destination scan has no alpha
        );
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CConstantAlphaSpan_scRGB
//
//  Synopsis:
//      Span class applying constant alpha on its input.
//
//------------------------------------------------------------------------------

CConstantAlphaSpan_scRGB::CConstantAlphaSpan_scRGB()
{
    m_flAlpha = 0.0f;
}

HRESULT CConstantAlphaSpan_scRGB::Initialize(FLOAT flAlpha)
{
    if (flAlpha < 0.0f)
    {
        flAlpha = 0.0f;
    }
    else if (flAlpha > 1.0f)
    {
        flAlpha = 1.0f;
    }

    m_flAlpha = flAlpha;

    return S_OK;
}

VOID FASTCALL ConstantAlpha_128bppPABGR(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    FLOAT fAlpha = DYNCAST(CConstantAlphaSpan_scRGB, pSOP->m_posd)->m_flAlpha;

    FLOAT *pOutput = static_cast<FLOAT*>(pSOP->m_pvDest);

    //
    // Apply the constant alpha to every pixel.
    //

    UINT nCount = pPP->m_uiCount;

    while (nCount-- > 0)
    {
        pOutput[0] *= fAlpha;
        pOutput[1] *= fAlpha;
        pOutput[2] *= fAlpha;
        pOutput[3] *= fAlpha;

        pOutput += 4;
    }
}


//+-----------------------------------------------------------------------------
//
//  Class:
//      CMaskAlphaSpan_scRGB
//
//  Synopsis:
//      Span class applying alpha mask on its input.
//
//------------------------------------------------------------------------------

CMaskAlphaSpan_scRGB::CMaskAlphaSpan_scRGB()
{
    m_pBuffer = NULL;
    m_nBufferLen = 0;
}

CMaskAlphaSpan_scRGB::~CMaskAlphaSpan_scRGB()
{
    GpFree(m_pBuffer);
}

HRESULT CMaskAlphaSpan_scRGB::Initialize(
    __in_ecount(1) IWGXBitmapSource *pIMask,
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatMaskToDevice,
    MilBitmapInterpolationMode::Enum interpolationMode,
    bool prefilterEnable,
    FLOAT prefilterThreshold,
    INT spanWidth
    )
{
    HRESULT hr = S_OK;

    UINT cbSpan;

    Assert(spanWidth >= 0);

    IFC(MultiplyUINT(static_cast<UINT>(spanWidth), sizeof(MilColorF), OUT cbSpan));

    IFC(EnsureBufferSize(
        Mt(CMaskAlphaSpan),
        cbSpan,
        &m_nBufferLen,
        reinterpret_cast<void**>(&m_pBuffer)
        ));

    IFC( m_Creator_scRGB.GetCS_PrefilterAndResample(
        pIMask,
        MilBitmapWrapMode::Extend,
        NULL,
        pmatMaskToDevice,
        interpolationMode,
        prefilterEnable,
        prefilterThreshold,
        NULL,
        &m_pMaskResampleCS) );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      MaskAlpha_128bppPABGR_128bppPABGR
//
//  Synopsis:
//      See MaskAlpha_32bppPARGB_32bppPARGB
//
//------------------------------------------------------------------------------

// MaskAlpha a 128bppPABGR mask bitmap over 128bppPABGR color data

VOID FASTCALL MaskAlpha_128bppPABGR_128bppPABGR(
    __in_ecount(1) const PipelineParams *pPP,
    __in_ecount(1) const ScanOpParams *pSOP
    )
{
    FLOAT *pOutput = static_cast<FLOAT*>(pSOP->m_pvDest);
    UINT nCount = pPP->m_uiCount;
    Assert( pOutput != NULL );

    const CMaskAlphaSpan_scRGB *pMAS = DYNCAST(CMaskAlphaSpan_scRGB, pSOP->m_posd);
    Assert(pMAS);

    //
    // Produce the pixels from the mask
    //

    ScanOpParams sopMask;

    sopMask.m_pvDest = pMAS->m_pBuffer;
    sopMask.m_posd = const_cast<CColorSource *>(pMAS->m_pMaskResampleCS);

    pMAS->m_pMaskResampleCS->GetScanOp()(pPP, &sopMask);

    //
    // Now scale each pixel by the alpha channel of the mask pixel
    //

    FLOAT *pMask = pMAS->m_pBuffer + 3;

    Assert ((static_cast<UINT>(nCount)*4) <= pMAS->m_nBufferLen);

    FLOAT flMask;
    while (nCount-- > 0)
    {
        flMask = *pMask;

        pOutput[0] *= flMask;
        pOutput[1] *= flMask;
        pOutput[2] *= flMask;
        pOutput[3] *= flMask;

        pOutput += 4;
        pMask += 4;
    }
}






