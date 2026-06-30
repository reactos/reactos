// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Contents:   Implementation of MIL API brush objects
//
//  Classes:
//              CMILBrushSolid
//              CMILBrushLinearGradient
//              CMILBrushRadialGradient
//              CMILBrushBitmap
//
//------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMILBrushSolid, MILApi, "CMILBrushSolid");
MtDefine(CMILBrushLinearGradient, MILApi, "CMILBrushLinearGradient");
MtDefine(CMILBrushRadialGradient, MILApi, "CMILBrushRadialGradient");
MtDefine(CMILBrushFocalGradient, MILApi, "CMILBrushFocalGradient");
MtDefine(CMILBrushBitmap, MILApi, "CMILBrushBitmap");

MtDefine(MBitmapBrushData, MILRawMemory, "MBitmapBrushData");
MtDefine(CMILBrushShaderEffect, MILApi, "CMILBrushShaderEffect");

/*=========================================================================*\
    CMILBrushSolid - MIL Solid Brush Object
\*=========================================================================*/

CMILBrushSolid::CMILBrushSolid(__in_ecount_opt(1) CMILFactory *pFactory)
    : CMILObject(pFactory)
{
}

CMILBrushSolid::~CMILBrushSolid()
{
}

HRESULT CMILBrushSolid::Create(
    __in_ecount_opt(1) CMILFactory *pFactory,
    __in_ecount_opt(1) const MilColorF *pColor,
    __deref_out_ecount(1) CMILBrushSolid **ppSolidBrush
    )
{
    HRESULT hr = S_OK;

    // Create Brush
    CMILBrushSolid *pTempBrush = new CMILBrushSolid(pFactory);
    if (pTempBrush == NULL)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        // Increment ref-count to 1
        pTempBrush->AddRef();
    }

    // Set color
    if (SUCCEEDED(hr))
    {
        if (NULL != pColor)
        {
            pTempBrush->m_SolidColor = *(const MilColorF UNALIGNED*)pColor;
        }
        else
        {
            pTempBrush->m_SolidColor = CMilColorF();
        }
    }

    // Return brush via out-param
    if (SUCCEEDED(hr))
    {
        *ppSolidBrush = pTempBrush;
        pTempBrush = NULL;
    }

    // Release brush if it's non-NULL
    if (pTempBrush != NULL)
    {
        pTempBrush->Release();
    }

    RRETURN(hr);
}


void 
CMILBrushSolid::GetColor(
    __out_ecount(1) MilColorF *pColor
    ) const
{
    *pColor = m_SolidColor;
}

void
CMILBrushSolid::SetColor(
    __in_ecount(1) const MilColorF *pColor
    ) 
{
    m_SolidColor = *pColor;
}


BOOL CMILBrushSolid::HasZeroAlpha() const
{
    BOOL fZeroAlpha;

    //
    // This check is somewhat conservative so that we can avoid determining
    // the final target (sRGB versus scRGB) and doing all the color conversions
    // required to figure out if any pixels will be touched.  Note that blend
    // mode is also a factor in getting a correct computation.
    //

    fZeroAlpha = (m_SolidColor.a < FLT_EPSILON && m_SolidColor.a > -FLT_EPSILON);

    return fZeroAlpha;
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT CMILBrushSolid::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        // Call our base object's HrFindInterface
        hr = CMILObject::HrFindInterface(riid, ppvObject);
    }

    return hr;
}

/*=========================================================================*\
    CMILBrushBitmap - MIL Bitmap Brush Object
\*=========================================================================*/

CMILBrushBitmap::CMILBrushBitmap(CMILFactory *pFactory)
    : CMILObject(pFactory), CObjectUniqueness(),
      m_xSamplingSpaceDefinition(XSpaceIsWorldSpace),
      m_pTexture(NULL),
      m_matBitmapToSamplingSpace(true),
      m_WrapMode(MilBitmapWrapMode::Tile),
      m_fUseSourceClip(FALSE),
      m_opacity(1.0f)
{
    // Allow identity as a valid starting transform
    m_matBitmapToSamplingSpace.DbgChangeToSpace
        <CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling>();

    ZeroMemory(&m_BorderColor, sizeof(m_BorderColor));
}

CMILBrushBitmap::~CMILBrushBitmap()
{
    ReleaseInterfaceNoNULL(m_pTexture);
}

HRESULT CMILBrushBitmap::Create(
    CMILFactory *pFactory,
    IWGXBitmapSource *piBitmap,
    CMILBrushBitmap **ppBrushBitmap
    )
{
    HRESULT hr = S_OK;

    // Create brush
    CMILBrushBitmap *pTempBrush = new CMILBrushBitmap(pFactory);
    if (pTempBrush == NULL)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        pTempBrush->AddRef();
    }

    // Set bitmap
    if (SUCCEEDED(hr))
    {
        pTempBrush->m_pTexture = piBitmap;

        if (pTempBrush->m_pTexture != NULL)
        {
            pTempBrush->m_pTexture->AddRef();
        }
    }

    // Return brush via out-param
    if (SUCCEEDED(hr))
    {
        *ppBrushBitmap = pTempBrush;
        pTempBrush = NULL;
    }

    // Release brush if it's non-NULL
    if (pTempBrush != NULL)
    {
        pTempBrush->Release();
    }

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CMILBrushBitmap::MayNeedNonPow2Tiling
//
//  Synopsis:  
//      Returns whether the brush may needs non-pow2 tiling. Now-pow2 tiling is
//      not implemented in hardware text rendering, so text uses this query to
//      determine if software should be used instead.
//

bool
CMILBrushBitmap::MayNeedNonPow2Tiling() const
{
    //
    // There are some cases where we are tiling a bitmap when the realized
    // texture that we are tiling is actually a power of 2. We don't try to
    // catch this here. Such logic would need to know what size the
    // hwbitmapcolorsource would rescale the image.
    //

    switch (m_WrapMode)
    {
    case MilBitmapWrapMode::FlipX:
    case MilBitmapWrapMode::FlipY:
    case MilBitmapWrapMode::FlipXY:
    case MilBitmapWrapMode::Tile:
        return true;
    default:
        return false;
    }
}

/*=========================================================================*\
    IMILBrushBitmap methods
\*=========================================================================*/

HRESULT
CMILBrushBitmap::SetBitmap(
    IN IWGXBitmapSource *pBitmapSource
    )
{
    HRESULT hr = S_OK;

    ReplaceInterface(m_pTexture, pBitmapSource);

    UpdateUniqueCount();

    return hr;
}

HRESULT
CMILBrushBitmap::GetBitmap(
    IN IWGXBitmapSource **ppBitmapSource
    ) const
{
    HRESULT hr = S_OK;

    if (ppBitmapSource == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        *ppBitmapSource = NULL;

        if (m_pTexture == NULL)
        {
            MIL_THR(E_FAIL);
        }
        else
        {
            *ppBitmapSource = m_pTexture;
            m_pTexture->AddRef();
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CMILBrushBitmap::SetBitmapToXSpaceTransform
//
//  Synopsis:  
//      Sets the matrix which transforms from bitmap space to rendering space
//

void
CMILBrushBitmap::SetBitmapToXSpaceTransform(
    __in_ecount(1) const CBaseMatrix *pmatBitmapToXSpace,
    XSpaceDefinition xSpaceDefinition
    DBG_COMMA_PARAM(__in_ecount_opt(1) const CMILMatrix *pmatDBGWorldToSampleSpace)
    )
{
    m_xSamplingSpaceDefinition = xSpaceDefinition;

    if (xSpaceDefinition == XSpaceIsSampleSpace)
    {
        m_matBitmapToSamplingSpace = *
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device>::ReinterpretBase
            (pmatBitmapToXSpace);
    }
    else
    {
        Assert(xSpaceDefinition == XSpaceIsWorldSpace);
        m_matBitmapToSamplingSpace = *
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling>::ReinterpretBase
            (pmatBitmapToXSpace);
    }

#if DBG
    if (   (xSpaceDefinition == XSpaceIsSampleSpace)
        && pmatDBGWorldToSampleSpace
       )
    {
        m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace = *pmatDBGWorldToSampleSpace;
    }
    else
    {
        m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace.SetToIdentity();
    }
#endif

    UpdateUniqueCount();
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CMILBrushBitmap::GetBitmapToSampleSpaceTransform
//
//  Synopsis:  
//      Gets the matrix which transforms from bitmap space to sample space
//

void
CMILBrushBitmap::GetBitmapToSampleSpaceTransform(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToSampleSpace,
    __out_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &matBitmapToSampleSpace
    )
{
#if DBG
    DBGAssertWorldToSampleSpaceHasNotChanged(&matWorldToSampleSpace);
#endif

    if (m_xSamplingSpaceDefinition == XSpaceIsSampleSpace)
    {
        matBitmapToSampleSpace = m_matBitmapToSamplingSpace;
    }
    else
    {
        Assert(m_xSamplingSpaceDefinition == XSpaceIsWorldSpace);

        matBitmapToSampleSpace.SetToMultiplyResult<CoordinateSpace::BaseSampling>(
            m_matBitmapToSamplingSpace,
            matWorldToSampleSpace
            );
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CMILBrushBitmap::GetBitmapToWorldSpaceTransform
//
//  Synopsis:  
//      Gets the matrix which transforms from bitmap space to world space.
//
//  WARNING:
//      Callers of this method must know that the the brush does not store it's
//      transform as transforming into SampleSpace.
//

void
CMILBrushBitmap::GetBitmapToWorldSpaceTransform(
    __out_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matBitmapToWorldSpace
    ) const
{
#if DBG
    Assert(m_xSamplingSpaceDefinition == XSpaceIsWorldSpace);
#endif
    matBitmapToWorldSpace = m_matBitmapToSamplingSpace;
}


//+----------------------------------------------------------------------------
//
//  Member:   
//      CMILBrushBitmap::GetSourceClipSampleSpace
//
//  Synopsis:  
//      Gets the source clip in sample space.
//

void
CMILBrushBitmap::GetSourceClipSampleSpace(
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace,
    __out_ecount(1) CParallelogram *pSourceClipSampleSpace
    ) const
{
#if DBG
    DBGAssertWorldToSampleSpaceHasNotChanged(pmatWorldToSampleSpace);
#endif

    const CBaseMatrix *pSourceClipToSampleSpace;
    if (m_xSamplingSpaceDefinition == XSpaceIsSampleSpace)
    {
        pSourceClipToSampleSpace = NULL;
    }
    else
    {
        pSourceClipToSampleSpace = pmatWorldToSampleSpace; // NULL is okay
    }
          
    pSourceClipSampleSpace->Set(
        m_sourceClipInSamplingSpace,
        pSourceClipToSampleSpace
        );
}

#if DBG
//+----------------------------------------------------------------------------
//
//  Member:    CMILBrushBitmap::DBGAssertWorldToSampleSpaceHasNotChanged
//
//  Synopsis:  
//      DBG method to make sure that the world to sample space matrix has not
//      changed... if the sample space changed then some of the member
//      variables would be in the wrong space.
//

void
CMILBrushBitmap::DBGAssertWorldToSampleSpaceHasNotChanged(
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> *pmatWorldToSampleSpace
    ) const
{
    if (m_xSamplingSpaceDefinition == XSpaceIsSampleSpace)
    {
        if (pmatWorldToSampleSpace)
        {
            Assert(memcmp(
                pmatWorldToSampleSpace, 
                &m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace,
                sizeof(m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace)
                ) == 0
                );
        }
        else
        {
            Assert(m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace.IsIdentity());
        }
    }
    else
    {
        Assert(m_xSamplingSpaceDefinition == XSpaceIsWorldSpace);

        // m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace would not
        // initialized so we should not check it.
    }
}
#endif

HRESULT
CMILBrushBitmap::SetWrapMode(
    MilBitmapWrapMode::Enum wrapMode,
    const MilColorF *pBorderColor
    )
{
    HRESULT hr = S_OK;

    switch (wrapMode)
    {
    case MilBitmapWrapMode::Extend:
    case MilBitmapWrapMode::FlipX:
    case MilBitmapWrapMode::FlipY:
    case MilBitmapWrapMode::FlipXY:
    case MilBitmapWrapMode::Tile:
    case MilBitmapWrapMode::Border:
        m_WrapMode = wrapMode;
        break;
    default:
        MIL_THR(E_INVALIDARG);
        break;
    }

    if (pBorderColor != NULL)
    {
        m_BorderColor = *pBorderColor;
    }
    else
    {
        m_BorderColor = CMilColorF();
    }

    UpdateUniqueCount();

    API_CHECK(hr);
    return hr;
}

HRESULT
CMILBrushBitmap::GetWrapMode(
    MilBitmapWrapMode::Enum *pWrapMode,
    MilColorF *pBorderColor
    ) const
{
    HRESULT hr = S_OK;

    if (pWrapMode != NULL)
    {
        *pWrapMode = m_WrapMode;
    }

    if (pBorderColor != NULL)
    {
        *pBorderColor = m_BorderColor;
    }

    API_CHECK(hr);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:  CMILBrushBitmap::SetSourceClipXSpace
//
//  Synopsis:  Set's an optional parallelogram that the fill object is to be 
//             clipped to.
//
//-----------------------------------------------------------------------------
HRESULT
CMILBrushBitmap::SetSourceClipXSpace(
    BOOL fUseSourceClip,
    BOOL fSourceClipIsEntireSource,
    __in_ecount(1) const CParallelogram *pSourceClipXSpace
    DBG_COMMA_PARAM(XSpaceDefinition xSpaceDefinition)
    DBG_COMMA_PARAM(__in_ecount_opt(1) const CMILMatrix *pmatDBGWorldToSampleSpace)
    )
{
    HRESULT hr = S_OK;

#if DBG
    if (xSpaceDefinition == XSpaceIsSampleSpace)
    {
        //
        // SetSourceClipXSpace must be called after SetBitmapToXSpaceTransform.
        // The WorldToSampleSpace matrix should not have changed between these
        // calls.
        //
        DBGAssertWorldToSampleSpaceHasNotChanged(
            // This may not actually be a transform to device space.  It may
            // transform to IdealSampling, which is an approximation of device
            // space for the purposes of realizations.
            CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device>::ReinterpretBase
            (CBaseMatrix::ReinterpretBase
             (pmatDBGWorldToSampleSpace))
            );
    }
#endif

    if (fUseSourceClip)
    {
        m_fSourceClipIsEntireSource = fSourceClipIsEntireSource;

        m_sourceClipInSamplingSpace.Set(
            *pSourceClipXSpace,
            NULL
            );
    }

    m_fUseSourceClip = fUseSourceClip;    

    UpdateUniqueCount();

    RRETURN(hr);
}

STDMETHODIMP_(void)
CMILBrushBitmap::GetUniquenessToken(
    __out_ecount(1) UINT *puToken
    ) const
{
    *puToken = GetUniqueCount();
}

/*=========================================================================*\
    CMILBrushBitmapLocalSetterWrapper - 
    Wrapper for doing temporary local changes to a CMILBrushBitmap
\*=========================================================================*/

void
CMILBrushBitmapLocalSetterWrapper::Initialize(
    __in_ecount(1) CMILBrushBitmap *pBrushBitmap,
    __in_ecount(1) IWGXBitmapSource *pTexture,
    MilBitmapWrapMode::Enum wrapMode,
    __in_ecount(1) const CBaseMatrix *pmatBitmapToXSpace,
    XSpaceDefinition xSpaceDefinition
    DBG_COMMA_PARAM(__in_ecount_opt(1) const CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> *pmatDBGWorldToSampleSpace)
    )
{
    m_pBrushBitmap = pBrushBitmap;
    
    m_pBrushBitmap->UpdateUniqueCount();
    m_pBrushBitmap->m_pTexture = pTexture;
    m_pBrushBitmap->m_WrapMode = wrapMode;
    if (xSpaceDefinition == XSpaceIsSampleSpace)
    {
        m_pBrushBitmap->m_matBitmapToSamplingSpace = *
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device>::ReinterpretBase
            (pmatBitmapToXSpace);
    }
    else
    {
        Assert(xSpaceDefinition == XSpaceIsWorldSpace);
        m_pBrushBitmap->m_matBitmapToSamplingSpace = *
            CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling>::ReinterpretBase
            (pmatBitmapToXSpace);
    }
    m_pBrushBitmap->m_xSamplingSpaceDefinition = xSpaceDefinition;

#if DBG
    if (pmatDBGWorldToSampleSpace)
    {
        m_pBrushBitmap->m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace = *pmatDBGWorldToSampleSpace;
    }
    else
    {
        m_pBrushBitmap->m_matDBGWorldToSampleSpaceWhenSetBitmapToSampleSpace.SetToIdentity();
    }
#endif
}

CMILBrushBitmapLocalSetterWrapper::~CMILBrushBitmapLocalSetterWrapper()
{
    m_pBrushBitmap->UpdateUniqueCount();
    m_pBrushBitmap->m_pTexture = NULL;
}

/*=========================================================================*\
    Support methods
\*=========================================================================*/

HRESULT CMILBrushBitmap::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    //
    // CMILBrushBitmap never need to be QI'ed. The subclass,
    // CMILTestBrushBitmap does need to be QI'd though
    //

    RIP("CMILBrushBitmap is not allowed to be QI'ed.");
    RRETURN(E_NOINTERFACE);
}

/*=========================================================================*\
    CMILBrushLinearGradient - MIL Linear Gradient Brush Object
\*=========================================================================*/

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::CMILBrushLinearGradient
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CMILBrushLinearGradient::CMILBrushLinearGradient(__in_ecount_opt(1) CMILFactory *pFactory)
    : CMILBrushGradient(pFactory)
{
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::~CMILBrushLinearGradient
//
//  Synopsis:   dctor
//
//-------------------------------------------------------------------------
CMILBrushLinearGradient::~CMILBrushLinearGradient()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::Create
//
//  Synopsis:   Creation method
//
//-------------------------------------------------------------------------
HRESULT CMILBrushLinearGradient::Create(
    CMILFactory *pFactory,
    CMILBrushLinearGradient **ppLinearGradientBrush
    )
{
    HRESULT hr = S_OK;

    // Create brush
    CMILBrushLinearGradient *pTempBrush = new CMILBrushLinearGradient(pFactory);
    if (pTempBrush == NULL)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        pTempBrush->AddRef();
    }

    // Return brush via out-param
    if (SUCCEEDED(hr))
    {
        *ppLinearGradientBrush = pTempBrush;
        pTempBrush = NULL;
    }

    // Release brush if it's non-NULL
    if (pTempBrush != NULL)
    {
        pTempBrush->Release();
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::SetLinePoints
//
//  Synopsis:   This function will compute the direction point automatically
//              For linear gradients, this function may be easier to use than
//              SetEndPoints.
//
//-------------------------------------------------------------------------
void
CMILBrushLinearGradient::SetLinePoints(
    __in_ecount(1) const MilPoint2F *pptBeginPoint,
    __in_ecount(1) const MilPoint2F *pptEndPoint
    )
{

    // (ptDirPoint-pptBeginPoint) is perpendicular to (pptEndPoint-pptBeginPoint)

    MilPoint2F ptDirPoint =  {
        -(pptEndPoint->Y - pptBeginPoint->Y) + pptBeginPoint->X,
        (pptEndPoint->X - pptBeginPoint->X) + pptBeginPoint->Y
    };

    CMILBrushGradient::SetEndPoints(pptBeginPoint, pptEndPoint, &ptDirPoint);
}


//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::HrFindInterface
//
//  Synopsis:   HrFindInterface implementation
//
//-------------------------------------------------------------------------
HRESULT
CMILBrushLinearGradient::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        // Call our base object's HrFindInterface
        hr = CMILObject::HrFindInterface(riid, ppvObject);
    }

    return hr;
}

/*=========================================================================*\
    CMILBrushRadialGradient - MIL Radial Gradient Brush Object
\*=========================================================================*/

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushRadialGradient::CMILBrushRadialGradient
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CMILBrushRadialGradient::CMILBrushRadialGradient(__in_ecount_opt(1) CMILFactory *pFactory)
    : CMILBrushGradient(pFactory)
{
    m_fHasSeparateOriginFromCenter = FALSE;
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushRadialGradient::~CMILBrushRadialGradient
//
//  Synopsis:   dctor
//
//-------------------------------------------------------------------------
CMILBrushRadialGradient::~CMILBrushRadialGradient()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushRadialGradient::Create
//
//  Synopsis:   Creation method
//
//-------------------------------------------------------------------------
HRESULT CMILBrushRadialGradient::Create(
    CMILFactory *pFactory,
    CMILBrushRadialGradient **ppRadialGradientBrush
    )
{
    HRESULT hr = S_OK;

    // Create brush
    CMILBrushRadialGradient *pTempBrush = new CMILBrushRadialGradient(pFactory);
    if (pTempBrush == NULL)
    {
        MIL_THR(E_OUTOFMEMORY);
    }
    else
    {
        pTempBrush->AddRef();
    }

    // Return brush via out-param
    if (SUCCEEDED(hr))
    {
        *ppRadialGradientBrush = pTempBrush;
        pTempBrush = NULL;
    }

    // Release brush if it's non-NULL
    if (pTempBrush != NULL)
    {
        pTempBrush->Release();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushRadialGradient::SetGradientOrigin
//
//  Synopsis:   Sets the origin of the radial gradient. If this function has
//              not been called, the origin is at the center of the ellipse. After
//              it is called, the origin may be at a different location than
//              the center.
//
//-------------------------------------------------------------------------
void
CMILBrushRadialGradient::SetGradientOrigin(
    BOOL fHasSeparateOriginFromCenter,
    __in_ecount(1) const MilPoint2F *pptGradientOrigin
    )
{
    if (fHasSeparateOriginFromCenter)
    {
        Assert(pptGradientOrigin);
        m_ptGradientOrigin = *pptGradientOrigin;
        m_fHasSeparateOriginFromCenter = TRUE;
    }
    else
    {
        m_fHasSeparateOriginFromCenter = FALSE;
    }
}

const MilPoint2F &
CMILBrushRadialGradient::GetGradientOrigin() const
{
    if (HasSeparateOriginFromCenter())
    {
        // Return gradient origin if it is different from the center
        return m_ptGradientOrigin; 
    }
    else
    {
        // If the gradient origin is the same as the center, we didn't
        // overwrite m_ptGradientOrigin during SetGradientOrigin, so
        // we should return the center.
        return m_ptStartPointOrCenter;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CMILBrushLinearGradient::HrFindInterface
//
//  Synopsis:   HrFindInterface implementation
//
//-------------------------------------------------------------------------
HRESULT
CMILBrushRadialGradient::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        // Call our base object's HrFindInterface
        hr = CMILObject::HrFindInterface(riid, ppvObject);
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::CMILBrushShaderEffect
//
//-------------------------------------------------------------------------

CMILBrushShaderEffect::CMILBrushShaderEffect(
    __in CMilEffectDuce *pShaderEffect
    )
    : CMILObject(NULL)
{
    m_pShaderEffectWeakRef = pShaderEffect;
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::~CMILBrushShaderEffect
//
//-------------------------------------------------------------------------

CMILBrushShaderEffect::~CMILBrushShaderEffect()
{
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::Create
//
//-------------------------------------------------------------------------

HRESULT
CMILBrushShaderEffect::Create(
    __in CMilEffectDuce *pShaderEffect,
    __deref_out CMILBrushShaderEffect **ppShaderEffectBrush
    )
{
    HRESULT hr = S_OK;

    CMILBrushShaderEffect *pBrushEffect = new CMILBrushShaderEffect(pShaderEffect);
    IFCOOM(pBrushEffect);

    pBrushEffect->AddRef();
    
    *ppShaderEffectBrush = pBrushEffect;
    pBrushEffect = NULL;

Cleanup:
    ReleaseInterface(pBrushEffect);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::ConfigurePass
//
//  Configures the brush with the specified shader and inputs. 
//-------------------------------------------------------------------------

HRESULT 
CMILBrushShaderEffect::ConfigurePass(
    __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matBitmapToBaseSamplingSpace
    )
{
    m_matBitmapToBaseSamplingSpace = matBitmapToBaseSamplingSpace;

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::GetBitmapToSampleSpaceTransform
//
//  Combines the local to device space transform with the texture to local
//  transform.
//-------------------------------------------------------------------------

void 
CMILBrushShaderEffect::GetBitmapToSampleSpaceTransform(
    __in const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matBaseSamplingToSampleSpace, // Composition: local to device.
    __out CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pMatBitmapToBaseSamplingSpace) // Composition: texture to local
{
    pMatBitmapToBaseSamplingSpace->SetToMultiplyResult(m_matBitmapToBaseSamplingSpace, matBaseSamplingToSampleSpace);
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::HrFindInterface
//
//-------------------------------------------------------------------------

HRESULT 
CMILBrushShaderEffect::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject)
{
    HRESULT hr = S_OK;

    if (ppvObject == NULL)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        // Call our base object's HrFindInterface
        hr = CMILObject::HrFindInterface(riid, ppvObject);
    }
    return hr;
}

//+------------------------------------------------------------------------
//
//  CMILBrushShaderEffect::PreparePass
//
//-------------------------------------------------------------------------

HRESULT 
CMILBrushShaderEffect::PreparePass(
    __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
    __inout CPixelShaderState *pPixelShaderState, 
    __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
    )
{
    RRETURN(m_pShaderEffectWeakRef->PrepareSoftwarePass(pRealizationSamplingToDevice, pPixelShaderState, ppPixelShaderCompiler));
}




